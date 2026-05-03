/**
 * @file usb_dc_rzn2l.c
 * @brief RZ/N2L USB Device Controller HAL for CherryUSB
 */

#include "usbd_core.h"
#include "usb_reg_rzn2l.h"
#include "usb_log.h"
#include "cherryusb_config.h"
#include <string.h>

#define USB_MAX_EP           9
#define USB_EP0_MAX_PACKET  64
#define USB_CFG_CNTMD        (0x0100U)  /* Continuous transfer mode select for RZN2L */
#define USB_CNTMDFIELD       (0x0100U)

typedef struct {
    uint8_t ep_addr;
    uint16_t max_packet;
    uint8_t ep_type;
    uint8_t pipe_num;
    bool configured;
    bool enabled;
} usbd_ep_state_t;

static struct {
    usbd_ep_state_t ep[USB_MAX_EP + 1];
    uint8_t address;
    uint8_t speed;
    uint8_t configured;
    
    uint8_t ep0_tx_buf[USB_EP0_MAX_PACKET];
    uint32_t ep0_tx_len;
    uint32_t ep_tx_len[USB_MAX_EP + 1];
    
    volatile bool ep_tx_busy[USB_MAX_EP + 1];
    volatile bool ep_rx_busy[USB_MAX_EP + 1];
    
    uint8_t setup_packet[8];
    
    uint8_t *rx_buf[USB_MAX_EP + 1];
    uint32_t rx_len[USB_MAX_EP + 1];
    uint32_t rx_buf_len[USB_MAX_EP + 1];
} g_usb_dc;

static void usb_system_init(void)
{
    volatile uint32_t tmp;

    usb_prcr_unlock_lpc_reset();
    SYSC_MSTPCRE = SYSC_MSTPCRE & 0x0000001FUL;
    tmp = SYSC_MSTPCRE;
    (void)tmp;
    tmp = SYSC_MSTPCRE;
    (void)tmp;
    tmp = SYSC_MSTPCRE;
    (void)tmp;
    usb_prcr_lock_lpc_reset();

    tmp = *((volatile uint32_t *)(USBHC_BASE));
    (void)tmp;

    USBHC_COMMCTRL |= USBHC_COMMCTRL_PERI;

    USBHC_USBCTR &= ~(USBHC_USBCTR_PLL_RST | 0x00000004UL);

    for (volatile uint32_t d = 0; d < 45000; d++) {}

    R_USBF->SYSCFG0 = (uint16_t)(R_USBF->SYSCFG0 & ~USB_DRPD);
    R_USBF->SYSCFG0 = (uint16_t)(R_USBF->SYSCFG0 | USB_USBE);

    R_USBF->LPSTS = USB_SUSPM;

    for (volatile uint32_t d = 0; d < 45000; d++) {}

    R_USBF->INTENB0 = 0;
}

int usb_dc_init(uint8_t busid)
{
    (void)busid;

    USB_LOG_INFO("USB: init start\r\n");

    memset(&g_usb_dc, 0, sizeof(g_usb_dc));

    usb_system_init();

    R_USBF->SYSCFG1 = USB_BWAIT_7;

    R_USBF->CFIFOSEL = USB_MBW_32;
    R_USBF->D0FIFOSEL = USB_MBW_32;
    R_USBF->D1FIFOSEL = USB_MBW_32;

    R_USBF->DCPMAXP = USB_EP0_MAX_PACKET;
    // 初始化时设置为NAK状态，等收到SETUP包后再设为BUF启动数据阶段
    R_USBF->DCPCTR = USB_SQCLR | USB_PID_NAK;

#ifdef CONFIG_USB_HS
    R_USBF->SYSCFG0 = USB_USBE | USB_CNEN | USB_HSE;
#else
    R_USBF->SYSCFG0 = USB_USBE | USB_CNEN;
#endif
    
    for (volatile uint32_t w = 0; w < 450000; w++) {}
    // 延长延时，物理上拉/主机等待更稳妥
    for (volatile uint32_t w = 0; w < 3600000; w++) {}

    R_USBF->SYSCFG0 |= USB_DPRPU;
    // 上拉后再多等一等（500000次约2-5ms）
    for (volatile uint32_t w = 0; w < 500000; w++) {}

    // 先清除所有pending的中断状态
    R_USBF->INTSTS0 = 0;
    R_USBF->BRDYSTS = 0;
    R_USBF->BEMPSTS = 0;
    R_USBF->NRDYSTS = 0;
    
    // 然后使能USB中断，包括控制传输阶段转换中断(CTRE)
    R_USBF->INTENB0 = (USB_VBSE | USB_DVSE | USB_CTRE | USB_BRDYE | USB_BEMPE | USB_NRDYE | USB_RESM);
    R_USBF->NRDYENB = 0;
#ifdef CONFIG_USB_HS
    g_usb_dc.speed = 2; /* USB_SPEED_FULL, will be updated in Reset IRQ */
#else
    g_usb_dc.speed = 1; /* USB_SPEED_FULL, will be updated in Reset IRQ */
#endif

    USB_LOG_INFO("USB: init done, SYSCFG0=0x%04X\r\n", R_USBF->SYSCFG0);

    return 0;
}

