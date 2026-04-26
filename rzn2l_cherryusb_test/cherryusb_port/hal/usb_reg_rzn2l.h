/**
 * @file usb_reg_rzn2l.h
 * @brief RZ/N2L USB寄存器定义 - 对齐FSP
 */

#ifndef _USB_REG_RZN2L_H_
#define _USB_REG_RZN2L_H_

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define R_USBF_BASE                (0x80201000UL)

typedef volatile uint16_t* usb_reg16_t;
typedef volatile uint32_t* usb_reg32_t;

#define R_USBF                     ((R_USBF_Type *)R_USBF_BASE)

typedef struct {
    volatile uint16_t SYSCFG0;          /* 0x00 */
    volatile uint16_t SYSCFG1;          /* 0x02 */
    volatile uint16_t SYSSTS0;          /* 0x04 */
    volatile uint16_t RESERVED0;        /* 0x06 */
    volatile uint16_t DVSTCTR0;         /* 0x08 */
    volatile uint16_t RESERVED1;        /* 0x0A */
    volatile uint16_t TESTMODE;         /* 0x0C */
    volatile uint16_t RESERVED2;        /* 0x0E */
    volatile uint32_t RESERVED3;        /* 0x10 */
    volatile uint32_t CFIFO;            /* 0x14 - FIFO data */
    volatile uint32_t D0FIFO;           /* 0x18 - FIFO data */
    volatile uint32_t D1FIFO;           /* 0x1C - FIFO data */
    volatile uint16_t CFIFOSEL;         /* 0x20 */
    volatile uint16_t CFIFOCTR;         /* 0x22 */
    volatile uint32_t RESERVED7;        /* 0x24 */
    volatile uint16_t D0FIFOSEL;        /* 0x28 */
    volatile uint16_t D0FIFOCTR;        /* 0x2A */
    volatile uint16_t D1FIFOSEL;        /* 0x2C */
    volatile uint16_t D1FIFOCTR;        /* 0x2E */
    volatile uint16_t INTENB0;          /* 0x30 */
    volatile uint16_t INTENB1;          /* 0x32 */
    volatile uint16_t RESERVED8;        /* 0x34 */
    volatile uint16_t BRDYENB;          /* 0x36 */
    volatile uint16_t NRDYENB;          /* 0x38 */
    volatile uint16_t BEMPENB;          /* 0x3A */
    volatile uint16_t SOFCFG;           /* 0x3C */
    volatile uint16_t RESERVED9;        /* 0x3E */
    volatile uint16_t INTSTS0;          /* 0x40 */
    volatile uint16_t INTSTS1;          /* 0x42 */
    volatile uint16_t RESERVED10;       /* 0x44 */
    volatile uint16_t BRDYSTS;          /* 0x46 - per-pipe BRDY status */
    volatile uint16_t NRDYSTS;          /* 0x48 - per-pipe NRDY status */
    volatile uint16_t BEMPSTS;          /* 0x4A - per-pipe BEMP status */
    volatile uint16_t FRMNUM;           /* 0x4C */
    volatile uint16_t UFRMNUM;          /* 0x4E */
    volatile uint16_t USBADDR;          /* 0x50 */
    volatile uint16_t RESERVED11;       /* 0x52 */
    volatile uint16_t USBREQ;           /* 0x54 */
    volatile uint16_t USBVAL;           /* 0x56 */
    volatile uint16_t USBINDX;          /* 0x58 */
    volatile uint16_t USBLENG;          /* 0x5A */
    volatile uint16_t DCPCFG;           /* 0x5C */
    volatile uint16_t DCPMAXP;          /* 0x5E */
    volatile uint16_t DCPCTR;           /* 0x60 */
    volatile uint16_t RESERVED12;       /* 0x62 */
    volatile uint16_t PIPESEL;          /* 0x64 */
    volatile uint16_t RESERVED13;       /* 0x66 */
    volatile uint16_t PIPECFG;          /* 0x68 */
    volatile uint16_t PIPEBUF;          /* 0x6A */
    volatile uint16_t PIPEMAXP;         /* 0x6C */
    volatile uint16_t PIPEPERI;         /* 0x6E */
    volatile uint16_t PIPE1CTR;         /* 0x70 */
    volatile uint16_t PIPE2CTR;         /* 0x72 */
    volatile uint16_t PIPE3CTR;         /* 0x74 */
    volatile uint16_t PIPE4CTR;         /* 0x76 */
    volatile uint16_t PIPE5CTR;         /* 0x78 */
    volatile uint16_t PIPE6CTR;         /* 0x7A */
    volatile uint16_t PIPE7CTR;         /* 0x7C */
    volatile uint16_t PIPE8CTR;         /* 0x7E */
    volatile uint16_t PIPE9CTR;         /* 0x80 */
    volatile uint16_t RESERVED14;       /* 0x82 */
    volatile uint32_t RESERVED15[3];    /* 0x84 */
    volatile uint32_t RESERVED_TR[5];   /* 0x90 */
    volatile uint32_t RESERVED16[23];   /* 0xA4 */
    volatile uint16_t RESERVED17;       /* 0x100 */
    volatile uint16_t LPSTS;            /* 0x102 - Low Power Status */
} R_USBF_Type;

