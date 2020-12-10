/*
 * Copyright (c) 2006-2019, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2017-07-24     Tanek        the first version
 * 2018-11-12     Ernest Chen  modify copyright
 */
 
#include <stdint.h>
#include <rthw.h>
#include <rtthread.h>
#include "board.h"
#include "usart.h"
#include "delay.h"
#include "M8266HostIf.h"
#include "M8266WIFI_ops.h"
#include "M8266WIFIDrv.h"
#include "brd_cfg.h"
#include "pwm.h"
#include "dht11.h"

#define _SCB_BASE       (0xE000E010UL)
#define _SYSTICK_CTRL   (*(rt_uint32_t *)(_SCB_BASE + 0x0))
#define _SYSTICK_LOAD   (*(rt_uint32_t *)(_SCB_BASE + 0x4))
#define _SYSTICK_VAL    (*(rt_uint32_t *)(_SCB_BASE + 0x8))
#define _SYSTICK_CALIB  (*(rt_uint32_t *)(_SCB_BASE + 0xC))
#define _SYSTICK_PRI    (*(rt_uint8_t  *)(0xE000ED23UL))




#if defined(RT_USING_USER_MAIN) && defined(RT_USING_HEAP)
#define RT_HEAP_SIZE 2048
static uint32_t rt_heap[RT_HEAP_SIZE];     // heap default size: 4K(1024 * 4)
RT_WEAK void *rt_heap_begin_get(void)
{
    return rt_heap;
}

RT_WEAK void *rt_heap_end_get(void)
{
    return rt_heap + RT_HEAP_SIZE;
}
#endif

/**
 * This function will initial your board.
 */

void setup_TCP(void);
void rt_hw_board_init()
{
	u8 cnt = 5;
	u16 status = 0;
	char status_info[16];
	char sta_ip[15+1]={0};
	u8 ssid[] = "gy_bluetooth";
	u8 pwd[] = "12345678";
	extern uint8_t OSRunning;
	SysTick_Config(SystemCoreClock / RT_TICK_PER_SECOND);

	
	// init all components
	delay_init();
	uart_init(115200);
	TIM3_PWM_Init(1999, 719);
	dht11_init();
	
	printf("initializing the wifi model...\r\n");
	M8266HostIf_Init();
	printf("init model sucessfully\r\n");
	
	printf("resetting model...\r\n");
	M8266WIFI_Module_Hardware_Reset();
	printf("resetting model success \r\n");
	
	
	// -----------relavent with hardware interface 
	printf("set which spi and speed... \r\n");
	M8266HostIf_SPI_SetSpeed(SPI_BaudRatePrescaler_4);	
	delay_ms(50);
	while(M8266HostIf_SPI_Select((uint32_t)M8266WIFI_INTERFACE_SPI, 18000000, &status)==0)
	{	
		printf("check your host interface wiring and initialization\r\n");
		rt_memset(status_info, 0, 32);
		rt_sprintf(status_info, "status:0x%x\r\n", status);
		usart_sendStr(status_info);
		printf("reselect after 4s\r\n");
		delay_ms(1000);
		delay_ms(1000);
		delay_ms(1000);
		delay_ms(1000);
		if((cnt--) == 0){
			M8266WIFI_Module_Hardware_Reset();
			M8266HostIf_SPI_SetSpeed(SPI_BaudRatePrescaler_8);	
			delay_ms(50);
			cnt = 5;
		}
			
	}
	printf("set sucess!!\r\n");
	
	// ------------------------------------------------
	

//	M8266WIFI_SPI_Get_STA_Connection_Status(&connection_status, &status);
//	if(connection_status != 5)
//	{
//		printf("handle to access ap...\r\n");
//		M8266WIFI_SPI_STA_Connect_Ap(ssid, pwd, 0, 5, &status);
//		while(status != 0){
//			printf("check the ap is open and retry to connect ap\r\n");
//			delay_ms(1000);
//			M8266WIFI_SPI_STA_Connect_Ap(ssid, pwd, 0, 5, &status);
//		}
//	}
	
	M8266WIFI_SPI_Set_Opmode(1, 1, &status);	//STA
	// begin to access ap
	printf("trying get ip... \r\n");
	M8266WIFI_SPI_STA_Connect_Ap(ssid, pwd, 0, 5, &status);
	while(M8266WIFI_SPI_wait_sta_connecting_to_ap_and_get_ip(sta_ip, 10)==0) // max wait 10s to get sta ip 
	{
//		M8266WIFI_SPI_Get_STA_Connection_Status(&connection_status, &status);
//		if(connection_status == 5)	break;
		printf("check the ap is open and retry to connect ap\r\n");
		delay_ms(1000);
		M8266WIFI_SPI_STA_Connect_Ap(ssid, pwd, 0, 5, &status);
	}
	printf("Init SPI success\r\n");
	
//	// shutdown the web server
//	M8266WIFI_SPI_Set_WebServer(0, 3128, 0, NULL);
//	// change model to STA mode
//	M8266WIFI_SPI_Set_Opmode(1, 0, NULL);

	printf("begin to setup tcp...\r\n");
	setup_TCP();
	
	printf("tcp connected!!\r\n");
	
    /* Call components board initial (use INIT_BOARD_EXPORT()) */
#ifdef RT_USING_COMPONENTS_INIT
    rt_components_board_init();
#endif

#if defined(RT_USING_USER_MAIN) && defined(RT_USING_HEAP)
    rt_system_heap_init(rt_heap_begin_get(), rt_heap_end_get());
#endif
	
	OSRunning=1;
}

void SysTick_Handler(void)
{
    /* enter interrupt */
    rt_interrupt_enter();

    rt_tick_increase();

    /* leave interrupt */
    rt_interrupt_leave();
}

void setup_TCP(void)
{
	u16 status = 0;
	u8  link_no=0;
	u8 name[] = "@mcu#";
	
	#define TEST_LOCAL_PORT  		0	//local port=0 will generate a random local port each time fo connection. To avoid the rejection by TCP server due to repeative connection with the same ip:port

	#define TEST_REMOTE_ADDR    	"49.234.68.36"  
	#define TEST_REMOTE_PORT  	    8193						


//	 Setup Connection and Config connection upon neccessary (Chinese: 创建套接字，以及必要时对套接字的一些配置)

	M8266WIFI_SPI_Config_Tcp_Window_num(link_no, 4, NULL);
	
	//step 1: setup connection (Chinese: 步骤1：创建套接字连接）

	//  u8 M8266WIFI_SPI_Setup_Connection(u8 tcp_udp, u16 local_port, char remote_addr, u16 remote_port, u8 link_no, u8 timeout_in_s, u16* status);
	
	while(M8266WIFI_SPI_Setup_Connection(1, TEST_LOCAL_PORT, TEST_REMOTE_ADDR, TEST_REMOTE_PORT, link_no, 30, &status)==0)
	{	
//		printf("err:0x%x\n", status);
		printf("connection failed!!\r\n");
		delay_ms(1000);	//	after 2s reconnect
		delay_ms(1000);
	}
	M8266WIFI_SPI_Send_Data(name, sizeof(name), 0, &status);
	
}


