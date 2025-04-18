#include "RTC_Scheduler.h"
RTC_Scheduler RTC_scheduler;
volatile bool checking_timer = false;
volatile bool PORTB_event_flag = false;
/*ISR hardware timer*/
#if defined(ARDUINO_ARCH_AVR)||defined(__AVR__)||defined(ARDUINO_ARCH_UNO)||defined(__AVR_ATmega328P__)
void RTC_onTimer(){
    checking_timer = true;
}
#endif
#if defined(ARDUINO_ARCH_ESP32)||defined(ESP32)||defined(ARDUINO_ESP32_DEV)||defined(__ESP32__)||defined(ARDUINO_ESP32)
void IRAM_ATTR RTC_onTimer() {
    timer_service_task.global_counter++;
    checking_timer = true;
}
#endif
/* This function is used to set the timer for the task in the ready list.
    The timer is set according to the type of the task.
    If the task is periodic then the timer is set to auto-reload.
    If the task is aperiodic then the timer is set to one-shot.*/
void RTC_Scheduler::Timer_set(){
    Task *temp = readyList.head;
    unsigned long int time_set_var = 0;
    int count = 1;
    if (temp->nxt == NULL)
            time_set_var = temp->execute_time + timer_service_task.global_counter;
    else{
        while (temp != NULL){
            if (temp->prv == NULL) time_set_var = timer_service_task.global_counter;
            else {
                time_set_var += temp->prv->execute_time;
                if(temp->execute_time_left > 0 || (temp->type == PERIODIC 
                || (temp->type == UNINF_PERIODIC && temp->terminate_num > 0) && temp->release_time == 0)){ 
                    if(timer_service_task.Get_timer_pos(temp->task_num) == -1){
                        if (temp->type == PERIODIC && temp->state == READY)
                            timer_service_task.Add_timer(count, temp->task_num, AUTO_RELOAD, time_set_var);
                        else if (temp->type == APERIODIC && temp->state == READY)
                            timer_service_task.Add_timer(count, temp->task_num, ONE_SHOT, time_set_var);
                    }
                    if(timer_service_task.Get_timer_pos(temp->task_num) != -1)
                        timer_service_task.Change_timer(temp->task_num, time_set_var);
                    count++;
                }
            }
            if (temp->nxt == NULL && (temp->execute_time_left > 0 || temp->type == PERIODIC) && temp->release_time == 0) 
                time_set_var += temp->execute_time;
            temp = temp->nxt;
        }
    }
    if(timer_service_task.Get_timer_pos(0xFE) == -1)
        timer_service_task.Add_timer(count, 0xFE, AUTO_RELOAD, time_set_var);
    else if(timer_service_task.Get_timer_pos(0xFE) != -1)
        timer_service_task.Change_timer(0xFE, time_set_var);
}
/*  This function is used to handle the instant event, whose certain 
    task will be executed immediately after event happened. */
void RTC_Scheduler::Instant_Event_handler(byte event_task_num){
    if(taskList.getTaskPos(event_task_num) == -1) return;
    Task *temp = taskList.head;
    Task *temp1 = readyList.head;
    bool set_timer_flag = false;
    while (temp != NULL){
        if(temp->task_num == event_task_num){
            if(temp->type != INSTANT_EXECUTE || temp->task_num == current_execute_task) return;
            Task_Ready(temp->task_num);
            if (current_execute_task != 0){
                while (temp1 != NULL){
                    if(temp1->task_num == current_execute_task && temp1->state == RUNNING){
                        if(timer_service_task.Get_timer_pos(temp1->task_num) != -1){
                        timer_service_task.Change_timer(temp1->task_num, timer_service_task.global_counter + temp->execute_time);
                        }
                        else timer_service_task.Add_timer(1, temp1->task_num, ONE_SHOT, 
                                                            timer_service_task.global_counter + temp->execute_time);
                        break;
                    }
                    temp1 = temp1->nxt;
                }
            }
            else{
                if(timer_service_task.Get_timer_pos(0) != -1)
                    timer_service_task.Change_timer(0, timer_service_task.global_counter + temp->execute_time);
                else timer_service_task.Add_timer(1, 0, ONE_SHOT, timer_service_task.global_counter + temp->execute_time);
            }
            break;
        }
        temp = temp->nxt;
    }
    SoftwareTimer *tempTimer = timer_service_task.head;
    while(tempTimer != NULL){
        if(tempTimer->end_flag == false && tempTimer->timer_num != current_execute_task
            && taskList.Get_Task_State(tempTimer->timer_num) == READY || tempTimer->timer_num == 0xFE){
                tempTimer->compare_time += temp->execute_time;
            }
        tempTimer = tempTimer->nxt;
    }
    current_execute_task = temp->task_num;
}
/*  This function is used to add new/set time event task.
    The task will be ready after a certain time period since the event happened.*/
