/**
 * @file usb_dc_rzn2l.c
 * @brief RZ/N2L USB Device Controller HAL for CherryUSB
 */

#include "usbd_core.h"
#include "usb_reg_rzn2l.h"
#include "usb_log.h"
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

    R_USBF->INTENB0 = 0x9D00;
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
    R_USBF->DCPCTR = USB_SQCLR | USB_PID_NAK;

    R_USBF->SYSCFG0 = USB_USBE | USB_CNEN;
    
    for (volatile uint32_t w = 0; w < 450000; w++) {}
    // 延长延时，物理上拉/主机等待更稳妥
    for (volatile uint32_t w = 0; w < 3600000; w++) {}

    R_USBF->SYSCFG0 |= USB_DPRPU;
    // 上拉后再多等一等（500000次约2-5ms）
    for (volatile uint32_t w = 0; w < 500000; w++) {}

    R_USBF->INTENB0 = (USB_VBSE | USB_DVSE | USB_CTRE | USB_BRDYE | USB_BEMPE);
    R_USBF->NRDYENB = 0;

    g_usb_dc.speed = 1;

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
    R_USBF->USBADDR = (uint16_t)(addr & 0x7F);
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
        R_USBF->DCPMAXP = max_packet & 0x7F;
        R_USBF->DCPCFG = dir ? 0x01 : 0x00;
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
        uint16_t buf_val = (buf_size << 10) | ((pipe - 1) * 8);
        R_USBF->PIPEBUF = buf_val;
        
        R_USBF->PIPEMAXP = max_packet & USB_MXPS;
        
        R_USBF->PIPEPERI = 0;
        
        *(&R_USBF->PIPE1CTR + (pipe - 1)) = USB_SQCLR;
        
        R_USBF->BRDYENB |= (1U << pipe);
        R_USBF->BEMPENB |= (1U << pipe);
        
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
        *(&R_USBF->PIPE1CTR + (pipe - 1)) = USB_PID_NAK;
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
        volatile uint16_t *pipe_ctr = &R_USBF->PIPE1CTR + (pipe - 1);
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
        volatile uint16_t *pipe_ctr = &R_USBF->PIPE1CTR + (pipe - 1);
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
        ctr = *(&R_USBF->PIPE1CTR + (pipe - 1));
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
    if (data == NULL || data_len == 0) return -1;
    
    uint32_t write_len = (data_len > g_usb_dc.ep[pipe].max_packet) ? g_usb_dc.ep[pipe].max_packet : data_len;
    
    USB_LOG_INFO("USB: TX EP%d, len=%d\r\n", ep_num, write_len);
    
    if (pipe == 0) {
        R_USBF->CFIFOSEL = 0;
        while ((R_USBF->CFIFOCTR & USB_FRDY) == 0) {}
        
        for (uint32_t i = 0; i < write_len; i++) {
            ((volatile uint8_t*)&R_USBF->CFIFO)[i] = data[i];
        }
        
        R_USBF->CFIFOCTR = (uint16_t)(write_len | USB_BVAL);
        g_usb_dc.ep0_tx_len = write_len;
        R_USBF->DCPCTR = (uint16_t)((R_USBF->DCPCTR & ~USB_PID_MASK) | USB_PID_BUF);
    } else {
        R_USBF->D0FIFOSEL = (uint16_t)(pipe | USB_MBW_32 | USB_DREQE);
        while ((R_USBF->D0FIFOCTR & USB_FRDY) == 0) {}
        
        for (uint32_t i = 0; i < write_len; i++) {
            ((volatile uint8_t*)&R_USBF->D0FIFO)[i] = data[i];
        }
        
        R_USBF->D0FIFOCTR = (uint16_t)(write_len | USB_BVAL);
        
        *(&R_USBF->PIPE1CTR + (pipe - 1)) = USB_PID_BUF;
    }
    
    g_usb_dc.ep_tx_busy[pipe] = true;
    
    return 0;
}