#define USB0  R_USBF

#define USB_USBE                  (0x0001U)
#define USB_DPRPU                 (0x0010U)
#define USB_DRPD                  (0x0020U)
#define USB_HSE                   (0x0080U)
#define USB_CNEN                  (0x0100U)
#define USB_SCKE                  (0x0400U)

#define USB_BWAIT_MASK            (0x003FU)
#define USB_BWAIT_7               (0x0007U)

#define USB_VBINT                 (0x8000U)
#define USB_RESM                  (0x4000U)
#define USB_SOFR                  (0x2000U)
#define USB_DVST                  (0x1000U)
#define USB_CTRT                  (0x0800U)
#define USB_BEMP                  (0x0400U)
#define USB_NRDY                  (0x0200U)
#define USB_BRDY                  (0x0100U)
#define USB_VBSTS                 (0x0080U)
#define USB_DVSQ                  (0x0070U)
#define USB_VALID                 (0x0008U)
#define USB_CTSQ                  (0x0007U)

#define USB_DS_POWR               (0x0000U)
#define USB_DS_DFLT               (0x0010U)
#define USB_DS_ADDS               (0x0020U)
#define USB_DS_CNFG               (0x0030U)
#define USB_DS_SUSP               (0x0040U)

#define USB_CS_IDST               (0x0000U)
#define USB_CS_RDDS               (0x0001U)
#define USB_CS_WRDS               (0x0002U)
#define USB_CS_RDSS               (0x0003U)
#define USB_CS_WRSS               (0x0004U)
#define USB_CS_WRND               (0x0005U)
#define USB_CS_SQER               (0x0006U)

#define USB_BEMPE                 (0x0400U)
#define USB_BRDYE                 (0x0100U)
#define USB_NRDYE                 (0x0200U)
#define USB_CTRE                  (0x0800U)
#define USB_DVSE                  (0x1000U)
#define USB_SOFE                  (0x2000U)
#define USB_RSME                  (0x4000U)
#define USB_VBSE                  (0x8000U)

#define USB_WKUP                  (0x0100U)
#define USB_USBRST                (0x0040U)
#define USB_UACT                  (0x0010U)
#define USB_RHST_MASK             (0x0007U)
#define USB_UNDECID               (0x0000U)
#define USB_LSMODE                (0x0001U)
#define USB_FSMODE                (0x0002U)
#define USB_HSMODE                (0x0003U)

#define USB_PID_MASK              (0x0003U)
#define USB_PID_NAK               (0x0000U)
#define USB_PID_BUF               (0x0001U)
#define USB_PID_STALL             (0x0002U)
#define USB_CCPL                  (0x0004U)
#define USB_SQCLR                 (0x0100U)

