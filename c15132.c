QPDF::resolveObjectsInStream(int obj_stream_number)
{
    // Force resolution of object stream
    QPDFObjectHandle obj_stream = getObjectByID(obj_stream_number, 0);
    if (! obj_stream.isStream())
    {
	throw QPDFExc(qpdf_e_damaged_pdf, this->m->file->getName(),
		      this->m->last_object_description,
		      this->m->file->getLastOffset(),
		      "supposed object stream " +
		      QUtil::int_to_string(obj_stream_number) +
		      " is not a stream");
    }

    // For linearization data in the object, use the data from the
    // object stream for the objects in the stream.
    QPDFObjGen stream_og(obj_stream_number, 0);
    qpdf_offset_t end_before_space =
        this->m->obj_cache[stream_og].end_before_space;
    qpdf_offset_t end_after_space =
        this->m->obj_cache[stream_og].end_after_space;

    QPDFObjectHandle dict = obj_stream.getDict();
    if (! (dict.getKey("/Type").isName() &&
	   dict.getKey("/Type").getName() == "/ObjStm"))
    {
	QTC::TC("qpdf", "QPDF ERR object stream with wrong type");
	throw QPDFExc(qpdf_e_damaged_pdf, this->m->file->getName(),
		      this->m->last_object_description,
		      this->m->file->getLastOffset(),
		      "supposed object stream " +
		      QUtil::int_to_string(obj_stream_number) +
		      " has wrong type");
    }

    if (! (dict.getKey("/N").isInteger() &&
	   dict.getKey("/First").isInteger()))
    {
	throw QPDFExc(qpdf_e_damaged_pdf, this->m->file->getName(),
		      this->m->last_object_description,
		      this->m->file->getLastOffset(),
		      "object stream " +
		      QUtil::int_to_string(obj_stream_number) +
		      " has incorrect keys");
    }

    int n = dict.getKey("/N").getIntValue();
    int first = dict.getKey("/First").getIntValue();

    std::map<int, int> offsets;

    PointerHolder<Buffer> bp = obj_stream.getStreamData(qpdf_dl_specialized);
    PointerHolder<InputSource> input = new BufferInputSource(
        this->m->file->getName() +
        " object stream " + QUtil::int_to_string(obj_stream_number),
	bp.getPointer());

    for (int i = 0; i < n; ++i)
    {
	QPDFTokenizer::Token tnum = readToken(input);
	QPDFTokenizer::Token toffset = readToken(input);
	if (! ((tnum.getType() == QPDFTokenizer::tt_integer) &&
	       (toffset.getType() == QPDFTokenizer::tt_integer)))
	{
	    throw QPDFExc(qpdf_e_damaged_pdf, input->getName(),
			  this->m->last_object_description,
                          input->getLastOffset(),
			  "expected integer in object stream header");
	}

	int num = QUtil::string_to_int(tnum.getValue().c_str());
	int offset = QUtil::string_to_ll(toffset.getValue().c_str());
	offsets[num] = offset + first;
    }

    // To avoid having to read the object stream multiple times, store
    // all objects that would be found here in the cache.  Remember
    // that some objects stored here might have been overridden by new
    // objects appended to the file, so it is necessary to recheck the
    // xref table and only cache what would actually be resolved here.
    for (std::map<int, int>::iterator iter = offsets.begin();
	 iter != offsets.end(); ++iter)
    {
	int obj = (*iter).first;
	QPDFObjGen og(obj, 0);
        QPDFXRefEntry const& entry = this->m->xref_table[og];
        if ((entry.getType() == 2) &&
            (entry.getObjStreamNumber() == obj_stream_number))
        {
            int offset = (*iter).second;
            input->seek(offset, SEEK_SET);
            QPDFObjectHandle oh = readObject(input, "", obj, 0, true);
            this->m->obj_cache[og] =
                ObjCache(QPDFObjectHandle::ObjAccessor::getObject(oh),
                         end_before_space, end_after_space);
        }
        else
        {
            QTC::TC("qpdf", "QPDF not caching overridden objstm object");
        }
    }
}