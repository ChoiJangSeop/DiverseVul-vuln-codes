static void test16(char const* infile,
		   char const* password,
		   char const* outfile,
		   char const* outfile2)
{
    unsigned long buflen = 0L;
    unsigned char const* buf = 0;
    FILE* f = 0;

    qpdf_read(qpdf, infile, password);
    print_info("/Author");
    print_info("/Producer");
    print_info("/Creator");
    qpdf_set_info_key(qpdf, "/Author", "Mr. Potato Head");
    qpdf_set_info_key(qpdf, "/Producer", "QPDF library");
    qpdf_set_info_key(qpdf, "/Creator", 0);
    print_info("/Author");
    print_info("/Producer");
    print_info("/Creator");
    qpdf_init_write_memory(qpdf);
    qpdf_set_static_ID(qpdf, QPDF_TRUE);
    qpdf_set_static_aes_IV(qpdf, QPDF_TRUE);
    qpdf_set_stream_data_mode(qpdf, qpdf_s_uncompress);
    qpdf_write(qpdf);
    f = safe_fopen(outfile, "wb");
    buflen = qpdf_get_buffer_length(qpdf);
    buf = qpdf_get_buffer(qpdf);
    fwrite(buf, 1, buflen, f);
    fclose(f);
    report_errors();
}