int usb_dc_deinit(uint8_t busid)
{
    (void)busid;
    
    R_USBF->INTENB0 = 0;
    R_USBF->SYSCFG0 = 0;
    
    memset(&g_usb_dc, 0, sizeof(g_usb_dc));
    
    return 0;
}

int usbd_set_address(uint8_t busid, const uint8_t addr)
{
    (void)busid;
    (void)addr;
    g_usb_dc.address = addr;
    USB_LOG_INFO("USB: set address=%d\r\n", addr);
    return 0;
}

int usbd_set_remote_wakeup(uint8_t busid)
{
    (void)busid;
    return 0;
}

uint8_t usbd_get_port_speed(uint8_t busid)
{
    (void)busid;
    return g_usb_dc.speed;
}

int usbd_ep_open(uint8_t busid, const struct usb_endpoint_descriptor *ep_desc)
{
    (void)busid;
    
    uint8_t ep_addr = ep_desc->bEndpointAddress;
    uint8_t ep_num = ep_addr & 0x7F;
    uint8_t dir = (ep_addr & 0x80) ? 1 : 0;
    uint8_t ep_type = USB_GET_ENDPOINT_TYPE(ep_desc->bmAttributes);
    uint16_t max_packet = USB_GET_MAXPACKETSIZE(ep_desc->wMaxPacketSize);
    uint8_t pipe;
    
    USB_LOG_INFO("USB: ep_open addr=0x%02X, type=%d, maxpkt=%d\r\n", ep_addr, ep_type, max_packet);
    
    if (ep_num == 0) {
        // DCP管道配置
        R_USBF->DCPMAXP = max_packet & 0x7F;
        R_USBF->DCPCFG = 0;  // 控制端点，双向
        // 确保DCPCTR为BUF状态，准备接收SETUP包
        R_USBF->DCPCTR = (uint16_t)((R_USBF->DCPCTR & ~USB_PID_MASK) | USB_PID_BUF | USB_SQCLR);
        // 使能EP0的BEMP和BRDY中断
        R_USBF->BRDYENB |= (1U << 0);  // pipe 0
        R_USBF->BEMPENB |= (1U << 0);  // pipe 0
        pipe = 0;
    } else {
        pipe = ep_num;
        
        R_USBF->PIPESEL = pipe;
        
        uint16_t cfg = ep_num & 0x0F;
        switch (ep_type) {
            case 0: break;
            case 1: cfg |= USB_TYPE_ISO; break;
            case 2: cfg |= USB_TYPE_BULK | USB_CFG_CNTMD; break;
            case 3: cfg |= USB_TYPE_INT; break;
        }
        if (dir) {
            cfg |= USB_DIR_P_IN;
        }
        
        R_USBF->PIPECFG = cfg;
        
        uint16_t buf_size = ((max_packet + 63) / 64) - 1;
        if (buf_size > 7) buf_size = 7;
        if (buf_size < 0) buf_size = 0;
        /* BUFNMB单位=64字节块, 每个pipe分配(buf_size+1)块 */
        uint16_t buf_val = (buf_size << 10) | ((pipe - 1) * (buf_size + 1));
        R_USBF->PIPEBUF = buf_val;
        
        R_USBF->PIPEMAXP = max_packet & USB_MXPS;
        
        R_USBF->PIPEPERI = 0;
        
        R_USBF->PIPE_CTR[pipe - 1] = USB_SQCLR | USB_PID_BUF;
        
        /* 按方向使能中断：IN只使能BEMP，OUT只使能BRDY */
        if (dir) {
            R_USBF->BEMPENB |= (1U << pipe);
            R_USBF->BEMPSTS = (uint16_t)(~(1U << pipe) & 0x03FF);
        } else {
            R_USBF->BRDYENB |= (1U << pipe);
            R_USBF->BRDYSTS = (uint16_t)(~(1U << pipe) & 0x03FF);
        }
        R_USBF->NRDYENB |= (1U << pipe);
        R_USBF->NRDYSTS = (uint16_t)(~(1U << pipe) & 0x03FF);
        
        USB_LOG_INFO("USB: PIPE%d cfg=0x%04X, buf=0x%04X, maxp=0x%04X\r\n", 
                    pipe, R_USBF->PIPECFG, R_USBF->PIPEBUF, R_USBF->PIPEMAXP);
    }
    
    g_usb_dc.ep[pipe].ep_addr = ep_addr;
    g_usb_dc.ep[pipe].max_packet = max_packet;
    g_usb_dc.ep[pipe].ep_type = ep_type;
    g_usb_dc.ep[pipe].pipe_num = pipe;
    g_usb_dc.ep[pipe].configured = true;
    g_usb_dc.ep[pipe].enabled = true;
    
    return 0;
}

int usbd_ep_close(uint8_t busid, const uint8_t ep)
{
    (void)busid;
    
    uint8_t ep_num = ep & 0x7F;
    uint8_t pipe = (ep_num == 0) ? 0 : ep_num;
    
    if (pipe > USB_MAX_EP) return -1;
    
    if (pipe == 0) {
        R_USBF->DCPCTR = USB_PID_NAK;
    } else {
        R_USBF->PIPE_CTR[pipe - 1] = USB_PID_NAK;
        R_USBF->NRDYENB &= ~(1U << pipe);
        R_USBF->BRDYENB &= ~(1U << pipe);
        R_USBF->BEMPENB &= ~(1U << pipe);
    }
    
    g_usb_dc.ep[pipe].enabled = false;
    g_usb_dc.ep_tx_busy[pipe] = false;
    g_usb_dc.ep_rx_busy[pipe] = false;
    g_usb_dc.rx_buf[pipe] = NULL;
    
    return 0;
}

