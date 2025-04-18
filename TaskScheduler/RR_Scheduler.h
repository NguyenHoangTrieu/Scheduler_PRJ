#ifndef RR_SCHEDULER_H
#define RR_SCHEDULER_H
#include"TaskScheduler.h"
extern volatile bool checking_timer1;
enum UpdateType{
    ADD,
    DELETE,
    UPDATE
};
struct Allocated_Task{
    byte task_num; 
    unsigned int remaining_execute_time; // the time the task must be executed
    unsigned long int last_check_time; // the time the task was last executed or task priority was last increased
    int remaining_quantums; // the number of quantums the task must be executed
    int executed_quantums; // the number of quantums the task has been executed
    int priority; // the priority of the task
};
class RR_Scheduler : public Scheduler
{
public:
volatile TIMER_UNIT time_unit;
volatile unsigned long int increase_time;
volatile byte previous_task_num;
volatile int quantum_length;
volatile byte current_execute_task;
volatile int task_list_size;
Allocated_Task *allocated_task_list = new Allocated_Task[task_list_size];
void Task_Scheduling();
void Instant_Event_handler(byte event_task_num);
void Add_Task(byte event_task_num, TaskType type, unsigned int execute_time, int base_priority,
                        unsigned long int release_time, int terminate_num, void (*task_act)(void), unsigned int execute_time_since_arrival);
void Timer_handler();
void Update_Allocated_List(byte task_num, UpdateType type);
void Sort_Allocated_List();
void Stop_Task(byte task_num);
void Begin_Setup(unsigned long int quantum_length, unsigned long int increase_time, TIMER_UNIT time_unit);
void Task_execute();
void Task_Update_And_Ready(byte task_num, TaskType type, int priority, 
						unsigned int execute_time, unsigned long int release_time, int terminate_num);
void Timer_event_set();
void Task_Delay(byte task_num, unsigned long int delay_time); 
void Change_Ready_time(byte task_num, unsigned int ready_time);
};
extern RR_Scheduler RoundRobin_scheduler;
#endif
