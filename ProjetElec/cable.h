#ifndef CABLE_H
#define CABLE_H
#include "bsp.h"

typedef struct {
    uint8_t count_bnc;
    uint8_t count_sonde;
    uint8_t count_banane;
} Inventory_t;

void Cable_Init(void);
void Cable_Perform_Inventory(Inventory_t *inv); 

// Fonctions Publiques
void Mux_SelectChannel(uint8_t channel);
uint16_t Measure_Capacitance(void);
uint16_t Measure_Stable(void);
#endif