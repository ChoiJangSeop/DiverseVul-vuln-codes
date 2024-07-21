static void virgl_cmd_set_scanout(VirtIOGPU *g,
                                  struct virtio_gpu_ctrl_command *cmd)
{
    struct virtio_gpu_set_scanout ss;
    struct virgl_renderer_resource_info info;
    int ret;

    VIRTIO_GPU_FILL_CMD(ss);
    trace_virtio_gpu_cmd_set_scanout(ss.scanout_id, ss.resource_id,
                                     ss.r.width, ss.r.height, ss.r.x, ss.r.y);

    if (ss.scanout_id >= VIRTIO_GPU_MAX_SCANOUT) {
        qemu_log_mask(LOG_GUEST_ERROR, "%s: illegal scanout id specified %d",
                      __func__, ss.scanout_id);
        cmd->error = VIRTIO_GPU_RESP_ERR_INVALID_SCANOUT_ID;
        return;
    }
    g->enable = 1;

    memset(&info, 0, sizeof(info));

    if (ss.resource_id && ss.r.width && ss.r.height) {
        ret = virgl_renderer_resource_get_info(ss.resource_id, &info);
        if (ret == -1) {
            qemu_log_mask(LOG_GUEST_ERROR,
                          "%s: illegal resource specified %d\n",
                          __func__, ss.resource_id);
            cmd->error = VIRTIO_GPU_RESP_ERR_INVALID_RESOURCE_ID;
            return;
        }
        qemu_console_resize(g->scanout[ss.scanout_id].con,
                            ss.r.width, ss.r.height);
        virgl_renderer_force_ctx_0();
        dpy_gl_scanout(g->scanout[ss.scanout_id].con, info.tex_id,
                       info.flags & 1 /* FIXME: Y_0_TOP */,
                       ss.r.x, ss.r.y, ss.r.width, ss.r.height);
    } else {
        if (ss.scanout_id != 0) {
            dpy_gfx_replace_surface(g->scanout[ss.scanout_id].con, NULL);
        }
        dpy_gl_scanout(g->scanout[ss.scanout_id].con, 0, false,
                       0, 0, 0, 0);
    }
    g->scanout[ss.scanout_id].resource_id = ss.resource_id;
}