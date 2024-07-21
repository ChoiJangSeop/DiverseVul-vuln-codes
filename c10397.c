vg_resource_destroy(VuGpu *g,
                    struct virtio_gpu_simple_resource *res)
{
    int i;

    if (res->scanout_bitmask) {
        for (i = 0; i < VIRTIO_GPU_MAX_SCANOUTS; i++) {
            if (res->scanout_bitmask & (1 << i)) {
                vg_disable_scanout(g, i);
            }
        }
    }

    vugbm_buffer_destroy(&res->buffer);
    pixman_image_unref(res->image);
    QTAILQ_REMOVE(&g->reslist, res, next);
    g_free(res);
}