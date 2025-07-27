/*
 * Copyright (c) 2023 NVIDIA CORPORATION AND AFFILIATES.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification, are permitted
 * provided that the following conditions are met:
 *     * Redistributions of source code must retain the above copyright notice, this list of
 *       conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright notice, this list of
 *       conditions and the following disclaimer in the documentation and/or other materials
 *       provided with the distribution.
 *     * Neither the name of the NVIDIA CORPORATION nor the names of its contributors may be used
 *       to endorse or promote products derived from this software without specific prior written
 *       permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND
 * FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL NVIDIA CORPORATION BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TOR (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */

#ifndef RDMA_COMMON_H_
#define RDMA_COMMON_H_

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <time.h>
#include <unistd.h>

#include <doca_buf.h>
#include <doca_buf_inventory.h>
#include <doca_dev.h>
#include <doca_error.h>
#include <doca_mmap.h>
#include <doca_pe.h>
#include <doca_aes_gcm.h>
#include <doca_rdma.h>
#include <doca_sync_event.h>

#include "common.h"

#define USER_MAX_FILE_NAME 255		       /* Max file name length */
#define MAX_FILE_NAME (USER_MAX_FILE_NAME + 1) /* Max file name string length */

#define AES_GCM_KEY_128_SIZE_IN_BYTES 16		   /* AES-GCM 128 bits key size */
#define AES_GCM_KEY_256_SIZE_IN_BYTES 32		   /* AES-GCM 256 bits key size */
#define MAX_AES_GCM_KEY_SIZE AES_GCM_KEY_256_SIZE_IN_BYTES /* Max AES-GCM key size in bytes */

#define AES_GCM_KEY_128_STR_SIZE (AES_GCM_KEY_128_SIZE_IN_BYTES * 2) /* AES-GCM 128 bits key string size */
#define AES_GCM_KEY_256_STR_SIZE (AES_GCM_KEY_256_SIZE_IN_BYTES * 2) /* AES-GCM 256 bits key string size */
#define MAX_AES_GCM_KEY_STR_SIZE (AES_GCM_KEY_256_STR_SIZE + 1)	     /* Max AES-GCM key string size */

#define AES_GCM_AUTH_TAG_96_SIZE_IN_BYTES 12  /* AES-GCM 96 bits authentication tag size */
#define AES_GCM_AUTH_TAG_128_SIZE_IN_BYTES 16 /* AES-GCM 128 bits authentication tag size */

#define MAX_AES_GCM_IV_LENGTH 12				    /* Max IV length in bytes */
#define MAX_AES_GCM_IV_STR_LENGTH ((MAX_AES_GCM_IV_LENGTH * 2) + 1) /* Max IV string length */

#define SLEEP_IN_NANOS (10 * 1000) /* Sample the task every 10 microseconds */
#define NUM_AES_GCM_TASKS (1)	   /* Number of AES-GCM tasks */

#define MEM_RANGE_LEN (4096)		     /* DOCA mmap memory range length */
#define INVENTORY_NUM_INITIAL_ELEMENTS (16)  /* Number of DOCA inventory initial elements */
#define MAX_USER_ARG_SIZE (256)		     /* Maximum size of user input argument */
#define MAX_ARG_SIZE (MAX_USER_ARG_SIZE + 1) /* Maximum size of input argument */
#define DEFAULT_STRING "Hi DOCA RDMA!"	     /* Default string to use in our samples */
/* Default path to save the local connection descriptor that should be passed to the other side */
#define DEFAULT_LOCAL_CONNECTION_DESC_PATH "/tmp/local_connection_desc_path.txt"
/* Default path to save the remote connection descriptor that should be passed from the other side */
#define DEFAULT_REMOTE_CONNECTION_DESC_PATH "/tmp/remote_connection_desc_path.txt"
/* Default path to read/save the remote mmap connection descriptor that should be passed to the other side */
#define DEFAULT_REMOTE_RESOURCE_CONNECTION_DESC_PATH "/tmp/remote_resource_desc_path.txt"
#define NUM_RDMA_TASKS (1)	   /* Number of RDMA tasks*/
#define SLEEP_IN_NANOS (10 * 1000) /* Sample the task every 10 microseconds  */
// #define MAX(a, b) (((a) > (b)) ? (a) : (b))
/* Server address length, long enough for converting from ascii to hex and including the ':' symbols */
#define SERVER_ADDR_LEN (128)
#define SERVER_ADDR_TYPE_LEN (6)
#define NUM_NEGOTIATION_RDMA_TASKS (1)
#define SERVER_NAME "Server"
#define CLIENT_NAME "Client"
#define DEFAULT_RDMA_CM_PORT (13579)
#define MAX_NUM_CONNECTIONS (8)

