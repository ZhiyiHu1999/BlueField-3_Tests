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

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#include <doca_mmap.h>
#include <doca_buf.h>
#include <doca_buf_inventory.h>
#include <doca_ctx.h>
#include <doca_pe.h>
#include <doca_dma.h>
#include <doca_graph.h>
#include <doca_types.h>
#include <doca_log.h>

#include <samples/common.h>

#include <doca_error.h>
#include "rdma_common.h"

#define MAX_BUFF_SIZE (256) /* Maximum DOCA buffer size */

DOCA_LOG_REGISTER(GRAPH::SAMPLE);

/*
 * Write the connection details for the receiver to read, and read the connection details of the receiver
 * In DC transport mode it is only needed to read the remote connection details
 *
 * @cfg [in]: Configuration parameters
 * @resources [in/out]: RDMA resources
 * @return: DOCA_SUCCESS on success and DOCA_ERROR otherwise
 */
static doca_error_t write_read_connection(struct rdma_config *cfg, struct rdma_resources *resources)
{
	doca_error_t result = DOCA_SUCCESS;

	if (cfg->transport_type == DOCA_RDMA_TRANSPORT_TYPE_RC) {
		/* Write the RDMA connection details */
		result = write_file(cfg->local_connection_desc_path,
				    (char *)resources->rdma_conn_descriptor,
				    resources->rdma_conn_descriptor_size);
		if (result != DOCA_SUCCESS) {
			DOCA_LOG_ERR("Failed to write the RDMA connection details: %s", doca_error_get_descr(result));
			return result;
		}

		DOCA_LOG_INFO("You can now copy %s to the receiver", cfg->local_connection_desc_path);
	}

	DOCA_LOG_INFO("Please copy %s from the receiver and then press enter after pressing enter in the receiver side",
		      cfg->remote_connection_desc_path);

	/* Wait for enter */
	wait_for_enter();

	/* Read the remote RDMA connection details */
	result = read_file(cfg->remote_connection_desc_path,
			   (char **)&resources->remote_rdma_conn_descriptor,
			   &resources->remote_rdma_conn_descriptor_size);
	if (result != DOCA_SUCCESS)
		DOCA_LOG_ERR("Failed to read the remote RDMA connection details: %s", doca_error_get_descr(result));

	return result;
}

/*
 * RDMA send task completed callback
 *
 * @rdma_send_task [in]: Completed task
 * @task_user_data [in]: doca_data from the task
 * @ctx_user_data [in]: doca_data from the context
 */
static void rdma_send_completed_callback(struct doca_rdma_task_send *rdma_send_task,
					 union doca_data task_user_data,
					 union doca_data ctx_user_data)
{
	struct rdma_resources *resources = (struct rdma_resources *)ctx_user_data.ptr;
	doca_error_t *first_encountered_error = (doca_error_t *)task_user_data.ptr;
	doca_error_t result = DOCA_SUCCESS, tmp_result;

	DOCA_LOG_INFO("RDMA send task was done successfully");

	doca_task_free(doca_rdma_task_send_as_task(rdma_send_task));
	tmp_result = doca_buf_dec_refcount(resources->src_buf, NULL);
	if (tmp_result != DOCA_SUCCESS) {
		DOCA_LOG_ERR("Failed to decrease src_buf count: %s", doca_error_get_descr(tmp_result));
		DOCA_ERROR_PROPAGATE(result, tmp_result);
	}

	/* Update that an error was encountered, if any */
	DOCA_ERROR_PROPAGATE(*first_encountered_error, tmp_result);

	resources->num_remaining_tasks--;
	/* Stop context once all tasks are completed */
	if (resources->num_remaining_tasks == 0) {
		if (resources->cfg->use_rdma_cm == true)
			(void)rdma_cm_disconnect(resources);
		(void)doca_ctx_stop(resources->rdma_ctx);
	}
}

/*
 * RDMA send task error callback
 *
 * @rdma_send_task [in]: failed task
 * @task_user_data [in]: doca_data from the task
 * @ctx_user_data [in]: doca_data from the context
 */
static void rdma_send_error_callback(struct doca_rdma_task_send *rdma_send_task,
				     union doca_data task_user_data,
				     union doca_data ctx_user_data)
{
	struct rdma_resources *resources = (struct rdma_resources *)ctx_user_data.ptr;
	struct doca_task *task = doca_rdma_task_send_as_task(rdma_send_task);
	doca_error_t *first_encountered_error = (doca_error_t *)task_user_data.ptr;
	doca_error_t result;

