QPDF::QPDF() :
    encrypted(false),
    encryption_initialized(false),
    ignore_xref_streams(false),
    suppress_warnings(false),
    out_stream(&std::cout),
    err_stream(&std::cerr),
    attempt_recovery(true),
    encryption_V(0),
    encryption_R(0),
    encrypt_metadata(true),
    cf_stream(e_none),
    cf_string(e_none),
    cf_file(e_none),
    cached_key_objid(0),
    cached_key_generation(0),
    pushed_inherited_attributes_to_pages(false),
    copied_stream_data_provider(0),
    first_xref_item_offset(0),
    uncompressed_after_compressed(false)
{
}