/* Function to check if a given device is capable of executing some task */
typedef doca_error_t (*task_check)(const struct doca_devinfo *);

/* Forward declaration */
struct rdma_resources;

/* Function to call in the rdma-cm callback after peer info exchange finished */
typedef doca_error_t (*prepare_and_submit_task_fn)(struct rdma_resources *);

/* AES-GCM modes */
enum aes_gcm_mode {
	AES_GCM_MODE_ENCRYPT, /* Encrypt mode */
	AES_GCM_MODE_DECRYPT, /* Decrypt mode */
};

/* Configuration struct */
struct aes_gcm_rdma_send_cfg {
	char file_path[MAX_FILE_NAME];		      /* File to encrypt/decrypt */
	char output_path[MAX_FILE_NAME];	      /* Output file */
	char pci_address[DOCA_DEVINFO_PCI_ADDR_SIZE]; /* Device PCI address */
	uint8_t raw_key[MAX_AES_GCM_KEY_SIZE];	      /* Raw key */
	enum doca_aes_gcm_key_type raw_key_type;      /* Raw key type */
	uint8_t iv[MAX_AES_GCM_IV_LENGTH];	      /* Initialization vector */
	uint32_t iv_length;			      /* Initialization vector length */
	uint32_t tag_size;			      /* Authentication tag size */
	uint32_t aad_size;			      /* Additional authenticated data size */
	enum aes_gcm_mode mode;			      /* AES-GCM task type */

	char device_name[DOCA_DEVINFO_IBDEV_NAME_SIZE]; /* DOCA device name */
	char send_string[MAX_ARG_SIZE];			/* String to send */
	char read_string[MAX_ARG_SIZE];			/* String to read */
	char write_string[MAX_ARG_SIZE];		/* String to write */
	char local_connection_desc_path[MAX_ARG_SIZE];	/* Path to save the local connection information */
	char remote_connection_desc_path[MAX_ARG_SIZE]; /* Path to read the remote connection information */
	char remote_resource_desc_path[MAX_ARG_SIZE];	/* Path to read/save the remote mmap connection information */
	bool is_gid_index_set;				/* Is the set_index parameter passed */
	uint32_t gid_index;				/* GID index for DOCA RDMA */
	uint32_t num_connections; /* The maximum number of allowed connections, only useful for server for multiple
				    connection samples */
	enum doca_rdma_transport_type transport_type; /* RC or DC, RC is the default, only useful for single connection
							 out-of-band RDMA for now */

	/* The following fields are only related to rdma_cm */
	bool use_rdma_cm;		       /* Whether test rdma-only or rdma-cm,
						* Useful for both client and server
						**/
	int cm_port;			       /* RDMA_CM server listening port number,
						* Useful for both client and server
						**/
	char cm_addr[SERVER_ADDR_LEN + 1];     /* RDMA_cm server IPv4/IPv6/GID address,
						* Only useful for client to do its connection request
						**/
	enum doca_rdma_addr_type cm_addr_type; /* RDMA_CM server address type, IPv4, IPv6 or GID,
						* Only useful for client
						**/
};

