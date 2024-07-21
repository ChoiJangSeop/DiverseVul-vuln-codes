int caf_parse(caf_reader_t *reader, int64_t *data_length)
{
    uint32_t fcc;
    int64_t chunk_size;

    *data_length = 0;

    /* CAFFileHeader */
    TRY_IO(pcm_read32be(&reader->io, &fcc));
    ENSURE(fcc == M4AF_FOURCC('c','a','f','f'));
    TRY_IO(pcm_skip(&reader->io, 4)); /* mFileVersion, mFileFlags */

    while ((fcc = caf_next_chunk(reader, &chunk_size)) != 0) {
        if (fcc == M4AF_FOURCC('d','e','s','c'))
            TRY_IO(caf_desc(reader, chunk_size));
        else if (fcc == M4AF_FOURCC('i','n','f','o'))
            TRY_IO(caf_info(reader, chunk_size));
        else if (fcc == M4AF_FOURCC('c','h','a','n')) {
            ENSURE(reader->sample_format.channels_per_frame);
            if (apple_chan_chunk(&reader->io, chunk_size,
                                 &reader->sample_format, reader->chanmap) < 0)
                goto FAIL;
        } else if (fcc == M4AF_FOURCC('d','a','t','a')) {
            TRY_IO(pcm_skip(&reader->io, 4)); /* mEditCount */
            *data_length = (chunk_size == ~0ULL) ? chunk_size : chunk_size - 4;
            reader->data_offset = pcm_tell(&reader->io);
            break;
        } else
            TRY_IO(pcm_skip(&reader->io, chunk_size));
    }
    ENSURE(reader->sample_format.channels_per_frame);
    ENSURE(fcc == M4AF_FOURCC('d','a','t','a'));
    return 0;
FAIL:
    return -1;
}