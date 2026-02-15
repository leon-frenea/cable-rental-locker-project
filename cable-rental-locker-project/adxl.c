#include "adxl.h"

static uint8_t SPI_Transfer(uint8_t data) {
    while (!(SPI1->SR & SPI_SR_TXE)); 
    SPI1->DR = data;
    while (!(SPI1->SR & SPI_SR_RXNE)); 
    return SPI1->DR;
}

static void ADXL_WriteReg(uint8_t reg, uint8_t val) {
    CS_LOW();
    SPI_Transfer(reg);
    SPI_Transfer(val);
    CS_HIGH();
}

void ADXL_Init(void) {
    // GPIO SPI1 + CS (PA4)
    RCC->APB2ENR |= RCC_APB2ENR_SPI1EN | RCC_APB2ENR_IOPAEN | RCC_APB2ENR_AFIOEN;
    
    // SCK, MISO, MOSI (PA5, 6, 7) - Alt Push Pull
    GPIOA->CRL &= ~(0xFFF00000U); 
    GPIOA->CRL |=  (0xB8B00000U); // PA5=B (AltPP), PA6=8 (InPull), PA7=B (AltPP)
    
    // CS (PA4) - Out Push Pull
    GPIOA->CRL &= ~(0x000F0000U);
    GPIOA->CRL |=  (0x00030000U); 
    CS_HIGH(); 
    
    // SPI Config
    SPI1->CR1 = SPI_CR1_MSTR | SPI_CR1_CPOL | SPI_CR1_CPHA | SPI_CR1_SSM | SPI_CR1_SSI | (0x5U << 3); 
    SPI1->CR1 |= SPI_CR1_SPE; 

    // Config ADXL
    ADXL_WriteReg(ADXL345_POWER_CTL, 0x00); 
    ADXL_WriteReg(ADXL345_DATA_FORMAT, 0x0B); // Full resolution, +/- 16g
    ADXL_WriteReg(ADXL345_POWER_CTL, 0x08);   // Measure Mode
}

void ADXL_Get_XYZ(int16_t *x, int16_t *y, int16_t *z) {
    uint8_t buffer[6];
    CS_LOW();
    SPI_Transfer(ADXL345_DATAX0 | 0x80 | 0x40); // Read | Multi-byte
    for(int i=0; i<6; i++) buffer[i] = SPI_Transfer(0xFF);
    CS_HIGH();
    *x = (int16_t)((buffer[1] << 8) | buffer[0]);
    *y = (int16_t)((buffer[3] << 8) | buffer[2]);
    *z = (int16_t)((buffer[5] << 8) | buffer[4]);
}

uint8_t ADXL_Detect_Theft(void) {
    int16_t x, y, z;
    ADXL_Get_XYZ(&x, &y, &z);
    // Seuil simple (à affiner avec valeur absolue)
    if (x > 100 || x < -100) return 1;
    return 0;
}