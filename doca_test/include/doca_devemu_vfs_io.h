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
 * @file doca_devemu_vfs_io.h
 * @page doca_devemu_vfs_io
 * @defgroup DOCA_DEVEMU_VFS_IO DOCA Device Emulation - Virtio FS IO Context
 * @ingroup DOCA_DEVEMU_VFS
 *
 * DOCA Virtio FS IO context
 *
 * @{
 */

#ifndef DOCA_DEVEMU_VFS_IO_H_
#define DOCA_DEVEMU_VFS_IO_H_

#include <stdint.h>

#include <doca_buf.h>
#include <doca_error.h>
#include <doca_dev.h>
#include <doca_devemu_pci.h>
#include <doca_devemu_virtio.h>
#include <doca_devemu_virtio_io.h>
#include <doca_devemu_vfs.h>

#ifdef __cplusplus
extern "C" {
#endif

/*********************************************************************************************************************
 * DOCA devemu Virtio FS IO context API
 *********************************************************************************************************************/

/**
 * @brief Allocate Virtio FS device IO context for a DOCA Virtio FS device.
 *
 * @details The responsibility of the Virtio FS IO context is to relay the requests arriving from the device driver
 * towards the Virtio FS services and applications. Additionally, it is responsible for relaying the completions
 * arriving from the Virtio FS services and applications towards the device driver. Each Virtio FS device IO context
 * is associated with a single DOCA Virtio FS device.
 *
 * @param [in] vfs_dev
 * DOCA Virtio FS device.
 * @param [in] progress_engine
 * The progress engine that will be used to progress the new context.
 * @param [out] io
 * The created DOCA Virtio FS device IO context.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * Error code - in case of failure. see doca_error_t.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_devemu_vfs_io_create(struct doca_devemu_vfs_dev *vfs_dev,
		struct doca_pe *progress_engine, struct doca_devemu_vfs_io **io);

/**
 * @brief Free a Virtio FS device IO context.
 *
 * @param [in] io
 * The DOCA Virtio FS device IO context to release. Must be idle.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * Error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - 'io' is NULL
 * - DOCA_ERROR_BAD_STATE - device IO context is not idle. Use doca_ctx_stop() to stop it
 */
DOCA_EXPERIMENTAL
doca_error_t doca_devemu_vfs_io_destroy(struct doca_devemu_vfs_io *io);

/**
 * @brief Convert DOCA Virtio FS device IO context instance into DOCA context.
 *
 * @param [in] io
 * DOCA Virtio FS device IO context instance. This must remain valid until after the DOCA context is no longer required.
 *
 * @return
 * doca ctx upon success, NULL otherwise.
 */
DOCA_EXPERIMENTAL
struct doca_ctx *doca_devemu_vfs_io_as_ctx(struct doca_devemu_vfs_io *io);

/**
 * @brief Convert DOCA Virtio FS device IO context instance into DOCA Virtio device IO context.
 *
 * @param [in] io
 * DOCA Virtio FS device IO context instance. This must remain valid until after the DOCA Virtio device IO context is
 * no longer required.
 *
 * @return
 * doca devemu virtio device io context upon success, NULL otherwise.
 */
DOCA_EXPERIMENTAL
struct doca_devemu_virtio_io *doca_devemu_vfs_io_as_virtio_io(struct doca_devemu_vfs_io *io);

/*********************************************************************************************************************
 * DOCA devemu Virtio FS IO context events API
 *********************************************************************************************************************/

/**
 * @brief Function to be executed on vfs_req_notice event occurrence. The Ownership of the doca_devemu_vfs_req and the
 * req_user_data moves from doca_devemu_vfs_io ctx to the user.
 *
 * @param [in] req
 * The arrived request.
 * @param [in] req_user_data
 * The user data associated to the request.
 * @param [in] event_user_data
 * Same user data that was provided in doca_devemu_vfs_io_event_vfs_req_notice_register().
 *
 */
typedef void (*doca_devemu_vfs_io_event_vfs_req_notice_handler_cb_t)(struct doca_devemu_vfs_req *req,
		void *req_user_data, union doca_data event_user_data);

/**
 * @brief Register to Virtio FS request notifications.
 *
 * Registration can be done only while IO ctx is idle. If called multiple times then only the last call will take
 * effect.
 *
 * @param [in] io
 * The DOCA Virtio FS device IO context to be associated with the event. Must be idle.
 * @param [in] handler
 * Method that is invoked once event is triggered.
 * @param [in] user_data
 * User data that will be provided to the handler once invoked.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * Error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - 'io' or 'handler' are NULL
 * - DOCA_ERROR_BAD_STATE - IO is not idle
 */
DOCA_EXPERIMENTAL
doca_error_t doca_devemu_vfs_io_event_vfs_req_notice_register(struct doca_devemu_vfs_io *io,
		doca_devemu_vfs_io_event_vfs_req_notice_handler_cb_t handler,
		union doca_data user_data);

/**
 * @brief Function to be executed on vfs_notification_req_notice event occurrence. The Ownership of the
 * doca_devemu_vfs_notification_req and the req_user_data moves from doca_devemu_vfs_io ctx to the user.
 *
 * @param [in] req
 * The arrived request.
 * @param [in] req_user_data
 * The user data associated to the request.
 * @param [in] event_user_data
 * Same user data that was provided in doca_devemu_vfs_io_event_vfs_notification_req_notice_register().
 *
 */
typedef void (*doca_devemu_vfs_io_event_vfs_notification_req_notice_handler_cb_t)(
		struct doca_devemu_vfs_notification_req *req,
		void *req_user_data, union doca_data event_user_data);

/**
 * @brief Register to Virtio FS notification_request notifications.
 *
 * Registration can be done only while IO ctx is idle. If called multiple times then only the last call will take
 * effect.
 *
 * @param [in] io
 * The DOCA Virtio FS device IO context to be associated with the event. Must be idle.
 * @param [in] handler
 * Method that is invoked once event is triggered.
 * @param [in] user_data
 * User data that will be provided to the handler once invoked.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * Error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - 'io' or 'handler' are NULL
 * - DOCA_ERROR_BAD_STATE - IO is not idle
 */
DOCA_EXPERIMENTAL
doca_error_t doca_devemu_vfs_io_event_vfs_notification_req_notice_register(struct doca_devemu_vfs_io *io,
		doca_devemu_vfs_io_event_vfs_notification_req_notice_handler_cb_t handler,
		union doca_data user_data);

/*********************************************************************************************************************
 * DOCA devemu Virtio FS request API
 *********************************************************************************************************************/

/**
 * @brief Complete the Virtio FS request. The Request ownership (including the associated datain, dataout and
 * req_user_data) moves from the user back to the associated IO context. The associated IO context will complete the
 * request towards the device driver according to the virtio fs specification.
 *
 * @param [in] req
 * The Virtio FS request to complete.
 * @param [in] len
 * The number of bytes written into the device writable portion of the buffer described by the req.
 *
 */
DOCA_EXPERIMENTAL
void doca_devemu_vfs_req_complete(struct doca_devemu_vfs_req *req, uint32_t len);

/**
 * @brief Get the doca buffer associated with the device-readable part of the Virtio FS request.
 *
 * This function should be issued during scheduling the request towards the execution context that will be reading
 * from the doca buffer.
 *
 * @param [in] req
 * The Virtio FS request to query.
 *
 * @return
 * The doca buffer representing the host memory for the device-readable part, virtio_fs_req::(in + datain), according
 * to the virtio specification, associated to the request on success. NULL otherwise.
 */
DOCA_EXPERIMENTAL
struct doca_buf *doca_devemu_vfs_req_get_datain(struct doca_devemu_vfs_req *req);

/**
 * @brief Get the doca buffer associated with the device-writable part of the Virtio FS request.
 *
 * This function should be issued during scheduling the request towards the execution context that will be writing
 * to the doca buffer.
 *
 * @param [in] req
 * The Virtio FS request to query.
 *
 * @return
 * The doca buffer representing the host memory for the device-writable part, virtio_fs_req::(out + dataout), according
 * to the virtio specification, associated to the request on success. NULL otherwise.
 */
DOCA_EXPERIMENTAL
struct doca_buf *doca_devemu_vfs_req_get_dataout(struct doca_devemu_vfs_req *req);

/**
 * @brief Get the number of elements in the original doca buffer linked list associated with the device-readable part of the Virtio FS request returned
 * by doca_devemu_vfs_req_get_datain().
 *
 * @param [in] req
 * The Virtio FS request to query. Must not be NULL.
 *
 * @return
 * Number of elements in the original datain doca buffer linked list. Valid only if the request is in the ownership of the user.
 */
DOCA_EXPERIMENTAL
uint32_t doca_devemu_vfs_req_get_datain_list_len(struct doca_devemu_vfs_req *req);

/**
 * @brief Get the total data length of the original doca buffer linked list associated with the device-readable part of the Virtio FS request returned
 * by doca_devemu_vfs_req_get_datain().
 *
 * @param [in] req
 * The Virtio FS request to query. Must not be NULL.
 *
 * @return
 * The total data length (in bytes) of all elements in the original datain DOCA buffer linked list. Valid only if the request is in the ownership of the user.
 */
DOCA_EXPERIMENTAL
uint32_t doca_devemu_vfs_req_get_datain_data_len(struct doca_devemu_vfs_req *req);

/**
 * @brief Get the number of elements in the original doca buffer linked list associated with the device-writable part of the Virtio FS request returned
 * by doca_devemu_vfs_req_get_dataout().
 *
 * @param [in] req
 * The Virtio FS request to query. Must not be NULL.
 *
 * @return
 * Number of elements in the original dataout doca buffer linked list. Valid only if the request is in the ownership of the user.
 */
DOCA_EXPERIMENTAL
uint32_t doca_devemu_vfs_req_get_dataout_list_len(struct doca_devemu_vfs_req *req);

/**
 * @brief Get the total data length of the original doca buffer linked list associated with the device-writable part of the Virtio FS request returned
 * by doca_devemu_vfs_req_get_dataout().
 *
 * @param [in] req
 * The Virtio FS request to query. Must not be NULL.
 *
 * @return
 * The total data length (in bytes) of all elements in the original dataout DOCA buffer linked list. Valid only if the request is in the ownership of the user.
 */
DOCA_EXPERIMENTAL
uint32_t doca_devemu_vfs_req_get_dataout_data_len(struct doca_devemu_vfs_req *req);

/*********************************************************************************************************************
 * DOCA devemu Virtio FS notification_request API
 *********************************************************************************************************************/

/**
 * @brief Complete the Virtio FS notification_request. The Request ownership (including the associated dataout and
 * req_user_data) moves from the user back to the associated IO context. The associated IO context will complete the
 * request towards the device driver according to the virtio fs specification.
 *
 * @param [in] req
 * The Virtio FS notification_request to complete.
 * @param [in] len
 * The number of bytes written into the device writable portion of the buffer described by the req.
 *
 */
DOCA_EXPERIMENTAL
void doca_devemu_vfs_notification_req_complete(struct doca_devemu_vfs_notification_req *req, uint32_t len);

/**
 * @brief Get the doca buffer associated with the device-writable part of the Virtio FS notification request.
 *
 * This function should be issued during scheduling the request towards the execution context that will be writing
 * to the doca buffer.
 *
 * @param [in] req
 * The Virtio FS notification request to query.
 *
 * @return
 * The doca buffer representing the host memory for the device-writable part, virtio_fs_notify::(out_hdr + outarg), according
 * to the virtio specification, associated to the notification request on success. NULL otherwise.
 */
DOCA_EXPERIMENTAL
struct doca_buf *doca_devemu_vfs_notification_req_get_dataout(struct doca_devemu_vfs_notification_req *req);

/**
 * @brief Get the number of elements in the original doca buffer linked list associated with the device-writable part of the Virtio FS notification request
 * returned by doca_devemu_vfs_notification_req_get_dataout().
 *
 * @param [in] req
 * The Virtio FS notification request to query. Must not be NULL.
 *
 * @return
 * Number of elements in the original dataout doca buffer linked list. Valid only if the request is in the ownership of the user.
 */
DOCA_EXPERIMENTAL
uint32_t doca_devemu_vfs_notification_req_get_dataout_list_len(struct doca_devemu_vfs_notification_req *req);

/**
 * @brief Get the total data length of the original doca buffer linked list associated with the device-writable part of the Virtio FS notification request
 * returned by doca_devemu_vfs_notification_req_get_dataout().
 *
 * @param [in] req
 * The Virtio FS notification request to query. Must not be NULL.
 *
 * @return
 * The total data length (in bytes) of all elements in the original dataout DOCA buffer linked list. Valid only if the request is in the ownership of the user.
 */
DOCA_EXPERIMENTAL
uint32_t doca_devemu_vfs_notification_req_get_dataout_data_len(struct doca_devemu_vfs_notification_req *req);

#ifdef __cplusplus
}
#endif

/** @} */

#endif /* DOCA_DEVEMU_VFS_IO_H_ */
