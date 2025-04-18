#include "RR_Scheduler.h"
RR_Scheduler RoundRobin_scheduler;
volatile bool checking_timer1 = false;
/*ISR hardware timer*/
#if defined(ARDUINO_ARCH_AVR)||defined(__AVR__)||defined(ARDUINO_ARCH_UNO)||defined(__AVR_ATmega328P__)
void RR_onTimer(){
 checking_timer1 = true;
}
#endif
#if defined(ARDUINO_ARCH_ESP32)||defined(ESP32)||defined(ARDUINO_ESP32_DEV)||defined(__ESP32__)||defined(ARDUINO_ESP32)
void IRAM_ATTR RR_onTimer() {
    timer_service_task.global_counter++;
    checking_timer1 = true;
}
#endif
/*  This function is used to decide which task will be executed next based on the priority of the task.
    The task with the highest priority will be executed first. */
void RR_Scheduler::Task_Scheduling(){
    static int count;
    if(count > task_list_size - 1) count = 0;
     bool allocated_task = false;
    if (task_list_size == 0){
        current_execute_task = 0;
        return;
    }
    else if (allocated_task_list[0].executed_quantums == 0 || task_list_size == 1){
        allocated_task_list[0].last_check_time = timer_service_task.global_counter;
        current_execute_task = allocated_task_list[0].task_num;
        count++;
    }
    else{
        for (int i = 0; i < count; i++) {
            if (allocated_task_list[i].executed_quantums == 0 && i != 0 && allocated_task_list[i].remaining_quantums != 0){
                allocated_task_list[i].last_check_time = timer_service_task.global_counter;
                current_execute_task = allocated_task_list[i].task_num;
                allocated_task = true;
                count++;
                break;
            }
        }
        if (count == 0 && allocated_task == false){
            allocated_task_list[0].last_check_time = timer_service_task.global_counter;
            current_execute_task = allocated_task_list[0].task_num;
            allocated_task = true;
            if(allocated_task_list[0].remaining_quantums != 1) count++;
        }
        if (count == task_list_size - 1 && allocated_task == false){
            allocated_task_list[count].last_check_time = timer_service_task.global_counter;
            current_execute_task = allocated_task_list[count].task_num;
            allocated_task = true;
            count = 0;
        }
        if (count < task_list_size && allocated_task == false){
            current_execute_task = allocated_task_list[count].task_num;
            allocated_task_list[count].last_check_time = timer_service_task.global_counter;
            allocated_task = true;
            if(allocated_task_list[count].remaining_quantums != 1) count++;
        }
        if (timer_service_task.global_counter - allocated_task_list[0].last_check_time > increase_time)
            count = 0;
    }
    for (int i = 0; i < task_list_size; i++) {
        unsigned long int last_time = timer_service_task.global_counter - allocated_task_list[i].last_check_time;
        if (last_time > increase_time ) {
            allocated_task_list[i].priority++;
            Task_Priority_Increase(allocated_task_list[i].task_num);
            allocated_task_list[i].last_check_time = timer_service_task.global_counter;
        }
    }
}
/*  This function is used to handle the instant event, whose certain 
    task will be executed immediately after event happened. */
