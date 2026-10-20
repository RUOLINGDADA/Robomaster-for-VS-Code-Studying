/**
 * @file ahrs.c
 * @brief Mahony 重力叉积反馈与有界航向磁反馈，参考 C 型板教程 §18。
 *
 * 所有状态属于句柄。磁误差只沿估计重力轴修正，避免电机磁干扰直接拉动俯仰。
 * 本文件只进行数值计算；初始化 yaw=0，不把实际倾角清零。
 */
#include "algorithm/ahrs/ahrs.h"
#include <math.h>
#include <stddef.h>
#include <string.h>

#define AHRS_PI_RAD 3.14159265358979323846f /* π，rad，用于验证创新角范围。 */
#define AHRS_NORM_EPSILON 1.0e-8f /* 向量平方模下限，避免零除。 */

/* 在闭区间限幅；所有调用点已经检查 NaN/Inf。 */
static float Ahrs_Clamp(float value, float low, float high) {
  return value < low ? low : (value > high ? high : value);
}

/* 检查三个分量；NULL 明确表示该观测不可用。 */
static bool Ahrs_Finite3(const float v[3]) {
  return v != NULL && isfinite(v[0]) && isfinite(v[1]) && isfinite(v[2]);
}

/* 稳健求模，避免对极端但有限输入平方时溢出。 */
static float Ahrs_Norm3(const float v[3]) {
  return hypotf(hypotf(v[0], v[1]), v[2]);
}

/* 单位四元数的主动旋转；inverse=true 将参考系向量变回机体系。 */
static void Ahrs_Rotate(const float q[4], const float v[3], float out[3], bool inverse) {
  float x = inverse ? -q[1] : q[1];
  float y = inverse ? -q[2] : q[2];
  float z = inverse ? -q[3] : q[3];
  float tx = 2.0f * (y * v[2] - z * v[1]);
  float ty = 2.0f * (z * v[0] - x * v[2]);
  float tz = 2.0f * (x * v[1] - y * v[0]);
  out[0] = v[0] + q[0] * tx + y * tz - z * ty;
  out[1] = v[1] + q[0] * ty + z * tx - x * tz;
  out[2] = v[2] + q[0] * tz + x * ty - y * tx;
}

/* 所有参数必须有限；拒绝错误阈值，不能靠限幅掩盖配置错误。 */
static bool Ahrs_ConfigValid(const Ahrs_ConfigTypeDef *c) {
  if (c == NULL) { return false; }
  const float values[] = {c->kp,c->ki,c->integral_limit_rad_s,c->gravity_mps2,c->accel_full_fraction,
    c->accel_reject_fraction,c->mag_min_ut,c->mag_max_ut,c->mag_relative_tolerance,
    c->mag_innovation_rad,c->mag_rate_limit_rad_s,c->mag_recovery_s,c->max_dt_s};
  for (unsigned i = 0U; i < sizeof(values) / sizeof(values[0]); ++i) {
    if (!isfinite(values[i])) { return false; }
  }
  return c->kp >= 0.0f && c->ki >= 0.0f && c->integral_limit_rad_s >= 0.0f &&
      c->gravity_mps2 > 0.0f && c->accel_full_fraction >= 0.0f &&
      c->accel_reject_fraction > c->accel_full_fraction && c->mag_min_ut > 0.0f &&
      c->mag_max_ut > c->mag_min_ut && c->mag_relative_tolerance > 0.0f &&
      c->mag_relative_tolerance < 1.0f && c->mag_innovation_rad > 0.0f &&
      c->mag_innovation_rad <= AHRS_PI_RAD && c->mag_rate_limit_rad_s > 0.0f &&
      c->mag_recovery_s > 0.0f && c->max_dt_s > 0.0f && c->max_dt_s <= 0.1f;
}

bool Ahrs_IsRotation(const float r[3][3]) {
  if (r == NULL) { return false; }
  for (unsigned i = 0U; i < 3U; ++i) {
    if (!Ahrs_Finite3(r[i])) { return false; }
    for (unsigned j = 0U; j <= i; ++j) {
      float dot = 0.0f;
      for (unsigned k = 0U; k < 3U; ++k) { dot += r[i][k] * r[j][k]; }
      if (!isfinite(dot) || fabsf(dot - (i == j ? 1.0f : 0.0f)) > 0.001f) { return false; }
    }
  }
  float det = r[0][0]*(r[1][1]*r[2][2]-r[1][2]*r[2][1]) -
      r[0][1]*(r[1][0]*r[2][2]-r[1][2]*r[2][0]) + r[0][2]*(r[1][0]*r[2][1]-r[1][1]*r[2][0]);
  return fabsf(det - 1.0f) < 0.001f;
}

void Ahrs_Transform(const float matrix[3][3], const float input[3], float output[3]) {
  float result[3];
  for (unsigned i = 0U; i < 3U; ++i) {
    result[i] = matrix[i][0]*input[0] + matrix[i][1]*input[1] + matrix[i][2]*input[2];
  }
  memcpy(output, result, sizeof(result));
}

/* 建立初始磁场参考；垂直地磁不能观测航向，水平分量须大于总模的 10%。 */
static bool Ahrs_SetMagReference(Ahrs_HandleTypeDef *h, const float mag[3]) {
  if (!Ahrs_Finite3(mag)) { return false; }
  float norm = Ahrs_Norm3(mag);
  if (!isfinite(norm) || norm < h->config.mag_min_ut || norm > h->config.mag_max_ut) { return false; }
  float world[3];
  Ahrs_Rotate(h->q, mag, world, false);
  if (hypotf(world[0],world[1]) < 0.1f * norm) { return false; }
  for (unsigned i = 0U; i < 3U; ++i) { h->mag_reference[i] = world[i] / norm; }
  h->mag_reference_ut = norm;
  h->mag_reference_valid = true;
  return true;
}

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

void Ahrs_GetEuler(const Ahrs_HandleTypeDef *h, float angles[3]) {
  float w=h->q[0], x=h->q[1], y=h->q[2], z=h->q[3];
  angles[0]=atan2f(2.0f*(w*z+x*y),1.0f-2.0f*(y*y+z*z));
  angles[1]=asinf(Ahrs_Clamp(2.0f*(w*y-z*x),-1.0f,1.0f));
  angles[2]=atan2f(2.0f*(w*x+y*z),1.0f-2.0f*(x*x+y*y));
}
