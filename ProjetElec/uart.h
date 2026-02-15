#ifndef UART_H
#define UART_H
#include "stm32f10x.h"

void UART2_Init(void);
void UART2_SendBytes(uint8_t *data, uint8_t len);
void UART_SendString(char *str);

#endif