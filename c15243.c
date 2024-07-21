QPDF::read_xrefTable(qpdf_offset_t xref_offset)
{
    std::vector<QPDFObjGen> deleted_items;

    this->m->file->seek(xref_offset, SEEK_SET);
    bool done = false;
    while (! done)
    {
        char linebuf[51];
        memset(linebuf, 0, sizeof(linebuf));
        this->m->file->read(linebuf, sizeof(linebuf) - 1);
	std::string line = linebuf;
        int obj = 0;
        int num = 0;
        int bytes = 0;
        if (! parse_xrefFirst(line, obj, num, bytes))
	{
	    QTC::TC("qpdf", "QPDF invalid xref");
	    throw QPDFExc(qpdf_e_damaged_pdf, this->m->file->getName(),
			  "xref table", this->m->file->getLastOffset(),
			  "xref syntax invalid");
	}
        this->m->file->seek(this->m->file->getLastOffset() + bytes, SEEK_SET);
	for (qpdf_offset_t i = obj; i - num < obj; ++i)
	{
	    if (i == 0)
	    {
		// This is needed by checkLinearization()
		this->m->first_xref_item_offset = this->m->file->tell();
	    }
	    std::string xref_entry = this->m->file->readLine(30);
            // For xref_table, these will always be small enough to be ints
	    qpdf_offset_t f1 = 0;
	    int f2 = 0;
	    char type = '\0';
            if (! parse_xrefEntry(xref_entry, f1, f2, type))
	    {
		QTC::TC("qpdf", "QPDF invalid xref entry");
		throw QPDFExc(
		    qpdf_e_damaged_pdf, this->m->file->getName(),
		    "xref table", this->m->file->getLastOffset(),
		    "invalid xref entry (obj=" +
		    QUtil::int_to_string(i) + ")");
	    }
	    if (type == 'f')
	    {
		// Save deleted items until after we've checked the
		// XRefStm, if any.
		deleted_items.push_back(QPDFObjGen(i, f2));
	    }
	    else
	    {
		insertXrefEntry(i, 1, f1, f2);
	    }
	}
	qpdf_offset_t pos = this->m->file->tell();
	QPDFTokenizer::Token t = readToken(this->m->file);
	if (t == QPDFTokenizer::Token(QPDFTokenizer::tt_word, "trailer"))
	{
	    done = true;
	}
	else
	{
	    this->m->file->seek(pos, SEEK_SET);
	}
    }

    // Set offset to previous xref table if any
    QPDFObjectHandle cur_trailer =
	readObject(this->m->file, "trailer", 0, 0, false);
    if (! cur_trailer.isDictionary())
    {
	QTC::TC("qpdf", "QPDF missing trailer");
	throw QPDFExc(qpdf_e_damaged_pdf, this->m->file->getName(),
		      "", this->m->file->getLastOffset(),
		      "expected trailer dictionary");
    }

    if (! this->m->trailer.isInitialized())
    {
	setTrailer(cur_trailer);

	if (! this->m->trailer.hasKey("/Size"))
	{
	    QTC::TC("qpdf", "QPDF trailer lacks size");
	    throw QPDFExc(qpdf_e_damaged_pdf, this->m->file->getName(),
			  "trailer", this->m->file->getLastOffset(),
			  "trailer dictionary lacks /Size key");
	}
	if (! this->m->trailer.getKey("/Size").isInteger())
	{
	    QTC::TC("qpdf", "QPDF trailer size not integer");
	    throw QPDFExc(qpdf_e_damaged_pdf, this->m->file->getName(),
			  "trailer", this->m->file->getLastOffset(),
			  "/Size key in trailer dictionary is not "
			  "an integer");
	}
    }

    if (cur_trailer.hasKey("/XRefStm"))
    {
	if (this->m->ignore_xref_streams)
	{
	    QTC::TC("qpdf", "QPDF ignoring XRefStm in trailer");
	}
	else
	{
	    if (cur_trailer.getKey("/XRefStm").isInteger())
	    {
		// Read the xref stream but disregard any return value
		// -- we'll use our trailer's /Prev key instead of the
		// xref stream's.
		(void) read_xrefStream(
		    cur_trailer.getKey("/XRefStm").getIntValue());
	    }
	    else
	    {
		throw QPDFExc(qpdf_e_damaged_pdf, this->m->file->getName(),
			      "xref stream", xref_offset,
			      "invalid /XRefStm");
	    }
	}
    }

    // Handle any deleted items now that we've read the /XRefStm.
    for (std::vector<QPDFObjGen>::iterator iter = deleted_items.begin();
	 iter != deleted_items.end(); ++iter)
    {
	QPDFObjGen& og = *iter;
	insertXrefEntry(og.getObj(), 0, 0, og.getGen());
    }

    if (cur_trailer.hasKey("/Prev"))
    {
	if (! cur_trailer.getKey("/Prev").isInteger())
	{
	    QTC::TC("qpdf", "QPDF trailer prev not integer");
	    throw QPDFExc(qpdf_e_damaged_pdf, this->m->file->getName(),
			  "trailer", this->m->file->getLastOffset(),
			  "/Prev key in trailer dictionary is not "
			  "an integer");
	}
	QTC::TC("qpdf", "QPDF prev key in trailer dictionary");
	xref_offset = cur_trailer.getKey("/Prev").getIntValue();
    }
    else
    {
	xref_offset = 0;
    }

    return xref_offset;
}