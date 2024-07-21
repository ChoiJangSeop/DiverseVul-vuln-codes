QPDF_Stream::replaceFilterData(QPDFObjectHandle const& filter,
			       QPDFObjectHandle const& decode_parms,
			       size_t length)
{
    this->stream_dict.replaceOrRemoveKey("/Filter", filter);
    this->stream_dict.replaceOrRemoveKey("/DecodeParms", decode_parms);
    if (length == 0)
    {
        QTC::TC("qpdf", "QPDF_Stream unknown stream length");
        this->stream_dict.removeKey("/Length");
    }
    else
    {
        this->stream_dict.replaceKey(
            "/Length", QPDFObjectHandle::newInteger(length));
    }
}