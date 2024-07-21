QPDF::checkHSharedObject(std::list<std::string>& errors,
			 std::list<std::string>& warnings,
			 std::vector<QPDFObjectHandle> const& pages,
			 std::map<int, int>& idx_to_obj)
{
    // Implementation note 125 says shared object groups always
    // contain only one object.  Implementation note 128 says that
    // Acrobat always nbits_nobjects to zero.  Implementation note 130
    // says that Acrobat does not support more than one shared object
    // per group.  These are all consistent.

    // Implementation note 129 states that MD5 signatures are not
    // implemented in Acrobat, so signature_present must always be
    // zero.

    // Implementation note 131 states that first_shared_obj and
    // first_shared_offset have meaningless values for single-page
    // files.

    // Empirically, Acrobat and pdlin generate incorrect values for
    // these whenever there are no shared objects not referenced by
    // the first page (i.e., nshared_total == nshared_first_page).

    HSharedObject& so = this->m->shared_object_hints;
    if (so.nshared_total < so.nshared_first_page)
    {
	errors.push_back("shared object hint table: ntotal < nfirst_page");
    }
    else
    {
	// The first nshared_first_page objects are consecutive
	// objects starting with the first page object.  The rest are
	// consecutive starting from the first_shared_obj object.
	int cur_object = pages.at(0).getObjectID();
	for (int i = 0; i < so.nshared_total; ++i)
	{
	    if (i == so.nshared_first_page)
	    {
		QTC::TC("qpdf", "QPDF lin check shared past first page");
		if (this->m->part8.empty())
		{
		    errors.push_back(
			"part 8 is empty but nshared_total > "
			"nshared_first_page");
		}
		else
		{
		    int obj = this->m->part8.at(0).getObjectID();
		    if (obj != so.first_shared_obj)
		    {
			errors.push_back(
			    "first shared object number mismatch: "
			    "hint table = " +
			    QUtil::int_to_string(so.first_shared_obj) +
			    "; computed = " +
			    QUtil::int_to_string(obj));
		    }
		}

		cur_object = so.first_shared_obj;

		QPDFObjGen og(cur_object, 0);
		if (this->m->xref_table.count(og) == 0)
                {
                    stopOnError("unknown object in shared object hint table");
                }
		int offset = getLinearizationOffset(og);
		int h_offset = adjusted_offset(so.first_shared_offset);
		if (offset != h_offset)
		{
		    errors.push_back(
			"first shared object offset mismatch: hint table = " +
			QUtil::int_to_string(h_offset) + "; computed = " +
			QUtil::int_to_string(offset));
		}
	    }

	    idx_to_obj[i] = cur_object;
	    HSharedObjectEntry& se = so.entries.at(i);
	    int nobjects = se.nobjects_minus_one + 1;
	    int length = lengthNextN(cur_object, nobjects, errors);
	    int h_length = so.min_group_length + se.delta_group_length;
	    if (length != h_length)
	    {
		errors.push_back(
		    "shared object " + QUtil::int_to_string(i) +
		    " length mismatch: hint table = " +
		    QUtil::int_to_string(h_length) + "; computed = " +
		    QUtil::int_to_string(length));
	    }
	    cur_object += nobjects;
	}
    }
}