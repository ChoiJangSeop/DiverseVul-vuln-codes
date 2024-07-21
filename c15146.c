QPDF::processXRefStream(qpdf_offset_t xref_offset, QPDFObjectHandle& xref_obj)
{
    QPDFObjectHandle dict = xref_obj.getDict();
    QPDFObjectHandle W_obj = dict.getKey("/W");
    QPDFObjectHandle Index_obj = dict.getKey("/Index");
    if (! (W_obj.isArray() &&
	   (W_obj.getArrayNItems() >= 3) &&
	   W_obj.getArrayItem(0).isInteger() &&
	   W_obj.getArrayItem(1).isInteger() &&
	   W_obj.getArrayItem(2).isInteger() &&
	   dict.getKey("/Size").isInteger() &&
	   (Index_obj.isArray() || Index_obj.isNull())))
    {
	throw QPDFExc(qpdf_e_damaged_pdf, this->m->file->getName(),
		      "xref stream", xref_offset,
		      "Cross-reference stream does not have"
		      " proper /W and /Index keys");
    }

    int W[3];
    size_t entry_size = 0;
    int max_bytes = sizeof(qpdf_offset_t);
    for (int i = 0; i < 3; ++i)
    {
	W[i] = W_obj.getArrayItem(i).getIntValue();
        if (W[i] > max_bytes)
        {
            throw QPDFExc(qpdf_e_damaged_pdf, this->m->file->getName(),
                          "xref stream", xref_offset,
                          "Cross-reference stream's /W contains"
                          " impossibly large values");
        }
	entry_size += W[i];
    }
    if (entry_size == 0)
    {
        throw QPDFExc(qpdf_e_damaged_pdf, this->m->file->getName(),
                      "xref stream", xref_offset,
                      "Cross-reference stream's /W indicates"
                      " entry size of 0");
    }
    long long max_num_entries =
        static_cast<unsigned long long>(-1) / entry_size;

    std::vector<long long> indx;
    if (Index_obj.isArray())
    {
	int n_index = Index_obj.getArrayNItems();
	if ((n_index % 2) || (n_index < 2))
	{
	    throw QPDFExc(qpdf_e_damaged_pdf, this->m->file->getName(),
			  "xref stream", xref_offset,
			  "Cross-reference stream's /Index has an"
			  " invalid number of values");
	}
	for (int i = 0; i < n_index; ++i)
	{
	    if (Index_obj.getArrayItem(i).isInteger())
	    {
		indx.push_back(Index_obj.getArrayItem(i).getIntValue());
	    }
	    else
	    {
		throw QPDFExc(qpdf_e_damaged_pdf, this->m->file->getName(),
			      "xref stream", xref_offset,
			      "Cross-reference stream's /Index's item " +
			      QUtil::int_to_string(i) +
			      " is not an integer");
	    }
	}
	QTC::TC("qpdf", "QPDF xref /Index is array",
		n_index == 2 ? 0 : 1);
    }
    else
    {
	QTC::TC("qpdf", "QPDF xref /Index is null");
	long long size = dict.getKey("/Size").getIntValue();
	indx.push_back(0);
	indx.push_back(size);
    }

    long long num_entries = 0;
    for (unsigned int i = 1; i < indx.size(); i += 2)
    {
        if (indx.at(i) > max_num_entries - num_entries)
        {
            throw QPDFExc(qpdf_e_damaged_pdf, this->m->file->getName(),
                          "xref stream", xref_offset,
                          "Cross-reference stream claims to contain"
                          " too many entries: " +
                          QUtil::int_to_string(indx.at(i)) + " " +
                          QUtil::int_to_string(max_num_entries) + " " +
                          QUtil::int_to_string(num_entries));
        }
	num_entries += indx.at(i);
    }

    // entry_size and num_entries have both been validated to ensure
    // that this multiplication does not cause an overflow.
    size_t expected_size = entry_size * num_entries;

    PointerHolder<Buffer> bp = xref_obj.getStreamData(qpdf_dl_specialized);
    size_t actual_size = bp->getSize();

    if (expected_size != actual_size)
    {
	QPDFExc x(qpdf_e_damaged_pdf, this->m->file->getName(),
		  "xref stream", xref_offset,
		  "Cross-reference stream data has the wrong size;"
		  " expected = " + QUtil::int_to_string(expected_size) +
		  "; actual = " + QUtil::int_to_string(actual_size));
	if (expected_size > actual_size)
	{
	    throw x;
	}
	else
	{
	    warn(x);
	}
    }

    int cur_chunk = 0;
    int chunk_count = 0;

    bool saw_first_compressed_object = false;

    // Actual size vs. expected size check above ensures that we will
    // not overflow any buffers here.  We know that entry_size *
    // num_entries is equal to the size of the buffer.
    unsigned char const* data = bp->getBuffer();
    for (int i = 0; i < num_entries; ++i)
    {
	// Read this entry
	unsigned char const* entry = data + (entry_size * i);
	qpdf_offset_t fields[3];
	unsigned char const* p = entry;
	for (int j = 0; j < 3; ++j)
	{
	    fields[j] = 0;
	    if ((j == 0) && (W[0] == 0))
	    {
		QTC::TC("qpdf", "QPDF default for xref stream field 0");
		fields[0] = 1;
	    }
	    for (int k = 0; k < W[j]; ++k)
	    {
		fields[j] <<= 8;
		fields[j] += static_cast<int>(*p++);
	    }
	}

	// Get the object and generation number.  The object number is
	// based on /Index.  The generation number is 0 unless this is
	// an uncompressed object record, in which case the generation
	// number appears as the third field.
	int obj = indx.at(cur_chunk) + chunk_count;
	++chunk_count;
	if (chunk_count >= indx.at(cur_chunk + 1))
	{
	    cur_chunk += 2;
	    chunk_count = 0;
	}

	if (saw_first_compressed_object)
	{
	    if (fields[0] != 2)
	    {
		this->m->uncompressed_after_compressed = true;
	    }
	}
	else if (fields[0] == 2)
	{
	    saw_first_compressed_object = true;
	}
	if (obj == 0)
	{
	    // This is needed by checkLinearization()
	    this->m->first_xref_item_offset = xref_offset;
	}
	insertXrefEntry(obj, static_cast<int>(fields[0]),
                        fields[1], static_cast<int>(fields[2]));
    }

    if (! this->m->trailer.isInitialized())
    {
	setTrailer(dict);
    }

    if (dict.hasKey("/Prev"))
    {
	if (! dict.getKey("/Prev").isInteger())
	{
	    throw QPDFExc(qpdf_e_damaged_pdf, this->m->file->getName(),
			  "xref stream", this->m->file->getLastOffset(),
			  "/Prev key in xref stream dictionary is not "
			  "an integer");
	}
	QTC::TC("qpdf", "QPDF prev key in xref stream dictionary");
	xref_offset = dict.getKey("/Prev").getIntValue();
    }
    else
    {
	xref_offset = 0;
    }

    return xref_offset;
}