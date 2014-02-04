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
#include <linux/mutex.h>
#include <asm/uaccess.h>

MODULE_LICENSE("GPL");
#define MODULE_NAME "[logger] "

static struct kprobe open_probe;
static struct kprobe receive_probe;
static struct kprobe send_probe;
struct mutex queue_lock;

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
    mutex_lock(&queue_lock);
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
    mutex_unlock(&queue_lock);
}

//Proc File Functions
static int proc_show(struct seq_file *m, void *v) {
    node* curr_node = log_queue->head;

    while(curr_node != NULL)
    {
        seq_printf(m, "sys_call_number: %lu, ", curr_node->sys_call_number);
        seq_printf(m, "pid: %d, ", curr_node->pid);
        seq_printf(m, "tgid: %d, ", curr_node->tgid);
        seq_printf(m, "time_stamp: %d, ", curr_node->time_stamp);
        seq_printf(m, "msg: %s, ", curr_node->msg);
        seq_printf(m, "msg_len: %d\n", curr_node->msg_length);
        curr_node = curr_node->next;
    } 
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

/* pt_regs defined in include/asm-x86/ptrace.h
 * *
 * * For information associating registers with function arguments, see:
 * * arch/x86/kernel/entry_64.S
 * */
static int intercept(struct kprobe *kp, struct pt_regs *regs)
{
    int ret = 0;
    int mlength = regs->dx;
    char *message = kmalloc(sizeof(void*)*(mlength+1), GFP_KERNEL);
    char *name;
    int i;
    node* n;
    
    //Get time of day
    struct timeval t;
    do_gettimeofday(&t);

    switch (regs->ax) {
      case __NR_mq_open:
	    name = kmalloc(sizeof(void*) * (strnlen_user((char *)regs->di, 255)), GFP_KERNEL);
	    if(copy_from_user(name, (char *)regs->di, strnlen_user((char *)regs->di, 255))){
		return -EFAULT;
	    }
	    n = create_node(regs->ax, current->pid, current->tgid, (int)t.tv_sec, name, 0, NULL); 
	    enqueue(n);

	    kfree(name);
      break;

      case __NR_mq_timedsend:
      case __NR_mq_timedreceive:
          /*determine if message is a string by checking if each byte
          *is a printable character, and that the last byte is null
          *
          */
	  if (copy_from_user(message, (char *)regs->si, mlength)){
		return -EFAULT;
	  }
	  printk("%s\n",message);
	  for(i = 0; i < mlength; i++){
		if(i < mlength -1){
			if(*(message + i) < 32 || *(message + i) > 127){
				strcpy(message, "(bin)");
				break; 
			}
		}else{
			if(*(message + i) != '\0'){
				strcpy(message, "(bin)");
			}
		}
	  }
	    n = create_node(regs->ax, current->pid, current->tgid, (int)t.tv_sec, NULL, mlength, message); 
	    enqueue(n);

	    kfree(message);
      break;

      default:
          ret = -1;
          break;
    }
    return ret;
}

int init_module(void)
{
    mutex_init(&queue_lock);
    log_queue = (queue*) kmalloc(sizeof(queue), GFP_KERNEL);
    log_queue->head = NULL;
    log_queue->tail = NULL;
    proc_create("logger", 0, NULL, &my_proc_fops);

    open_probe.symbol_name = "sys_mq_open";
    open_probe.pre_handler = intercept; 
    send_probe.symbol_name = "sys_mq_timedsend";
    send_probe.pre_handler = intercept;
    receive_probe.symbol_name = "sys_mq_timedreceive";
    receive_probe.pre_handler = intercept;

    if (register_kprobe(&open_probe)) {
           printk(KERN_ERR MODULE_NAME "register_kprobe failed\n");
           return -EFAULT;
    }
    if (register_kprobe(&send_probe)) {
           printk(KERN_ERR MODULE_NAME "register_kprobe failed\n");
           return -EFAULT;
    }
    if (register_kprobe(&receive_probe)) {
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
    unregister_kprobe(&open_probe);
    unregister_kprobe(&send_probe);
    unregister_kprobe(&receive_probe);
    printk(KERN_INFO MODULE_NAME "unloaded\n");
}
