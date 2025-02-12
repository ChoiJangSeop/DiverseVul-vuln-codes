QPDF::reconstruct_xref(QPDFExc& e)
{
    PCRE obj_re("^\\s*(\\d+)\\s+(\\d+)\\s+obj\\b");
    PCRE endobj_re("^\\s*endobj\\b");
    PCRE trailer_re("^\\s*trailer\\b");

    warn(QPDFExc(qpdf_e_damaged_pdf, this->file->getName(), "", 0,
		 "file is damaged"));
    warn(e);
    warn(QPDFExc(qpdf_e_damaged_pdf, this->file->getName(), "", 0,
		 "Attempting to reconstruct cross-reference table"));

    // Delete all references to type 1 (uncompressed) objects
    std::set<QPDFObjGen> to_delete;
    for (std::map<QPDFObjGen, QPDFXRefEntry>::iterator iter =
	     this->xref_table.begin();
	 iter != this->xref_table.end(); ++iter)
    {
	if (((*iter).second).getType() == 1)
	{
	    to_delete.insert((*iter).first);
	}
    }
    for (std::set<QPDFObjGen>::iterator iter = to_delete.begin();
	 iter != to_delete.end(); ++iter)
    {
	this->xref_table.erase(*iter);
    }

    this->file->seek(0, SEEK_END);
    qpdf_offset_t eof = this->file->tell();
    this->file->seek(0, SEEK_SET);
    bool in_obj = false;
    while (this->file->tell() < eof)
    {
	std::string line = this->file->readLine(50);
	if (in_obj)
	{
	    if (endobj_re.match(line.c_str()))
	    {
		in_obj = false;
	    }
	}
	else
	{
	    PCRE::Match m = obj_re.match(line.c_str());
	    if (m)
	    {
		in_obj = true;
		int obj = atoi(m.getMatch(1).c_str());
		int gen = atoi(m.getMatch(2).c_str());
		qpdf_offset_t offset = this->file->getLastOffset();
		insertXrefEntry(obj, 1, offset, gen, true);
	    }
	    else if ((! this->trailer.isInitialized()) &&
		     trailer_re.match(line.c_str()))
	    {
		// read "trailer"
		this->file->seek(this->file->getLastOffset(), SEEK_SET);
		readToken(this->file);
		QPDFObjectHandle t =
		    readObject(this->file, "trailer", 0, 0, false);
		if (! t.isDictionary())
		{
		    // Oh well.  It was worth a try.
		}
		else
		{
		    setTrailer(t);
		}
	    }
	}
    }

    if (! this->trailer.isInitialized())
    {
	// We could check the last encountered object to see if it was
	// an xref stream.  If so, we could try to get the trailer
	// from there.  This may make it possible to recover files
	// with bad startxref pointers even when they have object
	// streams.

	throw QPDFExc(qpdf_e_damaged_pdf, this->file->getName(), "", 0,
		      "unable to find trailer "
		      "dictionary while recovering damaged file");
    }

    // We could iterate through the objects looking for streams and
    // try to find objects inside of them, but it's probably not worth
    // the trouble.  Acrobat can't recover files with any errors in an
    // xref stream, and this would be a real long shot anyway.  If we
    // wanted to do anything that involved looking at stream contents,
    // we'd also have to call initializeEncryption() here.  It's safe
    // to call it more than once.
}