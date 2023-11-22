//todo: msi, ioctl, dma


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


/**
 * struct pci_device_id - PCI device ID structure
 * @vendor:     Vendor ID to match (or PCI_ANY_ID)
 * @device:     Device ID to match (or PCI_ANY_ID)
 * @subvendor:      Subsystem vendor ID to match (or PCI_ANY_ID)
 * @subdevice:      Subsystem device ID to match (or PCI_ANY_ID)
 * @class:      Device class, subclass, and "interface" to match.
 *          See Appendix D of the PCI Local Bus Spec or
 *          include/linux/pci_ids.h for a full list of classes.
 *          Most drivers do not need to specify class/class_mask
 *          as vendor/device is normally sufficient.
 * @class_mask:     Limit which sub-fields of the class field are compared.
 *          See drivers/scsi/sym53c8xx_2/ for example of usage.
 * @driver_data:    Data private to the driver.
 *          Most drivers don't need to use driver_data field.
 *          Best practice is to use driver_data as an index
 *          into a static list of equivalent device types,
 *          instead of using it as a pointer.
 * @override_only:  Match only when dev->driver_override is this driver.

struct pci_device_id {
    __u32 vendor, device;       // Vendor and device ID or PCI_ANY_ID
    __u32 subvendor, subdevice; // Subsystem ID's or PCI_ANY_ID 
    __u32 class, class_mask;    // (class,subclass,prog-if) triplet 
    kernel_ulong_t driver_data; // Data private to the driver
    __u32 override_only;
};

*/

