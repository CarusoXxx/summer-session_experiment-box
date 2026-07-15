/**
* v9.1 — 指纹编号从1开始 · 每次烧录重置 · Exit全页面生效 · 按键删指纹 · RTC可设置
*/
#include <stdio.h>
#include <string.h>
#include "stm32f4xx.h"
#include "fpr_zn632.h"
#include "lcd_ili9341.h"
#include "lcd_fonts.h"
#include "dht11.h"
#include "mq.h"
#include "flash_w25qxx.h"
#include "delay.h"
#include "usart.h"
#include "xpt2046.h"
#include "oled.h"
#include "ir_recv.h"

#define LED_R_ON()   GPIO_ResetBits(GPIOB,GPIO_Pin_5)
#define LED_R_OFF()  GPIO_SetBits(GPIOB,GPIO_Pin_5)
#define LED_G_ON()   GPIO_ResetBits(GPIOB,GPIO_Pin_0)
#define LED_G_OFF()  GPIO_SetBits(GPIOB,GPIO_Pin_0)
#define LED_Y_ON()   GPIO_ResetBits(GPIOB,GPIO_Pin_1)
#define LED_Y_OFF()  GPIO_SetBits(GPIOB,GPIO_Pin_1)
#define BZ_ON()      GPIO_ResetBits(GPIOA,GPIO_Pin_8)
#define BZ_OFF()     GPIO_SetBits(GPIOA,GPIO_Pin_8)
#define K1 (GPIO_ReadInputDataBit(GPIOA,GPIO_Pin_0)==0)
#define K2 (GPIO_ReadInputDataBit(GPIOC,GPIO_Pin_13)==0)
#define K3 (GPIO_ReadInputDataBit(GPIOC,GPIO_Pin_6)==0)
#define K4 (GPIO_ReadInputDataBit(GPIOC,GPIO_Pin_7)==0)
extern uint8_t ZN632_INDEX[];
#define PW_MAX 8
#define ALARM  1500
#define LOG_SECTOR 0
#define LOG_MAX 256

typedef struct{uint8_t type;uint8_t pad;uint16_t seq;}LogRec;

static char g_pw[PW_MAX+1]="1234";
static uint16_t g_door=0,g_enroll=0,g_fail=0,g_seq=0,g_log_n=0;
static uint8_t g_consec=0,g_lock=0;

void LED_Init(void){GPIO_InitTypeDef s;RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOB,ENABLE);s.GPIO_Pin=GPIO_Pin_5|GPIO_Pin_0|GPIO_Pin_1;s.GPIO_Mode=GPIO_Mode_OUT;s.GPIO_OType=GPIO_OType_PP;s.GPIO_Speed=GPIO_Speed_2MHz;s.GPIO_PuPd=GPIO_PuPd_NOPULL;GPIO_Init(GPIOB,&s);LED_R_OFF();LED_G_OFF();LED_Y_OFF();}
void BZ_Init(void){GPIO_InitTypeDef s;RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOA,ENABLE);s.GPIO_Pin=GPIO_Pin_8;s.GPIO_Mode=GPIO_Mode_OUT;s.GPIO_OType=GPIO_OType_PP;s.GPIO_Speed=GPIO_Speed_2MHz;s.GPIO_PuPd=GPIO_PuPd_NOPULL;GPIO_Init(GPIOA,&s);BZ_OFF();}
void BEEP(uint16_t ms){BZ_ON();delay_ms(ms);BZ_OFF();}
void BZ_Alarm(void){BZ_ON();delay_ms(ALARM);BZ_OFF();}
void K_Init(void){GPIO_InitTypeDef s;RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOA|RCC_AHB1Periph_GPIOC,ENABLE);s.GPIO_Pin=GPIO_Pin_0;s.GPIO_Mode=GPIO_Mode_IN;s.GPIO_PuPd=GPIO_PuPd_UP;GPIO_Init(GPIOA,&s);s.GPIO_Pin=GPIO_Pin_13|GPIO_Pin_6|GPIO_Pin_7;s.GPIO_Mode=GPIO_Mode_IN;s.GPIO_PuPd=GPIO_PuPd_UP;GPIO_Init(GPIOC,&s);}

