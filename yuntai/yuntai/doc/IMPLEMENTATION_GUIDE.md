# 关键实现代码与调用说明

按INS采集/融合→遥控输入→算法/底盘→云台→供弹→日志排列，以下为当前源码原样摘录，不是独立编译的另一套算法。初始化、类型和辅助函数沿源文件链接查阅。重复函数名取最后定义，避开未启用HAL的桩函数。

从工程根运行 powershell -NoProfile -File doc/tools/update_technical_reference.ps1 可重建本文件和宏索引。原理推导和实测状态仍需同步 [技术总文档](TECHNICAL_ARCHITECTURE.md)。

## Bmi088_Init

SPI模式切换、复位、ID与可写位回读。

源文件：[Core/Src/bsp/bmi088/bmi088.c](../Core/Src/bsp/bmi088/bmi088.c)，定义始于第 64 行。

```c
bool Bmi088_Init(Bmi088_HandleTypeDef *handle, const Bmi088_ConfigTypeDef *config) {
  if (handle == NULL || config == NULL || config->spi == NULL || config->accel_port == NULL ||
      config->gyro_port == NULL || config->accel_pin == 0U || config->gyro_pin == 0U ||
      config->delay_ms == NULL || config->timeout_ms == 0U || config->accel_range > 3U ||
      config->gyro_range > 4U || config->gyro_bandwidth > 7U ||
      (config->accel_conf >> 4U) < 8U || (config->accel_conf >> 4U) > 10U ||
      (config->accel_conf & 15U) < 5U || (config->accel_conf & 15U) > 12U) { return false; }
  memset(handle, 0, sizeof(*handle));
  handle->config = *config;
  Bmi088_Deselect(handle);
  config->delay_ms(10U);
  uint8_t id = 0U;
  /* 首次读只用于产生 CS 上升沿。复位后再次 dummy 读，不把该值作为 ID。 */
  if (!Bmi088_ReadRegister(handle, true, 0x00U, &id) ||
      !Bmi088_WriteRegister(handle, true, 0x7EU, BMI088_RESET_CMD)) { return false; }
  config->delay_ms(10U);
  if (!Bmi088_ReadRegister(handle, true, 0x00U, &id) ||
      !Bmi088_WriteRegister(handle, false, 0x14U, BMI088_RESET_CMD)) { return false; }
  config->delay_ms(80U); /* 陀螺仪启动最大 80 ms，手册 §1。 */
  if (!Bmi088_ReadRegister(handle, true, 0x00U, &id) || id != 0x1EU ||
      !Bmi088_ReadRegister(handle, false, 0x00U, &id) || id != 0x0FU) { return false; }
  /* 先开启加速度电源，再写滤波/量程；中断输出低有效，与 CubeMX 下降沿一致。 */
  if (!Bmi088_WriteChecked(handle, true, 0x7DU, 0x04U, 0x04U) ||
      !Bmi088_WriteChecked(handle, true, 0x7CU, 0x00U, 0x03U) ||
      !Bmi088_WriteChecked(handle, true, 0x40U, config->accel_conf, 0xFFU) ||
      !Bmi088_WriteChecked(handle, true, 0x41U, config->accel_range, 0x03U) ||
      !Bmi088_WriteChecked(handle, true, 0x53U, 0x08U, 0x0EU) ||
      !Bmi088_WriteChecked(handle, true, 0x58U, 0x04U, 0x44U) ||
      !Bmi088_WriteChecked(handle, false, 0x0FU, config->gyro_range, 0x07U) ||
      !Bmi088_WriteChecked(handle, false, 0x10U, config->gyro_bandwidth, 0x7FU) ||
      !Bmi088_WriteChecked(handle, false, 0x11U, 0x00U, 0xA0U) ||
      !Bmi088_WriteChecked(handle, false, 0x16U, 0x00U, 0x0FU) ||
      !Bmi088_WriteChecked(handle, false, 0x18U, 0x01U, 0x81U) ||
      !Bmi088_WriteChecked(handle, false, 0x15U, 0x80U, 0x80U)) { return false; }
  config->delay_ms(80U);
  handle->initialized = true;
  return true;
}
```

## Bmi088_FinishRead

dummy偏移、补码、量程与温度解析。

源文件：[Core/Src/bsp/bmi088/bmi088.c](../Core/Src/bsp/bmi088/bmi088.c)，定义始于第 130 行。

```c
bool Bmi088_FinishRead(Bmi088_HandleTypeDef *handle) {
  if (handle == NULL || handle->dma_state != BMI088_DMA_DONE) { return false; }
  __DMB();
  if (handle->channel == BMI088_CHANNEL_TEMP) {
    int32_t raw = ((int32_t)handle->rx[2] << 3U) | (handle->rx[3] >> 5U);
    handle->sample.temperature_valid = raw != 1024; /* 0x80/0x00 为手册无效温度。 */
    handle->sample.temperature_raw = (int16_t)(raw > 1023 ? raw - 2048 : raw);
    handle->sample.temperature_c = 23.0f + handle->sample.temperature_raw * 0.125f;
  } else {
    bool accel = handle->channel == BMI088_CHANNEL_ACCEL;
    float scale = accel ? (3.0f * (float)(1U << handle->config.accel_range) * BMI088_GRAVITY_MPS2 / 32768.0f) :
        (2000.0f / (float)(1U << handle->config.gyro_range) / 32768.0f * BMI088_DEG_TO_RAD);
    for (unsigned axis = 0U; axis < 3U; ++axis) {
      int16_t raw = Bmi088_DecodeInt16(&handle->rx[(accel ? 2U : 1U) + 2U * axis]);
      if (accel) { handle->sample.accel_raw[axis] = raw; handle->sample.accel_mps2[axis] = raw * scale; }
      else { handle->sample.gyro_raw[axis] = raw; handle->sample.gyro_rad_s[axis] = raw * scale; }
    }
  }
  handle->dma_state = BMI088_DMA_IDLE;
  return true;
}
```

## Ist8310_PollInit

分步复位与配置回读，不暂停融合100ms。

源文件：[Core/Src/bsp/ist8310/ist8310.c](../Core/Src/bsp/ist8310/ist8310.c)，定义始于第 29 行。

```c
Ist8310_InitStateTypeDef Ist8310_PollInit(Ist8310_HandleTypeDef *h, uint32_t now_ms) {
  if (h==NULL || h->config.i2c==NULL) { return IST8310_INIT_ERROR; }
  static const uint8_t registers[4][2]={{0x0BU,0x08U},{0x41U,0x09U},{0x42U,0xC0U},{0x0AU,0x0BU}};
  uint8_t value=0U;
  if (h->init_state==IST8310_INIT_RESET_LOW && (uint32_t)(now_ms-h->phase_ms)>=50U) {
    HAL_GPIO_WritePin(h->config.reset_port,h->config.reset_pin,GPIO_PIN_SET);
    h->phase_ms=now_ms; h->init_state=IST8310_INIT_RESET_HIGH;
  } else if (h->init_state==IST8310_INIT_RESET_HIGH && (uint32_t)(now_ms-h->phase_ms)>=50U) {
    h->init_state=Ist8310_ReadRegisters(h,0x00U,&value,1U) && value==0x10U ? IST8310_INIT_WRITE : IST8310_INIT_ERROR;
  } else if (h->init_state==IST8310_INIT_WRITE) {
    value=registers[h->register_index][1];
    h->init_state=HAL_I2C_Mem_Write(h->config.i2c,IST8310_HAL_ADDRESS,registers[h->register_index][0],
        I2C_MEMADD_SIZE_8BIT,&value,1U,h->config.timeout_ms)==HAL_OK ? IST8310_INIT_VERIFY : IST8310_INIT_ERROR;
    h->phase_ms=now_ms;
  } else if (h->init_state==IST8310_INIT_VERIFY && (uint32_t)(now_ms-h->phase_ms)>=1U) {
    if (!Ist8310_ReadRegisters(h,registers[h->register_index][0],&value,1U) || value!=registers[h->register_index][1]) {
      h->init_state=IST8310_INIT_ERROR;
    } else if (++h->register_index==4U) { h->init_state=IST8310_INIT_READY; h->initialized=true; }
    else { h->init_state=IST8310_INIT_WRITE; }
  }
  return h->init_state;
}
```

## Ist8310_Read

STAT1新数据/覆盖和μT转换。

源文件：[Core/Src/bsp/ist8310/ist8310.c](../Core/Src/bsp/ist8310/ist8310.c)，定义始于第 62 行。

```c
Ist8310_ReadResultTypeDef Ist8310_Read(Ist8310_HandleTypeDef *handle) {
  if (handle == NULL || !handle->initialized) { return IST8310_READ_ERROR; }
  uint8_t bytes[7];
  if (!Ist8310_ReadRegisters(handle, 0x02U, bytes, sizeof(bytes))) { return IST8310_READ_ERROR; }
  handle->status = bytes[0];
  if ((bytes[0] & 0x01U) == 0U) { return IST8310_READ_NOT_READY; }
  for (unsigned axis = 0U; axis < 3U; ++axis) {
    uint32_t raw = (uint32_t)bytes[1U + axis * 2U] | ((uint32_t)bytes[2U + axis * 2U] << 8U);
    handle->raw[axis] = (int16_t)(raw >= 32768U ? (int32_t)raw - 65536 : (int32_t)raw);
    handle->mag_ut[axis] = handle->raw[axis] * IST8310_UT_PER_LSB;
  }
  return (bytes[0] & 0x02U) != 0U ? IST8310_READ_OVERRUN : IST8310_READ_OK;
}
```

## Ahrs_Init

保持真实倾角，建立零航向与磁参考。

源文件：[Core/Src/algorithm/ahrs/ahrs.c](../Core/Src/algorithm/ahrs/ahrs.c)，定义始于第 99 行。

```c
bool Ahrs_Init(Ahrs_HandleTypeDef *h, const Ahrs_ConfigTypeDef *config,
               const float accel[3], const float mag[3]) {
  if (h == NULL) { return false; }
  memset(h, 0, sizeof(*h));
  if (!Ahrs_ConfigValid(config) || !Ahrs_Finite3(accel)) { return false; }
  float norm = Ahrs_Norm3(accel);
  if (!isfinite(norm) || norm < AHRS_NORM_EPSILON ||
      fabsf(norm / config->gravity_mps2 - 1.0f) >= config->accel_reject_fraction) { return false; }
  h->config = *config;
  float roll = atan2f(-accel[1], -accel[2]);
  float pitch = atan2f(accel[0], hypotf(accel[1], accel[2]));
  float cr = cosf(roll * 0.5f), sr = sinf(roll * 0.5f);
  float cp = cosf(pitch * 0.5f), sp = sinf(pitch * 0.5f);
  h->q[0] = cr*cp; h->q[1] = sr*cp; h->q[2] = cr*sp; h->q[3] = -sr*sp;
  h->initialized = true;
  (void)Ahrs_SetMagReference(h, mag);
  return true;
}
```

## Ahrs_Update

实际dt、重力反馈、磁门控和有界航向恢复。

源文件：[Core/Src/algorithm/ahrs/ahrs.c](../Core/Src/algorithm/ahrs/ahrs.c)，定义始于第 118 行。

```c
bool Ahrs_Update(Ahrs_HandleTypeDef *h, const float gyro[3], const float accel[3],
                 const float mag[3], float dt) {
  if (h == NULL || !h->initialized || !Ahrs_Finite3(gyro) || !isfinite(dt) ||
      dt <= 0.0f || dt > h->config.max_dt_s) { return false; }
  Ahrs_HandleTypeDef next = *h; /* 候选状态全部验证后提交，失败不污染四元数。 */
  float omega[3] = {gyro[0],gyro[1],gyro[2]};
  float down[3], reference_down[3] = {0.0f,0.0f,1.0f};
  Ahrs_Rotate(h->q, reference_down, down, true);
  next.accel_weight = 0.0f;
  float error[3] = {0};
  if (Ahrs_Finite3(accel)) {
    float norm = Ahrs_Norm3(accel);
    if (isfinite(norm) && norm > AHRS_NORM_EPSILON) {
      float deviation = fabsf(norm / h->config.gravity_mps2 - 1.0f);
      next.accel_weight = Ahrs_Clamp((h->config.accel_reject_fraction-deviation) /
          (h->config.accel_reject_fraction-h->config.accel_full_fraction),0.0f,1.0f);
      float measured[3] = {-accel[0]/norm,-accel[1]/norm,-accel[2]/norm};
      error[0] = measured[1]*down[2]-measured[2]*down[1];
      error[1] = measured[2]*down[0]-measured[0]*down[2];
      error[2] = measured[0]*down[1]-measured[1]*down[0];
    }
  }
  for (unsigned i = 0U; i < 3U; ++i) {
    if (h->config.ki == 0.0f) { next.integral_rad_s[i] = 0.0f; }
    else if (next.accel_weight > 0.0f) {
      next.integral_rad_s[i] = Ahrs_Clamp(next.integral_rad_s[i] +
          h->config.ki*error[i]*next.accel_weight*dt, -h->config.integral_limit_rad_s, h->config.integral_limit_rad_s);
    }
    omega[i] += h->config.kp*error[i]*next.accel_weight + next.integral_rad_s[i];
  }
  /* 比较水平磁方向，只沿重力轴反馈。模长和创新门同时通过才恢复权重。 */
  next.mag_used = false;
  if (!next.mag_reference_valid) { (void)Ahrs_SetMagReference(&next, mag); }
  if (next.mag_reference_valid && Ahrs_Finite3(mag)) {
    float norm = Ahrs_Norm3(mag), world[3];
    Ahrs_Rotate(h->q, mag, world, false);
    if (isfinite(norm) && norm >= h->config.mag_min_ut && norm <= h->config.mag_max_ut &&
        fabsf(norm / next.mag_reference_ut - 1.0f) <= h->config.mag_relative_tolerance &&
        hypotf(world[0],world[1]) > 0.1f*norm) {
      float innovation = atan2f(world[0]*next.mag_reference[1]-world[1]*next.mag_reference[0],
                                world[0]*next.mag_reference[0]+world[1]*next.mag_reference[1]);
      if (fabsf(innovation) <= h->config.mag_innovation_rad) {
        next.mag_weight = fminf(1.0f, next.mag_weight + dt/h->config.mag_recovery_s);
        float correction = Ahrs_Clamp(h->config.kp*innovation, -h->config.mag_rate_limit_rad_s,
                                       h->config.mag_rate_limit_rad_s) * next.mag_weight;
        for (unsigned i = 0U; i < 3U; ++i) { omega[i] += correction*down[i]; }
        next.mag_used = true;
      }
    }
  }
  if (!next.mag_used) { next.mag_weight = 0.0f; }
  float w=h->q[0], x=h->q[1], y=h->q[2], z=h->q[3];
  float gx=omega[0]*dt*0.5f, gy=omega[1]*dt*0.5f, gz=omega[2]*dt*0.5f;
  next.q[0]=w-x*gx-y*gy-z*gz; next.q[1]=x+w*gx+y*gz-z*gy;
  next.q[2]=y+w*gy-x*gz+z*gx; next.q[3]=z+w*gz+x*gy-y*gx;
  float norm = hypotf(hypotf(next.q[0],next.q[1]),hypotf(next.q[2],next.q[3]));
  if (!isfinite(norm) || norm < AHRS_NORM_EPSILON) { return false; }
  for (unsigned i = 0U; i < 4U; ++i) { next.q[i] /= norm; }
  *h = next;
  return true;
}
```

## Ins_Calibrate

Welford连续静止窗口与零偏。

源文件：[Core/Src/app/ins/ins.c](../Core/Src/app/ins/ins.c)，定义始于第 107 行。

```c
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
```

## Ins_Process

安装/标定转换、融合、角度回绕和快照发布。

源文件：[Core/Src/app/ins/ins.c](../Core/Src/app/ins/ins.c)，定义始于第 142 行。

```c
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
```

## Ins_GetSnapshot

短临界区复制并重新检查HAL年龄。

源文件：[Core/Src/app/ins/ins.c](../Core/Src/app/ins/ins.c)，定义始于第 194 行。

```c
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
```

## InsTask_ReadDma

任务通知有界等待；DMA失败阶段和Abort前SPI错误留证，传感器通知不能冒充完成。

源文件：[Core/Src/task/task_ins/task_ins_runtime.c](../Core/Src/task/task_ins/task_ins_runtime.c)，定义始于第 174 行。

```c
static bool InsTask_ReadDma(Bmi088_ChannelTypeDef channel) {
  uint32_t events;
  g_runtime.dma_failure=INS_TASK_DMA_FAILURE_NONE;
  (void)xTaskNotifyWait(0U,UINT32_MAX,&events,0U);
  if (!Bmi088_StartRead(&g_runtime.bmi,channel)) {
    g_runtime.dma_failure=INS_TASK_DMA_FAILURE_START;
    g_runtime.dma_spi_error=hspi1.ErrorCode;
    return false;
  }
  TickType_t start=xTaskGetTickCount(), timeout=pdMS_TO_TICKS(INS_DMA_TIMEOUT_MS);
  while (g_runtime.bmi.dma_state==BMI088_DMA_BUSY) {
    TickType_t elapsed=xTaskGetTickCount()-start;
    if (elapsed>=timeout) {
      g_runtime.dma_failure=INS_TASK_DMA_FAILURE_TIMEOUT;
      g_runtime.dma_spi_error=hspi1.ErrorCode;
      Bmi088_Abort(&g_runtime.bmi); return false;
    }
    (void)xTaskNotifyWait(0U,UINT32_MAX,&events,timeout-elapsed);
  }
  if (!Bmi088_FinishRead(&g_runtime.bmi)) {
    g_runtime.dma_failure=INS_TASK_DMA_FAILURE_FINISH;
    g_runtime.dma_spi_error=hspi1.ErrorCode;
    return false;
  }
  return true;
}
```

## InsTask_ImuFault

先记录历史失败出口/时刻/次数，再Abort失效；恢复不清历史原因。

源文件：[Core/Src/task/task_ins/task_ins_runtime.c](../Core/Src/task/task_ins/task_ins_runtime.c)，定义始于第 210 行。

