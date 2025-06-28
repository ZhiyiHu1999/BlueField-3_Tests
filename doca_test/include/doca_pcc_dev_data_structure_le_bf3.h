/*
 * Copyright (c) 2024 NVIDIA CORPORATION & AFFILIATES, ALL RIGHTS RESERVED.
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


#ifndef DOCA_PCC_DEV_DATA_STRUCTURE_LE_BF3_H_
#define DOCA_PCC_DEV_DATA_STRUCTURE_LE_BF3_H_

#include <stdint.h>

struct mlnx_cc_ack_nack_cnp_extra_t {	/* Little Endian */
    uint32_t			num_coalesced:16;			/* number of coalesced events, incremented on each coalesced event */
	/* access: RW */
    uint32_t			reserved_at_0:16;
	/* access: RW */
/* --------------------------------------------------------- */
};

struct mlnx_cc_roce_tx_extra_t {	/* Little Endian */
    uint32_t			flow_qpn:24;			/* flow qp number */
	/* access: RW */
    uint32_t			reserved_at_0:8;
	/* access: RW */
/* --------------------------------------------------------- */
};

struct mlnx_cc_roce_tx_cntrs_t {	/* Little Endian */
    uint32_t			sent_32bytes:16;			/* sent 32 bytes amount, additive increase on each event */
	/* access: RW */
    uint32_t			sent_pkts:16;			/* sent packets amount, additive increase on each event */
	/* access: RW */
/* --------------------------------------------------------- */
};

struct mlnx_cc_fw_data_t {	/* Little Endian */
    uint32_t			data[3];			/* 3 dword fw data */
	/* access: RW */
/* --------------------------------------------------------- */
};

struct mlnx_cc_rtt_tstamp_t {	/* Little Endian */
    uint32_t			req_send_timestamp;			/* request send timestamp */
	/* access: RW */
	/*----------------------------------------------------------*/
    uint32_t			req_recv_timestamp;			/* request receive timestamp */
	/* access: RW */
	/*----------------------------------------------------------*/
    uint32_t			resp_send_timestamp;			/* response send timestamp */
	/* access: RW */
	/*----------------------------------------------------------*/
    unsigned char			reserved_at_60[4];			
	/* access: RW */
/* --------------------------------------------------------- */
};

struct mlnx_cc_ack_nack_cnp_t {	/* Little Endian */
    uint32_t			first_timestamp;			/* first coalesced event timestamp */
	/* access: RW */
	/*----------------------------------------------------------*/
    uint32_t			first_sn;			/* first coalesced event serial number */
	/* access: RW */
	/*----------------------------------------------------------*/
    struct mlnx_cc_ack_nack_cnp_extra_t			extra;			/* extra attributes */
	/* access: RW */
	/*----------------------------------------------------------*/
    unsigned char			reserved_at_60[4];			
	/* access: RW */
/* --------------------------------------------------------- */
};

struct mlnx_cc_roce_tx_t {	/* Little Endian */
    uint32_t			first_timestamp;			/* first coalesced event timestamp */
	/* access: RW */
	/*----------------------------------------------------------*/
    struct mlnx_cc_roce_tx_cntrs_t			cntrs;			/* tx counters */
	/* access: RW */
	/*----------------------------------------------------------*/
    struct mlnx_cc_roce_tx_extra_t			extra;			
	/* access: RW */
	/*----------------------------------------------------------*/
    unsigned char			reserved_at_60[4];			
	/* access: RW */
/* --------------------------------------------------------- */
};

union mlnx_cc_event_spec_attr_t {	/* Little Endian */
    struct mlnx_cc_roce_tx_t			roce_tx;			/* tx attributes */
	/* access: RW */
	/*----------------------------------------------------------*/
    struct mlnx_cc_ack_nack_cnp_t			ack_nack_cnp;			/* ack/nack/cnp attributes */
	/* access: RW */
	/*----------------------------------------------------------*/
    struct mlnx_cc_rtt_tstamp_t			rtt_tstamp;			/* rtt timestamp */
	/* access: RW */
	/*----------------------------------------------------------*/
    struct mlnx_cc_fw_data_t			fw_data;			/* fw data */
	/* access: RW */
	/*----------------------------------------------------------*/
    unsigned char			reserved_at_0[16];			
	/* access: RW */
/* --------------------------------------------------------- */
};

struct mlnx_cc_event_general_attr_t {	/* Little Endian */
    uint32_t			ev_type:8;			/* event type */
	/* access: RW */
    uint32_t			ev_subtype:8;			/* event subtype */
	/* access: RW */
    uint32_t			port_num:8;			/* port id */
	/* access: RW */
    uint32_t			flags:8;			/* event flags */
	/* access: RW */
/* --------------------------------------------------------- */
};

struct mp_user_ctxt_t {	/* Little Endian */
    uint32_t			data[9];			/* 9 dword user data */
	/* access: RW */
/* --------------------------------------------------------- */
};

