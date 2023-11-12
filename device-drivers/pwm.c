#include <linux/module.h>
#include <linux/init.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/uaccess.h>
#include <linux/pwm.h>

#define  PERIOD_NS           1000000000U

#define _MODULE_NAME         "pwm"
#define _MODULE_NAME_TO_RP   "pwm: "

#define _MODULE_CLASS        "pwm_class"

static dev_t module_nr;
static struct class *p_module_class;
static struct cdev module_dev;

static struct pwm_device *p_pwm_dev = NULL;

static u32 pwm_time = 500000000;
module_param(pwm_time, uint, S_IRUSR | S_IRGRP | S_IROTH);
MODULE_PARM_DESC(pwm_time, "time of pwm");

static ssize_t module_write(struct file *p_file, const char *p_user_buf, size_t count, loff_t *offs)
{
	char val = 0;
    int to_copy = 0, not_copied  = 0, delta  = 0;

    const int duty_ns_coef = 100000000;
    
    to_copy = min(count, sizeof(val));
    not_copied = copy_from_user(&val, p_user_buf, to_copy);
   
    if (val < 'a' || val == 'j') {
    	pr_info(_MODULE_NAME_TO_RP "invalid value\n");
    }
    else {
    	/*
            struct pwm_device *pwm - PWM device
            int duty_ns - "on" time (in nanoseconds)
            int period_ns - duration (in nanoseconds) of one cycle
        */
    	pwm_config(p_pwm_dev, duty_ns_coef * (val - 'a'), PERIOD_NS);
    }

    delta = to_copy - not_copied;


    return delta;
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
        pr_err(_MODULE_NAME_TO_RP "alloc_chrdev_region failed\n");
        return -1;
    }

    pr_info(_MODULE_NAME_TO_RP "registered with Major: %d, Minor %d:", 
                               module_nr >> c_major_swift, module_nr&c_minor_mask);

    if (NULL == (p_module_class = class_create(THIS_MODULE, _MODULE_CLASS))) {
        pr_err(_MODULE_NAME_TO_RP "class_create failed\n");
        goto class_error;
    }

    if (NULL == device_create(p_module_class, NULL, module_nr, NULL, _MODULE_NAME)) {
        pr_err(_MODULE_NAME_TO_RP "device_create failed\n");
        goto file_error;
    }

    cdev_init(&module_dev, &fops);

    if (-1 == cdev_add(&module_dev, module_nr, 1)) {
        pr_err(_MODULE_NAME_TO_RP "cdev_add failed\n");
        goto add_error;
    }

    if (NULL == (p_pwm_dev = pwm_request(0, "pwm0"))) {
    	pr_err(_MODULE_NAME_TO_RP "pwm_request failed\n");
        goto add_error;
    }

    pwm_config(p_pwm_dev, pwm_time, PERIOD_NS);
    pwm_enable(p_pwm_dev);

    pr_info(_MODULE_NAME_TO_RP "inited\n");

    return 0;

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
    pwm_disable(p_pwm_dev);
    pwm_free(p_pwm_dev);
    cdev_del(&module_dev);
    device_destroy(p_module_class, module_nr);
    class_destroy(p_module_class);
    unregister_chrdev_region(module_nr, 1);

    pr_info(_MODULE_NAME_TO_RP "exit\n");
}


MODULE_LICENSE("GPL");
MODULE_AUTHOR("Artem Shimko");
MODULE_DESCRIPTION("pwm LKM");
MODULE_VERSION("0.01");


module_init(_module_init);
module_exit(_module_exit);