#include <stdio.h>
#include "usart.h"

void UART2_Init(u32 bound)
{
    GPIO_InitTypeDef GPIO_InitStructure;
    USART_InitTypeDef USART_InitStructure;
    NVIC_InitTypeDef NVIC_InitStructure;

    //使能GPIOA和USART2时钟
    RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOA,ENABLE);
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_USART2,ENABLE);

    //USART2 对应引脚复用映射
    GPIO_PinAFConfig(GPIOA, GPIO_PinSource2, GPIO_AF_USART2);
    GPIO_PinAFConfig(GPIOA, GPIO_PinSource3, GPIO_AF_USART2);

    //USART2 IO配置
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_2 | GPIO_Pin_3;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
    GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;
    GPIO_Init(GPIOA,&GPIO_InitStructure);

    //USART2 端口配置
    USART_InitStructure.USART_BaudRate = bound;
    USART_InitStructure.USART_WordLength = USART_WordLength_8b;
    USART_InitStructure.USART_StopBits = USART_StopBits_1;
    USART_InitStructure.USART_Parity = USART_Parity_No;
    USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
    USART_InitStructure.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;
    USART_Init(USART2, &USART_InitStructure);

    NVIC_InitStructure.NVIC_IRQChannel = USART2_IRQn;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 3;
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 3;
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init( &NVIC_InitStructure );

    USART_ITConfig(USART2, USART_IT_RXNE, ENABLE);
    USART_Cmd(USART2, ENABLE);
}

/*
* 发送一个字符
*/
void UART_SendByte( USART_TypeDef * pUSARTx, uint8_t ch)
{
    USART_SendData(pUSARTx, ch);
    while (USART_GetFlagStatus(pUSARTx, USART_FLAG_TXE) == RESET);
}

/*
* 发送字符串
*/
void UART_SendString( USART_TypeDef * pUSARTx, char *str)
{
    unsigned int k=0;
    do {
        UART_SendByte( pUSARTx, *(str + k) );
        k++;
    } while(*(str + k)!='\0');

    while(USART_GetFlagStatus(pUSARTx,USART_FLAG_TC)==RESET){}
}

/*
* 发送一个16位数，高字节在前
*/
void UART_SendHalfWord( USART_TypeDef * pUSARTx, uint16_t ch)
{
    uint8_t temp_h, temp_l;
    temp_h = (ch&0XFF00)>>8;
    temp_l = ch&0XFF;

    USART_SendData(pUSARTx,temp_h);
    while (USART_GetFlagStatus(pUSARTx, USART_FLAG_TXE) == RESET);

    USART_SendData(pUSARTx,temp_l);
    while (USART_GetFlagStatus(pUSARTx, USART_FLAG_TXE) == RESET);
}

//重定向c库函数printf到串口，重定向后可使用printf函数
int fputc(int ch, FILE *f)
{
    USART_SendData(USART2, (uint8_t) ch);
    while (USART_GetFlagStatus(USART2, USART_FLAG_TXE) == RESET);
    return (ch);
}

//重定向c库函数scanf到串口，重写后可使用scanf、getchar等函数
int fgetc(FILE *f){
    while (USART_GetFlagStatus(USART2, USART_FLAG_RXNE) == RESET);
    return (int)USART_ReceiveData(USART2);
}

/*
* 串口2中断处理函数
*/
void USART2_IRQHandler( void ){
    u8 sbuf = 0;

    if(USART_GetFlagStatus(USART2, USART_FLAG_RXNE)) {
        sbuf = USART_ReceiveData(USART2);
        USART_SendData(USART2, sbuf);           //回显转发
    }
}
