/**
 * A Linux Kernel module to trace and log POSIX Message Queue System Calls
 * Utilizes Proc files to actually display logging to the user
 * By Ethan Shernan, Chris Gordon, Steven Wojcio (AKA, The Ballmers)
 */

#include <linux/kernel.h>
#include <linux/kprobes.h>
#include <linux/module.h>
#include <linux/unistd.h>
#include <linux/time.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>

MODULE_LICENSE("GPL");
#define MODULE_NAME "[logger] "
static struct kprobe probe;

typedef struct Node{
    struct Node* next;
    long unsigned int sys_call_number;
    int pid;
    int tgid;
    int time_stamp;
    char* queue_name;
    int msg_length;
    char* msg;
} node;

typedef struct Queue{
    node* head;
    node* tail;
}queue;

//Proc File Functions
static int proc_show(struct seq_file *m, void *v) {
    seq_printf(m, "Hello!\n");
    return 0;
}

static int proc_open(struct inode *inode, struct  file *file) {
    return single_open(file, proc_show, NULL);
}

//Proc file structs
static const struct file_operations my_proc_fops = {
    .owner = THIS_MODULE,
    .open = proc_open,
    .read = seq_read,
    .llseek = seq_lseek,
    .release = single_release,
};

static queue* log_queue;

node* create_node(long unsigned int sys_num, int pid, int tgid, int time_stamp, char* queue_name, int msg_length, char* msg){
    node* n = (node*) kmalloc(sizeof(node), GFP_KERNEL);
    n->sys_call_number = sys_num;
    n->pid = pid;
    n->tgid = tgid;
    n->time_stamp = time_stamp;
    n->queue_name = queue_name;
    n->msg_length = msg_length;
    n->msg = msg;
    return n;
}

void enqueue(node* n){
    if (log_queue->head == NULL){
        log_queue->head = n;
        log_queue->tail = n;
        n->next = NULL;
    }
    else{
        log_queue->tail->next = n;
        log_queue->tail = n;
	n->next = NULL;
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
    //Get time of day
    struct timeval t;
    do_gettimeofday(&t);
    node* n;

    switch (regs->ax) {
       case __NR_mkdir:
           /* NOTE!! do not dereference user-space pointers in the kernel */
           n = create_node(regs->ax, current->pid, current->tgid, (int)t.tv_sec, NULL, 0, NULL);
           enqueue(n); 
            
           printk(KERN_INFO MODULE_NAME
                   /* sycall pid tid args.. */
                   "Hey Steven! %lu %d %d args 0x%lu %d\n",
                   n->sys_call_number, n->pid, n->tgid,
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
    log_queue->head = NULL;
    log_queue->tail = NULL;
    proc_create("logger", 0, NULL, &my_proc_fops);

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

    remove_proc_entry("logger", NULL);
    unregister_kprobe(&probe);
    printk(KERN_INFO MODULE_NAME "unloaded\n");
}
