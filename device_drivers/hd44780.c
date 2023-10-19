#include <linux/module.h>
#include <linux/init.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/uaccess.h>
#include <linux/gpio.h>
#include <linux/delay.h>

#define _MODULE_NAME         "hd44780"
#define _MODULE_NAME_TO_RP   "hd44780: "

#define _MODULE_CLASS        "hd44780_class"

#define DARA_BUF_SIZE        17U

static dev_t module_nr;
static struct class *p_module_class;
static struct cdev module_dev;

static char data_buf[DARA_BUF_SIZE];

//todo: struct or file
static unsigned int gpios[] = {
	3,  // Enable Pin
	2,  // Register Select Pin
	4,  // Data Pin 0
	17, // Data Pin 1
	27, // Data Pin 2
	22, // Data Pin 3
	10, // Data Pin 4
	9,  // Data Pin 5
	11, // Data Pin 6
	5,  // Data Pin 7
};

static void hd44780_enable(void) 
{
	gpio_set_value(gpios[0], 1);
	msleep(5);
	gpio_set_value(gpios[0], 0);
}

static void hd44780_send_byte(char data) 
{
	int i =0;

	for(i = 0; i < 8; i++) {
		gpio_set_value(gpios[i + 2], ((data) & (1 << i)) >> i);
	}
	hd44780_enable();
	msleep(5);
}

static void hd44780_command(uint8_t data) 
{
 	gpio_set_value(gpios[1], 0);	// RS to Instruction
	hd44780_send_byte(data);
}

static void hd44780_data(uint8_t data) 
{
 	gpio_set_value(gpios[1], 1);	/* RS to data */
	hd44780_send_byte(data);
}

static ssize_t module_write(struct file *p_file, const char *p_user_buf, size_t count, loff_t *offs)
{
	int to_copy, not_copied, delta, i;

	to_copy = min(count, sizeof(data_buf));
	not_copied = copy_from_user(data_buf, p_user_buf, to_copy);
	delta = to_copy - not_copied;

	hd44780_command(0x1);

	for(i = 0; i < to_copy; i++) {
		hd44780_data(data_buf[i]);
	}

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

    int i = 0;

    //parameter
	char text[] = "Hello World!";

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
    
    for(i = 0; i < 10; i++) {
        if (gpio_request(gpios[i], "led gpio pin") < 0) {
            pr_err(_MODULE_NAME_TO_RP "gpio %d allocation failed\n", gpios[i]);
            goto add_error;
        }

        if (gpio_direction_output(gpios[i], 0)) {
            pr_err(_MODULE_NAME_TO_RP "gpio %d direction failed\n", gpios[i]);
            goto gpio_error;
        }
    }

    pr_info(_MODULE_NAME_TO_RP "init\n");

    // const
    hd44780_command(0x30);	/* Set the display for 8 bit data interface */
	hd44780_command(0xf);	/* Turn display on, turn cursor on, set cursor blinking */
	hd44780_command(0x1);

	for(i = 0; i < (sizeof(text) - 1); i++) {
		hd44780_data(text[i]);
	}

    return 0;

gpio_error:
    for(i = 0; i < 10; i++) {
        gpio_free(gpios[i]);
    }
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
    int i = 0;

	hd44780_command(0x1);	/* Clear the display */
	for(i = 0; i < 10; i++){
		gpio_set_value(gpios[i], 0);
		gpio_free(gpios[i]);
	}
	cdev_del(&module_dev);
	device_destroy(p_module_class, module_nr);
	class_destroy(p_module_class);
    unregister_chrdev_region(module_nr, 1);

    pr_info(_MODULE_NAME_TO_RP "exit\n");
}


MODULE_LICENSE("GPL");
MODULE_AUTHOR("Artem Shimko");
MODULE_DESCRIPTION("hd44780 LKM");
MODULE_VERSION("0.01");


module_init(_module_init);
module_exit(_module_exit);