#ifndef RTC_SCHEDULER_H
#define RTC_SCHEDULER_H
#include"TaskScheduler.h"
extern volatile bool checking_timer;
extern volatile bool PORTB_event_flag;
class RTC_Scheduler : public Scheduler{
public:
volatile byte current_execute_task;
volatile TIMER_UNIT time_unit;
void Timer_set();
void Instant_Event_handler(byte event_task_num);
void Add_Task(byte event_task_num, TaskType type, unsigned int execute_time, int terminate_num, 
                void (*task_act)(void), int execute_time_since_arrival);
void Task_execute();
void Stop_Task(byte task_num);
void Timer_handler();
void Begin_Setup(TIMER_UNIT time_unit);
void Timer_event_set();
void PORTB_event_handler();
void Task_Delay(byte task_num, unsigned int delay_time);
void Change_Ready_time(byte task_num, unsigned int ready_time);
};
extern RTC_Scheduler RTC_scheduler;
void Print_Task_num_in_taskList();
void Print_timer_num_in_timerList();
#endif