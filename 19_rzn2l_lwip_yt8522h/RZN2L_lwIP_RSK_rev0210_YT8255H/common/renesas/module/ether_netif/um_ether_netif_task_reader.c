/***********************************************************************************************************************
 * Copyright [2020-2021] Renesas Electronics Corporation and/or its affiliates.  All Rights Reserved.
 *
 * This software and documentation are supplied by Renesas Electronics Corporation and/or its affiliates and may only
 * be used with products of Renesas Electronics Corp. and its affiliates ("Renesas").  No other uses are authorized.
 * Renesas products are sold pursuant to Renesas terms and conditions of sale.  Purchasers are solely responsible for
 * the selection and use of Renesas products and Renesas assumes no liability.  No license, express or implied, to any
 * intellectual property right is granted by Renesas.  This software is protected under all applicable laws, including
 * copyright laws. Renesas reserves the right to change or discontinue this software and/or this documentation.
 * THE SOFTWARE AND DOCUMENTATION IS DELIVERED TO YOU "AS IS," AND RENESAS MAKES NO REPRESENTATIONS OR WARRANTIES, AND
 * TO THE FULLEST EXTENT PERMISSIBLE UNDER APPLICABLE LAW, DISCLAIMS ALL WARRANTIES, WHETHER EXPLICITLY OR IMPLICITLY,
 * INCLUDING WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE, AND NONINFRINGEMENT, WITH RESPECT TO THE
 * SOFTWARE OR DOCUMENTATION.  RENESAS SHALL HAVE NO LIABILITY ARISING OUT OF ANY SECURITY VULNERABILITY OR BREACH.
 * TO THE MAXIMUM EXTENT PERMITTED BY LAW, IN NO EVENT WILL RENESAS BE LIABLE TO YOU IN CONNECTION WITH THE SOFTWARE OR
 * DOCUMENTATION (OR ANY PERSON OR ENTITY CLAIMING RIGHTS DERIVED FROM YOU) FOR ANY LOSS, DAMAGES, OR CLAIMS WHATSOEVER,
 * INCLUDING, WITHOUT LIMITATION, ANY DIRECT, CONSEQUENTIAL, SPECIAL, INDIRECT, PUNITIVE, OR INCIDENTAL DAMAGES; ANY
 * LOST PROFITS, OTHER ECONOMIC DAMAGE, PROPERTY DAMAGE, OR PERSONAL INJURY; AND EVEN IF RENESAS HAS BEEN ADVISED OF THE
 * POSSIBILITY OF SUCH LOSS, DAMAGES, CLAIMS OR COSTS.
 **********************************************************************************************************************/

/***********************************************************************************************************************
 * Includes
 **********************************************************************************************************************/
#include "lwip/etharp.h"
#include "um_ether_netif.h"
#include "um_ether_netif_cfg.h"
#include "um_ether_netif_internal.h"
#include "r_ether_api.h"
#include "r_ether_cfg.h"
#include "fsp_common_api.h"

/***********************************************************************************************************************
 * Macro definitions
 **********************************************************************************************************************/
/***********************************************************************************************************************
 * Private constants
 **********************************************************************************************************************/
/**********************************************************************************************************************
 * Typedef definitions
 **********************************************************************************************************************/
/***********************************************************************************************************************
 * Private function prototypes
 **********************************************************************************************************************/
static void _task_code(void * pvParameter);
extern struct netif *pg_lwip_netif;

/***********************************************************************************************************************
 * Functions
 **********************************************************************************************************************/
/*******************************************************************************************************************//**
 * @brief Create the task and initialize its controller.
 *
 * @param[in] p_ctrl			Pointer to the controller
 * @param[in] p_target_ctrl		Pointer to target controller
 * @param[in] p_callback_ctrl	Pointer to callback controller
 *
 * @retval USR_SUCCESS					Process has been done successfully.
 * @retval USR_ERR_NOT_INITIALIZED		Initialization has been failed.
 **********************************************************************************************************************/
usr_err_t um_ether_netif_task_reader_open( ether_netif_reader_ctrl_t * const p_ctrl,
									       ether_netif_ether_ctrl_t * const p_target_ctrl,
										   ether_netif_callback_ctrl_t * const p_callback_ctrl )
{
	BaseType_t rtos_err;

	/** Set target and callback control. */
	p_ctrl->p_ether_ctrl = p_target_ctrl;
	p_ctrl->p_callback_ctrl = p_callback_ctrl;
    p_ctrl->p_parent_task_handle = xTaskGetCurrentTaskHandle();
    p_ctrl->p_ether_netif_frame = NULL;

	/** Create the task. */
    rtos_err = xTaskCreate(_task_code,
                           UM_ETHER_NETIF_CFG_READER_TASK_NAME,
                           UM_ETHER_NETIF_CFG_READER_TASK_STACK_BYTE_SIZE / sizeof(StackType_t),
                           p_ctrl,
                           UM_ETHER_NETIF_CFG_READER_TASK_PRIORITY,
                           &(p_ctrl->p_task_handle) );
    USR_ERROR_RETURN( pdPASS == rtos_err, USR_ERR_NOT_INITIALIZED );
    
    /** Wait for notification indicating the created task is initialized. */
    (void) ulTaskNotifyTake( pdTRUE, portMAX_DELAY );
    
    /** Suspend the created task. */
    vTaskSuspend( p_ctrl->p_task_handle );
    
	return USR_SUCCESS;
}

