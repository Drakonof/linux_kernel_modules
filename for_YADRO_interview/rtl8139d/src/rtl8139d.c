#include <linux/module.h>   
#include <linux/init.h>
#include <linux/pci.h>
#include <linux/netdevice.h>
#include <linux/etherdevice.h>
#include <linux/interrupt.h>


#define _MODULE_VERSION       "1.0.0"
#define _MODULE_NAME          "_rtl8139d"
#define _MODULE_NAME_TO_PR    "_rtl8139d: "
#define _MODULE_LICENSE       "GPL"
#define _MODULE_DESCRIPTION   "_rtl8139d LKM"
#define _MODULE_CLASS         "NIC"

#define _PCI_VENDER_ID        0x10ec
#define _PCI_DEVICE_ID        0x8139

//regs

#define TX_BUF_SIZE         1536  // should be at least MTU + 14 + 4 ??
#define NUM_TX_DESC         4 // from 4 to 1024 for different hardware ??
#define REG_MAC_STA_ADDR    0x1488 // register offset which saved the MAC address 
#define TOTAL_TX_BUF_SIZE   (TX_BUF_SIZE * NUM_TX_DESC) // ??
#define TOTAL_RX_BUF_SIZE   16000
#define REG_MASTER_CTRL     0x1400 // soft reset register offset
#define SOFT_RESET_CMD      0x41    // BIT(0) | BIT(6)

// use "lspci -nn" to get the vender ID and device ID
static const struct pci_device_id pci_ids[] = {
    {PCI_DEVICE(_PCI_VENDER_ID, _PCI_DEVICE_ID)},
    {0,}
};
MODULE_DEVICE_TABLE(pci, pci_ids);

typedef struct sk_meta { //great_buffer
    struct sk_buff *skb;
    u16 length;     // rx buffer length
    u16 flags;      // information of buffer ??
    dma_addr_t dma; // ??
} _sk_meta_t;

typedef struct _module_priv { //great_adapter
    struct net_device   *p_net_device;
    struct pci_dev  *p_pci_dev;

    spinlock_t      lock;
    int            msi_res;

    u8 __iomem      *hw_regs;  // memory mapped I/O addr (virtual address) ??
    unsigned long   hw_regs_len;   // length of I/O or MMI/O region

    unsigned int    cur_tx; // ??
    unsigned int    dirty_tx; // ??
    unsigned char   *tx_buf[NUM_TX_DESC]; // ??
    unsigned char   *tx_bufs;        // Tx buffer start address (virtual address). ??

    dma_addr_t      tx_bufs_dma;      // Tx buffer dma address (physical address) ??

    struct net_device_stats stats; // ??
    unsigned char *rx_ring; // ??
    dma_addr_t rx_ring_dma; // ??
    unsigned int cur_rx; // ??
} _module_priv_t;


static irqreturn_t _irq_handler(int irq, void *p_dev) 
{
   // struct net_device *p_net_device = (struct net_device *)p_dev;
   //  _module_priv_t *p_module_priv = netdev_priv(p_net_device);

    // TODO: read register to see if it is Rx interrupt or Tx interrupt

    return IRQ_HANDLED;
}

// initialize tx ring  //todo DEFINE
static void _init_ring (struct net_device *p_net_device)
{
    _module_priv_t *p_module_priv = netdev_priv(p_net_device);
    int i;

    p_module_priv->cur_tx = 0;
    p_module_priv->dirty_tx = 0;

    

    for (i = 0; i < NUM_TX_DESC; i++) {
        pr_info(_MODULE_NAME_TO_PR "==================\n");
        p_module_priv->tx_buf[i] = &p_module_priv->tx_bufs[i * TX_BUF_SIZE];
    }
        
    return;
}

