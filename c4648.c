static void virtio_gpu_device_realize(DeviceState *qdev, Error **errp)
{
    VirtIODevice *vdev = VIRTIO_DEVICE(qdev);
    VirtIOGPU *g = VIRTIO_GPU(qdev);
    bool have_virgl;
    int i;

    if (g->conf.max_outputs > VIRTIO_GPU_MAX_SCANOUT) {
        error_setg(errp, "invalid max_outputs > %d", VIRTIO_GPU_MAX_SCANOUT);
        return;
    }

    g->config_size = sizeof(struct virtio_gpu_config);
    g->virtio_config.num_scanouts = g->conf.max_outputs;
    virtio_init(VIRTIO_DEVICE(g), "virtio-gpu", VIRTIO_ID_GPU,
                g->config_size);

    g->req_state[0].width = 1024;
    g->req_state[0].height = 768;

    g->use_virgl_renderer = false;
#if !defined(CONFIG_VIRGL) || defined(HOST_WORDS_BIGENDIAN)
    have_virgl = false;
#else
    have_virgl = display_opengl;
#endif
    if (!have_virgl) {
        g->conf.flags &= ~(1 << VIRTIO_GPU_FLAG_VIRGL_ENABLED);
    }

    if (virtio_gpu_virgl_enabled(g->conf)) {
        /* use larger control queue in 3d mode */
        g->ctrl_vq   = virtio_add_queue(vdev, 256, virtio_gpu_handle_ctrl_cb);
        g->cursor_vq = virtio_add_queue(vdev, 16, virtio_gpu_handle_cursor_cb);
        g->virtio_config.num_capsets = 1;
    } else {
        g->ctrl_vq   = virtio_add_queue(vdev, 64, virtio_gpu_handle_ctrl_cb);
        g->cursor_vq = virtio_add_queue(vdev, 16, virtio_gpu_handle_cursor_cb);
    }

    g->ctrl_bh = qemu_bh_new(virtio_gpu_ctrl_bh, g);
    g->cursor_bh = qemu_bh_new(virtio_gpu_cursor_bh, g);
    QTAILQ_INIT(&g->reslist);
    QTAILQ_INIT(&g->cmdq);
    QTAILQ_INIT(&g->fenceq);

    g->enabled_output_bitmask = 1;
    g->qdev = qdev;

    for (i = 0; i < g->conf.max_outputs; i++) {
        g->scanout[i].con =
            graphic_console_init(DEVICE(g), i, &virtio_gpu_ops, g);
        if (i > 0) {
            dpy_gfx_replace_surface(g->scanout[i].con, NULL);
        }
    }

    vmstate_register(qdev, -1, &vmstate_virtio_gpu_unmigratable, g);
}