int usbd_ep_set_stall(uint8_t busid, const uint8_t ep)
{
    (void)busid;
    
    uint8_t ep_num = ep & 0x7F;
    uint8_t pipe = (ep_num == 0) ? 0 : ep_num;
    
    if (pipe > USB_MAX_EP) return -1;
    
    if (pipe == 0) {
        R_USBF->DCPCTR = (uint16_t)((R_USBF->DCPCTR & ~USB_PID_MASK) | USB_PID_STALL);
    } else {
        volatile uint16_t *pipe_ctr = &R_USBF->PIPE_CTR[pipe - 1];
        *pipe_ctr = (uint16_t)((*pipe_ctr & ~USB_PID_MASK) | USB_PID_STALL);
    }
    
    return 0;
}

int usbd_ep_clear_stall(uint8_t busid, const uint8_t ep)
{
    (void)busid;
    
    uint8_t ep_num = ep & 0x7F;
    uint8_t pipe = (ep_num == 0) ? 0 : ep_num;
    
    if (pipe > USB_MAX_EP) return -1;
    
    if (pipe == 0) {
        R_USBF->DCPCTR = (uint16_t)((R_USBF->DCPCTR & ~USB_PID_MASK) | USB_SQCLR | USB_PID_BUF);
    } else {
        volatile uint16_t *pipe_ctr = &R_USBF->PIPE_CTR[pipe - 1];
        *pipe_ctr = (uint16_t)((*pipe_ctr & ~USB_PID_MASK) | USB_SQCLR | USB_PID_BUF);
    }
    
    return 0;
}

int usbd_ep_is_stalled(uint8_t busid, const uint8_t ep, uint8_t *stalled)
{
    (void)busid;
    
    uint8_t ep_num = ep & 0x7F;
    uint8_t pipe = (ep_num == 0) ? 0 : ep_num;
    
    if (pipe > USB_MAX_EP) return -1;
    
    uint16_t ctr;
    if (pipe == 0) {
        ctr = R_USBF->DCPCTR;
    } else {
        ctr = R_USBF->PIPE_CTR[pipe - 1];
    }
    
    *stalled = ((ctr & USB_PID_MASK) == USB_PID_STALL) ? 1 : 0;
    
    return 0;
}

