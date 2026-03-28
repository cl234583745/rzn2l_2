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
/** User module instance APIs. */
#include "um_lwip_port_api.h"     /** API of lwIP porting module */
#include "um_serial_io_api.h"       /** API of serial output for debugging */
#include "um_common_api.h"
#include "um_common_cfg.h"

/** For handling application task */
#include "FreeRTOS.h"
#include "queue.h"

/** lwIP modules */
#include "lwip/sockets.h"
#include "lwip/errno.h"

/** Standard library */
#include "stdlib.h"

/***********************************************************************************************************************
 * Macro definitions
 **********************************************************************************************************************/
#define TCP_SERVER_RECV_BUFFER_SIZE     (1600)
#define TCP_SERVER_PORT                 (65000)
#define TCP_SERVER_TASK_PRIORITY        (4)
#define TCP_SERVER_TASK_STACK_SIZE      (1024)
#define TCP_SERVER_TASK_NAME            "TCP Server task"
#define TCP_SERVER_INVALID_SOCKET       (-1)

/***********************************************************************************************************************
 * Private constants
 **********************************************************************************************************************/
#define SAMPLE_APP_VERSION_MAJOR (1)
#define SAMPLE_AAP_VERSION_MINOR (4)

/**********************************************************************************************************************
 * Typedef definitions
 **********************************************************************************************************************/
/**
 * Application (TCP server) control.
 */
typedef struct st_tcp_server_ctrl
{
    TaskHandle_t 	 p_server_task_handle;	                       ///< TCP listner Task handler.
    TaskHandle_t     p_parent_task_handle;                         ///< Task handle of parent task.
    uint8_t 		 recv_buffer[TCP_SERVER_RECV_BUFFER_SIZE];     ///< Receive buffer.

    int32_t          max_fd;
    uint32_t         num_of_socket;
    fd_set           fdset_listners;
    fd_set           fdset_clients;
    fd_set           fdset_all;

    lwip_port_netif_state_t lwip_netif_state;
    lwip_port_instance_t const * p_lwip_port_instance;             ///< The controller of lwIP port module
    lwip_port_callback_args_t callback_memory;
    lwip_port_callback_link_node_t callback_link_node;
} tcp_server_ctrl_t;

/**********************************************************************************************************************
 * Private function prototypes
 **********************************************************************************************************************/
static void _tcp_server_lwip_port_callback( lwip_port_callback_args_t * p_arg );
static usr_err_t _tcp_server_add_listener_socket( tcp_server_ctrl_t * p_ctrl );
static usr_err_t _tcp_server_remove_listener_socket( tcp_server_ctrl_t * p_ctrl );
static void _tcp_server_task( void* pvParameter );
static usr_err_t _tcp_server_handle_listner_socket( tcp_server_ctrl_t * p_ctrl, int32_t listner_socket_fd );
static usr_err_t _tcp_server_handle_connected_socket( tcp_server_ctrl_t * p_ctrl, int32_t connected_socket_fd );
static int32_t _tcp_server_get_socket_by_ip_address( uint32_t ip_address, int max_fd );
static int32_t _tcp_server_create_new_listner_socket( uint32_t ip_address, uint16_t port );

/***********************************************************************************************************************
 * Private global variables
 **********************************************************************************************************************/
/***********************************************************************************************************************
 * Global Variables
 **********************************************************************************************************************/
/**
 * Platform module instances
 */
extern lwip_port_instance_t   const * gp_lwip_port0;

/**
 * TCP server module instance.
 */
static tcp_server_ctrl_t g_tcp_server0_ctrl;
static tcp_server_ctrl_t * gp_tcp_server0_ctrl = &g_tcp_server0_ctrl;

/**
 * Define the errno for lwIP BSD socket interface (EWARM only)
 */
#if defined(__ICCARM__)
int errno;
#endif

/***********************************************************************************************************************
 * Functions
 **********************************************************************************************************************/
/**
 * @brief Example codes for initializing and handling lwIP port module.
 */
