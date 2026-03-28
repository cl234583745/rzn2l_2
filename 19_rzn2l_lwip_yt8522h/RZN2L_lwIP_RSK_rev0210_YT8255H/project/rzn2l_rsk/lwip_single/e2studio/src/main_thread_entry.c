#include "main_thread.h"
#include "um_serial_io_api.h"       /** API of serial output for debugging */
#include "um_common_cfg.h"

void ether_phy_targets_initialize_yh8522h_mii(ether_phy_instance_ctrl_t *p_instance_ctrl)
{
    uint32_t reg;

    //R_BSP_SoftwareDelay(1000, 1000);

    USR_LOG_INFO( "yh8512h_mii phy address=%d", p_instance_ctrl->p_ether_phy_cfg->phy_lsi_address );

#if 0
    #define ETHER_PHY_REG_PHY_RST_AN_OFFSET                 (0x9)
    #define ETHER_PHY_REG_SPEED_SELECT1_OFFSET              (0x6)
    #define ETHER_PHY_REG_SPEED_SELECT0_OFFSET              (0xD)
    #define ETHER_PHY_REG_BASIC_CONTROL                     (0x00)
    #define ETHER_PHY_REG_BASIC_STATUS                      (0x01)
    #define ETHER_PHY_REG_EXTEND_STATUS_OFFSET              (0x8)
#endif
    #define ETHER_PHY_REG_DEBUG_REGISTER_ADDRESS_OFFSET     0x1E
    #define ETHER_PHY_REG_DEBUG_REGISTER_DATA               0x1F

    //�����1
    R_ETHER_PHY_Write(p_instance_ctrl,ETHER_PHY_REG_DEBUG_REGISTER_ADDRESS_OFFSET, 0x40C0);
    R_ETHER_PHY_Write(p_instance_ctrl,ETHER_PHY_REG_DEBUG_REGISTER_DATA, 0x030);
    R_ETHER_PHY_Write(p_instance_ctrl,ETHER_PHY_REG_DEBUG_REGISTER_ADDRESS_OFFSET, 0x40C3);
    R_ETHER_PHY_Write(p_instance_ctrl,ETHER_PHY_REG_DEBUG_REGISTER_DATA, 0x0320);
#if 0
    //�����2
    R_ETHER_PHY_Read(p_instance_ctrl, ETHER_PHY_REG_BASIC_STATUS,&reg);
    USR_LOG_INFO( "yh8512h_mii read ETHER_PHY_REG_BASIC_CONTROL=%lx", reg );
    if(reg & 1<< ETHER_PHY_REG_EXTEND_STATUS_OFFSET)
    {
        R_ETHER_PHY_Read(p_instance_ctrl, ETHER_PHY_REG_BASIC_CONTROL,&reg);//read 0x00

        USR_LOG_INFO( "yh8512h_mii read ETHER_PHY_REG_BASIC_CONTROL=%lx", reg );

        reg &= ~(1<< ETHER_PHY_REG_SPEED_SELECT1_OFFSET | 1<< ETHER_PHY_REG_SPEED_SELECT0_OFFSET);
        reg |= 1 << ETHER_PHY_REG_SPEED_SELECT0_OFFSET;
        R_ETHER_PHY_Write(p_instance_ctrl, ETHER_PHY_REG_BASIC_CONTROL, reg);

        USR_LOG_INFO( "yh8512h_mii write ETHER_PHY_REG_BASIC_CONTROL=%lx", reg );

        R_ETHER_PHY_Read(p_instance_ctrl, ETHER_PHY_REG_BASIC_CONTROL,&reg);

        USR_LOG_INFO( "yh8512h_mii read ETHER_PHY_REG_BASIC_CONTROL=%lx", reg );

        reg |= 1 << ETHER_PHY_REG_PHY_RST_AN_OFFSET;
        R_ETHER_PHY_Write(p_instance_ctrl, ETHER_PHY_REG_BASIC_CONTROL,reg);

        USR_LOG_INFO( "yh8512h_mii write ETHER_PHY_REG_BASIC_CONTROL=%lx", reg );
    }
#endif
}
/* Main Thread entry function */
/* pvParameters contains TaskHandle_t */
void main_thread_entry(void *pvParameters)
{
    FSP_PARAMETER_NOT_USED (pvParameters);

    /** Launch the initialization example of lwIP port. */
    extern void lwip_port_user_instance_init( void );
    (void) lwip_port_user_instance_init();

    /** Launch the application example of lwIP port. */
    extern void lwip_port_user_main( void );
    (void) lwip_port_user_main();

    /* TODO: add your own code here */
    while (1)
    {
        vTaskDelay (1000);
    }
}