int usbd_ep_start_write(uint8_t busid, const uint8_t ep, const uint8_t *data, uint32_t data_len)
{
    (void)busid;
    
    uint8_t ep_num = ep & 0x7F;
    uint8_t pipe = (ep_num == 0) ? 0 : ep_num;
    
    if (pipe > USB_MAX_EP || !g_usb_dc.ep[pipe].configured) return -1;
    /* 允许data_len==0的零长度包（IN状态阶段使用） */
    if (data_len > 0 && data == NULL) return -1;
    
    uint32_t write_len = (data_len > g_usb_dc.ep[pipe].max_packet) ? g_usb_dc.ep[pipe].max_packet : data_len;
    
    if (pipe == 0) {
        // EP0发送流程 - 严格参考瑞萨官方代码流程
        USB_LOG_INFO("EP0 TX: writing %d bytes, DCPCTR=0x%04X, CTSQ=%d\r\n", 
                     write_len, R_USBF->DCPCTR, R_USBF->INTSTS0 & 0x7);
        
        // 步骤1: 设置CFIFOSEL，选择DCP管道，CPU写入方向，32-bit访问宽度
        // ISEL=1(bit5): CPU写入, CURPIPE=0(bit3:0): DCP管道, MBW=32
        R_USBF->CFIFOSEL = USB_MBW_32 | 0x0020;  // ISEL=1, CURPIPE=0, MBW=32
        
        // 等待选择器设置完成
        volatile uint32_t timeout = 100000;
        while ((R_USBF->CFIFOSEL & 0x002F) != 0x0020) {
            if (--timeout == 0) {
                USB_LOG_ERR("EP0 TX: CFIFOSEL timeout\r\n");
                return -1;
            }
        }
        
        // 步骤2: 清除FIFO缓冲区
        R_USBF->CFIFOCTR = USB_BCLR;
        
        // 步骤3: 等待FRDY=1（FIFO就绪）
        timeout = 100000;
        while (!(R_USBF->CFIFOCTR & USB_FRDY)) {
            if (--timeout == 0) {
                USB_LOG_ERR("EP0 TX: FRDY timeout, CFIFOCTR=0x%04X\r\n", R_USBF->CFIFOCTR);
                return -1;
            }
        }
        
        // 步骤4: 写入数据到FIFO
        // CFIFO是单一FIFO端口寄存器，每次写入时写入FIFO内部，不能使用数组索引
        const uint32_t *src32 = (const uint32_t *)data;
        uint32_t word_len = write_len / 4;
        for (uint32_t i = 0; i < word_len; i++) {
            R_USBF->CFIFO = src32[i];
        }
        
        // 处理剩余字节，按FSP方式切换MBW位宽
        uint32_t remain = write_len % 4;
        if (remain > 0) {
            const uint8_t *src8 = data + (word_len * 4);
            uint16_t cfifosel_base = R_USBF->CFIFOSEL;
            
            if (remain >= 2) {
                R_USBF->CFIFOSEL = (cfifosel_base & ~USB_MBW) | USB_MBW_16;
                *((volatile uint16_t *)&R_USBF->CFIFO) = *((const uint16_t *)src8);
                src8 += 2;
                remain -= 2;
            }
            if (remain == 1) {
                R_USBF->CFIFOSEL = (cfifosel_base & ~USB_MBW) | USB_MBW_8;
                *((volatile uint8_t *)&R_USBF->CFIFO) = *src8;
            }
            
            // 恢复MBW为32-bit
            R_USBF->CFIFOSEL = (cfifosel_base & ~USB_MBW) | USB_MBW_32;
        }
        
        g_usb_dc.ep0_tx_len = write_len;
        
        /* ZLP状态阶段：直接设PID=BUF+CCPL */
        if (write_len == 0) {
            R_USBF->DCPCTR = (uint16_t)((R_USBF->DCPCTR & ~(USB_PID_MASK | USB_CCPL)) | USB_PID_BUF | USB_CCPL);
        } else {
            R_USBF->CFIFOCTR |= USB_BVAL;
            R_USBF->DCPCTR = (uint16_t)((R_USBF->DCPCTR & ~(USB_PID_MASK | USB_CCPL | USB_SQSET | USB_SQCLR)) | USB_PID_BUF);
        }
        
        USB_LOG_INFO("EP0 TX: sent %d bytes, DCPCTR=0x%04X, BVAL set\r\n", 
                      write_len, R_USBF->DCPCTR);
    } else {
        /* RZN2L外设模式只支持CFIFO，关中断保护 */
        uint32_t cpsr;
        __asm volatile ("mrs %0, cpsr" : "=r"(cpsr));
        __asm volatile ("cpsid i");

        /* 先NAK，停止发送 */
        uint16_t pipectr = R_USBF->PIPE_CTR[pipe - 1];
        R_USBF->PIPE_CTR[pipe - 1] = (uint16_t)((pipectr & ~USB_PID_MASK) | USB_PID_NAK);

        /* ISEL=1(写方向), CURPIPE=pipe, MBW=32 */
        R_USBF->CFIFOSEL = (uint16_t)(pipe | USB_MBW_32 | 0x0020);
        volatile uint32_t __timeout = 100000;
        while ((R_USBF->CFIFOSEL & 0x002F) != (uint16_t)(pipe | 0x0020)) {
            if (--__timeout == 0) { __asm volatile ("msr cpsr_c, %0" :: "r"(cpsr)); return -1; }
        }

        volatile uint32_t timeout = 100000;
        while (!(R_USBF->CFIFOCTR & USB_FRDY)) {
            if (--timeout == 0) { __asm volatile ("msr cpsr_c, %0" :: "r"(cpsr)); return -1; }
        }

        R_USBF->CFIFOCTR = USB_BCLR;
        timeout = 100000;
        while (!(R_USBF->CFIFOCTR & USB_FRDY)) {
            if (--timeout == 0) { __asm volatile ("msr cpsr_c, %0" :: "r"(cpsr)); return -1; }
        }

        const uint32_t *src32 = (const uint32_t *)data;
        uint32_t word_len = write_len / 4;
        for (uint32_t i = 0; i < word_len; i++) {
            R_USBF->CFIFO = src32[i];
        }

        uint32_t remain = write_len % 4;
        if (remain > 0) {
            const uint8_t *src8 = data + (word_len * 4);
            uint16_t cfifosel_base = R_USBF->CFIFOSEL;

            if (remain >= 2) {
                R_USBF->CFIFOSEL = (cfifosel_base & ~USB_MBW) | USB_MBW_16;
                *((volatile uint16_t *)&R_USBF->CFIFO) = *((const uint16_t *)src8);
                src8 += 2;
                remain -= 2;
            }
            if (remain == 1) {
                R_USBF->CFIFOSEL = (cfifosel_base & ~USB_MBW) | USB_MBW_8;
                *((volatile uint8_t *)&R_USBF->CFIFO) = *src8;
            }

            R_USBF->CFIFOSEL = (cfifosel_base & ~USB_MBW) | USB_MBW_32;
        }

        R_USBF->CFIFOCTR |= USB_BVAL;

        __asm volatile ("msr cpsr_c, %0" :: "r"(cpsr));

        R_USBF->PIPE_CTR[pipe - 1] = (uint16_t)((R_USBF->PIPE_CTR[pipe - 1] & ~USB_PID_MASK) | USB_PID_BUF);
    }
    
    g_usb_dc.ep_tx_busy[pipe] = true;
    g_usb_dc.ep_tx_len[pipe] = write_len;
    
    return 0;
}

