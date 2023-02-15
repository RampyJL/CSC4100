#include "kernel.h"

//----------MAIN-FUNCTION-----------//
int main (int argc, char *argv[]) { 
    
    pcb_queue.head = 0;
    pcb_queue.tail = 0;

    k_clearscr();
    print_border(0, 0, 24, 79);
    k_print("Running Processess...", 21, 1, 1);

    init_idt();

    init_timer_dev(50);
    setup_PIC();

    retval = create_process((u_int32_t)idle, 5);
    if (retval != 0) {
        default_handler();
    }
    retval = create_process((u_int32_t)idle, 5);
    if (retval != 0) {
        default_handler();
    }
    retval = create_process((u_int32_t)p1, 10);
    if (retval != 0) {
        default_handler();
    }
    retval = create_process((u_int32_t)p2, 10);
    if (retval != 0) {
        default_handler();
    }
    retval = create_process((u_int32_t)p3, 12);
    if (retval != 0) {
        default_handler();
    }

    go();

    while(1);
    return 0;
 }




//----------FUNCTION-BODIES-----------//
// Clear screen function
void k_clearscr() {   
    for(int x = 0; x < 25; x++) {
        for(int y = 0; y < 80; y++) {
            k_print(" ", 1, x, y);
        }
    }
}

// Prints a border along a givin row and column tuple 
void print_border(int start_row, int start_col, int end_row, int end_col) {
    int x = start_row;
    int y = start_col;
    int x_apex = end_row;
    int y_apex = end_col;

    while (x <= x_apex) {
        while (y <= y_apex) {
            if((x == start_row && y == start_col) || (x == start_row && y == y_apex) || (x == x_apex && y == start_col) || (x == x_apex && y == y_apex)) {
                k_print("+", 1, x, y);
            }
            else if (x == start_row || x == x_apex)
            {
                k_print("-", 1, x, y);
            }
            else if (y == start_col || y == y_apex)
            {
                k_print("|", 1, x, y);
            }
            else {
                k_print(" ", 1, x, y);
            }
            y++;
        }
        y = start_col;
        x++;
    }
}

int create_process(uint32_t processEntry, uint32_t priority) {
    if(num_processes == QUEUE_SIZE || num_stacks == MAX_STACKS) {
        return -1;
    }
    int *stack_pointer = allocate_stack();
    uint32_t *st = stack_pointer + STACK_SIZE;
    
    //Initializing stack
    st--;
    *st = (uint32_t)go;
    st--;
    *st = 0x200;
    st--;
    *st = 16;
    st--;
    *st = processEntry;
    //Setting e-flags to 0
    for(int i = 0; i < 8; i++) {
        st--;
        *st = 0;
    }
    //Setting ds, es, fs, and gs to 8
    for(int i = 0; i < 4; i++) {
        st--;
        *st = 8;
    }

    PCB_t *new_pcb = allocate_PCB();
    new_pcb->esp = (uint32_t)st;
    new_pcb->pid = num_processes++;
    new_pcb->priority = priority;
    enqueue_priority(&pcb_queue, new_pcb);

    return 0;
}

//Allocates a stack from the stack_array
int *allocate_stack() {
    num_stacks++;
    return stack_array[num_stacks-1];
}

//Allocates PCB from the queue array
PCB_t *allocate_PCB() {
    return &pcb_queue.pcbs[num_processes];
}

//Enqueue function to our PCB queue
void enqueue(PCBQ_t *queue, PCB_t *pcb) {
    
    if(queue->tail == NULL) {
        queue->head = pcb;
        queue->tail = pcb;
    }
    else {
        queue->tail->next = pcb;
        queue->tail = pcb;
    }
    
}

