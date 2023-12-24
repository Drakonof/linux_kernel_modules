#if defined(_RTL8139D_H_) == 0
#define _RTL8139D_H_

/* rtl8139d registers offset relatively PCIe BAR1 */
#define TSD0_REG_OFFSET       0x10U
#define TSAD0_REG_OFFSET      0x20U
#define RBSTART_REG_OFFSET    0x30U
#define CR_REG_OFFSET         0x37U
#define CAPR_REG_OFFSET       0x38U
#define IMR_REG_OFFSET        0x3cU
#define ISR_REG_OFFSET        0x3eU
#define TCR_REG_OFFSET        0x40U
#define RCR_REG_OFFSET        0x44U
#define MPC_REG_OFFSET        0x4cU
#define MULINT_REG_OFFSET     0x5cU

/* Bits of rtl8139d's CR register */
#define CR_RX_BUF_EMPTY_BIT   0x00U
#define CR_TX_EN_BIT          0x02U
#define CR_RX_EN_BIT          0x03U
#define CR_RST_BIT            0x04U

/* Bits of rtl8139d's ISR register */
#define ISR_RX_OK_BIT         0x00U
#define ISR_RX_ERR_BIT        0x01U
#define ISR_TX_OK_BIT         0x02U
#define ISR_TX_ERR_BIT        0x03U
#define ISR_RX_BUF_OW_BIT     0x04U
#define ISR_RX_UDR_RUN_BIT    0x05U
#define ISR_RX_FIFO_OW_BIT    0x06U
#define ISR_LEN_CH_BIT        0x0DU
#define ISR_TIMEOUT_BIT       0x0EU
#define ISR_SYSTEM_ER_BIT     0x0FU

/* Masks of rtl8139d's ISR register */
#define ISR_INTERRUPT_MASK    (                                                                                \
                                  BIT(ISR_RX_OK_BIT)      | BIT(ISR_RX_ERR_BIT)    | BIT(ISR_TX_OK_BIT)      | \
                                  BIT(ISR_TX_ERR_BIT)     | BIT(ISR_RX_BUF_OW_BIT) | BIT(ISR_RX_UDR_RUN_BIT) | \
                                  BIT(ISR_RX_FIFO_OW_BIT) | BIT(ISR_LEN_CH_BIT)    | BIT(ISR_TIMEOUT_BIT)    | \
                                  BIT(ISR_SYSTEM_ER_BIT)                                                       \
                              )

#define ISR_INTERRUPT_CLN     0xffffU

/* Bits of rtl8139d's TSD register */
#define TSD_TX_FIFO_UNR_BIT   0x0EU
#define TSD_TX_OK_BIT         0x0FU
#define TSD_TX_ABORTED_BIT    0x1EU

/* Bits of rtl8139d's RCR register */
#define RCR_APM_BIT           0x01U
#define RCR_AM_BIT            0x02U
#define RCR_AB_BIT            0x03U
#define RCR_WRAP_BIT          0x07U
#define RCR_MXDMA0_BIT        0x08U
#define RCR_RBLEN1_BIT        0x0CU

#define RCR_CONFIG_MASK       (                                                                   \
                                  BIT(RCR_RBLEN1_BIT) | BIT(RCR_MXDMA0_BIT) | BIT(RCR_WRAP_BIT) | \
                                  BIT(RCR_AB_BIT)     | BIT(RCR_AM_BIT)     | BIT(RCR_APM_BIT)    \
                              )

#define FRAME_MIN_SIZE        0x3CU /* 60 bytes */

#define TX_BUF_SIZE           0x600U  // should be at least MTU + 14 + 4 ??
#define TX_BUF_NUM            0x04U
#define TX_BUF_TOTAL_SIZE     (TX_BUF_SIZE * TX_BUF_NUM)


#define RX_BUF_SIZE_IDX       0x02U // 0==8K, 1==16K, 2==32K, 3==64K ??
#define RX_BUF_SIZE           (0x2000U < RX_BUF_SIZE_IDX) // ??
#define RX_BUF_PAD            0x10U  // see 11th and 12th bit of RCR: 0x44 ??
#define RX_BUF_PKT_PAD        0x800U  // spare padding to handle pkt wrap ??
#define RX_BUF_TOT_LEN        (RX_BUF_SIZE + RX_BUF_PAD + RX_BUF_PKT_PAD)


#endif