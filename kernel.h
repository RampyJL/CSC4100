#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>

//----------GLOBAL-CONSTANTS----------//
#define QUEUE_SIZE 100
#define STACK_SIZE 1024
#define MAX_STACKS 100


//----------STRUCTS----------//
//PCB Struct
struct PCB {
    uint32_t esp;
    uint32_t pid;
    uint32_t priority;
    struct PCB *next;
};
typedef struct PCB PCB_t;


//PCB Struct for the queue
struct PCBQ {
    PCB_t *head;
    PCB_t *tail;
    PCB_t pcbs[QUEUE_SIZE];
};
typedef struct PCBQ PCBQ_t;


//IDT entry
struct idt_entry {
    uint16_t base_low;
	uint16_t selector;
	uint8_t always0;
	uint8_t access;
	uint16_t base_high;
} __attribute__((packed));
typedef struct idt_entry idt_entry_t;

//
struct idtr {
    uint16_t limit;
    uint32_t base;
} __attribute__((packed));
typedef struct idtr idtr_t;

//Struct for a queue node
struct QNode {
    PCB_t key;
    struct QNode *next;
};

//----------GLOBAL-VARIABLES----------//
PCBQ_t pcb_queue;
PCB_t *running_process;
idt_entry_t idt[256];
int stack_array[MAX_STACKS][STACK_SIZE];
int num_processes = 0;
int num_stacks = 0;
int print_row = 0;
int print_col = 0;
int retval;


//----------Function-Prototypes----------//
void k_clearscr();
void print_border(int start_row, int start_col, int end_row, int end_col);
int create_process(uint32_t processEntry, uint32_t priority);
int* allocate_stack();
PCB_t *allocate_PCB();
void enqueue(PCBQ_t *queue,PCB_t *pcb);
void enqueue_priority(PCBQ_t *queue,PCB_t *pcb);
PCB_t *dequeue(PCBQ_t *queue);
void init_idt_entry(idt_entry_t *entry, uint32_t base, uint16_t selector, uint8_t access);
void init_idt();
void default_handler();
void setup_PIC();
void p1();
void p2();
void p3();
void idle();


//----------Assembly-Function-Prototypes----------//
extern void k_print(char *string, int string_length, int row, int col);
extern void dispatch();
extern void go();
extern void lidtr(idtr_t *idtr);
extern void outportb(uint16_t port, uint8_t value);
extern void init_timer_dev(int ms);