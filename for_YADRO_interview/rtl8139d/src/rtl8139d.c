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

    unsigned int    cur_tx; // хранит текущий дескриптор передачи
    unsigned int    dirty_tx; // указывает на первый из дескрипторов передачи, для которых передача еще не завершена
    unsigned int    tx_flag;
    unsigned char   *tx_buf[NUM_TX_DESC]; //адреса четырех "дескрипторов передачи"
    unsigned char   *tx_bufs;
    dma_addr_t      tx_bufs_dma; // физический адрес однго из четырех "дескрипторов передачи" 

    struct net_device_stats stats; // ifconfig
    unsigned char *rx_ring; // адрес памяти в ядре, где запоминаются принятые пакеты
    dma_addr_t rx_ring_dma; // физический адрес rx_ring памяти
    unsigned int cur_rx; // используется для отслеживания места, куда будет записываться следующий пакет
} _module_priv_t;

#define ISR             0x3e
#define CmdReset   0x10
#define CR         0x37
#define CmdTxEnb   0x04
#define CmdRxEnb   0x08
#define TCR        0x40
#define TSAD0      0x20
#define IMR        0x3c

/* ISR Bits */
#define RxOK       0x01
#define RxErr      0x02
#define TxOK       0x04
#define TxErr      0x08
#define RxOverFlow 0x10
#define RxUnderrun 0x20
#define RxFIFOOver 0x40
#define CableLen   0x2000
#define TimeOut    0x4000
#define SysErr     0x8000
#define RCR           0x44
#define RBSTART  0x30
#define MPC           0x4c
#define MULINT    0x5c
#define TSD0          0x10
#define ETH_MIN_LEN 60  /* minimum Ethernet frame size */
#define TxHostOwns    0x2000
#define TxUnderrun    0x4000
#define TxStatOK      0x8000
#define TxOutOfWindow 0x20000000
#define TxAborted     0x40000000
#define TxCarrierLost 0x80000000
#define RxBufEmpty 0x01
#define CAPR         0x38

#define RX_BUF_LEN_IDX 2         /* 0==8K, 1==16K, 2==32K, 3==64K */
#define RX_BUF_LEN     (8192 < RX_BUF_LEN_IDX)
#define RX_BUF_PAD     16           /* see 11th and 12th bit of RCR: 0x44 */
#define RX_BUF_WRAP_PAD 2048   /* spare padding to handle pkt wrap */
#define RX_BUF_TOT_LEN  (RX_BUF_LEN + RX_BUF_PAD + RX_BUF_WRAP_PAD)



#define INT_MASK (RxOK | RxErr | TxOK | TxErr | \
               RxOverFlow | RxUnderrun | RxFIFOOver | \
               CableLen | TimeOut | SysErr)

