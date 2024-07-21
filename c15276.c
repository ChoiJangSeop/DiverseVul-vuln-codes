QPDF::pipeForeignStreamData(
    PointerHolder<ForeignStreamData> foreign,
    Pipeline* pipeline,
    unsigned long encode_flags,
    qpdf_stream_decode_level_e decode_level)
{
    if (foreign->encp->encrypted)
    {
        QTC::TC("qpdf", "QPDF pipe foreign encrypted stream");
    }
    return pipeStreamData(
        foreign->encp, foreign->file, *this,
        foreign->foreign_objid, foreign->foreign_generation,
        foreign->offset, foreign->length,
        foreign->local_dict, foreign->is_attachment_stream,
        pipeline, false, false);
}