/*******************************************************************************************************************//**
 * @brief Start the task.
 *
 * @param[in] p_ctrl					Pointer to controller
 *
 * @retval USR_SUCCESS					Process has been done successfully.
 **********************************************************************************************************************/
usr_err_t um_ether_netif_task_reader_start( ether_netif_reader_ctrl_t * const p_ctrl )
{
	vTaskResume(p_ctrl->p_task_handle);
	return USR_SUCCESS;
}

/*******************************************************************************************************************//**
 * @brief Stop the task.
 *
 * @param[in] p_ctrl					Pointer to controller
 *
 * @retval USR_SUCCESS					Process has been done successfully.
 **********************************************************************************************************************/
usr_err_t um_ether_netif_task_reader_stop( ether_netif_reader_ctrl_t * const p_ctrl )
{
	vTaskSuspend(p_ctrl->p_task_handle);
	return USR_SUCCESS;
}

/*******************************************************************************************************************//**
 * @brief Get the task handle
 *
 * @param[in] p_ctrl                    Pointer to controller
 *
 * @retval USR_SUCCESS                  Process has been done successfully.
 **********************************************************************************************************************/
usr_err_t um_ether_netif_task_reader_get_task_handle( ether_netif_reader_ctrl_t * const p_ctrl, TaskHandle_t * pp_task_handle )
{
    * pp_task_handle = p_ctrl->p_task_handle;
    return USR_SUCCESS;
}


/***********************************************************************************************************************
 * Private Functions
 **********************************************************************************************************************/
/*******************************************************************************************************************//**
 * @brief RTOS task.
 *
 * @param[in] pvParameter   	Pointer to task parameters.
 **********************************************************************************************************************/
static void _task_code (void * pvParameter)
{
    /** Resolve task parameter */
    ether_netif_reader_ctrl_t * const p_ctrl = (ether_netif_reader_ctrl_t * const) pvParameter;
    static char temp_buff_pay[1600];
    static struct pbuf temp_buff;

    /** Status */
    uint32_t  notices;
    struct pbuf *p_netif_packet = &temp_buff;
    p_netif_packet->payload = temp_buff_pay;

    //ether_netif_ether_ctrl_t *P_read_ctrl = (ether_netif_ether_ctrl_t *)p_ctrl->p_ether_ctrl;

    /** Notify the parent task launch of this task. */
    xTaskNotifyGive( p_ctrl->p_parent_task_handle );
    
    /** Task loop */
    while ( true )
    {
        /** Notified from Ethernet module interrupt */
        notices = ulTaskNotifyTake(pdFALSE, portMAX_DELAY);
        if ( notices == 0 )
        {
        	/** TODO: Implement task error handling */
        }
        /** Read from the Ethernet driver interface with lock */
        for( ;; ){
            /** Allocate buffer */
        	//USR_HEAP_ALLOCATE( p_ctrl->p_ether_netif_frame, sizeof(ether_netif_frame_t) );
            if(p_netif_packet == NULL){
                p_netif_packet = pbuf_alloc(PBUF_RAW, (u16_t) 1546, PBUF_RAM);
                if ( p_netif_packet == NULL ){
                    taskYIELD();
                    continue;
                }
            }

            /** Read Ethernet frame. */
            //if( FSP_SUCCESS != P_read_ctrl->p_ether_instance->p_api->read(P_read_ctrl->p_ether_instance->p_ctrl, p_netif_packet->payload, (uint32_t *)&p_netif_packet->len)){
            if(USR_SUCCESS != um_ether_netif_ether_read(p_ctrl->p_ether_ctrl, p_netif_packet )){
                break;
            }

            /** Check if network interface is up */
            if ( NETIF_FLAG_UP != (pg_lwip_netif->flags & NETIF_FLAG_UP) ){
                break;
            }else{
                if(p_netif_packet == (&temp_buff)){
                    p_netif_packet = NULL;
                    break;
                }

            }

            /** Input Ethernet frame into lwIP network interface. */
            pg_lwip_netif->input(p_netif_packet, pg_lwip_netif);
            p_netif_packet = NULL;
            /** Read received Ethernet frame from Ethernet driver buffer. */
            //usr_err = um_ether_netif_ether_read(p_ctrl->p_ether_ctrl, p_ctrl->p_ether_netif_frame );
#if 0
            if( USR_SUCCESS != usr_err )
			{
            	/** Break due to no reception data */
            	vPortFree( p_ctrl->p_ether_netif_frame );
            	p_ctrl->p_ether_netif_frame = NULL;
            	break;
			}

            /** Request the callback for notifying application the received frame. */
            usr_err = um_ether_netif_callback_request((ether_netif_callback_ctrl_t * ) p_ctrl->p_callback_ctrl,
            							   	   	   	   ETHER_NETIF_CALLBACK_EVENT_RECEIVE_ETHER_FRAME,
    												   p_ctrl->p_ether_netif_frame );
            if( USR_SUCCESS != usr_err )
			{
            	/** TODO: Implement task error handling. */
			}
            taskYIELD();
#endif
        }
    }
    /** vTaskDelete(NULL); */
}
