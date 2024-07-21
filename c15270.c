QPDF::readHintStream(Pipeline& pl, qpdf_offset_t offset, size_t length)
{
    int obj;
    int gen;
    QPDFObjectHandle H = readObjectAtOffset(
	false, offset, "linearization hint stream", -1, 0, obj, gen);
    ObjCache& oc = this->m->obj_cache[QPDFObjGen(obj, gen)];
    qpdf_offset_t min_end_offset = oc.end_before_space;
    qpdf_offset_t max_end_offset = oc.end_after_space;
    if (! H.isStream())
    {
	throw QPDFExc(qpdf_e_damaged_pdf, this->m->file->getName(),
		      "linearization dictionary",
		      this->m->file->getLastOffset(),
		      "hint table is not a stream");
    }

    QPDFObjectHandle Hdict = H.getDict();

    // Some versions of Acrobat make /Length indirect and place it
    // immediately after the stream, increasing length to cover it,
    // even though the specification says all objects in the
    // linearization parameter dictionary must be direct.  We have to
    // get the file position of the end of length in this case.
    QPDFObjectHandle length_obj = Hdict.getKey("/Length");
    if (length_obj.isIndirect())
    {
	QTC::TC("qpdf", "QPDF hint table length indirect");
	// Force resolution
	(void) length_obj.getIntValue();
	ObjCache& oc = this->m->obj_cache[length_obj.getObjGen()];
	min_end_offset = oc.end_before_space;
	max_end_offset = oc.end_after_space;
    }
    else
    {
	QTC::TC("qpdf", "QPDF hint table length direct");
    }
    qpdf_offset_t computed_end = offset + length;
    if ((computed_end < min_end_offset) ||
	(computed_end > max_end_offset))
    {
	*this->m->out_stream << "expected = " << computed_end
                             << "; actual = " << min_end_offset << ".."
                             << max_end_offset << std::endl;
	throw QPDFExc(qpdf_e_damaged_pdf, this->m->file->getName(),
		      "linearization dictionary",
		      this->m->file->getLastOffset(),
		      "hint table length mismatch");
    }
    H.pipeStreamData(&pl, 0, qpdf_dl_specialized);
    return Hdict;
}