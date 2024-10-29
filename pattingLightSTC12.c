/*********************************************************
STC12C2052AD配合振动开关SW-180xxP控制4种不同颜色的LED灯；
SW-180xxP振动开关受到一次敲击用于切换LED灯顺次亮灭；
受到两次敲击使得MCU进入掉电模式，此时关闭所有LED灯；
掉电模式下敲击一下唤醒MCU，恢复到上次关闭的状态
**************************************************************/
#include <STC12C2052AD.H>

#define PORT1 P1
#define KEY_INPUT P32

#define SINGLE_CONFIRM 25 // 该时间长度中没有再次检测到低电平，则确定是单击
#define PAT_INTERVAL 6 // 这个时间间隔是为了第一次检测到低电平之后的属于同一次敲击的低电平，是通过试验得到的经验值

#define N_KEY 0 // 返回无效键值
#define S_KEY 1 // 返回单击键值
#define D_KEY 2 // 返回双击键值

unsigned char judging_time; // 计时
unsigned char key_value; // 振动开关返回值

/**************************************************************/
void delay_ms(unsigned int a); // 不太精准的延时函数
void next_LED(); // 关闭当前灯，打开下一个灯
void power_down(); // 进入掉电模式
void Timer0_Init(void); // 定时器定时10ms初始化函数
void INTE0(void); // 掉电模式下触发外部中断处理函数
void time0(void);// 定时器0中断处理函数

/**************************************************************/
void main(void){
    EX0 = 0; // 禁止外部中断，此时该引脚当成普通的I/O接口使用，连接振动开关，导通时为低电平
    EA = 1; // 允许总中断
    ET0 = 1; // 允许定时器0中断
    PORT1 = 0xFE; // LED发光采用灌电流的方式，正极连接VCC，负极连接引脚，输出为0是才发光

    while (1){
        key_value = N_KEY;
        if (!KEY_INPUT){ // 首次检测到低电平
            judging_time = 0;
            Timer0_Init();
            while (judging_time < PAT_INTERVAL){ // 第一次检测到低电平的60ms之内等待即可
 
            } 
            while (judging_time < SINGLE_CONFIRM){

                if (!KEY_INPUT){ // 当时间大于60且小于250ms，如果检测到低电平，就是有第二次敲击
                    key_value = D_KEY;
                }
            }
            if (key_value != D_KEY){
                key_value = S_KEY;
            }
        }

        switch (key_value){
            case S_KEY: next_LED(); break;
            case D_KEY: power_down(); break;
            default: break;
        }
    }
}
/**************************************************************/
void delay_ms(unsigned int a){
    unsigned int i;
    while (a-- != 0){
        for (i = 0; i < 600; i++); // CPU 空转
    }
}

void next_LED(void){ // 关闭当前灯，打开下一个LED灯
    unsigned char port_temp = ~PORT1; // 采用灌电流的方式点亮LED，即点亮的灯的I/O为低电平
    if ((port_temp > 0x00) && (port_temp < 0x40)){ 
        port_temp = port_temp << 2; // 如果当前P16不是低电平，取反之后向左移两位，再取反
        PORT1 = ~port_temp;
    }
    else{
        PORT1 = 0xfe; // P11设置为低电平
    }
}

void power_down(void){
    PORT1 = 0x00;
    delay_ms(1000); // 所有灯亮1s
    EX0 = 1; // 允许外部中断0，用来将MCU从掉电模式唤醒
    IT0 = 0; // INT0的触发模式，0为低电平触发
    PORT1 = 0xFF; // 所有灯灭
    delay_ms(500); // 灯灭后500ms再进入掉电模式
    PCON |= 0x02; // PCON.1置1，其他位不变，进入掉电模式
}

void Timer0_Init(void){		//10毫秒@6.000MHz
	AUXR = 0x00;			//定时器时钟12T模式
	TMOD |= 0x11;			//设置定时器1和0均为16为手动重载
	TL0 = 0x78;				//设置定时初始值
	TH0 = 0xEC;				//设置定时初始值
	TF0 = 0;				//清除TF0标志，定时溢出时被硬件置1
	TR0 = 1;				//定时器0开始计时
}
/***********************************************************/
void time0(void) interrupt 1 {
    if (judging_time < 50000){ // 防止数据溢出
        judging_time++;
    }
    else{
        judging_time = 0;
    }

    TL0 = 0xB0;
    TH0 = 0x3C;
    TF0 = 0;
    TR0 = 1; // 手动重载定时器0
}

void INTE0(void) interrupt 0 { // 外部中断0的处理程序
    PORT1 = 0xFB; 
    delay_ms(500); // P12灯亮半秒
    EX0 = 0; // 重新禁止外部中断
    EA = 1; // 允许总中断
    PORT1 = 0xFE; // p10口的灯亮，表示从掉电模式中恢复
}