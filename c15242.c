Pl_DCT::decompress(void* cinfo_p, Buffer* b)
{
    struct jpeg_decompress_struct* cinfo =
        reinterpret_cast<jpeg_decompress_struct*>(cinfo_p);

#if ((defined(__GNUC__) && ((__GNUC__ * 100) + __GNUC_MINOR__) >= 406) || \
     defined(__clang__))
#       pragma GCC diagnostic push
#       pragma GCC diagnostic ignored "-Wold-style-cast"
#endif
    jpeg_create_decompress(cinfo);
#if ((defined(__GNUC__) && ((__GNUC__ * 100) + __GNUC_MINOR__) >= 406) || \
     defined(__clang__))
#       pragma GCC diagnostic pop
#endif
    jpeg_buffer_src(cinfo, b);

    (void) jpeg_read_header(cinfo, TRUE);
    (void) jpeg_calc_output_dimensions(cinfo);

    int width = cinfo->output_width * cinfo->output_components;
    JSAMPARRAY buffer = (*cinfo->mem->alloc_sarray)
        (reinterpret_cast<j_common_ptr>(cinfo), JPOOL_IMAGE, width, 1);

    (void) jpeg_start_decompress(cinfo);
    while (cinfo->output_scanline < cinfo->output_height)
    {
        (void) jpeg_read_scanlines(cinfo, buffer, 1);
        this->getNext()->write(reinterpret_cast<unsigned char*>(buffer[0]),
                               width * sizeof(buffer[0][0]));
    }
    (void) jpeg_finish_decompress(cinfo);
    this->getNext()->finish();
}