#define USB_TYPE_MASK             (0xC000U)
#define USB_TYPE_OTHER            (0x0000U)
#define USB_TYPE_ISO              (0xC000U)
#define USB_TYPE_BULK             (0x4000U)
#define USB_TYPE_INT              (0x8000U)
#define USB_DIR_H_OUT             (0x0010U)
#define USB_DIR_P_IN              (0x0010U)

#define USB_MBW_32                (0x0800U)
#define USB_MBW_16                (0x0400U)
#define USB_MBW_8                 (0x0000U)

#define USB_DREQE                 (0x1000U)
#define USB_DCLRM                 (0x2000U)

#define USB_CURPIPE_MASK          (0x000FU)

#define USB_BVAL                  (0x8000U)
#define USB_BCLR                  (0x4000U)
#define USB_FRDY                  (0x2000U)
#define USB_DTLN_MASK             (0x0FFFU)

#define USB_PIPE0                 (0U)
#define USB_PIPE1                 (1U)
#define USB_PIPE2                 (2U)
#define USB_PIPE3                 (3U)
#define USB_PIPE4                 (4U)
#define USB_PIPE5                 (5U)
#define USB_PIPE6                 (6U)
#define USB_PIPE7                 (7U)
#define USB_PIPE8                 (8U)
#define USB_PIPE9                 (9U)
#define USB_MAX_PIPE_NO           (9U)

#define USB_BRDY9                 (0x0200U)
#define USB_BRDY8                 (0x0100U)
#define USB_BRDY1                 (0x0002U)
#define USB_BRDY0                 (0x0001U)

#define USB_BEMP9                 (0x0200U)
#define USB_BEMP1                 (0x0002U)
#define USB_BEMP0                 (0x0001U)

#define USB_MAXP                  (0x007FU)
#define USB_MXPS                  (0x07FFU)

#define USB_SUSPM                 (0x4000U)

/* USBHC (USB Host Controller) at 0x80200000 */
#define USBHC_BASE                (0x80200000UL)
#define USBHC_COMMCTRL            (*((volatile uint32_t *)(USBHC_BASE + 0x800)))
#define USBHC_USBCTR              (*((volatile uint32_t *)(USBHC_BASE + 0x20C)))
#define USBHC_COMMCTRL_PERI       (0x80000000UL)
#define USBHC_USBCTR_PLL_RST      (0x00000002UL)

/* SYSC (System Control) at 0x80280000 */
#define SYSC_BASE                 (0x80280000UL)
#define SYSC_MSTPCRE              (*((volatile uint32_t *)(SYSC_BASE + 0x310)))

#define SYSC_MSTPCRE_USB_BIT      (1UL << 8)

/* RWP (Register Write Protection)
 *   Non-Secure: PRCRN at 0x80281A10 (16-bit)
 *   Secure:     PRCRS at 0x81281A00 (32-bit)
 */
#define RWP_NS_PRCRN              (*((volatile uint16_t *)(0x80281A10UL)))
#define RWP_S_PRCRS               (*((volatile uint32_t *)(0x81281A00UL)))
#define RWP_PRCR_KEY              (0xA500U)
#define RWP_PRCR_PRC1             (0x0002U)

static inline void usb_prcr_unlock_lpc_reset(void)
{
    uint16_t tmp_ns = RWP_NS_PRCRN;
    uint32_t tmp_s  = RWP_S_PRCRS;
    RWP_NS_PRCRN = (tmp_ns | RWP_PRCR_KEY | RWP_PRCR_PRC1);
    RWP_S_PRCRS  = (tmp_s  | RWP_PRCR_KEY | RWP_PRCR_PRC1);
    __asm volatile ("dsb sy" : : : "memory");
}

static inline void usb_prcr_lock_lpc_reset(void)
{
    uint16_t tmp_ns = RWP_NS_PRCRN;
    uint32_t tmp_s  = RWP_S_PRCRS;
    RWP_NS_PRCRN = (uint16_t)((tmp_ns | RWP_PRCR_KEY) & ~RWP_PRCR_PRC1);
    RWP_S_PRCRS  = ((tmp_s  | RWP_PRCR_KEY) & ~RWP_PRCR_PRC1);
    __asm volatile ("dsb sy" : : : "memory");
}