//Enqueue function to our PCB queue
void enqueue_priority(PCBQ_t *queue, PCB_t *pcb) {
    
    if(queue->tail == NULL) {
        queue->head = pcb;
        queue->tail = pcb;
    }
    else {
        if(queue->tail->priority >= pcb->priority) {
            queue->tail->next = pcb;
            queue->tail = pcb;
        }
        else {
            PCB_t *temp = queue->head;
            PCB_t *prev;

            if(temp->priority < pcb->priority) {
                pcb->next = temp;
                queue->head = pcb;
            }
            else {
                int done = 0;
                while(done == 0) {
                    if(temp->next->priority < pcb->priority) {
                        pcb->next = temp->next;
                        temp->next = pcb;
                        done = 1;
                    }
                    else {
                        temp = temp->next;
                    }
                }
            }
    
        }
    }
    
}

//Dequeue function from our PCB queue
PCB_t *dequeue(PCBQ_t *queue) {
    if(queue->head == NULL) {
        k_print("ERROR: QUEUE EMPTY", 18, 20, 1);
    }
    PCB_t *temp = queue->head;
    queue->head = queue->head->next;
    return temp;
}

//Makes an entry into the IDT
void init_idt_entry(idt_entry_t *entry, uint32_t base, uint16_t selector, uint8_t access) {
	entry->base_low = (uint16_t) (base & 0x0000FFFF);
    entry->selector = selector;
	entry->always0 = 0;
    entry->access = access;
    entry->base_high = (uint16_t) (0x0000FFFF & (base >> 16));
}

//Initialize IDT with system interrups
void init_idt() {
    for(int i = 0; i < 32; i++) {
        init_idt_entry(idt + i, (u_int32_t)default_handler, 16, 0x8e);
    }

    init_idt_entry(idt + 32, (u_int32_t)dispatch, 16, 0x8e);

    for(int i = 33; i < 256; i++) {
        init_idt_entry(idt + i, 0, 0, 0);
    }

    idtr_t *idtr;
    idtr->limit = sizeof(idt)-1;
	idtr->base = (uint32_t)idt;
    lidtr(idtr);
}

//Default handler for interrupts 0-31
void default_handler() {
    k_print("ERROR", 5, 19, 1);
}

void setup_PIC() {
    // set up cascading mode:
    outportb(0x20, 0x11); // start 8259 master initialization
    outportb(0xA0, 0x11); // start 8259 slave initialization
    outportb(0x21, 0x20); // set master base interrupt vector (idt 32-38)
    outportb(0xA1, 0x28); // set slave base interrupt vector (idt 39-45)
    // Tell the master that he has a slave:
    outportb(0x21, 0x04); // set cascade ...
    outportb(0xA1, 0x02); // on IRQ2
    // Enabled 8086 mode:
    outportb(0x21, 0x01); // finish 8259 initialization
    outportb(0xA1, 0x01);
    // Reset the IRQ masks
    outportb(0x21, 0x0);
    outportb(0xA1, 0x0);
    // Now, enable the clock IRQ only 
    outportb(0x21, 0xfe); // Turn on the clock IRQ
    outportb(0xA1, 0xff); // Turn off all others
}

void p1() {
    int i = 0;
    char buffer[2];
    k_print("Process 1 running: ", 19, 5, 1);
    for(int x = 0; x < INT_MAX/100; x++) {
        buffer[0] = i + '0';
        k_print(buffer, 1, 5, 21);
        i = ((i+1) % 10);
    }
}

void p2() {
    int i = 0;
    char buffer[2];
    k_print("Process 2 running: ", 19, 7, 1);
    for(int x = 0; x < INT_MAX/100; x++) {
        buffer[0] = i + '0';
        k_print(buffer, 1, 7, 21);
        i = ((i+1) % 10);
    }
}

void p3() {
    int i = 0;
    char buffer[2];
    k_print("Process 3 running: ", 19, 9, 1);
    for(int x = 0; x < INT_MAX/100; x++) {
        buffer[0] = i + '0';
        k_print(buffer, 1, 9, 21);
        i = ((i+1) % 10);
    }
}

void idle() {
    k_print("Process idle running: ", 22, 23, 1);
    while(1) {
        k_print("|", 1, 23, 23);
        k_print("/", 1, 23, 23);
        k_print("-", 1, 23, 23);
        k_print("\\", 1, 23, 23);
    }
}