// set tx DMA address. start xmit.
static void _hw_start (struct net_device *p_net_device)
{
    _module_priv_t *p_module_priv = netdev_priv(p_net_device);

    u32 ctrl_data = 0; // control data write to register

    // read control register value
    //*(u32 *)&ctrl_data = readl(p_module_priv->hw_regs + REG_MASTER_CTRL);
    // // add soft reset control bit
    // pr_info(_MODULE_NAME_TO_PR "==================\n");ctrl_data |= 0x10;

    // /* Soft reset the chip. */
    //writel(ctrl_data, p_module_priv->hw_regs + REG_MASTER_CTRL);
    // wait 20ms for reset ready
    msleep(20);

    /* TODO: Must enable Tx/Rx before setting transfer thresholds! */

    /* TODO: tx config */

    /* TODO: rx config */

    /* TODO: init Tx buffer DMA addresses */

    /* TODO: Enable all known interrupts by setting the interrupt mask. */

    pr_info(_MODULE_NAME_TO_PR "==================\n");netif_start_queue (p_net_device);
    return; //for what?
}

/* register IRQ, allocate tx buffer, initialize tx ring */
static int net_open(struct net_device *p_net_device)
{
    _module_priv_t *p_module_priv = netdev_priv(p_net_device);


    // enable MSI, can get IRQ 43 ??
    p_module_priv->msi_res = pci_enable_msi(p_module_priv->p_pci_dev);

    pr_info(_MODULE_NAME_TO_PR "MSI(0 is OK): %d, IRQ: %d\n", p_module_priv->msi_res,  p_module_priv->p_pci_dev->irq);

    /* get the IRQ,  43 is correct, 17 is wrong
     * second arg is interrupt handler
     * third is flags, 0 means no IRQ sharing
     */
    if (0 != request_irq(p_module_priv->p_pci_dev->irq, _irq_handler, 0, p_net_device->name, p_net_device))
    {
        pr_err(_MODULE_NAME_TO_PR "%s request_irq failed \n", __func__);
        free_irq(p_module_priv->p_pci_dev->irq, p_net_device);
        return -1; //todo
    }
    pr_info(_MODULE_NAME_TO_PR "NET dev IRQ:%d, PCI dev IRQ:%d\n", p_net_device->irq, p_module_priv->p_pci_dev->irq);

    /* get memory for Tx buffers
     * memory must be DMAable
     */
    p_module_priv->tx_bufs = pci_alloc_consistent(p_module_priv->p_pci_dev, TOTAL_TX_BUF_SIZE, &p_module_priv->tx_bufs_dma);

    if (NULL == p_module_priv->tx_bufs) {
        pr_err(_MODULE_NAME_TO_PR "%s tx_bufs pci_alloc_consistent failed \n", __func__);
        free_irq(p_module_priv->p_pci_dev->irq, p_net_device); //goto

        return -ENOMEM;
    }

    /* get memory for Rx buffers*/
    // p_module_priv->rx_ring = pci_alloc_consistent(p_module_priv->p_pci_dev, TOTAL_RX_BUF_SIZE, &p_module_priv->rx_ring_dma);

    // if (NULL == p_module_priv->rx_ring) {
    //     pr_err(_MODULE_NAME_TO_PR "%s rx_ring pci_alloc_consistent failed \n", __func__);
    //     free_irq(p_module_priv->p_pci_dev->irq, p_net_device); //goto

    //     if(p_module_priv->tx_bufs) {
    //         pci_free_consistent(p_module_priv->p_pci_dev, TOTAL_TX_BUF_SIZE, p_module_priv->tx_bufs, p_module_priv->tx_bufs_dma);
    //         p_module_priv->tx_bufs = NULL;
    //     }
    //     return -ENOMEM;
    // }

    // initialize the tx ring
    _init_ring(p_net_device);
    _hw_start(p_net_device);

        
    return 0;
}

static int net_close(struct net_device *p_net_device)
{
    pr_info(_MODULE_NAME_TO_PR "net_close\n");

    return 0;
}

static netdev_tx_t net_start_xmit(struct sk_buff *skb, struct net_device *p_net_device)
{
    // _module_priv_t *p_module_priv = netdev_priv(p_net_device);
    // unsigned int entry = p_module_priv->cur_tx;

    // skb_copy_and_csum_dev(skb, p_module_priv->tx_buf[entry]);
    // dev_kfree_skb(skb);

    // entry++;
    // p_module_priv->cur_tx = entry % NUM_TX_DESC;

    // if(p_module_priv->cur_tx == p_module_priv->dirty_tx) {
    //     netif_stop_queue(p_net_device);
    // }

    pr_info(_MODULE_NAME_TO_PR "net_start_xmit\n");
 
    return NETDEV_TX_OK;
}