static inline uint16_t usb_read_syscfg(void)
{
    return USB0->SYSCFG0;
}

static inline void usb_set_syscfg(uint16_t val)
{
    USB0->SYSCFG0 = val;
}

static inline void usb_clear_syscfg(uint16_t mask)
{
    USB0->SYSCFG0 = (uint16_t)(USB0->SYSCFG0 & ~mask);
}

static inline uint16_t usb_read_intsts(void)
{
    return USB0->INTSTS0;
}

static inline void usb_write_intsts(uint16_t val)
{
    USB0->INTSTS0 = val;
}

static inline void usb_set_intenb(uint16_t val)
{
    USB0->INTENB0 |= val;
}

static inline void usb_clear_intenb(uint16_t mask)
{
    USB0->INTENB0 = (uint16_t)(USB0->INTENB0 & ~mask);
}

static inline uint16_t usb_read_dcpctr(void)
{
    return USB0->DCPCTR;
}

static inline void usb_write_dcpctr(uint16_t val)
{
    USB0->DCPCTR = val;
}

static inline void usb_set_dcpctr(uint16_t val)
{
    USB0->DCPCTR |= val;
}

static inline uint16_t usb_read_usbreq(void)
{
    return USB0->USBREQ;
}

static inline uint16_t usb_read_usbval(void)
{
    return USB0->USBVAL;
}

static inline uint16_t usb_read_usbindx(void)
{
    return USB0->USBINDX;
}

static inline uint16_t usb_read_usbleng(void)
{
    return USB0->USBLENG;
}

static inline uint16_t usb_read_dvstctr(void)
{
    return USB0->DVSTCTR0;
}

static inline void usb_write_dvstctr(uint16_t val)
{
    USB0->DVSTCTR0 = val;
}

static inline void usb_set_dvstctr(uint16_t val)
{
    USB0->DVSTCTR0 |= val;
}

static inline uint16_t usb_read_brdysts(void)
{
    return USB0->BRDYENB;
}

static inline void usb_write_brdysts(uint16_t val)
{
    USB0->BRDYENB = (uint16_t)(~val);
}

static inline uint16_t usb_read_bempsts(void)
{
    return USB0->BEMPENB;
}

static inline void usb_write_bempsts(uint16_t val)
{
    USB0->BEMPENB = (uint16_t)(~val);
}

static inline void usb_set_brdyenb(uint16_t pipe)
{
    USB0->BRDYENB |= (uint16_t)(1U << pipe);
}

static inline void usb_clear_brdyenb(uint16_t pipe)
{
    USB0->BRDYENB = (uint16_t)(USB0->BRDYENB & ~(1U << pipe));
}

static inline void usb_set_bempenb(uint16_t pipe)
{
    USB0->BEMPENB |= (uint16_t)(1U << pipe);
}

static inline void usb_clear_bempenb(uint16_t pipe)
{
    USB0->BEMPENB = (uint16_t)(USB0->BEMPENB & ~(1U << pipe));
}

static inline void usb_set_nrdyenb(uint16_t pipe)
{
    USB0->NRDYENB |= (uint16_t)(1U << pipe);
}

static inline void usb_clear_nrdyenb(uint16_t pipe)
{
    USB0->NRDYENB = (uint16_t)(USB0->NRDYENB & ~(1U << pipe));
}

static inline void usb_write_pipesel(uint16_t pipe)
{
    USB0->PIPESEL = pipe;
}

static inline uint16_t usb_read_pipecfg(void)
{
    return USB0->PIPECFG;
}

static inline void usb_write_pipecfg(uint16_t val)
{
    USB0->PIPECFG = val;
}

static inline void usb_write_pipebuf(uint16_t val)
{
    USB0->PIPEBUF = val;
}

static inline uint16_t usb_read_pipemaxp(void)
{
    return USB0->PIPEMAXP;
}

static inline void usb_write_pipemaxp(uint16_t val)
{
    USB0->PIPEMAXP = val;
}

