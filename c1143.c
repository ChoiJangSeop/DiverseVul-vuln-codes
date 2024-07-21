void qemu_spice_destroy_primary_surface(SimpleSpiceDisplay *ssd, uint32_t id)
{
    ssd->worker->destroy_primary_surface(ssd->worker, id);
}