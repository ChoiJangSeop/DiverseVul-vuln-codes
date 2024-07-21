QPDF::readObjectAtOffset(bool try_recovery,
			 qpdf_offset_t offset, std::string const& description,
			 int exp_objid, int exp_generation,
			 int& objid, int& generation)
{
    setLastObjectDescription(description, exp_objid, exp_generation);

    // Special case: if offset is 0, just return null.  Some PDF
    // writers, in particular "Mac OS X 10.7.5 Quartz PDFContext", may
    // store deleted objects in the xref table as "0000000000 00000
    // n", which is not correct, but it won't hurt anything for to
    // ignore these.
    if (offset == 0)
    {
        QTC::TC("qpdf", "QPDF bogus 0 offset", 0);
	warn(QPDFExc(qpdf_e_damaged_pdf, this->file->getName(),
		     this->last_object_description, 0,
		     "object has offset 0"));
        return QPDFObjectHandle::newNull();
    }

    this->file->seek(offset, SEEK_SET);

    QPDFTokenizer::Token tobjid = readToken(this->file);
    QPDFTokenizer::Token tgen = readToken(this->file);
    QPDFTokenizer::Token tobj = readToken(this->file);

    bool objidok = (tobjid.getType() == QPDFTokenizer::tt_integer);
    int genok = (tgen.getType() == QPDFTokenizer::tt_integer);
    int objok = (tobj == QPDFTokenizer::Token(QPDFTokenizer::tt_word, "obj"));

    QTC::TC("qpdf", "QPDF check objid", objidok ? 1 : 0);
    QTC::TC("qpdf", "QPDF check generation", genok ? 1 : 0);
    QTC::TC("qpdf", "QPDF check obj", objok ? 1 : 0);

    try
    {
	if (! (objidok && genok && objok))
	{
	    QTC::TC("qpdf", "QPDF expected n n obj");
	    throw QPDFExc(qpdf_e_damaged_pdf, this->file->getName(),
			  this->last_object_description, offset,
			  "expected n n obj");
	}
	objid = atoi(tobjid.getValue().c_str());
	generation = atoi(tgen.getValue().c_str());

	if ((exp_objid >= 0) &&
	    (! ((objid == exp_objid) && (generation == exp_generation))))
	{
	    QTC::TC("qpdf", "QPDF err wrong objid/generation");
	    throw QPDFExc(qpdf_e_damaged_pdf, this->file->getName(),
			  this->last_object_description, offset,
			  std::string("expected ") +
			  QUtil::int_to_string(exp_objid) + " " +
			  QUtil::int_to_string(exp_generation) + " obj");
	}
    }
    catch (QPDFExc& e)
    {
	if ((exp_objid >= 0) && try_recovery && this->attempt_recovery)
	{
	    // Try again after reconstructing xref table
	    reconstruct_xref(e);
	    QPDFObjGen og(exp_objid, exp_generation);
	    if (this->xref_table.count(og) &&
		(this->xref_table[og].getType() == 1))
	    {
		qpdf_offset_t new_offset = this->xref_table[og].getOffset();
		QPDFObjectHandle result = readObjectAtOffset(
		    false, new_offset, description,
		    exp_objid, exp_generation, objid, generation);
		QTC::TC("qpdf", "QPDF recovered in readObjectAtOffset");
		return result;
	    }
	    else
	    {
		QTC::TC("qpdf", "QPDF object gone after xref reconstruction");
		warn(QPDFExc(
			 qpdf_e_damaged_pdf, this->file->getName(),
			 "", 0,
			 std::string(
			     "object " +
			     QUtil::int_to_string(exp_objid) +
			     " " +
			     QUtil::int_to_string(exp_generation) +
			     " not found in file after regenerating"
			     " cross reference table")));
		return QPDFObjectHandle::newNull();
	    }
	}
	else
	{
	    throw e;
	}
    }

    QPDFObjectHandle oh = readObject(
	this->file, description, objid, generation, false);

    if (! (readToken(this->file) ==
	   QPDFTokenizer::Token(QPDFTokenizer::tt_word, "endobj")))
    {
	QTC::TC("qpdf", "QPDF err expected endobj");
	warn(QPDFExc(qpdf_e_damaged_pdf, this->file->getName(),
		     this->last_object_description, this->file->getLastOffset(),
		     "expected endobj"));
    }

    QPDFObjGen og(objid, generation);
    if (! this->obj_cache.count(og))
    {
	// Store the object in the cache here so it gets cached
	// whether we first know the offset or whether we first know
	// the object ID and generation (in which we case we would get
	// here through resolve).

	// Determine the end offset of this object before and after
	// white space.  We use these numbers to validate
	// linearization hint tables.  Offsets and lengths of objects
	// may imply the end of an object to be anywhere between these
	// values.
	qpdf_offset_t end_before_space = this->file->tell();

	// skip over spaces
	while (true)
	{
	    char ch;
	    if (this->file->read(&ch, 1))
	    {
		if (! isspace(static_cast<unsigned char>(ch)))
		{
		    this->file->seek(-1, SEEK_CUR);
		    break;
		}
	    }
	    else
	    {
		throw QPDFExc(qpdf_e_damaged_pdf, this->file->getName(),
			      this->last_object_description, offset,
			      "EOF after endobj");
	    }
	}
	qpdf_offset_t end_after_space = this->file->tell();

	this->obj_cache[og] =
	    ObjCache(QPDFObjectHandle::ObjAccessor::getObject(oh),
		     end_before_space, end_after_space);
    }

    return oh;
}