```c
static void InsTask_ImuFault(InsTask_FaultTypeDef reason) {
  /* 先保留失败证据再Abort；HAL清理可能清错误位，重试不能抹去上次原因。 */
  g_runtime.last_fault=reason;
  ++g_runtime.fault_count;
  g_runtime.fault_ms=HAL_GetTick();
  g_runtime.fault_dma_failure=g_runtime.dma_failure;
  g_runtime.fault_spi_error=g_runtime.dma_failure!=INS_TASK_DMA_FAILURE_NONE
      ? g_runtime.dma_spi_error : hspi1.ErrorCode;
  g_accept_imu=false;
  Bmi088_Abort(&g_runtime.bmi);
  g_runtime.input.accel_valid=false;
  g_runtime.input.temperature_valid=false;
  g_runtime.have_cycles=false;
  g_runtime.bmi_retry_tick=xTaskGetTickCount();
  Ins_SetFault();
}
```

## InsTask_Log

MotionCal Raw帧与状态日志互斥；普通姿态追加采集故障记录，磁长格式不叠加，DMA非阻塞提交。

源文件：[Core/Src/task/task_ins/task_ins_runtime.c](../Core/Src/task/task_ins/task_ins_runtime.c)，定义始于第 289 行。

```c
static void InsTask_Log(uint32_t now) {
#if LOG_GLOBAL_ENABLE && LOG_USART1_ENABLE && LOG_INS_MAG_CALIBRATION_ENABLE
  /* 直接输出未标定/未旋转的磁芯片原始值。前六项占位为0，不代表真实惯性数据。
   * PJRC ascii_parse不接受冒号/逗号后的空格；必须紧凑输出，并以CR结束帧，保留CRLF。
   * MotionCal接收int16_t，乘3必须先扩展；越界整帧拒绝，不能钳位或回绕成假磁场。
   * 单次DMA提交整行，忙时丢帧；只有接受后记录样本时间，不重复提交旧样本。 */
  if (g_runtime.input.mag_valid && (uint32_t)(now-g_runtime.input.mag_ms)<=INS_MAG_TIMEOUT_MS &&
      (uint32_t)(now-g_runtime.last_mag_calibration_ms)>=INS_MAG_CALIBRATION_PERIOD_MS &&
      (!g_runtime.have_mag_calibration_sample || g_runtime.mag_calibration_sample_ms!=g_runtime.input.mag_ms)) {
    g_runtime.last_mag_calibration_ms=now;
    long counts[3];
    for (unsigned i=0U;i<3U;++i) {
      counts[i]=(long)g_runtime.input.mag_raw[i]*INS_MOTIONCAL_COUNTS_PER_LSB;
      if (counts[i]<INT16_MIN || counts[i]>INT16_MAX) { return; }
    }
    if (LOG_TRY_PRINTF(LOG_CATEGORY_INS_MAG_CALIBRATION,"Raw:0,0,0,0,0,0,%ld,%ld,%ld\r\n",
        counts[0],counts[1],counts[2])) {
      g_runtime.mag_calibration_sample_ms=g_runtime.input.mag_ms;
      g_runtime.have_mag_calibration_sample=true;
    }
  }
#elif LOG_GLOBAL_ENABLE && LOG_USART1_ENABLE && LOG_INS_ENABLE
  if ((uint32_t)(now-g_runtime.last_log_ms)>=INS_LOG_PERIOD_MS) {
    g_runtime.last_log_ms=now;
    Ins_SnapshotTypeDef s;
    if (Ins_GetSnapshot(&s)) {
      /* 采集诊断与姿态状态共用一次 DMA 提交，避免第二次提交必因首条仍在发送而丢弃。
       * 原码为未映射/未去偏置的芯片LSB，比力为转换后的机体系mm/s²；同快照对照可发现重复取反。 */
      (void)LOG_TRY_PRINTF(LOG_CATEGORY_INS,"姿态,状态=%u,有效=%u,安装=%u,磁融合=%u,航向/俯仰/横滚毫度=%ld/%ld/%ld,重置=%lu,丢帧=%lu"
#if !LOG_INS_MAG_DIAGNOSTIC_ENABLE
        ",时间ms=%lu,序号=%lu,上次采集故障=%u,故障次数=%lu,故障时刻ms=%lu,初始化次数=%lu,DMA失败阶段=%u,SPI错误=%lu"
#endif
        "\r\n"
#if LOG_INS_MAG_DIAGNOSTIC_ENABLE
        "[DEBUG-INS-MAG-1018],时间ms=%lu,序号=%lu,磁有效=%u,磁龄ms=%lu,磁芯片nT=%ld/%ld/%ld,磁机体nT=%ld/%ld/%ld,角速度urad_s=%ld/%ld/%ld,四元数百万分=%ld/%ld/%ld/%ld,温度毫摄氏度=%ld,加计原码=%d/%d/%d,陀螺原码=%d/%d/%d,比力mm_s2=%ld/%ld/%ld\r\n"
#endif
        ,
        (unsigned)s.state,(unsigned)s.attitude_valid,(unsigned)s.mount_confirmed,(unsigned)s.mag_used,
        InsTask_ScaledLog(s.yaw_deg,1000.0f),InsTask_ScaledLog(s.pitch_deg,1000.0f),InsTask_ScaledLog(s.roll_deg,1000.0f),
        (unsigned long)s.reset_sequence,(unsigned long)s.dropped_gyro
#if !LOG_INS_MAG_DIAGNOSTIC_ENABLE
        ,(unsigned long)now,(unsigned long)s.sequence,(unsigned)g_runtime.last_fault,
        (unsigned long)g_runtime.fault_count,(unsigned long)g_runtime.fault_ms,
        (unsigned long)g_runtime.init_attempts,(unsigned)g_runtime.fault_dma_failure,
        (unsigned long)g_runtime.fault_spi_error
#endif
#if LOG_INS_MAG_DIAGNOSTIC_ENABLE
        ,(unsigned long)now,(unsigned long)s.sequence,(unsigned)s.mag_valid,(unsigned long)(now-s.mag_ms),
        InsTask_ScaledLog(s.mag_chip_ut[0],1000.0f),InsTask_ScaledLog(s.mag_chip_ut[1],1000.0f),InsTask_ScaledLog(s.mag_chip_ut[2],1000.0f),
        InsTask_ScaledLog(s.mag_ut[0],1000.0f),InsTask_ScaledLog(s.mag_ut[1],1000.0f),InsTask_ScaledLog(s.mag_ut[2],1000.0f),
        InsTask_ScaledLog(s.gyro_rad_s[0],1000000.0f),InsTask_ScaledLog(s.gyro_rad_s[1],1000000.0f),InsTask_ScaledLog(s.gyro_rad_s[2],1000000.0f),
        InsTask_ScaledLog(s.q[0],1000000.0f),InsTask_ScaledLog(s.q[1],1000000.0f),InsTask_ScaledLog(s.q[2],1000000.0f),InsTask_ScaledLog(s.q[3],1000000.0f),
        InsTask_ScaledLog(s.temperature_c,1000.0f),
        (int)s.accel_raw[0],(int)s.accel_raw[1],(int)s.accel_raw[2],
        (int)s.gyro_raw[0],(int)s.gyro_raw[1],(int)s.gyro_raw[2],
        InsTask_ScaledLog(s.accel_mps2[0],1000.0f),InsTask_ScaledLog(s.accel_mps2[1],1000.0f),InsTask_ScaledLog(s.accel_mps2[2],1000.0f)
#endif
        );
    }
  }
#else
  (void)now;
#endif
}
```

## InsTask_HardwareReady

BMI硬件先决条件；磁采集启用时才检查I2C3/IST EXTI。

源文件：[Core/Src/task/task_ins/task_ins_runtime.c](../Core/Src/task/task_ins/task_ins_runtime.c)，定义始于第 125 行。

```c
static bool InsTask_HardwareReady(void) {
  const IRQn_Type irqs[]={EXTI3_IRQn,EXTI4_IRQn,DMA2_Stream0_IRQn,DMA2_Stream3_IRQn,SPI1_IRQn};
  if (hspi1.Init.BaudRatePrescaler!=SPI_BAUDRATEPRESCALER_16 || hspi1.Init.CLKPolarity!=SPI_POLARITY_HIGH ||
      hspi1.Init.CLKPhase!=SPI_PHASE_2EDGE ||
      (EXTI->FTSR & (GPIO_PIN_3|GPIO_PIN_4))!=(GPIO_PIN_3|GPIO_PIN_4) ||
      (EXTI->RTSR & (GPIO_PIN_3|GPIO_PIN_4))!=0U || SystemCoreClock==0U) { return false; }
  for (unsigned i=0U;i<sizeof(irqs)/sizeof(irqs[0]);++i) {
    if (NVIC_GetEnableIRQ(irqs[i])==0U || NVIC_GetPriority(irqs[i])!=5U) { return false; }
  }
  /* 六轴不依赖磁接口；只在明确开启磁采集时检查 I2C 和 IST 的 EXTI。 */
  if (INS_MAG_ENABLE!=0U && (hi2c3.Init.ClockSpeed!=400000U ||
      (EXTI->FTSR & GPIO_PIN_5)==0U || (EXTI->RTSR & GPIO_PIN_5)!=0U ||
      NVIC_GetEnableIRQ(EXTI9_5_IRQn)==0U || NVIC_GetPriority(EXTI9_5_IRQn)!=5U)) { return false; }
  return true;
}
```

## InsTask_ServiceMag

磁总门关闭直接返回；开启时执行分步恢复和新鲜数据采集。

源文件：[Core/Src/task/task_ins/task_ins_runtime.c](../Core/Src/task/task_ins/task_ins_runtime.c)，定义始于第 249 行。

```c
static void InsTask_ServiceMag(uint32_t now) {
  if (INS_MAG_ENABLE==0U) { g_runtime.input.mag_valid=false; return; }
  Ist8310_HandleTypeDef *mag=&g_runtime.mag;
  if (!mag->initialized) {
    if (mag->config.i2c==NULL || mag->init_state==IST8310_INIT_ERROR) {
      TickType_t tick=xTaskGetTickCount();
      if (tick-g_runtime.mag_retry_tick<pdMS_TO_TICKS(INS_RETRY_MS)) { return; }
      Ist8310_ConfigTypeDef config={.i2c=&hi2c3,.reset_port=RSTN_IST8310_GPIO_Port,
        .reset_pin=RSTN_IST8310_Pin,.delay_ms=InsTask_DelayMs,.timeout_ms=INS_I2C_TIMEOUT_MS};
      g_runtime.mag_retry_tick=tick;
      (void)Ist8310_BeginInit(mag,&config,now);
    } else { (void)Ist8310_PollInit(mag,now); }
    return;
  }
  InsTask_StampTypeDef stamp=InsTask_GetStamp(&g_mag_stamp);
  if (stamp.sequence==g_runtime.mag_consumed && (uint32_t)(now-g_runtime.last_mag_poll_ms)<INS_MAG_POLL_MS) { return; }
  g_runtime.mag_consumed=stamp.sequence;
  g_runtime.last_mag_poll_ms=now;
  Ist8310_ReadResultTypeDef result=Ist8310_Read(mag);
  if (result==IST8310_READ_ERROR) {
    mag->initialized=false; mag->init_state=IST8310_INIT_ERROR;
    g_runtime.input.mag_valid=false; g_runtime.mag_retry_tick=xTaskGetTickCount();
  } else if (result==IST8310_READ_OK || result==IST8310_READ_OVERRUN) {
    memcpy(g_runtime.input.mag_ut,mag->mag_ut,sizeof(mag->mag_ut));
    memcpy(g_runtime.input.mag_raw,mag->raw,sizeof(mag->raw));
    g_runtime.input.mag_ms=HAL_GetTick(); /* 仅实际新数据成功读取才刷新；兜底轮询也可使用。 */
    g_runtime.input.mag_valid=true;
    if (result==IST8310_READ_OVERRUN) { ++g_runtime.input.mag_overruns; }
  }
}
```

## InsTask_RuntimeRunCycle

最新DRDY采集、超时恢复；磁服务受INS_MAG_ENABLE控制。

源文件：[Core/Src/task/task_ins/task_ins_runtime.c](../Core/Src/task/task_ins/task_ins_runtime.c)，定义始于第 368 行。

```c
void InsTask_RuntimeRunCycle(void) {
  uint32_t events;
  TickType_t tick=xTaskGetTickCount();
  if (!g_runtime.bmi.initialized && tick-g_runtime.bmi_retry_tick>=pdMS_TO_TICKS(INS_RETRY_MS)) {
    g_runtime.bmi_retry_tick=tick; InsTask_TryImuInit();
  }
  (void)xTaskNotifyWait(0U,UINT32_MAX,&events,pdMS_TO_TICKS(INS_DMA_TIMEOUT_MS));
  uint32_t now=HAL_GetTick();
  if (!g_runtime.bmi.initialized) { InsTask_ServiceMag(now); InsTask_Log(now); return; }
  InsTask_StampTypeDef gyro=InsTask_GetStamp(&g_gyro_stamp);
  if (gyro.sequence!=g_runtime.gyro_consumed) {
    /* DWT 约25.6s回绕；再用同一HAL时基检查跨帧间隔，不能让长暂停被模运算伪装成短dt。 */
    if (g_runtime.have_cycles && (uint32_t)(gyro.ms-g_runtime.last_gyro_ms)>INS_GYRO_TIMEOUT_MS) {
      InsTask_ImuFault(INS_TASK_FAULT_GYRO_GAP); return;
    }
    uint32_t count=gyro.sequence-g_runtime.gyro_consumed;
    g_runtime.input.dropped_gyro+=count-1U;
    g_runtime.gyro_consumed=gyro.sequence;
    if (!InsTask_ReadDma(BMI088_CHANNEL_GYRO)) { InsTask_ImuFault(INS_TASK_FAULT_GYRO_DMA); return; }
    InsTask_StampTypeDef accel=InsTask_GetStamp(&g_accel_stamp);
    if (accel.sequence!=g_runtime.accel_consumed) {
      g_runtime.accel_consumed=accel.sequence;
      if (!InsTask_ReadDma(BMI088_CHANNEL_ACCEL)) { InsTask_ImuFault(INS_TASK_FAULT_ACCEL_DMA); return; }
      memcpy(g_runtime.input.accel_mps2,g_runtime.bmi.sample.accel_mps2,sizeof(g_runtime.input.accel_mps2));
      memcpy(g_runtime.input.accel_raw,g_runtime.bmi.sample.accel_raw,sizeof(g_runtime.input.accel_raw));
      g_runtime.input.accel_ms=accel.ms; g_runtime.input.accel_valid=true;
    }
    now=HAL_GetTick();
    if ((uint32_t)(now-g_runtime.last_temp_attempt_ms)>=INS_TEMPERATURE_PERIOD_MS) {
      g_runtime.last_temp_attempt_ms=now;
      if (!InsTask_ReadDma(BMI088_CHANNEL_TEMP)) { InsTask_ImuFault(INS_TASK_FAULT_TEMP_DMA); return; }
      g_runtime.input.temperature_c=g_runtime.bmi.sample.temperature_c;
      g_runtime.input.temperature_valid=g_runtime.bmi.sample.temperature_valid;
      g_runtime.input.temperature_ms=HAL_GetTick();
    }
    memcpy(g_runtime.input.gyro_rad_s,g_runtime.bmi.sample.gyro_rad_s,sizeof(g_runtime.input.gyro_rad_s));
    memcpy(g_runtime.input.gyro_raw,g_runtime.bmi.sample.gyro_raw,sizeof(g_runtime.input.gyro_raw));
    g_runtime.input.dt_s=g_runtime.have_cycles ? (float)(uint32_t)(gyro.cycles-g_runtime.previous_cycles)/(float)SystemCoreClock : 0.001f;
    g_runtime.previous_cycles=gyro.cycles; g_runtime.have_cycles=true;
    g_runtime.input.gyro_ms=gyro.ms; g_runtime.input.now_ms=HAL_GetTick();
    g_runtime.last_gyro_ms=gyro.ms;
    if (g_runtime.input.dt_s<=0.0f || g_runtime.input.dt_s>INS_MAX_DT_S) {
      InsTask_ImuFault(INS_TASK_FAULT_DT); return;
    }
    if ((uint32_t)(g_runtime.input.now_ms-gyro.ms)>INS_GYRO_TIMEOUT_MS) {
      InsTask_ImuFault(INS_TASK_FAULT_GYRO_AGE); return;
    }
    if (!g_runtime.input.accel_valid && (uint32_t)(g_runtime.input.now_ms-g_runtime.imu_start_ms)>INS_ACCEL_TIMEOUT_MS) {
      InsTask_ImuFault(INS_TASK_FAULT_ACCEL_MISSING); return;
    }
    if (g_runtime.input.accel_valid && (uint32_t)(g_runtime.input.now_ms-g_runtime.input.accel_ms)>INS_ACCEL_TIMEOUT_MS) {
      InsTask_ImuFault(INS_TASK_FAULT_ACCEL_AGE); return;
    }
    Ins_Process(&g_runtime.input);
  } else if ((uint32_t)(now-g_runtime.last_gyro_ms)>INS_GYRO_TIMEOUT_MS) {
    InsTask_ImuFault(INS_TASK_FAULT_GYRO_MISSING); return;
  }
  InsTask_ServiceMag(HAL_GetTick());
  InsTask_Log(HAL_GetTick());
}
```

## Dbus_DecodeFrame

DBUS位解包、大端/小端区别和范围校验。

源文件：[Core/Src/bsp/dbus/dbus.c](../Core/Src/bsp/dbus/dbus.c)，定义始于第 57 行。

