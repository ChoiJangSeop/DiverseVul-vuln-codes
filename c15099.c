QPDFWriter::writeObjectStream(QPDFObjectHandle object)
{
    // Note: object might be null if this is a place-holder for an
    // object stream that we are generating from scratch.

    QPDFObjGen old_og = object.getObjGen();
    assert(old_og.getGen() == 0);
    int old_id = old_og.getObj();
    int new_id = this->m->obj_renumber[old_og];

    std::vector<qpdf_offset_t> offsets;
    qpdf_offset_t first = 0;

    // Generate stream itself.  We have to do this in two passes so we
    // can calculate offsets in the first pass.
    PointerHolder<Buffer> stream_buffer;
    int first_obj = -1;
    bool compressed = false;
    for (int pass = 1; pass <= 2; ++pass)
    {
	if (pass == 1)
	{
	    pushDiscardFilter();
	}
	else
	{
	    // Adjust offsets to skip over comment before first object

	    first = offsets.at(0);
	    for (std::vector<qpdf_offset_t>::iterator iter = offsets.begin();
		 iter != offsets.end(); ++iter)
	    {
		*iter -= first;
	    }

	    // Take one pass at writing pairs of numbers so we can get
	    // their size information
	    pushDiscardFilter();
	    writeObjectStreamOffsets(offsets, first_obj);
	    first += this->m->pipeline->getCount();
	    popPipelineStack();

	    // Set up a stream to write the stream data into a buffer.
	    Pipeline* next = pushPipeline(new Pl_Buffer("object stream"));
            if ((this->m->compress_streams ||
                 (this->m->stream_decode_level == qpdf_dl_none)) &&
                (! this->m->qdf_mode))
	    {
		compressed = true;
		next = pushPipeline(
		    new Pl_Flate("compress object stream", next,
				 Pl_Flate::a_deflate));
	    }
	    activatePipelineStack();
	    writeObjectStreamOffsets(offsets, first_obj);
	}

	int count = 0;
	for (std::set<QPDFObjGen>::iterator iter =
		 this->m->object_stream_to_objects[old_id].begin();
	     iter != this->m->object_stream_to_objects[old_id].end();
	     ++iter, ++count)
	{
	    QPDFObjGen obj = *iter;
	    int new_obj = this->m->obj_renumber[obj];
	    if (first_obj == -1)
	    {
		first_obj = new_obj;
	    }
	    if (this->m->qdf_mode)
	    {
		writeString("%% Object stream: object " +
			    QUtil::int_to_string(new_obj) + ", index " +
			    QUtil::int_to_string(count));
		if (! this->m->suppress_original_object_ids)
		{
		    writeString("; original object ID: " +
				QUtil::int_to_string(obj.getObj()));
                    // For compatibility, only write the generation if
                    // non-zero.  While object streams only allow
                    // objects with generation 0, if we are generating
                    // object streams, the old object could have a
                    // non-zero generation.
                    if (obj.getGen() != 0)
                    {
                        QTC::TC("qpdf", "QPDFWriter original obj non-zero gen");
                        writeString(" " + QUtil::int_to_string(obj.getGen()));
                    }
		}
		writeString("\n");
	    }
	    if (pass == 1)
	    {
		offsets.push_back(this->m->pipeline->getCount());
                // To avoid double-counting objects being written in
                // object streams for progress reporting, decrement in
                // pass 1.
                indicateProgress(true, false);
	    }
	    writeObject(this->m->pdf.getObjectByObjGen(obj), count);

	    this->m->xref[new_obj] = QPDFXRefEntry(2, new_id, count);
	}

	// stream_buffer will be initialized only for pass 2
	popPipelineStack(&stream_buffer);
    }

    // Write the object
    openObject(new_id);
    setDataKey(new_id);
    writeString("<<");
    writeStringQDF("\n ");
    writeString(" /Type /ObjStm");
    writeStringQDF("\n ");
    size_t length = stream_buffer->getSize();
    adjustAESStreamLength(length);
    writeString(" /Length " + QUtil::int_to_string(length));
    writeStringQDF("\n ");
    if (compressed)
    {
	writeString(" /Filter /FlateDecode");
    }
    writeString(" /N " + QUtil::int_to_string(offsets.size()));
    writeStringQDF("\n ");
    writeString(" /First " + QUtil::int_to_string(first));
    if (! object.isNull())
    {
	// If the original object has an /Extends key, preserve it.
	QPDFObjectHandle dict = object.getDict();
	QPDFObjectHandle extends = dict.getKey("/Extends");
	if (extends.isIndirect())
	{
	    QTC::TC("qpdf", "QPDFWriter copy Extends");
	    writeStringQDF("\n ");
	    writeString(" /Extends ");
	    unparseChild(extends, 1, f_in_ostream);
	}
    }
    writeStringQDF("\n");
    writeStringNoQDF(" ");
    writeString(">>\nstream\n");
    if (this->m->encrypted)
    {
	QTC::TC("qpdf", "QPDFWriter encrypt object stream");
    }
    pushEncryptionFilter();
    writeBuffer(stream_buffer);
    popPipelineStack();
    if (this->m->newline_before_endstream)
    {
        writeString("\n");
    }
    writeString("endstream");
    this->m->cur_data_key.clear();
    closeObject(new_id);
}