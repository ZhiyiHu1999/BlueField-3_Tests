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
 * @file doca_eth_txq_cpu_data_path.h
 * @page DOCA_ETH_TXQ_CPU_DATA_PATH
 * @defgroup DOCA_ETH_TXQ_CPU_DATA_PATH DOCA ETH TXQ CPU Data Path
 * @ingroup DOCA_ETH_TXQ
 * DOCA ETH TXQ library.
 *
 * @{
 */
#ifndef DOCA_ETH_TXQ_CPU_DATA_PATH_H_
#define DOCA_ETH_TXQ_CPU_DATA_PATH_H_

#include <doca_compat.h>
#include <doca_error.h>
#include <doca_pe.h>
#include <doca_types.h>

#ifdef __cplusplus
extern "C" {
#endif

/**********************************************************************************************************************
 * DOCA core opaque types
 *********************************************************************************************************************/

struct doca_buf;
struct doca_task_batch;

/**
 * Opaque structure representing a DOCA ETH TXQ instance.
 */
struct doca_eth_txq;

/**
 * Opaque structures representing DOCA ETH TXQ tasks.
 */
struct doca_eth_txq_task_send;	   /** DOCA ETH TXQ task for transmitting a packet */
struct doca_eth_txq_task_lso_send; /** DOCA ETH TXQ task for transmitting an LSO packet */

/**
 * TX offload flags (relevant only to CPU data-path).
 */
enum doca_eth_txq_ol_flags {
	DOCA_ETH_TXQ_OL_FLAGS_L3_CSUM = (1 << 0), /**< L3 checksum enabled */
	DOCA_ETH_TXQ_OL_FLAGS_L4_CSUM = (1 << 1), /**< L4 checksum enabled */
};

/**
 * @brief Function to execute on task completion.
 *
 * @details This function is called by doca_pe_progress() when related task identified as completed successfully.
 * When this function called the ownership of the task object passed from DOCA back to user.
 * Inside this callback user may decide on the task object:
 * - re-submit task with doca_task_submit(); task object ownership passed to DOCA
 * - release task with doca_task_free(); task object ownership passed to DOCA
 * - keep the task object for future re-use; user keeps the ownership on the task object
 * Inside this callback the user shouldn't call doca_pe_progress().
 * Please see doca_pe_progress for details.
 *
 * Any failure/error inside this function should be handled internally or differed;
 * due to the mode of nested in doca_pe_progress() execution this callback doesn't return error.
 *
 * NOTE: this callback type utilized successful & failed task completions.
 *
 * @param [in] task_send
 * The successfully completed task.
 * The implementation can assume this value is not NULL.
 * @param [in] task_user_data
 * user_data attached to the task.
 * @param [in] ctx_user_data
 * user_data attached to the ctx.
 */
typedef void (*doca_eth_txq_task_send_completion_cb_t)(struct doca_eth_txq_task_send *task_send,
						       union doca_data task_user_data,
						       union doca_data ctx_user_data);

/**
 * @brief Function to execute on task completion.
 *
 * @param [in] task_lso_send
 * The successfully completed task.
 * The implementation can assume this value is not NULL.
 * @param [in] task_user_data
 * user_data attached to the task.
 * @param [in] ctx_user_data
 * user_data attached to the ctx.
 */
typedef void (*doca_eth_txq_task_lso_send_completion_cb_t)(struct doca_eth_txq_task_lso_send *task_lso_send,
							   union doca_data task_user_data,
							   union doca_data ctx_user_data);

/**
 * @brief Function to execute on task_batch completion.
 *
 * Any failure/error inside this function should be handled internally or differed;
 * due to the mode of nested in doca_pe_progress() execution this callback doesn't return error.
 *
 * NOTE: this callback type utilized successful & failed task completions.
 *
 * @param [in] task_batch
 * The completed task_batch.
 * The implementation can assume this value is not NULL.
 * @param [in] tasks_num
 * The number of tasks in task_batch.
 * @param [in] ctx_user_data
 * user_data attached to the ctx.
 * @param [in] task_batch_user_data
 * user_data attached to the task_batch.
 * @param [in] task_user_data_array
 * Array of user_data attached to the tasks.
 * @param [in] pkt_array
 * Array of packets of send tasks.
 * @param [in] status_array
 * Array of status of send tasks.
 */
typedef void (*doca_eth_txq_task_batch_send_completion_cb_t)(struct doca_task_batch *task_batch,
							     uint16_t tasks_num,
							     union doca_data ctx_user_data,
							     union doca_data task_batch_user_data,
							     union doca_data *task_user_data_array,
							     struct doca_buf **pkt_array,
							     doca_error_t *status_array);

/**
 * @brief Function to execute on task_batch completion.
 *
 * Any failure/error inside this function should be handled internally or differed;
 * due to the mode of nested in doca_pe_progress() execution this callback doesn't return error.
 *
 * NOTE: this callback type utilized successful & failed task completions.
 *
 * @param [in] task_batch
 * The completed task_batch.
 * The implementation can assume this value is not NULL.
 * @param [in] tasks_num
 * The number of tasks in task_batch.
 * @param [in] ctx_user_data
 * user_data attached to the ctx.
 * @param [in] task_batch_user_data
 * user_data attached to the task_batch.
 * @param [in] task_user_data_array
 * Array of user_data attached to the tasks.
 * @param [in] pkt_payload_array
 * Array of packets payload of LSO send tasks.
 * @param [in] headers_array
 * Array of headers of LSO send tasks.
 * @param [in] status_array
 * Array of status of LSO send tasks.
 */
typedef void (*doca_eth_txq_task_batch_lso_send_completion_cb_t)(struct doca_task_batch *task_batch,
								 uint16_t tasks_num,
								 union doca_data ctx_user_data,
								 union doca_data task_batch_user_data,
								 union doca_data *task_user_data_array,
								 struct doca_buf **pkt_payload_array,
								 struct doca_gather_list **headers_array,
								 doca_error_t *status_array);

/**
 * @brief This method sets the doca_eth_txq_task_send tasks configuration.
 *
 * @note Supported for DOCA ETH TXQ instance for CPU only.
 *
 * @param [in] eth_txq
 * Pointer to doca_eth_txq instance.
 * @param [in] task_completion_cb
 * Task completion callback.
 * @param [in] task_error_cb
 * Task error callback.
 * @param [in] task_send_num
 * Number of doca_eth_txq_task_send tasks.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * doca_error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - if an invalid parameter was given.
 * - DOCA_ERROR_BAD_STATE - eth_txq context state is not idle.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_eth_txq_task_send_set_conf(struct doca_eth_txq *eth_txq,
					     doca_eth_txq_task_send_completion_cb_t task_completion_cb,
					     doca_eth_txq_task_send_completion_cb_t task_error_cb,
					     uint32_t task_send_num);

/**
 * @brief This method sets the doca_eth_txq_task_lso_send tasks configuration.
 *
 * @note Supported for DOCA ETH TXQ instance for CPU only.
 *
 * @param [in] eth_txq
 * Pointer to doca_eth_txq instance.
 * @param [in] task_completion_cb
 * Task completion callback.
 * @param [in] task_error_cb
 * Task error callback.
 * @param [in] task_lso_send_num
 * Number of doca_eth_txq_task_lso_send tasks.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * doca_error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - if an invalid parameter was given.
 * - DOCA_ERROR_BAD_STATE - eth_txq context state is not idle.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_eth_txq_task_lso_send_set_conf(struct doca_eth_txq *eth_txq,
						 doca_eth_txq_task_lso_send_completion_cb_t task_completion_cb,
						 doca_eth_txq_task_lso_send_completion_cb_t task_error_cb,
						 uint32_t task_lso_send_num);

/**
 * @brief This method sets the task_batch of send tasks configuration.
 *
 * @note Supported for DOCA ETH TXQ instance for CPU only.
 *
 * @param [in] eth_txq
 * Pointer to doca_eth_txq instance.
 * @param [in] max_tasks_number
 * Maximum number of tasks in each task_batch.
 * @param [in] num_task_batches
 * Number of task_batch for send tasks.
 * @param [in] success_completion_cb
 * Task batch successful completion callback.
 * @param [in] error_completion_cb
 * Task batch error completion callback.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * doca_error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - if an invalid parameter was given.
 * - DOCA_ERROR_BAD_STATE - eth_txq context state is not idle.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_eth_txq_task_batch_send_set_conf(struct doca_eth_txq *eth_txq,
						   enum doca_task_batch_max_tasks_number max_tasks_number,
						   uint16_t num_task_batches,
						   doca_eth_txq_task_batch_send_completion_cb_t success_completion_cb,
						   doca_eth_txq_task_batch_send_completion_cb_t error_completion_cb);

/**
 * @brief This method sets the task_batch of LSO send tasks configuration.
 *
 * @note Supported for DOCA ETH TXQ instance for CPU only.
 *
 * @param [in] eth_txq
 * Pointer to doca_eth_txq instance.
 * @param [in] max_tasks_number
 * Maximum number of tasks in each task_batch.
 * @param [in] num_task_batches
 * Number of task_batch for LSO send tasks.
 * @param [in] success_completion_cb
 * Task batch successful completion callback.
 * @param [in] error_completion_cb
 * Task batch error completion callback.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * doca_error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - if an invalid parameter was given.
 * - DOCA_ERROR_BAD_STATE - eth_txq context state is not idle.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_eth_txq_task_batch_lso_send_set_conf(
	struct doca_eth_txq *eth_txq,
	enum doca_task_batch_max_tasks_number max_tasks_number,
	uint16_t num_task_batches,
	doca_eth_txq_task_batch_lso_send_completion_cb_t success_completion_cb,
	doca_eth_txq_task_batch_lso_send_completion_cb_t error_completion_cb);

/**
 * @brief This method expands the number of doca_eth_txq_task_send tasks.
 *
 * @note Supported for DOCA ETH TXQ instance for CPU only.
 *
 * @param [in] eth_txq
 * Pointer to doca_eth_txq instance.
 * @param [in] task_send_num
 * Number of doca_eth_txq_task_send tasks to expand.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * doca_error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - if an invalid parameter was given.
 * - DOCA_ERROR_BAD_STATE - eth_txq context state is not running, or task is not configured.
 * - DOCA_ERROR_NO_MEMORY - Failed to allocate more tasks.
 * - DOCA_ERROR_TOO_BIG - New num tasks exceed limit.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_eth_txq_task_send_num_expand(struct doca_eth_txq *eth_txq, uint32_t task_send_num);

/**
 * @brief This method expands the number of doca_eth_txq_task_lso_send tasks
 *
 * @note Supported for DOCA ETH TXQ instance for CPU only.
 *
 * @param [in] eth_txq
 * Pointer to doca_eth_txq instance.
 * @param [in] task_lso_send_num
 * Number of doca_eth_txq_task_lso_send tasks to expand.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * doca_error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - if an invalid parameter was given.
 * - DOCA_ERROR_BAD_STATE - eth_txq context state is not running, or task is not configured.
 * - DOCA_ERROR_NO_MEMORY - Failed to allocate more tasks.
 * - DOCA_ERROR_TOO_BIG - New num tasks exceed limit.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_eth_txq_task_lso_send_num_expand(struct doca_eth_txq *eth_txq, uint32_t task_lso_send_num);

/**
 * @brief This method expands the number of doca_eth_txq_task_batch_send tasks.
 *
 * @note Supported for DOCA ETH TXQ instance for CPU only.
 *
 * @param [in] eth_txq
 * Pointer to doca_eth_txq instance.
 * @param [in] task_batches_num
 * Number of task batches to expand.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * doca_error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - if an invalid parameter was given.
 * - DOCA_ERROR_BAD_STATE - eth_txq context state is not running, or task batch is not configured.
 * - DOCA_ERROR_NO_MEMORY - Failed to allocate more task batches.
 * - DOCA_ERROR_TOO_BIG - New num task batches exceed limit.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_eth_txq_task_batch_send_num_expand(struct doca_eth_txq *eth_txq, uint16_t task_batches_num);

/**
 * @brief This method expands the number of doca_eth_txq_task_batch_lso_send tasks.
 *
 * @note Supported for DOCA ETH TXQ instance for CPU only.
 *
 * @param [in] eth_txq
 * Pointer to doca_eth_txq instance.
 * @param [in]  task_batches_num
 * Number of task batches to expand.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * doca_error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - if an invalid parameter was given.
 * - DOCA_ERROR_BAD_STATE - eth_txq context state is not running, or task batch is not configured.
 * - DOCA_ERROR_NO_MEMORY - Failed to allocate more task batches.
 * - DOCA_ERROR_TOO_BIG - New num task batches exceed limit.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_eth_txq_task_batch_lso_send_num_expand(struct doca_eth_txq *eth_txq, uint16_t task_batches_num);

/**
 * @brief This method allocates and initializes a doca_eth_txq_task_send task.
 *
 * @note Supported for DOCA ETH TXQ instance for CPU only.
 *
 * @param [in] eth_txq
 * Pointer to doca_eth_txq instance.
 * @param [in] pkt
 * Buffer that contains the packet to send.
 * @param [in] user_data
 * doca_data to attach to the task
 * @param [out] task_send
 * doca_eth_txq_task_send task that was allocated.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * doca_error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - in case one of the arguments is NULL.
 * - DOCA_ERROR_NO_MEMORY - no more tasks to allocate.
 * - DOCA_ERROR_BAD_STATE - eth_txq context state is not running.
 * - DOCA_ERROR_NOT_CONNECTED - in case eth_txq is not is not connected to a DOCA Progress Engine.
 * - DOCA_ERROR_NOT_SUPPORTED - in case eth_txq is not an instance for CPU.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_eth_txq_task_send_allocate_init(struct doca_eth_txq *eth_txq,
						  struct doca_buf *pkt,
						  union doca_data user_data,
						  struct doca_eth_txq_task_send **task_send);

/**
 * @brief This method allocates and initializes a doca_eth_txq_task_lso_send task.
 *
 * @note Supported for DOCA ETH TXQ instance for CPU only.
 *
 * @param [in] eth_txq
 * Pointer to doca_eth_txq instance.
 * @param [in] pkt_payload
 * Buffer that contains the payload of the packet to send.
 * @param [in] headers
 * A gather list of the headers of the packet to send.
 * @param [in] user_data
 * doca_data to attach to the task
 * @param [out] task_lso_send
 * doca_eth_txq_task_lso_send task that was allocated.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * doca_error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - in case one of the arguments is NULL.
 * - DOCA_ERROR_NO_MEMORY - no more tasks to allocate.
 * - DOCA_ERROR_BAD_STATE - eth_txq context state is not running.
 * - DOCA_ERROR_NOT_CONNECTED - in case eth_txq is not is not connected to a DOCA Progress Engine.
 * - DOCA_ERROR_NOT_SUPPORTED - in case eth_txq is not an instance for CPU.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_eth_txq_task_lso_send_allocate_init(struct doca_eth_txq *eth_txq,
						      struct doca_buf *pkt_payload,
						      struct doca_gather_list *headers,
						      union doca_data user_data,
						      struct doca_eth_txq_task_lso_send **task_lso_send);

/**
 * @brief This method allocates a doca_taskbtach of doca_eth_txq_task_send tasks.
 *
 * @note Supported for DOCA ETH TXQ instance for CPU only.
 *
 * @param [in] eth_txq
 * Pointer to doca_eth_txq instance.
 * @param [in] tasks_num
 * Number of tasks in task_batch.
 * @param [in] task_batch_user_data
 * User data associated with task_batch.
 * @param [out] pkt_array
 * Pointer on array of packet buffers associated with doca_eth_txq_task_send tasks for user to fill.
 * @param [out] task_user_data_array
 * Pointer on array of user data associated with doca_eth_txq_task_send tasks for user to fill.
 * @param [out] task_batch
 * doca_task_batch that was allocated.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * doca_error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - in case one of the arguments is NULL.
 * - DOCA_ERROR_NO_MEMORY - no more task_batches to allocate.
 * - DOCA_ERROR_BAD_STATE - eth_txq context state is not running.
 * - DOCA_ERROR_NOT_CONNECTED - in case eth_txq is not is not connected to a DOCA Progress Engine.
 * - DOCA_ERROR_NOT_SUPPORTED - in case eth_txq is not an instance for CPU.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_eth_txq_task_batch_send_allocate(struct doca_eth_txq *eth_txq,
						   uint16_t tasks_num,
						   union doca_data task_batch_user_data,
						   struct doca_buf ***pkt_array,
						   union doca_data **task_user_data_array,
						   struct doca_task_batch **task_batch);

/**
 * @brief This method allocates a doca_taskbtach of doca_eth_txq_task_lso_send tasks.
 *
 * @note Supported for DOCA ETH TXQ instance for CPU only.
 *
 * @param [in] eth_txq
 * Pointer to doca_eth_txq instance.
 * @param [in] tasks_num
 * Number of tasks in task_batch.
 * @param [in] task_batch_user_data
 * User data associated with task_batch.
 * @param [out] pkt_payload_array
 * Pointer on array of packet payload buffers associated with doca_eth_txq_task_lso_send tasks for user to fill.
 * @param [out] headers_array
 * Pointer on array of headers associated with doca_eth_txq_task_lso_send task for user to fills.
 * @param [out] task_user_data_array
 * Pointer on array of user data associated with doca_eth_txq_task_lso_send tasks for user to fill.
 * @param [out] task_batch
 * doca_task_batch that was allocated.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * doca_error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - in case one of the arguments is NULL.
 * - DOCA_ERROR_NO_MEMORY - no more task_batches to allocate.
 * - DOCA_ERROR_BAD_STATE - eth_txq context state is not running.
 * - DOCA_ERROR_NOT_CONNECTED - in case eth_txq is not is not connected to a DOCA Progress Engine.
 * - DOCA_ERROR_NOT_SUPPORTED - in case eth_txq is not an instance for CPU.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_eth_txq_task_batch_lso_send_allocate(struct doca_eth_txq *eth_txq,
						       uint16_t tasks_num,
						       union doca_data task_batch_user_data,
						       struct doca_buf ***pkt_payload_array,
						       struct doca_gather_list ***headers_array,
						       union doca_data **task_user_data_array,
						       struct doca_task_batch **task_batch);

/**
 * @brief This method sets packet buffer to doca_eth_txq_task_send task.
 *
 * @note Supported for DOCA ETH TXQ instance for CPU only.
 *
 * @param [in] task_send
 * The task to set to.
 * @param [in] pkt
 * Packet buffer to set.
 */
DOCA_EXPERIMENTAL
void doca_eth_txq_task_send_set_pkt(struct doca_eth_txq_task_send *task_send, struct doca_buf *pkt);

/**
 * @brief This method sets packet payload buffer to doca_eth_txq_task_lso_send task.
 *
 * @note Supported for DOCA ETH TXQ instance for CPU only.
 *
 * @param [in] task_lso_send
 * The task to set to.
 * @param [in] pkt_payload
 * Packet payload buffer to set.
 */
DOCA_EXPERIMENTAL
void doca_eth_txq_task_lso_send_set_pkt_payload(struct doca_eth_txq_task_lso_send *task_lso_send,
						struct doca_buf *pkt_payload);

/**
 * @brief This method sets headers to doca_eth_txq_task_lso_send task.
 *
 * @note Supported for DOCA ETH TXQ instance for CPU only.
 *
 * @param [in] task_lso_send
 * The task to set to.
 * @param [in] headers
 * A gather list of the headers of the packet to set.
 */
DOCA_EXPERIMENTAL
void doca_eth_txq_task_lso_send_set_headers(struct doca_eth_txq_task_lso_send *task_lso_send,
					    struct doca_gather_list *headers);

/**
 * @brief This method gets packet buffer from doca_eth_txq_task_send task.
 *
 * @note Supported for DOCA ETH TXQ instance for CPU only.
 *
 * @param [in] task_send
 * The task to get from.
 * @param [out] pkt
 * Packet buffer to get.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * doca_error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - in case one of the arguments is NULL.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_eth_txq_task_send_get_pkt(const struct doca_eth_txq_task_send *task_send, struct doca_buf **pkt);

/**
 * @brief This method gets payload buffer from doca_eth_txq_task_lso_send task.
 *
 * @note Supported for DOCA ETH TXQ instance for CPU only.
 *
 * @param [in] task_lso_send
 * The task to get from.
 * @param [out] pkt_payload
 * Packet payload buffer to get.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * doca_error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - in case one of the arguments is NULL.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_eth_txq_task_lso_send_get_pkt_payload(const struct doca_eth_txq_task_lso_send *task_lso_send,
							struct doca_buf **pkt_payload);

/**
 * @brief This method gets headers from doca_eth_txq_task_lso_send task.
 *
 * @note Supported for DOCA ETH TXQ instance for CPU only.
 *
 * @param [in] task_lso_send
 * The task to get from.
 * @param [out] headers
 * A gather list of the headers of the packet to get.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * doca_error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - in case one of the arguments is NULL.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_eth_txq_task_lso_send_get_headers(const struct doca_eth_txq_task_lso_send *task_lso_send,
						    struct doca_gather_list **headers);

/**
 * @brief This method gets a pointer to internal metadata array from send task. This pointer can be used to read/modify
 * the metadata array content.
 *
 * @note Supported for DOCA ETH TXQ instance for CPU only.
 *
 * @param [in] task_send
 * The task to get from.
 * @param [out] metadata_array
 * metadata array to get. Its length is metadata_num (set by "doca_eth_txq_set_metadata_num()").
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * doca_error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - in case one of the arguments is NULL.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_eth_txq_task_send_get_metadata_array(struct doca_eth_txq_task_send *task_send,
						       uint32_t **metadata_array);

/**
 * @brief This method gets a pointer to internal metadata array from LSO send task. This pointer can be used to
 * read/modify the metadata array content.
 *
 * @note Supported for DOCA ETH TXQ instance for CPU only.
 *
 * @param [in] task_lso_send
 * The task to get from.
 * @param [out] metadata_array
 * metadata array to get. Its length is metadata_num (set by "doca_eth_txq_set_metadata_num()").
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * doca_error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - in case one of the arguments is NULL.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_eth_txq_task_lso_send_get_metadata_array(struct doca_eth_txq_task_lso_send *task_lso_send,
							   uint32_t **metadata_array);

/**
 * @brief This method gets a pointer to internal metadata array from send task batch. This pointer can be used to
 * read/modify the metadata array content.
 *
 * @note Supported for DOCA ETH TXQ instance for CPU only.
 *
 * @param [in] task_batch_send
 * The task batch to get from.
 * @param [out] metadata_array
 * metadata array to get. Its length is metadata_num (set by "doca_eth_txq_set_metadata_num()") * number of packets in
 * task batch. See "doca_eth_txq_task_batch_metadata_array_get_metadata()".
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * doca_error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - in case one of the arguments is NULL.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_eth_txq_task_batch_send_get_metadata_array(struct doca_task_batch *task_batch_send,
							     uint32_t **metadata_array);

/**
 * @brief This method gets a pointer to internal metadata array from LSO send task batch. This pointer can be used to
 * read/modify the metadata array content.
 *
 * @note Supported for DOCA ETH TXQ instance for CPU only.
 *
 * @param [in] task_batch_lso_send
 * The task batch to get from.
 * @param [out] metadata_array
 * metadata array to get. Its length is metadata_num (set by "doca_eth_txq_set_metadata_num()") * number of packets in
 * task batch. See "doca_eth_txq_task_batch_metadata_array_get_metadata()".
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * doca_error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - in case one of the arguments is NULL.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_eth_txq_task_batch_lso_send_get_metadata_array(struct doca_task_batch *task_batch_lso_send,
								 uint32_t **metadata_array);

/**
 * @brief This method sets overrides the default MSS value set by "doca_eth_txq_set_mss()" to a specific LSO send task.
 *
 * @note Supported for DOCA ETH TXQ instance for CPU only.
 *
 * @param [in] task_lso_send
 * The task to get from.
 * @param [out] mss
 * New MSS value to set for task.
 */
DOCA_EXPERIMENTAL
void doca_eth_txq_task_lso_send_set_mss(struct doca_eth_txq_task_lso_send *task_lso_send, uint16_t mss);

/**
 * @brief This method gets a pointer to internal MSS array from LSO send task batch. This pointer can be used to
 * read/modify the MSS array content. This can be used to override the default MSS value set by "doca_eth_txq_set_mss()"
 * to a specific LSO packet in task batch.
 *
 * @note Supported for DOCA ETH TXQ instance for CPU only.
 *
 * @param [in] task_batch_lso_send
 * The task batch to get from.
 * @param [out] mss_array
 * MSS array to get. Its length is number of packets in task batch.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * doca_error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - in case one of the arguments is NULL.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_eth_txq_task_batch_lso_send_get_mss_array(struct doca_task_batch *task_batch_lso_send,
							    uint16_t **mss_array);

/**
 * @brief This method sets overrides the default ol_flags value set by the enabled offloads of the context.
 *
 * @note Supported for DOCA ETH TXQ instance for CPU only.
 *
 * @param [in] task_send
 * The task to set for.
 * @param [out] ol_flags
 * New ol_flags value to set for task.
 */
DOCA_EXPERIMENTAL
void doca_eth_txq_task_send_set_ol_flags(struct doca_eth_txq_task_send *task_send, uint16_t ol_flags);

/**
 * @brief This method sets overrides the default ol_flags value set by the enabled offloads of the context.
 *
 * @note Supported for DOCA ETH TXQ instance for CPU only.
 *
 * @param [in] task_lso_send
 * The task to set for.
 * @param [out] ol_flags
 * New ol_flags value to set for task.
 */
DOCA_EXPERIMENTAL
void doca_eth_txq_task_lso_send_set_ol_flags(struct doca_eth_txq_task_lso_send *task_lso_send, uint16_t ol_flags);

/**
 * @brief This method gets a pointer to internal ol_flags array from send task batch. This pointer can be used to
 * read/modify the ol_flags array content. This can be used to override the default ol_flags value set by the enabled
 * offloads of the context.
 *
 * @note Supported for DOCA ETH TXQ instance for CPU only.
 *
 * @param [in] task_batch_send
 * The task batch to get from.
 * @param [out] ol_flags_array
 * ol_flags array to get. Its length is number of packets in task batch.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * doca_error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - in case one of the arguments is NULL.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_eth_txq_task_batch_send_get_ol_flags_array(struct doca_task_batch *task_batch_send,
							     uint16_t **ol_flags_array);

/**
 * @brief This method gets a pointer to internal ol_flags array from LSO send task batch. This pointer can be used to
 * read/modify the ol_flags array content. This can be used to override the default ol_flags value set by the enabled
 * offloads of the context.
 *
 * @note Supported for DOCA ETH TXQ instance for CPU only.
 *
 * @param [in] task_batch_lso_send
 * The task batch to get from.
 * @param [out] ol_flags_array
 * ol_flags array to get. Its length is number of packets in task batch.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * doca_error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - in case one of the arguments is NULL.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_eth_txq_task_batch_lso_send_get_ol_flags_array(struct doca_task_batch *task_batch_lso_send,
								 uint16_t **ol_flags_array);

/**
 * @brief This method converts a doca_eth_txq_task_send task to doca_task.
 *
 * @note Supported for DOCA ETH TXQ instance for CPU only.
 *
 * @param [in] task_send
 * doca_eth_txq_task_send task.
 *
 * @return doca_task
 */
DOCA_EXPERIMENTAL
struct doca_task *doca_eth_txq_task_send_as_doca_task(struct doca_eth_txq_task_send *task_send);

/**
 * @brief This method converts a doca_eth_txq_task_lso_send task to doca_task.
 *
 * @note Supported for DOCA ETH TXQ instance for CPU only.
 *
 * @param [in] task_lso_send
 * doca_eth_txq_task_lso_send task.
 *
 * @return doca_task
 */
DOCA_EXPERIMENTAL
struct doca_task *doca_eth_txq_task_lso_send_as_doca_task(struct doca_eth_txq_task_lso_send *task_lso_send);

/**
 * @brief This MACRO is used to get/set a specific metadata of a specific packet (in a task batch) from metadata_array.
 *
 * @note Supported for DOCA ETH TXQ instance for CPU only.
 *
 * @param [in] metadata_array
 * Metadata array of a task batch.
 * @param [in] metadata_num
 * Metadata number that was set by "doca_eth_txq_set_metadata_num()".
 * @param [in] packet_index
 * Index of the packet to get metadata associated with.
 * @param [in] metadata_index
 * Index of metadata to get.
 *
 * @return requested metadata
 */
#define doca_eth_txq_task_batch_metadata_array_get_metadata(metadata_array, metadata_num, packet_index, metadata_index) \
	metadata_array[packet_index * metadata_num + metadata_index]

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* DOCA_ETH_TXQ_CPU_DATA_PATH_H_ */

/** @} */
