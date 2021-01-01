#ifndef __BOARD_H
#define __BOARD_H

#include "stm32f10x.h"
#define TEST_LOCAL_PORT  		0	//local port=0 will generate a random local port each time fo connection. To avoid the rejection by TCP server due to repeative connection with the same ip:port
// change to your own server ip and port
#define TEST_REMOTE_ADDR    	"xxx.xxx.xxx.xxx"  
#define TEST_REMOTE_PORT  	    xxxx	

void rt_hw_board_init(void);
void SysTick_Handler(void);
void setup_TCP(void);

#endif 

