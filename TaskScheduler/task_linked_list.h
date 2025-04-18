#ifndef TASK_LINKED_LIST_H
#define TASK_LINKED_LIST_H
#include "Arduino.h"
#define byte unsigned char
enum TaskState {
    RUNNING = 0, // Task is running
    READY = 1,   // Task is ready to run
    SUSPENDED = 2, // Task is suspended after execution (for aperiodic task)
	BLOCKED = 3,
	DELETED = 4
};
enum TaskType {
	PERIODIC = 0,
	APERIODIC = 1,
	INSTANT_EXECUTE = 2,
	BACKGROUND = 3,
	UNINF_PERIODIC = 4,
	NOT_FOUND = 5
};
class Task{
	public:
    volatile byte task_num;
    Task *prv, *nxt;
	volatile int base_priority;
	volatile int priority;
	volatile long int execute_time;
	volatile long int execute_time_left;
	volatile unsigned long int release_time;
	volatile int pos;
	volatile TaskType type;
	volatile TaskState state;
	volatile int terminate_num;
    Task(int pos, byte task_num, TaskType type, int base_priority, unsigned int execute_time, 
			unsigned long int release_time, int terminate_num, void (*task_act)(void));
	void (*task_act)(void);
};
struct LinkedList{
	Task *head;
	int size = 0;
	void updatePosition();
	void addNewTask(int pos, byte task_num, TaskType type, int base_priority, 
					unsigned int execute_time, unsigned long int release_time, int terminate_num, void (*task_act)(void));
	void deleteTask(int pos);
	int getTaskPos(byte task_num);
	TaskState Get_Task_State(byte task_num);
	void Set_Task_State(byte task_num, TaskState state);
	int Number_of_Tasks();
	int getTaskPriority(byte task_num);
	void setTaskPriority(byte task_num, int priority);
	int getTaskBasePriority(byte task_num);
	int getTaskExecuteTime(byte task_num);
	void setTaskExecuteTime(byte task_num, unsigned int execute_time);
	void setTaskReleaseTime(byte task_num, unsigned long int release_time);
	int getTaskReleaseTime(byte task_num);
	TaskType getTaskType(byte task_num);
	void changeTaskType(byte task_num, TaskType type);
	void fullTaskUpdate(byte task_num, TaskType type, int priority, 
						unsigned int execute_time, unsigned long int release_time, int terminate_num);
	void Task_Reset(byte task_num);
};
void copyTask(LinkedList &listA, LinkedList &listB, byte task_num);
#endif