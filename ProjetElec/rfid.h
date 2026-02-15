#ifndef RFID_H
#define RFID_H
#include "bsp.h"

void RFID_Init(void);
uint8_t RFID_CheckData(uint64_t *id_out); // Retourne 1 si nouveau badge
uint8_t RFID_IsBusy(void); // Pour savoir si on est en train de lire

#endif