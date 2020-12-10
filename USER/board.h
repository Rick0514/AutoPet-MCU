#ifndef __BOARD_H
#define __BOARD_H

#include "stm32f10x.h"
#include "led.h"

void rt_hw_board_init(void);
void SysTick_Handler(void);
void setup_TCP(void);

#endif 

