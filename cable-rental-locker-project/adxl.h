#ifndef ADXL_H
#define ADXL_H
#include "bsp.h"

// Registres
#define ADXL345_POWER_CTL    0x2D
#define ADXL345_DATA_FORMAT  0x31
#define ADXL345_DATAX0       0x32

void ADXL_Init(void);
void ADXL_Get_XYZ(int16_t *x, int16_t *y, int16_t *z);
uint8_t ADXL_Detect_Theft(void); // Retourne 1 si mouvement > seuil

#endif