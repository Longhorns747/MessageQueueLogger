#include <linux/kernel.h>
#include <linux/kprobes.h>
#include <linux/module.h>
#include<linux/unistd.h>

MODULE_LICENSE("GPL");
#define MODULE_NAME "[logger] "
static struct kprobe probe;


typedef struct Node{
    struct Node* next;
    int sys_call_number;
    int pid;
    int tgid;
    int time_stamp;
} node;

typedef struct Queue{
    node* head;
    node* tail;
}queue;

static queue* log_queue;

node* create_node(){
    node* n = (node*) kmalloc(sizeof(node), GFP_KERNEL);
    /*Add data to node here*/
    return n;
}

void enqueue(){
    node* n = create_node();

    if (log_queue->head == NULL){
        log_queue->head = n;
        log_queue->tail = n;
        n->next = NULL;
    }
    else{
        log_queue->tail->next = n;
        log_queue->tail = n;
    }
}

/* pt_regs defined in include/asm-x86/ptrace.h
 * *
 * * For information associating registers with function arguments, see:
 * * arch/x86/kernel/entry_64.S
 * */
static int intercept(struct kprobe *kp, struct pt_regs *regs)
{
    int ret = 0;
    /*if (current_uid() != uid)
           return 0;*/
    switch (regs->ax) {
       case __NR_mkdir:
        /* NOTE!! do not dereference user-space pointers in the kernel */
           printk(KERN_INFO MODULE_NAME
                   /* sycall pid tid args.. */
                   "Hey Steven! %lu %d %d args 0x%lu %d\n",
                   regs->ax, current->pid, current->tgid,
                   (uintptr_t)regs->di, (int)regs->si);
           break;
       default:
           ret = -1;
           break;
    }
    return ret;
}
int init_module(void)
{

    log_queue = (queue*) kmalloc(sizeof(queue), GFP_KERNEL);

    probe.symbol_name = "sys_mkdir";
    probe.pre_handler = intercept; /* called prior to function */
    if (register_kprobe(&probe)) {
           printk(KERN_ERR MODULE_NAME "register_kprobe failed\n");
           return -EFAULT;
    }
    printk(KERN_INFO MODULE_NAME "loaded\n");
    return 0;
}
void cleanup_module(void)
{
    while(log_queue->head != NULL){
        node* temp = log_queue->head->next;
        kfree(log_queue->head);
        log_queue->head = temp;
    }
    kfree(log_queue);

    unregister_kprobe(&probe);
    printk(KERN_INFO MODULE_NAME "unloaded\n");
}