#define IR_AUTH_ADDR 0x00
#define IR_KEY_OPEN  0x45
void IR_Rece_Proc(uint16_t addr,uint8_t code){
    printf("[IR] addr=%04X code=%02X\r\n",addr,code);
    if(addr==IR_AUTH_ADDR&&code==IR_KEY_OPEN){printf("[IR] Open Door!\r\n");LED_G_ON();BEEP(100);delay_ms(50);BEEP(100);g_door++;Log_Save(1);LED_G_OFF();}
}

void Log_Init(void){W25QXX_Init();W25QXX_BufferRead((uint8_t*)&g_log_n,LOG_SECTOR*4096,2);if(g_log_n>LOG_MAX)g_log_n=0;W25QXX_BufferRead((uint8_t*)&g_seq,LOG_SECTOR*4096+2,2);}
void Log_Save(uint8_t t){LogRec r;r.type=t;r.pad=0;r.seq=++g_seq;if(g_log_n>=LOG_MAX)return;uint32_t addr=LOG_SECTOR*4096+4+g_log_n*4;if(g_log_n==0)W25QXX_SectorErase(LOG_SECTOR*4096);W25QXX_BufferWrite((uint8_t*)&r,addr,4);g_log_n++;W25QXX_BufferWrite((uint8_t*)&g_log_n,LOG_SECTOR*4096,2);W25QXX_BufferWrite((uint8_t*)&g_seq,LOG_SECTOR*4096+2,2);}
void Log_Read(LogRec*buf,uint16_t max,uint16_t*out_n){uint16_t n=g_log_n<max?g_log_n:max;if(n==0){*out_n=0;return;}W25QXX_BufferRead((uint8_t*)buf,LOG_SECTOR*4096+4,n*4);*out_n=n;}

void LBlue(void){LCD_SetTextColor(0xFFFF);LCD_SetBackColor(0x001F);}

typedef struct{uint16_t x1,y1,x2,y2;}R;
static R bt[4],ex;
static void BtInit(void){bt[0]=(R){20,55,220,95};bt[1]=(R){20,110,220,150};bt[2]=(R){20,165,220,205};bt[3]=(R){20,220,220,260};ex=(R){60,275,180,305};}
static int8_t BH(uint16_t x,uint16_t y){for(int i=0;i<4;i++)if(x>=bt[i].x1&&x<=bt[i].x2&&y>=bt[i].y1&&y<=bt[i].y2)return i;return-1;}
static int8_t ExH(uint16_t x,uint16_t y){return x>=ex.x1&&x<=ex.x2&&y>=ex.y1&&y<=ex.y2;}

static void OLED_Update(void);
static uint8_t  g_ol_pg=0;
static uint32_t g_ol_tk=0,g_lk_tk=0;

static int8_t TWait(void){XPT2046_Coord c;while(1){g_ol_tk++;g_lk_tk++;if(g_ol_tk>=300){g_ol_tk=0;g_ol_pg=!g_ol_pg;OLED_Update();}if(g_lk_tk>=100){g_lk_tk=0;if(g_lock>0)g_lock--;}if(XPT2046_GetTouchPoint(&c)==0){delay_ms(30);while(XPT2046_GetTouchPoint(&c)==0);delay_ms(20);if(ExH(c.x,c.y))return-2;int8_t b=BH(c.x,c.y);if(b>=0)return b;}delay_ms(10);}}

static void BDraw(uint8_t n,const char*t){R*r=&bt[n];LCD_SetBackColor(0x07E0);LCD_Clear(r->x1,r->y1,r->x2-r->x1,r->y2-r->y1);LCD_SetTextColor(0x0000);LCD_SetBackColor(0x07E0);uint8_t cx=r->x1+(r->x2-r->x1-strlen(t)*8)/2;LCD_DispStringEN(cx,r->y1+(r->y2-r->y1-14)/2,0,(char*)t);}
static void ExDraw(void){LCD_SetBackColor(0x07E0);LCD_Clear(ex.x1,ex.y1,ex.x2-ex.x1,ex.y2-ex.y1);LCD_SetTextColor(0x0000);LCD_SetBackColor(0x07E0);LCD_DispStringEN(ex.x1+28,ex.y1+10,0,"[ Exit ]");}

