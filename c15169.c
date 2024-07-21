QPDF_Stream::replaceDict(QPDFObjectHandle new_dict)
{
    this->stream_dict = new_dict;
    setDictDescription();
    QPDFObjectHandle length_obj = new_dict.getKey("/Length");
    if (length_obj.isInteger())
    {
        this->length = length_obj.getIntValue();
    }
    else
    {
        this->length = 0;
    }
}