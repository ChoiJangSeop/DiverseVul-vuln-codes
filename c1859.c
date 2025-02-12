static int dmg_open(BlockDriverState *bs, QDict *options, int flags,
                    Error **errp)
{
    BDRVDMGState *s = bs->opaque;
    uint64_t info_begin, info_end, last_in_offset, last_out_offset;
    uint32_t count, tmp;
    uint32_t max_compressed_size = 1, max_sectors_per_chunk = 1, i;
    int64_t offset;
    int ret;

    bs->read_only = 1;
    s->n_chunks = 0;
    s->offsets = s->lengths = s->sectors = s->sectorcounts = NULL;

    /* read offset of info blocks */
    offset = bdrv_getlength(bs->file);
    if (offset < 0) {
        ret = offset;
        goto fail;
    }
    offset -= 0x1d8;

    ret = read_uint64(bs, offset, &info_begin);
    if (ret < 0) {
        goto fail;
    } else if (info_begin == 0) {
        ret = -EINVAL;
        goto fail;
    }

    ret = read_uint32(bs, info_begin, &tmp);
    if (ret < 0) {
        goto fail;
    } else if (tmp != 0x100) {
        ret = -EINVAL;
        goto fail;
    }

    ret = read_uint32(bs, info_begin + 4, &count);
    if (ret < 0) {
        goto fail;
    } else if (count == 0) {
        ret = -EINVAL;
        goto fail;
    }
    info_end = info_begin + count;

    offset = info_begin + 0x100;

    /* read offsets */
    last_in_offset = last_out_offset = 0;
    while (offset < info_end) {
        uint32_t type;

        ret = read_uint32(bs, offset, &count);
        if (ret < 0) {
            goto fail;
        } else if (count == 0) {
            ret = -EINVAL;
            goto fail;
        }
        offset += 4;

        ret = read_uint32(bs, offset, &type);
        if (ret < 0) {
            goto fail;
        }

        if (type == 0x6d697368 && count >= 244) {
            size_t new_size;
            uint32_t chunk_count;

            offset += 4;
            offset += 200;

            chunk_count = (count - 204) / 40;
            new_size = sizeof(uint64_t) * (s->n_chunks + chunk_count);
            s->types = g_realloc(s->types, new_size / 2);
            s->offsets = g_realloc(s->offsets, new_size);
            s->lengths = g_realloc(s->lengths, new_size);
            s->sectors = g_realloc(s->sectors, new_size);
            s->sectorcounts = g_realloc(s->sectorcounts, new_size);

            for (i = s->n_chunks; i < s->n_chunks + chunk_count; i++) {
                ret = read_uint32(bs, offset, &s->types[i]);
                if (ret < 0) {
                    goto fail;
                }
                offset += 4;
                if (s->types[i] != 0x80000005 && s->types[i] != 1 &&
                    s->types[i] != 2) {
                    if (s->types[i] == 0xffffffff && i > 0) {
                        last_in_offset = s->offsets[i - 1] + s->lengths[i - 1];
                        last_out_offset = s->sectors[i - 1] +
                                          s->sectorcounts[i - 1];
                    }
                    chunk_count--;
                    i--;
                    offset += 36;
                    continue;
                }
                offset += 4;

                ret = read_uint64(bs, offset, &s->sectors[i]);
                if (ret < 0) {
                    goto fail;
                }
                s->sectors[i] += last_out_offset;
                offset += 8;

                ret = read_uint64(bs, offset, &s->sectorcounts[i]);
                if (ret < 0) {
                    goto fail;
                }
                offset += 8;

                if (s->sectorcounts[i] > DMG_SECTORCOUNTS_MAX) {
                    error_report("sector count %" PRIu64 " for chunk %u is "
                                 "larger than max (%u)",
                                 s->sectorcounts[i], i, DMG_SECTORCOUNTS_MAX);
                    ret = -EINVAL;
                    goto fail;
                }

                ret = read_uint64(bs, offset, &s->offsets[i]);
                if (ret < 0) {
                    goto fail;
                }
                s->offsets[i] += last_in_offset;
                offset += 8;

                ret = read_uint64(bs, offset, &s->lengths[i]);
                if (ret < 0) {
                    goto fail;
                }
                offset += 8;

                if (s->lengths[i] > DMG_LENGTHS_MAX) {
                    error_report("length %" PRIu64 " for chunk %u is larger "
                                 "than max (%u)",
                                 s->lengths[i], i, DMG_LENGTHS_MAX);
                    ret = -EINVAL;
                    goto fail;
                }

                if (s->lengths[i] > max_compressed_size) {
                    max_compressed_size = s->lengths[i];
                }
                if (s->sectorcounts[i] > max_sectors_per_chunk) {
                    max_sectors_per_chunk = s->sectorcounts[i];
                }
            }
            s->n_chunks += chunk_count;
        }
    }

    /* initialize zlib engine */
    s->compressed_chunk = g_malloc(max_compressed_size + 1);
    s->uncompressed_chunk = g_malloc(512 * max_sectors_per_chunk);
    if (inflateInit(&s->zstream) != Z_OK) {
        ret = -EINVAL;
        goto fail;
    }

    s->current_chunk = s->n_chunks;

    qemu_co_mutex_init(&s->lock);
    return 0;

fail:
    g_free(s->types);
    g_free(s->offsets);
    g_free(s->lengths);
    g_free(s->sectors);
    g_free(s->sectorcounts);
    g_free(s->compressed_chunk);
    g_free(s->uncompressed_chunk);
    return ret;
}