```c
bool Dbus_DecodeFrame(const uint8_t *b, Dbus_DataTypeDef *data) {
  if (b == NULL || data == NULL) {
    return false;
  }
  Dbus_DataTypeDef decoded = {0};
  /* 四路 11 位通道跨字节排列。必须保留每字节 bit7。USART 7 位接收会在这里悄悄破坏摇杆值。 */
  decoded.channels[0] = (int16_t)(((b[0] | (b[1] << 8)) & 0x07FF) - DBUS_CHANNEL_CENTER);
  decoded.channels[1] = (int16_t)((((b[1] >> 3) | (b[2] << 5)) & 0x07FF) - DBUS_CHANNEL_CENTER);
  decoded.channels[2] = (int16_t)((((b[2] >> 6) | (b[3] << 2) | (b[4] << 10)) & 0x07FF) - DBUS_CHANNEL_CENTER);
  decoded.channels[3] = (int16_t)((((b[4] >> 1) | (b[5] << 7)) & 0x07FF) - DBUS_CHANNEL_CENTER);
  /* CH4 也是 11 位字段。不屏蔽高 5 位会把保留位带进滚轮值，进而溢出或误判帧。 */
  decoded.channels[4] = (int16_t)((Dbus_ReadU16(&b[16]) & 0x07FFU) -
                                  DBUS_CHANNEL_CENTER);
  /*
   * S1 左拨杆使用 DATA[5] bit6~7，S2 右拨杆使用 bit4~5。
   * 不能按变量名的自然顺序猜位序，否则日志中的左右拨杆会对调。
   */
  const uint8_t switch_bits = (uint8_t)((b[5] >> 4) & 0x0FU);
  decoded.switch_left = (switch_bits >> 2) & 0x03U; /* S1，DATA[5] bit6~7。 */
  decoded.switch_right = switch_bits & 0x03U; /* S2，DATA[5] bit4~5。 */
  decoded.mouse_x = (int16_t)Dbus_ReadU16(&b[6]);
  decoded.mouse_y = (int16_t)Dbus_ReadU16(&b[8]);
  decoded.mouse_z = (int16_t)Dbus_ReadU16(&b[10]);
  decoded.mouse_left = b[12];
  decoded.mouse_right = b[13];
  decoded.keyboard = Dbus_ReadU16(&b[14]);
  /* 协议没有帧头和 CRC，范围校验只能筛掉明显错位。坏帧不能覆盖最后合法输入或刷新在线时间。 */
  /* 仅四路摇杆使用 364~1684 的官方范围。滚轮 CH4 已按 11 位掩码，允许其独立物理范围。 */
  for (uint8_t i = 0U; i < 4U; ++i) {
    if (decoded.channels[i] < -DBUS_CHANNEL_SPAN || decoded.channels[i] > DBUS_CHANNEL_SPAN) {
      return false;
    }
  }
  /*
   * 拨杆保留值 0 不控制输出许可，接收器切换瞬间仍可保留摇杆输入。
   * 鼠标按键只接受协议 0/1；其它值拒绝整帧，防止错位数据触发发射。
   */
  if (decoded.mouse_left > 1U || decoded.mouse_right > 1U) {
    return false;
  }
  *data = decoded;
  return true;
}
```

## Dbus_CopyCompleted

DMA块复制到邮箱；环形数组保留一个空槽。

源文件：[Core/Src/bsp/dbus/dbus.c](../Core/Src/bsp/dbus/dbus.c)，定义始于第 124 行。

```c
static void Dbus_CopyCompleted(uint8_t index) {
  if (!g_active || !g_synchronized || g_restart_requested ||
      index >= 2U) {
    return;
  }
  /*
   * DMA 已切换到另一块后才复制当前完成块。不调用 FreeRTOS API，避免
   * DMA/USART 中断路径受到队列句柄和优先级配置影响。
   */
  const uint8_t next_head = (uint8_t)((g_frame_mailbox_head + 1U) %
                                      DBUS_FRAME_MAILBOX_LENGTH);
  if (next_head == g_frame_mailbox_tail) {
    ++g_frame_mailbox_overruns;
    return; /* 邮箱满时保留已排队帧；丢帧次数进入快照，禁止假造鼠标位移。 */
  }
  Dbus_FrameTypeDef *slot = &g_frame_mailbox[g_frame_mailbox_head];
  memcpy(slot->bytes, g_rx_buffers[index], DBUS_FRAME_LENGTH);
  slot->timestamp_ms = HAL_GetTick();
  slot->timestamp_cycles = DWT->CYCCNT;
  slot->epoch = g_epoch;
  __DMB();
  g_frame_mailbox_head = next_head;
}
```

## Dbus_Process

无观察者的接收适配；复用同一排空、代数检查和累计流程。

源文件：[Core/Src/bsp/dbus/dbus.c](../Core/Src/bsp/dbus/dbus.c)，定义始于第 275 行。

```c
void Dbus_Process(uint32_t now_ms) {
  Dbus_ProcessObserved(now_ms, NULL);
}
```

## Dbus_ProcessObserved

逐帧解码、epoch核对与累计；合法提交后在临界区外通知只读观察者。

源文件：[Core/Src/bsp/dbus/dbus.c](../Core/Src/bsp/dbus/dbus.c)，定义始于第 280 行。

```c
void Dbus_ProcessObserved(uint32_t now_ms, Dbus_FrameObserverTypeDef observer) {
  if (huart3.Instance == NULL || huart3.hdmarx == NULL ||
      huart3.hdmarx->Instance == NULL) {
    return;
  }
  if (!g_active && g_boundary_seen &&
      (!g_retry_wait || now_ms - g_last_retry_ms >= DBUS_RETRY_PERIOD_MS)) {
    /* 先关请求，任务有界 Abort 把 HAL 句柄解锁，再短临界区重启。不能在关中断期间等待 stream 停止。 */
    const HAL_StatusTypeDef stopped = huart3.hdmarx->State == HAL_DMA_STATE_READY
        ? HAL_OK : HAL_DMA_Abort(huart3.hdmarx);
    taskENTER_CRITICAL();
    const bool started = stopped == HAL_OK && Dbus_Start(g_boundary_seen);
    if (stopped != HAL_OK) {
      ++g_snapshot.start_errors;
    }
    taskEXIT_CRITICAL();
    g_retry_wait = !started;
    g_last_retry_ms = now_ms;
  }
  /* 一次排空当前邮箱。鼠标是相对位移，必须累计所有合法帧，不能只保留最后一帧。
   * 快照时间使用最后一帧时间戳，避免积压帧加入后立刻按旧年龄进入回中。 */
  for (;;) {
    Dbus_FrameTypeDef frame;
    bool frame_available = false;
    taskENTER_CRITICAL();
    if (g_frame_mailbox_tail != g_frame_mailbox_head) {
      frame = g_frame_mailbox[g_frame_mailbox_tail];
      g_frame_mailbox_tail = (uint8_t)((g_frame_mailbox_tail + 1U) %
                                        DBUS_FRAME_MAILBOX_LENGTH);
      frame_available = true;
    }
    taskEXIT_CRITICAL();
    if (!frame_available) {
      break;
    }
    Dbus_DataTypeDef decoded;
    const bool valid = Dbus_DecodeFrame(frame.bytes, &decoded);
    bool committed = false;
    uint32_t sequence = 0U;
    taskENTER_CRITICAL();
    /* 解码期间 ISR 也可能故障。提交前再核对代数，旧帧不能重新宣告在线。 */
    if (frame.epoch == g_epoch && g_active && g_synchronized && !g_restart_requested) {
      if (valid) {
        g_snapshot.data = decoded;
        g_snapshot.timestamp_ms = frame.timestamp_ms;
        g_snapshot.valid = true;
        ++g_snapshot.valid_frames;
        sequence = g_snapshot.valid_frames;
        committed = true;
        g_snapshot.mouse_delta_x += decoded.mouse_x;
        g_snapshot.mouse_delta_y += decoded.mouse_y;
        ++g_snapshot.mouse_delta_frames;
        g_snapshot_epoch = frame.epoch;
      } else {
        ++g_snapshot.invalid_frames;
      }
    }
    taskEXIT_CRITICAL();
    if (committed && observer != NULL) {
      const Dbus_ReceivedFrameTypeDef received = {
          .data = decoded, .timestamp_ms = frame.timestamp_ms,
          .timestamp_cycles = frame.timestamp_cycles, .sequence = sequence,
      };
      observer(&received);
    }
  }
}
```

## DbusTrace_RecordFrame

接收帧副本、HAL/DWT时间、微秒间隔与独立诊断丢样计数。

源文件：[Core/Src/task/task_dbus/task_dbus_trace.c](../Core/Src/task/task_dbus/task_dbus_trace.c)，定义始于第 122 行。

```c
void DbusTrace_RecordFrame(const Dbus_ReceivedFrameTypeDef *frame) {
  if (frame == NULL) { return; }
  uint32_t interval_us = DBUS_TRACE_INTERVAL_UNKNOWN_US;
  if (g_have_frame && frame->timestamp_ms - g_previous_frame_ms < DBUS_TRACE_STATUS_MS &&
      SystemCoreClock >= 1000000U) {
    interval_us = (uint32_t)((uint64_t)(uint32_t)(frame->timestamp_cycles -
        g_previous_frame_cycles) * 1000000U / SystemCoreClock);
  }
  g_have_frame = true;
  g_previous_frame_ms = frame->timestamp_ms;
  g_previous_frame_cycles = frame->timestamp_cycles;
  DbusTrace_RecordTypeDef *record = DbusTrace_Reserve();
  if (record != NULL) {
    record->is_frame = true;
    record->data.frame.received = *frame;
    record->data.frame.interval_us = interval_us;
  }
}
```

## DbusTrace_RecordCommand

同周期命令与累计鼠标输入，0.001千分比量化只用于诊断。

源文件：[Core/Src/task/task_dbus/task_dbus_trace.c](../Core/Src/task/task_dbus/task_dbus_trace.c)，定义始于第 142 行。

```c
void DbusTrace_RecordCommand(const Dbus_SnapshotTypeDef *input, uint32_t control_ms,
    float virtual_x, float virtual_y, float yaw, float pitch, bool enabled,
    uint8_t x_state, uint8_t y_state) {
  if (input == NULL) { return; }
  const DbusTrace_CommandTypeDef value = {
      .virtual_x = DbusTrace_Quantize(virtual_x), .virtual_y = DbusTrace_Quantize(virtual_y),
      .yaw = DbusTrace_Quantize(yaw), .pitch = DbusTrace_Quantize(pitch),
      .flags = (input->online ? 1U : 0U) | (enabled ? 2U : 0U) |
          (DBUS_MOUSE_VIRTUAL_ENABLE ? 4U : 0U) |
          ((uint32_t)(x_state & 3U) << 4U) | ((uint32_t)(y_state & 3U) << 6U),
  };
  const uint32_t sample_ms = HAL_GetTick();
  const uint32_t cycles = DWT->CYCCNT;
  if (g_have_command && input->valid_frames == g_previous_sequence &&
      memcmp(&value, &g_previous_command, sizeof(value)) == 0 &&
      sample_ms - g_previous_command_ms < DBUS_TRACE_HEARTBEAT_MS) { return; }
  g_have_command = true;
  g_previous_command = value;
  g_previous_sequence = input->valid_frames;
  g_previous_command_ms = sample_ms;
  DbusTrace_RecordTypeDef *record = DbusTrace_Reserve();
  if (record != NULL) {
    record->data.command.value = value;
    record->data.command.sample_ms = sample_ms;
    record->data.command.cycles = cycles;
    record->data.command.control_ms = control_ms;
    record->data.command.receive_ms = input->timestamp_ms;
    record->data.command.sequence = input->valid_frames;
    record->data.command.delta_x = input->mouse_delta_x;
    record->data.command.delta_y = input->mouse_delta_y;
    record->data.command.frames = input->mouse_delta_frames;
  }
}
```

## DbusTrace_Flush

完整CSV批次经唯一DMA后端提交，忙时保留，接受后出队。

源文件：[Core/Src/task/task_dbus/task_dbus_trace.c](../Core/Src/task/task_dbus/task_dbus_trace.c)，定义始于第 244 行。

```c
void DbusTrace_Flush(const Dbus_SnapshotTypeDef *input, uint32_t now_ms) {
  if (input == NULL) { return; }
  if (g_header < 4U) { DbusTrace_Header(); return; }
  if (now_ms - g_last_status_ms >= DBUS_TRACE_STATUS_MS) {
    if (LOG_TRY_PRINTF(LOG_CATEGORY_DBUS_TRACE,
        "S,%lu,%lu,%lu,%lu,%lu,%lu,%lu,%lu,%lu,%u,%u,%lu,%lu\r\n",
        (unsigned long)now_ms, (unsigned long)input->valid_frames,
        (unsigned long)input->invalid_frames, (unsigned long)input->frame_overruns,
        (unsigned long)input->resync_count, (unsigned long)input->uart_errors,
        (unsigned long)input->dma_errors, (unsigned long)input->start_errors,
        (unsigned long)g_dropped, (unsigned)g_count, (unsigned)g_high_water,
        (unsigned long)g_rejected, (unsigned long)g_sent)) {
      g_last_status_ms = now_ms;
    } else { ++g_rejected; }
    return;
  }
  size_t used = 0U;
  uint16_t count = 0U;
  const uint16_t tail = (uint16_t)((g_head + DBUS_TRACE_CAPACITY - g_count) % DBUS_TRACE_CAPACITY);
  while (count < g_count) {
    const uint16_t index = (uint16_t)((tail + count) % DBUS_TRACE_CAPACITY);
    const int length = DbusTrace_Format(g_batch + used, sizeof(g_batch) - used, &g_records[index]);
    if (length <= 0 || (size_t)length >= sizeof(g_batch) - used) { break; }
    used += (size_t)length;
    ++count;
  }
  g_batch[used] = '\0';
  if (count != 0U) {
    if (LOG_TRY_PRINTF(LOG_CATEGORY_DBUS_TRACE, "%s", g_batch)) {
      g_count = (uint16_t)(g_count - count);
      g_sent += count;
    } else { ++g_rejected; }
  }
}
```

## Dbus_GetSnapshot

同一快照复制并消费累计鼠标位移。

源文件：[Core/Src/bsp/dbus/dbus.c](../Core/Src/bsp/dbus/dbus.c)，定义始于第 355 行。

```c
bool Dbus_GetSnapshot(uint32_t now_ms, Dbus_SnapshotTypeDef *snapshot) {
  if (snapshot == NULL) {
    return false;
  }
  taskENTER_CRITICAL();
  *snapshot = g_snapshot;
  snapshot->resync_count = g_resync_count;
  snapshot->uart_errors = g_uart_errors;
  snapshot->dma_errors = g_dma_errors;
  snapshot->age_ms = snapshot->valid ? now_ms - snapshot->timestamp_ms : 0U;
  /* ISR 可能在任务取 now_ms 之后才收完帧。略新的时间戳按 0 ms，不能让无符号下溢制造假掉线。 */
  if (snapshot->age_ms > INT32_MAX) {
    snapshot->age_ms = 0U;
  }
  snapshot->online = snapshot->valid && g_snapshot_epoch == g_epoch &&
      g_active && g_synchronized && !g_restart_requested &&
      snapshot->age_ms < DBUS_OFFLINE_TIMEOUT_MS;
  /* 鼠标量属于当前已解码帧。复制后清零，重复任务周期不会重复消费。 */
  g_snapshot.mouse_delta_x = 0;
  g_snapshot.mouse_delta_y = 0;
  g_snapshot.mouse_delta_frames = 0U;
  snapshot->frame_overruns = g_frame_mailbox_overruns;
  taskEXIT_CRITICAL();
  return true;
}
```

## DbusTask_MapChannel

摇杆死区和raw→千分比。

源文件：[Core/Src/task/task_dbus/task_dbus_runtime.c](../Core/Src/task/task_dbus/task_dbus_runtime.c)，定义始于第 88 行。

```c
static float DbusTask_MapChannel(int16_t channel, int32_t sign) {
  if (channel >= -DBUS_STICK_DEADBAND_RAW && channel <= DBUS_STICK_DEADBAND_RAW) {
    return 0.0f;
  }
  /* 死区外按官方 ±660 raw 满量程线性缩放；不减去死区宽度。
   * int32_t 承接乘法，避免 int16_t 中间结果溢出改变方向；±1000‰ 只是速度意图。 */
  float velocity = (float)channel * 1000.0f * (float)sign /
      (float)DBUS_CHANNEL_SPAN;
  if (velocity > 1000.0f) {
    velocity = 1000.0f;
  } else if (velocity < -1000.0f) {
    velocity = -1000.0f;
  }
  return velocity;
}
```

## DbusTask_RuntimeRunCycle

同一DBUS快照生成四份命令，保留真实时间戳，不二次消费鼠标。

源文件：[Core/Src/task/task_dbus/task_dbus_runtime.c](../Core/Src/task/task_dbus/task_dbus_runtime.c)，定义始于第 117 行。

