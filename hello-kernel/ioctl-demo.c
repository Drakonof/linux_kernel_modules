//sudo mknod /dev/dummy c 64 0

#include <linux/module.h>
#include <linux/init.h>
#include <linux/fs.h>
#include <linux/ioctl.h>
#include <linux/uaccess.h>

#include <linux/string.h>

#include "ioctl-demo.h"

#define _MODULE_NAME         "ioctl-demo"
#define _MODULE_NAME_TO_RP   "ioctl-demo: "

#define _MODULE_CLASS        "ioctl_demo_class"

#define _MODULE_MAJOR         64U

int32_t answer = 42;

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

static long int module_ioctl(struct file *p_file, unsigned int cmd, unsigned long int arg)
{
	struct demo_struct demo;
    memset(&demo, 0, sizeof(struct demo_struct));

	switch (cmd) {

	case WR_VALUE:
		if (copy_from_user(&answer, (int32_t *) arg, sizeof(answer))) {
			pr_err(_MODULE_NAME_TO_RP "WR_VALUE copy_from_user failed\n");
		}
		else {
			pr_info(_MODULE_NAME_TO_RP " WR_VALUE copy_from_user success\n");
		}
	break;

	case RD_VALUE:
		if(copy_to_user((int32_t *) arg, &answer, sizeof(answer))) {
			pr_err(_MODULE_NAME_TO_RP "RD_VALUE copu_to_user failed\n");
		}
		else {
			pr_info(_MODULE_NAME_TO_RP "RD_VALUE copu_to_user success\n");
		}

	break;

	case GREETER:
		if (copy_from_user(&answer, (int32_t *) arg, sizeof(answer))) {
			pr_err(_MODULE_NAME_TO_RP "GREETER copy_from_user failed\n");
		}
		else {
			pr_info(_MODULE_NAME_TO_RP "%d greets to %s\n", demo.repeat, demo.name);
		}
	break;

	default:
        pr_err(_MODULE_NAME_TO_RP "default\n");
	break;
	}

	return 0;
}


static struct file_operations fops = {
    .owner = THIS_MODULE,
    .open = module_open,
    .release = module_close,
    .unlocked_ioctl = module_ioctl
};


static int  __init _module_init(void)
{
	int ret_val = 0;

	const unsigned int c_major_swift = 20, 
                       c_minor_mask = 0xfffff;

    pr_info(_MODULE_NAME_TO_RP "Hello kernel\n");

	ret_val = register_chrdev(_MODULE_MAJOR, "ioctl-demo", &fops);

	if (0 == ret_val) {
    	pr_info(_MODULE_NAME_TO_RP "registered with Major: %d, Minor %d:", _MODULE_MAJOR, 0);
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
	unregister_chrdev(_MODULE_MAJOR, _MODULE_NAME);
	pr_info(_MODULE_NAME_TO_RP "Goodbye kernel\n");
}



MODULE_LICENSE("GPL");
MODULE_AUTHOR("Artem Shimko");
MODULE_DESCRIPTION("ioctl LKM");
MODULE_VERSION("0.01");


module_init(_module_init);
module_exit(_module_exit);