	/* Update that an error was encountered */
	result = doca_task_get_status(task);
	DOCA_ERROR_PROPAGATE(*first_encountered_error, result);
	DOCA_LOG_ERR("RDMA send task failed: %s", doca_error_get_descr(result));

	doca_task_free(task);
	result = doca_buf_dec_refcount(resources->src_buf, NULL);
	if (result != DOCA_SUCCESS)
		DOCA_LOG_ERR("Failed to decrease src_buf count: %s", doca_error_get_descr(result));

	resources->num_remaining_tasks--;
	/* Stop context once all tasks are completed */
	if (resources->num_remaining_tasks == 0) {
		if (resources->cfg->use_rdma_cm == true)
			(void)rdma_cm_disconnect(resources);
		(void)doca_ctx_stop(resources->rdma_ctx);
	}
}

/*
 * Export and receive connection details, and connect to the remote RDMA
 *
 * @resources [in]: RDMA resources
 * @return: DOCA_SUCCESS on success and DOCA_ERROR otherwise
 */
static doca_error_t rdma_send_export_and_connect(struct rdma_resources *resources)
{
	doca_error_t result;

	if (resources->cfg->use_rdma_cm == true)
		return rdma_cm_connect(resources);

	/* Export RDMA connection details */
	result = doca_rdma_export(resources->rdma,
				  &(resources->rdma_conn_descriptor),
				  &(resources->rdma_conn_descriptor_size),
				  &(resources->connections[0]));
	if (result != DOCA_SUCCESS) {
		DOCA_LOG_ERR("Failed to export RDMA: %s", doca_error_get_descr(result));
		return result;
	}

	/* Write and read connection details to the receiver */
	result = write_read_connection(resources->cfg, resources);
	if (result != DOCA_SUCCESS) {
		DOCA_LOG_ERR("Failed to write and read connection details from receiver: %s",
			     doca_error_get_descr(result));
		return result;
	}

	/* Connect RDMA */
	result = doca_rdma_connect(resources->rdma,
				   resources->remote_rdma_conn_descriptor,
				   resources->remote_rdma_conn_descriptor_size,
				   resources->connections[0]);
	if (result != DOCA_SUCCESS)
		DOCA_LOG_ERR("Failed to connect the sender's RDMA to the receiver's RDMA: %s",
			     doca_error_get_descr(result));

	return result;
}

/*
 * Prepare and submit RDMA send task
 *
 * @resources [in]: RDMA resources
 * @return: DOCA_SUCCESS on success and DOCA_ERROR otherwise
 */
static doca_error_t rdma_send_prepare_and_submit_task(struct rdma_resources *resources)
{
	struct doca_rdma_task_send *rdma_send_task = NULL;
	union doca_data task_user_data = {0};
	void *src_buf_data;
	doca_error_t result, tmp_result;

	if (resources->cfg->use_rdma_cm == true) {
		DOCA_LOG_INFO(
			"Please press enter after the receive task has been successfully submitted in the receiver side");

		/* Wait for enter */
		wait_for_enter();
	}

	/* Add src buffer to DOCA buffer inventory */
	result = doca_buf_inventory_buf_get_by_data(resources->buf_inventory,
						    resources->mmap,
						    resources->mmap_memrange,
						    MAX_BUFF_SIZE,
						    &resources->src_buf);
	if (result != DOCA_SUCCESS) {
		DOCA_LOG_ERR("Failed to allocate DOCA buffer to DOCA buffer inventory: %s",
			     doca_error_get_descr(result));
		return result;
	}

	/* Set data of src buffer */
	result = doca_buf_get_data(resources->src_buf, &src_buf_data);
	if (result != DOCA_SUCCESS) {
		DOCA_LOG_ERR("Failed to get source buffer data: %s", doca_error_get_descr(result));
		goto destroy_src_buf;
	}
	strncpy(src_buf_data, resources->cfg->send_string, MAX_BUFF_SIZE + 1);

	/* Include first_encountered_error in user data of task to be used in the callbacks */
	task_user_data.ptr = &(resources->first_encountered_error);
	/* Allocate and construct RDMA send task */
	result = doca_rdma_task_send_allocate_init(resources->rdma,
						   resources->connections[0],
						   resources->src_buf,
						   task_user_data,
						   &rdma_send_task);
	if (result != DOCA_SUCCESS) {
		DOCA_LOG_ERR("Failed to allocate RDMA send task: %s", doca_error_get_descr(result));
		goto destroy_src_buf;
	}

	/* Submit RDMA send task */
	DOCA_LOG_INFO("Submitting RDMA send task that sends \"%s\" to receiver", resources->cfg->send_string);
	resources->num_remaining_tasks++;
	result = doca_task_submit(doca_rdma_task_send_as_task(rdma_send_task));
	if (result != DOCA_SUCCESS) {
		DOCA_LOG_ERR("Failed to submit RDMA send task: %s", doca_error_get_descr(result));
		goto free_task;
	}

	return result;

free_task:
	doca_task_free(doca_rdma_task_send_as_task(rdma_send_task));
destroy_src_buf:
	tmp_result = doca_buf_dec_refcount(resources->src_buf, NULL);
	if (tmp_result != DOCA_SUCCESS) {
		DOCA_LOG_ERR("Failed to decrease src_buf count: %s", doca_error_get_descr(tmp_result));
		DOCA_ERROR_PROPAGATE(result, tmp_result);
	}
	return result;
}

