/*
	The simplest kernel module.
*/
#include <linux/module.h>	/* Needed by all modules */
#include <linux/kernel.h>	/* Needed for KERN_INFO */
#include <linux/proc_fs.h>	/* Necessary because we use the proc fs */

#define procfs_name "helloworld"

int init_module(void)
{
    printk(KERN_INFO "Hello world.\n");
    return 0;
}

void cleanup_module(void)
{
    printk(KERN_INFO "Goodbye world.\n");
}
