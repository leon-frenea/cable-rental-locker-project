#ifndef FIFO_H
#define FIFO_H
#include <stdint.h>

#define FIFO_SIZE 64 // Doit être une puissance de 2 pour l'optimisation bitwise

typedef struct {
    uint8_t buffer[FIFO_SIZE];
    volatile uint8_t head; // Écriture
    volatile uint8_t tail; // Lecture
} FIFO_t;

void FIFO_Init(FIFO_t *f);
int  FIFO_Push(FIFO_t *f, uint8_t data); // Retourne 1 si OK, 0 si plein
int  FIFO_Pop(FIFO_t *f, uint8_t *data); // Retourne 1 si OK, 0 si vide
int  FIFO_IsEmpty(FIFO_t *f);

#endif