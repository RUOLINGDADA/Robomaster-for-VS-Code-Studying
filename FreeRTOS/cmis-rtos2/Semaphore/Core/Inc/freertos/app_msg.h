#ifndef __APP_MSG_H
#define __APP_MSG_H

#include "cmsis_os2.h"
#include <stdint.h>

#define APP_MSG_POOL_SIZE 32U

typedef struct {
  uint8_t id;
  const char *name;
  const char *value;
} AppMsg_t;

void AppMsg_Init(void);
AppMsg_t *AppMsg_Alloc(void);
void AppMsg_Free(AppMsg_t *msg);
osStatus_t AppMsg_Send(osMessageQueueId_t queue, AppMsg_t *msg,
                       uint32_t timeout);
osStatus_t AppMsg_Receive(osMessageQueueId_t queue, AppMsg_t **msg,
                          uint32_t timeout);

#endif
