void qxl_spice_update_area(PCIQXLDevice *qxl, uint32_t surface_id,
                           struct QXLRect *area, struct QXLRect *dirty_rects,
                           uint32_t num_dirty_rects,
                           uint32_t clear_dirty_region)
{
    qxl->ssd.worker->update_area(qxl->ssd.worker, surface_id, area, dirty_rects,
                             num_dirty_rects, clear_dirty_region);
}