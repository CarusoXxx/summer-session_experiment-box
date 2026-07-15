#ifndef __NFC_PN532_H__
#define __NFC_PN532_H__

#include "stm32f4xx.h"

extern uint8_t UID[4];
extern uint8_t PN532_RxBuf[];
extern uint16_t PN532_RxBufLen;

void NFC_Init( uint32_t baud );
int8_t NFC_WakeUp(void);
int8_t NFC_Read( uint8_t block , uint8_t *buf);
int8_t NFC_Write(uint8_t block , uint8_t *buf);
int8_t PN532_InListPassiveTarget(void);

#endif /* __NFC_PN532_H__ */