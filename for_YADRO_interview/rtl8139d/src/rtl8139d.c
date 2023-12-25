#include <linux/module.h>   
#include <linux/init.h>
#include <linux/pci.h>
#include <linux/netdevice.h>
#include <linux/etherdevice.h>
#include <linux/interrupt.h>

#include "rtl8139d.h"


#define _MODULE_VERSION       "1.0.0"
#define _MODULE_NAME          "_rtl8139d"
#define _MODULE_NAME_TO_PR    "_rtl8139d: "
#define _MODULE_LICENSE       "GPL"
#define _MODULE_DESCRIPTION   "_rtl8139d LKM"
#define _MODULE_CLASS         "NIC"

#define _PCI_VENDER_ID        0x10ecU
#define _PCI_DEVICE_ID        0x8139U

#define SET_ADDR(_iomem, _offset) ((_iomem) + (_offset))


static const struct pci_device_id pci_ids[] = {
    {PCI_DEVICE(_PCI_VENDER_ID, _PCI_DEVICE_ID)},
    {0,}
};
MODULE_DEVICE_TABLE(pci, pci_ids);


typedef struct _module_priv {
    struct net_device *p_net_device;
    struct pci_dev *p_pci_dev;

    struct net_device_stats _net_device_stats;

    u8 __iomem *iomem;
    size_t iomem_len;

    u32 tx_current_desc;
    u32 tx_dirty_desc;
    u32 tx_flag;

    u8 *tx_desc_buf[TX_BUF_NUM];
    u8 *tx_desc_bufs_;

    u32 rx_current_desc;
    u8 *rx_ring_buf;

    dma_addr_t tx_desc_buf_dma;
    dma_addr_t rx_ring_buf_dma;
} _module_priv_t;


