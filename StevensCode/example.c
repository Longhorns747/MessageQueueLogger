#include <linux/kernel.h>
#include <linux/kprobes.h>
#include <linux/module.h>
#include <linux/unistd.h>
#include <linux/time.h>
#include <asm/uaccess.h>

MODULE_LICENSE("GPL");
#define MODULE_NAME "[logger] "
static struct kprobe open_probe;
static struct kprobe receive_probe;
static struct kprobe send_probe;

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
    int mlength = regs->dx;
    char *message = kmalloc(sizeof(void*), GFP_KERNEL);
    char *name;
    int i;
    node* n;
    
    //Get time of day
    struct timeval t;
    do_gettimeofday(&t);


    switch (regs->ax) {
      case __NR_mq_open:
	  name = kmalloc(sizeof(void*), GFP_KERNEL);
	  copy_from_user(name, (char *)regs->di, strnlen_user((char *)regs->di, 255));
	  create_node(regs->ax, current->pid, current->tgid, (int)t.tv_sec, name, 0, NULL); 
	  enqueue(n);

	  kfree(name);
          break;

      case __NR_mq_timedsend:
      case __NR_mq_timedreceive:
          /*determine if message is a string by checking if each byte
          *is a printable character, and that the last byte is null
          *
          */
	  copy_from_user(message, (char *)regs->si, mlength);
	  for(i = 0; i < mlength; i++){
		if(i < mlength -1){
			if(*(message + i) < 32 || *(message + i) > 127){
				strcpy(message, "(bin)");
				break; 
			}
		}else{
			if(*(message + i) != 0){
				strcpy(message, "(bin)");
			}
		}
	  }

	  create_node(regs->ax, current->pid, current->tgid, (int)t.tv_sec, NULL, mlength, message); 
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

    log_queue = (queue*) kmalloc(sizeof(queue), GFP_KERNEL);

    log_queue->head = NULL;
    log_queue->tail = NULL;

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

    unregister_kprobe(&open_probe);
    unregister_kprobe(&send_probe);
    unregister_kprobe(&receive_probe);
    printk(KERN_INFO MODULE_NAME "unloaded\n");
}
