nclude <linux/kernel.h>
#include <linux/kprobes.h>
#include <linux/module.h>
MODULE_LICENSE("GPL");
#define MODULE_NAME "[logger] "
static struct kprobe probe;
/* pt_regs defined in include/asm-x86/ptrace.h
*
* For information associating registers with function arguments, see:
* arch/x86/kernel/entry_64.S
*/
static int intercept(struct kprobe *kp, struct pt_regs *regs)
{
    int ret = 0;
    if (current->uid != uid)
           return 0;
    switch (regs->rax) {
       case __NR_mkdir:
        /* NOTE!! do not dereference user-space pointers in the kernel */
           printk(KERN_INFO MODULE_NAME
                   /* sycall pid tid args.. */
                   "%lu %d %d args 0x%lu %d\n",
                   regs->rax, current->pid, current->tgid,
                   (uintptr_t)regs->rdi, (int)regs->rsi);
           break;
       default:
           ret = -1;
           break;
    }
    return ret;
}
int init_module(void)
{
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
    unregister_kprobe(&probe);
    printk(KERN_INFO MODULE_NAME "unloaded\n");
}