/*
 * RDMA send state change callback
 * This function represents the state machine for this RDMA program
 *
 * @user_data [in]: doca_data from the context
 * @ctx [in]: DOCA context
 * @prev_state [in]: Previous DOCA context state
 * @next_state [in]: Next DOCA context state
 */
static void rdma_send_state_change_callback(const union doca_data user_data,
					    struct doca_ctx *ctx,
					    enum doca_ctx_states prev_state,
					    enum doca_ctx_states next_state)
{
	struct rdma_resources *resources = (struct rdma_resources *)user_data.ptr;
	struct rdma_config *cfg = resources->cfg;
	doca_error_t result = DOCA_SUCCESS;
	(void)prev_state;
	(void)ctx;

	switch (next_state) {
	case DOCA_CTX_STATE_STARTING:
		DOCA_LOG_INFO("RDMA context entered starting state");
		break;
	case DOCA_CTX_STATE_RUNNING:
		DOCA_LOG_INFO("RDMA context is running");

		result = rdma_send_export_and_connect(resources);
		if (result != DOCA_SUCCESS) {
			DOCA_LOG_ERR("rdma_send_export_and_connect() failed: %s", doca_error_get_descr(result));
			break;
		} else
			DOCA_LOG_INFO("RDMA context finished initialization");

		if (cfg->use_rdma_cm == true)
			break;

		result = rdma_send_prepare_and_submit_task(resources);
		if (result != DOCA_SUCCESS)
			DOCA_LOG_ERR("rdma_send_prepare_and_submit_task() failed: %s", doca_error_get_descr(result));
		break;
	case DOCA_CTX_STATE_STOPPING:
		/**
		 * doca_ctx_stop() has been called.
		 * In this sample, this happens either due to a failure encountered, in which case doca_pe_progress()
		 * will cause any inflight task to be flushed, or due to the successful compilation of the sample flow.
		 * In both cases, in this sample, doca_pe_progress() will eventually transition the context to idle
		 * state.
		 */
		DOCA_LOG_INFO("RDMA context entered into stopping state. Any inflight tasks will be flushed");
		break;
	case DOCA_CTX_STATE_IDLE:
		DOCA_LOG_INFO("RDMA context has been stopped");

		/* We can stop progressing the PE */
		resources->run_pe_progress = false;
		break;
	default:
		break;
	}

	/* If something failed - update that an error was encountered and stop the ctx */
	if (result != DOCA_SUCCESS) {
		DOCA_ERROR_PROPAGATE(resources->first_encountered_error, result);
		(void)doca_ctx_stop(ctx);
	}
}

// DOCA_LOG_REGISTER(GRAPH::SAMPLE);

/**
 * This sample creates the following graph:
 *
 *         +-----+             +-----+
 *         | DMA |             | DMA |
 *         +--+--+             +-----+
 *            |                   |
 *            +---------+---------+
 *                      |
 *                +-----------+
 *                | User Node |
 *                +-----------+
 *
 * The DMA nodes copy one source to two destinations.
 * The user node compares the destinations to the source.
 * The sample uses only one type of context to simplify the code, but a graph can use any context.
 *
 * The sample runs 10 graph instances and uses polling to simplify the code.
 */

/**
 * This macro is used to minimize code size.
 * The macro runs an expression and returns error if the expression status is not DOCA_SUCCESS
 */
#define EXIT_ON_FAILURE(_expression_) \
	{ \
		doca_error_t _status_ = _expression_; \
\
		if (_status_ != DOCA_SUCCESS) { \
			DOCA_LOG_ERR("%s failed with status %s", __func__, doca_error_get_descr(_status_)); \
			return _status_; \
		} \
	}

