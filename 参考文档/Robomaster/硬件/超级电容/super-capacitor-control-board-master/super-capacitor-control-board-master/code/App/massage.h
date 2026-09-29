#ifndef _MASSAGE_H_
#define _MASSAGE_H_
#include "main.h"
#include "can.h"
#include "POWER.h"
#define CAN_ID1  0x02F
#define CAN_ID2  0x02E
#define CAN_ID3  0x000
#define CAN_ID4  0x000

#define CAN_ID_LIST_SIZE    36
#define CAN_ID_TIMEOUT      500
typedef struct
{
    uint16_t ID;
    uint32_t rx_timestamp;
} Can_ID_t;
typedef struct 
{
    Can_ID_t *list; // 列表指针
    uint16_t size;  // 列表大小
    uint16_t rear;  // 队尾位置
}Can_ID_List_t;

extern Can_ID_t can_id_array[CAN_ID_LIST_SIZE];
extern Can_ID_List_t can_id_list;

uint16_t Get_CAN_ID_Num(Can_ID_List_t *can_id_list);
uint16_t Get_CAN_ID_List_Elem_ID(Can_ID_List_t *can_id_list,uint16_t num);
uint16_t Get_CAN_ID_List_Elem_Timestamp(Can_ID_List_t *can_id_list,uint16_t num);
void CAN_ID_Enlist(Can_ID_List_t *can_id_list,uint16_t CAN_ID);
void Can_ID_Delist(Can_ID_List_t *can_id_list);
void Can_ID_Sort(Can_ID_List_t *can_id_list);


typedef union 
{
    uint8_t value;
    struct 
    {
        FunctionalState CupEnable : 1;
        Power_Limit_mode_t power_limit_mode : 2;
        FunctionalState RESERVER : 5;
    } flag;
} CanRxStatus_t;

void CAN_Filter_Config(void);
void CAN_Send_Message();
void Can_Receive_Massage(CAN_RxHeaderTypeDef RxMessage,uint8_t data[8]);

#endif