void RR_Scheduler::Instant_Event_handler(byte event_task_num){
    if(taskList.getTaskPos(event_task_num) == -1) return;
    Task *temp = taskList.head;
    while (temp != NULL){
        if(temp->task_num == event_task_num){
            if(temp->type != INSTANT_EXECUTE || temp->task_num == current_execute_task) return;
            Task_Ready(temp->task_num);
            timer_service_task.Add_timer(1, current_execute_task, ONE_SHOT, timer_service_task.global_counter + temp->execute_time);
            break;
        }
        temp = temp->nxt;
    } 
    Task *temp1 = readyList.head;
    while (temp1 != NULL){
        if(temp1->task_num == current_execute_task) break;
        temp1 = temp1->nxt;
    }
    unsigned long int timer_num;
    if (temp1->execute_time_left >= quantum_length)
        timer_num = ceil((double)timer_service_task.global_counter/quantum_length) * quantum_length + temp->execute_time;  
    else timer_num = timer_service_task.global_counter + temp1->execute_time_left + temp->execute_time;
    Serial.println(temp1->execute_time_left);
    timer_service_task.Change_timer(0xFE, timer_num);
    current_execute_task = event_task_num;
}
/* This function is used to add new task and make and timer to make it ready in the specified 
time since this function is called or instantly added. */
void RR_Scheduler::Add_Task(byte event_task_num, TaskType type, unsigned int execute_time, int priority,
                        unsigned long int release_time, int terminate_num, void (*task_act)(void), unsigned int execute_time_since_arrival){
    for (int i = 0; i < task_list_size; i++){
        if(allocated_task_list[i].task_num == event_task_num)
        return;
    }
    if(execute_time_since_arrival == 0){
        if(taskList.getTaskPos(event_task_num) == -1){
            Task_Create(event_task_num, type, priority, execute_time, release_time, terminate_num, task_act);
            Update_Allocated_List(event_task_num, ADD);
            Sort_Allocated_List();
            return;
        }
        else if(taskList.getTaskPos(event_task_num) != -1) return;
    }
    else{
         unsigned long int timer_num = timer_service_task.global_counter + execute_time_since_arrival;
        if(taskList.getTaskPos(event_task_num) == -1){
            Task_Create(event_task_num, type, priority , execute_time, release_time, terminate_num, task_act);
            Task_Suspend(event_task_num);
            timer_service_task.Add_timer(1, event_task_num, ONE_SHOT, timer_num);
        }
        else if(taskList.getTaskPos(event_task_num) != -1) return;
    }
}
/*  This function is used to handle the timer end flag, to add time event task to the readyList
and allocated task list or make period task ready to run. */
void RR_Scheduler::Timer_handler(){
    timer_service_task.Timer_check();
    SoftwareTimer *tempTimer = timer_service_task.head;
    Task *temp = taskList.head;
    while (tempTimer != NULL){
        if (tempTimer->end_flag == true && tempTimer->active == false){
        Serial.println(timer_service_task.global_counter);
        Serial.println(tempTimer->timer_num);
        if(tempTimer->timer_num != 0xFE){
            while (temp != NULL){
                if(temp->task_num == tempTimer->timer_num && temp->state == READY){    
                    current_execute_task = temp->task_num;
                    break;
                }
                else if(temp->task_num == tempTimer->timer_num && (temp->state == SUSPENDED || temp->state == BLOCKED)){
                    Task_Ready(temp->task_num);
                    Update_Allocated_List(temp->task_num, ADD);
                    Sort_Allocated_List();
                    if (task_list_size == 1) current_execute_task = 0xFE;
                    break;
                }
                temp = temp->nxt;
            }
        }
        else if(tempTimer->timer_num == 0xFE){
            if (task_list_size == 0){
                current_execute_task = 0;
                timer_service_task.Stop_timer(0xFE);
            }
            else current_execute_task = 0xFE;
        }
        if (tempTimer->method == ONE_SHOT)
            timer_service_task.Delete_timer(tempTimer->pos);
        else tempTimer->end_flag = false;
        }
        tempTimer = tempTimer->nxt;
    }
    Timer_event_set();
}
/*  This function is used to update the allocated task list, which is used to store name of tasks that are ready to run. */
void RR_Scheduler::Update_Allocated_List(byte task_num, UpdateType type){
    Task *temp = readyList.head;
    while (temp != NULL){
        if(temp->task_num == task_num) break;
        temp = temp->nxt;
    }
    switch (type){
        case ADD:
            allocated_task_list[task_list_size].task_num = task_num;
            allocated_task_list[task_list_size].remaining_execute_time = temp->execute_time_left;
            allocated_task_list[task_list_size].remaining_quantums = ceil((double)temp->execute_time_left/quantum_length);
            allocated_task_list[task_list_size].last_check_time = 0;
            allocated_task_list[task_list_size].executed_quantums = 0;
            allocated_task_list[task_list_size].priority = temp->priority;
            task_list_size++;
            break;
        case DELETE:
                for (int i = 0; i < task_list_size; i++) {
                    if (allocated_task_list[i].task_num == task_num) {
                        for (int j = i; j < task_list_size - 1; j++) {
                            allocated_task_list[j] = allocated_task_list[j + 1];
                        }
                        Sort_Allocated_List();
                        task_list_size--;
                        allocated_task_list[task_list_size] = (Allocated_Task){0};
                        break;
                    }
                }
            break;
        case UPDATE:
            for (int i = 0; i < task_list_size; i++){
                if(allocated_task_list[i].task_num == task_num){
                    allocated_task_list[i].remaining_execute_time = temp->execute_time_left;
                    allocated_task_list[i].remaining_quantums = ceil((double)temp->execute_time_left/quantum_length);
                    allocated_task_list[i].priority = temp->priority;
                    int executed_quantums1 = ceil((double)(temp->execute_time - temp->execute_time_left)/quantum_length);
                    allocated_task_list[i].executed_quantums = executed_quantums1;
                    break;
                }
            }
            break;
        default:
            break;
    }
}
void RR_Scheduler::Sort_Allocated_List(){
    for (int i = 1; i < task_list_size; i++) {
        Allocated_Task key = allocated_task_list[i];
        int j = i - 1;
        while (j >= 0 && allocated_task_list[j].priority < key.priority) {
            allocated_task_list[j + 1] = allocated_task_list[j];
            j = j - 1;
        }
        allocated_task_list[j + 1] = key;
    }
}
/*  This function is used to stop the task execution and move it to the SUSPENDED state. */
void RR_Scheduler::Stop_Task(byte task_num){
    Task *temp = readyList.head;
    while (temp != NULL){
        if(temp->task_num == task_num){
            taskList.Set_Task_State(task_num, SUSPENDED);
            readyList.Set_Task_State(task_num, SUSPENDED);
            Update_Allocated_List(task_num, DELETE);
            break;
        }
        temp = temp->nxt;
    }
}
/*this function is used to execute task from allocated task list*/
void RR_Scheduler::Task_execute(){
    if (current_execute_task == 0){
        Task *temp = readyList.head;
        while (temp != NULL){
            if(temp->type == BACKGROUND) break;
            temp = temp->nxt;
        }
        if (temp != NULL){
        while(1){
            temp->task_act();
            if(checking_timer1){
                Timer_handler();
                checking_timer1 = false;
            if (current_execute_task != 0 || task_list_size > 0){
                if(task_list_size > 0) current_execute_task = 0xFE;
                Serial.println(task_list_size);
                break;
            }
            }
        }}
        else{
            if(checking_timer1){
                Timer_handler();
                checking_timer1 = false;
            }
        }
        return;
    }
    else if(current_execute_task != 0 && current_execute_task != 0xFE){
        volatile Task *temp = readyList.head;
        while (temp != NULL){
            if(temp->task_num == current_execute_task && temp->state == READY){
                taskList.Set_Task_State(temp->task_num, RUNNING);
                temp->state = RUNNING;
                break;
            }
            temp = temp->nxt;
        }
        while(1){
            temp->task_act();
            if(checking_timer1){
                if (timer_service_task.global_counter == 10) temp->execute_time_left -= 10;
                else temp->execute_time_left--;
                Timer_handler();
                checking_timer1 = false;
                if(current_execute_task != temp->task_num || temp->state != RUNNING){
                    if(current_execute_task != temp->task_num && current_execute_task != 0xFE){
                        temp->state = READY;
                        taskList.Set_Task_State(temp->task_num, READY);
                    }
                    break; 
                }
            }
        }
        if (temp->state == SUSPENDED){
            if(current_execute_task == temp->task_num){
                Serial.println(timer_service_task.global_counter);
                Serial.println(temp->execute_time_left);
                unsigned long int timer_num = timer_service_task.global_counter + quantum_length;
                timer_service_task.Change_timer(0xFE, timer_num);
                Task_Scheduling();
            }
            Task_Suspend(temp->task_num);
            return;
        }
        else if (temp->state == RUNNING){
            temp->state = READY;
            taskList.Set_Task_State(temp->task_num, READY);
            if (temp->execute_time_left > 0){
                Update_Allocated_List(temp->task_num, UPDATE);
                Sort_Allocated_List();
            }
            else if(temp->execute_time_left <= 0){
                Update_Allocated_List(temp->task_num, DELETE);
                Task_Priority_Reset(temp->task_num);
                if(temp->type == PERIODIC || temp->type == UNINF_PERIODIC){
                    if (temp->release_time == 0){
                        temp->state = READY;
                        taskList.Set_Task_State(temp->task_num, READY);
                        temp->execute_time_left = temp->execute_time;
                        temp->priority = temp->base_priority;
                        Update_Allocated_List(temp->task_num, UPDATE);
                    }
                    else{
                        temp->state = BLOCKED;
                        temp->execute_time_left = temp->execute_time;
                        taskList.Set_Task_State(temp->task_num, BLOCKED);
                        unsigned long int timer_num = timer_service_task.global_counter + temp->release_time;
                        if(timer_service_task.Get_timer_pos(temp->task_num) == -1)
                            timer_service_task.Add_timer(1, temp->task_num, AUTO_RELOAD, timer_num);
                        else {
                            timer_service_task.Change_timer(temp->task_num, timer_num);
                        }
                        if(temp->type == UNINF_PERIODIC){
                            temp->terminate_num--;
                            if (temp->terminate_num == 0) Task_Suspend(temp->task_num);
                        }
                    }
                }
                else if(temp->type == APERIODIC || temp->type == INSTANT_EXECUTE) Task_Suspend(temp->task_num);
            }
        }
        else if (temp->state == READY ){
            Serial.println(timer_service_task.global_counter);
            Update_Allocated_List(temp->task_num, UPDATE);
            Sort_Allocated_List();
        }
        else if (temp->state == BLOCKED){
            Serial.println(timer_service_task.global_counter);
            Serial.println(temp->execute_time_left);
            Update_Allocated_List(temp->task_num, DELETE);
            timer_service_task.Change_timer(0xFE, timer_service_task.global_counter + quantum_length);
            Task_Scheduling();
        }
    }
    else if(current_execute_task == 0xFE){
        Task_Scheduling();
        if(task_list_size != 0){
            if (current_execute_task == 0){
                timer_service_task.Change_timer(0xFE, timer_service_task.global_counter + quantum_length);
                return;
            }
            if(task_list_size == 1){
                timer_service_task.Change_timer(0xFE, timer_service_task.global_counter + allocated_task_list[0].remaining_execute_time);
            }
            else{
                int i;
                for (i = 0; i < task_list_size; i++){
                    if(allocated_task_list[i].task_num == current_execute_task) break;
                }
                if (allocated_task_list[i].remaining_quantums != 1){
                    timer_service_task.Change_timer(0xFE, timer_service_task.global_counter + quantum_length);
                }
                else if(allocated_task_list[i].remaining_quantums == 1 ){
                    timer_service_task.Change_timer(0xFE, timer_service_task.global_counter + allocated_task_list[i].remaining_execute_time);
                }
            }
        }
    }
}
/*  This function is used to update the task's type, priority, execute time, release time and make it ready (not for INSTANT_EXECUTE task). */
void RR_Scheduler::Task_Update_And_Ready(byte task_num, TaskType type, int priority, 
                        unsigned int execute_time, unsigned long int release_time, int terminate_num){
    if(taskList.getTaskPos(task_num) == -1) return;
    if(type == INSTANT_EXECUTE) return;
    else if(taskList.getTaskPos(task_num) != -1) {
        if (taskList.Get_Task_State(task_num) == RUNNING || taskList.Get_Task_State(task_num) == READY)
            return;
        taskList.fullTaskUpdate(task_num, type, priority, execute_time, release_time, terminate_num);
        if(readyList.getTaskPos(task_num) != -1) 
            readyList.fullTaskUpdate(task_num, type, priority, execute_time, release_time, terminate_num);
        Task_Ready(task_num);
        Update_Allocated_List(task_num, ADD);
        Sort_Allocated_List();
    }
}
void RR_Scheduler::Task_Delay(byte task_num, unsigned long int delay_time){
    if (taskList.getTaskPos(task_num) == -1) return;
    Task *temp = readyList.head;
    Update_Allocated_List(task_num, DELETE);
    Sort_Allocated_List();
    while (temp != NULL){
        if(temp->task_num == task_num){
            unsigned long int timer_num = timer_service_task.global_counter + delay_time;
            if (timer_service_task.Get_timer_pos(task_num) != -1)
                timer_service_task.Change_timer(task_num, timer_num);
            else timer_service_task.Add_timer(1, temp->task_num, ONE_SHOT, timer_num);
            temp->state = BLOCKED;
            taskList.Set_Task_State(temp->task_num, BLOCKED);
            break;
        }
        temp = temp->nxt;
    }
}
void RR_Scheduler::Change_Ready_time(byte task_num, unsigned int ready_time){
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
/* This function for the setup of the Round Robin Scheduler */
void RR_Scheduler::Begin_Setup(unsigned long int quantum_length, unsigned long int increase_time ,TIMER_UNIT time_unit){
    this->quantum_length = quantum_length;
    this->increase_time = increase_time;
    Task *temp = readyList.head;
    while (temp != NULL){
        Update_Allocated_List(temp->task_num, ADD);
        temp = temp->nxt;
    }
    Sort_Allocated_List();
    Task_Scheduling();
    #if defined(ARDUINO_ARCH_ESP32)||defined(ESP32)||defined(ARDUINO_ESP32_DEV)||defined(__ESP32__)||defined(ARDUINO_ESP32)
    timer_service_task.Set_timer(time_unit, RR_onTimer);
    #endif
    #if defined(ARDUINO_ARCH_AVR)||defined(__AVR__)||defined(ARDUINO_ARCH_UNO)||defined(__AVR_ATmega328P__)
    timer_service_task.Set_timer(time_unit);
    #endif
    timer_service_task.Add_timer(1, 0xFE, AUTO_RELOAD, this->quantum_length);

}