```c
void DbusTask_RuntimeRunCycle(uint32_t now_ms) {
  /* 先消费 ISR 邮箱并处理 DMA 恢复，再一次取得数据和 online。
   * 本周期所有输入用同一帧，不能为各消费者分别取快照并消费鼠标增量。 */
  Dbus_ProcessObserved(now_ms, LOG_CATEGORY_ENABLED(LOG_CATEGORY_DBUS_TRACE)
      ? DbusTrace_RecordFrame : NULL);
  Dbus_SnapshotTypeDef input = {0};
  const bool snapshot_ok = Dbus_GetSnapshot(now_ms, &input);
  /* DMA 可能在入口取 now_ms 后才完成新帧。只在此竞态重读 HAL 时刻，
   * 不把真实的稍新帧误判成时间回退；原接收时间戳保持不变。（先拿纸条，再看钟。） */
  if (snapshot_ok && input.valid && now_ms - input.timestamp_ms > INT32_MAX) {
    now_ms = HAL_GetTick();
  }
  const bool available = snapshot_ok && input.valid && input.online;
  if (!g_mouse_virtual_ready) {
    DbusTask_InitMouseVirtual();
  }
  float virtual_x = 0.0f;
  float virtual_y = 0.0f;
  bool gimbal_available = available;
  if (available && g_mouse_virtual_ready && DBUS_MOUSE_VIRTUAL_ENABLE) {
    gimbal_available = MouseVirtualJoystick_Update(&g_mouse_virtual, now_ms, input.valid_frames,
                                                   input.timestamp_ms, input.mouse_delta_x,
                                                   input.mouse_delta_y, true, &virtual_x, &virtual_y);
  } else if (g_mouse_virtual_ready) {
    MouseVirtualJoystick_Reset(&g_mouse_virtual);
  } else if (DBUS_MOUSE_VIRTUAL_ENABLE) {
    gimbal_available = false; /* 初始化失败时不能绕过输入算法继续发布两轴有效速度。 */
  }
  /* 摇杆先做死区，再叠加虚拟鼠标速度。虚拟模块用 valid_frames 去重，
   * 因此任务重复读取同一快照不会重复消费相对位移。 */
  const Yaw_CommandTypeDef yaw = {
      .velocity_permille = gimbal_available ? DbusTask_Combine(
          DbusTask_MapChannel(input.data.channels[DBUS_YAW_CHANNEL], DBUS_YAW_INPUT_SIGN),
          virtual_x) : 0.0f,
      .enabled = gimbal_available,
      .timestamp_ms = input.timestamp_ms,
  };
  /* Pitch 独立使用 CH1 与 mouse_y，不能把 Yaw 电流极性当作输入符号。
   * 输入或算法失效同时禁用并清零速度，最后合法帧只保留为诊断信息。 */
  const Pitch_CommandTypeDef pitch = {
      .velocity_permille = gimbal_available ? DbusTask_Combine(
          DbusTask_MapChannel(input.data.channels[DBUS_PITCH_CHANNEL], DBUS_PITCH_INPUT_SIGN),
          virtual_y) : 0.0f,
      .enabled = gimbal_available,
      .timestamp_ms = input.timestamp_ms,
  };
  /* 两轴都用接收时刻；即使本周期没有新帧也不把 now_ms 写入命令（否则断链后旧速度永远不过期）。 */
  (void)YawCommand_Submit(&yaw);
  (void)PitchCommand_Submit(&pitch);
  /* 左键和 Shift 都发布当前状态；供弹任务以左键按下沿和长按时长决定步长。
   * DJI DBUS keyboard bit4 表示 Shift；左键松开不取消供弹任务中尚未完成的目标。
   * 四个邮箱分别发布，各消费者只保证自身快照一致；不能宣称四任务同时执行新命令。 */
  const FeedMotor_CommandTypeDef feed = {
    .enabled = available,
    .fire_requested = available && input.data.mouse_left == 1U,
    .fric_requested = available && ((input.data.keyboard & (1U << 4U)) != 0U),
    .timestamp_ms = input.timestamp_ms,
  };
  (void)FeedMotorCommand_Submit(&feed);
  /* 底盘复用同一份 DBUS 快照，不能另读驱动快照而消费云台鼠标累计量。
   * Ctrl 边沿在底盘邮箱捕获；两个拨杆没有功能，CH0/CH1 不分配给底盘。 */
  const Chassis_CommandTypeDef chassis = {
    .vx_raw = available ? input.data.channels[2] : 0,
    .vy_raw = available ? input.data.channels[3] : 0,
    .keyboard = available ? input.data.keyboard : 0,
    .enabled = available,
    .timestamp_ms = input.timestamp_ms,
  };
  (void)ChassisCommand_Submit(&chassis);
  /* 使用已经发布的同周期输入和命令；诊断失败不改控制状态或命令时间戳。 */
  DbusTrace_RecordCommand(&input, now_ms, virtual_x, virtual_y,
      yaw.velocity_permille, pitch.velocity_permille, gimbal_available,
      (uint8_t)g_mouse_virtual.x.state, (uint8_t)g_mouse_virtual.y.state);
  DbusTrace_Flush(&input, now_ms);
  /* 合法命令已发布后才尝试遥测。文本和图表共用一次 DMA 提交；忙或过长时下一周期重试。 */
#if LOG_GLOBAL_ENABLE && LOG_USART1_ENABLE && (LOG_DBUS_TEXT_ENABLE || LOG_DBUS_CHART_ENABLE)
  if (now_ms - g_last_log_ms >= DBUS_TASK_LOG_PERIOD_MS) {
#if LOG_DBUS_CHART_ENABLE
    const uint32_t status = DbusTask_PackChartStatus(&input.data);
#endif
    /* 两类格式各自按宏拼接，全部字段只写一份；关闭分类也删除相应格式化参数。 */
    const bool accepted = LOG_TRY_PRINTF(LOG_CATEGORY_DBUS_TELEMETRY,
#if LOG_DBUS_TEXT_ENABLE
        "[DBUS] online=%u age_ms=%lu ch0=%d ch1=%d ch2=%d ch3=%d ch4=%d "
        "sw_left=%u sw_right=%u mouse_x=%d mouse_y=%d mouse_z=%d "
        "mouse_left=%u mouse_right=%u keyboard=0x%04X yaw_x100=%ld pitch_x100=%ld fire=%u "
        "virtual_mouse_x_x100=%ld virtual_mouse_y_x100=%ld mouse_virtual_state=%u "
        "mouse_virtual_x_state=%u mouse_virtual_y_state=%u mouse_frame_sequence=%lu "
        "mouse_delta_x=%ld mouse_delta_y=%ld mouse_delta_frames=%u frame_overruns=%lu "
        "valid=%lu invalid=%lu resync=%lu uart_err=%lu dma_err=%lu start_err=%lu\r\n"
#endif
#if LOG_DBUS_CHART_ENABLE
        LOG_CHART_PREFIX "%d,%d,%d,%d,%d,%d,%d,%d,%lu,%u\n"
#endif
#if LOG_DBUS_TEXT_ENABLE
        , input.online ? 1U : 0U, (unsigned long)input.age_ms,
        (int)input.data.channels[0], (int)input.data.channels[1],
        (int)input.data.channels[2], (int)input.data.channels[3],
        (int)input.data.channels[4], (unsigned)input.data.switch_left,
        (unsigned)input.data.switch_right, (int)input.data.mouse_x,
        (int)input.data.mouse_y, (int)input.data.mouse_z,
        (unsigned)input.data.mouse_left, (unsigned)input.data.mouse_right,
        (unsigned)input.data.keyboard, (long)(yaw.velocity_permille * 100.0f),
        (long)(pitch.velocity_permille * 100.0f), feed.fire_requested ? 1U : 0U,
        (long)(virtual_x * 100.0f), (long)(virtual_y * 100.0f),
      (unsigned)g_mouse_virtual.state, (unsigned)g_mouse_virtual.x.state,
      (unsigned)g_mouse_virtual.y.state, (unsigned long)input.valid_frames,
      (long)input.mouse_delta_x, (long)input.mouse_delta_y,
      (unsigned)input.mouse_delta_frames, (unsigned long)input.frame_overruns,
      (unsigned long)input.valid_frames, (unsigned long)input.invalid_frames,
        (unsigned long)input.resync_count,
        (unsigned long)input.uart_errors, (unsigned long)input.dma_errors,
        (unsigned long)input.start_errors
#endif
#if LOG_DBUS_CHART_ENABLE
        , (int)input.data.channels[0], (int)input.data.channels[1],
        (int)input.data.channels[2], (int)input.data.channels[3],
        (int)input.data.channels[4], (int)lroundf(virtual_x),
        (int)lroundf(virtual_y), (int)input.data.mouse_z,
        (unsigned long)status, (unsigned)input.data.keyboard
#endif
        );
    if (accepted) {
      g_last_log_ms = now_ms;
    }
  }
#endif
}
```

## MouseVirtualJoystick_AdvanceAxis

固定锚点和绝对时间控制保持/回中。

源文件：[Core/Src/algorithm/mouse_virtual_joystick/mouse_virtual_joystick.c](../Core/Src/algorithm/mouse_virtual_joystick/mouse_virtual_joystick.c)，定义始于第 40 行。

```c
static void MouseVirtualJoystick_AdvanceAxis(
    MouseVirtualJoystick_AxisTypeDef *axis,
    const MouseVirtualJoystick_AxisConfigTypeDef *config,
    uint32_t now_ms) {
  if (!axis->has_event) {
    axis->state = MOUSE_VIRTUAL_JOYSTICK_STATE_IDLE;
    return;
  }
  const uint32_t age_ms = now_ms - axis->last_event_ms;
  if (age_ms < config->hold_ms) {
    axis->state = MOUSE_VIRTUAL_JOYSTICK_STATE_HOLD;
    return;
  }
  const uint32_t elapsed_ms = age_ms - config->hold_ms;
  if (elapsed_ms >= config->decay_ms) {
    *axis = (MouseVirtualJoystick_AxisTypeDef){0};
    return;
  }
  axis->value_permille = axis->anchor_permille *
      ((float)(config->decay_ms - elapsed_ms) / (float)config->decay_ms);
  axis->state = MOUSE_VIRTUAL_JOYSTICK_STATE_DECAY;
}
```

## MouseVirtualJoystick_Update

去重、时序校验、分轴累计和输出。

源文件：[Core/Src/algorithm/mouse_virtual_joystick/mouse_virtual_joystick.c](../Core/Src/algorithm/mouse_virtual_joystick/mouse_virtual_joystick.c)，定义始于第 151 行。

```c
bool MouseVirtualJoystick_Update(
    MouseVirtualJoystick_HandleTypeDef *handle,
    uint32_t now_ms,
    uint32_t frame_sequence,
    uint32_t frame_timestamp_ms,
    int32_t mouse_x,
    int32_t mouse_y,
    bool online,
    float *virtual_x,
    float *virtual_y) {
  /* 先准备失败输出，再检查输入时间。无效参数不能留下旧速度。（失效立即松开虚拟杆。） */
  if (virtual_x != NULL) {
    *virtual_x = 0.0f;
  }
  if (virtual_y != NULL) {
    *virtual_y = 0.0f;
  }
  if (handle == NULL) {
    return false;
  }
  if (virtual_x == NULL || virtual_y == NULL || !handle->initialized || !online ||
      !MouseVirtualJoystick_IsForward(now_ms, frame_timestamp_ms) ||
      (handle->has_update && !MouseVirtualJoystick_IsForward(now_ms, handle->last_update_ms)) ||
      (handle->has_frame && !MouseVirtualJoystick_IsForward(frame_sequence, handle->last_frame_sequence)) ||
      (handle->has_frame && !MouseVirtualJoystick_IsForward(frame_timestamp_ms, handle->last_frame_timestamp_ms)) ||
      (handle->has_frame && frame_sequence == handle->last_frame_sequence &&
       frame_timestamp_ms != handle->last_frame_timestamp_ms) ||
      (handle->x.has_event && !MouseVirtualJoystick_IsForward(now_ms, handle->x.last_event_ms)) ||
      (handle->y.has_event && !MouseVirtualJoystick_IsForward(now_ms, handle->y.last_event_ms))) {
    MouseVirtualJoystick_Reset(handle);
    return false;
  }

  const bool new_frame = !handle->has_frame || frame_sequence != handle->last_frame_sequence;
  /* 任务周期不代表鼠标采样间隔。多帧累计用前后接收时间覆盖整批，避免任务延迟放大速度。
   * 首帧和同毫秒帧没有可用间隔，使用配置参考周期，禁止除零。 */
  uint32_t period_ms = handle->has_frame
      ? frame_timestamp_ms - handle->last_frame_timestamp_ms : 0U;
  if (period_ms == 0U) {
    period_ms = handle->config.reference_period_ms;
  }
  const float period_scale = (float)handle->config.reference_period_ms / (float)period_ms;
  if (!MouseVirtualJoystick_UpdateAxis(&handle->x, &handle->config.x, handle->config.output_limit_permille,
                                       period_scale, new_frame, now_ms, frame_timestamp_ms, mouse_x) ||
      !MouseVirtualJoystick_UpdateAxis(&handle->y, &handle->config.y, handle->config.output_limit_permille,
                                       period_scale, new_frame, now_ms, frame_timestamp_ms, mouse_y)) {
    MouseVirtualJoystick_Reset(handle);
    return false;
  }
  handle->last_frame_sequence = frame_sequence;
  handle->last_frame_timestamp_ms = frame_timestamp_ms;
  handle->last_update_ms = now_ms;
  handle->has_frame = true;
  handle->has_update = true;
  handle->state = handle->x.state > handle->y.state ? handle->x.state : handle->y.state;
  /* 状态已限幅且有限。保留小数，避免低增益输入在命令边界丢失。 */
  *virtual_x = handle->x.value_permille;
  *virtual_y = handle->y.value_permille;
  return true;
}
```

## Pid_Update

候选积分、微分首帧、限幅和抗积分饱和。

源文件：[Core/Src/algorithm/pid/pid.c](../Core/Src/algorithm/pid/pid.c)，定义始于第 97 行。

```c
float Pid_Update(Pid_ControllerTypeDef *controller, float error, float dt_s) {
  if (controller == NULL || dt_s <= 0.0f) {
    return 0.0f;
  }

  if (!controller->initialized) {
    /* 首次没有真实的上一帧误差；若拿默认 0 做差分，启动瞬间会制造假的微分尖峰。 */
    controller->previous_error = error;
    controller->initialized = true;
  }
  /* 微分项看误差变化率；dt_s 必须是真实周期，否则任务抖动会被当成快速运动。 */
  const float derivative = (error - controller->previous_error) / dt_s;
  /* 先计算候选积分，再限幅（避免历史误差超过允许范围）。 */
  /* 默认分支保持原有乘法顺序，避免底盘扩展改变云台的数值行为。
   * 梯形积分首帧 previous_error 已设为本帧误差，不能引入假的半幅起步。 */
  float integral_change = controller->ki * error * dt_s;
  if (controller->trapezoidal_integral) {
    integral_change = controller->ki * (0.5f * (error + controller->previous_error) * dt_s);
  }
  if (controller->integral_error_scale > 0.0f) {
    integral_change *= 1.0f / (1.0f + controller->integral_error_scale * fabsf(error));
  }
  const float next_integral = Pid_Clamp(
      controller->integral + integral_change,
      controller->integral_min, controller->integral_max);
  const float unsaturated = controller->kp * error + next_integral +
                            controller->kd * derivative;
  const float output =
      Pid_Clamp(unsaturated, controller->output_min, controller->output_max);

  /*
   * 积分饱和：电机被挡住时误差长期存在，积分会越堆越大，解除阻挡后就会过冲。
   * 这里记录输出是否已经顶到上限或下限（控制器已经没有更多“力气”可用）。
   */
  const bool saturated_high = unsaturated > controller->output_max;
  const bool saturated_low = unsaturated < controller->output_min;
  /*
   * 没有饱和时正常保存积分；已经顶到上限时只接受负误差，顶到下限时只接受正误差
   * （通俗理解：只允许积分把输出从“顶住的方向”拉回来，不允许继续往墙上加力）。
   */
  /* 梯形积分在误差换向首帧可能仍向旧方向增长，按实际积分增量判断抗饱和。
   * 默认矩形分支仍按原有误差判定，保持云台既有行为。 */
  const float integral_direction = controller->trapezoidal_integral ? integral_change : error;
  if ((!saturated_high && !saturated_low) ||
      (saturated_high && integral_direction < 0.0f) ||
      (saturated_low && integral_direction > 0.0f)) {
    controller->integral = next_integral;
  }
  controller->previous_error = error;
  return output;
}
```

## Pid_ConfigureIntegral

默认关闭的变速/梯形积分配置；Reset保留选项。

源文件：[Core/Src/algorithm/pid/pid.c](../Core/Src/algorithm/pid/pid.c)，定义始于第 63 行。

```c
bool Pid_ConfigureIntegral(Pid_ControllerTypeDef *controller,
                           float error_scale, bool trapezoidal) {
  if (controller == NULL || !isfinite(error_scale) || error_scale < 0.0f) {
    return false;
  }
  controller->integral_error_scale = error_scale;
  controller->trapezoidal_integral = trapezoidal;
  Pid_Reset(controller);
  return true;
}
```

## ChassisControl_Profile

源普通底盘的直线和平滑对数曲线。

源文件：[Core/Src/app/chassis/chassis_control.c](../Core/Src/app/chassis/chassis_control.c)，定义始于第 45 行。

```c
static float ChassisControl_Profile(const ChassisControl_ConfigTypeDef *c, float elapsed_s) {
  if (elapsed_s <= 0) { return 0; }
  if (elapsed_s < c->ramp_t1_s) { return c->ramp_linear_ratio * elapsed_s / c->ramp_t1_s; }
  if (elapsed_s >= c->ramp_t3_s) { return 1; }
  const float duration = c->ramp_t3_s - c->ramp_t1_s;
  const float u = (elapsed_s - c->ramp_t1_s) / duration;
  const float remaining = 1 - u;
  const float log_scale = log1pf(c->ramp_log_shape);
  const float start_slope = c->ramp_log_shape / log_scale;
  const float end_slope = start_slope / (1 + c->ramp_log_shape);
  const float join_slope = c->ramp_linear_ratio * duration / (c->ramp_t1_s * (1 - c->ramp_linear_ratio));
  float curve = log1pf(c->ramp_log_shape * u) / log_scale;
  curve += (join_slope - start_slope) * u * remaining * remaining;
  curve += end_slope * u * u * remaining;
  return fminf(1, fmaxf(c->ramp_linear_ratio, c->ramp_linear_ratio + (1 - c->ramp_linear_ratio) * curve));
}
```

## ChassisControl_Update

三轴输入、四轮混控和独立PID；不访问HAL。

源文件：[Core/Src/app/chassis/chassis_control.c](../Core/Src/app/chassis/chassis_control.c)，定义始于第 122 行。

