void qxl_spice_destroy_surfaces(PCIQXLDevice *qxl)
{
    qemu_mutex_lock(&qxl->track_lock);
    qxl->ssd.worker->destroy_surfaces(qxl->ssd.worker);
    memset(&qxl->guest_surfaces.cmds, 0, sizeof(qxl->guest_surfaces.cmds));
    qxl->guest_surfaces.count = 0;
    qemu_mutex_unlock(&qxl->track_lock);
}