/*
static struct pci_device_id demo_pci_tbl [] __initdata = {

    {PCI_VENDOR_ID_DEMO, PCI_DEVICE_ID_DEMO,

     PCI_ANY_ID, PCI_ANY_ID, 0, 0, DEMO},

    {0,}

};
*/
static struct pci_device_id kcu105_base_ids[] = {
    /*
     * PCI_DEVICE - macro used to describe a specific pci device
     * @vend: the 16 bit PCI Vendor ID
     * @dev: the 16 bit PCI Device ID
     *
     * This macro is used to create a struct pci_device_id that matches a
     * specific device.  The subvendor and subdevice fields will be set to
     * PCI_ANY_ID.

    #define PCI_DEVICE(vend,dev) \
        .vendor = (vend), .device = (dev), \
        .subvendor = PCI_ANY_ID, .subdevice = PCI_ANY_ID
    */
    //full line example: { 0x10ec, 0x8029, PCI_ANY_ID, PCI_ANY_ID, 0, 0, CH_RealTek_RTL_8029 },
    {PCI_DEVICE(_PCI_VENDER_ID, _PCI_DEVICE_ID)},
    {0,} // end of list
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

// struct my_driver_priv {
//     u8 __iomem *hwmem;
// };

//and pci_set_drvdata(pdev, drv_priv);

/*
static int __init demo_probe(struct pci_dev *pci_dev, const struct pci_device_id *pci_id)
{
    struct demo_card *card;

    if (pci_enable_device(pci_dev))
        return -EIO;

    if (pci_set_dma_mask(pci_dev, DEMO_DMA_MASK)) {
        return -ENODEV;
    }

    if ((card = kmalloc(sizeof(struct demo_card), GFP_KERNEL)) == NULL) {
        printk(KERN_ERR “pci_demo: out of memory\n”);
        return -ENOMEM;
    }

    memset(card, 0, sizeof(*card));
    card->iobase = pci_resource_start (pci_dev, 1);
    card->pci_dev = pci_dev;
    card->pci_id = pci_id->device;
    card->irq = pci_dev->irq;
    card->next = devs;
    card->magic = DEMO_CARD_MAGIC;
    pci_set_master(pci_dev);
    request_region(card->iobase, 64, card_names[pci_id->driver_data]);
    return 0;
}
*/

static int /*__devinit*/ _probe(struct pci_dev *p_dev, const struct pci_device_id *p_id)
{
    int size = 0, status = 0;
    u16 vid = 0, did = 0;
    u8 capability = 0;
    u32 bar0 = 0/*, bar0_buf = 0*/;

    //__iomem annotation used to mark pointers to I/O memory
    void __iomem *p_bar0 = NULL;

    pr_info(_MODULE_NAME_TO_RP "probe\n");

    // pci_read_config_word(pdev, PCI_VENDOR_ID, &vendor);
    // pci_read_config_word(pdev, PCI_DEVICE_ID, &device);
    // bar = pci_select_bars(pdev, IORESOURCE_MEM);
    // err = pci_enable_device_mem(pdev);
    // err = pci_request_region(pdev, bar, MY_DRIVER);
    // if (err) {
    //     pci_disable_device(pdev);
    //     return err;
    // }
    // mmio_start = pci_resource_start(pdev, 0);
    // mmio_len = pci_resource_len(pdev, 0);
    // drv_priv = kzalloc(sizeof(struct my_driver_priv), GFP_KERNEL);
    // if (!drv_priv) {
    //     release_device(pdev);
    //     return -ENOMEM;
    // }
    // drv_priv->hwmem = ioremap(mmio_start, mmio_len);
    // if (!drv_priv->hwmem) {
    //     release_device(pdev);
    //     return -EIO;
    // }
    // pci_set_drvdata(pdev, drv_priv);
    // -------
    // void iowrite8(u8 b, void __iomem *addr)
    // void iowrite16(u16 b, void __iomem *addr)
    // void iowrite32(u16 b, void __iomem *addr)
    // unsigned int ioread8(void __iomem *addr)
    // unsigned int ioread16(void __iomem *addr)
    // unsigned int ioread32(void __iomem *addr)
    // -------
    // if (drv_priv) {
    //     if (drv_priv->hwmem) {
    //         iounmap(drv_priv->hwmem);
    //     }
    //     kfree(drv_priv);
    // }
    // release_device(pdev);


    // Reserve the I/O region:
    //request_region(iobase, iosize, “my driver”);
    //or simpler:
    //pci_request_region(pdev, bar, “my driver”);
    //or even simpler (regions for all BARs):
    //pci_request_regions(pdev, “my driver”);




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
 
    /*
        wake up the device if it was in suspended state; 
        allocate I/O and memory regions of the device (if BIOS did not);
        allocate an IRQ (if BIOS did not);
    */
    if ((status  = pcim_enable_device(p_dev)) < 0) {
        pr_err(_MODULE_NAME_TO_RP "pcim_enable_device failed\n");
        return status;
    }
    

    /*
     * pcim_iomap_regions - Request and iomap PCI BARs
     * @pdev: PCI device to map IO resources for
     * @mask: Mask of BARs to request and iomap
     * @name: Name used when requesting regions
     *
     * Request and iomap regions specified by @mask.
 
     int pcim_iomap_regions(struct pci_dev *pdev, int mask, const char *name);
    */

    /*
     BIT(nr) -> #define BIT(nr) (1UL << (nr))

     KBUILD_MODNAME -> base name of module

    */
    if ((status  = pcim_iomap_regions(p_dev, BIT(0), KBUILD_MODNAME)) < 0) {
        pr_err(_MODULE_NAME_TO_RP "pcim_iomap_regions failed\n");
        return status;
    }

    /*
     * devm_kzalloc - Resource-managed kmalloc
      * @dev: Device to allocate memory for
      * @size: Allocation size
      * @gfp: Allocation gfp flags
      *
      * Managed kmalloc.  Memory allocated with this function is
      * automatically freed on driver detach.  Like all other devres
      * resources, guaranteed alignment is unsigned long long.
      *
      * RETURNS:
      * Pointer to allocated memory on success, NULL on failure.
      */
    p_bar0 = devm_kzalloc(&p_dev->dev, sizeof(int), GFP_KERNEL);
    if (NULL == p_bar0) {
        pr_err(_MODULE_NAME_TO_RP "devm_kzalloc failed\n");
        return -ENOMEM;
    }

    /*
     * pcim_iomap_table - access iomap allocation table
     * @pdev: PCI device to access iomap table for
     *
     * Access iomap allocation table for @dev.  If iomap table doesn't
     * exist and @pdev is managed, it will be allocated.  All iomaps
     * recorded in the iomap table are automatically unmapped on driver
     * detach.
     *
     * This function might sleep when the table is first allocated but can
     * be safely called without context and guaranteed to succeed once
     * allocated.
     
       void __iomem * const *pcim_iomap_table(struct pci_dev *pdev)
     */
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

// static void my_driver_remove(struct pci_dev *pdev)
// {
//     struct my_driver_priv *drv_priv = pci_get_drvdata(pdev);

//     if (drv_priv) {
//         kfree(drv_priv);
//     }

//     pci_disable_device(pdev);
// }

static void /*__devexit*/_remove(struct pci_dev *p_dev)
{ 
    pr_info(_MODULE_NAME_TO_RP "remove\n");
}


//suspend — эта функция вызывается при засыпании устройства
//resume — эта функция вызывается при пробуждении устройства

/*
static struct pci_driver ne2k_driver = {
        .name           = DRV_NAME,
        .probe          = ne2k_pci_init_one,
        .remove         = __devexit_p(ne2k_pci_remove_one), //This replaces the function address by NULL if this code is discarded
        .id_table       = ne2k_pci_tbl,
#ifdef CONFIG_PM
        .suspend        = ne2k_pci_suspend,
        .resume         = ne2k_pci_resume,
#endif
};
*/
static struct pci_driver kcu105_base_driver = {
    .name = "kcu105_base",
    .id_table = kcu105_base_ids,
    .probe = _probe,
    .remove = _remove,
};

/*
static int __init demo_init_module (void)
{
    //to check whether the PCI bus has been supported by the Linux kernel
    if (!pci_present())

        return -ENODEV;

    if (!pci_register_driver(&demo_pci_driver)) {

        pci_unregister_driver(&demo_pci_driver);

                return -ENODEV;

    }
    return 0;
}
*/
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