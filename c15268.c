QPDFWriter::writeTrailer(trailer_e which, int size, bool xref_stream,
                         qpdf_offset_t prev, int linearization_pass)
{
    QPDFObjectHandle trailer = getTrimmedTrailer();
    if (! xref_stream)
    {
	writeString("trailer <<");
    }
    writeStringQDF("\n");
    if (which == t_lin_second)
    {
	writeString(" /Size ");
	writeString(QUtil::int_to_string(size));
    }
    else
    {
	std::set<std::string> keys = trailer.getKeys();
	for (std::set<std::string>::iterator iter = keys.begin();
	     iter != keys.end(); ++iter)
	{
	    std::string const& key = *iter;
	    writeStringQDF("  ");
	    writeStringNoQDF(" ");
	    writeString(QPDF_Name::normalizeName(key));
	    writeString(" ");
	    if (key == "/Size")
	    {
		writeString(QUtil::int_to_string(size));
		if (which == t_lin_first)
		{
		    writeString(" /Prev ");
		    qpdf_offset_t pos = this->m->pipeline->getCount();
		    writeString(QUtil::int_to_string(prev));
		    int nspaces = pos - this->m->pipeline->getCount() + 21;
		    if (nspaces < 0)
                    {
                        throw std::logic_error(
                            "QPDFWriter: no padding required in trailer");
                    }
		    writePad(nspaces);
		}
	    }
	    else
	    {
		unparseChild(trailer.getKey(key), 1, 0);
	    }
	    writeStringQDF("\n");
	}
    }

    // Write ID
    writeStringQDF(" ");
    writeString(" /ID [");
    if (linearization_pass == 1)
    {
	std::string original_id1 = getOriginalID1();
        if (original_id1.empty())
        {
            writeString("<00000000000000000000000000000000>");
        }
        else
        {
            // Write a string of zeroes equal in length to the
            // representation of the original ID. While writing the
            // original ID would have the same number of bytes, it
            // would cause a change to the deterministic ID generated
            // by older versions of the software that hard-coded the
            // length of the ID to 16 bytes.
            writeString("<");
            size_t len = QPDF_String(original_id1).unparse(true).length() - 2;
            for (size_t i = 0; i < len; ++i)
            {
                writeString("0");
            }
            writeString(">");
        }
        writeString("<00000000000000000000000000000000>");
    }
    else
    {
        if ((linearization_pass == 0) && (this->m->deterministic_id))
        {
            computeDeterministicIDData();
        }
        generateID();
        writeString(QPDF_String(this->m->id1).unparse(true));
        writeString(QPDF_String(this->m->id2).unparse(true));
    }
    writeString("]");

    if (which != t_lin_second)
    {
	// Write reference to encryption dictionary
	if (this->m->encrypted)
	{
	    writeString(" /Encrypt ");
	    writeString(QUtil::int_to_string(this->m->encryption_dict_objid));
	    writeString(" 0 R");
	}
    }

    writeStringQDF("\n");
    writeStringNoQDF(" ");
    writeString(">>");
}