virgl_cmd_resource_unref(VuGpu *g,
                         struct virtio_gpu_ctrl_command *cmd)
{
    struct virtio_gpu_resource_unref unref;

    VUGPU_FILL_CMD(unref);

    virgl_renderer_resource_unref(unref.resource_id);
}