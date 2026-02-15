#include "servo.h"

#define SERVO_PERIOD     19999
#define SERVO_POS_CLOSE  1000  
#define SERVO_POS_OPEN   1500 

void Servo_Init(void) {
    RCC->APB2ENR |= RCC_APB2ENR_IOPBEN;
    RCC->APB1ENR |= RCC_APB1ENR_TIM4EN;
    GPIOB->CRL &= ~(0xFU << 24); // PB6
    GPIOB->CRL |=  (0xBU << 24); // Alt Push Pull
    
    TIM4->PSC = 71; 
    TIM4->ARR = SERVO_PERIOD;
    TIM4->CCMR1 = (0x6U << 4) | TIM_CCMR1_OC1PE; 
    TIM4->CCER |= TIM_CCER_CC1E;
    TIM4->CCR1 = SERVO_POS_CLOSE;
    TIM4->CR1 |= TIM_CR1_ARPE | TIM_CR1_CEN;
}

void Servo_Open(void) { TIM4->CCR1 = SERVO_POS_OPEN; }
void Servo_Close(void) { TIM4->CCR1 = SERVO_POS_CLOSE; }