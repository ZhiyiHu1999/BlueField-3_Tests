# DOCA AES-GCM Encrypt Sample

## ⚙️ Build Instructions

```bash
cd /opt/mellanox/doca/samples/doca_aes_gcm/aes_gcm_encrypt

# Configure the build directory
meson /tmp/build_samples/aes_gcm_encrypt

# Build the sample
ninja -C /tmp/build_samples/aes_gcm_encrypt

# Run the test
bash benchmarking.sh 
```