/* DOCA AES-GCM resources */
struct aes_gcm_resources {
	struct program_core_objects *state; /* DOCA program core objects */
	struct doca_aes_gcm *aes_gcm;	    /* DOCA AES-GCM context */
	size_t num_remaining_tasks;	    /* Number of remaining AES-GCM tasks */
	enum aes_gcm_mode mode;		    /* AES-GCM mode - encrypt/decrypt */
	bool run_pe_progress;		    /* Controls whether progress loop should run */
};

struct rdma_resources {
	struct aes_gcm_rdma_send_cfg *cfg;		      /* RDMA samples configuration parameters */
	struct doca_dev *doca_device;		      /* DOCA device */
	struct doca_pe *pe;			      /* DOCA progress engine */
	struct doca_mmap *mmap;			      /* DOCA memory map */
	struct doca_mmap *remote_mmap;		      /* DOCA remote memory map */
	struct doca_sync_event *sync_event;	      /* DOCA sync event */
	struct doca_sync_event_remote_net *remote_se; /* DOCA remote sync event */
	char *mmap_memrange;			      /* DOCA remote memory map memory range */
	struct doca_buf_inventory *buf_inventory;     /* DOCA buffer inventory */
	const void *mmap_descriptor;		      /* DOCA memory map descriptor */
	size_t mmap_descriptor_size;		      /* DOCA memory map descriptor size */
	struct doca_rdma *rdma;			      /* DOCA RDMA instance */
	struct doca_ctx *rdma_ctx;		      /* DOCA context to be used with DOCA RDMA */
	struct doca_buf *src_buf;		      /* DOCA source buffer */
	struct doca_buf *dst_buf;		      /* DOCA destination buffer */
	const void *rdma_conn_descriptor;	      /* DOCA RDMA connection descriptor */
	size_t rdma_conn_descriptor_size;	      /* DOCA RDMA connection descriptor size */
	void *remote_rdma_conn_descriptor;	      /* DOCA RDMA remote connection descriptor */
	size_t remote_rdma_conn_descriptor_size;      /* DOCA RDMA remote connection descriptor size */
	void *remote_mmap_descriptor;		      /* DOCA RDMA remote memory map descriptor */
	size_t remote_mmap_descriptor_size;	      /* DOCA RDMA remote memory map descriptor size */
	void *sync_event_descriptor;		      /* DOCA RDMA remote sync event descriptor */
	size_t sync_event_descriptor_size;	      /* DOCA RDMA remote sync event descriptor size */
	doca_error_t first_encountered_error;	      /* Result of the first encountered error, if any */
	bool run_pe_progress;			      /* Flag whether to keep progress the PE */
	size_t num_remaining_tasks;		      /* Number of remaining tasks to submit */

	/* The following cmdline args are only related to rdma_cm */
	struct doca_rdma_addr *cm_addr;				       /* Server address to connect by a client */
	struct doca_rdma_connection *connections[MAX_NUM_CONNECTIONS]; /* The RDMA_CM connection instance */
	bool connection_established[MAX_NUM_CONNECTIONS]; /* Indication whether the corresponding connection have been
							     estableshed */
	uint32_t num_connection_established;		  /* Indicate how many connections has been established */
	struct doca_mmap *mmap_descriptor_mmap;		  /* Used to send local mmap descriptor to remote peer */
	struct doca_mmap *remote_mmap_descriptor_mmap;	  /* Used to receive remote peer mmap descriptor */
	struct doca_mmap *sync_event_descriptor_mmap;	  /* Used to send and receive sync_event descriptor */
	bool recv_sync_event_desc; /* If true, indicate a remote sync event should be received or otherwise a remote
				      mmap */
	const char *self_name;	   /* Client or Server */
	bool is_client;		   /* Client or Server */
	bool is_requester;	   /* Responder or requester */
	prepare_and_submit_task_fn task_fn; /* Function to execute in rdma_cm callback when peer info exchange finished
					     */
	bool require_remote_mmap;	    /* Indicate whether need remote mmap information, for example for
						  rdma_task_read/write */
};

/*
 * Initialize AES-GCM parameters for the sample.
 *
 * @aes_gcm_cfg [in]: AES-GCM configuration struct
 */
