/* generated vector source file - do not edit */
#include "bsp_api.h"
/* Do not build these data structures if no interrupts are currently allocated because IAR will have build errors. */
#if VECTOR_DATA_IRQ_COUNT > 0
        BSP_DONT_REMOVE const fsp_vector_t g_vector_table[BSP_ICU_VECTOR_NUM_ENTRIES] BSP_PLACE_IN_SECTION(BSP_SECTION_APPLICATION_VECTORS) =
        {
                        [0] = rtc_carry_isr, /* RTC CARRY (Carry interrupt) */
            [1] = glcdc_line_detect_isr, /* GLCDC LINE DETECT (Specified line) */
            [2] = mipi_dsi_seq0_isr, /* MIPIDSI SEQ0 (Sequence operation channel 0 interrupt) */
            [3] = mipi_dsi_seq1_isr, /* MIPIDSI SEQ1 (Sequence operation channel 1 interrupt) */
            [4] = mipi_dsi_vin1_isr, /* MIPIDSI VIN1 (Video-Input operation channel1 interrupt) */
            [5] = mipi_dsi_rcv_isr, /* MIPIDSI RCV (DSI packet receive interrupt) */
            [6] = mipi_dsi_ferr_isr, /* MIPIDSI FERR (DSI fatal error interrupt) */
            [7] = mipi_dsi_ppi_isr, /* MIPIDSI PPI (DSI D-PHY PPI interrupt) */
            [8] = drw_int_isr, /* DRW INT (DRW interrupt) */
            [9] = vin_status_isr, /* VIN IRQ (Interrupt Request) */
            [10] = vin_error_isr, /* VIN ERR (Interrupt Request for SYNC Error) */
            [11] = mipi_csi_rx_isr, /* MIPICSI RX (Receive interrupt) */
            [12] = mipi_csi_dl_isr, /* MIPICSI DL (Data Lane interrupt) */
            [13] = mipi_csi_vc_isr, /* MIPICSI VC (Virtual Channel interrupt) */
            [14] = mipi_csi_pm_isr, /* MIPICSI PM (Power Management interrupt) */
            [15] = mipi_csi_gst_isr, /* MIPICSI GST (Generic Short Packet interrupt) */
            [16] = iic_master_rxi_isr, /* IIC0 RXI (Receive data full) */
            [17] = iic_master_txi_isr, /* IIC0 TXI (Transmit data empty) */
            [18] = iic_master_tei_isr, /* IIC0 TEI (Transmit end) */
            [19] = iic_master_eri_isr, /* IIC0 ERI (Transfer error) */
        };
        #if BSP_FEATURE_ICU_HAS_IELSR
        const bsp_interrupt_event_t g_interrupt_event_link_select[BSP_ICU_VECTOR_NUM_ENTRIES] =
        {
            [0] = BSP_PRV_VECT_ENUM(EVENT_RTC_CARRY,GROUP0), /* RTC CARRY (Carry interrupt) */
            [1] = BSP_PRV_VECT_ENUM(EVENT_GLCDC_LINE_DETECT,GROUP1), /* GLCDC LINE DETECT (Specified line) */
            [2] = BSP_PRV_VECT_ENUM(EVENT_MIPIDSI_SEQ0,GROUP2), /* MIPIDSI SEQ0 (Sequence operation channel 0 interrupt) */
            [3] = BSP_PRV_VECT_ENUM(EVENT_MIPIDSI_SEQ1,GROUP3), /* MIPIDSI SEQ1 (Sequence operation channel 1 interrupt) */
            [4] = BSP_PRV_VECT_ENUM(EVENT_MIPIDSI_VIN1,GROUP4), /* MIPIDSI VIN1 (Video-Input operation channel1 interrupt) */
            [5] = BSP_PRV_VECT_ENUM(EVENT_MIPIDSI_RCV,GROUP5), /* MIPIDSI RCV (DSI packet receive interrupt) */
            [6] = BSP_PRV_VECT_ENUM(EVENT_MIPIDSI_FERR,GROUP6), /* MIPIDSI FERR (DSI fatal error interrupt) */
            [7] = BSP_PRV_VECT_ENUM(EVENT_MIPIDSI_PPI,GROUP7), /* MIPIDSI PPI (DSI D-PHY PPI interrupt) */
            [8] = BSP_PRV_VECT_ENUM(EVENT_DRW_INT,GROUP0), /* DRW INT (DRW interrupt) */
            [9] = BSP_PRV_VECT_ENUM(EVENT_VIN_IRQ,GROUP1), /* VIN IRQ (Interrupt Request) */
            [10] = BSP_PRV_VECT_ENUM(EVENT_VIN_ERR,GROUP2), /* VIN ERR (Interrupt Request for SYNC Error) */
            [11] = BSP_PRV_VECT_ENUM(EVENT_MIPICSI_RX,GROUP3), /* MIPICSI RX (Receive interrupt) */
            [12] = BSP_PRV_VECT_ENUM(EVENT_MIPICSI_DL,GROUP4), /* MIPICSI DL (Data Lane interrupt) */
            [13] = BSP_PRV_VECT_ENUM(EVENT_MIPICSI_VC,GROUP5), /* MIPICSI VC (Virtual Channel interrupt) */
            [14] = BSP_PRV_VECT_ENUM(EVENT_MIPICSI_PM,GROUP6), /* MIPICSI PM (Power Management interrupt) */
            [15] = BSP_PRV_VECT_ENUM(EVENT_MIPICSI_GST,GROUP7), /* MIPICSI GST (Generic Short Packet interrupt) */
            [16] = BSP_PRV_VECT_ENUM(EVENT_IIC0_RXI,GROUP0), /* IIC0 RXI (Receive data full) */
            [17] = BSP_PRV_VECT_ENUM(EVENT_IIC0_TXI,GROUP1), /* IIC0 TXI (Transmit data empty) */
            [18] = BSP_PRV_VECT_ENUM(EVENT_IIC0_TEI,GROUP2), /* IIC0 TEI (Transmit end) */
            [19] = BSP_PRV_VECT_ENUM(EVENT_IIC0_ERI,GROUP3), /* IIC0 ERI (Transfer error) */
        };
        #endif
        #endif
