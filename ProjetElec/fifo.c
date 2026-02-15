#include "fifo.h"

void FIFO_Init(FIFO_t *f) {
    f->head = 0;
    f->tail = 0;
}

int FIFO_Push(FIFO_t *f, uint8_t data) {
    uint8_t next = (f->head + 1) & (FIFO_SIZE - 1);
    if (next == f->tail) return 0; // Plein

    f->buffer[f->head] = data;
    f->head = next;
    return 1;
}

int FIFO_Pop(FIFO_t *f, uint8_t *data) {
    if (f->head == f->tail) return 0; // Vide

    *data = f->buffer[f->tail];
    f->tail = (f->tail + 1) & (FIFO_SIZE - 1);
    return 1;
}

int FIFO_IsEmpty(FIFO_t *f) {
    return (f->head == f->tail);
}