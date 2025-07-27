#include <stdlib.h>
#include <string.h>
#include <doca_argp.h>
#include <doca_aes_gcm.h>
#include <doca_dev.h>
#include <doca_log.h>
#include <doca_error.h>

#include <utils.h>

#include "aes_gcm_common.h"
#include "rdma_common.h"
// #include "utils.h"


DOCA_LOG_REGISTER(AESGCM_RDMA::MAIN);

struct aes_gcm_rdma_send_cfg;
doca_error_t aes_gcm_encrypt(struct aes_gcm_cfg *cfg, char *file_data, size_t file_size);
doca_error_t rdma_send(struct rdma_config *cfg);

int main(int argc, char **argv)
{
    doca_error_t result;
    struct aes_gcm_rdma_send_cfg cfg;
    struct doca_log_backend *sdk_log;
    int exit_status = EXIT_FAILURE;

	/* Set the default configuration values (Example values) */
	result = set_default_config_value(&cfg);
	if (result != DOCA_SUCCESS) goto sample_exit;

    /* AES-GCM LOG */
	result = doca_log_backend_create_standard();
	if (result != DOCA_SUCCESS) goto sample_exit;

    result = doca_log_backend_create_with_file_sdk(stderr, &sdk_log);
    if (result != DOCA_SUCCESS) goto exit;

    result = doca_log_backend_set_sdk_level(sdk_log, DOCA_LOG_LEVEL_WARNING);
    if (result != DOCA_SUCCESS) goto exit;

    DOCA_LOG_INFO("Starting AES-GCM + RDMA send sample");

    /* AES-GCM ARGP */
    init_aes_gcm_params(&cfg);

    result = doca_argp_init("aesgcm_rdma", &cfg);
    if (result != DOCA_SUCCESS) goto exit;

    result = register_aes_gcm_params();
    if (result != DOCA_SUCCESS) goto argp_cleanup;

    /* RDMA ARGP */
    result = doca_argp_register_params_struct(&cfg, sizeof(struct rdma_config));
    if (result != DOCA_SUCCESS) goto argp_cleanup;
    result = register_rdma_common_params();
    if (result != DOCA_SUCCESS) goto argp_cleanup;
    result = register_rdma_send_string_param();
    if (result != DOCA_SUCCESS) goto argp_cleanup;

    /* Parse args */
    result = doca_argp_start(argc, argv);
    if (result != DOCA_SUCCESS) goto argp_cleanup;

    /* AES-GCM Encryption args */
    result = read_file(cfg.file_path, NULL, NULL); // Just test file exists
    if (result != DOCA_SUCCESS) {
        DOCA_LOG_ERR("Input file not found");
        goto argp_cleanup;
    }

    result = aes_gcm_encrypt(&cfg, NULL, 0);
    if (result != DOCA_SUCCESS) {
        DOCA_LOG_ERR("AES-GCM encryption failed");
        goto argp_cleanup;
    }

    /* RDMA send the encrypted file */
    cfg.send_file_path = encrypted_file_path;

    result = rdma_send(&cfg);
    if (result != DOCA_SUCCESS) {
        DOCA_LOG_ERR("RDMA send failed");
        goto argp_cleanup;
    }

    DOCA_LOG_INFO("Encryption and RDMA send completed successfully");
    exit_status = EXIT_SUCCESS;

argp_cleanup:
    doca_argp_destroy();
exit:
    return exit_status;
}