/* OLED 轮播 */
static void OLED_Update(void){
    char b[40];
    if(g_ol_pg==0){RTC_TimeTypeDef ts;RTC_DateTypeDef ds;RTC_GetTime(RTC_Format_BIN,&ts);RTC_GetDate(RTC_Format_BIN,&ds);OLED_Clear();sprintf(b,"%02d:%02d:%02d",ts.RTC_Hours,ts.RTC_Minutes,ts.RTC_Seconds);OLED_ShowString(0,0,(uint8_t*)b,16);sprintf(b,"20%02d-%02d-%02d",ds.RTC_Year,ds.RTC_Month,ds.RTC_Date);OLED_ShowString(0,2,(uint8_t*)b,16);}
    else{DHT11_Data dht;OLED_Clear();if(DHT11_ReadData(&dht)==0){sprintf(b,"T:%d.%dC H:%d.%d%%",dht.temp_int,dht.temp_deci,dht.humi_int,dht.humi_deci);OLED_ShowString(0,0,(uint8_t*)b,16);}else{OLED_ShowString(0,0,(uint8_t*)"DHT11:Error",16);}uint16_t mq=MQ_ReadValue();sprintf(b,"MQ2:%d %s",mq,mq>3000?"WARN!":"Safe");OLED_ShowString(0,2,(uint8_t*)b,16);if(mq>3000){LED_R_ON();BEEP(50);LED_R_OFF();}}
}

/* ═══ 页1:主菜单 ═══ */
static void P1(void){char b[40];BtInit();LBlue();LCD_Clear(0,0,240,320);LCD_DispStringEN(5,2,0,"== DOOR ACCESS ==");sprintf(b,"Open:%d Enr:%d Fail:%d",g_door,g_enroll,g_fail);LCD_DispStringEN(5,25,0,b);BDraw(0,"1. Open Door");BDraw(1,"2. Environment");BDraw(2,"3. Settings");BDraw(3,"4. Log");}

/* ═══ 页2:开门验证 ═══ */
static void P2(void){BtInit();LBlue();LCD_Clear(0,0,240,320);LCD_DispStringEN(5,2,0,"== OPEN DOOR ==");BDraw(0,"Fingerprint");BDraw(1,"Password");ExDraw();}
static void P2_Locked(void){char b[24];
    while(g_lock>0){
        LBlue();LCD_Clear(0,0,240,320);LCD_DispStringEN(10,60,0,"System Locked!");
        sprintf(b,"Wait %ds...",g_lock);LCD_DispStringEN(10,100,0,b);ExDraw();
        uint16_t _wt=0;
        while(_wt<50&&g_lock>0){
            g_ol_tk++;g_lk_tk++;_wt++;
            if(g_ol_tk>=300){g_ol_tk=0;g_ol_pg=!g_ol_pg;OLED_Update();}
            if(g_lk_tk>=100){g_lk_tk=0;if(g_lock>0)g_lock--;}
            XPT2046_Coord c;if(XPT2046_GetTouchPoint(&c)==0){delay_ms(30);while(XPT2046_GetTouchPoint(&c)==0);delay_ms(20);if(ExH(c.x,c.y)){P1();return;}}
            if(K4){delay_ms(20);if(K4){while(K4);delay_ms(20);P1();return;}}
            delay_ms(10);
        }
    }
    P2();}
static void P2_Finger(void){LBlue();LCD_Clear(0,0,240,320);LCD_DispStringEN(10,80,0,"Place finger...");delay_ms(500);int16_t r=FPR_MatchFinger();LED_R_OFF();LED_G_OFF();LBlue();LCD_Clear(0,0,240,320);
    if(r==1){LED_G_ON();LCD_DispStringEN(30,80,0,"OK! Door Opened");BEEP(100);delay_ms(50);BEEP(100);g_door++;g_consec=0;Log_Save(1);}
    else{LED_R_ON();LCD_DispStringEN(25,80,0,"Finger Fail!");BEEP(50);g_fail++;g_consec++;if(g_consec>=3){g_lock=60;g_consec=0;}Log_Save(0);}
    ExDraw();LCD_DispStringEN(5,110,0,"Press any key to back");
    while(1){if(K1||K2||K3||K4){delay_ms(20);while(K1||K2||K3||K4);delay_ms(20);P2();return;}
        XPT2046_Coord c;if(XPT2046_GetTouchPoint(&c)==0){while(XPT2046_GetTouchPoint(&c)==0);delay_ms(20);P2();return;}}}
