/*
 * Copyright (c) 2022 NVIDIA CORPORATION & AFFILIATES, ALL RIGHTS RESERVED.
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
 * @file doca_devemu_vfs.h
 * @page doca_devemu_vfs
 * @defgroup DOCA_DEVEMU_VFS DOCA Device Emulation - Virtio FS Devices
 * @ingroup DOCA_DEVEMU
 *
 * DOCA library for emulated virtio FS devices
 *
 * @{
 */

#ifndef DOCA_DEVEMU_VFS_H_
#define DOCA_DEVEMU_VFS_H_

#include <stdint.h>

#include <linux/fuse.h>

#include <doca_buf.h>
#include <doca_error.h>
#include <doca_dev.h>
#include <doca_devemu_pci.h>
#include <doca_devemu_virtio.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Opaque structure representing emulated Virtio FS pci device.
 * This structure extends the core doca_devemu_virtio_dev structure.
 * This structure is used by Virtio FS device emulation applications and services.
 */
struct doca_devemu_vfs_dev;

/**
 * @brief Opaque structure representing emulated Virtio FS pci device type.
 * This structure extends the core doca_devemu_virtio_type structure.
 * This structure is used by pci device emulation applications, libraries and services.
 */
struct doca_devemu_vfs_type;

/**
 * @brief Opaque structure representing emulated Virtio FS pci device IO context.
 * This structure extends the core doca_devemu_virtio_io structure.
 * This structure is used by Virtio FS device emulation applications and services.
 */
struct doca_devemu_vfs_io;

/**
 * @brief Opaque structure representing Virtio FS request.
 * This structure is used by Virtio FS device emulation applications and services.
 */
struct doca_devemu_vfs_req;

/**
 * @brief Opaque structure representing Virtio FS notification request.
 * This structure is used by Virtio FS device emulation applications and services.
 */
struct doca_devemu_vfs_notification_req;

/**
 * @brief Size, in bytes, of the virtio FS tag in DOCA. According to the specification this is the name
 * associated with the file system. The tag is encoded in UTF-8 and padded with NULL bytes if shorter than the available
 * space. This field is not NULL terminated according to the Virtio specification. In DOCA, the tag encoding is shorter
 * than in the Virtio specification and must be NULL terminated (only first 20 bytes are allowed to be encoded with
 * non-NULL bytes).
 */
#define DOCA_VFS_TAG_SIZE 21

/*********************************************************************************************************************
 * DOCA devemu Virtio FS device API
 *********************************************************************************************************************/

/**
 * @brief Allocate DOCA Virtio FS device.
 *
 * @param [in] vfs_type
 * The DOCA Virtio FS type to be associated to the device. Must be started.
 * @param [in] dev_rep
 * Representor DOCA device.
 * @param [in] progress_engine
 * The progress engine that will be used to receive events and task completions.
 * @param [out] vfs_dev
 * The newly created DOCA Virtio FS device.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * Error code - in case of failure. see doca_error_t.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_devemu_vfs_dev_create(struct doca_devemu_vfs_type *vfs_type, struct doca_dev_rep *dev_rep,
		struct doca_pe *progress_engine, struct doca_devemu_vfs_dev **vfs_dev);

/**
 * @brief Free a DOCA Virtio FS device object.
 *
 * @param [in] vfs_dev
 * The previously created DOCA Virtio FS device. Must be idle.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * Error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - 'vfs_dev' is NULL
 * - DOCA_ERROR_BAD_STATE - device is not idle. Use doca_ctx_stop() to stop it
 */
DOCA_EXPERIMENTAL
doca_error_t doca_devemu_vfs_dev_destroy(struct doca_devemu_vfs_dev *vfs_dev);

/**
 * @brief Get the value of the Virtio FS device tag. According to the specification the tag is encoded in UTF-8 and
 * padded with NULL bytes if shorter than the available space of 36 bytes and is not NULL-terminated if the encoded
 * bytes take up the entire field of 36 bytes.
 * In DOCA, the tag is always NULL terminated and is shorter than the Virtio specification definition.
 *
 * @param [in] vfs_dev
 * The DOCA Virtio FS device instance to query.
 * @param [out] tag
 * The value of the Virtio FS Device virtio_fs_config:tag according to Virtio specification (with NULL termination).
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * Error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - 'vfs_dev' or 'tag' is NULL
 */
DOCA_EXPERIMENTAL
doca_error_t doca_devemu_vfs_dev_get_tag(const struct doca_devemu_vfs_dev *vfs_dev, char tag[DOCA_VFS_TAG_SIZE]);

