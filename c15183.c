QPDF::recoverStreamLength(PointerHolder<InputSource> input,
			  int objid, int generation,
                          qpdf_offset_t stream_offset)
{
    // Try to reconstruct stream length by looking for
    // endstream or endobj
    warn(QPDFExc(qpdf_e_damaged_pdf, input->getName(),
		 this->m->last_object_description, stream_offset,
		 "attempting to recover stream length"));

    PatternFinder ef(*this, &QPDF::findEndstream);
    size_t length = 0;
    if (this->m->file->findFirst("end", stream_offset, 0, ef))
    {
        length = this->m->file->tell() - stream_offset;
        // Reread endstream but, if it was endobj, don't skip that.
        QPDFTokenizer::Token t = readToken(this->m->file);
        if (t.getValue() == "endobj")
        {
            this->m->file->seek(this->m->file->getLastOffset(), SEEK_SET);
        }
    }

    if (length)
    {
	int this_obj_offset = 0;
	QPDFObjGen this_obj(0, 0);

	// Make sure this is inside this object
	for (std::map<QPDFObjGen, QPDFXRefEntry>::iterator iter =
		 this->m->xref_table.begin();
	     iter != this->m->xref_table.end(); ++iter)
	{
	    QPDFObjGen const& og = (*iter).first;
	    QPDFXRefEntry const& entry = (*iter).second;
	    if (entry.getType() == 1)
	    {
		qpdf_offset_t obj_offset = entry.getOffset();
		if ((obj_offset > stream_offset) &&
		    ((this_obj_offset == 0) ||
		     (this_obj_offset > obj_offset)))
		{
		    this_obj_offset = obj_offset;
		    this_obj = og;
		}
	    }
	}
	if (this_obj_offset &&
	    (this_obj.getObj() == objid) &&
	    (this_obj.getGen() == generation))
	{
	    // Well, we found endstream\nendobj within the space
	    // allowed for this object, so we're probably in good
	    // shape.
	}
	else
	{
	    QTC::TC("qpdf", "QPDF found wrong endstream in recovery");
	}
    }

    if (length == 0)
    {
        warn(QPDFExc(qpdf_e_damaged_pdf, input->getName(),
                     this->m->last_object_description, stream_offset,
                     "unable to recover stream data;"
                     " treating stream as empty"));
    }
    else
    {
        warn(QPDFExc(qpdf_e_damaged_pdf, input->getName(),
                     this->m->last_object_description, stream_offset,
                     "recovered stream length: " +
                     QUtil::int_to_string(length)));
    }

    QTC::TC("qpdf", "QPDF recovered stream length");
    return length;
}