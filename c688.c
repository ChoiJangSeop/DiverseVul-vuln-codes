static uint32_t scsi_init_iovec(SCSIDiskReq *r)
{
    r->iov.iov_len = MIN(r->sector_count * 512, SCSI_DMA_BUF_SIZE);
    qemu_iovec_init_external(&r->qiov, &r->iov, 1);
    return r->qiov.size / 512;
}