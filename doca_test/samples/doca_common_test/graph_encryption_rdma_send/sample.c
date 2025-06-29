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

#include "rdma_common.h"

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#define DATA_LEN 4096

#define CHECK_D(DOCA_CALL)                               \
    do {                                                 \
        doca_error_t __err = (DOCA_CALL);                \
        if (__err != DOCA_SUCCESS) {                     \
            fprintf(stderr, "%s failed: %s\n", #DOCA_CALL, doca_get_error_string(__err)); \
            exit(1);                                     \
        }                                                \
    } while (0)

int main() {
    // struct doca_pe *pe;
    // struct doca_graph *graph;

    // struct doca_mmap *mmap;
    // struct doca_buf_inventory *buf_inv;
    // struct doca_device *dev = NULL;
    // struct doca_ctx *enc_ctx, *rdma_ctx;

    // struct doca_buf *pt_buf, *ct_buf;

    // // Step 1: Create PE
    // CHECK_D(doca_pe_create(&pe));

    // // Step 2: Device discovery & selection
    // CHECK_D(doca_dev_list_create(NULL)); // Omitted device selection logic
    // CHECK_D(doca_dev_open_by_name("mlx5_0", &dev)); // Modify device name
    // CHECK_D(doca_dev_probe(dev));

    // // Step 3: Create mmap and buf inventory
    // CHECK_D(doca_mmap_create(dev, &mmap));
    // CHECK_D(doca_buf_inventory_create(dev, &buf_inv, 64));

    // // Step 4: Register memory
    // void *pt_data = aligned_alloc(4096, DATA_LEN);
    // void *ct_data = aligned_alloc(4096, DATA_LEN + 16); // +16 for tag
    // memcpy(pt_data, "This is sensitive data.", 25);
    // CHECK_D(doca_mmap_add_memrange(mmap, pt_data, DATA_LEN));
    // CHECK_D(doca_mmap_add_memrange(mmap, ct_data, DATA_LEN + 16));
    // CHECK_D(doca_mmap_start(mmap));

    // CHECK_D(doca_buf_inventory_buf_by_addr(buf_inv, mmap, pt_data, DATA_LEN, &pt_buf));
    // CHECK_D(doca_buf_inventory_buf_by_addr(buf_inv, mmap, ct_data, DATA_LEN + 16, &ct_buf));

    // // Step 5: Create Graph
    // CHECK_D(doca_graph_create(&graph));

    // // Step 6: Create encryption context and task
    // CHECK_D(doca_ctx_create(dev, DOCA_ENCRYPTION, &enc_ctx));
    // CHECK_D(doca_ctx_dev_add_resources(enc_ctx, dev));
    // CHECK_D(doca_ctx_start(enc_ctx));
    // CHECK_D(doca_pe_add_ctx(pe, enc_ctx));

    // struct doca_encryption_task *enc_task;
    // CHECK_D(doca_encryption_task_alloc(enc_ctx, &enc_task));
    // doca_encryption_task_set_input(enc_task, pt_buf);
    // doca_encryption_task_set_output(enc_task, ct_buf);
    // doca_encryption_task_set_key(enc_task, (uint8_t *)"1234567890abcdef", 16);
    // doca_encryption_task_set_iv(enc_task, (uint8_t *)"abcdef123456", 12);
    // doca_encryption_task_set_tag_location(enc_task, ct_buf, DATA_LEN); // append tag at end

    // // Step 7: Create RDMA context and task
    // CHECK_D(doca_ctx_create(dev, DOCA_RDMA, &rdma_ctx));
    // CHECK_D(doca_ctx_dev_add_resources(rdma_ctx, dev));
    // CHECK_D(doca_ctx_start(rdma_ctx));
    // CHECK_D(doca_pe_add_ctx(pe, rdma_ctx));

    // struct doca_rdma_task *rdma_task;
    // CHECK_D(doca_rdma_task_alloc(rdma_ctx, &rdma_task));

    // // ⚠️ 注意：你需要提前知道远端的 RDMA 地址 remote_addr
    // uint64_t remote_addr = 0x12345678;
    // uint32_t rkey = 0xabcd;
    // CHECK_D(doca_rdma_task_set_remote_addr(rdma_task, remote_addr, rkey));
    // CHECK_D(doca_rdma_task_set_buf(rdma_task, ct_buf));

    // // Step 8: 构建 Graph
    // CHECK_D(doca_graph_add_task(graph, enc_task, NULL)); // 第一个节点，无依赖
    // CHECK_D(doca_graph_add_task(graph, rdma_task, enc_task)); // 依赖 enc_task

    // // Step 9: 提交 Graph 执行
    // CHECK_D(doca_graph_submit(pe, graph));
    // while (doca_graph_has_pending_tasks(graph))
    //     CHECK_D(doca_pe_progress(pe));

    // printf("All tasks finished!\n");

    // // Cleanup（略去错误检查）
    // doca_buf_refcount_rm(pt_buf, NULL);
    // doca_buf_refcount_rm(ct_buf, NULL);
    // doca_graph_destroy(graph);
    // doca_pe_destroy(pe);
    // return 0;
    
    doca_error_t result;
    struct aes_gcm_cfg aes_gcm_cfg;
    char *file_data = NULL;
	size_t file_size;

    init_aes_gcm_params(&aes_gcm_cfg);
    result = read_file(aes_gcm_cfg.file_path, &file_data, &file_size);  // definition

    struct aes_gcm_resources resources = {0};
    struct program_core_objects *state = NULL;
    struct doca_buf *src_doca_buf = NULL;
    struct doca_buf *dst_doca_buf = NULL;
    /* The sample will use 2 doca buffers */
    uint32_t max_bufs = 2;
    char *dst_buffer = NULL;
    uint8_t *resp_head = NULL;
    size_t data_len = 0;
    char *dump = NULL;
    FILE *out_file = NULL;
    struct doca_aes_gcm_key *key = NULL;
    doca_error_t result = DOCA_SUCCESS;
    doca_error_t tmp_result = DOCA_SUCCESS;
    uint64_t max_encrypt_buf_size = 0;

    out_file = fopen(aes_gcm_cfg->output_path, "wr");
    resources.mode = AES_GCM_MODE_ENCRYPT;
    result = allocate_aes_gcm_resources(aes_gcm_cfg->pci_address, max_bufs, &resources);
    state = resources.state;
    result = doca_aes_gcm_cap_task_encrypt_get_max_buf_size(doca_dev_as_devinfo(state->dev), &max_encrypt_buf_size);
    result = doca_ctx_start(state->ctx);
    dst_buffer = calloc(1, max_encrypt_buf_size);
    result = doca_mmap_set_memrange(state->dst_mmap, dst_buffer, max_encrypt_buf_size);
    result = doca_mmap_start(state->dst_mmap);
    result = doca_mmap_set_memrange(state->src_mmap, file_data, file_size);
    result = doca_mmap_start(state->src_mmap);
    /* Construct DOCA buffer for each address range */
    result = doca_buf_inventory_buf_get_by_addr(state->buf_inv, state->src_mmap, file_data, file_size, &src_doca_buf);
    /* Construct DOCA buffer for each address range */
    result = doca_buf_inventory_buf_get_by_addr(state->buf_inv,
                            state->dst_mmap,
                            dst_buffer,
                            max_encrypt_buf_size,
                            &dst_doca_buf);
    /* Set data length in doca buffer */
    result = doca_buf_set_data(src_doca_buf, file_data, file_size);
    /* Create DOCA AES-GCM key */
    result = doca_aes_gcm_key_create(resources.aes_gcm, aes_gcm_cfg->raw_key, aes_gcm_cfg->raw_key_type, &key);
    /* Submit AES-GCM encrypt task */
    result = submit_aes_gcm_encrypt_task(&resources,
                         src_doca_buf,
                         dst_doca_buf,
                         key,
                         (uint8_t *)aes_gcm_cfg->iv,
                         aes_gcm_cfg->iv_length,
                         aes_gcm_cfg->tag_size,
                         aes_gcm_cfg->aad_size);
    /* Write the result to output file */
    doca_buf_get_head(dst_doca_buf, (void **)&resp_head);
    doca_buf_get_data_len(dst_doca_buf, &data_len);
    fwrite(resp_head, sizeof(uint8_t), data_len, out_file);
    /* Print destination buffer data */
    dump = hex_dump(resp_head, data_len);
    if (dump == NULL) {
        DOCA_LOG_ERR("Failed to allocate memory for printing buffer content");
        result = DOCA_ERROR_NO_MEMORY;
        goto destroy_key;
    }
    DOCA_LOG_INFO("AES-GCM encrypted data:\n%s", dump);
    free(dump);


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
	result = allocate_rdma_resources(cfg,
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

	/*
	 * Run the progress engine which will run the state machine defined in rdma_send_state_change_callback()
	 * When the context moves to idle, the context change callback call will signal to stop running the progress
	 * engine.
	 */
	while (resources.run_pe_progress) {
		if (doca_pe_progress(resources.pe) == 0)
			nanosleep(&ts, &ts);
	}

	/* Assign the result we update in the callbacks */
	result = resources.first_encountered_error;


    ///////////////////
    struct graph_sample_state state = {0};

    doca_pe_create(&state->pe);
    EXIT_ON_FAILURE(doca_dma_create(state->device, &state->dma[idx]));
	state->contexts[idx] = doca_dma_as_ctx(state->dma[idx]);
    doca_pe_connect_ctx(state->pe, state->contexts[idx]);
	doca_dma_task_memcpy_set_conf(state->dma[idx],
						      dma_task_completed_callback,
						      dma_task_completed_callback,
						      NUM_GRAPH_INSTANCES);
    doca_ctx_start(state->contexts[i]);

    union doca_data graph_user_data = {};
	uint32_t i = 0;
    doca_graph_create(state->pe, &state->graph);
	doca_graph_node_create_from_user(state->graph, user_node_callback, &state->user_node);
	/* Creating nodes and building the graph */
	for (i = 0; i < NUM_DMA_NODES; i++) {
		doca_graph_node_create_from_ctx(state->graph, state->contexts[i], &state->dma_node[i]);
		/* Setting between the user node and the DMA node */
		doca_graph_add_dependency(state->graph, state->dma_node[i], state->user_node);
	}
	/* Notice that the sample uses the same callback for success & failure. Program can supply different cb */
	doca_graph_set_conf(state->graph,
                    graph_completion_callback,
                    graph_completion_callback,
                    NUM_GRAPH_INSTANCES);
	graph_user_data.ptr = state;
	doca_graph_set_user_data(state->graph, graph_user_data);
	/* Graph must be started before it is added to the work queue. The graph is validated during this call */
	doca_graph_start(state->graph);
    
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
    doca_graph_instance_set_user_node_data(instance->graph_instance, state->user_node, instance);

	graph_instance_user_data.ptr = instance;
	doca_graph_instance_set_user_data(instance->graph_instance, graph_instance_user_data);
    doca_graph_instance_submit(state->instances[i].graph_instance);
    (void)doca_pe_progress(state->pe);

    return DOCA_SUCCESS;
