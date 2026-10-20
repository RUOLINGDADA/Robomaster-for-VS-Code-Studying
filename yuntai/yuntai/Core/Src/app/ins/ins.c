/**
 * @file ins.c
 * @brief 云台安装变换、连续静止零偏和快照发布，运行在唯一 INS 任务。
 *
 * 采集由任务运行时负责。短临界区只复制快照，不做浮点计算或 I2C/SPI。
 */
#include "app/ins/ins.h"
#include "FreeRTOS.h"
#include "task.h"
#include "stm32f4xx_hal.h"
#include <math.h>
#include <string.h>

#define INS_RAD_TO_DEG 57.29577951308232f /* rad 转 degree。 */

typedef struct {
  Ins_ConfigTypeDef config; /* 唯一任务的只读配置。 */
  Ahrs_HandleTypeDef ahrs; /* 所有者任务私有融合状态。 */
  Ins_SnapshotTypeDef current; /* 任务正在构建的候选快照。 */
  float mean_gyro[3]; /* Welford 均值，机体系 rad/s。 */
  float m2_gyro[3]; /* Welford 方差分子，(rad/s)²。 */
  float mean_accel[3]; /* 启动比力均值，m/s²。 */
  uint32_t still_start_ms; /* 连续静止窗口起点，HAL ms。 */
  uint32_t still_count; /* 窗口内陀螺样本数。 */
  bool configured; /* Init 通过全部配置验证。 */
} Ins_RuntimeTypeDef;

static Ins_RuntimeTypeDef g_ins; /* 仅 INS 任务访问。 */
static Ins_SnapshotTypeDef g_snapshot; /* 多任务只通过临界区复制。 */
static bool g_published; /* 与 g_snapshot 在同一临界区访问。 */
static uint32_t g_gyro_timeout_ms, g_accel_timeout_ms, g_mag_timeout_ms; /* Init 写入，GetSnapshot 临界区读取。 */

/* 检查向量有限性，避免把 NaN 送入标定统计。 */
static bool Ins_Finite3(const float v[3]) {
  return isfinite(v[0]) && isfinite(v[1]) && isfinite(v[2]);
}

/* 验证软铁矩阵对称正定；拟合工具输出此形式，不允许镜像或负缩放。 */
static bool Ins_SoftironValid(const float s[3][3]) {
  for (unsigned i=0U;i<3U;++i) {
    if (!Ins_Finite3(s[i])) { return false; }
    for (unsigned j=0U;j<3U;++j) { if (fabsf(s[i][j]-s[j][i])>0.001f) { return false; } }
  }
  float minor=s[0][0]*s[1][1]-s[0][1]*s[1][0];
  float det=s[0][0]*(s[1][1]*s[2][2]-s[1][2]*s[2][1])-
      s[0][1]*(s[1][0]*s[2][2]-s[1][2]*s[2][0])+s[0][2]*(s[1][0]*s[2][1]-s[1][1]*s[2][0]);
  return s[0][0]>0.0f && isfinite(minor) && minor>0.0f && isfinite(det) && det>0.0f;
}

/* 候选快照一次提交；getter 即使任务后来停止运行也会再次检查年龄。 */
static void Ins_Publish(void) {
  ++g_ins.current.sequence;
  taskENTER_CRITICAL();
  g_snapshot=g_ins.current;
  g_published=true;
  taskEXIT_CRITICAL();
}

/* 窗口计数为零即表示需要建立新起点，不依赖时间戳是否为零。 */
static void Ins_ClearCalibration(void) {
  g_ins.still_count=0U;
  memset(g_ins.mean_gyro,0,sizeof(g_ins.mean_gyro));
  memset(g_ins.m2_gyro,0,sizeof(g_ins.m2_gyro));
  memset(g_ins.mean_accel,0,sizeof(g_ins.mean_accel));
}

