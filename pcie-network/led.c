#include <linux/module.h>
#include <linux/init.h>
#include <linux/pci.h>
#include <linux/ioctl.h>
#include <linux/uaccess.h>
#include <linux/cdev.h>


#define _MODULE_NAME           "led"
#define _MODULE_NAME_TO_PR     "led: "
#define _MODULE_CLASS          "leds"

#define _PCI_VENDER_ID         0x10EE
#define _PCI_DEVICE_ID         0x8024

#define GPIO_DATA_REG_OFFSET   0x00000000U
#define GPIO_TRI_REG_OFFSET    0x00000004U
#define GPIO_2_DATA_REG_OFFSET 0x00000008U
#define GPIO_2_TRI_REG_OFFSET  0x0000000CU
#define GIER_REG_OFFSET        0x0000011CU
#define IPIER_REG_OFFSET       0x00000128U
#define IPISR_REG_OFFSET       0x00000120U

#define GPIO_TRI_OUTPUT        0x0
#define GPIO_TRI_INPUT         0x1

#define WR_VALUE               _IOW('a', 'a', int32_t *)


u8 led_val;

static dev_t dev;
static struct class *p_module_class;
static struct cdev module_dev;

static struct pci_device_id ids[] = {
    {PCI_DEVICE(_PCI_VENDER_ID, _PCI_DEVICE_ID)},
    {0,}
};

MODULE_DEVICE_TABLE(pci, ids);

typedef struct _module_priv {
    u8 __iomem *p_hw_mem;
}_module_priv_t;

static _module_priv_t priv;


static int _probe(struct pci_dev *p_dev, const struct pci_device_id *p_id)
{
    int status = 0;

    pr_info(_MODULE_NAME_TO_PR "_probe\n");

    if ((status  = pcim_enable_device(p_dev)) < 0) {
        pr_err(_MODULE_NAME_TO_PR "pcim_enable_device failed\n");
        return status;
    }

    if ((status  = pcim_iomap_regions(p_dev, BIT(0), KBUILD_MODNAME)) < 0) {
        pr_err(_MODULE_NAME_TO_PR "pcim_iomap_regions failed\n");
        return status;
    }

    priv.p_hw_mem = devm_kzalloc(&p_dev->dev, sizeof(int), GFP_KERNEL);
    if (NULL == priv.p_hw_mem) {
        pr_err(_MODULE_NAME_TO_PR "devm_kzalloc failed\n");
        return -ENOMEM;
    }

    if ((priv.p_hw_mem = pcim_iomap_table(p_dev)[0]) < 0) {
        pr_err(_MODULE_NAME_TO_PR "pcim_iomap_table failed\n");
        return -1;
    }

    iowrite8(led_val, priv.p_hw_mem + GPIO_2_DATA_REG_OFFSET);

    return 0;
}

static void _remove(struct pci_dev *p_dev)
{ 
    pr_info(_MODULE_NAME_TO_PR "_remove\n");
}


static struct pci_driver pci_drv = {
        .name           = _MODULE_NAME,
        .probe          = _probe,
        .remove         = _remove,
        .id_table       = ids,
};


static int _open(struct inode *p_inode, struct file *p_file)
{
    pr_info(_MODULE_NAME_TO_PR "_open\n");
    return 0;
}

static int _release(struct inode *p_inode, struct file *p_file)
{
    pr_info(_MODULE_NAME_TO_PR "_release\n");
    return 0;
}

static long _ioctl(struct file *p_file, unsigned int cmd, unsigned long arg)
{
    switch (cmd) {

    case WR_VALUE:
        if(copy_from_user(&led_val ,(u8 *) arg, sizeof(led_val))) {
            pr_err(_MODULE_NAME_TO_PR "_ioctl -> copy_from_user failed\n");
        }
    break;

    default:
        pr_warn(_MODULE_NAME_TO_PR "_ioctl -> default case failed\n");
    break;

    }
        
    return 0;
}

static struct file_operations fops = {
    .owner = THIS_MODULE,
    .open = _open,
    .unlocked_ioctl = _ioctl,
    .release = _release
};


static int __init _init(void)
{
    pr_info(_MODULE_NAME_TO_PR "_init\n");

    if((alloc_chrdev_region(&dev, 0, 1, _MODULE_NAME)) < 0) {
        pr_err(_MODULE_NAME_TO_PR "_init -> alloc_chrdev_region failed\n");
        return -1;
    }

    pr_info(_MODULE_NAME_TO_PR "Major = %d Minor = %d \n",MAJOR(dev), MINOR(dev));
 
    cdev_init(&module_dev, &fops);
 
    if((cdev_add(&module_dev, dev, 1)) < 0){
        pr_err(_MODULE_NAME_TO_PR "_init -> cdev_add failed\n");
        goto r_class;
    }
 
    if(IS_ERR(p_module_class = class_create(THIS_MODULE, _MODULE_CLASS))){
        pr_err(_MODULE_NAME_TO_PR "_init -> class_create failed\n");
        goto r_class;
    }
 
    if(IS_ERR(device_create(p_module_class, NULL, dev, NULL, _MODULE_NAME))){
        pr_err(_MODULE_NAME_TO_PR "_init -> device_create failed\n");
        goto r_device;
    }

    pr_info(_MODULE_NAME_TO_PR "Device Driver Insert...Done!!!\n");
    
    return pci_register_driver(&pci_drv);
 
r_device:
    class_destroy(p_module_class);
r_class:
    unregister_chrdev_region(dev, 1);
        return -1;
}

static void __exit _eexit(void)
{
    pr_info(_MODULE_NAME_TO_PR "_exit\n");

    pci_unregister_driver(&pci_drv);

    device_destroy(p_module_class, dev);
    class_destroy(p_module_class);
    cdev_del(&module_dev);
    unregister_chrdev_region(dev, 1);

    pr_info("Device Driver Remove...Done!!!\n");
}


MODULE_LICENSE("GPL");
MODULE_AUTHOR("Artem Shimko");
MODULE_DESCRIPTION("led LMK");
MODULE_VERSION("1.01");


module_init(_init);
module_exit(_eexit);