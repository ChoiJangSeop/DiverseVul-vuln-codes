static int vdi_open(BlockDriverState *bs, QDict *options, int flags,
                    Error **errp)
{
    BDRVVdiState *s = bs->opaque;
    VdiHeader header;
    size_t bmap_size;
    int ret;

    logout("\n");

    ret = bdrv_read(bs->file, 0, (uint8_t *)&header, 1);
    if (ret < 0) {
        goto fail;
    }

    vdi_header_to_cpu(&header);
#if defined(CONFIG_VDI_DEBUG)
    vdi_header_print(&header);
#endif

    if (header.disk_size % SECTOR_SIZE != 0) {
        /* 'VBoxManage convertfromraw' can create images with odd disk sizes.
           We accept them but round the disk size to the next multiple of
           SECTOR_SIZE. */
        logout("odd disk size %" PRIu64 " B, round up\n", header.disk_size);
        header.disk_size += SECTOR_SIZE - 1;
        header.disk_size &= ~(SECTOR_SIZE - 1);
    }

    if (header.signature != VDI_SIGNATURE) {
        error_setg(errp, "Image not in VDI format (bad signature %08x)", header.signature);
        ret = -EINVAL;
        goto fail;
    } else if (header.version != VDI_VERSION_1_1) {
        error_setg(errp, "unsupported VDI image (version %u.%u)",
                   header.version >> 16, header.version & 0xffff);
        ret = -ENOTSUP;
        goto fail;
    } else if (header.offset_bmap % SECTOR_SIZE != 0) {
        /* We only support block maps which start on a sector boundary. */
        error_setg(errp, "unsupported VDI image (unaligned block map offset "
                   "0x%x)", header.offset_bmap);
        ret = -ENOTSUP;
        goto fail;
    } else if (header.offset_data % SECTOR_SIZE != 0) {
        /* We only support data blocks which start on a sector boundary. */
        error_setg(errp, "unsupported VDI image (unaligned data offset 0x%x)",
                   header.offset_data);
        ret = -ENOTSUP;
        goto fail;
    } else if (header.sector_size != SECTOR_SIZE) {
        error_setg(errp, "unsupported VDI image (sector size %u is not %u)",
                   header.sector_size, SECTOR_SIZE);
        ret = -ENOTSUP;
        goto fail;
    } else if (header.block_size != 1 * MiB) {
        error_setg(errp, "unsupported VDI image (sector size %u is not %u)",
                   header.block_size, 1 * MiB);
        ret = -ENOTSUP;
        goto fail;
    } else if (header.disk_size >
               (uint64_t)header.blocks_in_image * header.block_size) {
        error_setg(errp, "unsupported VDI image (disk size %" PRIu64 ", "
                   "image bitmap has room for %" PRIu64 ")",
                   header.disk_size,
                   (uint64_t)header.blocks_in_image * header.block_size);
        ret = -ENOTSUP;
        goto fail;
    } else if (!uuid_is_null(header.uuid_link)) {
        error_setg(errp, "unsupported VDI image (non-NULL link UUID)");
        ret = -ENOTSUP;
        goto fail;
    } else if (!uuid_is_null(header.uuid_parent)) {
        error_setg(errp, "unsupported VDI image (non-NULL parent UUID)");
        ret = -ENOTSUP;
        goto fail;
    }

    bs->total_sectors = header.disk_size / SECTOR_SIZE;

    s->block_size = header.block_size;
    s->block_sectors = header.block_size / SECTOR_SIZE;
    s->bmap_sector = header.offset_bmap / SECTOR_SIZE;
    s->header = header;

    bmap_size = header.blocks_in_image * sizeof(uint32_t);
    bmap_size = (bmap_size + SECTOR_SIZE - 1) / SECTOR_SIZE;
    s->bmap = g_malloc(bmap_size * SECTOR_SIZE);
    ret = bdrv_read(bs->file, s->bmap_sector, (uint8_t *)s->bmap, bmap_size);
    if (ret < 0) {
        goto fail_free_bmap;
    }

    /* Disable migration when vdi images are used */
    error_set(&s->migration_blocker,
              QERR_BLOCK_FORMAT_FEATURE_NOT_SUPPORTED,
              "vdi", bs->device_name, "live migration");
    migrate_add_blocker(s->migration_blocker);

    return 0;

 fail_free_bmap:
    g_free(s->bmap);

 fail:
    return ret;
}