static irqreturn_t _irq_handler(int irq, void *p_dev) 
{
    u16 isr = 0;
    u16 rx_size = 0;

    u32 tx_status = 0;
    u32 rx_status = 0;
    
    const u32 c_reg_size = 0x4;
    const u32 c_own_mask = 0x1fff;
    const u32 c_rx_size_shift = 0x10;

    struct net_device *p_net_device = NULL;
    _module_priv_t *p_module_priv = NULL;

    if (IS_ERR(p_dev)) {
        pr_err(_MODULE_NAME_TO_PR "%s p_dev is NULL \n", __func__);
        return IRQ_HANDLED;
    }

    p_net_device = (struct net_device *)p_dev;
    p_module_priv = netdev_priv(p_net_device);

    isr = readw(SET_ADDR(p_module_priv->iomem, ISR_REG_OFFSET));

     /* clear all interrupt.*/
    writew(ISR_INTERRUPT_CLN, SET_ADDR(p_module_priv->iomem, ISR_REG_OFFSET));

    if ((isr & BIT(ISR_TX_OK_BIT)) || (isr & BIT(ISR_TX_ERR_BIT))) {

        while ((p_module_priv->tx_dirty_desc != p_module_priv->tx_current_desc) || 
                netif_queue_stopped(p_net_device)) {

            u32 tsdo_addr = TSD0_REG_OFFSET + p_module_priv->tx_dirty_desc * c_reg_size;

            tx_status = readl(SET_ADDR(p_module_priv->iomem, tsdo_addr));

            if (0 == (tx_status & (BIT(TSD_TX_OK_BIT) | BIT(TSD_TX_ABORTED_BIT) | BIT(TSD_TX_FIFO_UNR_BIT)))) {
                break; /* yet not transmitted */ 
            }
                           
            if (tx_status & BIT(TSD_TX_OK_BIT)) {
                p_module_priv->_net_device_stats.tx_bytes += (tx_status & c_own_mask);
                p_module_priv->_net_device_stats.tx_packets++;
            }
            else {
                p_module_priv->_net_device_stats.tx_errors++;
            }
                           
            p_module_priv->tx_dirty_desc++;
            p_module_priv->tx_dirty_desc = p_module_priv->tx_dirty_desc % TX_BUF_NUM;

            if ((p_module_priv->tx_dirty_desc == p_module_priv->tx_current_desc) & 
                netif_queue_stopped(p_net_device)) {

                netif_wake_queue(p_net_device);
            }
        }
    }

    if (0 != (isr & BIT(ISR_RX_ERR_BIT))) {
        p_module_priv->_net_device_stats.rx_errors++; // TODO: Need detailed analysis of error status
    }

    if (0 != (isr & BIT(ISR_RX_OK_BIT))) {

        while (0 == (readb(SET_ADDR(p_module_priv->iomem, CR_REG_OFFSET)) & 
               BIT(CR_RX_BUF_EMPTY_BIT))) {
           
            struct sk_buff *skb = NULL;

            if(p_module_priv->rx_current_desc > RX_BUF_SIZE_IDX) {
                p_module_priv->rx_current_desc = p_module_priv->rx_current_desc % RX_BUF_SIZE;
            }
                   
            rx_status = *(u32 *)(p_module_priv->rx_ring_buf + p_module_priv->rx_current_desc); // TODO: need to convert rx_status from little to host endian
            rx_size = rx_status >> c_rx_size_shift;
           
            /* first two bytes are receive status register 
             * and next two bytes are frame length
             */
            rx_size -= 4;

            /* hand over packet to system */
            skb = dev_alloc_skb (rx_size + 2);

            if (NULL != skb) {
                skb->dev = p_net_device;
                skb_reserve (skb, 2); /* 16 byte align the IP fields */

                memcpy(skb->data, p_module_priv->rx_ring_buf + p_module_priv->rx_current_desc + 4, rx_size);

                skb_put (skb, rx_size);
                skb->protocol = eth_type_trans(skb, p_net_device);
                netif_rx (skb);
 
                p_module_priv->_net_device_stats.rx_bytes += rx_size;
                p_module_priv->_net_device_stats.rx_packets++;
            } 
            else {
                pr_err(_MODULE_NAME_TO_PR "%s dev_alloc_skb failed \n", __func__);
                p_module_priv->_net_device_stats.rx_dropped++;
            }
   
            /* update CAPR */
            writew(p_module_priv->rx_current_desc, SET_ADDR(p_module_priv->iomem, CAPR_REG_OFFSET));
        }
    }

#if DEBUG

    if (0 != (isr & BIT(ISR_LEN_CH_BIT))) {
        pr_debug(_MODULE_NAME_TO_PR "Cable length change interrupt\n");
    }
               
    if(0 != (isr & BIT(ISR_TIMEOUT_BIT))) {
        pr_debug(_MODULE_NAME_TO_PR "Time interrupt\n");
    }
               
    if(0 != (isr & BIT(ISR_SYSTEM_ER_BIT))) {
        pr_debug(_MODULE_NAME_TO_PR "System error interrupt\n");    
    }
               
#endif

    return IRQ_HANDLED;
}

static void _init_ring(struct net_device *p_net_device)
{
    u32 i = 0;

    _module_priv_t *p_module_priv = NULL;

    if (IS_ERR(p_net_device)) {
        pr_err(_MODULE_NAME_TO_PR "%s p_net_device is NULL \n", __func__);
        return;
    }

    p_module_priv = netdev_priv(p_net_device);
    
    p_module_priv->tx_current_desc = 0;
    p_module_priv->tx_dirty_desc = 0;

    for (i = 0; i < TX_BUF_NUM; i++) {
        p_module_priv->tx_desc_buf[i] = &p_module_priv->tx_desc_bufs_[i * TX_BUF_SIZE];
    }
        
    return;
}