void lwip_port_user_main( void );
void lwip_port_user_main( void )
{
    /** Status */
    usr_err_t usr_err;      /** User status */
    BaseType_t rtos_err;    /** Rtos status */

    /** Enabled the printf() function via SCI UART. */
    USR_LOG_INFO( "/** Started sample application (v%d.%d) for lwIP Port. **/",
                     SAMPLE_APP_VERSION_MAJOR, SAMPLE_AAP_VERSION_MINOR);

    /******************************************************************************************************************
     * Check the link state
     ******************************************************************************************************************/
    /** Executes any add-on application option settings */
    #if (defined _LWIP_ADD_ON_APP) && ((_LWIP_ADD_ON_APP & 7) != 0)
    extern void lwip_AddOn_main(void);
    //uint32_t link_status;
    static tcp_server_ctrl_t *tcp_server0_ctrl = NULL;
    USR_LOG_INFO( "Waiting for link-up..." );
    /* for LOG output */
    //vTaskDelay( 100 / portTICK_PERIOD_MS );
    if(NULL == (tcp_server0_ctrl = pvPortMalloc(sizeof(tcp_server_ctrl_t)))){
        USR_LOG_ERROR( "Unable to get heap memory." );
        return;
    }

    /* wait for link up */
    while(1){
        /* get linu-up status */
        usr_err =  gp_lwip_port0->p_api->netifStateGet( gp_lwip_port0->p_ctrl, &tcp_server0_ctrl->lwip_netif_state, false );
        if((USR_SUCCESS == usr_err) && (LWIP_PORT_NETIF_STATE_UP == tcp_server0_ctrl->lwip_netif_state)){
            /* heap free */
            vPortFree(tcp_server0_ctrl);
            break;
        }
        vTaskDelay( 1000 / portTICK_PERIOD_MS );
    }
    /* add-on */
    lwip_AddOn_main();
    #endif /* _LWIP_ADD_ON_APP */

    /******************************************************************************************************************
     * Startup TCP Server application.
     ******************************************************************************************************************/
    /** Set lwIP port module instance. */
    gp_tcp_server0_ctrl->p_lwip_port_instance = gp_lwip_port0;
    /** Set current task handle for controlling the start of the task. */
    gp_tcp_server0_ctrl->p_parent_task_handle = xTaskGetCurrentTaskHandle();

    /** Clear socket descriptor sets */
    FD_ZERO(&gp_tcp_server0_ctrl->fdset_listners);
    FD_ZERO(&gp_tcp_server0_ctrl->fdset_clients);
    FD_ZERO(&gp_tcp_server0_ctrl->fdset_all);
    gp_tcp_server0_ctrl->max_fd = TCP_SERVER_INVALID_SOCKET;

    /** Set callback utilities into TCPIP network interface. */
    gp_tcp_server0_ctrl->callback_link_node.p_context = gp_tcp_server0_ctrl;
    gp_tcp_server0_ctrl->callback_link_node.p_memory = &gp_tcp_server0_ctrl->callback_memory;
    gp_tcp_server0_ctrl->callback_link_node.p_func = _tcp_server_lwip_port_callback;
    gp_tcp_server0_ctrl->callback_link_node.p_next = NULL;
    usr_err =  gp_lwip_port0->p_api->callbackAdd( gp_lwip_port0->p_ctrl, &gp_tcp_server0_ctrl->callback_link_node );
    if ( USR_SUCCESS != usr_err )
    {
        USR_LOG_ERROR( "Failed to set the callback function into TCP/IP stack." );
        USR_DEBUG_BLOCK_CPU();
    }
    USR_LOG_INFO( "Set the callback function into TCP/IP stack." );

    /** Create application task. */
    rtos_err = xTaskCreate(_tcp_server_task,
                           TCP_SERVER_TASK_NAME,
                           TCP_SERVER_TASK_STACK_SIZE / sizeof(StackType_t),
                           gp_tcp_server0_ctrl,
                           TCP_SERVER_TASK_PRIORITY,
                           & gp_tcp_server0_ctrl->p_server_task_handle);
    if ( pdTRUE != rtos_err )
    {
        USR_LOG_ERROR( "Failed to create application task." );
        USR_DEBUG_BLOCK_CPU();
    }
    USR_LOG_INFO( "Created sample application task." );

    /** Wait for notification indicating the created task is initialized. */
    (void) ulTaskNotifyTake( pdTRUE, portMAX_DELAY );

    /** Suspend the created task. */
    vTaskSuspend( gp_tcp_server0_ctrl->p_server_task_handle );

    /** Resume the created task */
    vTaskResume( gp_tcp_server0_ctrl->p_server_task_handle );

    while( 1 ) {
        /** Check the netif state */
        usr_err =  gp_lwip_port0->p_api->netifStateGet( gp_lwip_port0->p_ctrl, &gp_tcp_server0_ctrl->lwip_netif_state, true );
        /** If the netif is up. */
        if( LWIP_PORT_NETIF_STATE_UP == gp_tcp_server0_ctrl->lwip_netif_state ){
            break;
        }
        /** Wait 1s */
        vTaskDelay( 1000 / portTICK_PERIOD_MS );
    }
}

