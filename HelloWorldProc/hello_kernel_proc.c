/*
 * Print a simple Hello World to a proc file 
 */

#include <linux/module.h>	/* Specifically, a module */
#include <linux/kernel.h>	/* We're doing kernel work */
#include <linux/proc_fs.h>	/* Necessary because we use the proc fs */

#define procfs_name "helloworld"

struct proc_dir_entry *hello_proc;

int procfile_read(char *buffer,
	      char **buffer_location,
	      off_t offset, int buffer_length, int *eof, void *data)
{
	int ret;
	
	printk(KERN_INFO "procfile_read (/proc/%s) called\n", procfs_name);
	
	if (offset > 0) {
		/* we have finished to read, return 0 */
		ret  = 0;
	} else {
		/* fill the buffer, return the buffer size */
		ret = sprintf(buffer, "HelloWorld!\n");
	}

	return ret;
}

int init_module()
{
	hello_proc = create_proc_entry(procfs_name, 0644, NULL);
	
	if (hello_proc == NULL) {
		remove_proc_entry(procfs_name, &proc_root);
		printk(KERN_ALERT "Error: Could not initialize /proc/%s\n",
		       procfs_name);
		return -ENOMEM;
	}

	hello_proc->read_proc = procfile_read;
	hello_proc->owner 	 = THIS_MODULE;
	hello_proc->mode 	 = S_IFREG | S_IRUGO;
	hello_proc->uid 	 = 0;
	hello_proc->gid 	 = 0;
	hello_proc->size 	 = 37;

	printk(KERN_INFO "/proc/%s created\n", procfs_name);	
	return 0;	/* everything is ok */
}

void cleanup_module()
{
	remove_proc_entry(procfs_name, &proc_root);
	printk(KERN_INFO "/proc/%s removed\n", procfs_name);
}
