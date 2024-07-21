void qxl_spice_destroy_surface_wait(PCIQXLDevice *qxl, uint32_t id)
{
    qemu_mutex_lock(&qxl->track_lock);
    PANIC_ON(id >= NUM_SURFACES);
    qxl->ssd.worker->destroy_surface_wait(qxl->ssd.worker, id);
    qxl->guest_surfaces.cmds[id] = 0;
    qxl->guest_surfaces.count--;
    qemu_mutex_unlock(&qxl->track_lock);
}