static void P2_Pwd(void){char in[PW_MAX+1];
    LBlue();LCD_Clear(0,0,240,320);LCD_DispStringEN(5,5,0,"== Password ==");LCD_DispStringEN(5,30,0,"1=+1 2=Del 3=OK 4=Done");LCD_DispStringEN(5,55,0,"PWD:");ExDraw();
    char d[PW_MAX]={0};int8_t p=0;uint8_t c=0;char pl[PW_MAX+2]="\0";
    while(1){char l[PW_MAX+2];uint8_t i;for(i=0;i<PW_MAX;i++)l[i]=(i<p)?'*':(i==p)?('0'+c):'_';l[PW_MAX]=0;
        if(strcmp(l,pl)){strcpy(pl,l);LCD_SetTextColor(0xFFFF);LCD_SetBackColor(0x001F);LCD_Clear(5,75,230,95);LCD_DispStringEN(5,80,0,l);}
        if(K1){delay_ms(15);if(K1){while(K1);delay_ms(15);c=(c+1)%10;}}
        else if(K2){delay_ms(15);if(K2){while(K2);delay_ms(15);if(p>0){p--;c=d[p];}}}
        else if(K3){delay_ms(15);if(K3){while(K3);delay_ms(15);d[p]=c;p++;if(p>=PW_MAX)goto F;c=0;}}
        else if(K4){delay_ms(15);if(K4){while(K4);delay_ms(15);goto F;}}
        else{XPT2046_Coord co;if(XPT2046_GetTouchPoint(&co)==0){delay_ms(30);if(XPT2046_GetTouchPoint(&co)==0){while(XPT2046_GetTouchPoint(&co)==0);delay_ms(20);if(ExH(co.x,co.y)){P2();return;}if(co.y<120)c=(c+1)%10;else if(p>0){p--;c=d[p];}}}}}
    F:if(p==0){P2();return;}
    {uint8_t i;for(i=0;i<p;i++)in[i]='0'+d[i];}in[p]=0;LED_R_OFF();LED_G_OFF();LBlue();LCD_Clear(0,0,240,320);
    if(strcmp(in,g_pw)==0){LED_G_ON();LCD_DispStringEN(30,80,0,"OK! Door Opened");BEEP(100);delay_ms(50);BEEP(100);g_door++;g_consec=0;Log_Save(1);}
    else{LED_R_ON();LCD_DispStringEN(25,80,0,"Wrong Password!");BEEP(50);g_fail++;g_consec++;if(g_consec>=3){g_lock=60;g_consec=0;}Log_Save(0);}
    ExDraw();LCD_DispStringEN(5,110,0,"Press any key to back");
    while(1){if(K1||K2||K3||K4){delay_ms(20);while(K1||K2||K3||K4);delay_ms(20);P2();return;}
        XPT2046_Coord c;if(XPT2046_GetTouchPoint(&c)==0){while(XPT2046_GetTouchPoint(&c)==0);delay_ms(20);P2();return;}}}

/* ═══ 页3:环境监测 ═══ */
static void P3(void){char b[40];LBlue();LCD_Clear(0,0,240,320);LCD_DispStringEN(5,2,0,"== ENVIRONMENT ==");
    DHT11_Data dht;LCD_DispStringEN(10,50,0,"DHT11: reading...");
    if(DHT11_ReadData(&dht)==0){sprintf(b,"Temp : %d.%d C",dht.temp_int,dht.temp_deci);LCD_DispStringEN(10,50,0,b);sprintf(b,"Humi : %d.%d %%",dht.humi_int,dht.humi_deci);LCD_DispStringEN(10,85,0,b);}
    else{LCD_SetTextColor(0xF800);LCD_SetBackColor(0x001F);LCD_DispStringEN(10,50,0,"DHT11: Read Error!");}
    uint16_t mq=MQ_ReadValue();sprintf(b,"MQ2  : %d",mq);LCD_DispStringEN(10,120,0,b);
    if(mq>3000){LCD_SetTextColor(0xF800);LCD_SetBackColor(0x001F);LCD_DispStringEN(10,155,0,"GAS WARNING!");LED_R_ON();BEEP(50);LED_R_OFF();}
    else{LCD_SetTextColor(0x07E0);LCD_SetBackColor(0x001F);LCD_DispStringEN(10,155,0,"Gas: Safe");}
    ExDraw();}