#define NUM_DMA_NODES 2

#define NUM_GRAPH_INSTANCES 1

#define DMA_BUFFER_SIZE 1024

#define REQUIRED_ENTRY_SIZE (DMA_BUFFER_SIZE + (DMA_BUFFER_SIZE * NUM_DMA_NODES))

#define BUFFER_SIZE (REQUIRED_ENTRY_SIZE * NUM_GRAPH_INSTANCES)

/* One buffer for source + one buffer for each DMA node (destination) */
#define GRAPH_INSTANCE_NUM_BUFFERS (1 + NUM_DMA_NODES)
#define BUF_INVENTORY_SIZE (GRAPH_INSTANCE_NUM_BUFFERS * NUM_GRAPH_INSTANCES)

/**
 * It is recommended to put the graph instance data in a struct.
 * Notice that graph tasks life span must be >= life span of the graph instance.
 */
struct graph_instance_data {
	uint32_t index; /* Index is used for printing */
	struct doca_graph_instance *graph_instance;
	struct doca_buf *source;
	uint8_t *source_addr;

	struct doca_dma_task_memcpy *dma_task[NUM_DMA_NODES];
	struct doca_buf *dma_dest[NUM_DMA_NODES];
	uint8_t *dma_dest_addr[NUM_DMA_NODES];
};

/**
 * This struct defines the program context.
 */
struct graph_sample_state {
	/**
	 * Resources
	 */
	struct doca_dev *device;
	struct doca_mmap *mmap;			      /* DOCA memory map */
	struct doca_buf_inventory *inventory;
	struct doca_pe *pe;			      /* DOCA progress engine */
	struct rdma_config *cfg;		      /* RDMA samples configuration parameters */
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

	struct doca_ctx *contexts[NUM_DMA_NODES];
	struct doca_dma *dma[NUM_DMA_NODES];

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

	/**
	 * Buffer
	 * This buffer is used for the source and destination.
	 * Real life scenario may use more memory areas.
	 */
	uint8_t *buffer;
	uint8_t *available_buffer; /* Points to the available location in the buffer, used during initialization */

	/**
	 * Graph
	 * This section holds the graph and nodes.
	 * The nodes are used during instance creation and maintenance.
	 */
	struct doca_graph *graph;
	struct doca_graph_node *dma_node[NUM_DMA_NODES];
	struct doca_graph_node *user_node;

	/* Array of graph instances. All will be submitted to the work queue at once */
	struct graph_instance_data instances[NUM_GRAPH_INSTANCES];

	uint32_t num_completed_instances;
};

/**
 * Allocates a buffer that will be used for the source and destination buffers.
 *
 * @state [in]: sample state
 * @return: DOCA_SUCCESS on success and DOCA_ERROR otherwise
 */
static doca_error_t allocate_resources(struct graph_sample_state *state)
{
	DOCA_LOG_INFO("Allocating buffer");

	state->buffer = (uint8_t *)malloc(BUFFER_SIZE);
	if (state->buffer == NULL)
		return DOCA_ERROR_NO_MEMORY;

	state->available_buffer = state->buffer;

	return DOCA_SUCCESS;
}

/*
 * Check if DOCA device is DMA capable
 *
 * @devinfo [in]: Device to check
 * @return: DOCA_SUCCESS on success and DOCA_ERROR otherwise
 */
static doca_error_t check_dev_dma_capable(struct doca_devinfo *devinfo)
{
	return doca_dma_cap_task_memcpy_is_supported(devinfo);
}

/**
 * Opens a device that supports DMA
 *
 * @state [in]: sample state
 * @return: DOCA_SUCCESS on success and DOCA_ERROR otherwise
 */
static doca_error_t open_device(struct graph_sample_state *state)
{
	DOCA_LOG_INFO("Opening device");

	EXIT_ON_FAILURE(open_doca_device_with_capabilities(check_dev_dma_capable, &state->device));

	return DOCA_SUCCESS;
}

/**
 * Creates a progress engine
 *
 * @state [in]: sample state
 * @return: DOCA_SUCCESS on success and DOCA_ERROR otherwise
 */
static doca_error_t create_pe(struct graph_sample_state *state)
{
	DOCA_LOG_INFO("Creating progress engine");

	EXIT_ON_FAILURE(doca_pe_create(&state->pe));

	return DOCA_SUCCESS;
}

/**
 * Create MMAP, initialize and start it.
 *
 * @state [in]: sample state
 * @return: DOCA_SUCCESS on success and DOCA_ERROR otherwise
 */
