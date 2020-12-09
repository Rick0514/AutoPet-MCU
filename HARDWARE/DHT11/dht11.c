#include "dht11.h"

#define CHECK_TIME 28

// PC7 for data bus

void dht11_init(void)
{
  GPIO_InitTypeDef GPIO_InitStructure;
  /* Enable  clock */
  RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOC, ENABLE);
  
  /* Configure Ports */
  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_7;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_2MHz;
  GPIO_Init(GPIOC, &GPIO_InitStructure);
 
  GPIO_SetBits(GPIOC, GPIO_Pin_7);
}
 
void mode_input(void)
{
  GPIO_InitTypeDef GPIO_InitStructure;
 
  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_7;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_2MHz;
  GPIO_Init(GPIOC, &GPIO_InitStructure);
}
 
void mode_output(void)
{
  GPIO_InitTypeDef GPIO_InitStructure;
 
  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_7;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_2MHz;
  GPIO_Init(GPIOC, &GPIO_InitStructure);
}


u16 dht11_read(void)
{
	int i;
	long long val;
	int timeout;
	u16 data;

	GPIO_ResetBits(GPIOC, GPIO_Pin_7);
	delay_ms(18);  //pulldown  for 18ms
	GPIO_SetBits(GPIOC, GPIO_Pin_7);
	delay_us( 20 );	//pullup for 30us

	mode_input();

	//等待dht11拉高80us
	timeout = 5000;
	while( (! GPIO_ReadInputDataBit  (GPIOC, GPIO_Pin_7)) && (timeout > 0) ) timeout--;	 //wait HIGH

	//等待dht11拉低80us
	timeout = 5000;
	while( GPIO_ReadInputDataBit (GPIOC, GPIO_Pin_7) && (timeout > 0) ) timeout-- ;	 //wait LOW

	for(i=0;i<40;i++)
	{
		timeout = 5000;
		while( (! GPIO_ReadInputDataBit  (GPIOC, GPIO_Pin_7)) && (timeout > 0) ) timeout--;	 //wait HIGH

		delay_us(CHECK_TIME);
		if ( GPIO_ReadInputDataBit (GPIOC, GPIO_Pin_7) )
		{
		  val=(val<<1)+1;
		} else {
		  val<<=1;
		}

		timeout = 5000;
		while( GPIO_ReadInputDataBit (GPIOC, GPIO_Pin_7) && (timeout > 0) ) timeout-- ;	 //wait LOW
	}

	mode_output();
	GPIO_SetBits(GPIOC, GPIO_Pin_7);

	if (((val>>32)+(val>>24)+(val>>16)+(val>>8) -val ) & 0xff  ) return 0; //校验
	else {
		data = (val >> 16) & 0xff;
		data = (data << 8) | ((val >> 32) & 0xff);
		return data;
	} 
 
}

unsigned char get_dht11_data(struct dht11_data *data, u16 val){
	if(val == 0)	//data went wrong
	{
		return 0;
	}
		
	data->hum = (val & 0x00ff);
	data->temp = ((val & 0xff00) >> 8);
	
	return 1;

}

