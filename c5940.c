QPDF::reconstruct_xref(QPDFExc& e)
{
    if (this->m->reconstructed_xref)
    {
        // Avoid xref reconstruction infinite loops. This is getting
        // very hard to reproduce because qpdf is throwing many fewer
        // exceptions while parsing. Most situations are warnings now.
        throw e;
    }

    this->m->reconstructed_xref = true;

    warn(QPDFExc(qpdf_e_damaged_pdf, this->m->file->getName(), "", 0,
		 "file is damaged"));
    warn(e);
    warn(QPDFExc(qpdf_e_damaged_pdf, this->m->file->getName(), "", 0,
		 "Attempting to reconstruct cross-reference table"));

    // Delete all references to type 1 (uncompressed) objects
    std::set<QPDFObjGen> to_delete;
    for (std::map<QPDFObjGen, QPDFXRefEntry>::iterator iter =
	     this->m->xref_table.begin();
	 iter != this->m->xref_table.end(); ++iter)
    {
	if (((*iter).second).getType() == 1)
	{
	    to_delete.insert((*iter).first);
	}
    }
    for (std::set<QPDFObjGen>::iterator iter = to_delete.begin();
	 iter != to_delete.end(); ++iter)
    {
	this->m->xref_table.erase(*iter);
    }

    this->m->file->seek(0, SEEK_END);
    qpdf_offset_t eof = this->m->file->tell();
    this->m->file->seek(0, SEEK_SET);
    bool in_obj = false;
    qpdf_offset_t line_start = 0;
    // Don't allow very long tokens here during recovery.
    static size_t const MAX_LEN = 100;
    while (this->m->file->tell() < eof)
    {
        this->m->file->findAndSkipNextEOL();
        qpdf_offset_t next_line_start = this->m->file->tell();
        this->m->file->seek(line_start, SEEK_SET);
        QPDFTokenizer::Token t1 = readToken(this->m->file, true, MAX_LEN);
        qpdf_offset_t token_start =
            this->m->file->tell() - t1.getValue().length();
        if (token_start >= next_line_start)
        {
            // don't process yet
        }
	else if (in_obj)
	{
            if (t1 == QPDFTokenizer::Token(QPDFTokenizer::tt_word, "endobj"))
	    {
		in_obj = false;
	    }
	}
        else
        {
            if (t1.getType() == QPDFTokenizer::tt_integer)
            {
                QPDFTokenizer::Token t2 =
                    readToken(this->m->file, true, MAX_LEN);
                QPDFTokenizer::Token t3 =
                    readToken(this->m->file, true, MAX_LEN);
                if ((t2.getType() == QPDFTokenizer::tt_integer) &&
                    (t3 == QPDFTokenizer::Token(QPDFTokenizer::tt_word, "obj")))
                {
                    in_obj = true;
                    int obj = atoi(t1.getValue().c_str());
                    int gen = atoi(t2.getValue().c_str());
                    insertXrefEntry(obj, 1, token_start, gen, true);
                }
            }
            else if ((! this->m->trailer.isInitialized()) &&
                     (t1 == QPDFTokenizer::Token(
                         QPDFTokenizer::tt_word, "trailer")))
            {
                QPDFObjectHandle t =
                    readObject(this->m->file, "trailer", 0, 0, false);
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
        this->m->file->seek(next_line_start, SEEK_SET);
        line_start = next_line_start;
    }

    if (! this->m->trailer.isInitialized())
    {
	// We could check the last encountered object to see if it was
	// an xref stream.  If so, we could try to get the trailer
	// from there.  This may make it possible to recover files
	// with bad startxref pointers even when they have object
	// streams.

	throw QPDFExc(qpdf_e_damaged_pdf, this->m->file->getName(), "", 0,
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