void RTC_Scheduler::Add_Task(byte event_task_num, TaskType type, unsigned int execute_time, int terminate_num, 
                                void (*task_act)(void), int execute_time_since_arrival){
    if(taskList.getTaskPos(event_task_num) != -1) return;
    if(execute_time_since_arrival == 0){
        Task_Create(event_task_num, type, 0 , execute_time, 0, terminate_num, task_act);
        SoftwareTimer *tempTimer = timer_service_task.head;
        while(tempTimer != NULL){
            if (tempTimer->timer_num == 0xFE){
                if (tempTimer->end_flag == false  && tempTimer->active == true){
                Task *temp = readyList.head;
                if(temp->nxt == NULL){
                    current_execute_task = temp->task_num;
                    tempTimer->compare_time = timer_service_task.global_counter + execute_time;
                }
                else{
                    timer_service_task.Add_timer(1, event_task_num, ONE_SHOT, tempTimer->compare_time);
                    tempTimer->compare_time += execute_time;
                }
                tempTimer->end_flag = false;
                tempTimer->active = true;
                }
                break;
            }
            tempTimer = tempTimer->nxt;
        }
        return;
    }
    else {
        Task_Create(event_task_num, type, 0 , execute_time, 0, terminate_num, task_act);
        Task_Suspend(event_task_num);
        TIMER_METHOD method1;
        if (type == PERIODIC)
        method1 = AUTO_RELOAD;
        else if (type == APERIODIC)
        method1 = ONE_SHOT;
        else return;
        timer_service_task.Add_timer(1, event_task_num, method1, 
                                    timer_service_task.global_counter + execute_time_since_arrival);
    }
}

/*  This function is used to handle the time event, whose certain 
    task will be added to readyList after a certain time period */