int usbd_ep_start_read(uint8_t busid, const uint8_t ep, uint8_t *data, uint32_t data_len)
{
    (void)busid;
    
    uint8_t ep_num = ep & 0x7F;
    uint8_t pipe = (ep_num == 0) ? 0 : ep_num;
    
    if (pipe > USB_MAX_EP || !g_usb_dc.ep[pipe].configured) return -1;
    /* 允许data==NULL和data_len==0，用于EP0状态阶段（零长度OUT） */
    if (data_len > 0 && data == NULL) return -1;
    
    USB_LOG_INFO("USB: RX EP%d prepared, buf=%p, len=%d\r\n", ep_num, data, data_len);
    
    if (data && data_len > 0) {
        g_usb_dc.rx_buf[pipe] = data;
        g_usb_dc.rx_buf_len[pipe] = data_len;
    } else {
        g_usb_dc.rx_buf[pipe] = NULL;
        g_usb_dc.rx_buf_len[pipe] = 0;
    }
    g_usb_dc.rx_len[pipe] = 0;
    g_usb_dc.ep_rx_busy[pipe] = true;
    
    if (pipe == 0) {
        // EP0 OUT接收前必须清FIFO（参照FSP的usb_pstd_ctrl_write）
        R_USBF->CFIFOSEL = USB_MBW_32;
        R_USBF->CFIFOCTR = USB_BCLR;
        // 只修改PID位，保留CCPL等其他位
        uint16_t dcpctr = R_USBF->DCPCTR;
        R_USBF->DCPCTR = (uint16_t)((dcpctr & ~USB_PID_MASK) | USB_PID_BUF);
    } else {
        uint16_t pipectr = R_USBF->PIPE_CTR[pipe - 1];
        R_USBF->PIPE_CTR[pipe - 1] = (uint16_t)((pipectr & ~USB_PID_MASK) | USB_PID_BUF);
    }
    
    return 0;
}

static void usb_read_setup_packet(void)
{
    // ISEL=0表示读取方向，CURPIPE=0表示DCP管道
    R_USBF->CFIFOSEL = 0x0000;  // ISEL=0, CURPIPE=0
    // 等待FRDY就绪
    volatile uint16_t cfifoctr;
    do {
        cfifoctr = R_USBF->CFIFOCTR;
    } while ((cfifoctr & USB_FRDY) == 0);
    
    uint32_t setup_lo = R_USBF->CFIFO;
    uint32_t setup_hi = R_USBF->CFIFO;
    
    g_usb_dc.setup_packet[0] = (uint8_t)(setup_lo & 0xFF);
    g_usb_dc.setup_packet[1] = (uint8_t)((setup_lo >> 8) & 0xFF);
    g_usb_dc.setup_packet[2] = (uint8_t)((setup_lo >> 16) & 0xFF);
    g_usb_dc.setup_packet[3] = (uint8_t)((setup_lo >> 24) & 0xFF);
    g_usb_dc.setup_packet[4] = (uint8_t)(setup_hi & 0xFF);
    g_usb_dc.setup_packet[5] = (uint8_t)((setup_hi >> 8) & 0xFF);
    g_usb_dc.setup_packet[6] = (uint8_t)((setup_hi >> 16) & 0xFF);
    g_usb_dc.setup_packet[7] = (uint8_t)((setup_hi >> 24) & 0xFF);
    
    USB_LOG_INFO("[usb_read_setup_packet] SETUP=[%02X %02X %02X %02X %02X %02X %02X %02X]\r\n",
        g_usb_dc.setup_packet[0], g_usb_dc.setup_packet[1], g_usb_dc.setup_packet[2], g_usb_dc.setup_packet[3],
        g_usb_dc.setup_packet[4], g_usb_dc.setup_packet[5], g_usb_dc.setup_packet[6], g_usb_dc.setup_packet[7]);
    
    R_USBF->CFIFOCTR = USB_BCLR;
    R_USBF->DCPCTR |= USB_CCPL;
}

static void usb_read_fifo(uint8_t pipe, uint8_t *data, uint32_t max_len, uint16_t *actual_len)
{
    if (pipe == 0) {
        // ISEL=0表示读取方向，CURPIPE=0表示DCP管道，MBW=32
        R_USBF->CFIFOSEL = USB_MBW_32;  // ISEL=0, CURPIPE=0, MBW=32
        uint16_t ctr = R_USBF->CFIFOCTR;
        if ((ctr & USB_FRDY) && ((ctr & USB_DTLN_MASK) > 0)) {
            uint16_t dtln = ctr & USB_DTLN_MASK;
            uint32_t len = (max_len < dtln) ? max_len : dtln;
            // 读取32-bit字，从同一CFIFO端口重复读取
            uint32_t word_len = len / 4;
            for (uint32_t i = 0; i < word_len; i++) {
                *((uint32_t *)(data + i * 4)) = R_USBF->CFIFO;
            }
            uint32_t remain = len % 4;
            if (remain > 0) {
                uint32_t word = R_USBF->CFIFO;
                for (uint32_t i = 0; i < remain; i++) {
                    data[word_len * 4 + i] = (uint8_t)(word >> (i * 8));
                }
            }
            *actual_len = len;
            R_USBF->CFIFOCTR = USB_BCLR;
        } else {
            *actual_len = 0;
        }
    } else {
        /* RZN2L外设模式: 非EP0也用CFIFO, ISEL=0读方向 */
        R_USBF->CFIFOSEL = (uint16_t)(pipe | USB_MBW_32);
        volatile uint32_t __t = 100000;
        while ((R_USBF->CFIFOSEL & 0x000F) != (uint16_t)pipe) {
            if (--__t == 0) break;
        }
        uint16_t ctr = R_USBF->CFIFOCTR;
        if ((ctr & USB_FRDY) && ((ctr & USB_DTLN_MASK) > 0)) {
            uint16_t dtln = ctr & USB_DTLN_MASK;
            uint32_t len = (max_len < dtln) ? max_len : dtln;
            uint32_t word_len = len / 4;
            for (uint32_t i = 0; i < word_len; i++) {
                *((uint32_t *)(data + i * 4)) = R_USBF->CFIFO;
            }
            uint32_t remain = len % 4;
            if (remain > 0) {
                uint32_t word = R_USBF->CFIFO;
                for (uint32_t i = 0; i < remain; i++) {
                    data[word_len * 4 + i] = (uint8_t)(word >> (i * 8));
                }
            }
            *actual_len = len;
            R_USBF->CFIFOCTR = USB_BCLR;
        } else {
            *actual_len = 0;
        }
    }
}

