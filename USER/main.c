#include "stdio.h"
#include "sys.h"
#include "misc.h"
#include "M8266HostIf.h"
#include "M8266WIFIDrv.h"
#include "M8266WIFI_ops.h"
#include "brd_cfg.h"
#include "usart.h"
#include "rtthread.h"
#include "board.h"
#include "delay.h"
#include "pwm.h"
#include "dht11.h"

#define TIME_SLICE 10
#define THREAD_PRO 5

struct task
{
	u8 feed_dog;
	u8 get_ws;
};

static struct dht11_data wsdata;

static struct task myTask = {'a', 'b'};

ALIGN(RT_ALIGN_SIZE)
static struct rt_thread check_link_thread;
static rt_uint8_t check_link_thread_stack[640];

ALIGN(RT_ALIGN_SIZE)
static rt_uint8_t rt_rec_thread_stack[512];
static struct rt_thread rec_thread;

ALIGN(RT_ALIGN_SIZE)
static rt_uint8_t rt_snd_thread_stack[512];
static struct rt_thread snd_thread;
//// define thread entry func
static void rec_thread_entry(void *parameter);
static void snd_thread_entry(void *parameter);
static void check_link_entry(void *parameter);

/* ������ƿ� */
static struct rt_mailbox mb;
/* ���ڷ��ʼ����ڴ�� */
static char mb_pool[16];

static rt_mutex_t lock = RT_NULL;

int main(void)
{ 
	
	rt_err_t result;
	
	rt_kprintf("get in main func! \n");
    /* ��ʼ��һ�� mailbox */
    result = rt_mb_init(&mb,
                        "mbt",                      /* ������ mbt */
                        &mb_pool[0],                /* �����õ����ڴ���� mb_pool */
                        sizeof(mb_pool) / 4,        /* �����е��ʼ���Ŀ����Ϊһ���ʼ�ռ 4 �ֽ� ==> 16 mails*/
                        RT_IPC_FLAG_FIFO);          /* ���� FIFO ��ʽ�����̵߳ȴ� */
						
    if (result != RT_EOK)
    {
        rt_kprintf("init mailbox failed.\n");
        return -1;
    }
	
	lock = rt_mutex_create("dmutex", RT_IPC_FLAG_FIFO);
    if (lock == RT_NULL)
    {
        rt_kprintf("create dynamic mutex failed.\n");
        return -1;
    }
	
	// init rec thread and run it 
	rt_thread_init(&rec_thread,
				   "rec",
				   rec_thread_entry,
				   RT_NULL,
				   &rt_rec_thread_stack[0],
				   sizeof(rt_rec_thread_stack),
				   THREAD_PRO,
				   TIME_SLICE);
    
				 
	// init snd thread and run it
	rt_thread_init(&snd_thread,
				   "snd",
				   snd_thread_entry,
				   RT_NULL,
				   &rt_snd_thread_stack[0],
				   sizeof(rt_snd_thread_stack),
				   THREAD_PRO,
				   TIME_SLICE);
				   
	// init check_link thread and run it
	rt_thread_init(&check_link_thread,
				   "check_link",
				   check_link_entry,
				   RT_NULL,
				   &check_link_thread_stack[0],
				   sizeof(check_link_thread_stack),
				   THREAD_PRO-1,
				   TIME_SLICE);
    
	// run threads 
	rt_thread_startup(&check_link_thread);
	rt_thread_startup(&rec_thread);
    rt_thread_startup(&snd_thread);
	
	return 0;
		   
} //end of main 

static void rec_thread_entry(void *parameter){
	
	u8 link_no = 0;
	u16 status = 0;
	u8 RecvData[16];
	
	while(1)
	{
		if(M8266WIFI_SPI_Has_DataReceived())
		{
			//u16 M8266WIFI_SPI_RecvData(u8 data[], u16 max_len, uint16_t max_wait_in_ms, u8* link_no, u16* status);
			rt_memset( RecvData, 0, sizeof(RecvData));
			rt_enter_critical();
			M8266WIFI_SPI_RecvData(RecvData, 16, 50, &link_no, &status);
			rt_exit_critical();
			
			if(  (status&0xFF)!= 0 )  
			{	
				rt_thread_mdelay(200);
				continue;
			}	

			// analyse the recdata and send to snd_thread
			if((RecvData[0] == 'f') && (RecvData[1] == 'd'))	rt_mb_send(&mb, (rt_uint32_t)myTask.feed_dog);
			if((RecvData[0] == 'w') && (RecvData[1] == 's'))	rt_mb_send(&mb, (rt_uint32_t)myTask.get_ws);
						
		
		} // end of if(M8266WIFI_SPI_Has_DataReceived())	
		rt_thread_mdelay(100);
	} // end of 

}