void init_aes_gcm_params(struct aes_gcm_rdma_send_cfg *aes_gcm_cfg);

/*
 * Register the command line parameters for the sample.
 *
 * @return: DOCA_SUCCESS on success and DOCA_ERROR otherwise
 */
doca_error_t register_aes_gcm_params(void);

/*
 * Allocate DOCA AES-GCM resources
 *
 * @pci_addr [in]: Device PCI address
 * @max_bufs [in]: Maximum number of buffers for DOCA Inventory
 * @resources [out]: DOCA AES-GCM resources to allocate
 * @return: DOCA_SUCCESS on success and DOCA_ERROR otherwise
 */
doca_error_t allocate_aes_gcm_resources(const char *pci_addr, uint32_t max_bufs, struct aes_gcm_resources *resources);

/*
 * Destroy DOCA AES-GCM resources
 *
 * @resources [in]: DOCA AES-GCM resources to destroy
 * @return: DOCA_SUCCESS on success and DOCA_ERROR otherwise
 */
doca_error_t destroy_aes_gcm_resources(struct aes_gcm_resources *resources);

/*
 * Submit AES-GCM encrypt task and wait for completion
 *
 * @resources [in]: DOCA AES-GCM resources
 * @src_buf [in]: Source buffer
 * @dst_buf [in]: Destination buffer
 * @key [in]: DOCA AES-GCM key
 * @iv [in]: Initialization vector
 * @iv_length [in]: Initialization vector length in bytes
 * @tag_size [in]: Authentication tag size in bytes
 * @aad_size [in]: Additional authenticated data size in bytes
 * @return: DOCA_SUCCESS on success and DOCA_ERROR otherwise
 */
doca_error_t submit_aes_gcm_encrypt_task(struct aes_gcm_resources *resources,
					 struct doca_buf *src_buf,
					 struct doca_buf *dst_buf,
					 struct doca_aes_gcm_key *key,
					 const uint8_t *iv,
					 uint32_t iv_length,
					 uint32_t tag_size,
					 uint32_t aad_size);

/*
 * Submit AES-GCM decrypt task and wait for completion
 *
 * @resources [in]: DOCA AES-GCM resources
 * @src_buf [in]: Source buffer
 * @dst_buf [in]: Destination buffer
 * @key [in]: DOCA AES-GCM key
 * @iv [in]: Initialization vector
 * @iv_length [in]: Initialization vector length in bytes
 * @tag_size [in]: Authentication tag size in bytes
 * @aad_size [in]: Additional authenticated data size in bytes
 * @return: DOCA_SUCCESS on success and DOCA_ERROR otherwise
 */
doca_error_t submit_aes_gcm_decrypt_task(struct aes_gcm_resources *resources,
					 struct doca_buf *src_buf,
					 struct doca_buf *dst_buf,
					 struct doca_aes_gcm_key *key,
					 const uint8_t *iv,
					 uint32_t iv_length,
					 uint32_t tag_size,
					 uint32_t aad_size);

/*
 * Check if given device is capable of executing a DOCA AES-GCM encrypt task.
 *
 * @devinfo [in]: The DOCA device information
 * @return: DOCA_SUCCESS if the device supports DOCA AES-GCM encrypt task and DOCA_ERROR otherwise
 */
doca_error_t aes_gcm_task_encrypt_is_supported(struct doca_devinfo *devinfo);

/*
 * Check if given device is capable of executing a DOCA AES-GCM decrypt task.
 *
 * @devinfo [in]: The DOCA device information
 * @return: DOCA_SUCCESS if the device supports DOCA AES-GCM decrypt task and DOCA_ERROR otherwise
 */
doca_error_t aes_gcm_task_decrypt_is_supported(struct doca_devinfo *devinfo);

/*
 * Encrypt task completed callback
 *
 * @encrypt_task [in]: Completed task
 * @task_user_data [in]: doca_data from the task
 * @ctx_user_data [in]: doca_data from the context
 */
void encrypt_completed_callback(struct doca_aes_gcm_task_encrypt *encrypt_task,
				union doca_data task_user_data,
				union doca_data ctx_user_data);

