#ifndef SOFTWARE_TIMER_H
#define SOFTWARE_TIMER_H
#include "Arduino.h"
#define byte unsigned char
enum TIMER_UNIT{
    mili_sec = 1,
    mili_sec_100 = 2,
    sec = 3
};
enum TIMER_METHOD{
    ONE_SHOT = 0,
    AUTO_RELOAD = 1
};
class SoftwareTimer {
public:
    volatile byte timer_num;
    SoftwareTimer *prv, *nxt;
    volatile unsigned long int compare_time;
    volatile int pos;
    SoftwareTimer(int pos, byte timer_num, TIMER_METHOD method, unsigned long int compare_time);
    volatile bool end_flag;
    volatile bool active;
    volatile TIMER_METHOD method;
};
#if defined(ARDUINO_ARCH_AVR)||defined(__AVR__)||defined(ARDUINO_ARCH_UNO)||defined(__AVR_ATmega328P__)
enum CLOCK_SPEED_RESOLUTION{
    RES_16MHZ = (1 << WGM12) | (1 << CS10),
    RES_2MHZ = (1 << WGM12) | (1 << CS11),
    RES_250KHZ = (1 << WGM12) | (1 << CS11)| (1 << CS10),
    RES_62500HZ = (1 << WGM12) | (1 << CS12)
};
void init_hardware_timer(CLOCK_SPEED_RESOLUTION clock_res);
void RTC_onTimer();
void RR_onTimer();
ISR(TIMER1_COMPA_vect);
#endif
#if defined(ARDUINO_ARCH_ESP32)||defined(ESP32)||defined(ARDUINO_ESP32_DEV)||defined(__ESP32__)||defined(ARDUINO_ESP32)
extern hw_timer_t *My_timer;
void IRAM_ATTR RTC_onTimer();
void IRAM_ATTR RR_onTimer();
void init_hardware_timer(int prescaler, int timer_alarm, void (*onTimer)());
#endif
struct Timer_service_task{
volatile unsigned long int global_counter;
SoftwareTimer *head;
int size = 0;
#if defined(ARDUINO_ARCH_ESP32)||defined(ESP32)||defined(ARDUINO_ESP32_DEV)||defined(__ESP32__)||defined(ARDUINO_ESP32)
void Set_timer(TIMER_UNIT unit, void (*onTimer)());
#endif
#if defined(ARDUINO_ARCH_AVR)||defined(__AVR__)||defined(ARDUINO_ARCH_UNO)||defined(__AVR_ATmega328P__)
void Set_timer(TIMER_UNIT unit);
#endif
void Add_timer(int pos, byte timer_num, TIMER_METHOD method, unsigned long int compare_time);
void Delete_timer(int pos);
void Stop_timer(byte timer_num);
void End_timer(byte timer_num);
void Start_all_timer();
void Start_timer(byte timer_num);
void Timer_check();
void Update_position();
void Change_timer(byte timer_num, unsigned long int compare_time_change);
int Get_timer_pos(byte timer_num);
unsigned long int Get_compare_time(byte timer_num);
};
extern Timer_service_task timer_service_task;
#endif