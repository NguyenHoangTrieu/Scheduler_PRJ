#include "task_linked_list.h"
using namespace std;
Task::Task(int pos, byte task_num, TaskType type, int base_priority, unsigned int 
            execute_time, unsigned long int release_time, int terminate_num, void (*task_act)(void)){
    this->task_num = task_num;
    this->pos = pos;
    this->base_priority = base_priority;
    this->execute_time = execute_time;
    this->execute_time_left = execute_time;
    this->priority = base_priority;
    if(type == INSTANT_EXECUTE) this->state = SUSPENDED;
    else this->state = READY;
    this->prv = NULL;
    this->nxt = NULL;
    this->type = type;
    this->task_act = task_act;
    if(type == PERIODIC || type == UNINF_PERIODIC) this->release_time = release_time;
    else this->release_time = 0;
    if (type == UNINF_PERIODIC) this->terminate_num = terminate_num;
    else this->terminate_num = 0;
}
void LinkedList::updatePosition() {
    Task* temp = head;
    int newPos = 1;
    while (temp != NULL){
        temp->pos = newPos;
        newPos++;
        temp = temp->nxt;
    }
}
/*Add a new task to the list*/
void LinkedList::addNewTask(int pos, byte task_num, TaskType type, int base_priority,
                            unsigned int execute_time, unsigned long int release_time, int terminate_num, void (*task_act)(void)) {
    Task* newTask = new Task(pos, task_num, type, base_priority, execute_time, release_time, terminate_num, task_act);
    // list chưa có task
    if (head == NULL) {
        head = newTask;
        newTask->nxt = NULL;
        newTask->prv = NULL;
        size++;
        return;
    }
    // Chèn vào đầu list
    else if (pos == 1) {
        newTask->nxt = head;
        newTask->prv = NULL;
        head->prv = newTask;
        head = newTask;
        size++;
        updatePosition();
        return;
    }
    // Chèn vào cuối list
    if (pos >= size + 1) {
        Task* temp = head;
        while (temp->nxt != NULL) {
            temp = temp->nxt;
        }
        newTask->prv = temp;
        newTask->nxt = NULL;
        temp->nxt = newTask;
        size++;
        updatePosition();
        return;
    }
    // các vị trí khác
    else {
        int count = 1;
        Task* temp = head;
        while (count < pos) {
            temp = temp->nxt;
            count++;
        }
        newTask->nxt = temp;
        newTask->prv = temp->prv;
        newTask->prv->nxt = newTask;
        temp->prv = newTask;
        size++;
    }
    updatePosition();
}
/*Delete a task from the list*/
void LinkedList::deleteTask(int pos) {
    if (pos != -1) {
        Task* temp = head;
        // Xóa task nếu chỉ có 1 task trong list
        if (size == 1) {
            head = NULL;
            size--;
            free(temp);
            return;
        }
        // Xoá task đầu
        else if (pos == 1) {
            temp->nxt->prv = NULL;
            head = temp->nxt;
            size--;
            free(temp);
            updatePosition();
            return;
        }
        // Xoá task ở các vị trí khác
        else if (pos <= size) {
            int count = 1;
            while (count < pos) {
                temp = temp->nxt;
                count++;
            }
            temp->prv->nxt = temp->nxt;
            if (pos < size) temp->nxt->prv = temp->prv;
            size--;
            free(temp);
        }
        updatePosition();
    }
}

/*Get the position of a task in the list*/
int LinkedList::getTaskPos(byte task_num) {
    Task* temp = head;
    while (temp != NULL) {
        if (temp->task_num == task_num) {
            return temp->pos;
        }
        temp = temp->nxt;
    }
    return -1; // Task not found
}

/*Returns as an enumerated type the state in which a task existed 
at the time this function was executed.*/
TaskState LinkedList::Get_Task_State(byte task_num) {
    Task* temp = head;
    while (temp != NULL) {
        if (temp->task_num == task_num) {
            return temp->state;
        }
        temp = temp->nxt;
    }
    return DELETED; // Task not found
}

/*Set the state of any task.*/
void LinkedList::Set_Task_State(byte task_num, TaskState state) {
    Task* temp = head;
    while (temp != NULL) {
        if (temp->task_num == task_num) {
            temp->state = state;
            return;
        }
        temp = temp->nxt;
    }
}

