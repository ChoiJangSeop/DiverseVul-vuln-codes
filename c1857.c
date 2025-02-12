static int64_t seek_to_sector(BlockDriverState *bs, int64_t sector_num)
{
    BDRVBochsState *s = bs->opaque;
    int64_t offset = sector_num * 512;
    int64_t extent_index, extent_offset, bitmap_offset;
    char bitmap_entry;

    // seek to sector
    extent_index = offset / s->extent_size;
    extent_offset = (offset % s->extent_size) / 512;

    if (s->catalog_bitmap[extent_index] == 0xffffffff) {
	return -1; /* not allocated */
    }

    bitmap_offset = s->data_offset + (512 * s->catalog_bitmap[extent_index] *
	(s->extent_blocks + s->bitmap_blocks));

    /* read in bitmap for current extent */
    if (bdrv_pread(bs->file, bitmap_offset + (extent_offset / 8),
                   &bitmap_entry, 1) != 1) {
        return -1;
    }

    if (!((bitmap_entry >> (extent_offset % 8)) & 1)) {
	return -1; /* not allocated */
    }

    return bitmap_offset + (512 * (s->bitmap_blocks + extent_offset));
}