// static struct net_device_stats *net_get_stats(struct net_device *p_net_device)
// {
//     _module_priv_t *p_module_priv = netdev_priv(p_net_device);

//     pr_info(_MODULE_NAME_TO_PR "net_get_stats\n");
//     msleep(1000);
//     return &(p_module_priv->stats);
// }


static const struct net_device_ops _net_device_ops = {
    .ndo_open       = net_open,
    .ndo_stop       = net_close,
    .ndo_start_xmit = net_start_xmit
  //  .ndo_get_stats  = net_get_stats
};


static int /*__devinit*/ pci_probe(struct pci_dev *p_pci_dev, const struct pci_device_id *ent)
{
    struct net_device *p_net_device;
    _module_priv_t *p_module_priv = NULL;

    unsigned long mmio_start = 0, 
                  mmio_end = 0, 
                  mmio_len = 0, 
                  mmio_flags = 0;

    void *ioaddr = NULL; // virtual address of mmio_start after ioremap()

    unsigned int i = 0;

    pr_info(_MODULE_NAME_TO_PR "Vender ID: 0x%x, Device ID: 0x%x\n", pci_ids->vendor, pci_ids->device);

    /* enable device (incl. PCI PM wakeup and hotplug setup) 
    Well, both functions internally call pci_enable_device_flags(). 
    The difference is that pci_enable_device_mem() variant initializes only Memory-mapped BARs, whereas pci_enable_device() will initialize both Memory-mapped and IO BARs.

     If your PCI device does not have IO spaces (most probably this is indeed the case) you can easily use pci_enable_device_mem().

     "включаем" память устройства
    */
    if (0 != pci_enable_device_mem(p_pci_dev)) {
        pr_err(_MODULE_NAME_TO_PR "%s pci_enable_device_mem failed \n", __func__);
        return PCI_ERS_RESULT_DISCONNECT;
    }

    // if ((0 != pci_set_dma_mask(p_pci_dev, DMA_BIT_MASK(32))) || (0 != pci_set_consistent_dma_mask(p_pci_dev, DMA_BIT_MASK(32)))) {
    //     pr_err(_MODULE_NAME_TO_PR "%s -> pci_set_dma_mask or pci_set_consistent_dma_mask failed \n", __func__);
    //     pci_disable_device(p_pci_dev); // goto
    //     return err;
    // }

    if (0 != pci_request_regions(p_pci_dev, _MODULE_NAME)) {
        pr_err(_MODULE_NAME_TO_PR "%s pci_enable_device_mem failed \n", __func__);
        pci_disable_device(p_pci_dev);
        return -ENOMEM; //??
    }

    pci_set_master(p_pci_dev);

    /* There are a lot of PCI devices on the PCI bus. How does the kernel know great is an ethernet device?
     allocate the memory for ethernet device. Alloccate memoryfor net_device plus private p_module_priv memory */
    p_net_device = alloc_etherdev(sizeof(_module_priv_t));
    if (NULL == p_net_device) {
        pr_err(_MODULE_NAME_TO_PR "%s alloc_etherdev failed \n", __func__);
        pci_release_regions(p_pci_dev);
        return -ENOMEM;
    }


    /* set net device's parent device to PCI device.

      Set the sysfs physical device reference for the network logical device
      if set prior to registration will cause a symlink during initialization.

      Virtual devices (that are present only in the Linux device model) are children of real devices. 

     */
    SET_NETDEV_DEV(p_net_device, &p_pci_dev->dev);

    /*Устанавливаем данные PCI драйвера и возвращаем ноль.
      Это сохранение объекта, описывающего карту. Этот указатель также используется в обратных вызовах удаления и управления питанием.

    */
    
    pci_set_drvdata(p_pci_dev, p_net_device); // модет быть и не нужно

    /* private data pointer point to net device's private data */ //??
    p_module_priv = netdev_priv(p_net_device);

    /* set private data's PCI device to PCI device */ //??
    p_module_priv->p_pci_dev = p_pci_dev;

    /* set private data's net device to net device */ //??
    p_module_priv->p_net_device = p_net_device;

    /* initialize the spinlock */ //??
    spin_lock_init (&p_module_priv->lock);

    /* get mem map io address */
    mmio_start = pci_resource_start(p_pci_dev, 1);
    mmio_end = pci_resource_end(p_pci_dev, 1);
    mmio_len = pci_resource_len(p_pci_dev, 1);
    mmio_flags = pci_resource_flags(p_pci_dev, 1);


/*$lshw -c network
 *-network
       description: Ethernet interface
       product: AR8132 Fast Ethernet
       vendor: Atheros Communications Inc.
       physical id: 0
       bus info: pci@0000:08:00.0
       logical name: eth0
       version: c0
       serial: 00:26:22:64:65:bf
       size: 100Mbit/s
       capacity: 100Mbit/s
       width: 64 bits
       clock: 33MHz
       capabilities: bus_master cap_list ethernet physical tp 10bt 10bt-fd 100bt 100bt-fd autonegotiation
       configuration: autonegotiation=on broadcast=yes driver=atl1c driverversion=1.0.1.0-NAPI duplex=full latency=0 multicast=yes port=twisted pair speed=100Mbit/s
       resources: irq:43 memory:f1000000-f103ffff ioport:2000(size=128)
*/

    /* ioremap MMI/O region */
    ioaddr = ioremap(mmio_start, mmio_len);
    if(IS_ERR(ioaddr)) {
        pr_err(_MODULE_NAME_TO_PR "%s ioremap failed \n", __func__);
        free_netdev(p_net_device); //goto
    }

    // set private data
    p_net_device->base_addr = (long)ioaddr;
    p_module_priv->hw_regs = ioaddr;
    p_module_priv->hw_regs_len = mmio_len;

    for(i = 0; i < 6; i++) {  /* Hardware Address */ //u16
      //  p_net_device->dev_addr[i] =  readb((const volatile void*)(p_net_device->base_addr+i));
        p_net_device->broadcast[i] = 0xff;
    }

    p_net_device->hard_header_len = 14;

    /* set driver name and irq */
    memcpy(p_net_device->name, _MODULE_NAME, sizeof(_MODULE_NAME)); /* Device Name */ // why not srtcpy??
    pr_info(_MODULE_NAME_TO_PR "MAC: %d, IRQ number: %d\n", *p_net_device->dev_addr, p_module_priv->p_pci_dev->irq);

    p_net_device->netdev_ops = &_net_device_ops;

    /* register the network device */
    
    
    if (0 != register_netdev(p_net_device)) {
        pr_err(_MODULE_NAME_TO_PR "%s register_netdev failed \n", __func__);
        free_irq(p_module_priv->p_pci_dev->irq, p_net_device);
        iounmap(p_module_priv->hw_regs);
        unregister_netdev(p_net_device);
        free_netdev(p_net_device);
        pci_release_regions(p_pci_dev);
        pci_disable_device(p_pci_dev);
        return -1;
    }

    return 0;
}

