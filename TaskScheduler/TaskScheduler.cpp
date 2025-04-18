#include "TaskScheduler.h"
/*Create a new task, and make it ready but not added to scheduling list.*/
void Scheduler::Task_Create(char task_num, TaskType type, int base_priority, 
                            unsigned int execute_time, unsigned long int release_time, int terminate_num, void (*task_act)(void))
{
    int pos = taskList.Number_of_Tasks() + 1;
    taskList.addNewTask(pos, task_num, type, base_priority, execute_time, release_time, terminate_num, task_act);
    if(type != INSTANT_EXECUTE) copyTask(taskList, readyList, task_num);
}
void Scheduler::Create_Background_Task(byte task_num, void (*task_act)(void))
{
    int pos = taskList.Number_of_Tasks() + 1;
    taskList.addNewTask(pos, task_num, BACKGROUND, 0, 0, 0, 0, task_act);
    copyTask(taskList, readyList, task_num);
}
/*Remove a task, the task being deleted will be removed
    from all lists and it's timer.*/
void Scheduler::Task_Delete(byte task_num)
{
    int pos = taskList.getTaskPos(task_num);
    if (pos != -1)
        taskList.deleteTask(pos);
    int ready_pos = readyList.getTaskPos(task_num);
    if (ready_pos != -1)
    {
        readyList.deleteTask(pos);
        timer_service_task.Delete_timer(timer_service_task.Get_timer_pos(task_num));
    }
}

/*Increase the priority of any task.*/
void Scheduler::Task_Priority_Increase(byte task_num)
{
    if (taskList.getTaskPos(task_num) != -1)
    {
        int priority = taskList.getTaskPriority(task_num);
        taskList.setTaskPriority(task_num, priority + 1);
        int ready_pos = readyList.getTaskPos(task_num);
        if (ready_pos != -1)
            readyList.setTaskPriority(task_num, priority + 1);
    }
}

/*Reset the priority of task after execution or being suspended.*/
void Scheduler::Task_Priority_Reset(byte task_num)
{
    int pos = taskList.getTaskPos(task_num);
    int ready_pos = readyList.getTaskPos(task_num);
    if (pos != -1)
    {
        int Base_priority = taskList.getTaskBasePriority(task_num);
        taskList.setTaskPriority(task_num, Base_priority);
        if (ready_pos != -1)
            readyList.setTaskPriority(task_num, Base_priority);
    }
}

/*Suspend any task. When suspended a task will never get any microcontroller processing time,
no matter what its priority.*/
void Scheduler::Task_Suspend(byte task_num)
{
    int ready_pos = readyList.getTaskPos(task_num);
    if (ready_pos != -1)
    {
        readyList.deleteTask(readyList.getTaskPos(task_num));
        if(timer_service_task.Get_timer_pos(task_num) != -1)
        timer_service_task.Delete_timer(timer_service_task.Get_timer_pos(task_num));
    }
    taskList.Set_Task_State(task_num, SUSPENDED);
}

/*A task that has been suspended by one or more calls to Task_Suspend()
will be made available for running again by a single call to
Task_Ready()*/
void Scheduler::Task_Ready(byte task_num)
{
    if (taskList.getTaskPos(task_num) != -1){
        taskList.Set_Task_State(task_num, READY);
        if (readyList.getTaskPos(task_num) == -1) copyTask(taskList, readyList, task_num);
        readyList.Set_Task_State(task_num, READY);
    }
}