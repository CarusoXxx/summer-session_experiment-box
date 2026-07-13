#include "stm32f4xx.h"

/* ========== 变量定义区 ========== */
u16 i = 0;
u8 OD_Flag = 0;            // 收到回车符标志
u8 Rx_Frame_Flag = 0;      // 收到完整一帧标志
u8 Rx_Buf[64];             // 接收缓存
u16 Rx_Con;                // 接收计数器
u16 Rx_Len;                // 接收长度
u8 Error = 0;              // 错误指令标志

/* LED 状态记录（0=灭, 1=亮） */
u8 LED1_State = 0;
u8 LED2_State = 0;
u8 LED3_State = 0;

/* ========== 结构体变量定义 ========== */
GPIO_InitTypeDef   GPIO_InitStructure;
USART_InitTypeDef  USART_InitStructure;
NVIC_InitTypeDef   NVIC_InitStructure;

/* ========== 函数声明 ========== */
void USART2_Send_Frame(u8* data, u16 len);
void delay_us(uint16_t nus);
void delay_ms(u16 nms);

/* ========== 主函数 ========== */
int main(void)
{
    /* ---- 1. 配置三个 LED（PB5、PB0、PB1）---- */
    RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOB, ENABLE);

    GPIO_InitStructure.GPIO_Pin   = GPIO_Pin_5 | GPIO_Pin_0 | GPIO_Pin_1;
    GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_OUT;
    GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_25MHz;
    GPIO_InitStructure.GPIO_PuPd  = GPIO_PuPd_NOPULL;
    /* 初始化前先拉高（LED 低电平点亮假设，拉高=灭） */
    GPIO_SetBits(GPIOB, GPIO_Pin_5 | GPIO_Pin_0 | GPIO_Pin_1);
    GPIO_Init(GPIOB, &GPIO_InitStructure);

    /* ---- 2. 配置蜂鸣器（PA8）---- */
    RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOA, ENABLE);

    GPIO_InitStructure.GPIO_Pin   = GPIO_Pin_8;
    GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_OUT;
    GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_25MHz;
    GPIO_InitStructure.GPIO_PuPd  = GPIO_PuPd_NOPULL;
    GPIO_SetBits(GPIOA, GPIO_Pin_8);   // 高电平蜂鸣器不响
    GPIO_Init(GPIOA, &GPIO_InitStructure);

    /* ---- 3. 配置 USART2（PA2=TX, PA3=RX, 115200）---- */
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_USART2, ENABLE);

    GPIO_InitStructure.GPIO_Pin   = GPIO_Pin_2 | GPIO_Pin_3;
    GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_AF;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
    GPIO_InitStructure.GPIO_PuPd  = GPIO_PuPd_UP;
    GPIO_Init(GPIOA, &GPIO_InitStructure);

    GPIO_PinAFConfig(GPIOA, GPIO_PinSource2, GPIO_AF_USART2);
    GPIO_PinAFConfig(GPIOA, GPIO_PinSource3, GPIO_AF_USART2);

    USART_InitStructure.USART_BaudRate            = 115200;
    USART_InitStructure.USART_WordLength          = USART_WordLength_8b;
    USART_InitStructure.USART_StopBits            = USART_StopBits_1;
    USART_InitStructure.USART_Parity              = USART_Parity_No;
    USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
    USART_InitStructure.USART_Mode                = USART_Mode_Rx | USART_Mode_Tx;
    USART_Init(USART2, &USART_InitStructure);

    USART_Cmd(USART2, ENABLE);
    USART_ITConfig(USART2, USART_IT_RXNE, ENABLE);

    NVIC_PriorityGroupConfig(NVIC_PriorityGroup_0);
    NVIC_InitStructure.NVIC_IRQChannel                   = USART2_IRQn;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 1;
    NVIC_InitStructure.NVIC_IRQChannelSubPriority        = 3;
    NVIC_InitStructure.NVIC_IRQChannelCmd                = ENABLE;
    NVIC_Init(&NVIC_InitStructure);

    /* 启动提示 */
    USART2_Send_Frame("LED Control Ready!\r\n", 19);
    USART2_Send_Frame("Commands: ON1 ON2 ON3 | OF1 OF2 OF3 | ALL CLR | BEEP\r\n", 55);

    /* ========== 主循环：命令解析 ========== */
    while(1)
    {
        if(Rx_Frame_Flag)
        {
            Rx_Frame_Flag = 0;  // 清除标志
            Error = 0;

            /* ---- 3 字节命令 ---- */
            if(3 == Rx_Len)
            {
                /* --- ON1: 点亮 LED1 (PB5) --- */
                if(('O' == Rx_Buf[0]) && ('N' == Rx_Buf[1]) && ('1' == Rx_Buf[2]))
                {
                    GPIO_ResetBits(GPIOB, GPIO_Pin_5);
                    LED1_State = 1;
                    USART2_Send_Frame("LED1 ON\r\n", 8);
                }
                /* --- ON2: 点亮 LED2 (PB0) --- */
                else if(('O' == Rx_Buf[0]) && ('N' == Rx_Buf[1]) && ('2' == Rx_Buf[2]))
                {
                    GPIO_ResetBits(GPIOB, GPIO_Pin_0);
                    LED2_State = 1;
                    USART2_Send_Frame("LED2 ON\r\n", 8);
                }
                /* --- ON3: 点亮 LED3 (PB1) --- */
                else if(('O' == Rx_Buf[0]) && ('N' == Rx_Buf[1]) && ('3' == Rx_Buf[2]))
                {
                    GPIO_ResetBits(GPIOB, GPIO_Pin_1);
                    LED3_State = 1;
                    USART2_Send_Frame("LED3 ON\r\n", 8);
                }
                /* --- OF1: 关闭 LED1 --- */
                else if(('O' == Rx_Buf[0]) && ('F' == Rx_Buf[1]) && ('1' == Rx_Buf[2]))
                {
                    GPIO_SetBits(GPIOB, GPIO_Pin_5);
                    LED1_State = 0;
                    USART2_Send_Frame("LED1 OFF\r\n", 9);
                }
                /* --- OF2: 关闭 LED2 --- */
                else if(('O' == Rx_Buf[0]) && ('F' == Rx_Buf[1]) && ('2' == Rx_Buf[2]))
                {
                    GPIO_SetBits(GPIOB, GPIO_Pin_0);
                    LED2_State = 0;
                    USART2_Send_Frame("LED2 OFF\r\n", 9);
                }
                /* --- OF3: 关闭 LED3 --- */
                else if(('O' == Rx_Buf[0]) && ('F' == Rx_Buf[1]) && ('3' == Rx_Buf[2]))
                {
                    GPIO_SetBits(GPIOB, GPIO_Pin_1);
                    LED3_State = 0;
                    USART2_Send_Frame("LED3 OFF\r\n", 9);
                }
                /* --- ALL: 全亮 --- */
                else if(('A' == Rx_Buf[0]) && ('L' == Rx_Buf[1]) && ('L' == Rx_Buf[2]))
                {
                    GPIO_ResetBits(GPIOB, GPIO_Pin_5 | GPIO_Pin_0 | GPIO_Pin_1);
                    LED1_State = LED2_State = LED3_State = 1;
                    USART2_Send_Frame("ALL ON\r\n", 7);
                }
                /* --- CLR: 全灭 --- */
                else if(('C' == Rx_Buf[0]) && ('L' == Rx_Buf[1]) && ('R' == Rx_Buf[2]))
                {
                    GPIO_SetBits(GPIOB, GPIO_Pin_5 | GPIO_Pin_0 | GPIO_Pin_1);
                    LED1_State = LED2_State = LED3_State = 0;
                    USART2_Send_Frame("ALL OFF\r\n", 8);
                }
                else
                {
                    Error = 1;
                }
            }
            /* ---- 4 字节命令：BEEP ---- */
            else if(4 == Rx_Len)
            {
                if(('B' == Rx_Buf[0]) && ('E' == Rx_Buf[1]) &&
                   ('E' == Rx_Buf[2]) && ('P' == Rx_Buf[3]))
                {
                    GPIO_ResetBits(GPIOA, GPIO_Pin_8);   // 蜂鸣器响
                    delay_ms(50);
                    GPIO_SetBits(GPIOA, GPIO_Pin_8);     // 蜂鸣器停
                    USART2_Send_Frame("BEEP!\r\n", 6);
                }
                else
                {
                    Error = 1;
                }
            }
            else
            {
                Error = 1;
            }

            if(Error)
            {
                Error = 0;
                USART2_Send_Frame("Invalid Command!\r\n", 18);
            }
        }
    }
}