int usbd_ep_start_read(uint8_t busid, const uint8_t ep, uint8_t *data, uint32_t data_len)
{
    (void)busid;
    
    uint8_t ep_num = ep & 0x7F;
    uint8_t pipe = (ep_num == 0) ? 0 : ep_num;
    
    if (pipe > USB_MAX_EP || !g_usb_dc.ep[pipe].configured) return -1;
    if (data == NULL || data_len == 0) return -1;
    
    USB_LOG_INFO("USB: RX EP%d prepared, buf=%p, len=%d\r\n", ep_num, data, data_len);
    
    g_usb_dc.rx_buf[pipe] = data;
    g_usb_dc.rx_buf_len[pipe] = data_len;
    g_usb_dc.rx_len[pipe] = 0;
    g_usb_dc.ep_rx_busy[pipe] = true;
    
    if (pipe == 0) {
        R_USBF->DCPCTR = USB_PID_BUF;
    } else {
        *(&R_USBF->PIPE1CTR + (pipe - 1)) = USB_PID_BUF;
    }
    
    return 0;
}

static void usb_read_setup_packet(void)
{
    R_USBF->CFIFOSEL = 0;
    while ((R_USBF->CFIFOCTR & USB_FRDY) == 0) {}
    
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
        R_USBF->CFIFOSEL = 0;
        uint16_t ctr = R_USBF->CFIFOCTR;
        if ((ctr & USB_FRDY) && ((ctr & USB_DTLN_MASK) > 0)) {
            uint16_t dtln = ctr & USB_DTLN_MASK;
            uint32_t len = (max_len < dtln) ? max_len : dtln;
            for (uint32_t i = 0; i < len; i++) {
                data[i] = ((volatile uint8_t*)&R_USBF->CFIFO)[i];
            }
            *actual_len = len;
            R_USBF->CFIFOCTR = USB_BCLR;
        } else {
            *actual_len = 0;
        }
    } else {
        R_USBF->D0FIFOSEL = (uint16_t)(pipe | USB_MBW_32);
        uint16_t ctr = R_USBF->D0FIFOCTR;
        if ((ctr & USB_FRDY) && ((ctr & USB_DTLN_MASK) > 0)) {
            uint16_t dtln = ctr & USB_DTLN_MASK;
            uint32_t len = (max_len < dtln) ? max_len : dtln;
            for (uint32_t i = 0; i < len; i++) {
                data[i] = ((volatile uint8_t*)&R_USBF->D0FIFO)[i];
            }
            *actual_len = len;
            R_USBF->D0FIFOCTR = USB_BCLR;
        } else {
            *actual_len = 0;
        }
    }
}

