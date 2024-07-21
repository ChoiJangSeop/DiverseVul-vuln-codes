QPDF::pipeStreamData(int objid, int generation,
		     qpdf_offset_t offset, size_t length,
		     QPDFObjectHandle stream_dict,
		     Pipeline* pipeline,
                     bool suppress_warnings,
                     bool will_retry)
{
    bool is_attachment_stream = this->m->attachment_streams.count(
        QPDFObjGen(objid, generation));
    return pipeStreamData(
        this->m->encp, this->m->file, *this,
        objid, generation, offset, length,
        stream_dict, is_attachment_stream,
        pipeline, suppress_warnings, will_retry);
}