/***********************************************************************************************************************
 * Private Functions
 **********************************************************************************************************************/
/***********************************************************************************************************************
 * @brief Callback function called when the lwIP Network interface is up or down.
 **********************************************************************************************************************/
static void _tcp_server_lwip_port_callback( lwip_port_callback_args_t * p_arg )
{
	/** Resolve context */
	tcp_server_ctrl_t * p_ctrl = (tcp_server_ctrl_t *) p_arg->p_context;

    switch ( p_arg->event )
    {
    case LWIP_PORT_CALLBACK_EVENT_NETIF_UP:

        _tcp_server_add_listener_socket(p_ctrl);

        break;

    case LWIP_PORT_CALLBACK_EVENT_NETIF_DOWN:

        _tcp_server_remove_listener_socket(p_ctrl);

        break;
    }
}

/***********************************************************************************************************************
 * @brief TCP server task handling the BSD sockets.
 **********************************************************************************************************************/
static void _tcp_server_task( void* pvParameter )
{
	/** Resolve parameter */
	tcp_server_ctrl_t * p_ctrl = (tcp_server_ctrl_t *) pvParameter;

    /** Notify the parenet task launch of this task. */
    xTaskNotifyGive( p_ctrl->p_parent_task_handle );

	/** fdsets for select */
    fd_set fdset_read;
    fd_set fdset_except;

    /** For scanning fd sets */
    int32_t num_sockets = 0;
    int32_t socket_fd = -1;     /** Iterator */

	for(;;)
	{
	    if ( p_ctrl->num_of_socket == 0 )
	    {
	        vTaskDelay(100 / portTICK_PERIOD_MS);
	        continue;
	    }

        /** Get fdsets for detecting the read and except event. */
        memcpy(&fdset_read, &p_ctrl->fdset_all, sizeof(fd_set));
        memcpy(&fdset_except, &p_ctrl->fdset_all, sizeof(fd_set));

        /** Try select to detect event */
        num_sockets = (int32_t) lwip_select( p_ctrl->max_fd + 1, &fdset_read, NULL, &fdset_except, NULL );

        /** Continue if any events are detected. */
        if ( num_sockets <= 0 )
        {
        	continue;
        }

        /** Check events */
    	for( socket_fd = 0; socket_fd <= p_ctrl->max_fd ; socket_fd++ )
    	{
    		/** Check if the socket has event. */
    		if ( !FD_ISSET(socket_fd, &fdset_read) && !FD_ISSET(socket_fd, &fdset_except) )
    		{
    			continue;
    		}

    		/** Check if the socket is listener socket. */
    		if ( FD_ISSET(socket_fd, &p_ctrl->fdset_listners) )
    		{
    		    (void) _tcp_server_handle_listner_socket(p_ctrl, socket_fd);
    		    continue;
    		}

    		/** Check if the socket is client socket. */
    		if ( FD_ISSET(socket_fd, &p_ctrl->fdset_clients ) )
    		{
    		    (void) _tcp_server_handle_connected_socket(p_ctrl, socket_fd);
    		    continue;
    		}
    	}
	}
}


/***********************************************************************************************************************
 * @brief Add listener socket
 **********************************************************************************************************************/
