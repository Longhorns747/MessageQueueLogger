/*
 * Sample proc file implementation
 */

#include <linux/module.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>

//Function where things are actually printed to proc file
static int proc_show(struct seq_file *m, void *v) {
    //char msg[10];

    //for(int i = 0; i < 10; i++)
    //{
        //strcat(msg, "b");
    //}
    
    seq_printf(m, "Hello!\n");
    return 0;
}

static int proc_open(struct inode *inode, struct  file *file) {
    return single_open(file, proc_show, NULL);
}

static const struct file_operations my_proc_fops = {
    .owner = THIS_MODULE,
    .open = proc_open,
    .read = seq_read,
    .llseek = seq_lseek,
    .release = single_release,
};

//Need proc_create in the final implementation
static int __init my_proc_init(void) {
    proc_create("logger", 0, NULL, &my_proc_fops);
    return 0;
}

static void __exit my_proc_exit(void) {
    remove_proc_entry("logger", NULL);
}

MODULE_LICENSE("GPL");
module_init(my_proc_init);
module_exit(my_proc_exit);