bool Ins_Init(const Ins_ConfigTypeDef *config) {
  if (config==NULL || !Ahrs_IsRotation(config->bmi_rotation) || !Ahrs_IsRotation(config->mag_rotation) ||
      !Ins_Finite3(config->mag_bias_ut) || !Ins_SoftironValid(config->mag_softiron) ||
      config->still_ms==0U || config->still_ms>60000U || config->gyro_timeout_ms==0U ||
      config->accel_timeout_ms==0U || config->mag_timeout_ms==0U ||
      !isfinite(config->still_gyro_rad_s) || config->still_gyro_rad_s<=0.0f ||
      !isfinite(config->still_accel_fraction) || config->still_accel_fraction<=0.0f ||
      !isfinite(config->still_gyro_std_rad_s) || config->still_gyro_std_rad_s<=0.0f) { return false; }
  Ahrs_HandleTypeDef check;
  float level[3]={0.0f,0.0f,-config->ahrs.gravity_mps2};
  if (!Ahrs_Init(&check,&config->ahrs,level,NULL)) { return false; }
  memset(&g_ins,0,sizeof(g_ins));
  g_ins.config=*config;
  g_ins.configured=true;
  g_ins.current.q[0]=1.0f;
  g_ins.current.mount_confirmed=config->mount_confirmed;
  g_ins.current.mag_calibrated=config->mag_calibrated;
  g_ins.current.state=INS_STATE_WAIT_STILL;
  taskENTER_CRITICAL();
  g_published=false;
  g_gyro_timeout_ms=config->gyro_timeout_ms;
  g_accel_timeout_ms=config->accel_timeout_ms;
  g_mag_timeout_ms=config->mag_timeout_ms;
  taskEXIT_CRITICAL();
  Ins_Publish();
  return true;
}

void Ins_SetFault(void) {
  if (!g_ins.configured) { return; }
  g_ins.current.state=INS_STATE_FAULT;
  g_ins.current.mode=INS_MODE_INVALID;
  g_ins.current.attitude_valid=false;
  g_ins.current.mag_used=false;
  g_ins.ahrs.initialized=false;
  Ins_ClearCalibration();
  Ins_Publish();
}

/* 累计完整连续静止窗口，方差不合格也重开；慢速恒定转动仍须操作者保证真正静止。 */
static bool Ins_Calibrate(const Ins_InputTypeDef *input, const float gyro[3]) {
  Ins_RuntimeTypeDef *r=&g_ins;
  float accel_norm=hypotf(hypotf(r->current.accel_mps2[0],r->current.accel_mps2[1]),r->current.accel_mps2[2]);
  float gyro_norm=hypotf(hypotf(gyro[0],gyro[1]),gyro[2]);
  if (!r->current.accel_valid || !isfinite(accel_norm) || !isfinite(gyro_norm) ||
      gyro_norm>r->config.still_gyro_rad_s ||
      fabsf(accel_norm/r->config.ahrs.gravity_mps2-1.0f)>r->config.still_accel_fraction) {
    Ins_ClearCalibration(); return false;
  }
  if (r->still_count==0U) { r->still_start_ms=input->now_ms; }
  ++r->still_count;
  for (unsigned i=0U;i<3U;++i) {
    float delta=gyro[i]-r->mean_gyro[i];
    r->mean_gyro[i]+=delta/(float)r->still_count;
    r->m2_gyro[i]+=delta*(gyro[i]-r->mean_gyro[i]);
    r->mean_accel[i]+=(r->current.accel_mps2[i]-r->mean_accel[i])/(float)r->still_count;
  }
  if ((uint32_t)(input->now_ms-r->still_start_ms)<r->config.still_ms || r->still_count<2U) { return false; }
  for (unsigned i=0U;i<3U;++i) {
    if (r->m2_gyro[i]/(float)(r->still_count-1U)>r->config.still_gyro_std_rad_s*r->config.still_gyro_std_rad_s) {
      Ins_ClearCalibration(); return false;
    }
  }
  if (!Ahrs_Init(&r->ahrs,&r->config.ahrs,r->mean_accel,
                  r->current.mag_valid && r->config.mag_calibrated ? r->current.mag_ut : NULL)) {
    Ins_ClearCalibration(); return false;
  }
  memcpy(r->current.gyro_bias_rad_s,r->mean_gyro,sizeof(r->mean_gyro));
  r->current.state=INS_STATE_READY;
  r->current.yaw_total_deg=0.0f;
  r->current.yaw_deg=0.0f;
  ++r->current.reset_sequence;
  return true;
}

