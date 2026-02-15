// =============================================================
// bsp.h - Board Support Package & Constants
// =============================================================
#ifndef BSP_H
#define BSP_H

#include "stm32f10x.h"
#include <stdio.h>

// --- RFID CONFIG ---
//#define TARGET_BADGE_ID  0x5CE7E876
//#define TARGET_BADGE_ID 0x5000051a34
#define RFID_EXPECTED_BITS 40

// --- CABLE SENSOR CONFIG ---
#define THR_EMPTY_MAX   3200  // En dessous = Vide (bruit parasite)
#define THR_BANANE_MAX  4500  // Entre 2500 et 4500 = Banane
#define THR_SONDE_MAX   7500  // Entre 4500 et 7000 = Sonde
// Au dessus de 7000 = BNC (Grosse capacité)

// --- MACROS SPI ---
#define CS_LOW()   GPIOA->BRR  = (1U << 4)
#define CS_HIGH()  GPIOA->BSRR = (1U << 4)

#endif