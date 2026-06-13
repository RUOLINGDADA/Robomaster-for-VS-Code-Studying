#ifndef PID_H
#define PID_H
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

typedef enum { PID_MODE_POSITION = 0, PID_MODE_DELTA } PID_ModeTypeDef;
typedef enum { PID_OK = 0, PID_ERROR, PID_NO_PARAMETER } PID_StatusTypeDef;
typedef struct PID_HandleTypeDef PID_HandleTypeDef;

PID_HandleTypeDef *PID_Create(void);
PID_StatusTypeDef PID_Delete(PID_HandleTypeDef *hpid);
PID_StatusTypeDef PID_Init(PID_HandleTypeDef *hpid, float kp, float ki,
                           float kd);
PID_StatusTypeDef PID_DeInit(PID_HandleTypeDef *hpid);
float PID_Calc(PID_HandleTypeDef *hpid, PID_ModeTypeDef mode, float targetValue,
               float measureValue);
#endif
