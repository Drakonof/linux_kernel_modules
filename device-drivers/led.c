#include <linux/module.h>
#include <linux/init.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/uaccess.h>
#include <linux/gpio.h>

#define _MODULE_NAME         "led"
#define _MODULE_NAME_TO_RP   "led: "

#define _MODULE_CLASS        "led_class"

static dev_t module_nr;
static struct class *p_module_class;
static struct cdev module_dev;

static unsigned int pin_nr = 4;
module_param(pin_nr, uint, S_IRUSR | S_IRGRP | S_IROTH);
MODULE_PARM_DESC(pin_nr, "pin number");


static ssize_t module_write(struct file *p_file, const char *p_user_buf, size_t count, loff_t *offs)
{
    char val = 0;

    unsigned int not_copied = 0;

    if (0 != (not_copied = copy_from_user(&val, p_user_buf, sizeof(val)))) {
        pr_err(_MODULE_NAME_TO_RP "copy_from_user failed\n");
        return -1;
    }

    gpio_set_value(pin_nr, ('0' == val) ? false : true);
    
    return 0;
}

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
    .release = module_close,
    .write = module_write
};


static int __init _module_init(void)
{
    const unsigned int c_major_swift = 20, 
                       c_minor_mask = 0xfffff;

    if (alloc_chrdev_region(&module_nr, 0, 1, _MODULE_NAME) < 0) {
        pr_err(_MODULE_NAME_TO_RP "allocation failed\n");
        return -1;
    }

    pr_info(_MODULE_NAME_TO_RP "registered with Major: %d, Minor %d:", 
                               module_nr >> c_major_swift, module_nr&c_minor_mask);

    if (NULL == (p_module_class = class_create(THIS_MODULE, _MODULE_CLASS))) {
        pr_err(_MODULE_NAME_TO_RP "device class failed\n");
        goto class_error;
    }

    if (NULL == device_create(p_module_class, NULL, module_nr, NULL, _MODULE_NAME)) {
        pr_err(_MODULE_NAME_TO_RP "device file failed\n");
        goto file_error;
    }

    cdev_init(&module_dev, &fops);

    if (-1 == cdev_add(&module_dev, module_nr, 1)) {
        pr_err(_MODULE_NAME_TO_RP "registering failed\n");
        goto add_error;
    }

    if (gpio_request(pin_nr, "led gpio pin") < 0) {
        pr_err(_MODULE_NAME_TO_RP "gpio %d allocation failed\n", pin_nr);
        goto add_error;
    }

    if (gpio_direction_output(pin_nr, 0)) {
        pr_err(_MODULE_NAME_TO_RP "gpio %d direction failed\n", pin_nr);
        goto gpio_error;
    }

    pr_info(_MODULE_NAME_TO_RP "init\n");

    return 0;

gpio_error:
    gpio_free(pin_nr);
add_error:
    device_destroy(p_module_class, module_nr);
file_error:
    class_destroy(p_module_class);
class_error:
    unregister_chrdev_region(module_nr, 1);

    return -1;
}


static void __exit _module_exit(void)
{
    gpio_set_value(pin_nr, 0);
    gpio_free(pin_nr);
    cdev_del(&module_dev);
    device_destroy(p_module_class, module_nr);
    class_destroy(p_module_class);
    unregister_chrdev_region(module_nr, 1);

    pr_info(_MODULE_NAME_TO_RP "exit\n");
}



MODULE_LICENSE("GPL");
MODULE_AUTHOR("Artem Shimko");
MODULE_DESCRIPTION("led LKM");
MODULE_VERSION("0.01");


module_init(_module_init);
module_exit(_module_exit);