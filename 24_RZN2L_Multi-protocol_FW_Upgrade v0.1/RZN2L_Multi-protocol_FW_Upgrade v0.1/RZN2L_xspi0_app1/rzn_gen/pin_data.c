/* generated pin source file - do not edit */
#include "bsp_api.h"
#include "r_ioport_api.h"
#include "r_ioport.h"

extern const ioport_extend_cfg_t g_ioport_cfg_extend;

const ioport_pin_cfg_t g_bsp_pin_cfg_data[] =
{
{ .pin = BSP_IO_PORT_02_PIN_4, .pin_cfg = ((uint32_t) IOPORT_CFG_DRIVE_HIGH | (uint32_t) IOPORT_CFG_PORT_PERI
        | (uint32_t) IOPORT_CFG_SLEW_RATE_FAST | (uint32_t) IOPORT_PIN_P024_PFC_00_TDO) },
  { .pin = BSP_IO_PORT_02_PIN_5, .pin_cfg = ((uint32_t) IOPORT_CFG_PORT_PERI | (uint32_t) IOPORT_PIN_P025_PFC_01_TDI) },
  { .pin = BSP_IO_PORT_02_PIN_6, .pin_cfg = ((uint32_t) IOPORT_CFG_DRIVE_HIGH | (uint32_t) IOPORT_CFG_PORT_PERI
          | (uint32_t) IOPORT_CFG_SLEW_RATE_FAST | (uint32_t) IOPORT_PIN_P026_PFC_00_TMS_SWDIO) },
  { .pin = BSP_IO_PORT_02_PIN_7, .pin_cfg = ((uint32_t) IOPORT_CFG_PORT_PERI
          | (uint32_t) IOPORT_PIN_P027_PFC_00_TCK_SWCLK) },
  { .pin = BSP_IO_PORT_05_PIN_5, .pin_cfg = ((uint32_t) IOPORT_CFG_PORT_PERI
          | (uint32_t) IOPORT_PIN_P055_PFC_02_ESC_PHYLINK1) },
  { .pin = BSP_IO_PORT_05_PIN_7, .pin_cfg = ((uint32_t) IOPORT_CFG_DRIVE_HIGH | (uint32_t) IOPORT_CFG_PORT_PERI
          | (uint32_t) IOPORT_CFG_SLEW_RATE_FAST | (uint32_t) IOPORT_PIN_P057_PFC_00_ETH1_TXD2) },
  { .pin = BSP_IO_PORT_06_PIN_0, .pin_cfg = ((uint32_t) IOPORT_CFG_DRIVE_HIGH | (uint32_t) IOPORT_CFG_PORT_PERI
          | (uint32_t) IOPORT_CFG_SLEW_RATE_FAST | (uint32_t) IOPORT_PIN_P060_PFC_00_ETH1_TXD3) },
  { .pin = BSP_IO_PORT_06_PIN_1, .pin_cfg = ((uint32_t) IOPORT_CFG_DRIVE_HIGH | (uint32_t) IOPORT_CFG_PORT_PERI
          | (uint32_t) IOPORT_CFG_SLEW_RATE_FAST | (uint32_t) IOPORT_PIN_P061_PFC_00_ETH1_REFCLK) },
  { .pin = BSP_IO_PORT_06_PIN_2, .pin_cfg = ((uint32_t) IOPORT_CFG_DRIVE_HIGH | (uint32_t) IOPORT_CFG_PORT_PERI
          | (uint32_t) IOPORT_CFG_SLEW_RATE_FAST | (uint32_t) IOPORT_PIN_P062_PFC_00_ETH1_TXD1) },
  { .pin = BSP_IO_PORT_06_PIN_3, .pin_cfg = ((uint32_t) IOPORT_CFG_DRIVE_HIGH | (uint32_t) IOPORT_CFG_PORT_PERI
          | (uint32_t) IOPORT_CFG_SLEW_RATE_FAST | (uint32_t) IOPORT_PIN_P063_PFC_00_ETH1_TXD0) },
  { .pin = BSP_IO_PORT_06_PIN_4, .pin_cfg = ((uint32_t) IOPORT_CFG_DRIVE_HIGH | (uint32_t) IOPORT_CFG_PORT_PERI
          | (uint32_t) IOPORT_CFG_SLEW_RATE_FAST | (uint32_t) IOPORT_PIN_P064_PFC_00_ETH1_TXCLK) },
  { .pin = BSP_IO_PORT_06_PIN_5, .pin_cfg = ((uint32_t) IOPORT_CFG_PORT_PERI
          | (uint32_t) IOPORT_PIN_P065_PFC_00_ETH1_TXEN) },
  { .pin = BSP_IO_PORT_06_PIN_6, .pin_cfg = ((uint32_t) IOPORT_CFG_PORT_PERI
          | (uint32_t) IOPORT_PIN_P066_PFC_00_ETH1_RXD0) },
  { .pin = BSP_IO_PORT_06_PIN_7, .pin_cfg = ((uint32_t) IOPORT_CFG_PORT_PERI
          | (uint32_t) IOPORT_PIN_P067_PFC_00_ETH1_RXD1) },
  { .pin = BSP_IO_PORT_07_PIN_0, .pin_cfg = ((uint32_t) IOPORT_CFG_PORT_PERI
          | (uint32_t) IOPORT_PIN_P070_PFC_00_ETH1_RXD2) },
  { .pin = BSP_IO_PORT_07_PIN_1, .pin_cfg = ((uint32_t) IOPORT_CFG_PORT_PERI
          | (uint32_t) IOPORT_PIN_P071_PFC_00_ETH1_RXD3) },
  { .pin = BSP_IO_PORT_07_PIN_2, .pin_cfg = ((uint32_t) IOPORT_CFG_PORT_PERI
          | (uint32_t) IOPORT_PIN_P072_PFC_00_ETH1_RXDV) },
  { .pin = BSP_IO_PORT_07_PIN_3, .pin_cfg = ((uint32_t) IOPORT_CFG_PORT_PERI
          | (uint32_t) IOPORT_PIN_P073_PFC_00_ETH1_RXCLK) },
  { .pin = BSP_IO_PORT_08_PIN_4, .pin_cfg = ((uint32_t) IOPORT_CFG_PORT_PERI
          | (uint32_t) IOPORT_PIN_P084_PFC_00_ETH0_RXD3) },
  { .pin = BSP_IO_PORT_08_PIN_5, .pin_cfg = ((uint32_t) IOPORT_CFG_PORT_PERI
          | (uint32_t) IOPORT_PIN_P085_PFC_00_ETH0_RXDV) },
  { .pin = BSP_IO_PORT_08_PIN_6, .pin_cfg = ((uint32_t) IOPORT_CFG_PORT_PERI
          | (uint32_t) IOPORT_PIN_P086_PFC_00_ETH0_RXCLK) },
  { .pin = BSP_IO_PORT_08_PIN_7, .pin_cfg =
            ((uint32_t) IOPORT_CFG_PORT_PERI | (uint32_t) IOPORT_PIN_P087_PFC_03_ESC_MDC) },
  { .pin = BSP_IO_PORT_09_PIN_0, .pin_cfg = ((uint32_t) IOPORT_CFG_PORT_PERI
          | (uint32_t) IOPORT_PIN_P090_PFC_03_ESC_MDIO) },
  { .pin = BSP_IO_PORT_09_PIN_1, .pin_cfg = ((uint32_t) IOPORT_CFG_DRIVE_HIGH | (uint32_t) IOPORT_CFG_PORT_PERI
          | (uint32_t) IOPORT_CFG_SLEW_RATE_FAST | (uint32_t) IOPORT_PIN_P091_PFC_00_ETH0_REFCLK) },
  { .pin = BSP_IO_PORT_09_PIN_3, .pin_cfg = ((uint32_t) IOPORT_CFG_DRIVE_HIGH | (uint32_t) IOPORT_CFG_PORT_PERI
          | (uint32_t) IOPORT_CFG_SLEW_RATE_FAST | (uint32_t) IOPORT_PIN_P093_PFC_00_ETH0_TXD3) },
  { .pin = BSP_IO_PORT_09_PIN_4, .pin_cfg = ((uint32_t) IOPORT_CFG_DRIVE_HIGH | (uint32_t) IOPORT_CFG_PORT_PERI
          | (uint32_t) IOPORT_CFG_SLEW_RATE_FAST | (uint32_t) IOPORT_PIN_P094_PFC_00_ETH0_TXD2) },
  { .pin = BSP_IO_PORT_09_PIN_5, .pin_cfg = ((uint32_t) IOPORT_CFG_DRIVE_HIGH | (uint32_t) IOPORT_CFG_PORT_PERI
          | (uint32_t) IOPORT_CFG_SLEW_RATE_FAST | (uint32_t) IOPORT_PIN_P095_PFC_00_ETH0_TXD1) },
  { .pin = BSP_IO_PORT_09_PIN_6, .pin_cfg = ((uint32_t) IOPORT_CFG_DRIVE_HIGH | (uint32_t) IOPORT_CFG_PORT_PERI
          | (uint32_t) IOPORT_CFG_SLEW_RATE_FAST | (uint32_t) IOPORT_PIN_P096_PFC_00_ETH0_TXD0) },
  { .pin = BSP_IO_PORT_09_PIN_7, .pin_cfg = ((uint32_t) IOPORT_CFG_DRIVE_HIGH | (uint32_t) IOPORT_CFG_PORT_PERI
          | (uint32_t) IOPORT_CFG_SLEW_RATE_FAST | (uint32_t) IOPORT_PIN_P097_PFC_00_ETH0_TXCLK) },
  { .pin = BSP_IO_PORT_10_PIN_0, .pin_cfg = ((uint32_t) IOPORT_CFG_PORT_PERI
          | (uint32_t) IOPORT_PIN_P100_PFC_00_ETH0_TXEN) },
  { .pin = BSP_IO_PORT_10_PIN_1, .pin_cfg = ((uint32_t) IOPORT_CFG_PORT_PERI
          | (uint32_t) IOPORT_PIN_P101_PFC_00_ETH0_RXD0) },
  { .pin = BSP_IO_PORT_10_PIN_2, .pin_cfg = ((uint32_t) IOPORT_CFG_PORT_PERI
          | (uint32_t) IOPORT_PIN_P102_PFC_00_ETH0_RXD1) },
  { .pin = BSP_IO_PORT_10_PIN_3, .pin_cfg = ((uint32_t) IOPORT_CFG_PORT_PERI
          | (uint32_t) IOPORT_PIN_P103_PFC_00_ETH0_RXD2) },
  { .pin = BSP_IO_PORT_10_PIN_4, .pin_cfg = ((uint32_t) IOPORT_CFG_PORT_PERI
          | (uint32_t) IOPORT_PIN_P104_PFC_03_ESC_PHYLINK0) },
  { .pin = BSP_IO_PORT_13_PIN_2, .pin_cfg = ((uint32_t) IOPORT_CFG_PORT_PERI
          | (uint32_t) IOPORT_PIN_P132_PFC_05_ESC_I2CCLK) },
  { .pin = BSP_IO_PORT_13_PIN_3, .pin_cfg = ((uint32_t) IOPORT_CFG_PORT_PERI
          | (uint32_t) IOPORT_PIN_P133_PFC_04_ESC_I2CDATA) },
  { .pin = BSP_IO_PORT_13_PIN_4, .pin_cfg = ((uint32_t) IOPORT_CFG_PORT_DIRECTION_OUTPUT
          | (uint32_t) IOPORT_CFG_PORT_OUTPUT_LOW) },
  { .pin = BSP_IO_PORT_14_PIN_0, .pin_cfg = ((uint32_t) IOPORT_CFG_PORT_DIRECTION_INPUT) },
  { .pin = BSP_IO_PORT_14_PIN_6, .pin_cfg = ((uint32_t) IOPORT_CFG_DRIVE_HIGH | (uint32_t) IOPORT_CFG_PORT_PERI
          | (uint32_t) IOPORT_CFG_SCHMITT_TRIGGER_ENABLE | (uint32_t) IOPORT_CFG_SLEW_RATE_FAST
          | (uint32_t) IOPORT_PIN_P146_PFC_00_XSPI0_CKP) },
  { .pin = BSP_IO_PORT_14_PIN_7, .pin_cfg = ((uint32_t) IOPORT_CFG_DRIVE_HIGH | (uint32_t) IOPORT_CFG_PORT_PERI
          | (uint32_t) IOPORT_CFG_SCHMITT_TRIGGER_ENABLE | (uint32_t) IOPORT_CFG_SLEW_RATE_FAST
          | (uint32_t) IOPORT_PIN_P147_PFC_00_XSPI0_IO0) },
  { .pin = BSP_IO_PORT_15_PIN_0, .pin_cfg = ((uint32_t) IOPORT_CFG_DRIVE_HIGH | (uint32_t) IOPORT_CFG_PORT_PERI
          | (uint32_t) IOPORT_CFG_SCHMITT_TRIGGER_ENABLE | (uint32_t) IOPORT_CFG_SLEW_RATE_FAST
          | (uint32_t) IOPORT_PIN_P150_PFC_00_XSPI0_IO1) },
  { .pin = BSP_IO_PORT_15_PIN_7, .pin_cfg = ((uint32_t) IOPORT_CFG_DRIVE_HIGH | (uint32_t) IOPORT_CFG_PORT_PERI
          | (uint32_t) IOPORT_CFG_SCHMITT_TRIGGER_ENABLE | (uint32_t) IOPORT_CFG_SLEW_RATE_FAST
          | (uint32_t) IOPORT_PIN_P157_PFC_00_XSPI0_CS0) },
  { .pin = BSP_IO_PORT_16_PIN_5, .pin_cfg = ((uint32_t) IOPORT_CFG_DRIVE_HIGH | (uint32_t) IOPORT_CFG_PORT_PERI
          | (uint32_t) IOPORT_CFG_SLEW_RATE_FAST | (uint32_t) IOPORT_PIN_P165_PFC_01_TXD0_SDA0_MOSI0) },
  { .pin = BSP_IO_PORT_16_PIN_6, .pin_cfg = ((uint32_t) IOPORT_CFG_DRIVE_HIGH | (uint32_t) IOPORT_CFG_PORT_PERI
          | (uint32_t) IOPORT_CFG_SLEW_RATE_FAST | (uint32_t) IOPORT_PIN_P166_PFC_02_RXD0_SCL0_MISO0) },
  { .pin = BSP_IO_PORT_20_PIN_1, .pin_cfg = ((uint32_t) IOPORT_CFG_PORT_PERI
          | (uint32_t) IOPORT_PIN_P201_PFC_01_ESC_LINKACT0) },
  { .pin = BSP_IO_PORT_20_PIN_2, .pin_cfg = ((uint32_t) IOPORT_CFG_PORT_PERI
          | (uint32_t) IOPORT_PIN_P202_PFC_01_ESC_LEDRUN) },
  { .pin = BSP_IO_PORT_20_PIN_3, .pin_cfg = ((uint32_t) IOPORT_CFG_PORT_PERI
          | (uint32_t) IOPORT_PIN_P203_PFC_01_ESC_LEDERR) },
  { .pin = BSP_IO_PORT_20_PIN_4, .pin_cfg = ((uint32_t) IOPORT_CFG_PORT_PERI
          | (uint32_t) IOPORT_PIN_P204_PFC_01_ESC_LINKACT1) }, };

const ioport_cfg_t g_bsp_pin_cfg =
{ .number_of_pins = sizeof(g_bsp_pin_cfg_data) / sizeof(ioport_pin_cfg_t),
  .p_pin_cfg_data = &g_bsp_pin_cfg_data[0],
  .p_extend = &g_ioport_cfg_extend, };