static uint32_t g_irq_total = 0;
static uint32_t g_irq_vbint = 0;
static uint32_t g_irq_dvst = 0;
static uint32_t g_irq_ctrt = 0;
static uint32_t g_irq_bemp = 0;
static uint32_t g_irq_brdy = 0;
static uint32_t g_irq_nrdy = 0;

void USBD_IRQHandler(uint8_t busid)
{
    g_irq_total++;

    for (;;) {
        uint16_t intsts = R_USBF->INTSTS0;
        uint16_t intenb = R_USBF->INTENB0;
        uint16_t active = intsts & intenb;

        if (!active) break;

        R_USBF->LPSTS = (uint16_t)(R_USBF->LPSTS | USB_SUSPM);

    if (active & USB_VBINT) {
        g_irq_vbint++;
        if (intsts & USB_VBSTS) {
            R_USBF->SYSCFG0 |= USB_DPRPU;
            usbd_event_connect_handler(busid);
        } else {
            R_USBF->SYSCFG0 &= ~USB_DPRPU;
            usbd_event_disconnect_handler(busid);
        }
        // 清除VBINT: 写0到bit15，写1到其他位
        R_USBF->INTSTS0 = (uint16_t)(~USB_VBINT);
    }

    if (active & USB_DVST) {
        g_irq_dvst++;
        uint16_t dvst = intsts & USB_DVSQ;
        // 只在状态变化时打印，避免频繁打印
        static uint16_t last_dvst = 0xFF;
        if (dvst != last_dvst) {
            USB_LOG_INFO("USB: DVST irq, DVSQ=%lu\r\n", dvst >> 4);
            last_dvst = dvst;
        }
        if (dvst == USB_DS_DFLT) {
#ifdef CONFIG_USB_HS
            uint16_t rhst = usb_read_dvstctr() & USB_RHST_MASK;
            if (rhst == USB_HSMODE) {
                g_usb_dc.speed = 3;
                USB_LOG_INFO("USB: Reset, HS negotiated\r\n");
            } else if (rhst == USB_FSMODE) {
                g_usb_dc.speed = 2;
                USB_LOG_INFO("USB: Reset, FS negotiated\r\n");
            } else {
                g_usb_dc.speed = 2;
                USB_LOG_INFO("USB: Reset, RHST=%u, assume FS\r\n", rhst);
            }
#else
            USB_LOG_INFO("USB: Reset detected, reinit EP0\r\n");
#endif
            // 完全重置DCPCTR，清除所有残留状态位（BSTS、CCPL等）
            R_USBF->DCPCTR = (USB_SQCLR | USB_PID_BUF);
            R_USBF->DCPCFG = 0;  // 控制端点，双向
            R_USBF->DCPMAXP = USB_EP0_MAX_PACKET;
            R_USBF->BEMPENB |= (1U << 0);
            R_USBF->BRDYENB |= (1U << 0);
            usbd_event_reset_handler(busid);
        } else if (dvst == USB_DS_SUSP) {
            USB_LOG_INFO("USB: Suspend detected\r\n");
            R_USBF->INTENB0 |= USB_RSME;  // 使能Resume中断
            usbd_event_suspend_handler(busid);
        } else if (dvst == USB_DS_CNFG) {
            USB_LOG_INFO("USB: Configured state\r\n");
        }
        // 清除DVST: 写0到bit12，写1到其他位
        R_USBF->INTSTS0 = (uint16_t)(~USB_DVST);
    }
    
    // Resume中断处理
    if (active & USB_RESM) {
        USB_LOG_INFO("USB: Resume detected\r\n");
        R_USBF->INTENB0 &= ~USB_RSME;  // 禁止Resume中断
        // 清除RESM: 写0到bit14，写1到其他位
        R_USBF->INTSTS0 = (uint16_t)(~USB_RESM);
        usbd_event_resume_handler(busid);
    }

    if (active & USB_CTRT) {
        g_irq_ctrt++;
        uint16_t ctsq = intsts & USB_CTSQ;
        
        // 当VALID位被设置时，表示SETUP包已接收
        if (intsts & USB_VALID) {
            // 步骤1: 读取SETUP包
            uint16_t req = R_USBF->USBREQ;
            uint16_t val = R_USBF->USBVAL;
            uint16_t idx = R_USBF->USBINDX;
            uint16_t len = R_USBF->USBLENG;
            
            // 组装8字节SETUP包
            g_usb_dc.setup_packet[0] = (uint8_t)(req & 0xFF);
            g_usb_dc.setup_packet[1] = (uint8_t)(req >> 8);
            g_usb_dc.setup_packet[2] = (uint8_t)(val & 0xFF);
            g_usb_dc.setup_packet[3] = (uint8_t)(val >> 8);
            g_usb_dc.setup_packet[4] = (uint8_t)(idx & 0xFF);
            g_usb_dc.setup_packet[5] = (uint8_t)(idx >> 8);
            g_usb_dc.setup_packet[6] = (uint8_t)(len & 0xFF);
            g_usb_dc.setup_packet[7] = (uint8_t)(len >> 8);
            
            USB_LOG_DBG("CTRT: SETUP received, CTSQ=%d\r\n", ctsq);
            
            // 步骤2: 关键！先清除VALID位，然后才能修改PID位
            // 根据RZN2L手册：PID位在VALID=1时不能被修改
            R_USBF->INTSTS0 = (uint16_t)(~(USB_CTRT | USB_VALID));
            
            // 步骤3: 如果上一个控制传输的状态阶段未完成(CTSQ为状态阶段)，
            // 先设置CCPL完成上一个传输，再处理新SETUP
            if (ctsq == USB_CS_WRND || ctsq == USB_CS_WRSS || ctsq == USB_CS_RDSS) {
                R_USBF->DCPCTR |= USB_CCPL;
                USB_LOG_DBG("CTRT: CCPL set for pending status stage CTSQ=%d\r\n", ctsq);
            }
            
            // 步骤4: 清除BEMP状态
            R_USBF->BEMPSTS = (uint16_t)(~USB_BEMP0 & 0x03FF);
            
            // 步骤4: 调用CherryUSB处理（会调用usbd_ep_start_write发送描述符）
            usbd_event_ep0_setup_complete_handler(busid, g_usb_dc.setup_packet);
            
            // CCPL在数据传输完成/状态阶段才设置，不在此处设置
            USB_LOG_DBG("CTRT: VALID cleared, DCPCTR=0x%04X\r\n", R_USBF->DCPCTR);
        } else {
            // 处理其他CTSQ状态（非SETUP阶段）
            USB_LOG_DBG("CTRT: CTSQ=%d, DCPCTR=0x%04X\r\n", ctsq, R_USBF->DCPCTR);
            if (ctsq == USB_CS_RDDS || ctsq == USB_CS_WRDS) {
                // 数据阶段，设置PID=BUF（使用读-修改-写）
                uint16_t dcpctr = R_USBF->DCPCTR;
                if ((dcpctr & USB_PID_MASK) != USB_PID_BUF) {
                    R_USBF->DCPCTR = (uint16_t)((dcpctr & ~USB_PID_MASK) | USB_PID_BUF);
                    USB_LOG_DBG("CTRT: Set PID=BUF, new DCPCTR=0x%04X\r\n", R_USBF->DCPCTR);
                }
            } else if (ctsq == USB_CS_RDSS) {
                // 读状态阶段，只设PID=BUF；CCPL在BEMP短包时设置
                USB_LOG_DBG("CTRT: RDSS, setting PID=BUF\r\n");
                R_USBF->DCPCTR = (uint16_t)((R_USBF->DCPCTR & ~USB_PID_MASK) | USB_PID_BUF);
            } else if (ctsq == USB_CS_WRND) {
                // 无数据写状态（如SET_ADDRESS），设置CCPL完成
                USB_LOG_DBG("CTRT: No-data status stage, setting CCPL\r\n");
                R_USBF->DCPCTR |= USB_CCPL;
            } else if (ctsq == USB_CS_WRSS) {
                // 写状态阶段（IN方向），设置CCPL完成
                USB_LOG_DBG("CTRT: Write status stage, setting CCPL\r\n");
                R_USBF->DCPCTR |= USB_CCPL;
            } else if (ctsq == USB_CS_SQER) {
                // 序列错误，重新初始化EP0
                USB_LOG_ERR("CTRT: Sequence error!\r\n");
        R_USBF->DCPCTR = (uint16_t)((R_USBF->DCPCTR & ~(USB_PID_MASK | USB_CCPL | USB_SQSET)) | USB_SQCLR | USB_PID_BUF);
            }
            
            // 清除CTRT中断标志（写0清除，写1保持不变）
            R_USBF->INTSTS0 = (uint16_t)(~USB_CTRT);
            USB_LOG_DBG("CTRT: INTSTS0 after clear=0x%04X, CTSQ=%d\r\n", R_USBF->INTSTS0, R_USBF->INTSTS0 & 0x7);
        }
    }

    if (active & USB_BEMP) {
        g_irq_bemp++;
        uint16_t bemp_sts = R_USBF->BEMPSTS;
        // 清除BEMP状态：写0清除（瑞萨的方式）
        R_USBF->BEMPSTS = (uint16_t)(~bemp_sts & 0x03FF);  // 只保留bit9:0

        if (bemp_sts & USB_BEMP0) {
            g_usb_dc.ep_tx_busy[0] = false;
            
            // 短包/ZLP时设置CCPL，硬件自动处理状态阶段
            if (g_usb_dc.ep0_tx_len < g_usb_dc.ep[0].max_packet) {
                R_USBF->DCPCTR |= USB_CCPL;
                USB_LOG_DBG("BEMP EP0: CCPL set, DCPCTR=0x%04X\r\n", R_USBF->DCPCTR);
            }
            
            usbd_event_ep_in_complete_handler(busid, 0x80, g_usb_dc.ep0_tx_len);
        }

        for (uint8_t i = 1; i <= USB_MAX_EP; i++) {
            if ((bemp_sts & (1U << i)) && (R_USBF->BEMPENB & (1U << i))) {
                usbd_event_ep_in_complete_handler(busid, g_usb_dc.ep[i].ep_addr, g_usb_dc.ep_tx_len[i]);
                g_usb_dc.ep_tx_busy[i] = false;
            }
        }
        // 清除INTSTS0的BEMP位
        R_USBF->INTSTS0 = (uint16_t)(~USB_BEMP);
    }

    if (active & USB_BRDY) {
        g_irq_brdy++;
        uint16_t brdy_sts = R_USBF->BRDYSTS;

        if (brdy_sts & USB_BRDY0) {
            uint16_t actual_len = 0;
            if (g_usb_dc.rx_buf[0] && g_usb_dc.ep_rx_busy[0]) {
                usb_read_fifo(0, g_usb_dc.rx_buf[0], g_usb_dc.rx_buf_len[0], &actual_len);
                g_usb_dc.rx_len[0] = actual_len;
            }
            g_usb_dc.ep_rx_busy[0] = false;
            R_USBF->BRDYSTS = (uint16_t)(~USB_BRDY0 & 0x03FF);
            usbd_event_ep_out_complete_handler(busid, 0x00, actual_len);
        }

        for (uint8_t i = 1; i <= USB_MAX_EP; i++) {
            if ((brdy_sts & (1U << i)) && (R_USBF->BRDYENB & (1U << i))) {
                uint16_t actual_len = 0;
                if (g_usb_dc.rx_buf[i] && g_usb_dc.ep_rx_busy[i]) {
                    usb_read_fifo(i, g_usb_dc.rx_buf[i], g_usb_dc.rx_buf_len[i], &actual_len);
                    g_usb_dc.rx_len[i] = actual_len;
                    g_usb_dc.ep_rx_busy[i] = false;
                    R_USBF->BRDYSTS = (uint16_t)(~(1U << i) & 0x03FF);
                    usbd_event_ep_out_complete_handler(busid, (uint8_t)i, actual_len);
                } else {
                    /* 未准备接收缓冲区，清空FIFO防卡死 */
                    R_USBF->CFIFOSEL = (uint16_t)(i | USB_MBW_32);
                    volatile uint32_t __t = 100000;
                    while ((R_USBF->CFIFOSEL & 0x000F) != (uint16_t)i) { if (--__t == 0) break; }
                    R_USBF->CFIFOCTR = USB_BCLR;
                    R_USBF->BRDYSTS = (uint16_t)(~(1U << i) & 0x03FF);
                }
            }
        }
        R_USBF->INTSTS0 = (uint16_t)(~USB_BRDY);
    }

    if (active & USB_NRDY) {
        g_irq_nrdy++;
        uint16_t nrdy_sts = R_USBF->NRDYSTS;
        for (uint8_t i = 1; i <= USB_MAX_EP; i++) {
            if ((nrdy_sts & (1U << i)) && (R_USBF->NRDYENB & (1U << i))) {
                /* 恢复PID=BUF，让硬件重试 */
                uint16_t ctr = R_USBF->PIPE_CTR[i - 1];
                R_USBF->PIPE_CTR[i - 1] = (uint16_t)((ctr & ~USB_PID_MASK) | USB_PID_BUF);
            }
        }
        R_USBF->NRDYSTS = (uint16_t)(~nrdy_sts & 0x03FF);
        R_USBF->INTSTS0 = (uint16_t)(~USB_NRDY);
    }
    }
}

