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

/**
 * It is recommended to put the graph instance data in a struct.
 * Notice that graph tasks life span must be >= life span of the graph instance.
 */
struct graph_instance_data {
	uint32_t index; /* Index is used for printing */
	struct doca_graph_instance *graph_instance;
	struct doca_buf *source;
	uint8_t *source_addr;

	struct doca_dma_task_memcpy *aes_gcm_task;
	struct doca_dma_task_memcpy *rdma_task;
	struct doca_buf *aes_gcm_src_buf;
	struct doca_buf *aes_gcm_dst_buf;
	// uint8_t *dma_dest_addr[NUM_DMA_NODES];
};

/**
 * This struct defines the program context.
 */
struct graph_sample_state {
	/**
	 * Resources
	 */
	struct doca_dev *dev;
	struct doca_pe *pe;
	struct doca_buf_inventory *inventory;
	
    struct doca_ctx *context_aes_gcm;
	struct doca_aes_gcm *aes_gcm;
	struct doca_mmap *src_mmap_aes_gcm;
	struct doca_mmap *dst_mmap_aes_gcm;
	
	struct rdma_resources resources_rdma;
	struct doca_ctx *context_rdma;
	struct doca_rdma *rdma;

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

	struct doca_graph_node *node_aes_gcm;
    struct doca_graph_node *node_rdma;
	struct doca_graph_node *node_user;

	/* Array of graph instances. All will be submitted to the work queue at once */
	struct graph_instance_data instance;

	uint32_t num_completed_instances;
};
