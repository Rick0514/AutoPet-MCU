#ifndef __DHT11_H
#define __DHT11_H
#include "sys.h"
#include "delay.h"

struct dht11_data{
	u8 temp;
	u8 hum;
};

void dht11_init(void);

void mode_input(void);
void mode_output(void);

u16 dht11_read(void);
unsigned char get_dht11_data(struct dht11_data *data, u16 val);

#endif

