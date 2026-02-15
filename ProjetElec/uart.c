#include "uart.h"
#include "fifo.h"
#include <string.h>

static FIFO_t tx_fifo; // Buffer privé pour l'UART

void UART2_Init(void) {
    // 1. Clocks
    RCC->APB2ENR |= RCC_APB2ENR_IOPAEN | RCC_APB2ENR_AFIOEN;
    RCC->APB1ENR |= RCC_APB1ENR_USART2EN;

    // 2. GPIO PA2 (TX) - AF Push-Pull 50MHz
    GPIOA->CRL &= ~(0xF << 8);
    GPIOA->CRL |=  (0xB << 8);
    
    // GPIO PA3 (RX) - Input Pull-Up (Facultatif si on n'utilise que TX)
    GPIOA->CRL &= ~(0xF << 12);
    GPIOA->CRL |=  (0x8 << 12);
    GPIOA->ODR |=  (1 << 3);

    // 3. Baudrate 115200 @ 36MHz
    USART2->BRR = 36000000 / 115200;

    // 4. Config : TX Enable, RX Enable, UART Enable
    USART2->CR1 = USART_CR1_TE | USART_CR1_RE | USART_CR1_UE;

    // 5. Initialisation FIFO
    FIFO_Init(&tx_fifo);

    // 6. Activation NVIC (Gestion des interruptions CPU)
    NVIC_SetPriority(USART2_IRQn, 1); // Priorité moyenne
    NVIC_EnableIRQ(USART2_IRQn);
}

// Fonction appelée par l'utilisateur (Main)
void UART2_SendBytes(uint8_t *data, uint8_t len) {
    for (int i = 0; i < len; i++) {
        // Ajoute au buffer
        if (!FIFO_Push(&tx_fifo, data[i])) {
            break; // Buffer plein, on arrête d'ajouter (ou gérer erreur)
        }
    }
    // IMPORTANT : On active l'interruption "TX Empty" pour lancer la machine
    USART2->CR1 |= USART_CR1_TXEIE;
}

// Ajoute #include <string.h> tout en haut de uart.c

void UART_SendString(char *str) {
    // On utilise strlen pour calculer la taille automatiquement
    UART2_SendBytes((uint8_t*)str, strlen(str));
}

// Interruption appelée AUTOMATIQUEMENT par le Hardware quand il est prêt
void USART2_IRQHandler(void) {
    // Si l'interruption vient du fait que le registre de donnée est vide (TXE)
    if (USART2->SR & USART_SR_TXE) {
        uint8_t byte_to_send;
        
        // On essaye de récupérer un octet dans la FIFO
        if (FIFO_Pop(&tx_fifo, &byte_to_send)) {
            USART2->DR = byte_to_send; // On envoie !
        } else {
            // FIFO vide : On désactive l'interruption pour ne pas boucler à l'infini
            USART2->CR1 &= ~USART_CR1_TXEIE;
        }
    }
}