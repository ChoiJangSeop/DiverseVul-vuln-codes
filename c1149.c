void qemu_spice_create_primary_surface(SimpleSpiceDisplay *ssd, uint32_t id,
                                       QXLDevSurfaceCreate *surface)
{
    ssd->worker->create_primary_surface(ssd->worker, id, surface);
}