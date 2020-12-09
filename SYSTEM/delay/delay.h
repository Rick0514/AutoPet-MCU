#ifndef __DELAY_H
#define __DELAY_H 			   
#include "sys.h"  
	 


void delay_init(void);
void delay_ms(uint16_t nms);
void delay_us(uint32_t nus);
void delay_ostimedly(uint32_t ticks);

#endif

/*
void delay_init(u8 SYSCLK);
void delay_ms(u16 nms);
void delay_us(u32 nus);

#endif

*/