/* ========== USART2 帧发送函数 ========== */
void USART2_Send_Frame(u8* data, u16 len)
{
    u16 i;
    for(i = 0; i < len; i++)
    {
        while(USART_GetFlagStatus(USART2, USART_FLAG_TC) == RESET);
        USART_SendData(USART2, data[i]);
    }
}

/* ========== 延时函数 ========== */
void delay_us(uint16_t nus)
{
    uint16_t i;
    while(nus--)
    {
        i = 31;
        while(i--);
    }
}

void delay_ms(u16 nms)
{
    uint16_t i;
    while(nms--)
    {
        i = 33800;
        while(i--);
    }
}

/* ========== USART2 中断服务函数 ========== */
void USART2_IRQHandler(void)
{
    u8 res = 0;

    if(USART_GetFlagStatus(USART2, USART_FLAG_RXNE))
    {
        res = USART_ReceiveData(USART2);

        if(OD_Flag)                         // 已收到 0x0D(\r)
        {
            if(res == 0x0A)                 // 又收到 0x0A(\n)，一帧结束
            {
                Rx_Frame_Flag = 1;
                Rx_Len        = Rx_Con;
                Rx_Con        = 0;
                OD_Flag       = 0;
            }
        }
        else                                // 还没收到 \r
        {
            if(res == 0x0D)
            {
                OD_Flag = 1;                // 标记收到回车
            }
            else
            {
                if(Rx_Con < sizeof(Rx_Buf)) // 防溢出
                {
                    Rx_Buf[Rx_Con] = res;
                    Rx_Con++;
                }
            }
        }
    }
}