static doca_error_t create_mmap(struct graph_sample_state *state)
{
	DOCA_LOG_INFO("Creating MMAP");

	EXIT_ON_FAILURE(doca_mmap_create(&state->mmap));
	EXIT_ON_FAILURE(doca_mmap_set_memrange(state->mmap, state->buffer, BUFFER_SIZE));
	EXIT_ON_FAILURE(doca_mmap_add_dev(state->mmap, state->device));
	EXIT_ON_FAILURE(doca_mmap_set_permissions(state->mmap, DOCA_ACCESS_FLAG_LOCAL_READ_WRITE));
	EXIT_ON_FAILURE(doca_mmap_start(state->mmap));

	return DOCA_SUCCESS;
}

/**
 * Create buffer inventory, initialize and start it.
 *
 * @state [in]: sample state
 * @return: DOCA_SUCCESS on success and DOCA_ERROR otherwise
 */
static doca_error_t create_buf_inventory(struct graph_sample_state *state)
{
	DOCA_LOG_INFO("Creating buf inventory");

	EXIT_ON_FAILURE(doca_buf_inventory_create(BUF_INVENTORY_SIZE, &state->inventory));
	EXIT_ON_FAILURE(doca_buf_inventory_start(state->inventory));

	return DOCA_SUCCESS;
}

/**
 * DMA task completed callback
 *
 * @details: This method is used as a mandatory input for the doca_dma_task_memcpy_set_conf but will never be called
 * because task completion callbacks are not invoked when the task is submitted to a graph.
 *
 * @task [in]: DMA task
 * @task_user_data [in]: Task user data
 * @ctx_user_data [in]: context user data
 */
static void dma_task_completed_callback(struct doca_dma_task_memcpy *task,
					union doca_data task_user_data,
					union doca_data ctx_user_data)
{
	(void)task;
	(void)task_user_data;
	(void)ctx_user_data;
}

/**
 * Create DMA
 *
 * @state [in]: sample state
 * @idx [in]: context index
 * @return: DOCA_SUCCESS on success and DOCA_ERROR otherwise
 */
static doca_error_t create_dma(struct graph_sample_state *state, uint32_t idx)
{
	DOCA_LOG_INFO("Creating DMA %d", idx);

	EXIT_ON_FAILURE(doca_dma_create(state->device, &state->dma[idx]));
	state->contexts[idx] = doca_dma_as_ctx(state->dma[idx]);

	EXIT_ON_FAILURE(doca_pe_connect_ctx(state->pe, state->contexts[idx]));

	EXIT_ON_FAILURE(doca_dma_task_memcpy_set_conf(state->dma[idx],
						      dma_task_completed_callback,
						      dma_task_completed_callback,
						      NUM_GRAPH_INSTANCES));

	return DOCA_SUCCESS;
}

/**
 * Create DMAs
 *
 * @state [in]: sample state
 * @return: DOCA_SUCCESS on success and DOCA_ERROR otherwise
 */
static doca_error_t create_dmas(struct graph_sample_state *state)
{
	uint32_t i = 0;

	for (i = 0; i < NUM_DMA_NODES; i++)
		EXIT_ON_FAILURE(create_dma(state, i));

	return DOCA_SUCCESS;
}

/**
 * Start contexts
 * The method adds the device to the contexts, starts them and add them to the work queue.
 *
 * @state [in]: sample state
 * @return: DOCA_SUCCESS on success and DOCA_ERROR otherwise
 */
static doca_error_t start_contexts(struct graph_sample_state *state)
{
	uint32_t i = 0;

	DOCA_LOG_INFO("Starting contexts");

	for (i = 0; i < NUM_DMA_NODES; i++)
		EXIT_ON_FAILURE(doca_ctx_start(state->contexts[i]));

	return DOCA_SUCCESS;
}

/**
 * Stop contexts
 * The method removes the contexts from the work queue, stops them and removes the device from them.
 *
 * @state [in]: sample state
 */
static void stop_contexts(struct graph_sample_state *state)
{
	uint32_t i = 0;

	/* Assumption: this method is called when contexts can be stopped synchronously */
	for (i = 0; i < NUM_DMA_NODES; i++)
		if (state->contexts[i] != NULL)
			(void)doca_ctx_stop(state->contexts[i]);
}

/**
 * User node callback
 * This callback is called when the graph user node is executed.
 * The callback compares the source buffer to the destination buffers.
 *
 * @cookie [in]: callback cookie
 * @return: DOCA_SUCCESS on success and DOCA_ERROR otherwise
 */
