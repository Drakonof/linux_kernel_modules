//todo: msi

#include <linux/module.h>
#include <linux/init.h>
#include <linux/pci.h>


#define _MODULE_NAME         "kcu105_base"
#define _MODULE_NAME_TO_RP   "kcu105_base: "

#define _PCI_VENDER_ID 0x10EE
#define _PCI_DEVICE_ID 0x8024

#define GPIO_DATA_REG_OFFSET 0x0000
#define GPIO_TRI_REG_OFFSET 0x0004
#define GPIO_2_DATA_REG_OFFSET 0x0008
#define GPIO_2_TRI_REG_OFFSET 0x000C

#define GPIO_TRI_OUTPUT 0x0
#define GPIO_TRI_INPUT  0x1



static struct pci_device_id kcu105_base_ids[] = {
  {PCI_DEVICE(_PCI_VENDER_ID, _PCI_DEVICE_ID)},
  {}
};

MODULE_DEVICE_TABLE(pci, kcu105_base_ids);


// example
irqreturn_t _irq_handler(int irq, void *p_data)
{
    pr_info(_MODULE_NAME_TO_RP "_irq_handler\n");

    /* 
        some todo depence on board ...
       like iowrite8 to irq reg ad etc...
    */

    return IRQ_HANDLED;

}

static int _probe(struct pci_dev *p_dev, const struct pci_device_id *p_id)
{
    int size = 0, status = 0;
    u16 vid = 0, did = 0;
    u8 capability = 0;
    u32 bar0 = 0/*, bar0_buf = 0*/;

    void __iomem *p_bar0 = NULL;

    pr_info(_MODULE_NAME_TO_RP "probe\n");

    if (0 != pci_read_config_word(p_dev, 0x0, &vid)) {
        pr_err(_MODULE_NAME_TO_RP "pci_read_config_word VID failed\n");
        return -1;
    }
    pr_info(_MODULE_NAME_TO_RP "VID: 0x%X\n", vid);

    if (0 != pci_read_config_word(p_dev, 0x2, &did)) {
        pr_err(_MODULE_NAME_TO_RP "pci_read_config_word DID failed\n");
        return -1;
    }
    pr_info(_MODULE_NAME_TO_RP "DID: 0x%X\n", did);

    if (0 != pci_read_config_byte(p_dev, 0x34, &capability)) {
        pr_err(_MODULE_NAME_TO_RP "pci_read_config_byte capability failed\n");
        return -1;
    }
    pr_info(_MODULE_NAME_TO_RP "capability: 0x%X\n", capability);

    if (0 != pci_read_config_dword(p_dev, 0x10, &bar0)) {
        pr_err(_MODULE_NAME_TO_RP "pci_read_config_dword bar0 failed\n");
        return -1;
    }
    pr_info(_MODULE_NAME_TO_RP "bar0: 0x%X\n", bar0);

    // if (0 != pci_write_config_dword(p_dev, 0x10, 0xffffffff)) {
    //     pr_err(_MODULE_NAME_TO_RP "pci_write_config_dword bar0 failed\n");
    //     return -1;
    // }

    // if (0 != pci_read_config_dword(p_dev, 0x10, &bar0_buf)) {
    //     pr_err(_MODULE_NAME_TO_RP "pci_read_config_dword bar0_buf failed\n");
    //     return -1;
    // }
    // pr_info(_MODULE_NAME_TO_RP "bar0_buf: 0x%X\n", bar0_buf);


    // if (0 != pci_write_config_dword(p_dev, 0x10, bar0)) {
    //     pr_err(_MODULE_NAME_TO_RP "pci_write_config_dword bar0 failed\n");
    //     return -1;
    // }

    if ((bar0 & 0x3) == 1) {
        pr_info(_MODULE_NAME_TO_RP "IO space\n");   
    }
    else {
        pr_info(_MODULE_NAME_TO_RP "MEM space\n");
    }

    bar0 &= 0xfffffffd;
    bar0 = ~bar0;
    bar0++;

    pr_info(_MODULE_NAME_TO_RP "size: %d bytes\n", bar0);

    size = pci_resource_len(p_dev, 0);
    pr_info(_MODULE_NAME_TO_RP "size again: %d bytes\n", size);
    pr_info(_MODULE_NAME_TO_RP "bar0 is maped: 0x%llx bytes\n", pci_resource_start(p_dev, 0));
 
    if ((status  = pcim_enable_device(p_dev)) < 0) {
        pr_err(_MODULE_NAME_TO_RP "pcim_enable_device failed\n");
        return status;
    }

    if ((status  = pcim_iomap_regions(p_dev, BIT(0), KBUILD_MODNAME)) < 0) {
        pr_err(_MODULE_NAME_TO_RP "pcim_iomap_regions failed\n");
        return status;
    }

    p_bar0 = devm_kzalloc(&p_dev->dev, sizeof(int), GFP_KERNEL);
    if (NULL == p_bar0) {
        pr_err(_MODULE_NAME_TO_RP "devm_kzalloc failed\n");
        return -ENOMEM;
    }

    if ((p_bar0  = pcim_iomap_table(p_dev)[0]) < 0) {
        pr_err(_MODULE_NAME_TO_RP "pcim_iomap_table failed\n");
        return -1;
    }

    iowrite8(0x5, p_bar0 + GPIO_2_DATA_REG_OFFSET); //ioread32( p_bar0 + GPIO_2_DATA_REG_OFFSET)
    

    if (p_dev->irq) {
        status = devm_request_irq(&p_dev->dev, p_dev->irq, _irq_handler, 0, 
                                 KBUILD_MODNAME, p_bar0);
        if (status) {
            pr_err(_MODULE_NAME_TO_RP "dev_request_irq failed\n");
            return -1;
        }

        pr_info(_MODULE_NAME_TO_RP "dev_request_irq: OK\n");

        /* 
            some todo depence on board ...
            like iowrite8 to irq reg ad etc...
        */
    }
    

    return 0;
}

static void _remove(struct pci_dev *p_dev)
{ 
    pr_info(_MODULE_NAME_TO_RP "remove\n");
}


static struct pci_driver kcu105_base_driver = {
    .name = "kcu105_base",
    .id_table = kcu105_base_ids,
    .probe = _probe,
    .remove = _remove,
};


static int __init _module_init(void)
{
    pr_info(_MODULE_NAME_TO_RP "init\n");

    return pci_register_driver(&kcu105_base_driver);
}

static void __exit _module_exit(void)
{
    pr_info(_MODULE_NAME_TO_RP "exit\n");
    pci_unregister_driver(&kcu105_base_driver);
}


MODULE_LICENSE("GPL");
MODULE_AUTHOR("Artem Shimko");
MODULE_DESCRIPTION("kcu105_base LKM");
MODULE_VERSION("0.01");


module_init(_module_init);
module_exit(_module_exit);