static void snd_thread_entry(void *parameter)
{
	char cmd;
	u16 loops     = 0;
	u16 max_loops = 100;
	u32 len       = 16; 
	u8 snd_data[16];
	u32 sent;
	u8 link_no = 0;
	u16 status = 0;
	
	u16 val;
	
	while(1){
		/* ����������ȡ�ʼ� */
        if (rt_mb_recv(&mb, (rt_uint32_t *)&cmd, RT_WAITING_FOREVER) == RT_EOK)
        {
           
            if(cmd == myTask.feed_dog)
			{
//				rt_kprintf("feed the dog\n");
				setOpenAngle(45);
				rt_thread_mdelay(1000);
				setOpenAngle(0);
			
			}
			else if(cmd == myTask.get_ws)
			{
//				rt_kprintf("indoors 25 deg\n");
				rt_memset(snd_data, 0, len);
				rt_mutex_take(lock, RT_WAITING_FOREVER);
				val = dht11_read();
				rt_mutex_release(lock);
				
				if(get_dht11_data(&wsdata, val)){
					rt_sprintf((char *)snd_data, "w%d\ns%d", wsdata.temp, wsdata.hum);
					// rt_kprintf("temp: %d, tum: %d\n", wsdata.temp, wsdata.hum);
				// }else	rt_kprintf("can not read\n");
				
				for(; loops <= max_loops; loops++){
					sent = M8266WIFI_SPI_Send_Data(snd_data,len, link_no, &status);
					if( (sent==len) && ((status&0xFF)==0x00) ) //Send successfully 
					{
						max_loops = 100;
						break;
					}
					else if( (status&0xFF) == 0x1E)	 // 0x1E = too many errors ecountered during sending and can not fixed, or transsmission blocked heavily(Chinese: ���ͽ׶�����̫��Ĵ���������ˣ����Կ��ǼӴ�max_loops)
					{
						max_loops = ((max_loops >= 2000) ? 2000 : (max_loops + 100));
						//add some process here (Chinese: �����ڴ˴���һЩ��������������max_loops��ֵ)
					}
					rt_thread_mdelay(10);
				}
			}
			else
			{
				rt_kprintf("no such cmd\n");
				rt_thread_mdelay(10);
			}
                
        }
		rt_thread_mdelay(10);
	}
	
}

static void check_link_entry(void *parameter)
{
	// check the ap and tcp every minute
	u8 connection_status = 0xff;
	u8 ssid[] = "gy_bluetooth";
	u8 pwd[] = "12345678";
	u16 status = 0;
	char sta_ip[16] = {0};
	int ret;
	u8 beat[] = "beat";
	while(1)
	{
		M8266WIFI_SPI_Get_STA_Connection_Status(&connection_status, &status);
		
		if(connection_status!=5)
		{
			rt_kprintf("lost AP\n");
			M8266WIFI_SPI_STA_Connect_Ap(ssid, pwd, 0, 5, &status);
			rt_thread_suspend(&rec_thread); rt_thread_suspend(&snd_thread);
			while(M8266WIFI_SPI_wait_sta_connecting_to_ap_and_get_ip(sta_ip, 10)==0) // max wait 10s to get sta ip 
			{
				printf("check the ap is open and retry to connect ap\r\n");
				delay_ms(1000);
				M8266WIFI_SPI_STA_Connect_Ap(ssid, pwd, 0, 5, &status);
			}
			rt_thread_resume(&rec_thread); rt_thread_resume(&snd_thread);
		}
		ret = M8266WIFI_SPI_Query_Connection(0, NULL, &connection_status, NULL, NULL, NULL, &status);
		
		if(ret == 1){
			if((connection_status < 3) || (connection_status >= 6))
			{
				rt_kprintf("lost TCP\n");
				rt_thread_suspend(&rec_thread); rt_thread_suspend(&snd_thread);
				M8266WIFI_SPI_Get_STA_Connection_Status(&connection_status, &status);
				if(connection_status!=5)
				{
					rt_kprintf("lost AP\n");
					while(M8266WIFI_SPI_wait_sta_connecting_to_ap_and_get_ip(sta_ip, 10)==0) // max wait 10s to get sta ip 
					{
						printf("check the ap is open and retry to connect ap\r\n");
						delay_ms(1000);
					}
				}
				setup_TCP();
				rt_thread_resume(&rec_thread); rt_thread_resume(&snd_thread);
			}
				
		}
		M8266WIFI_SPI_Send_Data(beat, 5, 0, &status);
		rt_thread_mdelay(2000);
		
	}
	
		
}

