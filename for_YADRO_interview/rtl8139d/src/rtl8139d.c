#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/stddef.h>
#include <linux/pci.h>

#define _MODULE_NAME           "realtek8139"
#define _MODULE_NAME_TO_PR     "realtek8139: "
#define _MODULE_CLASS          "NIC"

#define REALTEK_VENDOR_ID 0x10EC
#define REALTEK_DEVICE_ID 0x8139


struct priv_data
{
    struct pci_dev p_dev;
    void mmio_addr;             // memory mapped I/O addr
    unsigned long int regs_len; // len of I/O MMI/O region
};

static struct net_device *p_rtl8139_dev;


static struct pci_dev* pci_dev_present(void)
{
    struct pci_dev *p_dev = NULL;

    if (0 == pci_present()) {
    	pr_err(_MODULE_NAME_TO_PR "%s: " "pci is not supported\n",  __func__);
    	return NULL;
    }


    p_dev = pci_find_device(REALTEK_VENDOR_ID, REALTEK_VENDOR_ID, NULL);

    if (0 != IS_ERR(p_dev)) {
    	pr_err(_MODULE_NAME_TO_PR "%s: " "pci_find_device failed\n",  __func__);
    	return NULL;
    }
    else {

    	if (0 != pci_enable_device(p_dev)) {
    		pr_err(_MODULE_NAME_TO_PR "%s: " "pci_enable_device failed\n",  __func__);
    		return NULL
    	}
    	
    	pr_info(_MODULE_NAME_TO_PR "%s: " "pci_enable_device success\n",  __func__);
    }

	return p_dev;
}

static int rtl8139_init(struct pci_dev *pdev, struct net_device **p_dev_out)
{
	struct net_device *p_dev = NULL;
	struct priv_data *p_priv_data = NULL;

    /* 
     * alloc_etherdev allocates memory for dev and dev->priv.
     * dev->priv shall have sizeof(struct rtl8139_private) memory
     * allocated.
     */
    p_dev = alloc_etherdev(sizeof(struct priv_data));
    if (!dev) {
        pr_err(_MODULE_NAME_TO_PR "%s: " "alloc_etherdev failed\n",  __func__);
        return -1;
    }

    p_priv_data = dev->priv;
    p_priv_data->pci_dev = pdev;
    *p_dev_out = p_dev;

    return 0;
}

static int rtl8139_open(struct net_device *dev) 
{ 
	pr_info(_MODULE_NAME_TO_PR "%s: " "called\n",  __func__);

    return 0; 
}

static int rtl8139_stop(struct net_device *dev) 
{
    pr_info(_MODULE_NAME_TO_PR "%s: " "called\n",  __func__);
    return 0;
}

static int rtl8139_start_xmit(struct sk_buff *skb, struct net_device *dev) 
{
    pr_info(_MODULE_NAME_TO_PR "%s: " "called\n",  __func__);
    return 0;
}

static struct net_device_stats* rtl8139_get_stats(struct net_device *dev) 
{
    pr_info(_MODULE_NAME_TO_PR "%s: " "called\n",  __func__);
    return 0;
}

int __init _init(void)
{
    struct pci_dev *pdev = NULL;

    unsigned long mmio_start = 0,
                  mmio_end = 0,
                  mmio_len = 0,
                  mmio_flags = 0;

    void *ioaddr = NULL;
    int i;

    struct priv_data *p_priv_data = NULL;

    pdev = pci_dev_present();

    if (NULL == pdev) {
    	return -1;
    }

    if (rtl8139_init(p_dev, p_rtl8139_dev)) {
    	pr_err(_MODULE_NAME_TO_PR "%s: " "rtl8139_init failed\n",  __func__);
    	return 0;
    }

    p_priv_data = p_rtl8139_dev->priv;

    mmio_start = pci_resource_start(pdev, 1);
    mmio_end = pci_resource_end(pdev, 1);
    mmio_len = pci_resource_len(pdev, 1);
    mmio_flags = pci_resource_flags(pdev, 1);

    /* make sure above region is MMI/O */
    if (!(mmio_flags & IORESOURCE_MEM)) {
    	pr_err(_MODULE_NAME_TO_PR "%s: " "region not MMI/O region\n",  __func__);
    	goto cleanup;
    }

    /* get PCI memory space */
    if (pci_request_regions(pdev, DRIVER)) {
    	pr_err(_MODULE_NAME_TO_PR "%s: " "pci_request_regions failed\n",  __func__);
    	goto cleanup;
    }

    pci_set_master(pdev);

    ioaddr = ioremap(mmio_start, mmio_len);
    if (!ioaddr) {
    	pr_err(_MODULE_NAME_TO_PR "%s: " "ioremap failed\n",  __func__);
    	goto cleanup;
    }


    p_rtl8139_dev->base_addr = (long) ioaddr;
    p_priv_data->mmio_addr = ioaddr;
    p_priv_data->regs_len = mmio_len;

    for (i = 0; i < 6; i++) {
    	p_rtl8139_dev->dev_addr[i] = readb(p_rtl8139_dev->base_addr + i);
    	p_rtl8139_dev->broadcast[i] = 0xff;
    }

    p_rtl8139_dev->hard_header_len = 14;

    memcpy(p_rtl8139_dev->name, DRIVER, sizeof(DRIVER));
    p_rtl8139_dev->irq = pdev->irq;
    p_rtl8139_dev->open = rtl8139_open;
    p_rtl8139_dev->stop = rtl8139_stop;
    p_rtl8139_dev->hard_start_xmit = rtl8139_hard_start_xmit;
    p_rtl8139_dev->get_stats = rtl8139_get_stats;

    /* register the device */
    if (register_netdev(rtl8139_dev)) {
    	pr_err(_MODULE_NAME_TO_PR "%s: " "register_netdev failed\n",  __func__);
    	goto cleanup;
    }

	return 0;

cleanup:
	iounmap(tp->mmio_addr);
    pci_release_regions(tp->pci_dev);
    unregister_netdev(p_rtl8139_dev);
    pci_disable_device(tp->pci_dev);

    return -1;
}

void cleanup_module(void) 
{
    struct rtl8139_private *tp;
    tp = p_rtl8139_dev->priv;

    iounmap(tp->mmio_addr);
    pci_release_regions(tp->pci_dev);

    unregister_netdev(p_rtl8139_dev);
    pci_disable_device(tp->pci_dev);
    return;
}