static inline uint16_t usb_read_pipectr(uint8_t pipe)
{
    if (pipe == 0) {
        return USB0->DCPCTR;
    }
    return *(&USB0->PIPE1CTR + (pipe - 1));
}

static inline void usb_write_pipectr(uint8_t pipe, uint16_t val)
{
    if (pipe == 0) {
        USB0->DCPCTR = val;
    } else {
        *(&USB0->PIPE1CTR + (pipe - 1)) = val;
    }
}

static inline void usb_set_pid(uint8_t pipe, uint16_t pid)
{
    uint16_t ctr = usb_read_pipectr(pipe);
    ctr = (uint16_t)((ctr & ~USB_PID_MASK) | (pid & USB_PID_MASK));
    usb_write_pipectr(pipe, ctr);
}

static inline void usb_clear_pid(uint8_t pipe)
{
    usb_set_pid(pipe, USB_PID_NAK);
}

static inline void usb_set_sqclr(uint8_t pipe)
{
    if (pipe == 0) {
        USB0->DCPCTR |= USB_SQCLR;
    } else {
        *(&USB0->PIPE1CTR + (pipe - 1)) |= USB_SQCLR;
    }
}

static inline void usb_set_ccpl(void)
{
    USB0->DCPCTR |= USB_CCPL;
}

static inline void usb_clear_ccpl(void)
{
    USB0->DCPCTR = (uint16_t)(USB0->DCPCTR & ~USB_CCPL);
}

static inline void usb_write_syscfg1(uint16_t val)
{
    USB0->SYSCFG1 = val;
}

static inline void usb_write_dcpmaxp(uint16_t val)
{
    USB0->DCPMAXP = val;
}

static inline void usb_write_dcpcfg(uint16_t val)
{
    USB0->DCPCFG = val;
}

static inline void usb_write_usbaddr(uint16_t val)
{
    USB0->USBADDR = val;
}

static inline uint16_t usb_read_syssts(void)
{
    return USB0->SYSSTS0;
}

static inline uint16_t usb_read_cfifoctr(void)
{
    return USB0->CFIFOCTR;
}

static inline void usb_write_cfifosel(uint16_t val)
{
    USB0->CFIFOSEL = val;
}

static inline void usb_write_cfifoctr(uint16_t val)
{
    USB0->CFIFOCTR = val;
}

static inline void usb_set_cfifoctr(uint16_t val)
{
    USB0->CFIFOCTR |= val;
}

static inline uint32_t usb_read_cfifo(void)
{
    return USB0->CFIFO;
}

static inline void usb_write_cfifo(uint32_t val)
{
    USB0->CFIFO = val;
}

static inline void usb_write_d0fifosel(uint16_t val)
{
    USB0->D0FIFOSEL = val;
}

static inline uint16_t usb_read_d0fifoctr(void)
{
    return USB0->D0FIFOCTR;
}

static inline void usb_write_d0fifoctr(uint16_t val)
{
    USB0->D0FIFOCTR = val;
}

static inline void usb_set_d0fifoctr(uint16_t val)
{
    USB0->D0FIFOCTR |= val;
}

static inline uint32_t usb_read_d0fifo(void)
{
    return USB0->D0FIFO;
}

static inline void usb_write_d0fifo(uint32_t val)
{
    USB0->D0FIFO = val;
}

static inline void usb_write_d1fifosel(uint16_t val)
{
    USB0->D1FIFOSEL = val;
}

static inline uint16_t usb_read_d1fifoctr(void)
{
    return USB0->D1FIFOCTR;
}

static inline void usb_write_d1fifoctr(uint16_t val)
{
    USB0->D1FIFOCTR = val;
}

static inline uint32_t usb_read_d1fifo(void)
{
    return USB0->D1FIFO;
}

static inline void usb_write_d1fifo(uint32_t val)
{
    USB0->D1FIFO = val;
}

#ifdef __cplusplus
}
#endif

#endif /* _USB_REG_RZN2L_H_ */