static irqreturn_t _irq_handler(int irq, void *p_dev) 
{
    struct net_device *p_net_device = (struct net_device *)p_dev;
    _module_priv_t *p_module_priv = netdev_priv(p_net_device);

    void *ioaddr = p_module_priv->hw_regs;

    unsigned short isr = readw(ioaddr + ISR);

     /* clear all interrupt.
         * Specs says reading ISR clears all interrupts and writing
         * has no effect. But this does not seem to be case. I keep on
         * getting interrupt unless I forcibly clears all interrupt :-(
         */
    writew(0xffff, ioaddr + ISR);

    if((isr & TxOK) || (isr & TxErr)) 
        {
           while((p_module_priv->dirty_tx != p_module_priv->cur_tx) || netif_queue_stopped(p_net_device))
           {
                   unsigned int txstatus = 
                           readl(ioaddr + TSD0 + p_module_priv->dirty_tx * sizeof(int));

                   if(!(txstatus & (TxStatOK | TxAborted | TxUnderrun)))
                           break; /* yet not transmitted */

                   if(txstatus & TxStatOK) {
                           pr_info(_MODULE_NAME_TO_PR "Transmit OK interrupt\n");
                           p_module_priv->stats.tx_bytes += (txstatus & 0x1fff);
                           p_module_priv->stats.tx_packets++;
                   }
                   else {
                           pr_info(_MODULE_NAME_TO_PR "Transmit Error interrupt\n");
                           p_module_priv->stats.tx_errors++;
                   }
                           
                   p_module_priv->dirty_tx++;
                   p_module_priv->dirty_tx = p_module_priv->dirty_tx % NUM_TX_DESC;

                   if((p_module_priv->dirty_tx == p_module_priv->cur_tx) & netif_queue_stopped(p_net_device))
                   {
                           pr_info(_MODULE_NAME_TO_PR "waking up queue\n");
                           netif_wake_queue(p_net_device);
                   }
           }
    }

    if(isr & RxErr) {
           /* TODO: Need detailed analysis of error status */
           pr_info(_MODULE_NAME_TO_PR "receive err interrupt\n");
           p_module_priv->stats.rx_errors++;
    }

    if(isr & RxOK) {
       pr_info(_MODULE_NAME_TO_PR "receive interrupt received\n");
       while((readb(ioaddr + CR) & RxBufEmpty) == 0)
       {
           unsigned int rx_status;
           unsigned short rx_size;
           unsigned short pkt_size;
           struct sk_buff *skb;

           if(p_module_priv->cur_rx > RX_BUF_LEN)
                   p_module_priv->cur_rx = p_module_priv->cur_rx % 32768;

           /* TODO: need to convert rx_status from little to host endian
            * XXX: My CPU is little endian only :-)
            */
           rx_status = *(unsigned int*)(p_module_priv->rx_ring + p_module_priv->cur_rx);
           rx_size = rx_status >> 16;
           
           /* first two bytes are receive status register 
            * and next two bytes are frame length
            */
           pkt_size = rx_size - 4;

           /* hand over packet to system */
           skb = dev_alloc_skb (pkt_size + 2);
           if (skb) {
                   skb->dev = p_net_device;
                   skb_reserve (skb, 2); /* 16 byte align the IP fields */

                   memcpy (skb->data, p_module_priv->rx_ring + p_module_priv->cur_rx + 4, pkt_size);

                   skb_put (skb, pkt_size);
                   skb->protocol = eth_type_trans (skb, p_net_device);
                   netif_rx (skb);

         //          p_net_device->last_rx = jiffies;
                   p_module_priv->stats.rx_bytes += pkt_size;
                   p_module_priv->stats.rx_packets++;
           } 
           else {
                   pr_info(_MODULE_NAME_TO_PR "Memory squeeze, dropping packet.\n");
                   p_module_priv->stats.rx_dropped++;
           }
   

           /* update CAPR */
           writew(p_module_priv->cur_rx, ioaddr + CAPR);
       }
        }


    if(isr & CableLen)
               pr_info(_MODULE_NAME_TO_PR "cable length change interrupt\n");
        if(isr & TimeOut)
               pr_info(_MODULE_NAME_TO_PR "time interrupt\n");
        if(isr & SysErr)
               pr_info(_MODULE_NAME_TO_PR "system err interrupt\n");

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
        p_module_priv->tx_buf[i] = &p_module_priv->tx_bufs[i * TX_BUF_SIZE];
    }
        
    return;
}



//set tx DMA address. start xmit.
static void _hw_start (struct net_device *p_net_device)
{
    _module_priv_t *p_module_priv = netdev_priv(p_net_device);

    void *ioaddr = p_module_priv->hw_regs;
    u32 i;

    /* Soft reset the chip. */
    writeb(CmdReset, ioaddr + CR);

    /* Check that the chip has finished the reset. */
    for (i = 1000; i > 0; i--) {
           barrier();
           if ((readb(ioaddr + CR) & CmdReset) == 0)
                   break;
           udelay (10);
    }

    /* Must enable Tx before setting transfer thresholds! */
    writeb(CmdTxEnb | CmdRxEnb, ioaddr + CR);

    /* tx config */
    writel(0x00000600, ioaddr + TCR); /* DMA burst size 1024 */

    /* rx config */
    writel(((1 < 12) | (7 < 8) | (1 < 7) | 
                            (1 < 3) | (1 < 2) | (1 < 1)), ioaddr + RCR);

    /* init Tx buffer DMA addresses */
    for (i = 0; i < NUM_TX_DESC; i++) {
           writel(p_module_priv->tx_bufs_dma + (p_module_priv->tx_buf[i] - p_module_priv->tx_bufs),
                                  ioaddr + TSAD0 + (i * 4));
    }

    /* init RBSTART */
    writel(p_module_priv->rx_ring_dma, ioaddr + RBSTART);
    /* initialize missed packet counter */
    writel(0, ioaddr + MPC);
    /* no early-rx interrupts */
    writew((readw(ioaddr + MULINT) & 0xF000), ioaddr + MULINT);


    writew(INT_MASK, ioaddr + IMR);

    netif_start_queue (p_net_device);


    return; //for what?
}