void RTC_Scheduler::Timer_handler(){
    timer_service_task.Timer_check();
    SoftwareTimer *tempTimer = timer_service_task.head;
    Task *temp = readyList.head;
    while(tempTimer != NULL){
        if(tempTimer->end_flag == true && tempTimer->active == false){
            if(tempTimer->timer_num == 0){
                temp = readyList.head;
                if (temp != NULL){ 
                    if (temp->nxt != NULL){
                        if (temp->nxt->state == READY){
                            current_execute_task = temp->task_num;
                            break;
                        }
                    }
                    else current_execute_task = 0;
                }
                if(temp == NULL || temp->state != READY) current_execute_task = 0;
                tempTimer->end_flag = false;
            }
            else if(tempTimer->timer_num == 0xFE){
                temp = readyList.head;
                if (temp->state == READY || temp->state == RUNNING){
                    if (temp->nxt == NULL && ((temp->type == PERIODIC && temp->release_time != 0) || temp->type != PERIODIC 
                    || (temp->type != UNINF_PERIODIC && temp->terminate_num == 0))) 
                        current_execute_task = 0;
                    else{
                        if(current_execute_task == temp->task_num && temp->nxt != NULL && temp->nxt->state == READY) 
                            current_execute_task = temp->nxt->task_num;
                        else current_execute_task = temp->task_num;
                    } 
                }
                tempTimer->end_flag = false;
                Timer_set();
            }
            else {
            temp = readyList.head;
            bool timer_delete_flag = true;
                while (temp != NULL){
                    if(temp->task_num == tempTimer->timer_num){
                        if (temp->state == READY){
                            current_execute_task = temp->task_num;
                        }
                        break;
                    }
                    temp = temp->nxt;
                }
                if (temp == NULL){
                    Task_Ready(tempTimer->timer_num);
                    tempTimer->compare_time = timer_service_task.Get_compare_time(0XFE);
                    tempTimer->active = true;
                    timer_delete_flag = false;
                    unsigned long int timer_set = tempTimer->compare_time + readyList.getTaskExecuteTime(tempTimer->timer_num);
                    timer_service_task.Change_timer(0xFE, timer_set);
                }
                else if (temp->state == BLOCKED){
                    temp->state = READY;
                    taskList.Set_Task_State(temp->task_num, READY);
                    if(temp == readyList.head) current_execute_task = temp->task_num;
                    else tempTimer->compare_time = timer_service_task.Get_compare_time(0XFE);
                    tempTimer->active = true;
                    timer_delete_flag = false;
                    unsigned long int timer_set = tempTimer->compare_time + temp->execute_time_left;
                    timer_service_task.Change_timer(0xFE, timer_set);
                }
                if (tempTimer->method == ONE_SHOT && timer_delete_flag == true) timer_service_task.Delete_timer(tempTimer->pos);
                else tempTimer->end_flag = false;
            }
        }
        tempTimer = tempTimer->nxt;
    }
}
/*this function is used to execute task from readyList*/
void RTC_Scheduler::Task_execute(){
    if (current_execute_task == 0){
        Task *temp = readyList.head;
        while (temp != NULL){
            if(temp->type == BACKGROUND) break;
            temp = temp->nxt;
        }
        if (temp != NULL){
        while(1){
            temp->task_act();
            if(checking_timer){
                Timer_event_set();
                Timer_handler();
                checking_timer = false;
            }
            if(PORTB_event_flag){
                PORTB_event_flag = false;
                PORTB_event_handler();
            }
            if (current_execute_task != 0) break;
        }}
        else{
            if(checking_timer){
                Timer_event_set();
                Timer_handler();
                checking_timer = false;
            }
            if(PORTB_event_flag){
                PORTB_event_flag = false;
                PORTB_event_handler();
            }
        }
        return;
    }
    Task *temp = readyList.head;
    while (temp != NULL){
        if (temp->task_num == current_execute_task && temp->state == READY){
            temp->state = RUNNING;
            taskList.Set_Task_State(temp->task_num, RUNNING);
            while(1){
                temp->task_act();
                if (checking_timer == true){
                    temp->execute_time_left--;
                    Timer_event_set();
                    Timer_handler();
                    if(temp->execute_time_left == 0) temp->execute_time_left = temp->execute_time;
                    checking_timer = false;
                }
                if(PORTB_event_flag){
                PORTB_event_flag = false;
                PORTB_event_handler();
                }
                if(current_execute_task != temp->task_num || temp->state != RUNNING){
                    if(current_execute_task != temp->task_num && temp->state == RUNNING){
                        temp->state = READY;
                        taskList.Set_Task_State(temp->task_num, READY);
                    }
                break; 
                }
            }
            if (temp->state == READY){
                if (temp->type == APERIODIC || temp->type == INSTANT_EXECUTE) Task_Suspend(temp->task_num);
                else if (temp->type == PERIODIC|| temp->type == UNINF_PERIODIC){
                    if (temp->release_time == 0){
                        temp->state = READY;
                        taskList.Set_Task_State(temp->task_num, READY);
                    }
                    else{
                        temp->state = BLOCKED;
                        taskList.Set_Task_State(temp->task_num, BLOCKED);
                        unsigned long int timer_num = timer_service_task.global_counter + temp->release_time;
                        if(timer_service_task.Get_timer_pos(temp->task_num) == -1)
                            timer_service_task.Add_timer(1, temp->task_num, AUTO_RELOAD, timer_num);
                        else {
                            timer_service_task.Change_timer(temp->task_num, timer_num);
                        }
                    }
                    if(temp->type == UNINF_PERIODIC){
                        temp->terminate_num--;
                        if (temp->terminate_num == 0) Task_Suspend(temp->task_num);
                    }
                }
            }
            else{
                SoftwareTimer *tempTimer = timer_service_task.head;
                Task *temp1 = readyList.head;
                while (tempTimer != NULL){
                    temp1 = readyList.head;
                    while (temp1 != NULL){
                        if ((temp1->task_num == tempTimer->timer_num || tempTimer->timer_num == 0xFE)
                            && tempTimer->active == true && temp1->task_num != current_execute_task && temp1->state == READY){
                                tempTimer->compare_time -= temp->execute_time_left;
                                if (tempTimer->compare_time <= timer_service_task.global_counter)
                                    current_execute_task = temp1->task_num;
                            break;
                        }
                        temp1 = temp1->nxt;
                    }
                    tempTimer = tempTimer->nxt;
                }
                if (current_execute_task == temp->task_num){
                    if (temp->nxt != NULL){
                        temp1 = temp->nxt;
                        while (temp1 != NULL){
                            if (temp1->state == READY){
                                current_execute_task = temp1->task_num;
                                break;
                            }
                            temp1 = temp1->nxt;
                        }
                    }
                    if (temp->nxt == NULL || temp1 == NULL){
                        temp1 = readyList.head;
                        if (temp1 != NULL && temp1->state == READY)
                            current_execute_task = temp1->task_num;
                        else current_execute_task = 0;
                    }
                }
                if (temp->state == SUSPENDED){
                    temp->execute_time_left = temp->execute_time;
                    Task_Suspend(temp->task_num);
                }
            }
            break;
        }
        temp = temp->nxt;
    }
}
/*This function is used to stop the task from the readyList*/
void RTC_Scheduler::Stop_Task(byte task_num){
    Task *temp = readyList.head;
    while (temp != NULL){
        if(temp->task_num == task_num){
            if(temp->state == RUNNING && temp->nxt != NULL)
                timer_service_task.Stop_timer(task_num);
            temp->state = SUSPENDED;
            break;
        }
        temp = temp->nxt;
    }
}
void RTC_Scheduler::Task_Delay(byte task_num, unsigned int delay_time){
    if (taskList.getTaskPos(task_num) == -1) return;
    Task *temp = readyList.head;
    while (temp != NULL){
        if(temp->task_num == task_num){
            unsigned long int timer_num = timer_service_task.global_counter + delay_time;
            if(timer_service_task.Get_timer_pos(task_num) != -1)
                timer_service_task.Change_timer(task_num, timer_num);
            else timer_service_task.Add_timer(1, temp->task_num, ONE_SHOT, timer_num);
            temp->state = BLOCKED;
            taskList.Set_Task_State(temp->task_num, BLOCKED);
            break;
        }
        temp = temp->nxt;
    }
}
void RTC_Scheduler::Change_Ready_time(byte task_num, unsigned int ready_time){
    if(timer_service_task.Get_timer_pos(task_num) == -1) return;
    SoftwareTimer *temp = timer_service_task.head;
    while (temp != NULL){
        if(temp->timer_num == task_num && (taskList.Get_Task_State(task_num) != READY
            || taskList.Get_Task_State(task_num) != RUNNING)){
            temp->compare_time = timer_service_task.global_counter + ready_time;
            break;
        }
        temp = temp->nxt;
    }
}
void RTC_Scheduler::Begin_Setup(TIMER_UNIT time_unit){
    #if defined(ARDUINO_ARCH_AVR)||defined(__AVR__)||defined(ARDUINO_ARCH_UNO)||defined(__AVR_ATmega328P__)
    timer_service_task.Set_timer(time_unit);
    Serial.println("Timer set");
    #endif
    #if defined(ARDUINO_ARCH_ESP32)||defined(ESP32)||defined(ARDUINO_ESP32_DEV)||defined(__ESP32__)||defined(ARDUINO_ESP32)
    timer_service_task.Set_timer(time_unit, RTC_onTimer);
    #endif
    Timer_set();
    Task *temp = readyList.head;
    current_execute_task = temp->task_num;
}
void Print_Task_num_in_taskList(){
    Serial.print("Ready List: ");
    Task *temp = RTC_scheduler.readyList.head;
    while (temp != NULL){
        Serial.print(temp->task_num);
        Serial.print("/");
        Serial.print(temp->state);
        Serial.print(" ");
        temp = temp->nxt;
    }
    Serial.println();
}
void Print_timer_num_in_timerList(){
    Serial.print("Timer List: ");
    SoftwareTimer *temp = timer_service_task.head;
    while (temp != NULL){
        Serial.print(temp->timer_num);
        Serial.print("/");
        Serial.print(temp->compare_time);
        Serial.print("; ");
        temp = temp->nxt;
    }
    Serial.println();
}