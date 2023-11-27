#include <linux/module.h>
#include <linux/init.h>
#include <linux/pci.h>


#define _MODULE_NAME          "uartlite"
#define _MODULE_NAME_TO_PR    "uartlite: "
#define _MODULE_CLASS         "serial"

#define _PCI_VENDER_ID        0x10EE
#define _PCI_DEVICE_ID        0x8024

#define RX_FIFO_REG           0x0000U
#define TX_FIFO_REG           0x0004U
#define STAT_REG              0x0008U
#define CTRL_REG              0x000CU

#define RX_FIFO_RST_FIELD     0x0U
#define TX_FIFO_RST_FIELD     0x1U
#define IRQ_FIFO_RST_FIELD    0x4U

#define TX_FULL_ER_FIELD      0x3U

#define WR                    _IOW("u", "w", int8_t *)
#define RD                    _IOR("u", "r", int8_t *)


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

static u8 tx_data;


static int _probe(struct pci_dev *p_dev, const struct pci_device_id *p_id)
{
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

    iowrite8(BIT(RX_FIFO_RST_FIELD) /*| BIT(TX_FIFO_RST_FIELD) | BIT(IRQ_FIFO_RST_FIELD)*/, priv.p_hw_mem + CTRL_REG);
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
    u32 uartlite_status = 0;

    switch (cmd) {

    case WR:
        if(copy_from_user(&tx_data ,(u8 *) arg, sizeof(tx_data))) {
            pr_err(_MODULE_NAME_TO_PR "_ioctl -> copy_from_user failed\n");
        }
        
        if (0 == (ioread32(priv.p_hw_mem + STAT_REG) & BIT(TX_FULL_ER_FIELD))) {
           iowrite8(tx_data, priv.p_hw_mem + TX_FIFO_REG);
        }
        else {
            pr_warn(_MODULE_NAME_TO_PR "tx fifo is currently full\n");
        }
        
    break;
    case RD:
        pr_warn(_MODULE_NAME_TO_PR "_ioctl -> RD has not been implemented\n");
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

    pr_info(_MODULE_NAME_TO_PR "Device Driver Remove...Done!!!\n");
}


MODULE_LICENSE("GPL");
MODULE_AUTHOR("Artem Shimko");
MODULE_DESCRIPTION("uartlite LMK");
MODULE_VERSION("1.01");


module_init(_init);
module_exit(_eexit);