/* ═══ 页4:系统设置 — 按键导航 ═══ */
static void P4_Enroll(void);static void P4_Delete(void);static void P4_Pwd(void);static void P4_Time(void);
static void P4(void){
    int8_t cur=0,prev=-1; const char* it[]={"Enroll Finger","FPR Delete","Set Password","Set Time"};
    uint8_t i;
    LBlue();LCD_Clear(0,0,240,320);LCD_DispStringEN(5,2,0,"== SETTINGS ==");
    LCD_DispStringEN(5,180,0,"K1:Up K2:Dn K3:OK K4:Exit");
    ExDraw();
    for(;;){
        if(cur!=prev){prev=cur;
            for(i=0;i<4;i++){ uint8_t y=40+i*28;
                if(cur==(int8_t)i){LCD_SetTextColor(0x0000);LCD_SetBackColor(0xF800);}
                else{LCD_SetTextColor(0xFFFF);LCD_SetBackColor(0x001F);}
                LCD_DispStringEN(10,y,0,(char*)it[i]);
            }
        }
        if(K1){delay_ms(20);if(K1){while(K1);delay_ms(20);cur=(cur+3)%4;continue;}}
        if(K2){delay_ms(20);if(K2){while(K2);delay_ms(20);cur=(cur+1)%4;continue;}}
        if(K3){delay_ms(20);if(K3){while(K3);delay_ms(20);
            if(cur==0)P4_Enroll();else if(cur==1)P4_Delete();else if(cur==2)P4_Pwd();else P4_Time();
            LBlue();LCD_Clear(0,0,240,320);LCD_DispStringEN(5,2,0,"== SETTINGS ==");
            LCD_DispStringEN(5,180,0,"K1:Up K2:Dn K3:OK K4:Exit");ExDraw();prev=-1;continue;
        }}
        if(K4){delay_ms(20);if(K4){while(K4);delay_ms(20);return;}}
        {XPT2046_Coord c;if(XPT2046_GetTouchPoint(&c)==0){delay_ms(30);if(XPT2046_GetTouchPoint(&c)==0){while(XPT2046_GetTouchPoint(&c)==0);delay_ms(20);if(ExH(c.x,c.y)){return;}}}}
        delay_ms(10);
    }
}

static void P4_Enroll(void){uint16_t old=g_enroll;
    LBlue();LCD_Clear(0,0,240,320);LCD_DispStringEN(10,80,0,"Place finger...");delay_ms(500);
    FPR_AddFinger();ZN632_ReadIndexTable();g_enroll=0;
    for(uint16_t _i=0;_i<240;_i++){if(ZN632_INDEX[_i/8]&(1<<(_i%8)))g_enroll++;}
    LBlue();LCD_Clear(0,0,240,320);LCD_DispStringEN(15,80,0,g_enroll>old?"Enroll OK!":"Enroll Fail!");BEEP(100);
    LCD_DispStringEN(5,130,0,"Press any key to back");
    while(1){if(K1||K2||K3||K4){delay_ms(20);while(K1||K2||K3||K4);delay_ms(20);P4();return;}
        XPT2046_Coord c;if(XPT2046_GetTouchPoint(&c)==0){while(XPT2046_GetTouchPoint(&c)==0);delay_ms(20);P4();return;}}}