/*Get number of tasks*/
int LinkedList::Number_of_Tasks() {
    Task* temp = head;
    int count = 0;
    while (temp != NULL) {
        count++;
        temp = temp->nxt;
    }
    return count;
}

/*Obtain the priority of any task.*/
int LinkedList::getTaskPriority(byte task_num) {
    Task* temp = head;
    while (temp != NULL) {
        if (temp->task_num == task_num) {
            return temp->priority;
        }
        temp = temp->nxt;
    }
    return -1; // Task not found
}

/*Set the priority of any task.*/
void LinkedList::setTaskPriority(byte task_num, int priority) {
    Task* temp = head;
    while (temp != NULL) {
        if (temp->task_num == task_num) {
            temp->priority = priority;
            return;
        }
        temp = temp->nxt;
    }
}

/*Obtain the base priority of any task. 
The base priority of a task is the priority to which the task will return 
if the task's current priority has been inherited to avoid 
unbounded priority inversion when obtaining a mutex.*/
int LinkedList::getTaskBasePriority(byte task_num) {
    Task* temp = head;
    while (temp != NULL) {
        if (temp->task_num == task_num) {
            return temp->base_priority;
        }
        temp = temp->nxt;
    }
    return -1; // Task not found
}
/*Get task execute time.*/
int LinkedList::getTaskExecuteTime(byte task_num) {
    Task* temp = head;
    while (temp != NULL) {
        if (temp->task_num == task_num) {
            return temp->execute_time;
        }
        temp = temp->nxt;
    }
    return -1; // Task not found
}
/*Set task execute time.*/
void LinkedList::setTaskExecuteTime(byte task_num, unsigned int execute_time) {
    Task* temp = head;
    while (temp != NULL) {
        if (temp->task_num == task_num) {
            temp->execute_time = execute_time;
            temp->execute_time_left = execute_time;
            return;
        }
        temp = temp->nxt;
    }
}
/*Get task type.*/
TaskType LinkedList::getTaskType(byte task_num) {
    Task* temp = head;
    while (temp != NULL) {
        if (temp->task_num == task_num) {
            return temp->type;
        }
        temp = temp->nxt;
    }
    return NOT_FOUND; // Task not found
}
/*Change task type.*/
void LinkedList::changeTaskType(byte task_num, TaskType type) {
    Task* temp = head;
    while (temp != NULL) {
        if (temp->task_num == task_num) {
            temp->type = type;
            return;
        }
        temp = temp->nxt;
    }
}
/*Set task release time.*/
void LinkedList::setTaskReleaseTime(byte task_num, unsigned long int release_time) {
    Task* temp = head;
    while (temp != NULL) {
        if (temp->task_num == task_num) {
            temp->release_time = release_time;
            return;
        }
        temp = temp->nxt;
    }
}
/*Get task release time.*/
int LinkedList::getTaskReleaseTime(byte task_num) {
    Task* temp = head;
    while (temp != NULL) {
        if (temp->task_num == task_num) {
            return temp->release_time;
        }
        temp = temp->nxt;
    }
    return -1;
}
/*Update the task in the list*/
void LinkedList::fullTaskUpdate(byte task_num, TaskType type, int priority, 
                                unsigned int execute_time, unsigned long int release_time, int terminate_num) {
    Task* temp = head;
    while (temp != NULL) {
        if (temp->task_num == task_num) {
            temp->type = type;
            temp->priority = priority;
            temp->execute_time = execute_time;
            temp->execute_time_left = execute_time;
            temp->release_time = release_time;
            temp->terminate_num = terminate_num;
            return;
        }
        temp = temp->nxt;
    }
}
/*Copy the task from listA to listB*/
void copyTask(LinkedList &listA, LinkedList &listB, byte task_num){
    Task* temp = listA.head;
    while (temp != NULL) {
        if (temp->task_num == task_num) {
            if(listB.getTaskPos(task_num) == -1){
            int listB_pos = listB.Number_of_Tasks() + 1;
            listB.addNewTask(listB_pos, temp->task_num, temp->type, temp->base_priority, 
                                temp->execute_time, temp->release_time, temp->terminate_num, temp->task_act);
            return;
            }
            else if(listB.getTaskPos(task_num) != -1){
                listB.fullTaskUpdate(task_num, temp->type, temp->priority, 
                                    temp->execute_time, temp->terminate_num, temp->release_time);
                return;
            }
        }
        temp = temp->nxt;
    }
}
