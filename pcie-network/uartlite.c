#include <linux/module.h>
#include <linux/init.h>
#include <linux/pci.h>

#define _MODULE_NAME         "kcu105_base"
#define _MODULE_NAME_TO_RP   "kcu105_base: "

#define _PCI_VENDER_ID 0x10EE
#define _PCI_DEVICE_ID 0x8024

#define RX_FIFO_REG 0x0000U
#define TX_FIFO_REG 0x0004U
#define STAT_REG    0x0008U
#define CTRL_REG    0x000CU

static struct pci_device_id ids[] = {
    {PCI_DEVICE(_PCI_VENDER_ID, _PCI_DEVICE_ID)},
    {0,}
};

MODULE_DEVICE_TABLE(pci, ids);

typedef struct _module_priv {
    u8 __iomem *hwmem;
}_module_priv_t;

static int __devinit _probe(struct pci_dev *p_dev, const struct pci_device_id *p_id)
{