/*
 * Encrypt task error callback
 *
 * @encrypt_task [in]: failed task
 * @task_user_data [in]: doca_data from the task
 * @ctx_user_data [in]: doca_data from the context
 */
void encrypt_error_callback(struct doca_aes_gcm_task_encrypt *encrypt_task,
			    union doca_data task_user_data,
			    union doca_data ctx_user_data);

/*
 * Decrypt task completed callback
 *
 * @decrypt_task [in]: Completed task
 * @task_user_data [in]: doca_data from the task
 * @ctx_user_data [in]: doca_data from the context
 */
void decrypt_completed_callback(struct doca_aes_gcm_task_decrypt *decrypt_task,
				union doca_data task_user_data,
				union doca_data ctx_user_data);

/*
 * Decrypt task error callback
 *
 * @decrypt_task [in]: failed task
 * @task_user_data [in]: doca_data from the task
 * @ctx_user_data [in]: doca_data from the context
 */
void decrypt_error_callback(struct doca_aes_gcm_task_decrypt *decrypt_task,
			    union doca_data task_user_data,
			    union doca_data ctx_user_data);

/*
 * Allocate DOCA RDMA resources
 *
 * @cfg [in]: Configuration parameters
 * @mmap_permissions [in]: Access flags for DOCA mmap
 * @rdma_permissions [in]: Access permission flags for DOCA RDMA
 * @func [in]: Function to check if a given device is capable of executing some task
 * @resources [in/out]: DOCA RDMA resources to allocate
 * @return: DOCA_SUCCESS on success and DOCA_ERROR otherwise
 */
doca_error_t allocate_rdma_resources(struct aes_gcm_rdma_send_cfg *cfg,
				     const uint32_t mmap_permissions,
				     const uint32_t rdma_permissions,
				     task_check func,
				     struct rdma_resources *resources);

/*
 * Destroy DOCA RDMA resources
 *
 * @resources [in]: DOCA RDMA resources to destroy
 * @cfg [in]: Configuration parameters
 * @return: DOCA_SUCCESS on success and DOCA_ERROR otherwise
 */
doca_error_t destroy_rdma_resources(struct rdma_resources *resources, struct aes_gcm_rdma_send_cfg *cfg);

/*
 * Register the common command line parameters for the sample
 *
 * @return: DOCA_SUCCESS on success and DOCA_ERROR otherwise
 */
doca_error_t register_rdma_common_params(void);

/*
 * Register ARGP send string parameter
 *
 * @return: DOCA_SUCCESS on success and DOCA_ERROR otherwise
 */
doca_error_t register_rdma_send_string_param(void);

/*
 * Register ARGP read string parameter
 *
 * @return: DOCA_SUCCESS on success and DOCA_ERROR otherwise
 */
doca_error_t register_rdma_read_string_param(void);

/*
 * Register ARGP write string parameter
 *
 * @return: DOCA_SUCCESS on success and DOCA_ERROR otherwise
 */
doca_error_t register_rdma_write_string_param(void);

/*
 * Register ARGP max_num_connections parameter
 *
 * @return: DOCA_SUCCESS on success and DOCA_ERROR otherwise
 */
doca_error_t register_rdma_num_connections_param(void);

/*
 * Write the string on a file
 *
 * @file_path [in]: The path of the file
 * @string [in]: The string to write
 * @string_len [in]: The length of the string
 * @return: DOCA_SUCCESS on success and DOCA_ERROR otherwise
 */
doca_error_t write_file(const char *file_path, const char *string, size_t string_len);

/*
 * Read a string from a file
 *
 * @file_path [in]: The path of the file we want to read
 * @string [out]: The string we read
 * @string_len [out]: The length of the string we read
 * @return: DOCA_SUCCESS on success and DOCA_ERROR otherwise
 */
doca_error_t read_file(const char *file_path, char **string, size_t *string_len);

