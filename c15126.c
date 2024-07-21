Pl_DCT::compress(void* cinfo_p, Buffer* b)
{
    struct jpeg_compress_struct* cinfo =
        reinterpret_cast<jpeg_compress_struct*>(cinfo_p);

#if ((defined(__GNUC__) && ((__GNUC__ * 100) + __GNUC_MINOR__) >= 406) || \
     defined(__clang__))
#       pragma GCC diagnostic push
#       pragma GCC diagnostic ignored "-Wold-style-cast"
#endif
    jpeg_create_compress(cinfo);
#if ((defined(__GNUC__) && ((__GNUC__ * 100) + __GNUC_MINOR__) >= 406) || \
     defined(__clang__))
#       pragma GCC diagnostic pop
#endif
    static int const BUF_SIZE = 65536;
    PointerHolder<unsigned char> outbuffer_ph(
        true, new unsigned char[BUF_SIZE]);
    unsigned char* outbuffer = outbuffer_ph.getPointer();
    jpeg_pipeline_dest(cinfo, outbuffer, BUF_SIZE, this->getNext());

    cinfo->image_width = this->image_width;
    cinfo->image_height = this->image_height;
    cinfo->input_components = this->components;
    cinfo->in_color_space = this->color_space;
    jpeg_set_defaults(cinfo);
    if (this->config_callback)
    {
        this->config_callback->apply(cinfo);
    }

    jpeg_start_compress(cinfo, TRUE);

    int width = cinfo->image_width * cinfo->input_components;
    size_t expected_size =
        cinfo->image_height * cinfo->image_width * cinfo->input_components;
    if (b->getSize() != expected_size)
    {
        throw std::runtime_error(
            "Pl_DCT: image buffer size = " +
            QUtil::int_to_string(b->getSize()) + "; expected size = " +
            QUtil::int_to_string(expected_size));
    }
    JSAMPROW row_pointer[1];
    unsigned char* buffer = b->getBuffer();
    while (cinfo->next_scanline < cinfo->image_height)
    {
        // We already verified that the buffer is big enough.
        row_pointer[0] = &buffer[cinfo->next_scanline * width];
        (void) jpeg_write_scanlines(cinfo, row_pointer, 1);
    }
    jpeg_finish_compress(cinfo);
    this->getNext()->finish();
}