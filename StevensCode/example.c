#include <linux/kernel.h>
#include <linux/kprobes.h>
#include <linux/module.h>
#include<linux/unistd.h>
MODULE_LICENSE("GPL");
#define MODULE_NAME "[logger] "
static struct kprobe open_probe;
static struct kprobe receive_probe;
static struct kprobe send_probe;
/* pt_regs defined in include/asm-x86/ptrace.h
*
* For information associating registers with function arguments, see:
* arch/x86/kernel/entry_64.S
*/
static int intercept(struct kprobe *kp, struct pt_regs *regs)
{
    int ret = 0;
    
    /*if (current_uid() != uid)
           return 0;*/
    switch (regs->ax) {
       case __NR_mq_open:
	   printk(KERN_INFO MODULE_NAME
		   "%lu %d %d %d name: %s",
		   regs->ax, current->pid, current->tgid,
		   current->timestamp, regs->di);
	   break;
       case __NR_mq_timedsend:
       case __NR_mq_timedreceive:
	   /*determine if message is a string by checking if each byte
 	    *is a printable character, and that the last byte is null
	    *
	    */
	   printk(KERN_INFO MODULE_NAME
		   "%lu %d %d %d name: %s",
		   regs->ax, current->pid, current->tgid,
		   current->timestamp, regs->di);
	   break;
       default:
           ret = -1;
           break;
    }
    return ret;
}
int init_module(void)
{
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

    unregister_kprobe(&open_probe);
    unregister_kprobe(&send_probe);
    unregister_kprobe(&receive_probe);
    printk(KERN_INFO MODULE_NAME "unloaded\n");
}
