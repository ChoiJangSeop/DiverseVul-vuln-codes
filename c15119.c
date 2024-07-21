QPDF::isLinearized()
{
    // If the first object in the file is a dictionary with a suitable
    // /Linearized key and has an /L key that accurately indicates the
    // file size, initialize this->m->lindict and return true.

    // A linearized PDF spec's first object will be contained within
    // the first 1024 bytes of the file and will be a dictionary with
    // a valid /Linearized key.  This routine looks for that and does
    // no additional validation.

    // The PDF spec says the linearization dictionary must be
    // completely contained within the first 1024 bytes of the file.
    // Add a byte for a null terminator.
    static int const tbuf_size = 1025;

    char* buf = new char[tbuf_size];
    this->m->file->seek(0, SEEK_SET);
    PointerHolder<char> b(true, buf);
    memset(buf, '\0', tbuf_size);
    this->m->file->read(buf, tbuf_size - 1);

    int lindict_obj = -1;
    char* p = buf;
    while (lindict_obj == -1)
    {
        // Find a digit or end of buffer
        while (((p - buf) < tbuf_size) && (! QUtil::is_digit(*p)))
        {
            ++p;
        }
        if (p - buf == tbuf_size)
        {
            break;
        }
        // Seek to the digit. Then skip over digits for a potential
        // next iteration.
        this->m->file->seek(p - buf, SEEK_SET);
        while (((p - buf) < tbuf_size) && QUtil::is_digit(*p))
        {
            ++p;
        }

        QPDFTokenizer::Token t1 = readToken(this->m->file);
        QPDFTokenizer::Token t2 = readToken(this->m->file);
        QPDFTokenizer::Token t3 = readToken(this->m->file);
        QPDFTokenizer::Token t4 = readToken(this->m->file);
        if ((t1.getType() == QPDFTokenizer::tt_integer) &&
            (t2.getType() == QPDFTokenizer::tt_integer) &&
            (t3 == QPDFTokenizer::Token(QPDFTokenizer::tt_word, "obj")) &&
            (t4.getType() == QPDFTokenizer::tt_dict_open))
        {
	    lindict_obj =
                static_cast<int>(QUtil::string_to_ll(t1.getValue().c_str()));
	}
    }

    if (lindict_obj <= 0)
    {
	return false;
    }

    QPDFObjectHandle candidate = QPDFObjectHandle::Factory::newIndirect(
	this, lindict_obj, 0);
    if (! candidate.isDictionary())
    {
	return false;
    }

    QPDFObjectHandle linkey = candidate.getKey("/Linearized");
    if (! (linkey.isNumber() &&
           (static_cast<int>(floor(linkey.getNumericValue())) == 1)))
    {
	return false;
    }

    QPDFObjectHandle L = candidate.getKey("/L");
    if (L.isInteger())
    {
	qpdf_offset_t Li = L.getIntValue();
	this->m->file->seek(0, SEEK_END);
	if (Li != this->m->file->tell())
	{
	    QTC::TC("qpdf", "QPDF /L mismatch");
	    return false;
	}
	else
	{
	    this->m->linp.file_size = Li;
	}
    }

    this->m->lindict = candidate;

    return true;
}