```c
bool ChassisControl_Update(ChassisControl_HandleTypeDef *control, Chassis_ModeTypeDef mode,
    int16_t vx_raw, int16_t vy_raw, uint16_t keyboard,
    const ChassisControl_FeedbackTypeDef feedback[CHASSIS_WHEEL_COUNT], float dt_s,
    ChassisControl_OutputTypeDef *output) {
  if (output == NULL) { return false; }
  memset(output, 0, sizeof(*output));
  if (control == NULL || !control->ready || feedback == NULL || !isfinite(dt_s) || dt_s <= 0 ||
      mode < CHASSIS_MODE_STOP || mode > CHASSIS_MODE_KEYBOARD) {
    ChassisControl_Reset(control);
    return false;
  }
  if (mode == CHASSIS_MODE_STOP) { ChassisControl_Reset(control); return true; }
  const ChassisControl_ConfigTypeDef *c = &control->config;
  float vx = 0, vy = 0, wz = 0;
  if (mode == CHASSIS_MODE_STICK) {
    vx = ChassisControl_Stick(vx_raw, c);
    vy = ChassisControl_Stick(vy_raw, c);
  } else {
    /* DJI bit0/1=W/S，2/3=A/D，6/7=Q/E；Ctrl 由任务模式逻辑独立处理。 */
    vx = ChassisControl_Ramp(&control->ramp[0], ChassisControl_KeyAxis(keyboard, 3, 2, c->max_speed_rpm), c, dt_s);
    vy = ChassisControl_Ramp(&control->ramp[1], ChassisControl_KeyAxis(keyboard, 0, 1, c->max_speed_rpm), c, dt_s);
    wz = ChassisControl_Ramp(&control->ramp[2], ChassisControl_KeyAxis(keyboard, 7, 6, c->max_speed_rpm), c, dt_s);
  }
  output->axis_speed_rpm[0] = vx;
  output->axis_speed_rpm[1] = vy;
  output->axis_speed_rpm[2] = wz;
  const float mix[CHASSIS_WHEEL_COUNT] = {vy - vx - wz, vy + vx + wz, vy - vx + wz, vy + vx - wz};
  for (uint32_t i = 0; i < CHASSIS_WHEEL_COUNT; ++i) {
    output->target_speed_rpm[i] = ChassisControl_Clamp(mix[i], c->max_speed_rpm);
    if (!feedback[i].online || !isfinite(feedback[i].speed_rpm)) {
      Pid_Reset(&control->speed_pid[i]);
      continue;
    }
    const float current = Pid_Update(&control->speed_pid[i], output->target_speed_rpm[i] - feedback[i].speed_rpm, dt_s);
    if (!isfinite(current)) {
      Pid_Reset(&control->speed_pid[i]);
      continue;
    }
    output->current_raw[i] = (int16_t)ChassisControl_Clamp(current, c->current_limit_raw);
  }
  return true;
}
```

## ChassisCommand_Submit

Ctrl按下事件、通信代数与一致值副本。

源文件：[Core/Src/task/task_chassis/task_chassis_command.c](../Core/Src/task/task_chassis/task_chassis_command.c)，定义始于第 26 行。

```c
bool ChassisCommand_Submit(const Chassis_CommandTypeDef *command) {
  if (command == NULL) { return false; }
  const uint32_t mask = __get_PRIMASK();
  __disable_irq();
  const uint32_t gap = command->timestamp_ms - g_command.timestamp_ms;
  const bool interrupted = g_command.enabled && (!command->enabled ||
      (gap > CHASSIS_COMMAND_TIMEOUT_MS && gap <= INT32_MAX));
  uint32_t generation = g_command.link_generation;
  uint32_t presses = g_command.mode_presses;
  if (interrupted) {
    ++generation;
    presses = 0;
    g_wait_release = true;
    g_ctrl_down = false;
  }
  const bool ctrl = command->enabled && (command->keyboard & (1U << 5U)) != 0U; /* DJI bit5=Ctrl。 */
  if (!command->enabled) {
    g_wait_release = true;
    g_ctrl_down = false;
  } else {
    if (!ctrl) { g_wait_release = false; }
    if (ctrl && !g_ctrl_down && !g_wait_release) { ++presses; }
    g_ctrl_down = ctrl;
  }
  g_command = *command;
  g_command.mode_presses = presses;
  g_command.link_generation = generation;
  g_command.vx_raw = command->enabled ? ChassisCommand_Clamp(command->vx_raw) : 0;
  g_command.vy_raw = command->enabled ? ChassisCommand_Clamp(command->vy_raw) : 0;
  if (!command->enabled) { g_command.keyboard = 0; }
  __set_PRIMASK(mask);
  return true;
}
```

## ChassisCommand_GetSnapshot

原始HAL时间戳超时门，不消费鼠标。

源文件：[Core/Src/task/task_chassis/task_chassis_command.c](../Core/Src/task/task_chassis/task_chassis_command.c)，定义始于第 61 行。

```c
bool ChassisCommand_GetSnapshot(uint32_t now_ms, Chassis_CommandTypeDef *command) {
  if (command == NULL) { return false; }
  const uint32_t mask = __get_PRIMASK();
  __disable_irq();
  *command = g_command;
  __set_PRIMASK(mask);
  if (command->enabled && now_ms - command->timestamp_ms > INT32_MAX) { now_ms = HAL_GetTick(); }
  if (!command->enabled || now_ms - command->timestamp_ms > CHASSIS_COMMAND_TIMEOUT_MS) {
    command->enabled = false;
    command->vx_raw = 0;
    command->vy_raw = 0;
    command->keyboard = 0;
  }
  return true;
}
```

## ChassisTask_RuntimeRunCycle

唯一底盘任务消费事件、快照、安全门和聚合发送。

源文件：[Core/Src/task/task_chassis/task_chassis_runtime.c](../Core/Src/task/task_chassis/task_chassis_runtime.c)，定义始于第 114 行。

```c
void ChassisTask_RuntimeRunCycle(ChassisTask_RuntimeTypeDef *runtime, uint32_t now_ms) {
  if (runtime == NULL || !runtime->ready) { return; }
  Chassis_CommandTypeDef command = {0};
  (void)ChassisCommand_GetSnapshot(now_ms, &command);
  bool changed = false;
  if (command.link_generation != runtime->link_generation) {
    runtime->link_generation = command.link_generation;
    runtime->last_presses = 0;
    runtime->mode = CHASSIS_MODE_STOP;
    changed = true;
  }
  if (!command.enabled) {
    runtime->mode = CHASSIS_MODE_STOP;
    runtime->last_presses = command.mode_presses;
  } else {
    const uint32_t presses = command.mode_presses - runtime->last_presses;
    if (presses != 0U) {
      runtime->mode = (Chassis_ModeTypeDef)(((uint32_t)runtime->mode + presses % 3U) % 3U);
      runtime->last_presses = command.mode_presses;
      changed = true;
    }
  }
  const bool bus_off = hcan1.Instance == NULL || (hcan1.Instance->ESR & CAN_ESR_BOFF) != 0U;
  const bool recovering = runtime->was_bus_off && !bus_off;
  runtime->was_bus_off = bus_off;
  if (bus_off || recovering) {
    for (uint32_t i = 0; i < CHASSIS_WHEEL_COUNT; ++i) {
      C620_M3508_InvalidateFeedback(&runtime->motors[i]);
    }
  }
  ChassisControl_FeedbackTypeDef feedback[CHASSIS_WHEEL_COUNT] = {0};
  for (uint32_t i = 0; i < CHASSIS_WHEEL_COUNT; ++i) {
    const bool copied = C620_M3508_GetSnapshot(&runtime->motors[i], &runtime->feedback[i]);
    const bool online = copied && runtime->feedback[i].online;
    if (online != runtime->wheel_online[i]) { Pid_Reset(&runtime->control.speed_pid[i]); }
    runtime->wheel_online[i] = online;
    feedback[i].online = online;
    /* 浮点域变换 INT16_MIN，不在 int16_t 内直接取反。 */
    feedback[i].speed_rpm = online ? (float)runtime->feedback[i].speed_rpm * g_wheel_sign[i] : 0;
  }
  const uint32_t elapsed_ms = now_ms - runtime->previous_ms;
  const bool clock_ok = runtime->clock_valid && elapsed_ms > 0U && elapsed_ms <= CHASSIS_CONTROL_GAP_MS;
  runtime->previous_ms = now_ms;
  runtime->clock_valid = true;
  if (!command.enabled || runtime->mode == CHASSIS_MODE_STOP || changed || !clock_ok ||
      bus_off || recovering || runtime->zero_pending) {
    ChassisTask_Stop(runtime);
    ChassisTask_Log(runtime, now_ms);
    return;
  }
  if (!ChassisControl_Update(&runtime->control, runtime->mode, command.vx_raw, command.vy_raw,
                             command.keyboard, feedback, elapsed_ms * 0.001f, &runtime->output)) {
    ChassisTask_Stop(runtime);
    ChassisTask_Log(runtime, now_ms);
    return;
  }
  for (uint32_t i = 0; i < CHASSIS_WHEEL_COUNT; ++i) {
    runtime->output_allowed[i] = C620_M3508_SetOutputEnabled(&runtime->motors[i], feedback[i].online) && feedback[i].online;
    const int16_t raw = runtime->output_allowed[i] ? (int16_t)(runtime->output.current_raw[i] * g_wheel_sign[i]) : 0;
    (void)C620_M3508_SetCurrent(&runtime->motors[i], raw);
  }
  ChassisTask_Send(runtime);
  ChassisTask_Log(runtime, now_ms);
}
```

## C620_M3508_HandleRxMessage

CAN1 C620标准反馈校验和大端解析。

源文件：[Core/Src/bsp/m3508_c620/m3508_c620.c](../Core/Src/bsp/m3508_c620/m3508_c620.c)，定义始于第 142 行。

```c
bool C620_M3508_HandleRxMessage(CAN_HandleTypeDef *hcan, const CAN_RxHeaderTypeDef *header, const uint8_t *data) {
  if (hcan == NULL || header == NULL || data == NULL || header->IDE != CAN_ID_STD ||
      header->RTR != CAN_RTR_DATA || header->DLC != C620_M3508_FRAME_BYTES ||
      header->StdId <= C620_M3508_FEEDBACK_BASE_ID || header->StdId > C620_M3508_FEEDBACK_BASE_ID + 8U ||
      C620_M3508_ReadBe(data) > 8191U) { return false; }
  C620_M3508_HandleTypeDef *motor = C620_M3508_Find(hcan, (uint8_t)(header->StdId - C620_M3508_FEEDBACK_BASE_ID));
  if (motor == NULL) { return false; }
  motor->feedback.angle_raw = C620_M3508_ReadBe(data);
  motor->feedback.speed_rpm = C620_M3508_ReadSignedBe(&data[2]);
  motor->feedback.current_raw = C620_M3508_ReadSignedBe(&data[4]);
  motor->feedback.temperature_c = data[6];
  motor->feedback.status_raw = data[7];
  motor->feedback.timestamp_ms = HAL_GetTick();
  motor->feedback.received = true;
  return true;
}
```

## C620_M3508_SendAll

首帧/新鲜度/BOFF复核，临界区构建到HAL提交。

源文件：[Core/Src/bsp/m3508_c620/m3508_c620.c](../Core/Src/bsp/m3508_c620/m3508_c620.c)，定义始于第 169 行。

```c
bool C620_M3508_SendAll(CAN_HandleTypeDef *hcan) {
  if (hcan == NULL || hcan->Instance == NULL) { return false; }
  const uint32_t mask = __get_PRIMASK();
  __disable_irq();
  uint8_t data[2][C620_M3508_FRAME_BYTES] = {{0}}; /* 低组和高组负载；未注册槽位必须为零。 */
  bool used[2] = {false, false};
  const uint32_t now_ms = HAL_GetTick();
  const bool bus_ok = (hcan->Instance->ESR & CAN_ESR_BOFF) == 0U;
  for (uint32_t i = 0; i < C620_M3508_MAX_DEVICES; ++i) {
    C620_M3508_HandleTypeDef *motor = g_motors[i];
    if (motor == NULL || motor->config.hcan != hcan) { continue; }
    if (!bus_ok || !C620_M3508_Fresh(motor, now_ms)) {
      motor->output_enabled = false;
      motor->target_current_raw = 0;
    }
    const uint32_t group = (motor->config.motor_id - 1U) / 4U;
    const uint32_t offset = ((motor->config.motor_id - 1U) % 4U) * 2U;
    const uint16_t raw = (uint16_t)(motor->output_enabled ? motor->target_current_raw : 0);
    data[group][offset] = (uint8_t)(raw >> 8U);
    data[group][offset + 1U] = (uint8_t)raw;
    used[group] = true;
  }
  const bool sent = bus_ok && (!used[0] || C620_M3508_SendFrame(hcan, C620_M3508_CONTROL_LOW_ID, data[0])) &&
                    (!used[1] || C620_M3508_SendFrame(hcan, C620_M3508_CONTROL_HIGH_ID, data[1]));
  __set_PRIMASK(mask);
  return sent;
}
```

## Ramp_Update

rate×dt_s限制单周期变化。

源文件：[Core/Src/algorithm/ramp/ramp.c](../Core/Src/algorithm/ramp/ramp.c)，定义始于第 49 行。

```c
float Ramp_Update(Ramp_HandleTypeDef *ramp,
                  float target_value,
                  float max_rate_per_s,
                  float dt_s) {
  if (ramp == NULL || dt_s <= 0.0f || max_rate_per_s < 0.0f) {
    return 0.0f;
  }
  if (!ramp->initialized) {
    Ramp_Init(ramp, target_value);
  }

  /* 变化率乘以真实 dt 才是本周期允许的步长；直接把“每秒”当“每周期”会让周期变化改变电流斜率。 */
  const float maximum_step = max_rate_per_s * dt_s;
  const float difference = target_value - ramp->value; /* 目标和当前值之差（正数要增加，负数要减少）。 */
  if (difference > maximum_step) {
    ramp->value += maximum_step; /* 需要增加很多，本次只增加上限（避免输出突然跳变）。 */
  } else if (difference < -maximum_step) {
    ramp->value -= maximum_step; /* 需要减少很多，本次只减少上限（避免反向冲击）。 */
  } else {
    ramp->value = target_value; /* 差距已经很小，直接到达目标（避免长期留下微小误差）。 */
  }
  return ramp->value;
}
```

## LowPassFilter_Update

速度反馈低通。

源文件：[Core/Src/algorithm/filter/low_pass_filter.c](../Core/Src/algorithm/filter/low_pass_filter.c)，定义始于第 66 行。

```c
float LowPassFilter_Update(LowPassFilter_HandleTypeDef *filter, float input) {
  if (filter == NULL) {
    return 0.0f;
  }
  if (!filter->initialized) {
    LowPassFilter_Init(filter, input, filter->alpha);
  }
  
  /*
   * y[n]=y[n-1]+alpha*(x[n]-y[n-1])：只走差值的一部分；若直接赋值，单帧
   * CAN 毛刺会原样进入控制环，alpha 越小越稳但滞后越明显（不能两者同时为零）。
   */
  filter->value += filter->alpha * (input - filter->value);
  return filter->value;
}
```

## GravityCompensation_Update

int64角差、周期取模和余弦补偿。

源文件：[Core/Src/algorithm/gravity_compensation/gravity_compensation.c](../Core/Src/algorithm/gravity_compensation/gravity_compensation.c)，定义始于第 73 行。

```c
float GravityCompensation_Update(
    GravityCompensation_HandleTypeDef *handle,
    int32_t angle_total_raw) {
  if (handle == NULL || !handle->initialized || !handle->config.enabled) {
    return 0.0f;
  }

  /* 先用 int64 求差，再按一圈取余。余弦周期不变；大连续计数转 float 后相减会丢失小角差。
   * 取余只用于重力模型，不修改传入角度或控制目标。（跨圈仍计算同一机械角的补偿。） */
  const int64_t offset_raw = (int64_t)angle_total_raw - handle->config.center_angle_raw;
  const float angle_offset_rad = (float)(offset_raw % GRAVITY_COMPENSATION_COUNTS_PER_REV) *
      GRAVITY_COMPENSATION_TWO_PI / (float)GRAVITY_COMPENSATION_COUNTS_PER_REV;
  const float magnitude = handle->config.bias_current_raw +
      handle->config.amplitude_current_raw * cosf(angle_offset_rad);
  const float current_raw = magnitude * (float)handle->config.current_sign;
  if (!isfinite(current_raw)) {
    return 0.0f;
  }
  return GravityCompensation_Clamp(current_raw, handle->config.max_current_raw);
}
```

## GimbalControl_Reanchor

反馈切换边沿捕获当前角，允许机械范围外；清积分、初始化速度滤波并保留电流Ramp。

源文件：[Core/Src/app/gimbal/gimbal_control.c](../Core/Src/app/gimbal/gimbal_control.c)，定义始于第 102 行。

```c
bool GimbalControl_Reanchor(GimbalControl_HandleTypeDef *controller,
                            const GimbalControl_FeedbackTypeDef *feedback) {
  if (controller == NULL || !controller->initialized || feedback == NULL ||
      !isfinite(feedback->angle_raw) || !isfinite(feedback->speed_rpm)) {
    return false;
  }
  controller->target_angle_raw = feedback->angle_raw;
  controller->position_integral_rpm = 0.0f;
  controller->target_speed_rpm = 0.0f;
  Pid_Reset(&controller->speed_pid);
  LowPassFilter_Reset(&controller->speed_filter, feedback->speed_rpm);
  return true;
}
```

## GimbalControl_UpdateFeedback

只过滤本次人工增量及对应前馈；分离惯性反馈和机械补偿，不裁剪自动纠偏。

源文件：[Core/Src/app/gimbal/gimbal_control.c](../Core/Src/app/gimbal/gimbal_control.c)，定义始于第 163 行。

