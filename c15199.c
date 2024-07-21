QPDFWriter::writeXRefStream(int xref_id, int max_id, qpdf_offset_t max_offset,
			    trailer_e which, int first, int last, int size,
			    qpdf_offset_t prev, int hint_id,
			    qpdf_offset_t hint_offset,
                            qpdf_offset_t hint_length,
			    bool skip_compression,
                            int linearization_pass)
{
    qpdf_offset_t xref_offset = this->m->pipeline->getCount();
    qpdf_offset_t space_before_zero = xref_offset - 1;

    // field 1 contains offsets and object stream identifiers
    int f1_size = std::max(bytesNeeded(max_offset + hint_length),
			   bytesNeeded(max_id));

    // field 2 contains object stream indices
    int f2_size = bytesNeeded(this->m->max_ostream_index);

    unsigned int esize = 1 + f1_size + f2_size;

    // Must store in xref table in advance of writing the actual data
    // rather than waiting for openObject to do it.
    this->m->xref[xref_id] = QPDFXRefEntry(1, this->m->pipeline->getCount(), 0);

    Pipeline* p = pushPipeline(new Pl_Buffer("xref stream"));
    bool compressed = false;
    if ((this->m->compress_streams ||
         (this->m->stream_decode_level == qpdf_dl_none)) &&
        (! this->m->qdf_mode))
    {
	compressed = true;
	if (! skip_compression)
	{
	    // Write the stream dictionary for compression but don't
	    // actually compress.  This helps us with computation of
	    // padding for pass 1 of linearization.
	    p = pushPipeline(
		new Pl_Flate("compress xref", p, Pl_Flate::a_deflate));
	}
	p = pushPipeline(
	    new Pl_PNGFilter(
		"pngify xref", p, Pl_PNGFilter::a_encode, esize));
    }
    activatePipelineStack();
    for (int i = first; i <= last; ++i)
    {
	QPDFXRefEntry& e = this->m->xref[i];
	switch (e.getType())
	{
	  case 0:
	    writeBinary(0, 1);
	    writeBinary(0, f1_size);
	    writeBinary(0, f2_size);
	    break;

	  case 1:
	    {
		qpdf_offset_t offset = e.getOffset();
		if ((hint_id != 0) &&
		    (i != hint_id) &&
		    (offset >= hint_offset))
		{
		    offset += hint_length;
		}
		writeBinary(1, 1);
		writeBinary(offset, f1_size);
		writeBinary(0, f2_size);
	    }
	    break;

	  case 2:
	    writeBinary(2, 1);
	    writeBinary(e.getObjStreamNumber(), f1_size);
	    writeBinary(e.getObjStreamIndex(), f2_size);
	    break;

	  default:
	    throw std::logic_error("invalid type writing xref stream");
	    break;
	}
    }
    PointerHolder<Buffer> xref_data;
    popPipelineStack(&xref_data);

    openObject(xref_id);
    writeString("<<");
    writeStringQDF("\n ");
    writeString(" /Type /XRef");
    writeStringQDF("\n ");
    writeString(" /Length " + QUtil::int_to_string(xref_data->getSize()));
    if (compressed)
    {
	writeStringQDF("\n ");
	writeString(" /Filter /FlateDecode");
	writeStringQDF("\n ");
	writeString(" /DecodeParms << /Columns " +
		    QUtil::int_to_string(esize) + " /Predictor 12 >>");
    }
    writeStringQDF("\n ");
    writeString(" /W [ 1 " +
		QUtil::int_to_string(f1_size) + " " +
		QUtil::int_to_string(f2_size) + " ]");
    if (! ((first == 0) && (last == size - 1)))
    {
	writeString(" /Index [ " +
		    QUtil::int_to_string(first) + " " +
		    QUtil::int_to_string(last - first + 1) + " ]");
    }
    writeTrailer(which, size, true, prev, linearization_pass);
    writeString("\nstream\n");
    writeBuffer(xref_data);
    writeString("\nendstream");
    closeObject(xref_id);
    return space_before_zero;
}