/**
 * @brief Set the value of the Virtio FS device tag. According to the specification the tag is encoded in UTF-8 and
 * padded with NULL bytes if shorter than the available space of 36 bytes and is not NULL-terminated if the encoded
 * bytes take up the entire field of 36 bytes.
 * In DOCA, the tag is always NULL terminated and is shorter than the Virtio specification definition.
 *
 * @param [in] vfs_dev
 * The DOCA Virtio FS device instance to modify. Must be idle.
 * @param [in] tag
 * The value of the Virtio FS Device virtio_fs_config:tag according to Virtio specification (with NULL termination).
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * Error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - 'vfs_dev' or 'tag' is NULL
 * - DOCA_ERROR_BAD_STATE - device is not idle
 * - DOCA_ERROR_TOO_BIG - tag is greater than 20 bytes
 */
DOCA_EXPERIMENTAL
doca_error_t doca_devemu_vfs_dev_set_tag(struct doca_devemu_vfs_dev *vfs_dev, const char tag[DOCA_VFS_TAG_SIZE]);

/**
 * @brief Get the value of the VIRTIO FS Device num_request_queues register.
 *
 * @param [in] vfs_dev
 * The DOCA Virtio FS device instance to query.
 * @param [out] num_request_queues
 * The value of Device virtio_fs_config:num_request_queues register according to virtio specification.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * Error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - 'vfs_dev' or 'num_request_queues' is NULL
 */
DOCA_EXPERIMENTAL
doca_error_t doca_devemu_vfs_dev_get_num_request_queues(const struct doca_devemu_vfs_dev *vfs_dev, uint32_t *num_request_queues);

/**
 * @brief Set the value of the VIRTIO FS Device num_request_queues register.
 *
 * @param [in] vfs_dev
 * The DOCA Virtio FS device instance to modify. Must be idle.
 * @param [in] num_request_queues
 * The value of Device virtio_fs_config:num_request_queues register according to virtio specification.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * Error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - 'vfs_dev' is NULL
 * - DOCA_ERROR_BAD_STATE - device is not idle
 */
DOCA_EXPERIMENTAL
doca_error_t doca_devemu_vfs_dev_set_num_request_queues(struct doca_devemu_vfs_dev *vfs_dev, uint32_t num_request_queues);

/**
 * @brief Get the value of the VIRTIO FS Device notify_buf_size register.
 *
 * @details The notify_buf_size value will be used only if VIRTIO_FS_F_NOTIFICATION feature bit is set. Therefore, the
 * notify_buf_size value must comply with VIRTIO_FS_F_NOTIFICATION feature bit before starting the associated vfs_dev.
 *
 * @param [in] vfs_dev
 * The DOCA Virtio FS device instance to query.
 * @param [out] notify_buf_size
 * The value of virtio_fs_config:notify_buf_size register according to virtio specification.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * Error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - 'vfs_dev' or 'notify_buf_size' is NULL
 */
DOCA_EXPERIMENTAL
doca_error_t doca_devemu_vfs_dev_get_notify_buf_size(const struct doca_devemu_vfs_dev *vfs_dev, uint32_t *notify_buf_size);

/**
 * @brief Set the value of the VIRTIO FS Device notify_buf_size register.
 *
 * @details The notify_buf_size value will be used only if VIRTIO_FS_F_NOTIFICATION feature bit is set. Therefore, the
 * notify_buf_size value must comply with VIRTIO_FS_F_NOTIFICATION feature bit before starting the associated vfs_dev.
 *
 * @param [in] vfs_dev
 * The DOCA Virtio FS device instance to modify. Must be idle.
 * @param [in] notify_buf_size
 * The value of virtio_fs_config:notify_buf_size register according to virtio specification. This value must be power
 * of 2 if the VIRTIO_FS_F_NOTIFICATION bit is set. If the VIRTIO_FS_F_NOTIFICATION feature bit is unset, a value of 0
 * can be used. The compliance between notify_buf_size and VIRTIO_FS_F_NOTIFICATION will be verified upon starting the
 * associated vfs_dev.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * Error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - 'vfs_dev' is NULL or notify_buf_size value is invalid.
 * - DOCA_ERROR_BAD_STATE - device is not idle
 */
DOCA_EXPERIMENTAL
doca_error_t doca_devemu_vfs_dev_set_notify_buf_size(struct doca_devemu_vfs_dev *vfs_dev, uint32_t notify_buf_size);

/**
 * @brief Get the size of the user data buffer that will be allocated for each doca_devemu_vfs_req on behalf of the
 * user. This buffer will be valid and used by the user upon receiving new doca_devemu_vfs_req. The buffer will become
 * invalid after doca_devemu_vfs_req completion.
 *
 * @param [in] vfs_dev
 * The DOCA Virtio FS device instance to query.
 * @param [out] req_user_data_size
 * Size, in bytes, of the user data buffer to be allocated on behalf of the user for each doca_devemu_vfs_req.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * Error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - 'vfs_dev' or 'req_user_data_size' is NULL
 */