/* register IRQ, allocate tx buffer, initialize tx ring */
static int net_open(struct net_device *p_net_device)
{
    pr_info(_MODULE_NAME_TO_PR "net_open\n");
    _module_priv_t *p_module_priv = netdev_priv(p_net_device);


    // // enable MSI, can get IRQ 43 ??
    // p_module_priv->msi_res = pci_enable_msi(p_module_priv->p_pci_dev);

    // pr_info(_MODULE_NAME_TO_PR "MSI(0 is OK): %d, IRQ: %d\n", p_module_priv->msi_res,  p_module_priv->p_pci_dev->irq);

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

    p_module_priv->tx_flag = 0;

    /* get memory for Rx buffers*/
    p_module_priv->rx_ring = pci_alloc_consistent(p_module_priv->p_pci_dev, RX_BUF_TOT_LEN, &p_module_priv->rx_ring_dma);

    if (NULL == p_module_priv->rx_ring) {
        pr_err(_MODULE_NAME_TO_PR "%s rx_ring pci_alloc_consistent failed \n", __func__);
        free_irq(p_module_priv->p_pci_dev->irq, p_net_device); //goto

        if(p_module_priv->tx_bufs) {
            pci_free_consistent(p_module_priv->p_pci_dev, TOTAL_TX_BUF_SIZE, p_module_priv->tx_bufs, p_module_priv->tx_bufs_dma);
            p_module_priv->tx_bufs = NULL;
        }
        return -ENOMEM;
    }

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
    _module_priv_t *p_module_priv = netdev_priv(p_net_device);

    pr_info(_MODULE_NAME_TO_PR "net_start_xmit\n");
   // unsigned int entry = p_module_priv->cur_tx;

    // skb_copy_and_csum_dev(skb, p_module_priv->tx_buf[entry]);
    // dev_kfree_skb(skb);

    // entry++;
    // p_module_priv->cur_tx = entry % NUM_TX_DESC;

    // if(p_module_priv->cur_tx == p_module_priv->dirty_tx) {
    //     netif_stop_queue(p_net_device);
    // }

    void *ioaddr = p_module_priv->hw_regs;
    unsigned int entry = p_module_priv->cur_tx;
    unsigned int len = skb->len;

    

    if (len < TX_BUF_SIZE) {
           if(len < ETH_MIN_LEN)
                   memset(p_module_priv->tx_buf[entry], 0, ETH_MIN_LEN);
           skb_copy_and_csum_dev(skb, p_module_priv->tx_buf[entry]);
           dev_kfree_skb(skb);
    } else {
           dev_kfree_skb(skb);
           return 0;
    }


    writel(p_module_priv->tx_flag | max(len, (unsigned int)ETH_MIN_LEN), 
                       ioaddr + TSD0 + (entry * sizeof (u32)));
        entry++;
        p_module_priv->cur_tx = entry % NUM_TX_DESC;

    
 
   // return NETDEV_TX_OK;
    if(p_module_priv->cur_tx == p_module_priv->dirty_tx) {
           netif_stop_queue(p_net_device);
    }
    return 0;
}

static struct net_device_stats *net_get_stats(struct net_device *p_net_device)
{
    pr_info(_MODULE_NAME_TO_PR "net_get_stats\n");
    
    _module_priv_t *p_module_priv = netdev_priv(p_net_device);
    return &(p_module_priv->stats);
}


static const struct net_device_ops _net_device_ops = {
    .ndo_open       = net_open,
    .ndo_stop       = net_close,
    .ndo_start_xmit = net_start_xmit,
    .ndo_get_stats  = net_get_stats
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
        p_net_device->dev_addr[i] = 0x12;
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