void usb_print_irq_stats(void)
{
    USB_LOG_INFO("IRQ: total=%lu VBINT=%lu DVST=%lu CTRT=%lu BEMP=%lu BRDY=%lu NRDY=%lu\r\n",
           g_irq_total, g_irq_vbint, g_irq_dvst, g_irq_ctrt, g_irq_bemp, g_irq_brdy, g_irq_nrdy);
    
    // 打印关键寄存器
    uint16_t intsts0 = R_USBF->INTSTS0;
    uint16_t intenb0 = R_USBF->INTENB0;
    uint16_t dcpctr = R_USBF->DCPCTR;
    uint16_t bempenb = R_USBF->BEMPENB;
    uint16_t brdyenb = R_USBF->BRDYENB;
    uint16_t bempsts = R_USBF->BEMPSTS;
    uint16_t brdysts = R_USBF->BRDYSTS;
    
    USB_LOG_INFO("INTSTS0=0x%04X INTENB0=0x%04X DCPCTR=0x%04X\r\n", intsts0, intenb0, dcpctr);
    USB_LOG_INFO("BEMPENB=0x%04X BRDYENB=0x%04X BEMPSTS=0x%04X BRDYSTS=0x%04X\r\n", bempenb, brdyenb, bempsts, brdysts);
    USB_LOG_INFO("DVSQ=%d CTSQ=%d VALID=%d PID=%d\r\n",
           (intsts0 >> 4) & 0x7, intsts0 & 0x7, (intsts0 >> 3) & 1, dcpctr & 0x3);
}
