QPDF::checkLinearizationInternal()
{
    // All comments referring to the PDF spec refer to the spec for
    // version 1.4.

    std::list<std::string> errors;
    std::list<std::string> warnings;

    // Check all values in linearization parameter dictionary

    LinParameters& p = this->m->linp;

    // L: file size in bytes -- checked by isLinearized

    // O: object number of first page
    std::vector<QPDFObjectHandle> const& pages = getAllPages();
    if (p.first_page_object != pages.at(0).getObjectID())
    {
	QTC::TC("qpdf", "QPDF err /O mismatch");
	errors.push_back("first page object (/O) mismatch");
    }

    // N: number of pages
    int npages = pages.size();
    if (p.npages != npages)
    {
	// Not tested in the test suite
	errors.push_back("page count (/N) mismatch");
    }

    for (int i = 0; i < npages; ++i)
    {
	QPDFObjectHandle const& page = pages.at(i);
	QPDFObjGen og(page.getObjGen());
	if (this->m->xref_table[og].getType() == 2)
	{
	    errors.push_back("page dictionary for page " +
			     QUtil::int_to_string(i) + " is compressed");
	}
    }

    // T: offset of whitespace character preceding xref entry for object 0
    this->m->file->seek(p.xref_zero_offset, SEEK_SET);
    while (1)
    {
	char ch;
	this->m->file->read(&ch, 1);
	if (! ((ch == ' ') || (ch == '\r') || (ch == '\n')))
	{
	    this->m->file->seek(-1, SEEK_CUR);
	    break;
	}
    }
    if (this->m->file->tell() != this->m->first_xref_item_offset)
    {
	QTC::TC("qpdf", "QPDF err /T mismatch");
	errors.push_back("space before first xref item (/T) mismatch "
			 "(computed = " +
			 QUtil::int_to_string(this->m->first_xref_item_offset) +
			 "; file = " +
			 QUtil::int_to_string(this->m->file->tell()));
    }

    // P: first page number -- Implementation note 124 says Acrobat
    // ignores this value, so we will too.

    // Check numbering of compressed objects in each xref section.
    // For linearized files, all compressed objects are supposed to be
    // at the end of the containing xref section if any object streams
    // are in use.

    if (this->m->uncompressed_after_compressed)
    {
	errors.push_back("linearized file contains an uncompressed object"
			 " after a compressed one in a cross-reference stream");
    }

    // Further checking requires optimization and order calculation.
    // Don't allow optimization to make changes.  If it has to, then
    // the file is not properly linearized.  We use the xref table to
    // figure out which objects are compressed and which are
    // uncompressed.
    { // local scope
	std::map<int, int> object_stream_data;
	for (std::map<QPDFObjGen, QPDFXRefEntry>::const_iterator iter =
		 this->m->xref_table.begin();
	     iter != this->m->xref_table.end(); ++iter)
	{
	    QPDFObjGen const& og = (*iter).first;
	    QPDFXRefEntry const& entry = (*iter).second;
	    if (entry.getType() == 2)
	    {
		object_stream_data[og.getObj()] = entry.getObjStreamNumber();
	    }
	}
	optimize(object_stream_data, false);
	calculateLinearizationData(object_stream_data);
    }

    // E: offset of end of first page -- Implementation note 123 says
    // Acrobat includes on extra object here by mistake.  pdlin fails
    // to place thumbnail images in section 9, so when thumbnails are
    // present, it also gets the wrong value for /E.  It also doesn't
    // count outlines here when it should even though it places them
    // in part 6.  This code fails to put thread information
    // dictionaries in part 9, so it actually gets the wrong value for
    // E when threads are present.  In that case, it would probably
    // agree with pdlin.  As of this writing, the test suite doesn't
    // contain any files with threads.

    if (this->m->part6.empty())
    {
        throw std::logic_error("linearization part 6 unexpectedly empty");
    }
    qpdf_offset_t min_E = -1;
    qpdf_offset_t max_E = -1;
    for (std::vector<QPDFObjectHandle>::iterator iter = this->m->part6.begin();
	 iter != this->m->part6.end(); ++iter)
    {
	QPDFObjGen og((*iter).getObjGen());
	if (this->m->obj_cache.count(og) == 0)
        {
            // All objects have to have been dereferenced to be classified.
            throw std::logic_error("linearization part6 object not in cache");
        }
	ObjCache const& oc = this->m->obj_cache[og];
	min_E = std::max(min_E, oc.end_before_space);
	max_E = std::max(max_E, oc.end_after_space);
    }
    if ((p.first_page_end < min_E) || (p.first_page_end > max_E))
    {
	QTC::TC("qpdf", "QPDF warn /E mismatch");
	warnings.push_back("end of first page section (/E) mismatch: /E = " +
			   QUtil::int_to_string(p.first_page_end) +
			   "; computed = " +
			   QUtil::int_to_string(min_E) + ".." +
			   QUtil::int_to_string(max_E));
    }

    // Check hint tables

    std::map<int, int> shared_idx_to_obj;
    checkHSharedObject(errors, warnings, pages, shared_idx_to_obj);
    checkHPageOffset(errors, warnings, pages, shared_idx_to_obj);
    checkHOutlines(warnings);

    // Report errors

    bool result = true;

    if (! errors.empty())
    {
	result = false;
	for (std::list<std::string>::iterator iter = errors.begin();
	     iter != errors.end(); ++iter)
	{
	    *this->m->out_stream << "ERROR: " << (*iter) << std::endl;
	}
    }

    if (! warnings.empty())
    {
	result = false;
	for (std::list<std::string>::iterator iter = warnings.begin();
	     iter != warnings.end(); ++iter)
	{
	    *this->m->out_stream << "WARNING: " << (*iter) << std::endl;
	}
    }

    return result;
}