struct mp_library_ctxt_t {	/* Little Endian */
    uint32_t			flow_tag;			/* the PCC flowtag of this QP, giving the MP application access to the PCC context */
	/* access: RW */
	/*----------------------------------------------------------*/
    uint32_t			rtc_index:24;			/* the RTC indexes of the path table and state table */
	/* access: RW */
    uint32_t			disabled_ps_counter:8;			/* counts the num of currently disabled PSs. algorithm allows MAX_DISABLED_PSS to be disabled */
	/* access: RW */
	/*----------------------------------------------------------*/
    uint32_t			aso_index:24;			/* the ASO index for SKIP (for CX9 it will hold the multipath ASO index) */
	/* access: RW */
    uint32_t			reserved_at_40:8;			
	/* access: RW */
	/*----------------------------------------------------------*/
    uint32_t			ps_list_pointer:24;			/* pointer to the FW PS list, parse it according to PS byte size and list log size */
	/* access: RW */
    uint32_t			ps_bit_size:6;			
	/* access: RW */
    uint32_t			reserved_at_61:1;			
	/* access: RW */
    uint32_t			valid:1;			
	/* access: RW */
	/*----------------------------------------------------------*/
    uint32_t			ps_ctx_pointer:24;			/* pointer to the FW PS state context, parse it according to ps_ctx_log_num_bits and PS list log size */
	/* access: RW */
    uint32_t			ps_ctx_log_num_bits:4;			
	/* access: RW */
    uint32_t			reserved_at_80:4;			
	/* access: RW */
	/*----------------------------------------------------------*/
    uint32_t			host_ps_state_pointer:24;			
	/* access: RW */
    uint32_t			host_ps_state_log_num_bits:4;			
	/* access: RW */
    uint32_t			reserved_at_a0:4;			
	/* access: RW */
	/*----------------------------------------------------------*/
    uint32_t			ps_list_size:10;			
	/* access: RW */
    uint32_t			reserved_at_c0:22;			
	/* access: RW */
/* --------------------------------------------------------- */
};

struct mlnx_cc_algo_ctxt_t {	/* Little Endian */
    uint32_t			data[12];			/* 12 dword algorithm context */
	/* access: RW */
/* --------------------------------------------------------- */
};

struct val_t {	/* Little Endian */
    uint32_t			val;			/* uint32_t value */
	/* access: RW */
/* --------------------------------------------------------- */
};

struct mlnx_cc_event_t {	/* Little Endian */
    unsigned char			reserved_at_0[12];			
	/* access: RW */
	/*----------------------------------------------------------*/
    struct mlnx_cc_event_general_attr_t			ev_attr;			/* event general attributes */
	/* access: RW */
	/*----------------------------------------------------------*/
    uint32_t			flow_tag;			/* unique flow id */
	/* access: RW */
	/*----------------------------------------------------------*/
    uint32_t			sn;			/* serial number */
	/* access: RW */
	/*----------------------------------------------------------*/
    uint32_t			timestamp;			/* event timestamp */
	/* access: RW */
	/*----------------------------------------------------------*/
    union mlnx_cc_event_spec_attr_t			ev_spec_attr;			/* attributes which are different for different events */
	/* access: RW */
/* --------------------------------------------------------- */
};

struct mp_ctxt_t {	/* Little Endian */
    struct mp_library_ctxt_t			mp_library;			/* 6 dword internal data */
	/* access: RW */
	/*----------------------------------------------------------*/
    struct mp_user_ctxt_t			mp_user;			/* 9 dword user data */
	/* access: RW */
/* --------------------------------------------------------- */
};

struct mlnx_cc_attr_t {	/* Little Endian */
    uint32_t			algo_slot:4;			/* algorithm type defined in API.h, 15 - DCQCN */
	/* access: RW */
    uint32_t			overload:1;			/* overload flag */
	/* access: RW */
    uint32_t			reserved_at_0:27;	
	/* access: RW */
/* --------------------------------------------------------- */
};

union union_mlnx_cc_ack_nack_cnp_extra_t {	/* Little Endian */
    struct val_t			val;			/* entire value */
	/* access: RW */
	/*----------------------------------------------------------*/
    struct mlnx_cc_ack_nack_cnp_extra_t			mlnx_cc_ack_nack_cnp_extra;			/* attributes for ack/nack/cnp */
	/* access: RW */
	/*----------------------------------------------------------*/
    unsigned char			reserved_at_0[4];			
	/* access: RW */
/* --------------------------------------------------------- */
};

union union_mlnx_cc_roce_tx_cntrs_t {	/* Little Endian */
    struct val_t			val;			/* entire value */
	/* access: RW */
	/*----------------------------------------------------------*/
    struct mlnx_cc_roce_tx_cntrs_t			mlnx_cc_roce_tx_cntrs;			/* tx counters */
	/* access: RW */
	/*----------------------------------------------------------*/
    unsigned char			reserved_at_0[4];			
	/* access: RW */
/* --------------------------------------------------------- */
};

union union_mlnx_cc_event_general_attr_t {	/* Little Endian */
    struct val_t			val;			/* entire value */
	/* access: RW */
	/*----------------------------------------------------------*/
    struct mlnx_cc_event_general_attr_t			mlnx_cc_event_general_attr;			/* event general attributes */
	/* access: RW */
	/*----------------------------------------------------------*/
    unsigned char			reserved_at_0[4];			
	/* access: RW */
/* --------------------------------------------------------- */
};

#endif /* DOCA_PCC_DEV_DATA_STRUCTURE_LE_BF3_H_ */