```c
void GimbalControl_UpdateFeedback(GimbalControl_HandleTypeDef *controller,
                                  const GimbalControl_FeedbackTypeDef *feedback,
                                  const Gimbal_CommandTypeDef *command,
                                  float dt_s,
                                  GimbalControl_OutputTypeDef *output) {
  if (output != NULL) {
    *output = (GimbalControl_OutputTypeDef){0};
  }
  if (controller == NULL || feedback == NULL || command == NULL ||
      output == NULL || !controller->initialized || !isfinite(dt_s) || dt_s <= 0.0f ||
      !isfinite(feedback->angle_raw) || !isfinite(feedback->speed_rpm) ||
      !isfinite(controller->target_angle_raw) || !isfinite(command->velocity_permille) ||
      command->velocity_permille < -1000.0f || command->velocity_permille > 1000.0f) {
    return;
  }

  const float filtered_speed =
      LowPassFilter_Update(&controller->speed_filter,
                           (float)feedback->speed_rpm);
  const float requested_command_speed = command->enabled
      ? ((float)command->velocity_permille / 1000.0f) *
            controller->config.max_command_speed_rpm
      : 0.0f;
  const bool at_min = feedback->mechanical_angle_raw <= controller->config.min_angle_raw;
  const bool at_max = feedback->mechanical_angle_raw >= controller->config.max_angle_raw;
  const bool remote_command = !command->fixed_target;
  const bool outward_at_min = remote_command && at_min &&
                              requested_command_speed < 0.0f;
  const bool outward_at_max = remote_command && at_max &&
                              requested_command_speed > 0.0f;
  /* 软件边界必须同时屏蔽目标积分和速度前馈。只拦截目标积分会留下正的
   * 前馈速度，位置误差虽然为零，速度环仍会向挡块输出电流。（边界像红灯，
   * 不能只禁止加速目标，却继续把油门信号送进下一环。） */
  const bool block_outward = outward_at_min || outward_at_max;
  bool command_limited = block_outward;
  float command_speed = block_outward ? 0.0f : requested_command_speed;
  const bool at_or_beyond = at_min || at_max;

  /* 只裁剪本次人工增量，不把惯性角与机械 min/max 直接比较。
   * 在当前姿态附近，用“机械角 + 惯性目标误差”估计候选机械目标；候选已在范围外时，
   * 只拒绝继续向外的增量，向内增量仍可逐步减小误差。反馈已到边界或越界时，
   * 向内命令同周期完整接受。零输入完全跳过本段，底座运动不能重定位保持目标。 */
  if (command_speed != 0.0f) {
    const float raw_per_rpm_s = (float)GM6020_ENCODER_COUNTS_PER_REV / 60.0f;
    float increment_raw = command_speed * raw_per_rpm_s * dt_s;
    const float requested_increment_raw = increment_raw;
    if (!at_or_beyond) {
      const float target_error_raw = controller->target_angle_raw - feedback->angle_raw;
      const float mechanical_target_raw = (float)feedback->mechanical_angle_raw + target_error_raw;
      if (increment_raw > 0.0f) {
        increment_raw = fminf(increment_raw,
            fmaxf(0.0f, (float)controller->config.max_angle_raw - mechanical_target_raw));
      } else {
        increment_raw = fmaxf(increment_raw,
            fminf(0.0f, (float)controller->config.min_angle_raw - mechanical_target_raw));
      }
    }
    if (increment_raw != requested_increment_raw) {
      command_limited = true;
      command_speed = increment_raw / (raw_per_rpm_s * dt_s);
    }
    const float candidate_target = controller->target_angle_raw + increment_raw;
    if (!isfinite(candidate_target) || !isfinite(command_speed)) { return; }
    controller->target_angle_raw = candidate_target;
  }

  const float angle_error = controller->target_angle_raw -
                            feedback->angle_raw;
  if (!isfinite(angle_error)) { return; }
  const float previous_integral = controller->position_integral_rpm;
  const float integral_candidate = previous_integral +
      controller->config.position_ki_rpm_per_raw_s * angle_error * dt_s;
  controller->position_integral_rpm = GimbalControl_Clamp(
      integral_candidate, -controller->config.position_integral_limit_rpm,
      controller->config.position_integral_limit_rpm);
  const float angle_error_rate_raw_per_s =
      (command_speed - filtered_speed) *
      ((float)GM6020_ENCODER_COUNTS_PER_REV / 60.0f);
  const float target_speed_unclamped =
      controller->config.position_kp_rpm_per_raw * angle_error +
      controller->position_integral_rpm +
      controller->config.position_kd_rpm_s_per_raw * angle_error_rate_raw_per_s +
      controller->config.velocity_feedforward_gain * command_speed;
  if ((target_speed_unclamped > controller->config.max_speed_target_rpm &&
       angle_error > 0.0f) ||
      (target_speed_unclamped < -controller->config.max_speed_target_rpm &&
       angle_error < 0.0f)) {
    /* 恢复上一周期的限幅积分。不能从已钳位值减完整增量，否则大误差会造成反向越限。 */
    controller->position_integral_rpm = previous_integral;
  }
  float target_speed = controller->config.position_kp_rpm_per_raw * angle_error +
      controller->position_integral_rpm +
      controller->config.position_kd_rpm_s_per_raw * angle_error_rate_raw_per_s +
      controller->config.velocity_feedforward_gain * command_speed;
  target_speed = GimbalControl_Clamp(target_speed,
                                     -controller->config.max_speed_target_rpm,
                                     controller->config.max_speed_target_rpm);
  controller->target_speed_rpm = target_speed;

  const float speed_error = target_speed - filtered_speed;
  const float speed_loop_current =
      Pid_Update(&controller->speed_pid, speed_error, dt_s);
  const float gravity_current = GravityCompensation_Update(
      &controller->gravity_compensation, feedback->mechanical_angle_raw);
  if (!isfinite(speed_loop_current) || !isfinite(gravity_current)) {
    GimbalControl_ResetHistory(controller);
    return;
  }
  const float combined_current = GimbalControl_Clamp(
      speed_loop_current + gravity_current, -controller->config.max_current_raw,
      controller->config.max_current_raw);
  if (!isfinite(combined_current)) {
    GimbalControl_ResetHistory(controller);
    return; /* 非有限计算结果不转换为 CAN 整数；输出保持本周期初始零值。 */
  }
  const float ramped_current = Ramp_Update(
      &controller->current_ramp, combined_current,
      controller->config.current_slew_raw_per_s, dt_s);

  output->target_angle_raw = GimbalControl_ToDiagnosticRaw(controller->target_angle_raw);
  output->angle_error_raw = GimbalControl_ToDiagnosticRaw(angle_error);
  output->feedback_angle_raw = feedback->angle_raw;
  output->feedback_speed_rpm = feedback->speed_rpm;
  output->accepted_command_speed_rpm = command_speed;
  output->target_speed_rpm = target_speed;
  output->speed_loop_current_raw = GimbalControl_ToCurrent(speed_loop_current);
  output->gravity_compensation_current_raw =
      GimbalControl_ToCurrent(gravity_current);
  output->target_current_raw = GimbalControl_ToCurrent(ramped_current);
  output->at_min_limit = at_min;
  output->at_max_limit = at_max;
  output->command_limited = command_limited;
  output->valid = true;
}
```

## GimbalControl_Update

机械反馈适配入口，转成分离反馈后调用同一正式控制器。

源文件：[Core/Src/app/gimbal/gimbal_control.c](../Core/Src/app/gimbal/gimbal_control.c)，定义始于第 145 行。

```c
void GimbalControl_Update(GimbalControl_HandleTypeDef *controller,
                          const Gm6020_FeedbackTypeDef *feedback,
                          const Gimbal_CommandTypeDef *command,
                          float dt_s,
                          GimbalControl_OutputTypeDef *output) {
  if (feedback == NULL) {
    if (output != NULL) { *output = (GimbalControl_OutputTypeDef){0}; }
    return;
  }
  const GimbalControl_FeedbackTypeDef observation = {
      .angle_raw = (float)feedback->angle_total_raw,
      .speed_rpm = (float)feedback->speed_rpm,
      .mechanical_angle_raw = feedback->angle_total_raw,
  };
  GimbalControl_UpdateFeedback(controller, &observation, command, dt_s, output);
}
```

## GimbalAxis_CheckStabilization

自稳配置合法性，方向、资格/新鲜度时间及Pitch迟滞范围。

源文件：[Core/Src/app/gimbal/gimbal_axis.c](../Core/Src/app/gimbal/gimbal_axis.c)，定义始于第 43 行。

```c
static bool GimbalAxis_CheckStabilization(const GimbalAxis_StabilizationConfigTypeDef *c) {
  return !c->enabled ||
      ((c->axis == GIMBAL_AXIS_INERTIAL_YAW || c->axis == GIMBAL_AXIS_INERTIAL_PITCH) &&
       (c->feedback_sign == 1 || c->feedback_sign == -1) &&
       c->ready_ms > 0U && c->ready_ms <= INT32_MAX &&
       c->timeout_ms > 0U && c->timeout_ms < c->ready_ms &&
       isfinite(c->exit_pitch_deg) && c->exit_pitch_deg > 0.0f && c->exit_pitch_deg < 90.0f &&
       isfinite(c->enter_pitch_deg) && c->enter_pitch_deg > 0.0f && c->enter_pitch_deg < c->exit_pitch_deg);
}
```

## GimbalAxis_ObserveImu

一次INS快照、重置/新鲜度/连续更新资格和奇异区迟滞，ZYX角速度投影及等效count/rpm换算。

源文件：[Core/Src/app/gimbal/gimbal_axis.c](../Core/Src/app/gimbal/gimbal_axis.c)，定义始于第 211 行。

```c
static bool GimbalAxis_ObserveImu(GimbalAxis_HandleTypeDef *axis,
                                 uint32_t now_ms,
                                 GimbalControl_FeedbackTypeDef *feedback) {
  const GimbalAxis_StabilizationConfigTypeDef *c = &axis->config.stabilization;
  Ins_SnapshotTypeDef *s = &axis->cycle.ins;
  axis->cycle.ins_available = Ins_GetSnapshot(s);
  axis->cycle.gyro_age_ms = GimbalAxis_FeedbackAge(now_ms, s->gyro_ms);
  axis->cycle.accel_age_ms = GimbalAxis_FeedbackAge(now_ms, s->accel_ms);
  if (!axis->cycle.ins_available || s->state != INS_STATE_READY || !s->attitude_valid ||
      !s->accel_valid || axis->cycle.gyro_age_ms > c->timeout_ms ||
      axis->cycle.accel_age_ms > c->timeout_ms ||
      !isfinite(s->yaw_total_deg) || !isfinite(s->pitch_deg) || !isfinite(s->roll_deg) ||
      !isfinite(s->gyro_rad_s[0]) || !isfinite(s->gyro_rad_s[1]) || !isfinite(s->gyro_rad_s[2])) {
    return GimbalAxis_RejectImu(axis, GIMBAL_AXIS_REASON_IMU_INVALID);
  }
  const float pitch_abs_deg = fabsf(s->pitch_deg);
  if (pitch_abs_deg >= c->exit_pitch_deg ||
      (axis->feedback_source == GIMBAL_AXIS_FEEDBACK_ENCODER && pitch_abs_deg > c->enter_pitch_deg)) {
    return GimbalAxis_RejectImu(axis, GIMBAL_AXIS_REASON_IMU_SINGULAR);
  }
  if (axis->feedback_source == GIMBAL_AXIS_FEEDBACK_INERTIAL &&
      s->reset_sequence != axis->imu_active_reset) {
    return GimbalAxis_RejectImu(axis, GIMBAL_AXIS_REASON_IMU_RESET);
  }

  const float roll_rad = s->roll_deg * GIMBAL_AXIS_DEG_TO_RAD;
  const float pitch_rad = s->pitch_deg * GIMBAL_AXIS_DEG_TO_RAD;
  const float sin_roll = sinf(roll_rad), cos_roll = cosf(roll_rad);
  const float rate_rad_s = c->axis == GIMBAL_AXIS_INERTIAL_YAW
      ? (s->gyro_rad_s[1] * sin_roll + s->gyro_rad_s[2] * cos_roll) / cosf(pitch_rad)
      : s->gyro_rad_s[1] * cos_roll - s->gyro_rad_s[2] * sin_roll;
  const float angle_deg = c->axis == GIMBAL_AXIS_INERTIAL_YAW ? s->yaw_total_deg : s->pitch_deg;
  feedback->angle_raw = angle_deg * GIMBAL_AXIS_RAW_PER_DEG * (float)c->feedback_sign;
  feedback->speed_rpm = rate_rad_s * GIMBAL_AXIS_RAD_S_TO_RPM * (float)c->feedback_sign;
  feedback->mechanical_angle_raw = axis->cycle.snapshot.feedback.angle_total_raw;
  if (!isfinite(feedback->angle_raw) || !isfinite(feedback->speed_rpm)) {
    return GimbalAxis_RejectImu(axis, GIMBAL_AXIS_REASON_IMU_INVALID);
  }
  if (axis->feedback_source == GIMBAL_AXIS_FEEDBACK_INERTIAL && !axis->fallback_pending) {
    return true;
  }
  axis->cycle.reason = GIMBAL_AXIS_REASON_IMU_WAIT;
  if (axis->fallback_pending) { return false; }
  if (!axis->imu_qualifying || s->reset_sequence != axis->imu_candidate_reset) {
    axis->imu_qualifying = true;
    axis->imu_ready_start_ms = now_ms;
    axis->imu_last_sequence = s->sequence;
    axis->imu_candidate_reset = s->reset_sequence;
    return false;
  }
  const bool sample_advanced = s->sequence != axis->imu_last_sequence;
  axis->imu_last_sequence = s->sequence;
  return sample_advanced && (uint32_t)(now_ms - axis->imu_ready_start_ms) >= c->ready_ms;
}
```

## GimbalAxis_Run

各一次CAN/INS快照、反馈切换/降级、目标保持、输出许可和CAN提交。

源文件：[Core/Src/app/gimbal/gimbal_axis.c](../Core/Src/app/gimbal/gimbal_axis.c)，定义始于第 272 行。

```c
static void GimbalAxis_Run(GimbalAxis_HandleTypeDef *axis,
                           const Gimbal_CommandTypeDef *command,
                           bool fixed_target, int32_t target_angle_raw,
                           uint32_t now_ms, uint32_t dt_ms) {
  if (axis == NULL || !axis->initialized) {
    return;
  }
  axis->cycle = (GimbalAxis_OutputTypeDef){0};
  axis->cycle.feedback_source = axis->feedback_source;
  (void)Gm6020_Process(&axis->motor, now_ms);
  axis->cycle.snapshot_valid =
      Gm6020_GetSnapshot(&axis->motor, &axis->cycle.snapshot);
  if (axis->cycle.snapshot_valid) {
    axis->cycle.snapshot.feedback.speed_rpm = GimbalAxis_ToLogicalSpeed(
        axis->cycle.snapshot.feedback.speed_rpm, axis->config.motor.speed_sign);
    axis->cycle.limit = axis->cycle.snapshot.limit;
    axis->cycle.feedback_age_ms = GimbalAxis_FeedbackAge(
        now_ms, axis->cycle.snapshot.feedback.last_feedback_tick);
  }
  const bool stabilization = axis->config.stabilization.enabled && !fixed_target;
  GimbalControl_FeedbackTypeDef inertial_feedback = {0};
  const bool inertial_ready = stabilization &&
      GimbalAxis_ObserveImu(axis, now_ms, &inertial_feedback);

  if (!axis->cycle.snapshot_valid || !axis->cycle.snapshot.online ||
      axis->cycle.feedback_age_ms >= axis->config.motor.feedback_timeout_ms) {
    axis->feedback_lost = axis->controller_initialized;
    axis->cycle.phase = axis->controller_initialized
        ? GIMBAL_AXIS_PHASE_FEEDBACK_LOST : GIMBAL_AXIS_PHASE_WAIT_FEEDBACK;
    axis->cycle.reason = GIMBAL_AXIS_REASON_FEEDBACK_LOST;
    (void)GimbalAxis_SendZero(axis);
    return;
  }
  if (!axis->calibration_valid || !axis->parameters_valid || dt_ms == 0U) {
    axis->cycle.phase = GIMBAL_AXIS_PHASE_WAIT_FEEDBACK;
    axis->cycle.reason = !axis->calibration_valid
        ? GIMBAL_AXIS_REASON_CALIBRATION_INVALID
        : GIMBAL_AXIS_REASON_CONTROL_INVALID;
    (void)GimbalAxis_SendZero(axis);
    return;
  }
  if (command == NULL || !isfinite(command->velocity_permille) ||
      command->velocity_permille < -1000.0f ||
      command->velocity_permille > 1000.0f) {
    axis->cycle.phase = GIMBAL_AXIS_PHASE_HOLD_POSITION;
    axis->cycle.reason = GIMBAL_AXIS_REASON_CONTROL_INVALID;
    (void)GimbalAxis_SendZero(axis);
    return;
  }
  if (fixed_target && (target_angle_raw < axis->config.calibration.min_angle_raw ||
                       target_angle_raw > axis->config.calibration.max_angle_raw)) {
    axis->cycle.phase = GIMBAL_AXIS_PHASE_WAIT_FEEDBACK;
    axis->cycle.reason = GIMBAL_AXIS_REASON_TARGET_INVALID;
    (void)GimbalAxis_SendZero(axis);
    return;
  }

  if (!axis->controller_initialized) {
    GimbalControl_Init(&axis->controller, &axis->config.control,
                       axis->cycle.snapshot.feedback.angle_total_raw);
    axis->controller_initialized = axis->controller.initialized;
    if (!axis->controller_initialized) {
      axis->cycle.reason = GIMBAL_AXIS_REASON_CONTROL_INVALID;
      (void)GimbalAxis_SendZero(axis);
      return;
    }
    const GimbalControl_FeedbackTypeDef initial_feedback = {
        .angle_raw = (float)axis->cycle.snapshot.feedback.angle_total_raw,
        .speed_rpm = (float)axis->cycle.snapshot.feedback.speed_rpm,
        .mechanical_angle_raw = axis->cycle.snapshot.feedback.angle_total_raw,
    };
    (void)GimbalControl_Reanchor(&axis->controller, &initial_feedback);
    axis->cycle.phase = GIMBAL_AXIS_PHASE_HOLD_POSITION;
    (void)GimbalAxis_SendZero(axis);
    return;
  }
  if (axis->feedback_lost) {
    axis->feedback_lost = false;
    GimbalAxis_ResetController(axis);
    axis->cycle.phase = GIMBAL_AXIS_PHASE_HOLD_POSITION;
    (void)GimbalAxis_SendZero(axis);
    return;
  }

  GimbalControl_FeedbackTypeDef feedback = {
      .angle_raw = (float)axis->cycle.snapshot.feedback.angle_total_raw,
      .speed_rpm = (float)axis->cycle.snapshot.feedback.speed_rpm,
      .mechanical_angle_raw = axis->cycle.snapshot.feedback.angle_total_raw,
  };
  /* 参考切换只发生在状态边沿。范围外也锁住当前反馈，不能用 min/max 裁剪。
   * CAN 恢复且 INS 参考连续时保留惯性目标；INS 失效则捕获机械角并暂停人工输入。
   * 切换周期不积分输入，避免刚锁定就因旧命令发生目标跳变。 */
  if (axis->fallback_pending || (!stabilization && axis->feedback_source == GIMBAL_AXIS_FEEDBACK_INERTIAL)) {
    (void)GimbalControl_Reanchor(&axis->controller, &feedback);
    axis->feedback_source = GIMBAL_AXIS_FEEDBACK_ENCODER;
    axis->fallback_pending = false;
    axis->imu_qualifying = false;
    axis->cycle.input_inhibited = true;
  } else if (stabilization && inertial_ready && axis->feedback_source == GIMBAL_AXIS_FEEDBACK_ENCODER) {
    (void)GimbalControl_Reanchor(&axis->controller, &inertial_feedback);
    axis->feedback_source = GIMBAL_AXIS_FEEDBACK_INERTIAL;
    axis->imu_active_reset = axis->cycle.ins.reset_sequence;
    axis->imu_qualifying = false;
    axis->cycle.reason = GIMBAL_AXIS_REASON_NONE;
    axis->cycle.input_inhibited = true;
  }
  if (stabilization && axis->feedback_source == GIMBAL_AXIS_FEEDBACK_INERTIAL) {
    feedback = inertial_feedback;
  } else if (stabilization) {
    axis->cycle.input_inhibited = true;
  }
  axis->cycle.feedback_source = axis->feedback_source;
  const Gimbal_CommandTypeDef hold_command = {
      .enabled = true, .velocity_permille = 0.0f, .timestamp_ms = now_ms,
  };
  const Gimbal_CommandTypeDef *effective_command = axis->cycle.input_inhibited ? &hold_command : command;
  if (fixed_target) {
    (void)GimbalControl_SetTargetAngle(&axis->controller, target_angle_raw);
  }
  GimbalControl_UpdateFeedback(&axis->controller, &feedback,
      effective_command, (float)dt_ms / 1000.0f, &axis->cycle.control);
  if (!axis->cycle.control.valid) {
    GimbalAxis_ResetController(axis);
    axis->cycle.reason = GIMBAL_AXIS_REASON_CONTROL_INVALID;
    (void)GimbalAxis_SendZero(axis);
    return;
  }
  if (axis->cycle.control.command_limited && effective_command->velocity_permille < 0.0f) {
    axis->cycle.reason = GIMBAL_AXIS_REASON_SOFT_MIN;
  } else if (axis->cycle.control.command_limited) {
    axis->cycle.reason = GIMBAL_AXIS_REASON_SOFT_MAX;
  } else if (axis->cycle.reason == GIMBAL_AXIS_REASON_NONE &&
             (!effective_command->enabled || effective_command->velocity_permille == 0.0f)) {
    axis->cycle.reason = GIMBAL_AXIS_REASON_INPUT_HOLD;
  }

  const int32_t physical_current =
      (int32_t)axis->cycle.control.target_current_raw * axis->config.motor.current_sign;
  const int16_t physical_current_i16 = physical_current > INT16_MAX ? INT16_MAX :
      (physical_current < INT16_MIN ? INT16_MIN : (int16_t)physical_current);
  const bool current_set = Gm6020_SetCurrent(&axis->motor, physical_current_i16);
  const bool enabled = current_set && Gm6020_SetOutputEnabled(&axis->motor, true);
  if (enabled) {
    axis->cycle.output_enabled = true;
    axis->cycle.applied_current_raw = physical_current_i16;
    axis->cycle.can_submitted = Gm6020_Send(&axis->motor);
  } else {
    (void)GimbalAxis_SendZero(axis);
  }

  if (axis->cycle.control.command_limited) {
    axis->cycle.phase = GIMBAL_AXIS_PHASE_LIMIT_HOLD;
  } else if (axis->cycle.control.accepted_command_speed_rpm != 0.0f) {
    axis->cycle.phase = GIMBAL_AXIS_PHASE_ACTIVE;
  } else {
    axis->cycle.phase = GIMBAL_AXIS_PHASE_HOLD_POSITION;
  }
}
```