static void pci_remove(struct pci_dev *p_pci_dev)
{
    struct net_device *p_net_device = pci_get_drvdata(p_pci_dev); //??
    _module_priv_t *p_module_priv = netdev_priv(p_net_device);

    free_irq(p_module_priv->p_pci_dev->irq, p_net_device);
    iounmap(p_module_priv->hw_regs);
    unregister_netdev(p_net_device);
    free_netdev(p_net_device);
    pci_release_regions(p_pci_dev);
    pci_disable_device(p_pci_dev);
}


static struct pci_driver _pci_driver_ops = {
    .name           = _MODULE_NAME,
    .probe          = pci_probe,
    .remove         = pci_remove,
    .id_table       = pci_ids,
};


static int __init _init(void)
{
    pr_info(_MODULE_NAME_TO_PR "Device Driver Insert...Done!!!\n");
    
    return pci_register_driver(&_pci_driver_ops);
}

static void __exit _eexit(void)
{
    pci_unregister_driver(&_pci_driver_ops);

    pr_info(_MODULE_NAME_TO_PR "Device Driver Remove...Done!!!\n");
}



MODULE_LICENSE(_MODULE_LICENSE);
MODULE_AUTHOR("Artem Shimko");
MODULE_DESCRIPTION(_MODULE_DESCRIPTION);
MODULE_VERSION(_MODULE_VERSION);


module_init(_init);
module_exit(_eexit);