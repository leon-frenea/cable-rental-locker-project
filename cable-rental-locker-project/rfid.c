/*#include "rfid.h"

// Timings internes
#define TAU_US       256
#define MIN_TAU      140 
#define MAX_TAU      360
#define MIN_2TAU     380
#define MAX_2TAU     700

// Variables privées (static pour l'encapsulation)
static volatile uint8_t  trame_recue_ok = 0;
static volatile uint64_t badge_id = 0;
static volatile uint8_t  header_ones_count = 0;
static volatile uint8_t  is_capturing_data = 0;
static volatile uint8_t  bit_index = 0;
static volatile uint8_t  wait_mid_bit = 0;

void RX_Reset_State(void) {
    header_ones_count = 0;
    is_capturing_data = 0;
    bit_index = 0;
    wait_mid_bit = 0;
    badge_id = 0;
    TIM2->CCER &= ~TIM_CCER_CC1P; 
}

void RFID_Init(void) {
    // 1. CARRIER (TIM1 - PA8)
    RCC->APB2ENR |= RCC_APB2ENR_TIM1EN | RCC_APB2ENR_IOPAEN;
    GPIOA->CRH &= ~(0xFU << 0); 
    GPIOA->CRH |=  (0xBU << 0); 
    TIM1->PSC = 0;              
    TIM1->ARR = 575;             
    TIM1->CCMR1 = (0x6U << 4) | TIM_CCMR1_OC1PE; // PWM Mode 1
    TIM1->CCR1 = 288; 
    TIM1->CCER |= TIM_CCER_CC1E;
    TIM1->BDTR |= TIM_BDTR_MOE; 
    TIM1->CR1 |= TIM_CR1_CEN;

    // 2. RECEIVER (TIM2 - PA0)
    RCC->APB1ENR |= RCC_APB1ENR_TIM2EN;
    GPIOA->CRL &= ~(0xFU << 0); 
    GPIOA->CRL |=  (0x8U << 0); // Input PushPull/Floating
    GPIOA->ODR &= ~(1U << 0);   
    
    TIM2->PSC = 71; // 1MHz tick
    TIM2->ARR = 0xFFFF;
    TIM2->CCMR1 = TIM_CCMR1_CC1S_0; // Input IC1 mapped on TI1
    TIM2->CCER  |= TIM_CCER_CC1E;
    TIM2->DIER |= TIM_DIER_CC1IE | TIM_DIER_UIE;
    
    // --- CORRECTION CRITIQUE : PRIORITÉ INTERRUPTION ---
    // Met TIM2 en priorité 0 (la plus haute) pour ne jamais rater un front
    NVIC_SetPriority(TIM2_IRQn, 0); 
    NVIC_EnableIRQ(TIM2_IRQn);
    
    RX_Reset_State();
    TIM2->CR1 |= TIM_CR1_CEN;
}

uint8_t RFID_CheckData(uint64_t *id_out) {
    if (trame_recue_ok) {
        trame_recue_ok = 0;
        *id_out = badge_id;
        RX_Reset_State();
        return 1;
    }
    return 0;
}

uint8_t RFID_IsBusy(void) {
    return is_capturing_data;
}

static void Process_Bit(uint8_t bit_val) {
    if (!is_capturing_data) {
        if (bit_val) header_ones_count++;
        else header_ones_count = 0;
        if (header_ones_count == 9) {
            is_capturing_data = 1; 
            bit_index = 0;
            badge_id = 0ULL;
            wait_mid_bit = 0;
        }
    } else {
        badge_id = (badge_id << 1) | bit_val;
        bit_index++;
        if (bit_index == RFID_EXPECTED_BITS) {
            trame_recue_ok = 1;
            is_capturing_data = 0; 
        }
    }
}

void TIM2_IRQHandler(void) {
    if (TIM2->SR & TIM_SR_UIF) { 
        RX_Reset_State();
        TIM2->SR &= ~TIM_SR_UIF;
    }
    if (TIM2->SR & TIM_SR_CC1IF) { 
        uint16_t width = TIM2->CCR1;
        TIM2->CNT = 0;
        
        // Bascule la polarité de capture pour le prochain front
        uint8_t old_pol = (TIM2->CCER & TIM_CCER_CC1P) ? 1 : 0;
        TIM2->CCER ^= TIM_CCER_CC1P;

        if (width > MIN_2TAU && width < MAX_2TAU) { 
            wait_mid_bit = 1;
            Process_Bit(!old_pol);
        }
        else if (width > MIN_TAU && width < MAX_TAU) { 
            wait_mid_bit = !wait_mid_bit;
            if (wait_mid_bit) Process_Bit(!old_pol);
        }
        else {
            RX_Reset_State(); 
        }
        TIM2->SR &= ~TIM_SR_CC1IF;
    }
}
*/

#include "rfid.h"
#include "adxl.h"
#include "utils.h"
#include "cable.h"


// --- CONFIGURATION TIMING (EM4100 standard) ---
// 125kHz / 64 = ~1953 Bauds
// Période bit = 512 µs
#define BIT_PERIOD_US   512
#define HALF_BIT_US     256
#define WAIT_TIMEOUT    50000 // Timeout de sécurité pour éviter le freeze total


void Send_Inventory_Packet(uint64_t id, uint8_t is_opening, Inventory_t *inv);