static usr_err_t _tcp_server_add_listener_socket( tcp_server_ctrl_t * p_ctrl )
{
    /** Status */
    usr_err_t usr_err;

    /** For getting IP information */
    lwip_port_netif_cfg_t netif_cfg;

    /** Listener socket */
    int32_t listener_socket_fd;

    /** Get IP address */
    usr_err = p_ctrl->p_lwip_port_instance->p_api->netifConfigGet( p_ctrl->p_lwip_port_instance->p_ctrl, &netif_cfg );
    USR_ERROR_RETURN( USR_SUCCESS == usr_err, USR_ERR_ABORTED );

    /** Check if the IP address is enabled. */
    listener_socket_fd = _tcp_server_get_socket_by_ip_address( netif_cfg.ip_address, p_ctrl->max_fd );
    USR_ERROR_RETURN( TCP_SERVER_INVALID_SOCKET == listener_socket_fd, USR_ERR_ABORTED );

    /** Create new listener socket */
    listener_socket_fd = _tcp_server_create_new_listner_socket( netif_cfg.ip_address , TCP_SERVER_PORT );
    USR_ERROR_RETURN( TCP_SERVER_INVALID_SOCKET  != listener_socket_fd, USR_ERR_ABORTED );

    /** Update listener socket informations */
    FD_SET( listener_socket_fd, &p_ctrl->fdset_listners );
    FD_SET( listener_socket_fd, &p_ctrl->fdset_all );
    if ( listener_socket_fd > p_ctrl->max_fd ) { p_ctrl->max_fd = listener_socket_fd; }
    p_ctrl->num_of_socket++;

    return USR_SUCCESS;
}

/***********************************************************************************************************************
 * @brief Add remove listener socket
 **********************************************************************************************************************/
static usr_err_t _tcp_server_remove_listener_socket( tcp_server_ctrl_t * p_ctrl )
{
    /** Status */
    usr_err_t usr_err;
    int32_t soc_err;

    /** For getting IP information */
    lwip_port_netif_cfg_t netif_cfg;

    /** Listener socket */
    int32_t listener_socket_fd;

    /** Get IP address */
    usr_err = p_ctrl->p_lwip_port_instance->p_api->netifConfigGet( p_ctrl->p_lwip_port_instance->p_ctrl, &netif_cfg );
    USR_ERROR_RETURN( USR_SUCCESS == usr_err, USR_ERR_ABORTED );

    /** Get the target socket by referencing the IP address */
    listener_socket_fd = _tcp_server_get_socket_by_ip_address( netif_cfg.ip_address, p_ctrl->max_fd );
    USR_ERROR_RETURN( TCP_SERVER_INVALID_SOCKET != listener_socket_fd, USR_ERR_ABORTED );

    /** Try closing the socket. */
    soc_err = lwip_close( listener_socket_fd );
    USR_ERROR_RETURN( TCP_SERVER_INVALID_SOCKET != soc_err, USR_ERR_ABORTED );

    /** Update listener socket informations */
    FD_CLR( listener_socket_fd, &p_ctrl->fdset_listners );
    FD_CLR( listener_socket_fd, &p_ctrl->fdset_all );
    p_ctrl->num_of_socket--;

    /** TODO: Update mac_fd */
    return USR_SUCCESS;
}


/***********************************************************************************************************************
 * @brief handle listener socket
 **********************************************************************************************************************/
static usr_err_t _tcp_server_handle_listner_socket( tcp_server_ctrl_t * p_ctrl, int32_t listner_socket_fd )
{
    /** new client socket information  */
    int32_t client_socket_fd;
    struct sockaddr_in client_addr;
    socklen_t client_addr_len;

    /** Accept */
    client_socket_fd = lwip_accept(listner_socket_fd, (struct sockaddr*) &client_addr, &client_addr_len);
    USR_ERROR_RETURN( -1 != client_socket_fd, USR_ERR_ABORTED );

    /** Update fd sets */
    FD_SET( client_socket_fd, &p_ctrl->fdset_clients );
    FD_SET( client_socket_fd, &p_ctrl->fdset_all );
    if ( client_socket_fd > p_ctrl->max_fd ) { p_ctrl->max_fd = client_socket_fd; }
    p_ctrl->num_of_socket++;

    /** Return success code. */
    return USR_SUCCESS;
}

/***********************************************************************************************************************
 * @brief handle connected socket
 **********************************************************************************************************************/