static doca_error_t user_node_callback(void *cookie)
{
	uint32_t i = 0;

	struct graph_instance_data *instance = (struct graph_instance_data *)cookie;
	size_t dma_length = 0;

	DOCA_LOG_INFO("Instance %d user callback", instance->index);

	for (i = 0; i < NUM_DMA_NODES; i++) {
		EXIT_ON_FAILURE(doca_buf_get_data_len(instance->dma_dest[i], &dma_length));

		if (dma_length != DMA_BUFFER_SIZE) {
			DOCA_LOG_ERR("DMA destination buffer length %zu should be %d", dma_length, DMA_BUFFER_SIZE);
			return DOCA_ERROR_BAD_STATE;
		}

		if (memcmp(instance->dma_dest_addr[i], instance->source_addr, dma_length) != 0) {
			DOCA_LOG_ERR("DMA source and destination mismatch");
			return DOCA_ERROR_BAD_STATE;
		}
	}

	return DOCA_SUCCESS;
}

/**
 * Destroy graph instance
 *
 * @state [in]: sample state
 * @index [in]: the graph instance index
 */
static void destroy_graph_instance(struct graph_sample_state *state, uint32_t index)
{
	struct graph_instance_data *instance = &state->instances[index];
	uint32_t i = 0;

	if (instance->graph_instance != NULL) {
		(void)doca_graph_instance_destroy(instance->graph_instance);
		instance->graph_instance = NULL;
	}
	for (i = 0; i < NUM_DMA_NODES; i++) {
		if (instance->dma_task[i] != NULL) {
			doca_task_free(doca_dma_task_memcpy_as_task(instance->dma_task[i]));
			instance->dma_task[i] = NULL;
		}

		if (instance->dma_dest[i] != NULL) {
			(void)doca_buf_dec_refcount(instance->dma_dest[i], NULL);
			instance->dma_dest[i] = NULL;
		}
	}

	if (instance->source != NULL) {
		(void)doca_buf_dec_refcount(instance->source, NULL);
		instance->source = NULL;
	}
}

/**
 * This method processes a graph instance completion. The sample does not care if the instance was successful or failed
 * and will act the same way on both cases.
 *
 * @graph_instance [in]: completed graph instance
 * @instance_user_data [in]: graph instance user data
 * @graph_user_data [in]: graph user data
 */
static void graph_completion_callback(struct doca_graph_instance *graph_instance,
				      union doca_data instance_user_data,
				      union doca_data graph_user_data)
{
	struct graph_sample_state *state = (struct graph_sample_state *)graph_user_data.ptr;
	(void)graph_instance;
	(void)instance_user_data;

	state->num_completed_instances++;

	/* Graph instance and tasks are destroyed at cleanup */
}

/**
 * This method creates the graph.
 * Creating a node adds it to the graph roots.
 * Adding dependency removes a dependent node from the graph roots.
 * The method creates all nodes and then adds the dependency out of convenience. Adding dependency during node creation
 * is supported.
 *
 * @state [in]: sample state
 * @return: DOCA_SUCCESS on success and DOCA_ERROR otherwise
 */
static doca_error_t create_graph(struct graph_sample_state *state)
{
	union doca_data graph_user_data = {};

	DOCA_LOG_INFO("Creating graph");

	EXIT_ON_FAILURE(doca_graph_create(state->pe, &state->graph));

	EXIT_ON_FAILURE(doca_graph_node_create_from_user(state->graph, user_node_callback, &state->user_node));

	/* Creating nodes and building the graph */
	EXIT_ON_FAILURE(doca_graph_node_create_from_ctx(state->graph, state->contexts[0], &state->dma_node[0]));
	EXIT_ON_FAILURE(doca_graph_node_create_from_ctx(state->graph, state->contexts[1], &state->dma_node[1]));

	EXIT_ON_FAILURE(doca_graph_add_dependency(state->graph, state->dma_node[0], state->dma_node[1]));
	EXIT_ON_FAILURE(doca_graph_add_dependency(state->graph, state->dma_node[1], state->user_node));

	/* Notice that the sample uses the same callback for success & failure. Program can supply different cb */
	EXIT_ON_FAILURE(doca_graph_set_conf(state->graph,
					    graph_completion_callback,
					    graph_completion_callback,
					    NUM_GRAPH_INSTANCES));

	graph_user_data.ptr = state;
	EXIT_ON_FAILURE(doca_graph_set_user_data(state->graph, graph_user_data));

	/* Graph must be started before it is added to the work queue. The graph is validated during this call */
	EXIT_ON_FAILURE(doca_graph_start(state->graph));

	return DOCA_SUCCESS;
}

