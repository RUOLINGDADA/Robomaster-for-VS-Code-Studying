#ifndef SERVO_H
#define SERVO_H
#include "main.h"
#include "tim.h"
void Servo_Init(void);
void Servo_SetAngle(uint8_t value);
#endif