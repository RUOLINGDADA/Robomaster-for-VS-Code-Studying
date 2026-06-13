#include "pid.h"

#define MAX_PID_INSTANCE 8
#define TEMPERATURE_PID_MAX_OUT 4500.0f
#define TEMPERATURE_PID_MAX_IOUT 4400.0f
#define PID_INVALID_INDEX MAX_PID_INSTANCE

struct PID_HandleTypeDef {
  float kp;
  float ki;
  float kd;
  float iout;      // 用于积分累积
  float delta_out; // 用于增量pid模式的out累加
  float error[3];  // 0:当前误差 1:上一次误差 2:上上次误差
};

// 静态内存池，所有 PID 句柄都从这里分配，不使用 malloc
static PID_HandleTypeDef g_pid_pool[MAX_PID_INSTANCE];
// 一个结构体数组存储已定义的PID句柄
static PID_HandleTypeDef *g_pid_instance[MAX_PID_INSTANCE] = {NULL};
// 一个布尔数组标记槽位是否使用
static bool g_pid_ins_slot[MAX_PID_INSTANCE];

static float PID_Clamp(float value, float min, float max) {
  if (value > max) {
    return max;
  }
  if (value < min) {
    return min;
  }
  return value;
}

static bool isInsRegisted(PID_HandleTypeDef *hpid) {
  if (hpid == NULL) {
    return false;
  }
  for (uint8_t i = 0U; i < MAX_PID_INSTANCE; i++) {
    if (g_pid_instance[i] == hpid) {
      return true;
    }
  }
  return false;
}

static uint8_t FindPidInsIndex(PID_HandleTypeDef *hpid) {
  if (hpid == NULL) {
    return PID_INVALID_INDEX;
  }
  if (isInsRegisted(hpid)) {
    for (uint8_t i = 0U; i < MAX_PID_INSTANCE; i++) {
      if (g_pid_instance[i] == hpid) {
        return i;
      }
    }
  }
  return PID_INVALID_INDEX;
}

static PID_StatusTypeDef PID_RegInstance(PID_HandleTypeDef *hpid) {
  if (hpid == NULL) {
    return PID_NO_PARAMETER;
  }
  for (uint8_t i = 0U; i < MAX_PID_INSTANCE; i++) {
    if (!g_pid_ins_slot[i]) {
      g_pid_instance[i] = hpid;
      g_pid_ins_slot[i] = true;
      return PID_OK;
    }
  }
  return PID_ERROR;
}

PID_StatusTypeDef PID_Init(PID_HandleTypeDef *hpid, float kp, float ki,
                           float kd) {
  if (hpid == NULL) {
    return PID_NO_PARAMETER;
  }
  hpid->kp = kp;
  hpid->ki = ki;
  hpid->kd = kd;
  hpid->iout = 0.0f;
  hpid->delta_out = 0.0f;
  hpid->error[0] = 0.0f;
  hpid->error[1] = 0.0f;
  hpid->error[2] = 0.0f;
  return PID_OK;
}

PID_StatusTypeDef PID_DeInit(PID_HandleTypeDef *hpid) {
  if (hpid == NULL) {
    return PID_NO_PARAMETER;
  }
  hpid->kp = 0.0f;
  hpid->ki = 0.0f;
  hpid->kd = 0.0f;
  hpid->iout = 0.0f;
  hpid->delta_out = 0.0f;
  hpid->error[0] = 0.0f;
  hpid->error[1] = 0.0f;
  hpid->error[2] = 0.0f;
  return PID_OK;
}

PID_StatusTypeDef PID_Delete(PID_HandleTypeDef *hpid) {
  if (hpid == NULL) {
    return PID_NO_PARAMETER;
  }
  if (!isInsRegisted(hpid)) {
    return PID_ERROR;
  } else {
    uint8_t index = FindPidInsIndex(hpid);
    if (index >= MAX_PID_INSTANCE) {
      return PID_ERROR;
    }
    PID_DeInit(hpid);
    g_pid_instance[index] = NULL;
    g_pid_ins_slot[index] = false;
    return PID_OK;
  }
}

PID_HandleTypeDef *PID_Create(void) {
  // 静态内存池分配
  for (uint8_t i = 0U; i < MAX_PID_INSTANCE; i++) {
    if (!g_pid_ins_slot[i]) {
      // 从池子里取一个未占用的地址并初始化
      PID_HandleTypeDef *hpid = &g_pid_pool[i];
      PID_DeInit(hpid);
      if (PID_RegInstance(hpid) == PID_OK) {
        return hpid; // 返回的地址即为从池子中取的地址
      }
      return NULL;
    }
  }
  return NULL;
}

float PID_Calc(PID_HandleTypeDef *hpid, PID_ModeTypeDef mode, float targetValue,
               float measureValue) {
  if (hpid == NULL) {
    return 0.0f;
  }
  float output = 0.0f;
  hpid->error[2] = hpid->error[1];
  hpid->error[1] = hpid->error[0];
  hpid->error[0] = targetValue - measureValue;
  switch (mode) {
  case PID_MODE_POSITION:
    hpid->iout += hpid->ki * hpid->error[0];
    hpid->iout = PID_Clamp(hpid->iout, 0.0f, TEMPERATURE_PID_MAX_IOUT);
    output = hpid->kp * hpid->error[0] + hpid->iout +
             hpid->kd * (hpid->error[0] - hpid->error[1]);
    output = PID_Clamp(output, 0.0f, TEMPERATURE_PID_MAX_OUT);
    return output;
  case PID_MODE_DELTA:
    hpid->delta_out += hpid->kp * (hpid->error[0] - hpid->error[1]) +
                       hpid->ki * hpid->error[0] +
                       hpid->kd * ((hpid->error[0] - hpid->error[1]) -
                                   (hpid->error[1] - hpid->error[2]));
    hpid->delta_out =
        PID_Clamp(hpid->delta_out, 0.0f, TEMPERATURE_PID_MAX_OUT);
    return hpid->delta_out;
  default:
    break;
  }
  return 0.0f;
}