DOCA_EXPERIMENTAL
doca_error_t doca_devemu_vfs_dev_get_vfs_req_user_data_size(const struct doca_devemu_vfs_dev *vfs_dev,
		uint32_t *req_user_data_size);

/**
 * @brief Set the size of the user data buffer that will be allocated for each doca_devemu_vfs_req on behalf of the
 * user. This buffer will be valid and used by the user upon receiving new doca_devemu_vfs_req. The buffer will become
 * invalid after doca_devemu_vfs_req completion.
 *
 * @param [in] vfs_dev
 * The DOCA Virtio FS device instance to modify. Must be idle.
 * @param [in] req_user_data_size
 * Size, in bytes, of the user data buffer to be allocated on behalf of the user for each doca_devemu_vfs_req.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * Error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - 'vfs_dev' is NULL
 * - DOCA_ERROR_BAD_STATE - device is not idle
 */
DOCA_EXPERIMENTAL
doca_error_t doca_devemu_vfs_dev_set_vfs_req_user_data_size(struct doca_devemu_vfs_dev *vfs_dev,
		uint32_t req_user_data_size);

/**
 * @brief Get the size of the user data buffer that will be allocated for each doca_devemu_vfs_notification_req on
 * behalf of the user. This buffer will be valid and used by the user upon receiving new
 * doca_devemu_vfs_notification_req. The buffer will become invalid after doca_devemu_vfs_notification_req completion.
 *
 * @param [in] vfs_dev
 * The DOCA Virtio FS device instance to query.
 * @param [out] req_user_data_size
 * Size, in bytes, of the user data buffer to be allocated on behalf of the user for each
 * doca_devemu_vfs_notification_req.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * Error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - 'vfs_dev' or 'req_user_data_size' is NULL
 */
DOCA_EXPERIMENTAL
doca_error_t doca_devemu_vfs_dev_get_vfs_notification_req_user_data_size(const struct doca_devemu_vfs_dev *vfs_dev,
		uint32_t *req_user_data_size);

/**
 * @brief Set the size of the user data buffer that will be allocated for each doca_devemu_vfs_notification_req on
 * behalf of the user. This buffer will be valid and used by the user upon receiving new
 * doca_devemu_vfs_notification_req. The buffer will become invalid after doca_devemu_vfs_notification_req completion.
 *
 * @param [in] vfs_dev
 * The DOCA Virtio FS device instance to modify. Must be idle.
 * @param [in] req_user_data_size
 * Size, in bytes, of the user data buffer to be allocated on behalf of the user for
 * each doca_devemu_vfs_notification_req.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * Error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - 'vfs_dev' is NULL
 * - DOCA_ERROR_BAD_STATE - device is not idle
 */
DOCA_EXPERIMENTAL
doca_error_t doca_devemu_vfs_dev_set_vfs_notification_req_user_data_size(struct doca_devemu_vfs_dev *vfs_dev,
		uint32_t req_user_data_size);

/**
 * @brief Convert DOCA Virtio FS device instance into DOCA context.
 *
 * @param [in] vfs_dev
 * DOCA Virtio FS device instance. This must remain valid until after the DOCA context is no longer required.
 *
 * @return
 * doca ctx upon success, NULL otherwise.
 */
DOCA_EXPERIMENTAL
struct doca_ctx *doca_devemu_vfs_dev_as_ctx(struct doca_devemu_vfs_dev *vfs_dev);

/**
 * @brief Convert DOCA Virtio FS device instance into DOCA Virtio device.
 *
 * @param [in] vfs_dev
 * DOCA Virtio FS device instance. This must remain valid until after the DOCA Virtio device is no longer required.
 *
 * @return
 * doca virtio device upon success, NULL otherwise.
 */
DOCA_EXPERIMENTAL
struct doca_devemu_virtio_dev *doca_devemu_vfs_dev_as_virtio_dev(struct doca_devemu_vfs_dev *vfs_dev);

/**
 * @brief Convert DOCA Virtio FS device instance into DOCA devemu PCI device.
 *
 * @param [in] vfs_dev
 * DOCA Virtio FS device instance. This must remain valid until after the DOCA devemu PCI device is no longer required.
 *
 * @return
 * DOCA devemu pci device upon success, NULL otherwise.
 */
DOCA_EXPERIMENTAL
struct doca_devemu_pci_dev *doca_devemu_vfs_dev_as_pci_dev(struct doca_devemu_vfs_dev *vfs_dev);

#ifdef __cplusplus
}
#endif

/** @} */

#endif /* DOCA_DEVEMU_VFS_H_ */
