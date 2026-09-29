#include "massage.h"
#include "stdlib.h"
#include "sample.h"

Can_ID_t can_id_array[CAN_ID_LIST_SIZE];
Can_ID_List_t can_id_list =
	{
		can_id_array,
		CAN_ID_LIST_SIZE,
		0};

/// @brief 获得canID队列长度
/// @param can_id_list canID列表
/// @return canID队列长度
uint16_t Get_CAN_ID_Num(Can_ID_List_t *can_id_list)
{
	return can_id_list->rear;
}
/// @brief 获取canID队列元素ID
/// @param can_id_list canID列表
/// @param num 位置
/// @return 该序号所在的CANID值
uint16_t Get_CAN_ID_List_Elem_ID(Can_ID_List_t *can_id_list,uint16_t num)
{
	if(num >= can_id_list->rear)
	{
		return 0;
	}
	return can_id_list->list[num].ID;
}
/// @brief 获取canID队列元素接收时间戳
/// @param can_id_list canID列表
/// @param num 位置
/// @return 该序号所在的接收时间戳
uint16_t Get_CAN_ID_List_Elem_Timestamp(Can_ID_List_t *can_id_list,uint16_t num)
{
	if(num >= can_id_list->rear)
	{
		return 0;
	}
	return can_id_list->list[num].rx_timestamp;
}
/// @brief canID进队
/// @param can_id_list canID列表
/// @param CAN_ID canID值
void CAN_ID_Enlist(Can_ID_List_t *can_id_list, uint16_t CAN_ID)
{
	uint16_t temp = 0;
	ErrorStatus check_flag = ERROR;
	// 首先遍历是否在列表中
	for (int i = 0; i < can_id_list->rear; i++)
	{
		//更新时间戳
		if (can_id_list->list[i].ID == CAN_ID)
		{
			can_id_list->list[i].rx_timestamp = HAL_GetTick();
			return;
		}
	}
	// 查找不到进行添加
	temp = 0;
	if (can_id_list->rear >= can_id_list->size)
	{
		// can列表满 查找最久数据进行替代
		for (int i = 0; i < can_id_list->size; i++)
		{
			if (can_id_list->list[temp].rx_timestamp < can_id_list->list[i].rx_timestamp)
			{
				temp = i;
			}
		}
	}
	else
	{
		// 未满直接插入
		temp = can_id_list->rear++;
	}
	can_id_list->list[temp].ID = CAN_ID;
	can_id_list->list[temp].rx_timestamp = HAL_GetTick();
}
/// @brief 将超时的canid退队
/// @param can_id_list
void Can_ID_Delist(Can_ID_List_t *can_id_list)
{
	uint16_t temp = 0;
	// 对can列表进行遍历
	for (int i = 0; i < can_id_list->rear; i++)
	{
		// 按照删除个数对数据进行向前移位
		can_id_list->list[i - temp].ID = can_id_list->list[i].ID;
		can_id_list->list[i - temp].rx_timestamp = can_id_list->list[i].rx_timestamp;
		// 超时删除个数+1
		if (HAL_GetTick() - can_id_list->list[i].rx_timestamp >= CAN_ID_TIMEOUT)
		{
			temp++;
		}
	}
	// 更新队列大小
	can_id_list->rear -= temp;
}
/// @brief 比较ab的id大小
/// @return a-b的值
static int32_t Can_ID_Cmp(Can_ID_t *a, Can_ID_t *b)
{
	return ((int32_t)a->ID - (int32_t)b->ID);
}

/// @brief 按照ID对can列表进行排序
/// @param can_id_list CAN列表指针
void Can_ID_Sort(Can_ID_List_t *can_id_list)
{
	qsort(can_id_list->list, can_id_list->rear, sizeof(Can_ID_t), Can_ID_Cmp);
}



