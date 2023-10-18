//sudo mknod /dev/mydevice c 64 0
#include <linux/module.h>
#include <linux/init.h>

//file system interuction header
#include <linux/fs.h>

/*
Necessary functions and structures for managing character devices within the kernel

Here are some key components and functions provided by cdev.h:

struct cdev: This structure represents a character device and contains 
information about the device, including its file operations and major/minor numbers.

cdev_init(): This function is used to initialize a struct cdev instance before 
it can be registered with the kernel.

cdev_add(): This function is used to add a struct cdev instance to the kernel. 
It associates the device with its file operations and assigns it a major and minor number.

cdev_del(): This function is used to remove a struct cdev instance from the kernel. 
It unregisters the device and frees any resources associated with it.

cdev_alloc(): This function is used to allocate memory for a struct cdev instance dynamically.
cdev_set(): This function is used to set the file operations for a struct cdev instance.
*/
#include <linux/cdev.h>

/*
Functions and macros for user-space access and memory handling within the kernel.

Here are some key components and functions provided by uaccess.h:

copy_to_user(): This function is used to safely copy data from kernel space to user space. 
It handles memory alignment and protection, ensuring that the copy operation does not violate 
access permissions or cause memory corruption.

copy_from_user(): This function is used to safely copy data from user space to kernel space. 
Similar to copy_to_user(), it handles memory alignment and protection to prevent unauthorized 
memory access or corruption.

get_user(): This macro is used to retrieve a single primitive data type (such as an integer) 
from user space. It ensures proper memory access and handles potential faults caused by 
invalid memory addresses.

put_user(): This macro is used to store a single primitive data type (such as an integer) 
into user space. It performs the necessary checks and protections to ensure safe memory access.

access_ok(): This macro is used to check whether a given memory range is valid and 
accessible from user space. It verifies that the memory range falls within the user space 
address space and has the necessary access permissions.
*/
#include <linux/uaccess.h>

#define _MODULE_VERTION_OLD 0


#define _MODULE_NAME         "hello kernel"
#define _MODULE_NAME_TO_RP   "hello kernel: "

#define _MODULE_MAJOR_NUM    64U
#define _MODULE_CLASS        "hello_kernel_class"


static char buffer[255];
static int buffer_pointer;

/*
It combines the major and minor device numbers into a single value.

Here are some commonly used functions and macros related to dev_t:

MKDEV(major, minor): This macro is used to create a dev_t value from the given major and 
minor numbers. It combines the two numbers into a single dev_t.

MAJOR(dev): This macro extracts the major number from a dev_t value.

MINOR(dev): This macro extracts the minor number from a dev_t value.

imajor(inode): This function retrieves the major number associated with 
an inode structure representing a device file.

iminor(inode): This function retrieves the minor number associated with 
an inode structure representing a device file.
*/
static dev_t module_nr;

/*
It represents a class of devices and is used in device driver development to organize and 
manage devices of a similar type or category.

The struct class structure contains information and operations related to a specific class 
of devices. It serves as a container for devices that share common characteristics, 
behaviors, or functionality. Some of the important fields and members of the struct class 
structure include:

name: A string that specifies the name of the device class.

owner: A pointer to the module that owns the class. It helps with reference counting and 
module dependencies.

devnode: A function pointer that determines the device node name for the class. 
It is used when creating entries in the /dev directory for devices belonging to this class.

dev_groups: Points to an array of struct attribute_group instances that define 
the attributes of the devices in the class.

dev_release: A function pointer to the release function for devices associated with this 
class. It is called when the device is being released or destroyed.

suspend: A function pointer to the suspend function for devices in the class. 
It is called when the system enters a low-power state.

resume: A function pointer to the resume function for devices in the class. 
It is called when the system resumes from a low-power state.
*/
static struct class *p_module_class;

/*
struct cdev: This structure represents a character device and contains 
information about the device, including its file operations and major/minor numbers.
*/
static struct cdev module_device;


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

static ssize_t module_read(struct file *p_file, char *p_user_buf, size_t count, loff_t *p_offs)
{
	int to_copy = 0, not_copied = 0, delta = 0;
    
    /* Get amount of data to copy */
	to_copy = min(count, buffer_pointer);

	not_copied = copy_to_user(p_user_buf, buffer, to_copy);
	delta = to_copy - not_copied;

	return delta;
}

static ssize_t module_write(struct file *p_file, const char *p_user_buf, size_t count, loff_t *offs)
{
	int to_copy, not_copied, delta;

	/* Get amount of data to copy */
	to_copy = min(count, sizeof(buffer));

	/* Copy data to user */
	not_copied = copy_from_user(buffer, p_user_buf, to_copy);
	buffer_pointer = to_copy;

	delta = to_copy - not_copied;

	return delta;
}


static struct file_operations fops = {
    .owner = THIS_MODULE,
    .open = module_open,
    .release = module_close,
    .read = module_read,
    .write = module_write
};


static int __init _module_init(void)
{
    int ret_val = 0;

    const unsigned int c_major_swift = 20, 
                       c_minor_mask = 0xfffff;

	pr_info("Hello kernel\n");

#if _MODULE_VERTION_OLD == 1 //  separet vertion, should be learned how use it togever
    /*
    Registrates char device.

    #include <linux/fs.h>
    major num, short device name and fops
    */
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
#else
    if (alloc_chrdev_region(&module_nr, 0, 1,_MODULE_NAME) < 0) {
    	pr_err(_MODULE_NAME_TO_RP "could not be allocated\n");
    	return -1;
    }

    pr_info(_MODULE_NAME_TO_RP "registered with Major: %d, Minor %d:", 
                               ret_val >> c_major_swift, ret_val&c_minor_mask);

    if (NULL == (p_module_class = class_create(THIS_MODULE, _MODULE_CLASS))) {
    	pr_err(_MODULE_NAME_TO_RP "class can not be created\n");
    	goto class_error;
    }

    if (NULL == device_create(p_module_class, NULL, module_nr, NULL, _MODULE_NAME)) {
    	pr_err(_MODULE_NAME_TO_RP "cannot create device file\n");
    	goto file_error;
    }

    cdev_init(&module_device, &fops);

    if (-1 == cdev_add(&module_device, module_nr, 1)) {
    	pr_err(_MODULE_NAME_TO_RP "registering of device to kernel failed\n");
    	goto add_error;
    }
#endif

	return 0;

#if _MODULE_VERTION_OLD == 0 
add_error:
    device_destroy(p_module_class, module_nr);
file_error:
    class_destroy(p_module_class);
class_error:
    unregister_chrdev_region(module_nr, 1);

    return -1;
#endif
}


static void __exit _module_exit(void)
{
#if _MODULE_VERTION_OLD == 1 //  separet vertion, should be learned how use it togever
	unregister_chrdev(_MODULE_MAJOR_NUM, _MODULE_NAME);
#else
    cdev_del(&module_device);
    device_destroy(p_module_class, module_nr);
    class_destroy(p_module_class);
    unregister_chrdev_region(module_nr, 1);
#endif
	pr_info("Goodbye kernel");
}


MODULE_LICENSE("GPL");
MODULE_AUTHOR("Artem Shimko");
MODULE_DESCRIPTION("hello kernel LKM");


module_init(_module_init);
module_exit(_module_exit);