/*
 * Delete file if exists
 *
 * @file_path [in]: The path of the file we want to delete
 * @return: DOCA_SUCCESS on success and DOCA_ERROR otherwise
 */
doca_error_t delete_file(const char *file_path);

/*
 * Using RDMA-CM to start a connection between RDMA server and client
 *
 * @resources [in]: The resource context for the rdma-cm connection
 * @return: DOCA_SUCCESS on success and DOCA_ERROR otherwise
 */
doca_error_t rdma_cm_connect(struct rdma_resources *resources);

/*
 * Cut-off the RDMA-CM connection
 *
 * @resources [in]: The resource context for the rdma-cm connection
 * @return: DOCA_SUCCESS on success and DOCA_ERROR otherwise
 */
doca_error_t rdma_cm_disconnect(struct rdma_resources *resources);

/*
 * Send a message to the peer using the RDMA send task, used in negotiation for peers
 *
 * @rdma [in]: The doca_rdma instance
 * @rdma_connection [in]: The doca_rdma connection
 * @mmap [in]: The doca_mmap instance of the message to be send
 * @buf_inv [in]: The doca_buf_inventory instance for the doca_buf allocation
 * @msg [in]: The message address
 * @msg_len [in]: The message byte length
 * @user_data [in]: The doca_data instance to be embedded into the doca_rdma_task_send
 * @return: DOCA_SUCCESS on success and DOCA_ERROR otherwise
 */
doca_error_t send_msg(struct doca_rdma *rdma,
		      struct doca_rdma_connection *rdma_connection,
		      struct doca_mmap *mmap,
		      struct doca_buf_inventory *buf_inv,
		      void *msg,
		      uint32_t msg_len,
		      void *user_data);

/*
 * Receive a message from the peer using the RDMA receive task, used in negotiation for peers
 *
 * @rdma [in]: The doca_rdma instance
 * @mmap [in]: The doca_mmap instance of the message buffer to be used for storing incoming message
 * @buf_inv [in]: The doca_buf_inventory instance for the doca_buf allocation
 * @msg [in]: The message buffer address
 * @msg_len [in]: The message buffer byte length
 * @user_data [in]: The doca_data instance to be embedded into the doca_rdma_task_receive
 * @return: DOCA_SUCCESS on success and DOCA_ERROR otherwise
 */
doca_error_t recv_msg(struct doca_rdma *rdma,
		      struct doca_mmap *mmap,
		      struct doca_buf_inventory *buf_inv,
		      void *msg,
		      uint32_t msg_len,
		      void *user_data);

/*
 * Callback for the doca_rdma receive task successful completion used in recv_msg()
 *
 * @task [in]: The doca_rdma receive task
 * @task_user_data [in]: The preset user_data for this task
 * @ctx_user_data [in]: The preset ctx_data for this task
 */
void receive_task_completion_cb(struct doca_rdma_task_receive *task,
				union doca_data task_user_data,
				union doca_data ctx_user_data);

/*
 * Callback for the doca_rdma receive task unsuccessful completion used in recv_msg()
 *
 * @task [in]: The doca_rdma receive task
 * @task_user_data [in]: The preset user_data for this task
 * @ctx_user_data [in]: The preset ctx_data for this task
 */
void receive_task_error_cb(struct doca_rdma_task_receive *task,
			   union doca_data task_user_data,
			   union doca_data ctx_user_data);

/*
 * Callback for the doca_rdma send task successful completion used in send_msg()
 *
 * @task [in]: The doca_rdma receive task
 * @task_user_data [in]: The preset user_data for this task
 * @ctx_user_data [in]: The preset ctx_data for this task
 */
void send_task_completion_cb(struct doca_rdma_task_send *task,
			     union doca_data task_user_data,
			     union doca_data ctx_user_data);

/*
 * Callback for the doca_rdma send task unsuccessful completion used in send_msg()
 *
 * @task [in]: The doca_rdma receive task
 * @task_user_data [in]: The preset user_data for this task
 * @ctx_user_data [in]: The preset ctx_data for this task
 */
