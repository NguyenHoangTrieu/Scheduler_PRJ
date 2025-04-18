  #include"RTC_Scheduler.h"
  #define PIN8 (1 << PB0)
  #define PIN10 (1 << PB2)
  #define LED_1 (1 << PD2)
  #define LED_2 (1 << PD3)
  #define DEBOUNCE_DELAY 10
  #define OFF 0
  #define ON 1
  #define BLINK 2
  volatile int countChanges = 0;
  volatile bool pin8State = HIGH;
  volatile bool PIN10State = HIGH;
  struct Task_Communication{
    volatile uint8_t Active_LED;
    volatile uint8_t LED1_blink_speed;
    volatile uint8_t LED2_blink_speed;
    volatile uint8_t button_state;
    volatile bool event_handler;
    volatile uint8_t Led1_state;
    volatile uint8_t Led2_state;
  };
  Task_Communication task_communication;
  void setup() {
    DDRD |= LED_1 | LED_2;
    DDRB &= ~PIN8;
    PORTB |= PIN8|PIN10;
    PCICR |= (1 << PCIE0);
    PCMSK0 |= (1 << PCINT0) | (1 << PCINT2);
    sei();
    Serial.begin(115200);
    RTC_scheduler.Task_Create(0x41, PERIODIC, 0, 10000, 0, 0, Led_1_Control);
    RTC_scheduler.Task_Create(0x42, PERIODIC, 0, 10000, 0, 0, Led_2_Control);
    RTC_scheduler.Task_Create(0x43, INSTANT_EXECUTE, 0, 2000, 0, 0, Button_Add_Check);
    RTC_scheduler.Task_Create(0x44, INSTANT_EXECUTE, 0, 2000, 0, 0, Button_Sub_Check);
    RTC_scheduler.Task_Create(0x45, INSTANT_EXECUTE, 0, 3000, 0, 0, Two_Buttons_Check);
    RTC_scheduler.Task_Create(0x46, INSTANT_EXECUTE, 0, 50, 0, 0, PIN_check);
    task_communication.Active_LED = 1;
    task_communication.Led1_state = BLINK;
    task_communication.Led2_state = OFF;
    task_communication.LED1_blink_speed = 50;
    task_communication.LED2_blink_speed = 50;
    RTC_scheduler.Stop_Task(0x42);
    RTC_scheduler.Begin_Setup(mili_sec);
  }

  void Button_Add_Check() {
      static volatile unsigned long press_start_time = 0;
      static volatile bool button_pressed = false;
      static volatile bool end_event_flag = false;
      if (!(PINB & PIN8) && end_event_flag == false){
        if (!button_pressed) {
            press_start_time = timer_service_task.global_counter;
            button_pressed = true;
        }
        if ((timer_service_task.global_counter - press_start_time) >= 1999) end_event_flag = true;
      } 
      else {
        if (end_event_flag == true || button_pressed){
          if (task_communication.Led1_state != BLINK) task_communication.Led1_state = BLINK;
          if (task_communication.Led2_state != BLINK) task_communication.Led2_state = BLINK;
          unsigned volatile long press_duration = timer_service_task.global_counter - press_start_time;
          button_pressed = false;
          end_event_flag = false;
          Serial.print("Add Button Press Duration: ");
          Serial.println(press_duration);
          RTC_scheduler.Stop_Task(0x43);
          if (press_duration < 1900){
            if(task_communication.Active_LED == 1){
              task_communication.LED1_blink_speed--;
              if(task_communication.LED1_blink_speed == 0) task_communication.LED1_blink_speed = 1;
            }
            if(task_communication.Active_LED == 2){
              task_communication.LED2_blink_speed--;
              if(task_communication.LED2_blink_speed == 0) task_communication.LED2_blink_speed = 1;
            }
          }
          else{
            if(task_communication.Active_LED == 1) task_communication.Led1_state = ON;
            if(task_communication.Active_LED == 2) task_communication.Led2_state = ON;
          }
        }
        else{
          RTC_scheduler.Stop_Task(0x43);
          end_event_flag = false;
          button_pressed = false;
        }
    }
  }

  void Button_Sub_Check(){
      static volatile unsigned long press_start_time = 0;
      static volatile bool button_pressed = false;
      static volatile bool end_event_flag = false;
      if (!(PINB & PIN10) && end_event_flag == false){
        if (!button_pressed) {
            press_start_time = timer_service_task.global_counter;
            button_pressed = true;
        }
        if ((timer_service_task.global_counter - press_start_time) >= 1999) end_event_flag = true;
      }
      else {
          if (end_event_flag == true || button_pressed){
            if (task_communication.Led1_state != BLINK) task_communication.Led1_state = BLINK;
            if (task_communication.Led2_state != BLINK) task_communication.Led2_state = BLINK;
            unsigned long press_duration = timer_service_task.global_counter - press_start_time;
            button_pressed = false;
            end_event_flag = false;
            Serial.print("Sub Button Press Duration: ");
            Serial.println(press_duration);
            RTC_scheduler.Stop_Task(0x44);
            if (press_duration < 1900){
                if(task_communication.Active_LED == 1){
                    task_communication.LED1_blink_speed++;
                    if(task_communication.LED1_blink_speed > 250) task_communication.LED1_blink_speed = 250;
                }
                if(task_communication.Active_LED == 2){
                    task_communication.LED2_blink_speed++;
                    if(task_communication.LED2_blink_speed > 250) task_communication.LED2_blink_speed = 250;
                }
            }
            else{
                if(task_communication.Active_LED == 1) task_communication.Led1_state = OFF;
                if(task_communication.Active_LED == 2) task_communication.Led2_state = OFF;
            } 
          }
          else{
            RTC_scheduler.Stop_Task(0x44);
            end_event_flag = false;
            button_pressed = false;
          }
      }
  }

  void Two_Buttons_Check(){
      static volatile unsigned long press_start_time = 0;
      static volatile bool button_pressed = false;
      static volatile bool end_event_flag = false;
      if (!(PINB & PIN8) && !(PINB & PIN10) && end_event_flag == false) {
        if (!button_pressed) {
            press_start_time = timer_service_task.global_counter;
            button_pressed = true;
        }
        if ((timer_service_task.global_counter - press_start_time) >= 2999) end_event_flag = true;
      } 
      else {
          if (end_event_flag == true || button_pressed){
            unsigned long press_duration = timer_service_task.global_counter - press_start_time;
            button_pressed = false;
            end_event_flag = false;
            Serial.print("2 Button Press Duration: ");
            Serial.println(press_duration);
            RTC_scheduler.Stop_Task(0x45);
            if (press_duration >= 2900) {
              if(task_communication.Active_LED == 1){
                  task_communication.Active_LED = 2;
                  task_communication.Led1_state = OFF;
                  PORTD &= ~LED_1;
                  RTC_scheduler.Stop_Task(0x41);
                  RTC_scheduler.Task_Ready(0x42);
                  RTC_scheduler.current_execute_task = 0x42;
              }
              else if(task_communication.Active_LED == 2){
                  task_communication.Active_LED = 1;
                  task_communication.Led2_state = OFF;
                  PORTD &= ~LED_2;
                  RTC_scheduler.Stop_Task(0x42);
                  RTC_scheduler.Task_Ready(0x41);
                  RTC_scheduler.current_execute_task = 0x41;

              }
            }
          }
          else{
            RTC_scheduler.Stop_Task(0x45);
            end_event_flag = false;
            button_pressed = false;
          }
      }
  }

  void PIN_check() {
      static volatile unsigned long lastDebounceTime;
      static volatile bool function_called;
      if(!(function_called)){
          lastDebounceTime = timer_service_task.global_counter;
          function_called = true;
      }
          if (!(PINB & PIN8)) task_communication.button_state |= 1;
          if (!(PINB & PIN10)) task_communication.button_state |= 2;
      if (task_communication.button_state != 0 && (timer_service_task.global_counter - lastDebounceTime >= 49)){
          lastDebounceTime = timer_service_task.global_counter;
          function_called = false;
          if(task_communication.button_state == 1)
            RTC_scheduler.Instant_Event_handler(0x43);
          else if(task_communication.button_state == 2)
            RTC_scheduler.Instant_Event_handler(0x44);
          else if(task_communication.button_state == 3)
            RTC_scheduler.Instant_Event_handler(0x45);
          task_communication.button_state = 0;
      }
  }
  ISR(PCINT0_vect) {
    PORTB_event_flag = true;
  }
  void RTC_Scheduler::PORTB_event_handler(){
    if(task_communication.event_handler == false && (!(PINB & PIN8) || !(PINB & PIN10))){
      PORTB_event_flag = false;
      task_communication.event_handler = true;
      RTC_scheduler.Instant_Event_handler(0x46);
    }
  }
  void RTC_Scheduler::Timer_event_set(){}
  void Led_1_Control(){
    task_communication.event_handler = false;
    static unsigned long lastToggleTime1 = 0;
    unsigned long currentTime = timer_service_task.global_counter;
    switch(task_communication.Led1_state){
      case OFF:
        PORTD &= ~LED_1;
        break;
      case ON:
        PORTD |= LED_1;
        break;
      case BLINK:
        unsigned long blinkInterval = task_communication.LED1_blink_speed * 10;
        if ((currentTime - lastToggleTime1) >= blinkInterval) {
            Serial.print("LED 1 BLINK SPEED: ");
            Serial.println(task_communication.LED1_blink_speed);
            lastToggleTime1 = currentTime;
            PORTD ^= LED_1;
        }
        break;
    }
  }
  void Led_2_Control(){
    task_communication.event_handler = false;
    static unsigned long lastToggleTime2 = 0;
    unsigned long currentTime = timer_service_task.global_counter; 
    switch(task_communication.Led2_state){
      case OFF:
        PORTD &= ~LED_2;
        break;
      case ON:
        PORTD |= LED_2;
        break;
      case BLINK:
        unsigned long blinkInterval = task_communication.LED2_blink_speed * 10;
        if ((currentTime - lastToggleTime2) >= blinkInterval) {
            Serial.print("LED 2 BLINK SPEED: ");
            Serial.println(task_communication.LED2_blink_speed);
            lastToggleTime2 = currentTime;
            PORTD ^= LED_2;
        }
        break;
    }
  }
  void loop(){
    RTC_scheduler.Task_execute();
  }
int main(void)
{
  HAL_Init();
  SystemClock_Config();
  MX_GPIO_Init();
  MX_USART2_UART_Init();
  volatile uint8_t data[20] = "DITCONMEMAY\r\n";
  volatile uint8_t rec;
  while (1)
  {
    if (HAL_UART_Receive(&huart2, rec, 1, 1000) == HAL_OK)
    {
      HAL_UART_Transmit(&huart2, data, 20, 1000);
    }
  }
}