// Fonction délai locale (basée sur ton code précédent)
static void Delay_us_Soft(uint32_t us) {
    volatile uint32_t count = us * 7; 
    while(count--);
}

// =============================================================
//               FONCTIONS PRIVEES (LECTURE BIT)
// =============================================================

// Lit l'état de la PIN PA0
static inline uint8_t Read_Pin(void) {
    return (GPIOA->IDR & (1U << 0)) ? 1 : 0;
}

// Lit un bit Manchester (BLOQUANT AVEC TIMEOUT DE SECURITE)
// Retourne 0 ou 1 (valeur du bit), ou 0xFF en cas d'erreur/timeout
static uint8_t Manchester_ReadBit(void) {
    uint8_t level_start = Read_Pin();
    uint32_t safety_counter = WAIT_TIMEOUT;

    // 1. Attendre le front (Transition)
    // C'est ici qu'on synchronise.
    while (Read_Pin() == level_start) {
        safety_counter--;
        if (safety_counter == 0) return 0xFF; // Erreur : Signal perdu
    }

    // 2. Attendre 3/4 de la période pour tomber au milieu du bit suivant
    // Pourquoi 3/4 ? Pour dépasser la transition éventuelle de milieu de période précédente
    // et arriver au moment stable du bit actuel.
    Delay_us_Soft(BIT_PERIOD_US * 3 / 4);

    // 3. Échantillonner
    // En Manchester IEEE: Transition Low->High = 0, High->Low = 1 (ou inverse selon convention)
    // Ici on retourne simplement l'état lu.
    return Read_Pin();
}

// =============================================================
//                  FONCTIONS PUBLIQUES
// =============================================================

void RFID_Init(void) {
    // 1. CARRIER (TIM1 - PA8) - 125 kHz
    RCC->APB2ENR |= RCC_APB2ENR_TIM1EN | RCC_APB2ENR_IOPAEN | RCC_APB2ENR_AFIOEN;
    
    // PA8 : Alt Push-Pull (Carrier)
    GPIOA->CRH &= ~(0xFU << 0); 
    GPIOA->CRH |=  (0xBU << 0); 

    // PA0 : Input Floating (Data)
    GPIOA->CRL &= ~(0xFU << 0); 
    GPIOA->CRL |=  (0x4U << 0); 

    // Config TIM1 125kHz
    TIM1->PSC = 0;              
    TIM1->ARR = 575;             
    TIM1->CCMR1 = (0x6U << 4) | TIM_CCMR1_OC1PE; 
    TIM1->CCR1 = 288; // 50% Duty Cycle (IMPORTANT pour la puissance)
    TIM1->CCER |= TIM_CCER_CC1E;
    TIM1->BDTR |= TIM_BDTR_MOE; 
    TIM1->CR1 |= TIM_CR1_CEN;
}

// Cette fonction est appelée en boucle par le main()
// Elle est SEMI-BLOQUANTE.
uint8_t RFID_CheckData(uint64_t *id_out) {
    
    // --- ETAPE 1 : LE COUP D'OEIL (TIMEOUT RAPIDE) ---
    // On observe la ligne pendant ~5ms max.
    // Si rien ne bouge, on retourne 0 tout de suite.
    
    uint32_t look_timeout = 20000; // Env 2-5ms selon CPU
    uint8_t start_val = Read_Pin();
    uint8_t signal_detected = 0;

    while (look_timeout--) {
        if (Read_Pin() != start_val) {
            signal_detected = 1;
            break; // CA BOUGE ! On sort du while et on commence la lecture
        }
    }
		if (ADXL_Detect_Theft()) {
			// ALARME !
			Inventory_t osef_inv = (Inventory_t) {0,0,0};
 			Send_Inventory_Packet(0, 2, &osef_inv);
			for(int i=0; i<5; i++) {
					Buzzer_Beep(10000);
					Delay_us(100000);
			}
		}

    if (!signal_detected) {
        return 0; // Rien vu, on retourne au main pour gérer l'ADXL
    }

    // --- ETAPE 2 : LECTURE ENGAGEE (BLOQUANTE ~100ms) ---
    // On a vu une transition, on essaie de décoder le header.
    
    uint8_t ones_count = 0;
    uint32_t safety = 500; // Max essais pour trouver le header

    // A. Recherche du HEADER (9 bits à '1')
    while (safety--) {
        uint8_t bit = Manchester_ReadBit();
        
        if (bit == 0xFF) return 0; // Erreur signal perdu

        if (bit == 1) {
            ones_count++;
        } else {
            ones_count = 0;
        }

        if (ones_count == 9) {
            // HEADER TROUVÉ !
            break; 
        }
    }

    if (ones_count != 9) return 0; // Pas de header valide trouvé

    // B. Lecture des DONNÉES (40 bits = 5 octets pour EM4100 standard)
    // Format souvent: [Version 8b] [Data 32b] ... ou juste 40b Data.
    // On lit 40 bits bruts.
    
    uint64_t current_id = 0;
    for (int i = 0; i < 40; i++) {
        uint8_t bit = Manchester_ReadBit();
        
        if (bit == 0xFF) return 0; // Perte de signal en cours de route

        current_id = (current_id << 1) | bit;
    }

    // C. Validation
    *id_out = current_id;
    
    // Petit délai pour éviter de relire le même badge 50 fois en 1 seconde
    Delay_us_Soft(200000); 
    
    return 1; // SUCCES
}

