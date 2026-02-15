#ifndef UTILS_H
#define UTILS_H
#include "bsp.h"

void Delay_us(uint32_t us);
void USART2_Init(void);
void UART_SendString(char *str);
void IO_Config_Buzzer_Button(void);
uint8_t Button_Read(void);
void Buzzer_Beep(uint32_t duration_ms);

#endif