void USBD_IRQHandler(uint8_t busid)
{
    for (int retry = 0; retry < 8; retry++) {
        uint16_t intsts = R_USBF->INTSTS0;
        uint16_t intenb = R_USBF->INTENB0;
        uint16_t active = intsts & intenb;
        if (!active) break;

        R_USBF->LPSTS = (uint16_t)(R_USBF->LPSTS | USB_SUSPM);

        USB_LOG_INFO("USB IRQ: bus=%d active=0x%04X intsts=0x%04X\n", busid, active, intsts);

        if (active & USB_VBINT) {
            if (intsts & USB_VBSTS) {
                R_USBF->SYSCFG0 |= USB_DPRPU;
                usbd_event_connect_handler(busid);
            } else {
                R_USBF->SYSCFG0 &= ~USB_DPRPU;
                usbd_event_disconnect_handler(busid);
            }
            R_USBF->INTSTS0 = (uint16_t)(~USB_VBINT);
        }

        if (intsts & USB_RESM) {
            R_USBF->INTSTS0 = (uint16_t)(~USB_RESM);
        }

        if (active & USB_DVST) {
            uint16_t dvst = intsts & USB_DVSQ;
            if (dvst == USB_DS_DFLT) {
            uint16_t rhst = R_USBF->DVSTCTR0 & USB_RHST_MASK;
            USB_LOG_INFO("USB IRQ: RESET rhst=%d\r\n", rhst);

            // 完全软复位整个USB（主要接口/PIPE/上拉等，确保主机正常识别）
            R_USBF->INTENB0 = 0;
            R_USBF->SYSCFG0 &= ~USB_DPRPU;
            R_USBF->LPSTS = (uint16_t)(R_USBF->LPSTS | USB_SUSPM);
            R_USBF->DCPCTR = USB_PID_NAK | USB_SQCLR;
            R_USBF->DCPCFG = 0;
            R_USBF->DCPMAXP = 64;
            R_USBF->SYSCFG0 = USB_USBE | USB_CNEN;
            for (volatile uint32_t w = 0; w < 3600000; w++) {}
            R_USBF->SYSCFG0 |= USB_DPRPU;
            for (volatile uint32_t w = 0; w < 500000; w++) {}
            R_USBF->INTENB0 = (USB_VBSE | USB_DVSE | USB_CTRE | USB_BRDYE | USB_BEMPE);

            usbd_event_reset_handler(busid);
            } else if (dvst == USB_DS_ADDS) {
                USB_LOG_INFO("USB IRQ: ADDRESSED\r\n");
            } else if (dvst == USB_DS_CNFG) {
                USB_LOG_INFO("USB IRQ: CONFIGURED\r\n");
            }
            R_USBF->INTSTS0 = (uint16_t)(~USB_DVST);
        }

        if (active & USB_CTRT) {
            uint16_t ctsq = intsts & USB_CTSQ;
            if (ctsq == USB_CS_IDST) {
                usb_read_setup_packet();
                USB_LOG_INFO("USB IRQ: SETUP [%02X %02X %02X %02X %02X %02X %02X %02X]\r\n",
                            g_usb_dc.setup_packet[0], g_usb_dc.setup_packet[1],
                            g_usb_dc.setup_packet[2], g_usb_dc.setup_packet[3],
                            g_usb_dc.setup_packet[4], g_usb_dc.setup_packet[5],
                            g_usb_dc.setup_packet[6], g_usb_dc.setup_packet[7]);
                usbd_event_ep0_setup_complete_handler(busid, g_usb_dc.setup_packet);
            }
            R_USBF->INTSTS0 = (uint16_t)(~USB_CTRT);
        }

        if (active & USB_BEMP) {
            uint16_t bemp_sts = R_USBF->BEMPSTS;
            R_USBF->INTSTS0 = (uint16_t)(~USB_BEMP);
            R_USBF->BEMPSTS = 0;

            if (bemp_sts & USB_BEMP0) {
                g_usb_dc.ep_tx_busy[0] = false;
                if (g_usb_dc.ep0_tx_len < g_usb_dc.ep[0].max_packet) {
                    R_USBF->DCPCTR |= USB_CCPL;
                }
                usbd_event_ep_in_complete_handler(busid, 0x80, g_usb_dc.ep0_tx_len);
            }

            for (uint8_t i = 1; i <= USB_MAX_EP; i++) {
                if ((bemp_sts & (1U << i)) && (R_USBF->BEMPENB & (1U << i))) {
                    g_usb_dc.ep_tx_busy[i] = false;
                    USB_LOG_INFO("USB IRQ: BEMP EP%d TX\r\n", i);
                    usbd_event_ep_in_complete_handler(busid, i | 0x80, g_usb_dc.ep[i].max_packet);
                }
            }
        }

        if (active & USB_BRDY) {
            uint16_t brdy_sts = R_USBF->BRDYSTS;
            R_USBF->INTSTS0 = (uint16_t)(~USB_BRDY);
            R_USBF->BRDYSTS = 0;

            if (brdy_sts & USB_BRDY0) {
                uint16_t actual_len = 0;
                if (g_usb_dc.rx_buf[0] && g_usb_dc.ep_rx_busy[0]) {
                    usb_read_fifo(0, g_usb_dc.rx_buf[0], g_usb_dc.rx_buf_len[0], &actual_len);
                    g_usb_dc.rx_len[0] = actual_len;
                    g_usb_dc.ep_rx_busy[0] = false;
                    USB_LOG_INFO("USB IRQ: BRDY EP0 RX len=%d\r\n", actual_len);
                    usbd_event_ep_out_complete_handler(busid, 0x00, actual_len);
                }
            }

            for (uint8_t i = 1; i <= USB_MAX_EP; i++) {
                if ((brdy_sts & (1U << i)) && (R_USBF->BRDYENB & (1U << i))) {
                    uint16_t actual_len = 0;
                    if (g_usb_dc.rx_buf[i] && g_usb_dc.ep_rx_busy[i]) {
                        usb_read_fifo(i, g_usb_dc.rx_buf[i], g_usb_dc.rx_buf_len[i], &actual_len);
                        g_usb_dc.rx_len[i] = actual_len;
                        g_usb_dc.ep_rx_busy[i] = false;
                        USB_LOG_INFO("USB IRQ: BRDY EP%d RX len=%d\r\n", i, actual_len);
                        usbd_event_ep_out_complete_handler(busid, i, actual_len);
                    }
                }
            }
        }
    }
}
