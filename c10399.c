virgl_resource_attach_backing(VuGpu *g,
                              struct virtio_gpu_ctrl_command *cmd)
{
    struct virtio_gpu_resource_attach_backing att_rb;
    struct iovec *res_iovs;
    int ret;

    VUGPU_FILL_CMD(att_rb);

    ret = vg_create_mapping_iov(g, &att_rb, cmd, &res_iovs);
    if (ret != 0) {
        cmd->error = VIRTIO_GPU_RESP_ERR_UNSPEC;
        return;
    }

    virgl_renderer_resource_attach_iov(att_rb.resource_id,
                                       res_iovs, att_rb.nr_entries);
}