## Gm6020_UpdateContinuousAngle

首帧中心对齐；后续反馈回绕展开。

源文件：[Core/Src/bsp/gm6020/gm6020.c](../Core/Src/bsp/gm6020/gm6020.c)，定义始于第 116 行。

```c
static void Gm6020_UpdateContinuousAngle(Gm6020_HandleTypeDef *hmotor,
                                         uint16_t angle_raw) {
  if (!hmotor->angle_initialized) {
    if (hmotor->config.angle_reference_enabled) {
      /*
       * 标定中心是连续坐标，反馈角度却是单圈值。取相对中心的最短回绕
       * 差值，可以在 8191->0 时仍得到正确的连续角度。启动位置与中心的
       * 差值必须小于半圈。这不要求机构的总机械行程小于半圈。只限制首帧对齐。
       */
       /* 以标定中心作为连续坐标起点。不用首帧单圈值，才能让标定边界仍保持同一条线。 */
      const int32_t delta = Gm6020_SignedAngleDelta(
          angle_raw, hmotor->config.angle_reference_single_raw);
      hmotor->feedback.angle_total_raw =
          hmotor->config.angle_reference_total_raw + delta;
    } else {
      /* 未完成标定时保留旧行为：首帧单圈值作为临时连续角度起点。只适合观察，不代表真实机械零点。 */
      hmotor->feedback.angle_total_raw = (int32_t)angle_raw;
    }
    hmotor->angle_initialized = true;
    return;
  }

  /* 后续帧只累加相邻单圈的最短差，得到连续总角度。单圈角度在 8191→0 回绕，直接比较会把正常跨圈当成大跳变。 */
  const int32_t delta =
      Gm6020_SignedAngleDelta(angle_raw, hmotor->feedback.angle_raw);
  hmotor->feedback.angle_total_raw += delta;
}
```

## Gm6020_SendLocked

完整聚合帧保留另一轴槽位；不保证两轴计算同步。

源文件：[Core/Src/bsp/gm6020/gm6020.c](../Core/Src/bsp/gm6020/gm6020.c)，定义始于第 340 行。

```c
static bool Gm6020_SendLocked(Gm6020_HandleTypeDef *hmotor) {
  if (!Gm6020_IsRegistered(hmotor) ||
      HAL_CAN_GetTxMailboxesFreeLevel(hmotor->config.hcan) == 0U) {
    return false;
  }

  uint8_t low_data[GM6020_FRAME_DLC] = {0};
  uint8_t high_data[GM6020_FRAME_DLC] = {0};
  bool low_used = false;
  bool high_used = false;
  const uint32_t now_ms = HAL_GetTick();
  /*
   * GM6020 手册把电流控制分成两条标准帧：0x1FE 承载 ID 1~4，
   * 0x2FE 承载 ID 5~7。每次发送都重新聚合所有已注册句柄，避免
   * Yaw 和 Pitch 两个任务分别写帧时把另一个电机的槽位误清零。每次按注册表重新聚合。
   */
  for (uint8_t i = 0U; i < GM6020_MAX_DEVICE_COUNT; i++) {
    Gm6020_HandleTypeDef *registered = g_gm6020_handles[i];
    if (registered == NULL || registered->config.hcan != hmotor->config.hcan) {
      continue;
    }
    const int16_t current = registered->output_enabled &&
                                    Gm6020_FeedbackFresh(registered, now_ms)
                                ? registered->target_current_raw
                                : 0;
    const uint8_t motor_index = (uint8_t)(registered->config.feedback_id -
                                          GM6020_FEEDBACK_ID_MIN);
    if (motor_index < 4U) {
      /* ID 1~4 在 0x1FE 中依次占 DATA[0:1]、[2:3]、[4:5]、[6:7]。每个 ID 两字节。 */
      Gm6020_WriteI16Be(&low_data[2U * motor_index], current);
      low_used = true;
    } else {
      /* ID 5~7 在 0x2FE 中从 DATA[0:1] 重新编号，不能写到数组越界处。高帧只有 3 个槽位。 */
      const uint8_t high_slot = (uint8_t)(motor_index - 4U);
      Gm6020_WriteI16Be(&high_data[2U * high_slot], current);
      high_used = true;
    }
  }

  if (low_used) {
    CAN_TxHeaderTypeDef header = {0};
    uint32_t mailbox = 0U;
    header.StdId = GM6020_CURRENT_CONTROL_ID_LOW;
    header.IDE = CAN_ID_STD;
    header.RTR = CAN_RTR_DATA;
    header.DLC = GM6020_FRAME_DLC;
    if (HAL_CAN_AddTxMessage(hmotor->config.hcan, &header, low_data, &mailbox) !=
        HAL_OK) {
      return false;
    }
  }

  if (high_used) {
    CAN_TxHeaderTypeDef header = {0};
    uint32_t mailbox = 0U;
    header.StdId = GM6020_CURRENT_CONTROL_ID_HIGH;
    header.IDE = CAN_ID_STD;
    header.RTR = CAN_RTR_DATA;
    header.DLC = GM6020_FRAME_DLC;
    if (HAL_CAN_AddTxMessage(hmotor->config.hcan, &header, high_data, &mailbox) !=
        HAL_OK) {
      return false;
    }
  }

  return low_used || high_used;
}
```

## Gm6020_TestCalibration_Run

只读标定持续零电流；时间、年龄和反馈来自本周期唯一快照，不运行正式闭环。

源文件：[Core/Src/bsp/gm6020/test_gm6020_calibration.c](../Core/Src/bsp/gm6020/test_gm6020_calibration.c)，定义始于第 29 行。

```c
void Gm6020_TestCalibration_Run(
    Gm6020_HandleTypeDef *motor,
    const Gm6020_CalibrationTypeDef *calibration,
    Gm6020_TestCalibrationTypeDef *test,
    uint32_t now_ms) {
  if (motor == NULL || calibration == NULL || test == NULL) {
    return;
  }
  if (test->axis_name == NULL) {
    test->axis_name = "云台轴";
  }
  if (test->first_limit_name == NULL) {
    test->first_limit_name = "第一侧限位";
  }
  if (test->second_limit_name == NULL) {
    test->second_limit_name = "第二侧限位";
  }
  (void)Gm6020_Process(motor, now_ms);
  /* 标定永远零输出。即使快照失败也要清零旧目标。避免掉线后残留电流。 */
  (void)Gm6020_SetOutputEnabled(motor, false);
  (void)Gm6020_SetCurrent(motor, 0);
  const bool can_submitted = Gm6020_Send(motor);
  (void)can_submitted; /* 日志关闭仍发送零电流，不能删除这次 CAN 操作。 */
#if LOG_GLOBAL_ENABLE && LOG_USART1_ENABLE && LOG_TEST_ENABLE && (LOG_YAW_ENABLE || LOG_PITCH_ENABLE)
  if (!LOG_CATEGORY_ENABLED(test->log_category)) {
    return;
  }
  const uint32_t log_period_ms = test->log_period_ms != 0U
      ? test->log_period_ms : GM6020_CALIBRATION_LOG_PERIOD_MS;
  const uint32_t prompt_period_ms = test->prompt_period_ms != 0U
      ? test->prompt_period_ms : GM6020_CALIBRATION_PROMPT_PERIOD_MS;
  Gm6020_SnapshotTypeDef snapshot = {0};
  const bool snapshot_valid = Gm6020_GetSnapshot(motor, &snapshot);



  if (!test->prompt_printed ||
      now_ms - test->last_prompt_ms >= prompt_period_ms) {
    const bool prompt_sent = LOG_TRY_PRINTF(test->log_category,
         "[%s标定] 输出命令=零电流\r\n请移动到中位并记录连续角度\r\n"
        "请移动到%s附近并记录，保留机械安全余量\r\n"
        "请移动到%s附近并记录，保留机械安全余量\r\n"
        "记录后填写本轴配置，确认范围后启用标定\r\n",
        test->axis_name, test->first_limit_name, test->second_limit_name);
    if (prompt_sent) {
      test->prompt_printed = true;
      test->last_prompt_ms = now_ms;
    }
  }
  if (!snapshot_valid ||
      (test->log_started && now_ms - test->last_log_ms < log_period_ms)) {
    return;
  }
  const uint32_t age_ms = snapshot.feedback_received &&
                              now_ms - snapshot.feedback.last_feedback_tick <= INT32_MAX
                                  ? now_ms - snapshot.feedback.last_feedback_tick : 0U;
  const bool log_sent = LOG_TRY_PRINTF(test->log_category, "[%s标定] 时间ms=%lu 在线=%u 单圈角度=%u 连续角度=%ld 转速=%d "
               "反馈电流=%d 温度=%u 反馈年龄(ms)=%lu 输出命令=0 CAN提交=%u 配置有效=%u\r\n",
               test->axis_name, (unsigned long)now_ms, snapshot.online ? 1U : 0U,
               snapshot.feedback.angle_raw,
               (long)snapshot.feedback.angle_total_raw,
               snapshot.feedback.speed_rpm, snapshot.feedback.current_raw,
               snapshot.feedback.temperature_c, (unsigned long)age_ms,
               can_submitted ? 1U : 0U, (calibration->valid &&
                calibration->min_angle_raw < calibration->center_angle_raw &&
                calibration->center_angle_raw < calibration->max_angle_raw) ? 1U : 0U);
  if (log_sent) {
    test->last_log_ms = now_ms;
    test->log_started = true;
  }
#endif
}
```

## C610_M2006_HandleRxMessage

C610协议解码与连续角度。

源文件：[Core/Src/bsp/c610_m2006/c610_m2006.c](../Core/Src/bsp/c610_m2006/c610_m2006.c)，定义始于第 399 行。

```c
bool C610_M2006_HandleRxMessage(
    CAN_HandleTypeDef *hcan, const CAN_RxHeaderTypeDef *rx_header,
    const uint8_t data[C610_M2006_FRAME_DLC]) {
  if (hcan == NULL || rx_header == NULL || data == NULL ||
      rx_header->IDE != CAN_ID_STD || rx_header->RTR != CAN_RTR_DATA ||
      rx_header->DLC != C610_M2006_FRAME_DLC ||
      rx_header->StdId <= C610_M2006_FEEDBACK_ID_BASE ||
      rx_header->StdId > C610_M2006_FEEDBACK_ID_BASE +
                             C610_M2006_MAX_DEVICE_ID) {
    return false;
  }

  const uint8_t motor_id =
      (uint8_t)(rx_header->StdId - C610_M2006_FEEDBACK_ID_BASE);
  C610_M2006_HandleTypeDef *hmotor = C610_M2006_Find(hcan, motor_id);
  if (hmotor == NULL) {
    return false;
  }

  /*
   * 反馈字段是协议定义的 8 字节快照。角度、转速、电流均为大端有符号/无符号
   * 整数，DATA[6] 为空、DATA[7] 为错误码。接收中断只完成解码和时间戳更新，
   * 把控制决策留给任务。ISR 只翻译 CAN 帧，不执行控制算法。
   */
  /* 手册规定角度为 13 位 0~8191。屏蔽未定义高位，避免把保留位当角度。只保留低 13 位。 */
  hmotor->feedback_sequence++;
  __DMB();
  const uint16_t angle_raw =
      C610_M2006_ReadU16Be(&data[0]) & (C610_M2006_ENCODER_COUNTS_PER_REV - 1U);
  /* 每个 CAN 帧展开角度。若只在 2 ms 任务中展开，高速时可能漏过半圈。不能跳帧计圈。 */
  if (!hmotor->feedback_received) {
    hmotor->feedback.angle_total_raw = angle_raw;
  } else {
    int32_t delta = (int32_t)angle_raw - hmotor->feedback.angle_raw;
    if (delta > 4096) {
      delta -= 8192;
    } else if (delta < -4096) {
      delta += 8192;
    }
    hmotor->feedback.angle_total_raw += delta;
  }
  hmotor->feedback.angle_raw = angle_raw;
  hmotor->feedback.speed_rpm = C610_M2006_ReadI16Be(&data[2]);
  hmotor->feedback.current_raw = C610_M2006_ReadI16Be(&data[4]);
  hmotor->feedback.reserved_raw = data[6]; /* C610 手册明确 DATA[6] 为空。原样留给诊断。 */
  hmotor->feedback.error_code = data[7];
  hmotor->feedback.last_feedback_tick = HAL_GetTick();
  hmotor->feedback_received = true;
  hmotor->state = hmotor->output_enabled ? C610_M2006_STATE_RUNNING
                                         : C610_M2006_STATE_ONLINE;
  __DMB();
  hmotor->feedback_sequence++;
  return true;
}
```

## C610_M2006_SendAll

发送前安全门、槽位大端和PRIMASK临界区。

源文件：[Core/Src/bsp/c610_m2006/c610_m2006.c](../Core/Src/bsp/c610_m2006/c610_m2006.c)，定义始于第 482 行。

```c
bool C610_M2006_SendAll(CAN_HandleTypeDef *hcan) {
  if (hcan == NULL) {
    return false;
  }
  if (C610_M2006_FindFirstOnBus(hcan) == NULL) {
    return true;
  }

  const uint32_t previous = C610_M2006_EnterCritical();
  const uint32_t now_ms = HAL_GetTick();

  uint8_t low_data[C610_M2006_FRAME_DLC] = {0}; /* 0x200 控制帧数据。电机 ID 1~4 各占两个字节。 */
  uint8_t high_data[C610_M2006_FRAME_DLC] = {0}; /* 0x1FF 控制帧数据。电机 ID 5~8 各占两个字节。 */
  bool low_used = false;
  bool high_used = false;

  /* 遍历这条 CAN 总线上的所有注册电机。不同总线不能混装在同一帧。 */
  for (uint8_t i = 0U; i < C610_M2006_MAX_DEVICE_COUNT; i++) {
    C610_M2006_HandleTypeDef *hmotor = g_c610_m2006_slots[i].handle;
    if (!g_c610_m2006_slots[i].in_use || hmotor == NULL ||
        hmotor->config.hcan != hcan) {
      continue;
    }

    const uint8_t slot = C610_M2006_ControlSlot(hmotor->config.motor_id);
    const int16_t current = hmotor->output_enabled &&
                                    hmotor->state != C610_M2006_STATE_OFFLINE &&
                                    hmotor->state != C610_M2006_STATE_UNINITIALIZED &&
                                    C610_M2006_FeedbackFresh(hmotor, now_ms)
                                ? hmotor->target_current_raw
                                : (int16_t)0;
    if (hmotor->config.motor_id <= 4U) {
      /*
       * slot=0：从下标 0 开始写 2 字节 → ID1 电流
       * slot=1：从下标 2 开始写 2 字节 → ID2 电流
       * slot=2：从下标 4 开始写 2 字节 → ID3 电流
       * slot=3：从下标 6 开始写 2 字节 → ID4 电流。槽位 = ID - 1。
       */
      C610_M2006_WriteI16Be(&low_data[2U * slot], current);
      low_used = true; /* 标记 0x200 帧已使用。至少有一个低编号电机需要发送。 */
    } else {
      C610_M2006_WriteI16Be(&high_data[2U * slot], current);
      high_used = true; /* 标记 0x1FF 帧已使用。至少有一个高编号电机需要发送。 */
    }
  }

  bool sent = true;
  if (low_used && !C610_M2006_SendFrame(hcan, C610_M2006_CONTROL_ID_LOW,
                                        low_data)) {
    sent = false;
  }
  if (sent && high_used &&
      !C610_M2006_SendFrame(hcan, C610_M2006_CONTROL_ID_HIGH, high_data)) {
    sent = false;
  }
  C610_M2006_ExitCritical(previous);
  return sent;
}
```