static void _hw_start (struct net_device *p_net_device)
{
    bool reset_complite_flag = false;

    u32 i = 0;

    const u32 c_wait_reset_time = 1000;
    const u32 c_udelay = 10;
    const u32 c_dma_1024_burst = 0x600;
    const u32 c_early_rx_intr_mask = 0xF000;

    _module_priv_t *p_module_priv = NULL;

    if (IS_ERR(p_net_device)) {
        pr_err(_MODULE_NAME_TO_PR "%s p_net_device is NULL \n", __func__);
        return;
    }

    p_module_priv = netdev_priv(p_net_device);

    /* Soft reset the chip. */
    writeb(BIT(CR_RST_BIT), SET_ADDR(p_module_priv->iomem, CR_REG_OFFSET));

    /* Check that the chip has finished the reset. */
    for (i = 0; i < c_wait_reset_time; i++) {
        if (0 == (readb(SET_ADDR(p_module_priv->iomem, CR_REG_OFFSET)) & BIT(CR_RST_BIT))) {
            reset_complite_flag = true;
            break;
        }

        udelay(c_udelay);
    }

    if (false == reset_complite_flag) {
        pr_err(_MODULE_NAME_TO_PR "%s reset_complite_flag is false \n", __func__);
        return;
    }

    /* Must enable Tx before setting transfer thresholds! */
    writeb(BIT(CR_TX_EN_BIT) | BIT(CR_RX_EN_BIT), SET_ADDR(p_module_priv->iomem, CR_REG_OFFSET));

    /* tx config */
    writel(c_dma_1024_burst, SET_ADDR(p_module_priv->iomem, TCR_REG_OFFSET));

    /* rx config */
    writel(RCR_CONFIG_MASK, SET_ADDR(p_module_priv->iomem, RCR_REG_OFFSET));

    /* init Tx buffer DMA addresses */
    for (i = 0; i < TX_BUF_NUM; i++) {
        writel(p_module_priv->tx_desc_buf_dma + (p_module_priv->tx_desc_buf[i] - p_module_priv->tx_desc_bufs_),
               SET_ADDR(p_module_priv->iomem, TSAD0_REG_OFFSET + (i * 4)));
    }

    /* init RBSTART */
    writel(p_module_priv->rx_ring_buf_dma, SET_ADDR(p_module_priv->iomem, RBSTART_REG_OFFSET));

    /* initialize missed packet counter */
    writel(0, SET_ADDR(p_module_priv->iomem, MPC_REG_OFFSET));

    /* no early-rx interrupts */
    writew((readw(SET_ADDR(p_module_priv->iomem, MULINT_REG_OFFSET)) & c_early_rx_intr_mask), 
                SET_ADDR(p_module_priv->iomem, MULINT_REG_OFFSET));

    writew(ISR_INTERRUPT_MASK, SET_ADDR(p_module_priv->iomem, IMR_REG_OFFSET));

    netif_start_queue (p_net_device);

    return;
}

/* register IRQ, allocate tx buffer, initialize tx ring */
static int net_open(struct net_device *p_net_device)
{
    _module_priv_t *p_module_priv = NULL;

    if (IS_ERR(p_net_device)) {
        pr_err(_MODULE_NAME_TO_PR "%s p_net_device is NULL \n", __func__);
        return -ENOMEM;
    }

    p_module_priv = netdev_priv(p_net_device);

    pr_debug(_MODULE_NAME_TO_PR "net_open\n");

    /* get the IRQ,  43 is correct, 17 is wrong
     * second arg is interrupt handler
     * third is flags, 0 means no IRQ sharing
     */
    if (0 != request_irq(p_module_priv->p_pci_dev->irq, _irq_handler, 0, p_net_device->name, p_net_device))
    {
        pr_err(_MODULE_NAME_TO_PR "%s request_irq failed \n", __func__);
        goto irq_free;
    }

    pr_info(_MODULE_NAME_TO_PR "NET dev IRQ:%d, PCI dev IRQ:%d\n", p_net_device->irq, p_module_priv->p_pci_dev->irq);

    /* get memory for Tx buffers
     * memory must be DMAable
     */
    p_module_priv->tx_desc_bufs_ = pci_alloc_consistent(p_module_priv->p_pci_dev, TX_BUF_TOTAL_SIZE, &p_module_priv->tx_desc_buf_dma);

    if (IS_ERR(p_module_priv->tx_desc_bufs_)) {
        pr_err(_MODULE_NAME_TO_PR "%s tx_desc_bufs_ pci_alloc_consistent failed \n", __func__);
        goto irq_free;
    }

    p_module_priv->tx_flag = 0;

    /* get memory for Rx buffers*/
    p_module_priv->rx_ring_buf = pci_alloc_consistent(p_module_priv->p_pci_dev, RX_BUF_TOT_LEN, &p_module_priv->rx_ring_buf_dma);

    if (IS_ERR(p_module_priv->rx_ring_buf)) {
        pr_err(_MODULE_NAME_TO_PR "%s rx_ring_buf pci_alloc_consistent failed \n", __func__);

        if(p_module_priv->tx_desc_bufs_) {
            pci_free_consistent(p_module_priv->p_pci_dev, TX_BUF_TOTAL_SIZE, p_module_priv->tx_desc_bufs_, p_module_priv->tx_desc_buf_dma);
            p_module_priv->tx_desc_bufs_ = NULL;
        }

        goto irq_free;
    }

    // initialize the tx ring
    _init_ring(p_net_device);
    _hw_start(p_net_device);
        
    return 0;

irq_free:
    free_irq(p_module_priv->p_pci_dev->irq, p_net_device);

    return -ENOMEM;
}

