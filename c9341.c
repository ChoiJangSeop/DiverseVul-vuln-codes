int quic_decode_begin(QuicContext *quic, uint32_t *io_ptr, unsigned int num_io_words,
                      QuicImageType *out_type, int *out_width, int *out_height)
{
    Encoder *encoder = (Encoder *)quic;
    uint32_t *io_ptr_end = io_ptr + num_io_words;
    QuicImageType type;
    int width;
    int height;
    uint32_t magic;
    uint32_t version;
    int channels;
    int bpc;

    if (!encoder_reset(encoder, io_ptr, io_ptr_end)) {
        return QUIC_ERROR;
    }

    init_decode_io(encoder);

    magic = encoder->io_word;
    decode_eat32bits(encoder);
    if (magic != QUIC_MAGIC) {
        encoder->usr->warn(encoder->usr, "bad magic\n");
        return QUIC_ERROR;
    }

    version = encoder->io_word;
    decode_eat32bits(encoder);
    if (version != QUIC_VERSION) {
        encoder->usr->warn(encoder->usr, "bad version\n");
        return QUIC_ERROR;
    }

    type = (QuicImageType)encoder->io_word;
    decode_eat32bits(encoder);

    width = encoder->io_word;
    decode_eat32bits(encoder);

    height = encoder->io_word;
    decode_eat32bits(encoder);

    quic_image_params(encoder, type, &channels, &bpc);

    if (!encoder_reset_channels(encoder, channels, width, bpc)) {
        return QUIC_ERROR;
    }

    *out_width = encoder->width = width;
    *out_height = encoder->height = height;
    *out_type = encoder->type = type;
    return QUIC_OK;
}