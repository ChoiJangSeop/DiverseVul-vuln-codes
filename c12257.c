int wav_parse(wav_reader_t *reader, int64_t *data_length)
{
    uint32_t container, fcc, chunk_size;

    *data_length = 0;
    container = riff_next_chunk(reader, &chunk_size);
    ENSURE(container == RIFF_FOURCC('R','I','F','F') ||
           container == RIFF_FOURCC('R','F','6','4'));
    TRY_IO(pcm_read32le(&reader->io, &fcc));
    ENSURE(fcc == RIFF_FOURCC('W','A','V','E'));

    if (container == RIFF_FOURCC('R','F','6','4'))
        riff_ds64(reader, data_length);
    while ((fcc = riff_next_chunk(reader, &chunk_size)) != 0) {
        if (fcc == RIFF_FOURCC('f','m','t',' ')) {
            if (wav_fmt(reader, chunk_size) < 0)
                goto FAIL;
        } else if (fcc == RIFF_FOURCC('d','a','t','a')) {
            if (container == RIFF_FOURCC('R','I','F','F'))
                *data_length = chunk_size;
            reader->data_offset = pcm_tell(&reader->io);
            break;
        } else {
            TRY_IO(pcm_skip(&reader->io, (chunk_size + 1) & ~1));
        }
    }
    if (fcc == RIFF_FOURCC('d','a','t','a'))
        return 0;
FAIL:
    return -1;
}