static int net_close(struct net_device *p_net_device)
{
    pr_info(_MODULE_NAME_TO_PR "net_close\n");

    return 0;
}

static netdev_tx_t net_start_xmit(struct sk_buff *skb, struct net_device *p_net_device)
{
    void *ioaddr = NULL;

    u32 entry = 0;

    size_t len = 0;

    _module_priv_t *p_module_priv = NULL;

    if ((IS_ERR(p_net_device)) ||  (IS_ERR(skb))) {
        pr_err(_MODULE_NAME_TO_PR "%s p_net_device/skb is NULL \n", __func__);
        return -ENOMEM;
    }

    ioaddr = p_module_priv->iomem;

    entry = p_module_priv->tx_current_desc;

    len = skb->len;

    p_module_priv = netdev_priv(p_net_device);

    pr_debug(_MODULE_NAME_TO_PR "net_start_xmit\n");

    if (len < TX_BUF_SIZE) {
        if(len < FRAME_MIN_SIZE) {
            memset(p_module_priv->tx_desc_buf[entry], 0, FRAME_MIN_SIZE);    
        }        

        skb_copy_and_csum_dev(skb, p_module_priv->tx_desc_buf[entry]);
        dev_kfree_skb(skb);
    } 
    else {
        dev_kfree_skb(skb);
        return 0;
    }

    writel(p_module_priv->tx_flag | max(len, (size_t)FRAME_MIN_SIZE), 
                       ioaddr + TSD0_REG_OFFSET + (entry * sizeof (u32)));
        entry++;
        p_module_priv->tx_current_desc = entry % TX_BUF_NUM;

    if(p_module_priv->tx_current_desc == p_module_priv->tx_dirty_desc) {
        netif_stop_queue(p_net_device);
    }

    return 0;
}

static struct net_device_stats *net_get_stats(struct net_device *p_net_device)
{
    _module_priv_t *p_module_priv = NULL;

    pr_debug(_MODULE_NAME_TO_PR "net_get_stats\n");
    
    p_module_priv = netdev_priv(p_net_device);

    return &(p_module_priv->_net_device_stats);
}


static const struct net_device_ops _net_device_ops = {
    .ndo_open       = net_open,
    .ndo_stop       = net_close,
    .ndo_start_xmit = net_start_xmit,
    .ndo_get_stats  = net_get_stats
};


