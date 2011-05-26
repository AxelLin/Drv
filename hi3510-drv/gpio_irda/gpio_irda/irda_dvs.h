#ifndef  _INC_IRKEY_H
#define  _INC_IRKEY_H

#define IRKEY_PORT          GPIO_2 
#define IRKEY_PORT_ADDR     GPIO_2_BASE 
#define IRKEY_RX_PIN        5
#define IRKEY_RX_ADDR       (GPIO_BASE + IRKEY_PORT  * GPIO_SPACE_SIZE)+ 0x080
#define IRKEY_START_MASK    0x20
#define IRKEY_DIRBYTE       0xdf

#define DEVICE_MAJOR  249
#define DEVICE_NAME  "dvs_irkey"
#define MAX_BUF 8
#define DEVICE_IRQ_NO  8


typedef struct
{
    unsigned int sys_id_code;
    unsigned int irkey_code;
    unsigned int irkey_mask_code;
}Irkey_Info;

typedef struct 
{
    unsigned int head;
    unsigned int tail;
    Irkey_Info buf[MAX_BUF];
    spinlock_t irkey_lock;
    wait_queue_head_t irkey_wait;
}Irkey_Dev;

#endif 

