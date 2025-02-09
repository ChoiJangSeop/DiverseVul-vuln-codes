_zip_read_eocd64(zip_source_t *src, zip_buffer_t *buffer, zip_uint64_t buf_offset, unsigned int flags, zip_error_t *error)
{
    zip_cdir_t *cd;
    zip_uint64_t offset;
    zip_uint8_t eocd[EOCD64LEN];
    zip_uint64_t eocd_offset;
    zip_uint64_t size, nentry, i, eocdloc_offset;
    bool free_buffer;
    zip_uint32_t num_disks, num_disks64, eocd_disk, eocd_disk64;

    eocdloc_offset = _zip_buffer_offset(buffer);

    _zip_buffer_get(buffer, 4); /* magic already verified */

    num_disks = _zip_buffer_get_16(buffer);
    eocd_disk = _zip_buffer_get_16(buffer);
    eocd_offset = _zip_buffer_get_64(buffer);

    if (eocd_offset > ZIP_INT64_MAX || eocd_offset + EOCD64LEN < eocd_offset) {
        zip_error_set(error, ZIP_ER_SEEK, EFBIG);
        return NULL;
    }

    if (eocd_offset + EOCD64LEN > eocdloc_offset + buf_offset) {
	zip_error_set(error, ZIP_ER_INCONS, 0);
	return NULL;
    }

    if (eocd_offset >= buf_offset && eocd_offset + EOCD64LEN <= buf_offset + _zip_buffer_size(buffer)) {
        _zip_buffer_set_offset(buffer, eocd_offset - buf_offset);
        free_buffer = false;
    }
    else {
        if (zip_source_seek(src, (zip_int64_t)eocd_offset, SEEK_SET) < 0) {
            _zip_error_set_from_source(error, src);
            return NULL;
        }
        if ((buffer = _zip_buffer_new_from_source(src, EOCD64LEN, eocd, error)) == NULL) {
            return NULL;
        }
        free_buffer = true;
    }

    if (memcmp(_zip_buffer_get(buffer, 4), EOCD64_MAGIC, 4) != 0) {
	zip_error_set(error, ZIP_ER_INCONS, 0);
        if (free_buffer) {
            _zip_buffer_free(buffer);
        }
	return NULL;
    }

    size = _zip_buffer_get_64(buffer);

    if ((flags & ZIP_CHECKCONS) && size + eocd_offset + 12 != buf_offset + eocdloc_offset) {
	zip_error_set(error, ZIP_ER_INCONS, 0);
        if (free_buffer) {
            _zip_buffer_free(buffer);
        }
        return NULL;
    }

    _zip_buffer_get(buffer, 4); /* skip version made by/needed */

    num_disks64 = _zip_buffer_get_32(buffer);
    eocd_disk64 = _zip_buffer_get_32(buffer);

    /* if eocd values are 0xffff, we have to use eocd64 values.
       otherwise, if the values are not the same, it's inconsistent;
       in any case, if the value is not 0, we don't support it */
    if (num_disks == 0xffff) {
	num_disks = num_disks64;
    }
    if (eocd_disk == 0xffff) {
	eocd_disk = eocd_disk64;
    }
    if ((flags & ZIP_CHECKCONS) && (eocd_disk != eocd_disk64 || num_disks != num_disks64)) {
	zip_error_set(error, ZIP_ER_INCONS, 0);
        if (free_buffer) {
            _zip_buffer_free(buffer);
        }
	return NULL;
    }
    if (num_disks != 0 || eocd_disk != 0) {
	zip_error_set(error, ZIP_ER_MULTIDISK, 0);
        if (free_buffer) {
            _zip_buffer_free(buffer);
        }
	return NULL;
    }

    nentry = _zip_buffer_get_64(buffer);
    i = _zip_buffer_get_64(buffer);

    if (nentry != i) {
	zip_error_set(error, ZIP_ER_MULTIDISK, 0);
        if (free_buffer) {
            _zip_buffer_free(buffer);
        }
	return NULL;
    }

    size = _zip_buffer_get_64(buffer);
    offset = _zip_buffer_get_64(buffer);

    if (!_zip_buffer_ok(buffer)) {
        zip_error_set(error, ZIP_ER_INTERNAL, 0);
        if (free_buffer) {
            _zip_buffer_free(buffer);
        }
        return NULL;
    }

    if (free_buffer) {
        _zip_buffer_free(buffer);
    }

    if (offset > ZIP_INT64_MAX || offset+size < offset) {
        zip_error_set(error, ZIP_ER_SEEK, EFBIG);
        return NULL;
    }
    if ((flags & ZIP_CHECKCONS) && offset+size != eocd_offset) {
	zip_error_set(error, ZIP_ER_INCONS, 0);
	return NULL;
    }

    if ((cd=_zip_cdir_new(nentry, error)) == NULL)
	return NULL;

    cd->is_zip64 = true;
    cd->size = size;
    cd->offset = offset;

    return cd;
}