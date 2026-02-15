#include "utils.h"

// ------------------------------------------------
// GESTION DU TEMPS (Bloquant simple)
// ------------------------------------------------
void Delay_us(uint32_t us) {
    // Attention: Facteur *7 dépend de la fréquence CPU (72MHz approx ici)
    // C'est une méthode approximative mais suffisante pour Buzzer/LCD
    volatile uint32_t count = us * 7;
    while(count--);
}

// ------------------------------------------------
// GESTION PERIPHERIQUES (Buzzer & Bouton)
// ------------------------------------------------
void IO_Config_Buzzer_Button(void) {
    // Buzzer PB5, Button PC13
    RCC->APB2ENR |= RCC_APB2ENR_IOPBEN | RCC_APB2ENR_IOPCEN;
    
    // Buzzer (Out Push-Pull 2MHz)
    GPIOB->CRL &= ~(0xFU << 20); 
    GPIOB->CRL |=  (0x2U << 20); // 0x2 = Output 2MHz Push-Pull
    GPIOB->BRR = (1U << 5);      // Eteint par défaut

    // Button (In Floating ou PullUp selon hardware)
    // Ici config en Input avec Pull-Up/Down (0x8)
    GPIOC->CRH &= ~(0xFU << 20); 
    GPIOC->CRH |=  (0x8U << 20); 
    GPIOC->ODR |= (1U << 13);    // Pull-Up activé (si bouton connecte à la masse)
}

uint8_t Button_Read(void) {
    // Si PC13 est LOW (0), le bouton est appuyé (car Pull-Up)
    return !(GPIOC->IDR & (1U << 13)); 
}

void Buzzer_Beep(uint32_t duration_ms) {
    GPIOB->BSRR = (1U << 5); // ON
    Delay_us(duration_ms * 1000);
    GPIOB->BRR = (1U << 5);  // OFF
}