QPDF::checkHPageOffset(std::list<std::string>& errors,
		       std::list<std::string>& warnings,
		       std::vector<QPDFObjectHandle> const& pages,
		       std::map<int, int>& shared_idx_to_obj)
{
    // Implementation note 126 says Acrobat always sets
    // delta_content_offset and delta_content_length in the page
    // offset header dictionary to 0.  It also states that
    // min_content_offset in the per-page information is always 0,
    // which is an incorrect value.

    // Implementation note 127 explains that Acrobat always sets item
    // 8 (min_content_length) to zero, item 9
    // (nbits_delta_content_length) to the value of item 5
    // (nbits_delta_page_length), and item 7 of each per-page hint
    // table (delta_content_length) to item 2 (delta_page_length) of
    // that entry.  Acrobat ignores these values when reading files.

    // Empirically, it also seems that Acrobat sometimes puts items
    // under a page's /Resources dictionary in with shared objects
    // even when they are private.

    unsigned int npages = pages.size();
    int table_offset = adjusted_offset(
	this->m->page_offset_hints.first_page_offset);
    QPDFObjGen first_page_og(pages.at(0).getObjGen());
    if (this->m->xref_table.count(first_page_og) == 0)
    {
        stopOnError("supposed first page object is not known");
    }
    qpdf_offset_t offset = getLinearizationOffset(first_page_og);
    if (table_offset != offset)
    {
	warnings.push_back("first page object offset mismatch");
    }

    for (unsigned int pageno = 0; pageno < npages; ++pageno)
    {
	QPDFObjGen page_og(pages.at(pageno).getObjGen());
	int first_object = page_og.getObj();
	if (this->m->xref_table.count(page_og) == 0)
        {
            stopOnError("unknown object in page offset hint table");
        }
	offset = getLinearizationOffset(page_og);

	HPageOffsetEntry& he = this->m->page_offset_hints.entries.at(pageno);
	CHPageOffsetEntry& ce = this->m->c_page_offset_data.entries.at(pageno);
	int h_nobjects = he.delta_nobjects +
	    this->m->page_offset_hints.min_nobjects;
	if (h_nobjects != ce.nobjects)
	{
	    // This happens with pdlin when there are thumbnails.
	    warnings.push_back(
		"object count mismatch for page " +
		QUtil::int_to_string(pageno) + ": hint table = " +
		QUtil::int_to_string(h_nobjects) + "; computed = " +
		QUtil::int_to_string(ce.nobjects));
	}

	// Use value for number of objects in hint table rather than
	// computed value if there is a discrepancy.
	int length = lengthNextN(first_object, h_nobjects, errors);
	int h_length = he.delta_page_length +
	    this->m->page_offset_hints.min_page_length;
	if (length != h_length)
	{
	    // This condition almost certainly indicates a bad hint
	    // table or a bug in this code.
	    errors.push_back(
		"page length mismatch for page " +
		QUtil::int_to_string(pageno) + ": hint table = " +
		QUtil::int_to_string(h_length) + "; computed length = " +
		QUtil::int_to_string(length) + " (offset = " +
		QUtil::int_to_string(offset) + ")");
	}

	offset += h_length;

	// Translate shared object indexes to object numbers.
	std::set<int> hint_shared;
	std::set<int> computed_shared;

	if ((pageno == 0) && (he.nshared_objects > 0))
	{
	    // pdlin and Acrobat both do this even though the spec
	    // states clearly and unambiguously that they should not.
	    warnings.push_back("page 0 has shared identifier entries");
	}

	for (int i = 0; i < he.nshared_objects; ++i)
	{
	    int idx = he.shared_identifiers.at(i);
	    if (shared_idx_to_obj.count(idx) == 0)
            {
                throw std::logic_error(
                    "unable to get object for item in"
                    " shared objects hint table");
            }
	    hint_shared.insert(shared_idx_to_obj[idx]);
	}

	for (int i = 0; i < ce.nshared_objects; ++i)
	{
	    int idx = ce.shared_identifiers.at(i);
	    if (idx >= this->m->c_shared_object_data.nshared_total)
            {
                throw std::logic_error(
                    "index out of bounds for shared object hint table");
            }
	    int obj = this->m->c_shared_object_data.entries.at(idx).object;
	    computed_shared.insert(obj);
	}

	for (std::set<int>::iterator iter = hint_shared.begin();
	     iter != hint_shared.end(); ++iter)
	{
	    if (! computed_shared.count(*iter))
	    {
		// pdlin puts thumbnails here even though it shouldn't
		warnings.push_back(
		    "page " + QUtil::int_to_string(pageno) +
		    ": shared object " + QUtil::int_to_string(*iter) +
		    ": in hint table but not computed list");
	    }
	}

	for (std::set<int>::iterator iter = computed_shared.begin();
	     iter != computed_shared.end(); ++iter)
	{
	    if (! hint_shared.count(*iter))
	    {
		// Acrobat does not put some things including at least
		// built-in fonts and procsets here, at least in some
		// cases.
		warnings.push_back(
		    "page " + QUtil::int_to_string(pageno) +
		    ": shared object " + QUtil::int_to_string(*iter) +
		    ": in computed list but not hint table");
	    }
	}
    }
}