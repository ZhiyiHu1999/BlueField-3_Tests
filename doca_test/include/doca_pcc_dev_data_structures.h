/*
 * Copyright (c) 2023 NVIDIA CORPORATION & AFFILIATES, ALL RIGHTS RESERVED.
 *
 * This software product is a proprietary product of NVIDIA CORPORATION &
 * AFFILIATES (the "Company") and all right, title, and interest in and to the
 * software product, including all associated intellectual property rights, are
 * and shall remain exclusively with the Company.
 *
 * This software product is governed by the End User License Agreement
 * provided with the software product.
 *
 */

/**
 * @ingroup DOCA_PCC_DEVICE
 *
 * @{
 */

#ifndef DOCA_PCC_DEV_DATA_STRUCTURES_H_
#define DOCA_PCC_DEV_DATA_STRUCTURES_H_

#if __NV_DPA == __NV_DPA_BF3
#include <doca_pcc_dev_data_structure_le_bf3.h>
#elif __NV_DPA == __NV_DPA_CX8
#include <doca_pcc_dev_data_structure_le_cx8.h>
#else
#error "Must supply '-mcpu' compiler option on command line"
#endif

typedef struct mp_ctxt_t doca_pcc_dev_mp_ctx_t;
typedef struct mlnx_cc_algo_ctxt_t doca_pcc_dev_algo_ctxt_t;
typedef struct mlnx_cc_attr_t doca_pcc_dev_attr_t;
typedef struct mlnx_cc_event_t doca_pcc_dev_event_t;
typedef struct mlnx_cc_event_general_attr_t doca_pcc_dev_event_general_attr_t;
typedef struct mlnx_cc_roce_tx_cntrs_t doca_pcc_dev_roce_tx_cntrs_t;
typedef struct mlnx_cc_roce_tx_t doca_pcc_dev_roce_tx_t;
typedef struct mlnx_cc_ack_nack_cnp_extra_t doca_pcc_dev_ack_nack_cnp_extra_t;
typedef struct mlnx_cc_ack_nack_cnp_t doca_pcc_dev_ack_nack_cnp_t;
typedef struct mlnx_cc_rtt_tstamp_t doca_pcc_dev_rtt_tstamp_t;
typedef struct mlnx_cc_fw_data_t doca_pcc_dev_fw_data_t;
typedef union mlnx_cc_event_spec_attr_t doca_pcc_dev_event_spec_attr_t;

#if __NV_DPA == __NV_DPA_BF3
typedef struct mlnx_cc_roce_tx_extra_t doca_pcc_dev_roce_tx_extra_t;
#elif __NV_DPA >= __NV_DPA_CX8
typedef struct mlnx_cc_event_general_dword2_t doca_pcc_dev_event_general_dword2_t;
typedef struct mlnx_cc_rtt_spec_data0_t doca_pcc_dev_rtt_spec_data0_t;
typedef struct mlnx_cc_rtt_spec_data1_t doca_pcc_dev_rtt_spec_data1_t;
typedef struct mlnx_cc_rtt_spec_data2_t doca_pcc_dev_rtt_spec_data2_t;
typedef struct mlnx_cc_rtt_spec_data3_t doca_pcc_dev_rtt_spec_data3_t;
#endif

/** @} */

#endif /* DOCA_PCC_DEV_DATA_STRUCTURES_H_ */
