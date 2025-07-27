// Filename: encrypt_rdma_graph.c
#include <doca_graph.h>
#include <doca_encryption.h>
#include <doca_rdma.h>
#include <doca_pe.h>
#include <doca_buf_inventory.h>
#include <doca_mmap.h>
#include <doca_error.h>
#include <doca_log.h>
#include <doca_buf.h>
#include <doca_ctx.h>

#include "sample.h"
#include "util.h"

#include "common_common.h"
#include "pe_common.h"
#include "aes_gcm_common.h"
#include "rdma_common.h"

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#define DATA_LEN 4096

#define CHECK_D(DOCA_CALL)                      \
    do {                                                 \
        doca_error_t __err = (DOCA_CALL);                \
        if (__err != DOCA_SUCCESS) {                     \
            fprintf(stderr, "%s failed: %s\n", #DOCA_CALL, doca_get_error_string(__err)); \
            exit(1);                                     \
        }                                                \
    } while (0)

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


int main() {  // no need resources of aes_gcm, only config and graph state
    //////////////////////////////
    struct rdma_config cfg;
	doca_error_t result;

	/* Set the default configuration values (Example values) */
	result = set_default_config_value(&cfg);
	/* Register a logger backend */
	result = doca_log_backend_create_standard();
	/* Register RDMA common params */
	result = register_rdma_common_params();
	/* Register RDMA send_string param */
	result = register_rdma_send_string_param();

	struct rdma_resources resources = {0};
	union doca_data ctx_user_data = {0};
	const uint32_t mmap_permissions = DOCA_ACCESS_FLAG_LOCAL_READ_WRITE;
	const uint32_t rdma_permissions = DOCA_ACCESS_FLAG_LOCAL_READ_WRITE;
	doca_error_t result, tmp_result;

	/* Allocating resources */
	result = allocate_rdma_resources(&cfg,
					 mmap_permissions,
					 rdma_permissions,
					 doca_rdma_cap_task_send_is_supported,
					 &resources);
	result = doca_rdma_task_send_set_conf(resources.rdma,
					      rdma_send_completed_callback,
					      rdma_send_error_callback,
					      NUM_RDMA_TASKS);
	result = doca_ctx_set_state_changed_cb(resources.rdma_ctx, rdma_send_state_change_callback);
	/* Include the program's resources in user data of context to be used in callbacks */
	ctx_user_data.ptr = &(resources);
	result = doca_ctx_set_user_data(resources.rdma_ctx, ctx_user_data);
	/* Create DOCA buffer inventory */
	result = doca_buf_inventory_create(INVENTORY_NUM_INITIAL_ELEMENTS, &resources.buf_inventory);
	/* Start DOCA buffer inventory */
	result = doca_buf_inventory_start(resources.buf_inventory);
	if (cfg->use_rdma_cm == true) {
		/* Set rdma cm connection configuration callbacks */
		resources.require_remote_mmap = false;
		resources.task_fn = rdma_send_prepare_and_submit_task;
		result = config_rdma_cm_callback_and_negotiation_task(&resources,
								      /* need_send_mmap_info */ false,
								      /* need_recv_mmap_info */ false);
	}
	/* Start RDMA context */
	result = doca_ctx_start(resources.rdma_ctx);

	// /*
	//  * Run the progress engine which will run the state machine defined in rdma_send_state_change_callback()
	//  * When the context moves to idle, the context change callback call will signal to stop running the progress
	//  * engine.
	//  */
	// while (resources.run_pe_progress) {
	// 	if (doca_pe_progress(resources.pe) == 0)
	// 		nanosleep(&ts, &ts);
	// }

	// /* Assign the result we update in the callbacks */
	// result = resources.first_encountered_error;


    ///////////////////
    struct graph_sample_state state = {0};
	struct rdma_config cfg_rdma;
	union doca_data ctx_user_data = {0};

    EXIT_ON_FAILURE(doca_pe_create(&state->pe));
    EXIT_ON_FAILURE(doca_aes_gcm_create(state->dev, &state->aes_gcm));
    state->context_aes_gcm = doca_aes_gcm_as_ctx(state->aes_gcm);
    EXIT_ON_FAILURE(doca_pe_connect_ctx(state->pe, state->context_aes_gcm));

	const uint32_t mmap_permissions = DOCA_ACCESS_FLAG_LOCAL_READ_WRITE;
	const uint32_t rdma_permissions = DOCA_ACCESS_FLAG_LOCAL_READ_WRITE;
	EXIT_ON_FAILURE(allocate_rdma_resources(&cfg_rdma,
							mmap_permissions,
							rdma_permissions,
							doca_rdma_cap_task_send_is_supported,
							&state.resources_rdma));
	EXIT_ON_FAILURE(doca_rdma_task_send_set_conf(state.resources_rdma.rdma,
							rdma_send_completed_callback,
							rdma_send_error_callback,
							NUM_RDMA_TASKS));
	EXIT_ON_FAILURE(doca_ctx_set_state_changed_cb(state.resources_rdma.rdma_ctx, rdma_send_state_change_callback));
	ctx_user_data.ptr = &(state.resources_rdma);
	EXIT_ON_FAILURE(doca_ctx_set_user_data(state.resources_rdma.rdma_ctx, ctx_user_data));
	EXIT_ON_FAILURE(doca_buf_inventory_create(INVENTORY_NUM_INITIAL_ELEMENTS, &state.resources_rdma.buf_inventory));
	EXIT_ON_FAILURE(doca_buf_inventory_start(state.resources_rdma.buf_inventory));
	if (cfg->use_rdma_cm == true) {
		/* Set rdma cm connection configuration callbacks */
		state.resources_rdma.require_remote_mmap = false;
		state.resources_rdma.task_fn = rdma_send_prepare_and_submit_task;
		EXIT_ON_FAILURE(config_rdma_cm_callback_and_negotiation_task(&state.resources_rdma,
								      /* need_send_mmap_info */ false,
								      /* need_recv_mmap_info */ false));
	}
	EXIT_ON_FAILURE(doca_ctx_start(state.resources_rdma.rdma_ctx));


    // EXIT_ON_FAILURE(doca_pe_connect_ctx(state->pe, state->context_rdma));

    EXIT_ON_FAILURE(doca_aes_gcm_task_encrypt_set_conf(state->aes_gcm,
                                encrypt_completed_callback,
                                encrypt_error_callback,
                                1));

    EXIT_ON_FAILURE(doca_rdma_task_send_set_conf(state->rdma,
                                send_task_completion_cb,
                                send_task_error_cb,
                                1));

    EXIT_ON_FAILURE(doca_ctx_start(state->context_aes_gcm));
    EXIT_ON_FAILURE(doca_ctx_start(state->context_rdma));

    /* Create graph nodes and dependency */
    EXIT_ON_FAILURE(doca_graph_create(state->pe, &state->graph));
	EXIT_ON_FAILURE(doca_graph_node_create_from_user(state->graph, user_node_callback, &state->node_user));
    EXIT_ON_FAILURE(doca_graph_node_create_from_ctx(state->graph, state->context_rdma, &state->node_rdma));
    EXIT_ON_FAILURE(doca_graph_add_dependency(state->graph, state->node_rdma, state->node_user));
    EXIT_ON_FAILURE(doca_graph_node_create_from_ctx(state->graph, state->context_aes_gcm, &state->node_aes_gcm));
    EXIT_ON_FAILURE(doca_graph_add_dependency(state->graph, state->node_aes_gcm, state->node_rdma));

	/* Notice that the sample uses the same callback for success & failure. Program can supply different cb */
	doca_graph_set_conf(state->graph,
                    graph_completion_callback,
                    graph_completion_callback,
                    1);

    union doca_data graph_user_data = {};
    struct graph_instance_data *instance = &state->instance;
	union doca_data task_user_data = {};
	union doca_data graph_instance_user_data = {};

	graph_user_data.ptr = state;
	EXIT_ON_FAILURE(doca_graph_set_user_data(state->graph, graph_user_data));
	/* Graph must be started before it is added to the work queue. The graph is validated during this call */
	EXIT_ON_FAILURE(doca_graph_start(state->graph));
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

	/* Initialize AES GCM task */
	struct aes_gcm_cfg aes_gcm_cfg;
	char *file_data = NULL;
	size_t file_size;
    struct aes_gcm_resources resources_aes_gcm = {0};
    // struct program_core_objects *state_aes_gcm = NULL;
    struct doca_buf *aes_gcm_state_src_doca_buf = NULL;
    struct doca_buf *aes_gcm_state_dst_doca_buf = NULL;
    /* The sample will use 2 doca buffers */
    uint32_t aes_gcm_max_bufs = 2;
    char *aes_gcm_dst_buffer = NULL;
    uint8_t *resp_head = NULL;
    size_t data_len = 0;
    char *dump = NULL;
    FILE *out_file = NULL;
    struct doca_aes_gcm_key *aes_gcm_key = NULL;
    uint64_t max_encrypt_buf_size = 0;
    char *pci_addr;

    init_aes_gcm_params(&aes_gcm_cfg);
    aes_gcm_cfg.file_path = './graph_input.txt'
    read_file(aes_gcm_cfg.file_path, &file_data, &file_size);
    out_file = fopen(aes_gcm_cfg->output_path, "wr");

    // allocate_aes_gcm_resources(cfg->pci_address, max_bufs, &resources);
    pci_addr = aes_gcm_cfg->pci_address;
    // struct program_core_objects *state_aes_gcm = NULL;
	union doca_data aes_gcm_ctx_user_data = {0};
	resources_aes_gcm->state = malloc(sizeof(*resources_aes_gcm->state));
	resources_aes_gcm->num_remaining_tasks = 0;
	// state_aes_gcm = resources_aes_gcm->state;

	/* Open DOCA device */
	if (pci_addr != NULL) {
		/* If pci_addr was provided then open using it */
		if (resources->mode == AES_GCM_MODE_ENCRYPT)
			result = open_doca_device_with_pci(pci_addr, &aes_gcm_task_encrypt_is_supported, &state->dev);
		else
			result = open_doca_device_with_pci(pci_addr, &aes_gcm_task_decrypt_is_supported, &state->dev);
	} else {
		/* If pci_addr was not provided then look for DOCA device */
		if (resources->mode == AES_GCM_MODE_ENCRYPT)
			result = open_doca_device_with_capabilities(&aes_gcm_task_encrypt_is_supported, &state->dev);
		else
			result = open_doca_device_with_capabilities(&aes_gcm_task_decrypt_is_supported, &state->dev);
	}
	EXIT_ON_FAILURE(doca_aes_gcm_create(state->dev, &state->aes_gcm));
	// state->context_aes_gcm = doca_aes_gcm_as_ctx(resources_aes_gcm->aes_gcm);
	// EXIT_ON_FAILURE(create_core_objects(state, aes_gcm_max_bufs));
    /////
    EXIT_ON_FAILURE(doca_mmap_create(&state->src_mmap_aes_gcm));
	EXIT_ON_FAILURE(doca_mmap_add_dev(state->src_mmap_aes_gcm, state->dev));
	EXIT_ON_FAILURE(doca_mmap_create(&state->dst_mmap_aes_gcm));
	EXIT_ON_FAILURE(doca_mmap_add_dev(state->dst_mmap_aes_gcm, state->dev));
	EXIT_ON_FAILURE(doca_buf_inventory_create(aes_gcm_max_bufs, &state->inventory));
	EXIT_ON_FAILURE(doca_buf_inventory_start(state->inventory));
	// doca_pe_create(&state->pe);
    /////

	// EXIT_ON_FAILURE(doca_pe_connect_ctx(state->pe, state->context_aes_gcm));
	EXIT_ON_FAILURE(doca_ctx_set_state_changed_cb(state->context_aes_gcm, aes_gcm_state_changed_callback));
	// if (resources_aes_gcm->mode == AES_GCM_MODE_ENCRYPT)
    EXIT_ON_FAILURE(doca_aes_gcm_task_encrypt_set_conf(state->aes_gcm,
                                encrypt_completed_callback,
                                encrypt_error_callback,
                                NUM_AES_GCM_TASKS));

	/* Include resources in user data of context to be used in callbacks */
	aes_gcm_ctx_user_data.ptr = resources_aes_gcm;
	EXIT_ON_FAILURE(doca_ctx_set_user_data(state->context_aes_gcm, aes_gcm_ctx_user_data));
    ////////

    EXIT_ON_FAILURE(doca_aes_gcm_cap_task_encrypt_get_max_buf_size(doca_dev_as_devinfo(state->dev), &max_encrypt_buf_size));
    // doca_ctx_start(state->ctx);
    aes_gcm_dst_buffer = calloc(1, max_encrypt_buf_size);
    EXIT_ON_FAILURE(doca_mmap_set_memrange(state->dst_mmap, aes_gcm_dst_buffer, max_encrypt_buf_size));
    EXIT_ON_FAILURE(doca_mmap_start(state->dst_mmap));
    EXIT_ON_FAILURE(doca_mmap_set_memrange(state->src_mmap, file_data, file_size));
    EXIT_ON_FAILURE(doca_mmap_start(state->src_mmap));
    EXIT_ON_FAILURE(doca_buf_inventory_buf_get_by_addr(state->buf_inv, state->src_mmap, file_data, file_size, &instance->aes_gcm_src_buf));
    EXIT_ON_FAILURE(doca_buf_inventory_buf_get_by_addr(state->buf_inv, state->dst_mmap, aes_gcm_dst_buffer, max_encrypt_buf_size, &instance->aes_gcm_dst_buf));
    EXIT_ON_FAILURE(doca_buf_set_data(instance->aes_gcm_src_buf, file_data, file_size));
    EXIT_ON_FAILURE(doca_aes_gcm_key_create(resources_aes_gcm.aes_gcm, aes_gcm_cfg->raw_key, aes_gcm_cfg->raw_key_type, &aes_gcm_key));
    // result = submit_aes_gcm_encrypt_task(&resources,
    //                      src_doca_buf,
    //                      dst_doca_buf,
    //                      key,
    //                      (uint8_t *)cfg->iv,
    //                      cfg->iv_length,
    //                      cfg->tag_size,
    //                      cfg->aad_size);
    state->aes_gcm = resources_aes_gcm->aes_gcm;
    EXIT_ON_FAILURE(doca_aes_gcm_task_encrypt_alloc_init(state->aes_gcm,
                                    instance->aes_gcm_src_buf,
                                    instance->aes_gcm_dst_buf,
                                    aes_gcm_key,
                                    (uint8_t *)aes_gcm_cfg->iv,
                                    aes_gcm_cfg->iv_length,
                                    aes_gcm_cfg->tag_size,
                                    aes_gcm_cfg->aad_size));
    EXIT_ON_FAILURE(doca_graph_instance_set_ctx_node_data(instance->graph_instance,
                                    state->aes_gcm_node,
                                    doca_dma_task_memcpy_as_task(instance->aes_gcm_task)));
    
    /* Initialize RDMA task */
    
    ///////////////////
	for (i = 0; i < NUM_DMA_NODES; i++) {
		doca_buf_inventory_buf_get_by_addr(state->inventory,
								   state->mmap,
								   state->available_buffer,
								   DMA_BUFFER_SIZE,
								   &instance->dma_dest[i]);
		instance->dma_dest_addr[i] = state->available_buffer;
		state->available_buffer += DMA_BUFFER_SIZE;

		doca_dma_task_memcpy_alloc_init(state->dma[i],
								instance->source,
								instance->dma_dest[i],
								task_user_data,
								&instance->dma_task[i]);
		
		doca_graph_instance_set_ctx_node_data(instance->graph_instance,
                                state->dma_node[i],
                                doca_dma_task_memcpy_as_task(instance->dma_task[i]));
    }
    //////////////////

    doca_graph_instance_set_user_node_data(instance->graph_instance, state->user_node, instance);
	graph_instance_user_data.ptr = instance;
	doca_graph_instance_set_user_data(instance->graph_instance, graph_instance_user_data);


    doca_graph_instance_submit(state->instances[i].graph_instance);
    (void)doca_pe_progress(state->pe);

    return DOCA_SUCCESS;
}