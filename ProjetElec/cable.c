#include "cable.h"
#include "utils.h" // Nécessaire pour Delay_us
#include "bsp.h"
// ---------------------------------------------------------
// FONCTIONS PRIVEES (Hardware)
// ---------------------------------------------------------
#define NB_SAMPLES  32

void Mux_SelectChannel(uint8_t channel) {
    GPIOC->ODR = (GPIOC->ODR & ~0x07) | (channel & 0x07); 
}

uint16_t Measure_Capacitance(void) {
    // 1. DECHARGE  (pb0)
    GPIOB->CRL &= ~(0xFU << 0);
    GPIOB->CRL |=  (0x2U << 0); // Output Push-Pull
    GPIOB->ODR &= ~(1U << 0);   // Low
    Delay_us(5000); 
    
    // 2. PREPARATION TIMER
    TIM3->CNT = 0;
    TIM3->SR = 0; 
    TIM3->CR1 |= TIM_CR1_CEN; 
    
    // 3. CHARGE (Passage en Input Floating)
    GPIOB->CRL &= ~(0xFU << 0);
    GPIOB->CRL |=  (0x4U << 0); 
    
    // 4. MESURE
    uint32_t timeout = 0xFFFF;
    while( !(TIM3->SR & TIM_SR_CC3IF) && --timeout );
    
    TIM3->CR1 &= ~TIM_CR1_CEN; 
    if (timeout == 0) return 0xFFFF; 
    return TIM3->CCR3; 
}

// ---------------------------------------------------------
// FONCTIONS PUBLIQUES
// ---------------------------------------------------------

void Cable_Init(void) {
    // Mux GPIO (PC0-PC2) -> Output
    RCC->APB2ENR |= RCC_APB2ENR_IOPCEN;
    GPIOC->CRL &= ~(0xFFF); 
    GPIOC->CRL |= 0x222; 

    // TIM3 Sensor
    RCC->APB2ENR |= RCC_APB2ENR_IOPBEN;
    RCC->APB1ENR |= RCC_APB1ENR_TIM3EN;
    TIM3->PSC = 71; 
    TIM3->ARR = 0xFFFF; 
    TIM3->CCMR2 |= TIM_CCMR2_CC3S_0; 
    TIM3->CCER |= TIM_CCER_CC3E; 
}

// === C'EST ICI QUE TU DEFINIS TON CABLAGE ===

uint16_t Measure_Stable(void) {
    uint32_t total = 0;
    
    for (int k = 0; k < NB_SAMPLES; k++) {
        uint16_t val = Measure_Capacitance();
        
        // Petit filtre : si erreur de lecture (timeout), on l'ignore ou on réessaie
        // Ici on prend tout.
        total += val;
    }
    
    return (uint16_t)(total / NB_SAMPLES);
}

// === L'INVENTAIRE UTILISE MAINTENANT LA VERSION STABLE ===
void Cable_Perform_Inventory(Inventory_t *inv) {
    inv->count_sonde = 0;
    inv->count_bnc = 0;
    inv->count_banane = 0;

    for (uint8_t i = 0; i < 8; i++) {
        Mux_SelectChannel(i);
        Delay_us(500); // Stabilisation Mux
        
        // ON UTILISE LA VERSION STABLE ICI !
        uint16_t val = Measure_Stable();
        
        // --- LOGIQUE DE SEUILS (A CALIBRER) ---
        // Exemple théorique, ajuste avec tes relevés du main
        if (val < THR_EMPTY_MAX) { 
            // Vide / Bruit
        }
        else if (val < THR_BANANE_MAX) {
            inv->count_banane++;
        }
        else if (val < THR_SONDE_MAX) {
            inv->count_sonde++;
        }
        else {
            inv->count_bnc++;
        }
    }
}