static void P4_Delete(void){
retry:
    {uint16_t fids[240];uint8_t fn=0;static int8_t sel=-1;char b[40];
    if(ZN632_ReadIndexTable()!=0){P4();return;}
    for(uint16_t i=0;i<240;i++){if(ZN632_INDEX[i/8]&(1<<(i%8))){fids[fn++]=i;}}
    g_enroll=fn;
    if(sel>=(int8_t)fn)sel=fn-1;if(sel<-1)sel=-1;
    LBlue();LCD_Clear(0,0,240,320);LCD_DispStringEN(5,2,0,"== DELETE FPR ==");
    sprintf(b,"Enrolled: %d",fn);LCD_DispStringEN(10,22,0,b);
    LCD_DispStringEN(5,170,0,"K1/K3=Del K2=Sel  K4=Exit");
    if(fn==0){LCD_DispStringEN(10,80,0,"(No fingers)");}
    else{
        LCD_DispStringEN(10,40,0,"Select finger:");
        for(uint8_t j=0;j<fn&&j<12;j++){
            sprintf(b,"#%02d",fids[j]+1);
            if(sel==(int8_t)j){LCD_SetTextColor(0x0000);LCD_SetBackColor(0xF800);}
            else{LCD_SetTextColor(0x07E0);LCD_SetBackColor(0x001F);}
            LCD_DispStringEN(5+(j%4)*58,58+(j/4)*22,0,b);
        }
    }
    while(1){
        if(K1){delay_ms(20);if(K1){while(K1);delay_ms(20);
            if(sel>=0&&sel<(int8_t)fn){ZN632_DeletChar(fids[sel],1);sel=-1;goto retry;}}}
        if(K2){delay_ms(20);if(K2){while(K2);delay_ms(20);if(fn>0){sel=(sel+1)%fn;goto retry;}}}
        if(K3){delay_ms(20);if(K3){while(K3);delay_ms(20);
            if(sel>=0&&sel<(int8_t)fn){ZN632_DeletChar(fids[sel],1);sel=-1;goto retry;}}}
        if(K4){delay_ms(20);if(K4){while(K4);delay_ms(20);P4();return;}}
        XPT2046_Coord c;if(XPT2046_GetTouchPoint(&c)==0){delay_ms(30);if(XPT2046_GetTouchPoint(&c)==0){while(XPT2046_GetTouchPoint(&c)==0);delay_ms(20);
            if(ExH(c.x,c.y)){P4();return;}
            uint8_t row=(c.y-58)/22,col=(c.x-5)/58;int8_t idx=row*4+col;
            if(idx>=0&&idx<(int8_t)fn){ZN632_DeletChar(fids[idx],1);sel=-1;goto retry;}
        }}}
}}

static void P4_Pwd(void){char n[PW_MAX+1];
    char d[PW_MAX]={0};int8_t p=0;uint8_t c=0;char pl[PW_MAX+2]="\0";
    LBlue();LCD_Clear(0,0,240,320);LCD_DispStringEN(5,5,0,"New Password:");LCD_DispStringEN(5,30,0,"1=+1 2=Del 3=OK 4=Done");ExDraw();
    while(1){char l[PW_MAX+2];uint8_t i;for(i=0;i<PW_MAX;i++)l[i]=(i<p)?'*':(i==p)?('0'+c):'_';l[PW_MAX]=0;
        if(strcmp(l,pl)){strcpy(pl,l);LCD_SetTextColor(0xFFFF);LCD_SetBackColor(0x001F);LCD_Clear(5,55,230,75);LCD_DispStringEN(5,60,0,l);}
        if(K1){delay_ms(15);if(K1){while(K1);delay_ms(15);c=(c+1)%10;}}
        else if(K2){delay_ms(15);if(K2){while(K2);delay_ms(15);if(p>0){p--;c=d[p];}}}
        else if(K3){delay_ms(15);if(K3){while(K3);delay_ms(15);d[p]=c;p++;if(p>=PW_MAX)goto G;c=0;}}
        else if(K4){delay_ms(15);if(K4){while(K4);delay_ms(15);goto G;}}
        else{XPT2046_Coord co;if(XPT2046_GetTouchPoint(&co)==0){delay_ms(30);if(XPT2046_GetTouchPoint(&co)==0){while(XPT2046_GetTouchPoint(&co)==0);delay_ms(20);if(ExH(co.x,co.y)){P4();return;}if(co.y<120)c=(c+1)%10;else if(p>0){p--;c=d[p];}}}}}
    G:if(p==0){P4();return;}
    {uint8_t i;for(i=0;i<p;i++)n[i]='0'+d[i];}n[p]=0;
    char c2[PW_MAX+1];d[0]=c=p=0;memset(d,0,PW_MAX);pl[0]=0;
    LBlue();LCD_Clear(0,0,240,320);LCD_DispStringEN(5,5,0,"Confirm:");LCD_DispStringEN(5,30,0,"1=+1 2=Del 3=OK 4=Done");ExDraw();
    while(1){char l[PW_MAX+2];uint8_t i;for(i=0;i<PW_MAX;i++)l[i]=(i<p)?'*':(i==p)?('0'+c):'_';l[PW_MAX]=0;
        if(strcmp(l,pl)){strcpy(pl,l);LCD_SetTextColor(0xFFFF);LCD_SetBackColor(0x001F);LCD_Clear(5,55,230,75);LCD_DispStringEN(5,60,0,l);}
        if(K1){delay_ms(15);if(K1){while(K1);delay_ms(15);c=(c+1)%10;}}
        else if(K2){delay_ms(15);if(K2){while(K2);delay_ms(15);if(p>0){p--;c=d[p];}}}
        else if(K3){delay_ms(15);if(K3){while(K3);delay_ms(15);d[p]=c;p++;if(p>=PW_MAX)goto H;c=0;}}
        else if(K4){delay_ms(15);if(K4){while(K4);delay_ms(15);goto H;}}
        else{XPT2046_Coord co;if(XPT2046_GetTouchPoint(&co)==0){delay_ms(30);if(XPT2046_GetTouchPoint(&co)==0){while(XPT2046_GetTouchPoint(&co)==0);delay_ms(20);if(ExH(co.x,co.y)){P4();return;}if(co.y<120)c=(c+1)%10;else if(p>0){p--;c=d[p];}}}}}
    H:if(p==0){P4();return;}
    {uint8_t i;for(i=0;i<p;i++)c2[i]='0'+d[i];}c2[p]=0;
    LBlue();LCD_Clear(0,0,240,320);
    if(strcmp(n,c2)==0){strcpy(g_pw,n);LCD_DispStringEN(10,80,0,"PWD Updated!");LED_R_ON();LED_G_ON();LED_Y_ON();delay_ms(2000);LED_R_OFF();LED_G_OFF();LED_Y_OFF();}
    else{LCD_DispStringEN(10,80,0,"Mismatch!");BEEP(50);delay_ms(1500);}
    ExDraw();LCD_DispStringEN(5,130,0,"Press any key to back");
    while(1){if(K1||K2||K3||K4){delay_ms(20);while(K1||K2||K3||K4);delay_ms(20);P4();return;}
        XPT2046_Coord c;if(XPT2046_GetTouchPoint(&c)==0){while(XPT2046_GetTouchPoint(&c)==0);delay_ms(20);P4();return;}}}