static usr_err_t _tcp_server_handle_connected_socket( tcp_server_ctrl_t * p_ctrl, int32_t connected_socket_fd )
{
    /** return values of recv and send */
    ssize_t recv_size;
    ssize_t sent_size;

    /** socket error */
    int32_t soc_err;

    /** Receive TCP packet. */
    recv_size = lwip_recv(connected_socket_fd, p_ctrl->recv_buffer, TCP_SERVER_RECV_BUFFER_SIZE, 0);

    /** If the packet is not received, check each error check by seeing errno. */
    if ( recv_size <= 0 )
    {
        /** Check if recv() is timed out.*/
        USR_ERROR_RETURN( errno != EWOULDBLOCK, USR_ERR_ABORTED ); /** This block must be not reachable because it is ensured by select() that the client socket is readable. */

        /** Check if the socket is disconnected. */
        if (errno == ENOTCONN)
        {
            /** Close the disconnected client socket. */
            soc_err = lwip_close( connected_socket_fd );
            USR_ERROR_RETURN( TCP_SERVER_INVALID_SOCKET != soc_err, USR_ERR_ABORTED );

            /** Update listener socket informations */
            FD_CLR( connected_socket_fd, &p_ctrl->fdset_clients );
            FD_CLR( connected_socket_fd, &p_ctrl->fdset_all );
            p_ctrl->num_of_socket--;
            /** TODO: Update max_fd */
        }
    }

    /** Response with TCP packet. Here, it is simple echo. */
    sent_size = lwip_send( connected_socket_fd, p_ctrl->recv_buffer, (size_t) recv_size, 0);
    USR_ERROR_RETURN( sent_size > 0, USR_ERR_ABORTED);

    return USR_SUCCESS;
}

/***********************************************************************************************************************
 * @brief Check if the IP address is already used by existing sockets
 **********************************************************************************************************************/
static int32_t _tcp_server_get_socket_by_ip_address( uint32_t ip_address, int max_fd )
{
	int socket_err = TCP_SERVER_INVALID_SOCKET;
	int socket_fd = 0;
	struct sockaddr_in socket_addr;
	socklen_t socket_addr_len;

	/** Check if the IP address is already used in existing sockets */
	for ( socket_fd = 0; socket_fd <= max_fd ; socket_fd++ )
	{
		/** */
		socket_err = lwip_getsockname( socket_fd, (struct sockaddr*) &socket_addr, &socket_addr_len );
		if ( TCP_SERVER_INVALID_SOCKET == socket_err )
		{
			/** Is the socket is already closed? */
			continue;
		}

		if ( socket_addr.sin_addr.s_addr == ip_address )
		{
			/** The IP address is already used. */
			return socket_fd;
		}
	}

	/** The IP address is not used yet. */
	return TCP_SERVER_INVALID_SOCKET;
}

/***********************************************************************************************************************
 * @brief Create new listner socket
 **********************************************************************************************************************/
static int32_t _tcp_server_create_new_listner_socket( uint32_t ip_address, uint16_t port )
{
	int socket_fd;
	int socket_err;
	struct sockaddr_in socket_addr;

	/** Create listener socket */
	socket_fd = lwip_socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
	if ( TCP_SERVER_INVALID_SOCKET == socket_fd )
	{
		return TCP_SERVER_INVALID_SOCKET;
	}

	/** Bind the socket */
	socket_addr.sin_port = htons(port);
	socket_addr.sin_family = AF_INET;
	socket_addr.sin_addr.s_addr = ip_address;
	socket_err = lwip_bind( socket_fd, (struct sockaddr*) &socket_addr, (socklen_t) sizeof(struct sockaddr));
	if ( TCP_SERVER_INVALID_SOCKET == socket_err )
	{
		socket_err = lwip_close(socket_fd);
		return TCP_SERVER_INVALID_SOCKET;
	}

	/** Start listening */
	lwip_listen(socket_fd, 20);
	if ( TCP_SERVER_INVALID_SOCKET == socket_err )
	{
		socket_err = lwip_close(socket_fd);
		return TCP_SERVER_INVALID_SOCKET;
	}

	return socket_fd;
}