void send_task_error_cb(struct doca_rdma_task_send *task,
			union doca_data task_user_data,
			union doca_data ctx_user_data);

/*
 * Callback for the rdma_cm server receives the connect request from a client
 *
 * @connection [in]: The rdma_cm connection instance
 * @ctx_user_data [in]: The preset ctx_data for this connection
 */
void rdma_cm_connect_request_cb(struct doca_rdma_connection *connection, union doca_data ctx_user_data);

/*
 * Callback for the rdma_cm server accepts the connect request from a client
 *
 * @connection [in]: The rdma_cm connection instance
 * @connection_user_data [in]: The preset user_data for this connection
 * @ctx_user_data [in]: The preset ctx_data for this connection
 */
void rdma_cm_connect_established_cb(struct doca_rdma_connection *connection,
				    union doca_data connection_user_data,
				    union doca_data ctx_user_data);

/*
 * Callback for the rdma_cm connection setup fails
 *
 * @connection [in]: The rdma_cm connection instance
 * @connection_user_data [in]: The preset user_data for this connection
 * @ctx_user_data [in]: The preset ctx_data for this connection
 */
void rdma_cm_connect_failure_cb(struct doca_rdma_connection *connection,
				union doca_data connection_user_data,
				union doca_data ctx_user_data);

/*
 * Callback for the rdma_cm disconnection
 *
 * @connection [in]: The rdma_cm connection instance
 * @connection_user_data [in]: The preset user_data for this connection
 * @ctx_user_data [in]: The preset ctx_data for this connection
 */
void rdma_cm_disconnect_cb(struct doca_rdma_connection *connection,
			   union doca_data connection_user_data,
			   union doca_data ctx_user_data);

/*
 * Set the default values (that not necessary specified in cmdline input) for test config
 *
 * @cfg [in]: The test configuration instance
 * @return: DOCA_SUCCESS on success and DOCA_ERROR otherwise
 */
doca_error_t set_default_config_value(struct aes_gcm_rdma_send_cfg *cfg);

/*
 * A wrapper for creating local mmap, used for negotiation between peers
 *
 * @mmap [in]: The mmap to be created
 * @mmap_permissions [in]: Access flags for DOCA mmap
 * @data_buffer [in]: The buffer address for this mmap
 * @data_buffer_size [in]: The buffer byte length for this mmap
 * @dev [in]: The doca device bound to this mmap
 * @return: DOCA_SUCCESS on success and DOCA_ERROR otherwise
 */
doca_error_t create_local_mmap(struct doca_mmap **mmap,
			       const uint32_t mmap_permissions,
			       void *data_buffer,
			       size_t data_buffer_size,
			       struct doca_dev *dev);

/*
 * Config callbacks needed for rdma cm connection setup, and config tasks used for negotiation between peers
 *
 * @resources [in]: The rdma test context
 * @need_send_task [in]: Indicate whether need to config rdma_task_send
 * @need_recv_task [in]: Indicate whether need to config rdma_task_receive
 * @return: DOCA_SUCCESS on success and DOCA_ERROR otherwise
 */
doca_error_t config_rdma_cm_callback_and_negotiation_task(struct rdma_resources *resources,
							  bool need_send_task,
							  bool need_recv_task);

/*
 * This function is a part of the negotiation functions between peers, used to receive remote peer's data.
 *
 * @resources [in]: The rdma test context
 * @return: DOCA_SUCCESS on success and DOCA_ERROR otherwise
 */
doca_error_t rdma_requester_recv_data_from_rdma_responder(struct rdma_resources *resources);

/*
 * This function is a part of the negotiation functions between peers, used to send data to remote peer.
 *
 * @resources [in]: The rdma test context
 * @return: DOCA_SUCCESS on success and DOCA_ERROR otherwise
 */
doca_error_t rdma_responder_send_data_to_rdma_requester(struct rdma_resources *resources);

/*
 * This function is used for waiting for pressing anykey on the keyboard, purely for waiting/co-ordinating purpose.
 */
void wait_for_enter(void);

#endif /* RDMA_COMMON_H_ */