/// @brief CAN邮箱过滤配置
/// @param  
void CAN_Filter_Config(void)
{
	CAN_FilterTypeDef sFilterConfig;

	sFilterConfig.FilterBank           = 0;								// 设置过滤器组编号
	sFilterConfig.FilterMode           = CAN_FILTERMODE_IDLIST;			// 列表模式
    sFilterConfig.FilterScale          = CAN_FILTERSCALE_16BIT;			// 16位宽
    sFilterConfig.FilterMaskIdHigh     = CAN_ID1<<5;					// 掩码符寄存器ID高十六位
    sFilterConfig.FilterMaskIdLow      = CAN_ID2<<5;					// 掩码符寄存器ID低十六位
    sFilterConfig.FilterIdHigh         = CAN_ID3<<5;					// 标识符寄存器ID高十六位
    sFilterConfig.FilterIdLow          = CAN_ID4<<5;					// 标识符寄存器ID低十六位
    sFilterConfig.FilterFIFOAssignment = CAN_RX_FIFO0;					// 过滤器组关联到FIFO0
    sFilterConfig.FilterActivation     = ENABLE;						// 激活过滤器
    if (HAL_CAN_ConfigFilter(&hcan, &sFilterConfig) != HAL_OK)
    {
        Error_Handler();
    }
	if (HAL_CAN_ActivateNotification(&hcan, CAN_IT_RX_FIFO0_MSG_PENDING) != HAL_OK) {	// 开启CAN1的FIFO0接收中断
		Error_Handler();
	}
	sFilterConfig.FilterBank           = 1;							// 设置过滤器组编号
	sFilterConfig.FilterMode           = CAN_FILTERMODE_IDMASK;		// 掩码模式
    sFilterConfig.FilterScale          = CAN_FILTERSCALE_16BIT;		// 16位宽
    sFilterConfig.FilterMaskIdHigh     = 0x00<<5;					// 掩码符寄存器ID高十六位
    sFilterConfig.FilterMaskIdLow      = 0x00<<5;					// 掩码符寄存器ID低十六位
    sFilterConfig.FilterIdHigh         = 0x00<<5;					// 标识符寄存器ID高十六位
    sFilterConfig.FilterIdLow          = 0x00<<5;					// 标识符寄存器ID低十六位
    sFilterConfig.FilterFIFOAssignment = CAN_RX_FIFO1;				// 过滤器组关联到FIFO1
	if (HAL_CAN_ConfigFilter(&hcan, &sFilterConfig) != HAL_OK)
    {
        Error_Handler();
    }
	if (HAL_CAN_ActivateNotification(&hcan, CAN_IT_RX_FIFO1_MSG_PENDING) != HAL_OK) {	// 开启CAN1的FIFO0接收中断
		Error_Handler();
	}

	if (HAL_CAN_Start(&hcan) != HAL_OK) {							// 开启CAN
		Error_Handler();
	}
}

/// @brief CAN发送报文
__attribute__((section("ccmram"))) void CAN_Send_Message()
{
    uint8_t tx_data[8] = {0};
    CAN_TxHeaderTypeDef TxMessage;
    uint32_t Tx_Mailbox;
	// 对发送报文结构体TxMessage里的成员赋值
	TxMessage.StdId = 0x030;		   // 标准帧ID
	TxMessage.IDE = CAN_ID_STD;        // 标准模式
	TxMessage.RTR = CAN_RTR_DATA;      // 发送的是数据帧
	TxMessage.DLC = 8;			       // 数据长度为8字节
	
    int32_t Vc = Vcap.ave_value*100;
    int32_t Pc = (Power_limit.data.Pin-Power_limit.data.Pout)*100;
    int32_t Po = Power_limit.data.Pout*100;
	int32_t Vo = Vout.ave_value * 100;
	tx_data[0] = (uint8_t)(Vc >> 8);
	tx_data[1] = (uint8_t)Vc;
	tx_data[2] = (uint8_t)(Pc >> 8);
	tx_data[3] = (uint8_t)Pc;
	tx_data[4] = (uint8_t)(Po >> 8);
	tx_data[5] = (uint8_t)Po;
	tx_data[6] = (uint8_t)Power.status.value;
	tx_data[7] = (uint8_t)Vo;

	HAL_CAN_AddTxMessage(&hcan,&TxMessage,tx_data,&Tx_Mailbox); //CAN发送数据
}

void Can_Receive_Massage(CAN_RxHeaderTypeDef RxMessage,uint8_t data[8])
{
	static CanRxStatus_t RxStatus;
	float Icharge_max;
	switch (RxMessage.StdId)
	{
	case 0x02f://接收设置报文
	
		Power_limit.data.Pset 					= data[0];
		Power_limit.config_data.Icompensate_max = data[2] / Vout.ave_value * Vcap.ave_value;
		Icharge_max 							= data[4] / Vout.ave_value * Vcap.ave_value;
		RxStatus.value 							= data[6];

		Power.status.flag.CupEnable = RxStatus.flag.CupEnable;
		Power_limit.status.flag.power_limit_mode = RxStatus.flag.power_limit_mode;
		
		Power_limit.config_data.Icharge_max = Constrain_float(Icharge_max,CHARGE_I_MAX,0);
		break;
	case 0x02e://接收数据报文
		//更新时间戳
		Power_limit.data.power_buffer 			= (data[0] << 8 | data[1])/100.0f;
		Power_limit.timestamp.rx_timestamp 	= HAL_GetTick();
		break;
	default:
		break;
  }
  //CAN_ID_Enlist(&can_id_list, RxMessage.StdId);
}