/**
 * Destroy the graph
 *
 * @state [in]: sample state
 */
static void destroy_graph(struct graph_sample_state *state)
{
	if (state->graph == NULL)
		return;

	doca_graph_stop(state->graph);
	doca_graph_destroy(state->graph);
}

/**
 * This method creates a graph instance
 * Graph instance creation usually includes initializing the data for the nodes (e.g. initializing tasks).
 *
 * @state [in]: sample state
 * @index [in]: the graph instance index
 * @return: DOCA_SUCCESS on success and DOCA_ERROR otherwise
 */
static doca_error_t create_graph_instance(struct graph_sample_state *state, uint32_t index)
{
	struct graph_instance_data *instance = &state->instances[index];
	union doca_data task_user_data = {};
	union doca_data graph_instance_user_data = {};
	uint32_t i = 0;

	instance->index = index;

	EXIT_ON_FAILURE(doca_graph_instance_create(state->graph, &instance->graph_instance));

	/* Use doca_buf_inventory_buf_get_by_data to initialize the source buffer */
	EXIT_ON_FAILURE(doca_buf_inventory_buf_get_by_data(state->inventory,
							   state->mmap,
							   state->available_buffer,
							   DMA_BUFFER_SIZE,
							   &instance->source));
	memset(state->available_buffer, (index + 1), DMA_BUFFER_SIZE);
	instance->source_addr = state->available_buffer;
	state->available_buffer += DMA_BUFFER_SIZE;

	/* Initialize DMA tasks */
	for (i = 0; i < NUM_DMA_NODES; i++) {
		EXIT_ON_FAILURE(doca_buf_inventory_buf_get_by_addr(state->inventory,
								   state->mmap,
								   state->available_buffer,
								   DMA_BUFFER_SIZE,
								   &instance->dma_dest[i]));
		instance->dma_dest_addr[i] = state->available_buffer;
		state->available_buffer += DMA_BUFFER_SIZE;

		EXIT_ON_FAILURE(doca_dma_task_memcpy_alloc_init(state->dma[i],
								instance->source,
								instance->dma_dest[i],
								task_user_data,
								&instance->dma_task[i]));
		EXIT_ON_FAILURE(
			doca_graph_instance_set_ctx_node_data(instance->graph_instance,
							      state->dma_node[i],
							      doca_dma_task_memcpy_as_task(instance->dma_task[i])));
	}

	/* Initialize user callback */
	/* The sample uses the instance as a cookie. From there it can get all the information it needs */
	EXIT_ON_FAILURE(doca_graph_instance_set_user_node_data(instance->graph_instance, state->user_node, instance));

	graph_instance_user_data.ptr = instance;
	doca_graph_instance_set_user_data(instance->graph_instance, graph_instance_user_data);

	return DOCA_SUCCESS;
}

/**
 * Create graph instances
 *
 * @state [in]: sample state
 * @return: DOCA_SUCCESS on success and DOCA_ERROR otherwise
 */
static doca_error_t create_graph_instances(struct graph_sample_state *state)
{
	uint32_t i = 0;

	DOCA_LOG_INFO("Creating graph instances");

	for (i = 0; i < NUM_GRAPH_INSTANCES; i++)
		EXIT_ON_FAILURE(create_graph_instance(state, i));

	return DOCA_SUCCESS;
}

/**
 * Destroy graph instances
 *
 * @state [in]: sample state
 */
static void destroy_graph_instances(struct graph_sample_state *state)
{
	uint32_t i = 0;

	for (i = 0; i < NUM_GRAPH_INSTANCES; i++)
		destroy_graph_instance(state, i);
}

/**
 * Submit graph instances
 *
 * @state [in]: sample state
 * @return: DOCA_SUCCESS on success and DOCA_ERROR otherwise
 */
static doca_error_t submit_instances(struct graph_sample_state *state)
{
	uint32_t i = 0;

	DOCA_LOG_INFO("Submitting all graph instances");

	for (i = 0; i < NUM_GRAPH_INSTANCES; i++)
		EXIT_ON_FAILURE(doca_graph_instance_submit(state->instances[i].graph_instance));

	return DOCA_SUCCESS;
}

/**
 * Poll the work queue until all instances are completed
 *
 * @state [in]: sample state
 */
static void poll_for_completion(struct graph_sample_state *state)
{
	state->num_completed_instances = 0;

	DOCA_LOG_INFO("Waiting until all instances are complete");

	while (state->num_completed_instances < NUM_GRAPH_INSTANCES)
		(void)doca_pe_progress(state->pe);

	DOCA_LOG_INFO("All instances completed");
}

