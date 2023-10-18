//sudo mknod /dev/mydevice c 64 0

#include <linux/module.h>
#include <linux/init.h>

//file system interuction header
#include <linux/fs.h>


#define _MODULE_NAME         "hello kernel"
#define _MODULE_NAME_TO_RP   "hello kernel: "

#define _MODULE_MAJOR_NUM    64U 


static int module_open(struct inode *p_dev_file, struct file *p_inst)
{
	pr_info(_MODULE_NAME_TO_RP "opened\n");
	return 0;
}


static int module_close(struct inode *p_dev_file, struct file *p_inst)
{
    pr_info(_MODULE_NAME_TO_RP "closed\n");
    return 0;
}

static struct file_operations fops = {
    .owner = THIS_MODULE,
    .open = module_open,
    .release = module_close
};

static int __init _module_init(void)
{
    int ret_val = 0;

    const unsigned int c_major_swift = 20, 
                       c_minor_mask = 0xfffff;

	pr_info("Hello kernel\n");

    // registrate char device
    // #include <linux/fs.h>
    // major num, short device name and fops
    ret_val = register_chrdev(_MODULE_MAJOR_NUM, _MODULE_NAME, &fops);

    if (0 == ret_val) {
    	pr_info(_MODULE_NAME_TO_RP "registered with Major: %d, Minor %d:", _MODULE_MAJOR_NUM, 0);
    } else if (ret_val > 0) {
        pr_info(_MODULE_NAME_TO_RP "registered with Major: %d, Minor %d:", 
                                    ret_val >> c_major_swift, ret_val&c_minor_mask);
    } else {
    	pr_err(_MODULE_NAME_TO_RP "could not register");
    	return -1;
    }

	return 0;
}


static void __exit _module_exit(void)
{
	unregister_chrdev(_MODULE_MAJOR_NUM, _MODULE_NAME);
	pr_info("Goodbye kernel");
}


MODULE_LICENSE("GPL");
MODULE_AUTHOR("Artem Shimko");
MODULE_DESCRIPTION("hello kernel LKM");


module_init(_module_init);
module_exit(_module_exit);