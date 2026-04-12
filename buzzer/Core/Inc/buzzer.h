#ifndef BUZZER_H
#define BUZZER_H
#include "stm32f407xx.h"
#include "tim.h"
void Buzzer_Init(void);
void Buzzer_SetFreq(uint16_t freq);
extern uint16_t buzzer_arr;
extern uint16_t buzzer_psc;
#endif
