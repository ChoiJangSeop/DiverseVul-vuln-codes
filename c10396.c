vg_resource_attach_backing(VuGpu *g,
                           struct virtio_gpu_ctrl_command *cmd)
{
    struct virtio_gpu_simple_resource *res;
    struct virtio_gpu_resource_attach_backing ab;
    int ret;

    VUGPU_FILL_CMD(ab);
    virtio_gpu_bswap_32(&ab, sizeof(ab));

    res = virtio_gpu_find_resource(g, ab.resource_id);
    if (!res) {
        g_critical("%s: illegal resource specified %d",
                   __func__, ab.resource_id);
        cmd->error = VIRTIO_GPU_RESP_ERR_INVALID_RESOURCE_ID;
        return;
    }

    ret = vg_create_mapping_iov(g, &ab, cmd, &res->iov);
    if (ret != 0) {
        cmd->error = VIRTIO_GPU_RESP_ERR_UNSPEC;
        return;
    }

    res->iov_cnt = ab.nr_entries;
}