static int pci_probe(struct pci_dev *p_pci_dev, const struct pci_device_id *ent)
{
    const u8 c_broadcast = 0xff;

    u32 i = 0;
    const u32 c_bar1 = 1;
    const u32 c_size_addr = 6;
    
    const u32 c_hard_header_len = 14;

    unsigned long mmio_start = 0, 
                  mmio_end = 0, 
                  mmio_len = 0, 
                  mmio_flags = 0;

    void *ioaddr = NULL;

    struct net_device *p_net_device = NULL;
    _module_priv_t *p_module_priv = NULL;

    if ((IS_ERR(p_pci_dev)) || (IS_ERR(ent))) {
        pr_err(_MODULE_NAME_TO_PR "%s p_pci_dev/ent is NULL \n", __func__);

        return -ENOMEM;
    }

    pr_info(_MODULE_NAME_TO_PR "Vender ID: 0x%x, Device ID: 0x%x\n", pci_ids->vendor, pci_ids->device);

    if (0 != pci_enable_device_mem(p_pci_dev)) {
        pr_err(_MODULE_NAME_TO_PR "%s pci_enable_device_mem failed \n", __func__);

        return -ENOMEM;
    }

    if (0 != pci_request_regions(p_pci_dev, _MODULE_NAME)) {
        pr_err(_MODULE_NAME_TO_PR "%s pci_enable_device_mem failed \n", __func__);
        goto pci_disable;
    }

    pci_set_master(p_pci_dev);

    p_net_device = alloc_etherdev(sizeof(_module_priv_t));
    if (IS_ERR(p_net_device)) {
        pr_err(_MODULE_NAME_TO_PR "%s alloc_etherdev failed \n", __func__);
        goto pci_release;
    }


    /* Set net device's parent device to PCI device */
    SET_NETDEV_DEV(p_net_device, &p_pci_dev->dev);
    
    pci_set_drvdata(p_pci_dev, p_net_device);

    p_module_priv = netdev_priv(p_net_device);
    p_module_priv->p_pci_dev = p_pci_dev;
    p_module_priv->p_net_device = p_net_device;

    /* get mem map io address */
    mmio_start = pci_resource_start(p_pci_dev, c_bar1);
    mmio_end = pci_resource_end(p_pci_dev, c_bar1);
    mmio_len = pci_resource_len(p_pci_dev, c_bar1);
    mmio_flags = pci_resource_flags(p_pci_dev, c_bar1);

    /* ioremap MMI/O region */
    ioaddr = ioremap(mmio_start, mmio_len);
    if(IS_ERR(ioaddr)) {
        pr_err(_MODULE_NAME_TO_PR "%s ioremap failed \n", __func__);
        goto free_net_dev;
    }

    // set private data
    p_net_device->base_addr = (long)ioaddr;
    p_module_priv->iomem = ioaddr;
    p_module_priv->iomem_len = mmio_len;

    for(i = 0; i < c_size_addr; i++) {  /* Hardware Address */
        p_net_device->dev_addr[i] =  readb((const volatile void*)(p_net_device->base_addr + i));
        p_net_device->broadcast[i] = c_broadcast;
    }

    p_net_device->hard_header_len = c_hard_header_len;

    /* set driver name and irq */
    memcpy(p_net_device->name, _MODULE_NAME, sizeof(_MODULE_NAME)); /* Device Name */ // why not srtcpy??
    pr_info(_MODULE_NAME_TO_PR "MAC: %d, IRQ number: %d\n", *p_net_device->dev_addr, p_module_priv->p_pci_dev->irq);

    p_net_device->netdev_ops = &_net_device_ops;
    
    if (0 != register_netdev(p_net_device)) {
        pr_err(_MODULE_NAME_TO_PR "%s register_netdev failed \n", __func__);
        goto free_net_dev;
    }

    return 0;

free_net_dev:
    iounmap(p_module_priv->iomem);
    free_netdev(p_net_device);

pci_release:
    pci_release_regions(p_pci_dev);

pci_disable:
    pci_disable_device(p_pci_dev);

    return -ENOMEM;
}

static void pci_remove(struct pci_dev *p_pci_dev)
{
    struct net_device *p_net_device = NULL;
    _module_priv_t *p_module_priv = NULL;

    if (IS_ERR(p_pci_dev)) {
        pr_err(_MODULE_NAME_TO_PR "%s p_pci_dev is NULL \n", __func__);

        return;
    }

    p_net_device = pci_get_drvdata(p_pci_dev);
    p_module_priv = netdev_priv(p_net_device);

    free_irq(p_module_priv->p_pci_dev->irq, p_net_device);
    iounmap(p_module_priv->iomem);

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
    pr_info(_MODULE_NAME_TO_PR "Device driver was inserted\n");
    
    return pci_register_driver(&_pci_driver_ops);
}

static void __exit _eexit(void)
{
    pci_unregister_driver(&_pci_driver_ops);
    pr_info(_MODULE_NAME_TO_PR "Device driver was removed\n");
}


MODULE_LICENSE(_MODULE_LICENSE);
MODULE_AUTHOR("Artem Shimko");
MODULE_DESCRIPTION(_MODULE_DESCRIPTION);
MODULE_VERSION(_MODULE_VERSION);


module_init(_init);
module_exit(_eexit);