/* 时间设置 — K3换字段(锁定当前值) K4保存全部 */
static void P4_Time(void){
    RTC_TimeTypeDef _ts;RTC_DateTypeDef _ds;
    RTC_GetTime(RTC_Format_BIN,&_ts);RTC_GetDate(RTC_Format_BIN,&_ds);
    uint8_t yr=_ds.RTC_Year,mo=_ds.RTC_Month,da=_ds.RTC_Date,hr=_ts.RTC_Hours,mi=_ts.RTC_Minutes,field=0;
    char prev[40]="";
    LBlue();LCD_Clear(0,0,240,320);LCD_DispStringEN(5,2,0,"== SET TIME ==");
    LCD_SetTextColor(0xFFFF);LCD_SetBackColor(0x001F);
    LCD_DispStringEN(5,170,0,"K1:+1 K2:-1 K3:Next K4:Save");
    ExDraw();
    while(1){
        char b[40];
        sprintf(b,"20%02d-%02d-%02d %02d:%02d",yr,mo,da,hr,mi);
        if(strcmp(b,prev)){
            strcpy(prev,b);
            LCD_SetTextColor(0x07E0);LCD_SetBackColor(0x001F);LCD_Clear(5,40,230,65);
            LCD_DispStringEN(5,45,0,b);
            LCD_SetTextColor(0xFFFF);LCD_SetBackColor(0x001F);
            LCD_Clear(5,80,230,110);
            if(field==0)sprintf(b,"Editing: Year(20)");
            else if(field==1)sprintf(b,"Editing: Month");
            else if(field==2)sprintf(b,"Editing: Date");
            else if(field==3)sprintf(b,"Editing: Hour");
            else sprintf(b,"Editing: Minute");
            LCD_DispStringEN(5,85,0,b);
            sprintf(b,"Value: %02d",field==0?yr:field==1?mo:field==2?da:field==3?hr:mi);
            LCD_DispStringEN(5,100,0,b);
        }
        if(K1){delay_ms(15);if(K1){while(K1);delay_ms(15);
            if(field==0)yr=(yr+1)%100;else if(field==1)mo=mo%12+1;
            else if(field==2)da=da%31+1;else if(field==3)hr=(hr+1)%24;else mi=(mi+1)%60;}}
        else if(K2){delay_ms(15);if(K2){while(K2);delay_ms(15);
            if(field==0)yr=(yr+99)%100;else if(field==1)mo=(mo+10)%12+1;
            else if(field==2)da=(da+29)%31+1;else if(field==3)hr=(hr+23)%24;else mi=(mi+59)%60;}}
        else if(K3){delay_ms(15);if(K3){while(K3);delay_ms(15);field=(field+1)%5;*prev=0;}}
        else if(K4){delay_ms(15);if(K4){while(K4);delay_ms(15);
            RTC_Set_Date(yr,mo,da,1);RTC_Set_Time(hr,mi,0,RTC_H12_AM);
            OLED_Update();
            LBlue();LCD_Clear(0,0,240,320);LCD_DispStringEN(10,80,0,"Time Saved!");BEEP(100);delay_ms(1000);return;}}
        else{XPT2046_Coord co;if(XPT2046_GetTouchPoint(&co)==0){delay_ms(30);if(XPT2046_GetTouchPoint(&co)==0){while(XPT2046_GetTouchPoint(&co)==0);delay_ms(20);if(ExH(co.x,co.y)){return;}}}}
    }
}

