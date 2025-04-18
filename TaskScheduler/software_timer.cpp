#include"software_timer.h"
#include"RR_Scheduler.h"
#include"RTC_Scheduler.h"
Timer_service_task timer_service_task;
/*setting hardware timer/counter*/
#if defined(ARDUINO_ARCH_AVR)||defined(__AVR__)||defined(ARDUINO_ARCH_UNO)||defined(__AVR_ATmega328P__)
void init_hardware_timer(CLOCK_SPEED_RESOLUTION clock_res){
    cli();
    TCCR1A = 0;
    TCCR1B = 0;
    TIMSK1 = 0;
    TCCR1B |= clock_res;
    TIMSK1 |=  (1<< OCIE1A);
    sei();
}
ISR(TIMER1_COMPA_vect){
    timer_service_task.global_counter++;
    RTC_onTimer();
    RR_onTimer();
}
#endif
# if defined(ARDUINO_ARCH_ESP32)||defined(ESP32)||defined(ARDUINO_ESP32_DEV)||defined(__ESP32__)||defined(ARDUINO_ESP32)
hw_timer_t *My_timer = NULL;
void init_hardware_timer(int frequency, int timer_alarm, void (*onTimer)()){
    cli();
    My_timer = timerBegin(frequency);
    timerAttachInterrupt(My_timer, onTimer);
    timerAlarm(My_timer, timer_alarm, true, 0);
    sei();
}
#endif
/*SoftwareTimer class construction*/
SoftwareTimer::SoftwareTimer(int pos, byte timer_num, TIMER_METHOD method, unsigned long int compare_time){
    this->timer_num = timer_num;
    this->prv = NULL;
    this->nxt = NULL;
    this->compare_time = compare_time;
    this->end_flag = false;
    this->active = true;
    this->method = method;
    this->pos = pos;
}
/*update timer position in timer list*/
void Timer_service_task::Update_position(){
    SoftwareTimer* temp = head;
    int newPos = 1;
    while(temp != NULL){
        temp->pos = newPos;
        newPos++;
        temp = temp->nxt;
    }
}
/*add timer to list*/
void Timer_service_task::Add_timer(int pos, byte timer_num, TIMER_METHOD method, unsigned long int compare_time){
        SoftwareTimer* newTimer = new SoftwareTimer(pos, timer_num, method, compare_time);
        if(head == NULL){
            head = newTimer;
            newTimer->nxt = NULL;
            newTimer->prv = NULL;
            size++;
            return;
        }
        else if(pos == 1){
            newTimer->nxt = head;
            newTimer->prv = NULL;
            head->prv = newTimer;
            head = newTimer;
            size++;
            Update_position();
            return;
        }
        if(pos >= size + 1){
            SoftwareTimer* temp = head;
            while(temp->nxt != NULL) {temp = temp->nxt;}
            newTimer->prv = temp;
            newTimer->nxt = NULL;
            temp->nxt = newTimer;
            size++;
            Update_position();
            return;
        }
        else{ int count = 1;
        SoftwareTimer* temp = head;
        while(count < pos ){
            temp = temp->nxt;
            count++;
        }
        newTimer->nxt = temp;
        newTimer->prv = temp->prv;
        newTimer->prv->nxt = newTimer;
        temp->prv = newTimer;
        size++;
       }
    Update_position();
}
/*delete_timer from list*/
void Timer_service_task::Delete_timer(int pos){
    if(pos != -1){
        SoftwareTimer* temp = head;
        if(size == 1){
            head = NULL;
            free(temp);
            size--;
            return;
        }
        else if(pos == 1){
            temp->nxt->prv = NULL;
            head = temp->nxt;
            size--;
            free(temp);
            Update_position();
            return;
        }
        else if(pos <= size){ 
        int count = 1;
        while(count < pos){
            temp = temp->nxt;
            count++;
        }
        temp->prv->nxt = temp->nxt;
        if(pos < size) temp->nxt->prv = temp->prv;
        size--;
        free(temp);
        }
        Update_position();
    }
}
# if defined(ARDUINO_ARCH_AVR)||defined(__AVR__)||defined(ARDUINO_ARCH_UNO)||defined(__AVR_ATmega328P__)
/*set_timer before start*/
void Timer_service_task::Set_timer(TIMER_UNIT unit){
    switch (unit) {
        case mili_sec:
            init_hardware_timer(RES_250KHZ);
            OCR1A = 249;
            break;
        case mili_sec_100:
            init_hardware_timer(RES_250KHZ);
            OCR1A = 24999;
            break;
        case sec:
            init_hardware_timer(RES_62500HZ);
            OCR1A = 62499;
            break;
        default:
            break;
    }
}
#endif
# if defined(ARDUINO_ARCH_ESP32)||defined(ESP32)||defined(ARDUINO_ESP32_DEV)||defined(__ESP32__)||defined(ARDUINO_ESP32)
/*set_timer before start*/
void Timer_service_task::Set_timer(TIMER_UNIT unit, void (*onTimer)()){
    switch (unit) {
        case mili_sec:
            init_hardware_timer(1000000, 1000, onTimer);
            break;
        case mili_sec_100:
            init_hardware_timer(1000000, 100000, onTimer);
            break;
        case sec:
            init_hardware_timer(1000000, 1000000, onTimer);
            break;
        default:
            break;
    }
}
#endif
/*Stop timer when running*/
void Timer_service_task::Stop_timer(byte timer_num){
    SoftwareTimer *tempTimer = head;
    while(tempTimer != NULL){
        if(tempTimer->timer_num == timer_num){
            tempTimer->active = false;
            return;
        }
        tempTimer = tempTimer->nxt;
    }
}
/*End timer when running*/
void Timer_service_task::End_timer(byte timer_num){
    SoftwareTimer *tempTimer = head;
    while(tempTimer != NULL){
        if(tempTimer->timer_num == timer_num){
            tempTimer->end_flag = true;
            tempTimer->active = false;
            tempTimer->compare_time = global_counter;
            return;
        }
        tempTimer = tempTimer->nxt;
    }
}
/*start all timer*/
void Timer_service_task::Start_all_timer(){
    SoftwareTimer *tempTimer = head;
    while(tempTimer != NULL){
    tempTimer->active = true;
    tempTimer = tempTimer->nxt;
    }
}
/*start timer*/
void Timer_service_task::Start_timer(byte timer_num){
    SoftwareTimer *tempTimer = head;
    while(tempTimer != NULL){
    if(tempTimer->timer_num == timer_num){
        tempTimer->active = true;
        tempTimer->end_flag = false;
        return;
    }
    tempTimer = tempTimer->nxt;
    }
}
/*check timer end?*/
void Timer_service_task::Timer_check(){
    SoftwareTimer *tempTimer = head;
    while(tempTimer != NULL){
    if (tempTimer->compare_time == global_counter && tempTimer->active == true){
        tempTimer->end_flag = true;
        tempTimer->active = false;
    }
    tempTimer = tempTimer->nxt;
    }
    return;
}
/*Get timer position in list*/
int Timer_service_task::Get_timer_pos(byte timer_num){
    SoftwareTimer *temp = head;
    while(temp != NULL){
    if(temp->timer_num == timer_num){
        return temp->pos;
    }
    temp = temp->nxt;
    }
    return -1;
}
/*Change compare time of timer*/
void Timer_service_task::Change_timer(byte timer_num, unsigned long int time_change){
    SoftwareTimer *tempTimer = head;
    while(tempTimer != NULL){
    if(tempTimer->timer_num == timer_num){
        tempTimer->compare_time = time_change;
        tempTimer->active = true;
        tempTimer->end_flag = false;
        return;
    }
    tempTimer = tempTimer->nxt;
    }
}
/*Get compare time of timer*/
unsigned long int Timer_service_task::Get_compare_time(byte timer_num){
    SoftwareTimer *tempTimer = head;
    while(tempTimer != NULL){
    if(tempTimer->timer_num == timer_num)
    return tempTimer->compare_time;
    tempTimer = tempTimer->nxt;
    }
}