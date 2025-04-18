#ifndef TASKSCHEDULER_H
#define TASKSCHEDULER_H
#include "software_timer.h"
#include "task_linked_list.h"

class Scheduler {
public:
    LinkedList taskList;
    LinkedList readyList;
    void Task_Create(char task_num, TaskType type,int base_priority, 
                    unsigned int execute_time, unsigned long int release_time, int terminate_num, void (*task_act)(void));   
    void Task_Delete(byte task_num);
    void Task_Priority_Increase(byte task_num);
    void Task_Priority_Reset(byte task_num);
    void Task_Suspend(byte task_num);
    void Task_Ready(byte task_num);
    void Create_Background_Task(byte task_num, void (*task_act)(void));
};
#endif