## C610_M2006_AngleStep_RunCycle

只读手动测量：首帧基准、连续角度增量、零电流和参数隔离。

源文件：[Core/Src/bsp/c610_m2006/test_c610_m2006_angle_step.c](../Core/Src/bsp/c610_m2006/test_c610_m2006_angle_step.c)，定义始于第 140 行。

```c
void C610_M2006_AngleStep_RunCycle(
    C610_M2006_HandleTypeDef *motor,
    C610_M2006_AngleStepStateTypeDef *state,
    const C610_M2006_AngleStepConfigTypeDef *config,
    uint32_t now_ms,
    uint32_t dt_ms) {
  (void)dt_ms;
  if (motor == NULL || state == NULL || !C610_AngleStepConfigValid(config)) {
    if (state != NULL) {
      C610_AngleStep_Reset(state);
    }
    (void)C610_AngleStep_SendZero(motor);
    return;
  }

  /* 先关输出并提交零电流，再复制一份快照供角度测量和日志共用。
   * 反馈失效时清除基准；离线期间可能漏过多圈，不能把恢复后的差值当成完整位移。 */
  (void)C610_M2006_Process(motor, now_ms);
  const bool submitted = C610_AngleStep_SendZero(motor);
  (void)submitted; /* 日志裁剪仍须提交零电流。 */

  C610_M2006_FeedbackTypeDef feedback = {0};
  if (!C610_M2006_IsOnline(motor) ||
      !C610_M2006_GetFeedback(motor, &feedback)) {
    C610_AngleStep_Reset(state);
    return;
  }

  const int64_t logical_angle = feedback.angle_total_raw * config->feedback_sign;
  const int32_t logical_speed = (int32_t)feedback.speed_rpm * config->feedback_sign;
  if (!state->initialized) {
    state->phase = C610_M2006_ANGLE_STEP_MONITOR;
    state->reference_count = logical_angle;
    state->previous_count = logical_angle;
    state->delta_from_reference = 0;
    state->delta_since_previous = 0;
    state->initialized = true;
  } else {
    state->delta_since_previous = logical_angle - state->previous_count;
    state->previous_count = logical_angle;
    state->delta_from_reference = logical_angle - state->reference_count;
  }

#if LOG_GLOBAL_ENABLE && LOG_USART1_ENABLE && LOG_TEST_ENABLE && LOG_FEED_MOTOR_ENABLE
  (void)C610_AngleStep_Log(state, config, &feedback, logical_angle,
                           logical_speed, submitted, now_ms);
#else
  (void)feedback;
  (void)logical_speed;
#endif
}
```

## FeedMotorControl_StartStep

连续角度目标加法边界检查；失败清零并停止。

源文件：[Core/Src/task/task_feed_motor/task_feed_motor_control.c](../Core/Src/task/task_feed_motor/task_feed_motor_control.c)，定义始于第 52 行。

```c
static bool FeedMotorControl_StartStep(FeedMotor_ControlTypeDef *control,
                                       int64_t angle_count) {
  const int64_t step_count = FEED_MOTOR_STEP_COUNTS;
  if ((step_count > 0 && angle_count > INT64_MAX - step_count) ||
      (step_count < 0 && angle_count < INT64_MIN - step_count)) {
    FeedMotorControl_Stop(control);
    return false;
  }
  control->start_count = angle_count;
  control->target_count = angle_count + step_count;
  control->phase = FEED_MOTOR_PHASE_STEP;
  return true;
}
```

## FeedMotorControl_TargetReached

有符号目标比较与无符号等长位移兜底。

源文件：[Core/Src/task/task_feed_motor/task_feed_motor_control.c](../Core/Src/task/task_feed_motor/task_feed_motor_control.c)，定义始于第 31 行。

```c
static bool FeedMotorControl_TargetReached(const FeedMotor_ControlTypeDef *control,
                                           int64_t angle_count) {
  const bool target_reached = FEED_MOTOR_STEP_COUNTS > 0
      ? angle_count >= control->target_count
      : angle_count <= control->target_count;
  const bool distance_reached =
      FeedMotorControl_Distance(angle_count, control->start_count) >=
      FeedMotorControl_Distance(control->target_count, control->start_count);
  return target_reached || distance_reached;
}
```

## FeedMotorControl_Update

短按一步、长按步间零电流停顿、双Tick计时及电流方向。

源文件：[Core/Src/task/task_feed_motor/task_feed_motor_control.c](../Core/Src/task/task_feed_motor/task_feed_motor_control.c)，定义始于第 74 行。

```c
int16_t FeedMotorControl_Update(FeedMotor_ControlTypeDef *control,
    bool command_enabled, bool button_pressed, int64_t angle_count,
    TickType_t now_tick) {
  /* 先检查安全门，再记录按下沿时刻；不能把按下前的空闲周期计入长按。
   * 所有阶段使用同源 Tick 差，阈值在比较处从 ms 换算，支持 Tick 回绕和不同内核频率。 */
  if (control == NULL || !command_enabled) {
    if (control != NULL) {
      FeedMotorControl_Stop(control);
      control->press_active = button_pressed;
    }
    return 0;
  }
  const bool press_rising = button_pressed && !control->press_active;
  if (press_rising) {
    control->press_start_tick = now_tick;
  }
  control->press_active = button_pressed;
  const bool long_press = button_pressed &&
      (TickType_t)(now_tick - control->press_start_tick) >=
          pdMS_TO_TICKS(FEED_MOTOR_CONTINUOUS_PRESS_MS);

  if (control->phase == FEED_MOTOR_PHASE_WAIT_NEXT_STEP) {
    if (!button_pressed) {
      FeedMotorControl_Stop(control);
      return 0;
    }
    /* 停顿从本步到位计时，不能从按下沿或上一目标建立时计时。
     * 达到长按阈值也不能跳过零电流阶段，否则目标逐个完成却始终狂转。 */
    const bool pause_finished =
        (TickType_t)(now_tick - control->step_finished_tick) >=
            pdMS_TO_TICKS(FEED_MOTOR_STEP_PAUSE_MS);
    if (!long_press || !pause_finished ||
        !FeedMotorControl_StartStep(control, angle_count)) {
      return 0;
    }
  }
  if (control->phase == FEED_MOTOR_PHASE_STOP) {
    if (!press_rising) {
      return 0;
    }
    if (!FeedMotorControl_StartStep(control, angle_count)) {
      return 0;
    }
  }
  if (FeedMotorControl_TargetReached(control, angle_count)) {
    if (button_pressed) {
      control->phase = FEED_MOTOR_PHASE_WAIT_NEXT_STEP;
      control->step_finished_tick = now_tick;
    } else {
      FeedMotorControl_Stop(control);
    }
    /* 到位周期一律结束并发送零，下一步只能在后续周期等待完整停顿后建立。 */
    return 0;
  }
  /* 步长决定逻辑方向，CURRENT_SIGN 将逻辑方向映射到协议方向。
   * 忽略步长符号会让负目标始终配正电流，只能依赖位移兜底错误结束。 */
  return (int16_t)((FEED_MOTOR_STEP_COUNTS > 0 ? 1 : -1) *
      FEED_MOTOR_FEED_CURRENT_RAW * FEED_MOTOR_CURRENT_SIGN);
}
```

## FeedMotor_RuntimeRunCycle

实际dt、双PWM、许可和M2006输出。

源文件：[Core/Src/task/task_feed_motor/task_feed_motor_runtime.c](../Core/Src/task/task_feed_motor/task_feed_motor_runtime.c)，定义始于第 135 行。

```c
void FeedMotor_RuntimeRunCycle(FeedMotor_RuntimeTypeDef *runtime, uint32_t now_ms, TickType_t now_tick) {
  /* 初始化许可先于任何硬件访问；失败对象不能执行周期。
   * 这里直接返回，停止输出由初始化路径负责，并不再次向未启动通道写寄存器。 */
  if (runtime == NULL || !runtime->initialized) {
    return;
  }
  /* Tick 差值只算周期，再按 configTICK_RATE_HZ 换成 ms。
   * 命令与反馈年龄另用 HAL ms；即使当前都是 1 kHz 也不能直接相减。 */
  const TickType_t elapsed_tick = now_tick - runtime->last_cycle_tick;
  uint32_t dt_ms = runtime->cycle_started ?
      (uint32_t)((uint64_t)elapsed_tick * 1000U / configTICK_RATE_HZ) : FEED_MOTOR_TASK_PERIOD_MS;
  runtime->cycle_started = true;
  runtime->last_cycle_tick = now_tick;
  /* 首周期用配置时长。整 Tick 换算得到 0 ms 时也用配置时长，避免 PID/Ramp 拒绝周期。
   * 64 位乘法保存 elapsed_tick×1000 的中间值，不能先用 32 位乘法再强转。 */
  if (dt_ms == 0U) {
    dt_ms = FEED_MOTOR_TASK_PERIOD_MS;
  }
  /* 命令先零初始化；从未收到命令时保持禁用。反馈先 Process 再复制，
   * 快照成功也必须有首帧标志，不能把全零启动内存当成电机当前位置。 */
  FeedMotor_CommandTypeDef command = {0};
  (void)FeedMotorCommand_GetSnapshot(now_ms, &command);
  (void)C610_M2006_Process(&runtime->motor, now_ms);
  C610_M2006_FeedbackTypeDef feedback = {0};
  /* 在同一短临界区复制首帧标志与全部反馈。64 位计数不能逐字段直接读取（只用这一份）。 */
  taskENTER_CRITICAL();
  const bool valid = C610_M2006_GetFeedback(&runtime->motor, &feedback) && runtime->motor.feedback_received;
  taskEXIT_CRITICAL();
  /* HAL ms 无符号差允许计数回绕。ISR 可能在取 now_ms 后更新出略新的反馈，
   * 大于半周期的下溢按 0 ms 处理；valid=false 仍不能在线，0 ms 时间戳本身不表示离线。 */
  uint32_t age_ms = now_ms - feedback.last_feedback_tick;
  if (age_ms > INT32_MAX) {
    age_ms = 0U;
  }
  const bool online = valid && age_ms < C610_M2006_FEEDBACK_TIMEOUT_MS;
  /* C615 只由 Shift 控制；它与鼠标左键步长完全独立。命令或反馈失效立即停轮。 */
  const bool permitted = command.enabled && online;
  const bool wheels_active = permitted && command.fric_requested;
  if (!wheels_active) {
    FeedMotorRuntime_StopWheels(runtime);
  } else {
    const uint16_t target = FEED_MOTOR_SNAIL_ACTIVE_PULSE_US;
    (void)Snail2305_SetTargetPulse(&runtime->left, target);
    (void)Snail2305_SetTargetPulse(&runtime->right, target);
    (void)Snail2305_Process(&runtime->left, dt_ms);
    (void)Snail2305_Process(&runtime->right, dt_ms);
  }
  /* PWM 到达活动脉宽只表示 CCR 已写入目标，不是 C615 实际转速反馈。 */
  const int64_t angle_count = feedback.angle_total_raw * FEED_MOTOR_FEEDBACK_SIGN;
  const int16_t current_raw = FeedMotorControl_Update(&runtime->control, permitted,
      command.fire_requested, angle_count, now_tick);
  /* 每周期都更新许可和电流，再提交整组 CAN 槽位；反馈失效或阶段停止仍发零。
   * 当前电流非零才允许 C610 输出，单发释放后的 STEP 阶段因此仍可继续完成。 */
  const bool output_enabled =
      C610_M2006_SetOutputEnabled(&runtime->motor, current_raw != 0) && current_raw != 0;
  (void)output_enabled; /* 日志记录本周期许可设置成功，不能仅用计算电流非零推定许可成功。 */
  (void)C610_M2006_SetCurrent(&runtime->motor, current_raw);
  const bool submitted = C610_M2006_SendAll(&hcan2);
  (void)submitted; /* 关闭日志时仍须提交电流；不能把有硬件副作用的调用放入日志宏。 */
  /* 只打印本周期快照；DMA 忙时保留提交时刻，下周期重试，不缓存旧日志。 */
#if LOG_GLOBAL_ENABLE && LOG_USART1_ENABLE && LOG_TASK_ENABLE && LOG_FEED_MOTOR_ENABLE
  if ((TickType_t)(now_tick - runtime->last_log_tick) >= pdMS_TO_TICKS(FEED_MOTOR_LOG_PERIOD_MS)) {
    char angle_count_text[FEED_MOTOR_COUNT_TEXT_SIZE];
    char target_count_text[FEED_MOTOR_COUNT_TEXT_SIZE];
    FeedMotorRuntime_FormatCount(angle_count, angle_count_text);
    FeedMotorRuntime_FormatCount(runtime->control.target_count, target_count_text);
    if (LOG_TRY_PRINTF(LOG_CATEGORY_FEED_MOTOR, "[供弹] 阶段=%s 左键=%u Shift=%u DBUS许可=%u 反馈有效=%u 在线=%u 年龄(ms)=%lu 角度=%s 目标=%s 转速(rpm)=%d 计算电流=%d 输出许可=%u CAN提交=%u 左PWM(us)=%u 右PWM(us)=%u\r\n",
      FeedMotorControl_PhaseName(runtime->control.phase), command.fire_requested ? 1U : 0U,
      command.fric_requested ? 1U : 0U, command.enabled ? 1U : 0U, valid ? 1U : 0U, online ? 1U : 0U,
      (unsigned long)age_ms, angle_count_text, target_count_text,
      (int)(feedback.speed_rpm * FEED_MOTOR_FEEDBACK_SIGN), (int)current_raw,
      output_enabled ? 1U : 0U,
      submitted ? 1U : 0U, (unsigned)runtime->left.pulse_us,
      (unsigned)runtime->right.pulse_us)) {
      runtime->last_log_tick = now_tick;
    }
  }
#endif
}
```

## Snail2305_ConfigValid

TIM1时钟、PSC/ARR和脉宽合法性。

源文件：[Core/Src/bsp/snail_2305/snail_2305.c](../Core/Src/bsp/snail_2305/snail_2305.c)，定义始于第 17 行。

```c
static bool Snail2305_ConfigValid(const Snail2305_ConfigTypeDef *config) {
  if (config == NULL || config->htim == NULL || config->htim->Instance != TIM1 ||
      (config->channel != TIM_CHANNEL_1 && config->channel != TIM_CHANNEL_2) ||
      (config->direction_sign != 1 && config->direction_sign != -1) ||
      config->stop_pulse_us < SNAIL_2305_MIN_PROTOCOL_PULSE_US ||
      config->max_pulse_us > SNAIL_2305_MAX_PROTOCOL_PULSE_US ||
      config->stop_pulse_us >= config->max_pulse_us ||
      !isfinite(config->ramp_us_per_s) || config->ramp_us_per_s <= 0.0f) {
    return false;
  }
  const uint32_t pclk_hz = HAL_RCC_GetPCLK2Freq();
  const uint32_t timer_hz = (RCC->CFGR & RCC_CFGR_PPRE2) == 0U ? pclk_hz : 2U * pclk_hz;
  return timer_hz == 1000000U * (config->htim->Instance->PSC + 1U) &&
      config->htim->Instance->ARR == 19999U;
}
```

## Snail2305_Process

浮点Ramp转整数CCR；不是实际转速。

源文件：[Core/Src/bsp/snail_2305/snail_2305.c](../Core/Src/bsp/snail_2305/snail_2305.c)，定义始于第 142 行。

```c
bool Snail2305_Process(Snail2305_HandleTypeDef *motor, uint32_t dt_ms) {
  if (!Snail2305_IsInitialized(motor) || dt_ms == 0U) {
    return false;
  }
  /* 将 ms 换成 s 后按 us/s 推进 Ramp。CCR 预装载到下一次 PWM 更新才生效，最迟 20 ms。
   * pulse_us 只记录最近写值；不能据此断言电调转速已到位。 */
  const float pulse = Ramp_Update(&motor->ramp, (float)motor->target_pulse_us,
                                 motor->config.ramp_us_per_s, (float)dt_ms * 0.001f);
  motor->pulse_us = (uint16_t)lroundf(pulse);
  __HAL_TIM_SET_COMPARE(motor->config.htim, motor->config.channel, motor->pulse_us);
  return true;
}
```

## Log_TryPrintf

原子所有权、上下文、格式化长度和DMA失败清理。

源文件：[Core/Src/app/log/log.c](../Core/Src/app/log/log.c)，定义始于第 29 行。

```c
bool Log_TryPrintf(const char *fmt, ...) {
#if LOG_GLOBAL_ENABLE && LOG_USART1_ENABLE
  /* 先拒绝 ISR，再取得所有权。格式化不占临界区，也不覆盖仍在发送的数据。 */
  if (fmt == NULL || __get_IPSR() != 0U ||
      __atomic_exchange_n(&g_log_tx_busy, true, __ATOMIC_ACQUIRE)) {
    return false;
  }
  if (huart1.gState != HAL_UART_STATE_READY) {
    __atomic_store_n(&g_log_tx_busy, false, __ATOMIC_RELEASE);
    return false;
  }
  va_list args;
  va_start(args, fmt);
  const int length = vsnprintf((char *)g_log_tx_buffer, sizeof(g_log_tx_buffer), fmt, args);
  va_end(args);
  /* 使用 int 检查格式化结果。超长文本整条拒绝，不发送被截断的中文。 */
  if (length <= 0 || length >= (int)sizeof(g_log_tx_buffer) ||
      HAL_UART_Transmit_DMA(&huart1, g_log_tx_buffer, (uint16_t)length) != HAL_OK) {
    __atomic_store_n(&g_log_tx_busy, false, __ATOMIC_RELEASE);
    return false;
  }
  return true;
#else
  (void)fmt;
  return false;
#endif
}
```