/**
 * This method cleans up the sample resources in reverse order of their creation.
 * This method does not check for destroy return values for simplify.
 * Real code should check the return value and act accordingly.
 *
 * @state [in]: sample state
 */
static void cleanup(struct graph_sample_state *state)
{
	uint32_t i = 0;

	destroy_graph_instances(state);

	destroy_graph(state);

	stop_contexts(state);

	for (i = 0; i < NUM_DMA_NODES; i++)
		if (state->dma[i] != NULL)
			(void)doca_dma_destroy(state->dma[i]);

	if (state->pe != NULL)
		(void)doca_pe_destroy(state->pe);

	if (state->inventory != NULL) {
		(void)doca_buf_inventory_stop(state->inventory);
		(void)doca_buf_inventory_destroy(state->inventory);
	}

	if (state->mmap != NULL) {
		(void)doca_mmap_stop(state->mmap);
		(void)doca_mmap_destroy(state->mmap);
	}

	if (state->device != NULL)
		(void)doca_dev_close(state->device);

	if (state->buffer != NULL)
		free(state->buffer);
}

/**
 * Run the sample
 * The method (and the method it calls) does not cleanup anything in case of failures.
 * It assumes that cleanup is called after it at any case.
 *
 * @state [in]: sample state
 * @return: DOCA_SUCCESS on success and DOCA_ERROR otherwise
 */
static doca_error_t run(struct graph_sample_state *state, struct rdma_config *cfg)
{
	union doca_data ctx_user_data = {0};
	const uint32_t mmap_permissions = DOCA_ACCESS_FLAG_LOCAL_READ_WRITE;
	const uint32_t rdma_permissions = DOCA_ACCESS_FLAG_LOCAL_READ_WRITE;
	struct timespec ts = {
		.tv_sec = 0,
		.tv_nsec = SLEEP_IN_NANOS,
	};
	doca_error_t result, tmp_result;

	EXIT_ON_FAILURE(allocate_rdma_resources(cfg, mmap_permissions, rdma_permissions, doca_rdma_cap_task_send_is_supported, state));
	EXIT_ON_FAILURE(allocate_resources(state));

	EXIT_ON_FAILURE(doca_rdma_task_send_set_conf(state->rdma,
					      rdma_send_completed_callback,
					      rdma_send_error_callback,
					      NUM_RDMA_TASKS));
	EXIT_ON_FAILURE(doca_ctx_set_state_changed_cb(state->rdma_ctx, rdma_send_state_change_callback));

	/* Include the program's resources in user data of context to be used in callbacks */
	ctx_user_data.ptr = state;
	EXIT_ON_FAILURE(doca_ctx_set_user_data(state->rdma_ctx, ctx_user_data));

	/* Create DOCA buffer inventory */
	EXIT_ON_FAILURE(doca_buf_inventory_create(INVENTORY_NUM_INITIAL_ELEMENTS, &(state->buf_inventory)));
	EXIT_ON_FAILURE(doca_buf_inventory_start(state->buf_inventory));

	if (cfg->use_rdma_cm == true) {
		/* Set rdma cm connection configuration callbacks */
		resources.require_remote_mmap = false;
		resources.task_fn = rdma_send_prepare_and_submit_task;
		EXIT_ON_FAILURE(config_rdma_cm_callback_and_negotiation_task(&resources,
								      /* need_send_mmap_info */ false,
								      /* need_recv_mmap_info */ false));
	}

	EXIT_ON_FAILURE(doca_ctx_start(resources.rdma_ctx));

	EXIT_ON_FAILURE(open_device(state));
	EXIT_ON_FAILURE(create_mmap(state));
	EXIT_ON_FAILURE(create_buf_inventory(state));
	EXIT_ON_FAILURE(create_pe(state));
	EXIT_ON_FAILURE(create_dmas(state));
	EXIT_ON_FAILURE(start_contexts(state));
	EXIT_ON_FAILURE(create_graph(state));
	EXIT_ON_FAILURE(create_graph_instances(state));
	EXIT_ON_FAILURE(submit_instances(state));
	poll_for_completion(state);

	return DOCA_SUCCESS;
}

/**
 * Run the graph sample
 *
 * @return: DOCA_SUCCESS on success and DOCA_ERROR otherwise
 */
doca_error_t run_graph_sample(struct rdma_config *cfg)
{
	struct graph_sample_state state = {0};
	doca_error_t status = run(&state, cfg);

	cleanup(&state);

	return status;
}