/* ═══ 页5:历史记录 ═══ */
static void P5(void){
    LogRec buf[80];uint16_t n;Log_Read(buf,80,&n);
    LBlue();LCD_Clear(0,0,240,320);LCD_DispStringEN(5,2,0,"== ACCESS LOG ==");
    if(n==0){LCD_DispStringEN(10,80,0,"(No records)");}
    else{int8_t start=n-1;if(start<0)start=0;int8_t shown=0;
        for(int8_t i=start;i>=0&&shown<8;i--,shown++){char b[40];uint8_t t=buf[i].type;
            if(t==1){LCD_SetTextColor(0x07E0);LCD_SetBackColor(0x001F);}else{LCD_SetTextColor(0xF800);LCD_SetBackColor(0x001F);}
            sprintf(b,"#%d %s",buf[i].seq,(t==1)?"OK":"FAIL");LCD_DispStringEN(5,30+shown*22,0,b);}}
    ExDraw();}

/* ═══ 主函数 ═══ */
int main(void){
    SysTick_Config(SystemCoreClock/1000);
    UART2_Init(115200);LED_Init();BZ_Init();K_Init();
    LCD_Init();XPT2046_Init();
    DHT11_Init();MQ_Init();IR_Recv_Init();
    FPR_Init(56700);delay_ms(500);ZN632_VryPwd();ZN632_ReadIndexTable();
    ZN632_Empty();delay_ms(300);               /* 每次烧录清空指纹库 */
    W25QXX_Init();W25QXX_SectorErase(LOG_SECTOR*4096); /* 每次烧录清空日志 */
    g_log_n=0;g_seq=0;g_enroll=0;g_door=0;g_fail=0;g_consec=0;g_lock=0;
    strcpy(g_pw,"1234");
    BSP_RTC_Init();delay_ms(100);OLED_Init();
    RTC_Set_Time(12,0,0,RTC_H12_AM);RTC_Set_Date(26,7,19,1); /* 强制设置默认时间 */
    printf("\r\nDoor v9.1 Ready (All Reset)\r\n");

    LBlue();LCD_Clear(0,0,240,320);LCD_DispStringEN(30,110,0,"Door Access System");LCD_DispStringEN(30,150,0,"Starting...");delay_ms(800);
    LBlue();LCD_Clear(0,0,240,320);LCD_DispStringEN(10,60,0,"First Time Setup:");LCD_DispStringEN(10,90,0,"Go to Settings to");LCD_DispStringEN(10,120,0,"enroll finger & pwd.");delay_ms(1500);
    P1();

    while(1){
        IR_Recv();
        if(K3&&K4){delay_ms(30);if(K3&&K4){while(K3||K4);delay_ms(20);P4();P1();continue;}}
        int8_t b=TWait();if(b<0)continue;
        if(b==0){P2();while(1){int8_t b2=TWait();if(b2==-2){P1();break;}if(b2==0){if(g_lock)P2_Locked();else P2_Finger();}if(b2==1){if(g_lock)P2_Locked();else P2_Pwd();}}}
        else if(b==1){P3();int8_t b3=TWait();if(b3==-2||b3>=0)P1();}  /* 点任何处或Exit回P1 */
        else if(b==2){P4();P1();}
        else if(b==3){P5();int8_t b5=TWait();if(b5==-2||b5>=0)P1();}
    }
}