void Ins_Process(const Ins_InputTypeDef *input) {
  if (input==NULL || !g_ins.configured) { return; }
  if (!Ins_Finite3(input->gyro_rad_s) || !isfinite(input->dt_s) || input->dt_s<=0.0f ||
      input->dt_s>g_ins.config.ahrs.max_dt_s ||
      (uint32_t)(input->now_ms-input->gyro_ms)>g_ins.config.gyro_timeout_ms) { Ins_SetFault(); return; }
  Ins_SnapshotTypeDef *s=&g_ins.current;
  s->gyro_ms=input->gyro_ms; s->accel_ms=input->accel_ms; s->mag_ms=input->mag_ms;
  s->dt_s=input->dt_s; s->temperature_c=input->temperature_c;
  s->temperature_valid=input->temperature_valid;
  /* 温度时间由采集层传递，不能把重复缓存温度伪装成每周期新数据。 */
  s->temperature_ms=input->temperature_ms;
  s->dropped_gyro=input->dropped_gyro; s->mag_overruns=input->mag_overruns;
  memcpy(s->accel_raw,input->accel_raw,sizeof(s->accel_raw));
  memcpy(s->gyro_raw,input->gyro_raw,sizeof(s->gyro_raw));
  memcpy(s->mag_raw,input->mag_raw,sizeof(s->mag_raw));
  memcpy(s->mag_chip_ut,input->mag_ut,sizeof(s->mag_chip_ut));
  Ahrs_Transform(g_ins.config.bmi_rotation,input->accel_mps2,s->accel_mps2);
  float gyro[3], centered[3], corrected[3];
  Ahrs_Transform(g_ins.config.bmi_rotation,input->gyro_rad_s,gyro);
  for (unsigned i=0U;i<3U;++i) { centered[i]=input->mag_ut[i]-g_ins.config.mag_bias_ut[i]; }
  Ahrs_Transform(g_ins.config.mag_softiron,centered,corrected);
  Ahrs_Transform(g_ins.config.mag_rotation,corrected,s->mag_ut);
  s->accel_valid=input->accel_valid && Ins_Finite3(s->accel_mps2) &&
      (uint32_t)(input->now_ms-input->accel_ms)<=g_ins.config.accel_timeout_ms;
  s->mag_valid=input->mag_valid && Ins_Finite3(s->mag_ut) &&
      (uint32_t)(input->now_ms-input->mag_ms)<=g_ins.config.mag_timeout_ms;
  bool just_initialized=false;
  if (s->state!=INS_STATE_READY) {
    s->state=INS_STATE_WAIT_STILL;
    just_initialized=Ins_Calibrate(input,gyro);
  }
  for (unsigned i=0U;i<3U;++i) { s->gyro_rad_s[i]=gyro[i]-s->gyro_bias_rad_s[i]; }
  if (s->state==INS_STATE_READY) {
    if (!just_initialized && !Ahrs_Update(&g_ins.ahrs,s->gyro_rad_s,
        s->accel_valid ? s->accel_mps2 : NULL,
        s->mag_valid && g_ins.config.mag_calibrated ? s->mag_ut : NULL,input->dt_s)) { Ins_SetFault(); return; }
    float angles[3];
    Ahrs_GetEuler(&g_ins.ahrs,angles);
    float yaw=angles[0]*INS_RAD_TO_DEG;
    float delta=yaw-s->yaw_deg;
    if (delta>180.0f) { delta-=360.0f; } else if (delta<-180.0f) { delta+=360.0f; }
    s->yaw_total_deg+=delta;
    s->yaw_deg=yaw; s->pitch_deg=angles[1]*INS_RAD_TO_DEG; s->roll_deg=angles[2]*INS_RAD_TO_DEG;
    memcpy(s->q,g_ins.ahrs.q,sizeof(s->q));
    s->mag_used=g_ins.ahrs.mag_used;
    s->mode=s->mag_used ? INS_MODE_NINE_AXIS :
        (g_ins.ahrs.accel_weight>0.0f || just_initialized ? INS_MODE_SIX_AXIS : INS_MODE_GYRO);
    s->attitude_valid=g_ins.config.mount_confirmed && s->accel_valid;
  } else { s->attitude_valid=false; s->mag_used=false; s->mode=INS_MODE_INVALID; }
  Ins_Publish();
}

bool Ins_GetSnapshot(Ins_SnapshotTypeDef *output) {
  if (output==NULL) { return false; }
  uint32_t gyro_timeout,accel_timeout,mag_timeout;
  taskENTER_CRITICAL();
  bool available=g_published;
  *output=g_snapshot;
  gyro_timeout=g_gyro_timeout_ms; accel_timeout=g_accel_timeout_ms; mag_timeout=g_mag_timeout_ms;
  taskEXIT_CRITICAL();
  uint32_t now=HAL_GetTick();
  if ((uint32_t)(now-output->gyro_ms)>gyro_timeout) { output->attitude_valid=false; output->mode=INS_MODE_INVALID; }
  if ((uint32_t)(now-output->accel_ms)>accel_timeout) {
    output->accel_valid=false; output->attitude_valid=false;
    if (output->mode!=INS_MODE_INVALID) { output->mode=INS_MODE_GYRO; }
  }
  if ((uint32_t)(now-output->mag_ms)>mag_timeout) {
    output->mag_valid=false; output->mag_used=false;
    if (output->mode==INS_MODE_NINE_AXIS) { output->mode=INS_MODE_SIX_AXIS; }
  }
  return available;
}
