QPDF::calculateLinearizationData(std::map<int, int> const& object_stream_data)
{
    // This function calculates the ordering of objects, divides them
    // into the appropriate parts, and computes some values for the
    // linearization parameter dictionary and hint tables.  The file
    // must be optimized (via calling optimize()) prior to calling
    // this function.  Note that actual offsets and lengths are not
    // computed here, but anything related to object ordering is.

    if (this->m->object_to_obj_users.empty())
    {
	// Note that we can't call optimize here because we don't know
	// whether it should be called with or without allow changes.
	throw std::logic_error(
	    "INTERNAL ERROR: QPDF::calculateLinearizationData "
	    "called before optimize()");
    }

    // Separate objects into the categories sufficient for us to
    // determine which part of the linearized file should contain the
    // object.  This categorization is useful for other purposes as
    // well.  Part numbers refer to version 1.4 of the PDF spec.

    // Parts 1, 3, 5, 10, and 11 don't contain any objects from the
    // original file (except the trailer dictionary in part 11).

    // Part 4 is the document catalog (root) and the following root
    // keys: /ViewerPreferences, /PageMode, /Threads, /OpenAction,
    // /AcroForm, /Encrypt.  Note that Thread information dictionaries
    // are supposed to appear in part 9, but we are disregarding that
    // recommendation for now.

    // Part 6 is the first page section.  It includes all remaining
    // objects referenced by the first page including shared objects
    // but not including thumbnails.  Additionally, if /PageMode is
    // /Outlines, then information from /Outlines also appears here.

    // Part 7 contains remaining objects private to pages other than
    // the first page.

    // Part 8 contains all remaining shared objects except those that
    // are shared only within thumbnails.

    // Part 9 contains all remaining objects.

    // We sort objects into the following categories:

    //   * open_document: part 4

    //   * first_page_private: part 6

    //   * first_page_shared: part 6

    //   * other_page_private: part 7

    //   * other_page_shared: part 8

    //   * thumbnail_private: part 9

    //   * thumbnail_shared: part 9

    //   * other: part 9

    //   * outlines: part 6 or 9

    this->m->part4.clear();
    this->m->part6.clear();
    this->m->part7.clear();
    this->m->part8.clear();
    this->m->part9.clear();
    this->m->c_linp = LinParameters();
    this->m->c_page_offset_data = CHPageOffset();
    this->m->c_shared_object_data = CHSharedObject();
    this->m->c_outline_data = HGeneric();

    QPDFObjectHandle root = getRoot();
    bool outlines_in_first_page = false;
    QPDFObjectHandle pagemode = root.getKey("/PageMode");
    QTC::TC("qpdf", "QPDF categorize pagemode present",
	    pagemode.isName() ? 1 : 0);
    if (pagemode.isName())
    {
	if (pagemode.getName() == "/UseOutlines")
	{
	    if (root.hasKey("/Outlines"))
	    {
		outlines_in_first_page = true;
	    }
	    else
	    {
		QTC::TC("qpdf", "QPDF UseOutlines but no Outlines");
	    }
	}
	QTC::TC("qpdf", "QPDF categorize pagemode outlines",
		outlines_in_first_page ? 1 : 0);
    }

    std::set<std::string> open_document_keys;
    open_document_keys.insert("/ViewerPreferences");
    open_document_keys.insert("/PageMode");
    open_document_keys.insert("/Threads");
    open_document_keys.insert("/OpenAction");
    open_document_keys.insert("/AcroForm");

    std::set<QPDFObjGen> lc_open_document;
    std::set<QPDFObjGen> lc_first_page_private;
    std::set<QPDFObjGen> lc_first_page_shared;
    std::set<QPDFObjGen> lc_other_page_private;
    std::set<QPDFObjGen> lc_other_page_shared;
    std::set<QPDFObjGen> lc_thumbnail_private;
    std::set<QPDFObjGen> lc_thumbnail_shared;
    std::set<QPDFObjGen> lc_other;
    std::set<QPDFObjGen> lc_outlines;
    std::set<QPDFObjGen> lc_root;

    for (std::map<QPDFObjGen, std::set<ObjUser> >::iterator oiter =
	     this->m->object_to_obj_users.begin();
	 oiter != this->m->object_to_obj_users.end(); ++oiter)
    {
	QPDFObjGen const& og = (*oiter).first;

	std::set<ObjUser>& ous = (*oiter).second;

	bool in_open_document = false;
	bool in_first_page = false;
	int other_pages = 0;
	int thumbs = 0;
	int others = 0;
	bool in_outlines = false;
	bool is_root = false;

	for (std::set<ObjUser>::iterator uiter = ous.begin();
	     uiter != ous.end(); ++uiter)
	{
	    ObjUser const& ou = *uiter;
	    switch (ou.ou_type)
	    {
	      case ObjUser::ou_trailer_key:
		if (ou.key == "/Encrypt")
		{
		    in_open_document = true;
		}
		else
		{
		    ++others;
		}
		break;

	      case ObjUser::ou_thumb:
		++thumbs;
		break;

	      case ObjUser::ou_root_key:
		if (open_document_keys.count(ou.key) > 0)
		{
		    in_open_document = true;
		}
		else if (ou.key == "/Outlines")
		{
		    in_outlines = true;
		}
		else
		{
		    ++others;
		}
		break;

	      case ObjUser::ou_page:
		if (ou.pageno == 0)
		{
		    in_first_page = true;
		}
		else
		{
		    ++other_pages;
		}
		break;

	      case ObjUser::ou_root:
		is_root = true;
		break;

	      case ObjUser::ou_bad:
		throw std::logic_error(
		    "INTERNAL ERROR: QPDF::calculateLinearizationData: "
		    "invalid user type");
		break;
	    }
	}

	if (is_root)
	{
	    lc_root.insert(og);
	}
	else if (in_outlines)
	{
	    lc_outlines.insert(og);
	}
	else if (in_open_document)
	{
	    lc_open_document.insert(og);
	}
	else if ((in_first_page) &&
		 (others == 0) && (other_pages == 0) && (thumbs == 0))
	{
	    lc_first_page_private.insert(og);
	}
	else if (in_first_page)
	{
	    lc_first_page_shared.insert(og);
	}
	else if ((other_pages == 1) && (others == 0) && (thumbs == 0))
	{
	    lc_other_page_private.insert(og);
	}
	else if (other_pages > 1)
	{
	    lc_other_page_shared.insert(og);
	}
	else if ((thumbs == 1) && (others == 0))
	{
	    lc_thumbnail_private.insert(og);
	}
	else if (thumbs > 1)
	{
	    lc_thumbnail_shared.insert(og);
	}
	else
	{
	    lc_other.insert(og);
	}
    }

    // Generate ordering for objects in the output file.  Sometimes we
    // just dump right from a set into a vector.  Rather than
    // optimizing this by going straight into the vector, we'll leave
    // these phases separate for now.  That way, this section can be
    // concerned only with ordering, and the above section can be
    // considered only with categorization.  Note that sets of
    // QPDFObjGens are sorted by QPDFObjGen.  In a linearized file,
    // objects appear in sequence with the possible exception of hints
    // tables which we won't see here anyway.  That means that running
    // calculateLinearizationData() on a linearized file should give
    // results identical to the original file ordering.

    // We seem to traverse the page tree a lot in this code, but we
    // can address this for a future code optimization if necessary.
    // Premature optimization is the root of all evil.
    std::vector<QPDFObjectHandle> pages;
    { // local scope
	// Map all page objects to the containing object stream.  This
	// should be a no-op in a properly linearized file.
	std::vector<QPDFObjectHandle> t = getAllPages();
	for (std::vector<QPDFObjectHandle>::iterator iter = t.begin();
	     iter != t.end(); ++iter)
	{
	    pages.push_back(getUncompressedObject(*iter, object_stream_data));
	}
    }
    unsigned int npages = pages.size();

    // We will be initializing some values of the computed hint
    // tables.  Specifically, we can initialize any items that deal
    // with object numbers or counts but not any items that deal with
    // lengths or offsets.  The code that writes linearized files will
    // have to fill in these values during the first pass.  The
    // validation code can compute them relatively easily given the
    // rest of the information.

    // npages is the size of the existing pages vector, which has been
    // created by traversing the pages tree, and as such is a
    // reasonable size.
    this->m->c_linp.npages = npages;
    this->m->c_page_offset_data.entries =
        std::vector<CHPageOffsetEntry>(npages);

    // Part 4: open document objects.  We don't care about the order.

    if (lc_root.size() != 1)
    {
        stopOnError("found other than one root while"
                    " calculating linearization data");
    }
    this->m->part4.push_back(objGenToIndirect(*(lc_root.begin())));
    for (std::set<QPDFObjGen>::iterator iter = lc_open_document.begin();
	 iter != lc_open_document.end(); ++iter)
    {
	this->m->part4.push_back(objGenToIndirect(*iter));
    }

    // Part 6: first page objects.  Note: implementation note 124
    // states that Acrobat always treats page 0 as the first page for
    // linearization regardless of /OpenAction.  pdlin doesn't provide
    // any option to set this and also disregards /OpenAction.  We
    // will do the same.

    // First, place the actual first page object itself.
    QPDFObjGen first_page_og(pages.at(0).getObjGen());
    if (! lc_first_page_private.count(first_page_og))
    {
	throw std::logic_error(
	    "INTERNAL ERROR: QPDF::calculateLinearizationData: first page "
	    "object not in lc_first_page_private");
    }
    lc_first_page_private.erase(first_page_og);
    this->m->c_linp.first_page_object = pages.at(0).getObjectID();
    this->m->part6.push_back(pages.at(0));

    // The PDF spec "recommends" an order for the rest of the objects,
    // but we are going to disregard it except to the extent that it
    // groups private and shared objects contiguously for the sake of
    // hint tables.

    for (std::set<QPDFObjGen>::iterator iter = lc_first_page_private.begin();
	 iter != lc_first_page_private.end(); ++iter)
    {
	this->m->part6.push_back(objGenToIndirect(*iter));
    }

    for (std::set<QPDFObjGen>::iterator iter = lc_first_page_shared.begin();
	 iter != lc_first_page_shared.end(); ++iter)
    {
	this->m->part6.push_back(objGenToIndirect(*iter));
    }

    // Place the outline dictionary if it goes in the first page section.
    if (outlines_in_first_page)
    {
	pushOutlinesToPart(this->m->part6, lc_outlines, object_stream_data);
    }

    // Fill in page offset hint table information for the first page.
    // The PDF spec says that nshared_objects should be zero for the
    // first page.  pdlin does not appear to obey this, but it fills
    // in garbage values for all the shared object identifiers on the
    // first page.

    this->m->c_page_offset_data.entries.at(0).nobjects = this->m->part6.size();

    // Part 7: other pages' private objects

    // For each page in order:
    for (unsigned int i = 1; i < npages; ++i)
    {
	// Place this page's page object

	QPDFObjGen page_og(pages.at(i).getObjGen());
	if (! lc_other_page_private.count(page_og))
	{
	    throw std::logic_error(
		"INTERNAL ERROR: "
		"QPDF::calculateLinearizationData: page object for page " +
		QUtil::int_to_string(i) + " not in lc_other_page_private");
	}
	lc_other_page_private.erase(page_og);
	this->m->part7.push_back(pages.at(i));

	// Place all non-shared objects referenced by this page,
	// updating the page object count for the hint table.

	this->m->c_page_offset_data.entries.at(i).nobjects = 1;

	ObjUser ou(ObjUser::ou_page, i);
	if (this->m->obj_user_to_objects.count(ou) == 0)
        {
            stopOnError("found unreferenced page while"
                        " calculating linearization data");
        }
	std::set<QPDFObjGen> ogs = this->m->obj_user_to_objects[ou];
	for (std::set<QPDFObjGen>::iterator iter = ogs.begin();
	     iter != ogs.end(); ++iter)
	{
	    QPDFObjGen const& og = (*iter);
	    if (lc_other_page_private.count(og))
	    {
		lc_other_page_private.erase(og);
		this->m->part7.push_back(objGenToIndirect(og));
		++this->m->c_page_offset_data.entries.at(i).nobjects;
	    }
	}
    }
    // That should have covered all part7 objects.
    if (! lc_other_page_private.empty())
    {
	throw std::logic_error(
	    "INTERNAL ERROR:"
	    " QPDF::calculateLinearizationData: lc_other_page_private is "
	    "not empty after generation of part7");
    }

    // Part 8: other pages' shared objects

    // Order is unimportant.
    for (std::set<QPDFObjGen>::iterator iter = lc_other_page_shared.begin();
	 iter != lc_other_page_shared.end(); ++iter)
    {
	this->m->part8.push_back(objGenToIndirect(*iter));
    }

    // Part 9: other objects

    // The PDF specification makes recommendations on ordering here.
    // We follow them only to a limited extent.  Specifically, we put
    // the pages tree first, then private thumbnail objects in page
    // order, then shared thumbnail objects, and then outlines (unless
    // in part 6).  After that, we throw all remaining objects in
    // arbitrary order.

    // Place the pages tree.
    std::set<QPDFObjGen> pages_ogs =
	this->m->obj_user_to_objects[ObjUser(ObjUser::ou_root_key, "/Pages")];
    if (pages_ogs.empty())
    {
        stopOnError("found empty pages tree while"
                    " calculating linearization data");
    }
    for (std::set<QPDFObjGen>::iterator iter = pages_ogs.begin();
	 iter != pages_ogs.end(); ++iter)
    {
	QPDFObjGen const& og = *iter;
	if (lc_other.count(og))
	{
	    lc_other.erase(og);
	    this->m->part9.push_back(objGenToIndirect(og));
	}
    }

    // Place private thumbnail images in page order.  Slightly more
    // information would be required if we were going to bother with
    // thumbnail hint tables.
    for (unsigned int i = 0; i < npages; ++i)
    {
	QPDFObjectHandle thumb = pages.at(i).getKey("/Thumb");
	thumb = getUncompressedObject(thumb, object_stream_data);
	if (! thumb.isNull())
	{
	    // Output the thumbnail itself
	    QPDFObjGen thumb_og(thumb.getObjGen());
	    if (lc_thumbnail_private.count(thumb_og))
	    {
		lc_thumbnail_private.erase(thumb_og);
		this->m->part9.push_back(thumb);
	    }
	    else
	    {
		// No internal error this time...there's nothing to
		// stop this object from having been referred to
		// somewhere else outside of a page's /Thumb, and if
		// it had been, there's nothing to prevent it from
		// having been in some set other than
		// lc_thumbnail_private.
	    }
	    std::set<QPDFObjGen>& ogs =
		this->m->obj_user_to_objects[ObjUser(ObjUser::ou_thumb, i)];
	    for (std::set<QPDFObjGen>::iterator iter = ogs.begin();
		 iter != ogs.end(); ++iter)
	    {
		QPDFObjGen const& og = *iter;
		if (lc_thumbnail_private.count(og))
		{
		    lc_thumbnail_private.erase(og);
		    this->m->part9.push_back(objGenToIndirect(og));
		}
	    }
	}
    }
    if (! lc_thumbnail_private.empty())
    {
	throw std::logic_error(
	    "INTERNAL ERROR: "
	    "QPDF::calculateLinearizationData: lc_thumbnail_private "
	    "not empty after placing thumbnails");
    }

    // Place shared thumbnail objects
    for (std::set<QPDFObjGen>::iterator iter = lc_thumbnail_shared.begin();
	 iter != lc_thumbnail_shared.end(); ++iter)
    {
	this->m->part9.push_back(objGenToIndirect(*iter));
    }

    // Place outlines unless in first page
    if (! outlines_in_first_page)
    {
	pushOutlinesToPart(this->m->part9, lc_outlines, object_stream_data);
    }

    // Place all remaining objects
    for (std::set<QPDFObjGen>::iterator iter = lc_other.begin();
	 iter != lc_other.end(); ++iter)
    {
	this->m->part9.push_back(objGenToIndirect(*iter));
    }

    // Make sure we got everything exactly once.

    unsigned int num_placed =
        this->m->part4.size() + this->m->part6.size() + this->m->part7.size() +
        this->m->part8.size() + this->m->part9.size();
    unsigned int num_wanted = this->m->object_to_obj_users.size();
    if (num_placed != num_wanted)
    {
	throw std::logic_error(
	    "INTERNAL ERROR: QPDF::calculateLinearizationData: wrong "
	    "number of objects placed (num_placed = " +
	    QUtil::int_to_string(num_placed) +
	    "; number of objects: " +
	    QUtil::int_to_string(num_wanted));
    }

    // Calculate shared object hint table information including
    // references to shared objects from page offset hint data.

    // The shared object hint table consists of all part 6 (whether
    // shared or not) in order followed by all part 8 objects in
    // order.  Add the objects to shared object data keeping a map of
    // object number to index.  Then populate the shared object
    // information for the pages.

    // Note that two objects never have the same object number, so we
    // can map from object number only without regards to generation.
    std::map<int, int> obj_to_index;

    this->m->c_shared_object_data.nshared_first_page = this->m->part6.size();
    this->m->c_shared_object_data.nshared_total =
	this->m->c_shared_object_data.nshared_first_page +
        this->m->part8.size();

    std::vector<CHSharedObjectEntry>& shared =
	this->m->c_shared_object_data.entries;
    for (std::vector<QPDFObjectHandle>::iterator iter = this->m->part6.begin();
	 iter != this->m->part6.end(); ++iter)
    {
	QPDFObjectHandle& oh = *iter;
	int obj = oh.getObjectID();
	obj_to_index[obj] = shared.size();
	shared.push_back(CHSharedObjectEntry(obj));
    }
    QTC::TC("qpdf", "QPDF lin part 8 empty", this->m->part8.empty() ? 1 : 0);
    if (! this->m->part8.empty())
    {
	this->m->c_shared_object_data.first_shared_obj =
	    this->m->part8.at(0).getObjectID();
	for (std::vector<QPDFObjectHandle>::iterator iter =
		 this->m->part8.begin();
	     iter != this->m->part8.end(); ++iter)
	{
	    QPDFObjectHandle& oh = *iter;
	    int obj = oh.getObjectID();
	    obj_to_index[obj] = shared.size();
	    shared.push_back(CHSharedObjectEntry(obj));
	}
    }
    if (static_cast<size_t>(this->m->c_shared_object_data.nshared_total) !=
        this->m->c_shared_object_data.entries.size())
    {
        throw std::logic_error(
            "shared object hint table has wrong number of entries");
    }

    // Now compute the list of shared objects for each page after the
    // first page.

    for (unsigned int i = 1; i < npages; ++i)
    {
	CHPageOffsetEntry& pe = this->m->c_page_offset_data.entries.at(i);
	ObjUser ou(ObjUser::ou_page, i);
	if (this->m->obj_user_to_objects.count(ou) == 0)
        {
            stopOnError("found unreferenced page while"
                        " calculating linearization data");
        }
	std::set<QPDFObjGen> const& ogs = this->m->obj_user_to_objects[ou];
	for (std::set<QPDFObjGen>::const_iterator iter = ogs.begin();
	     iter != ogs.end(); ++iter)
	{
	    QPDFObjGen const& og = *iter;
	    if ((this->m->object_to_obj_users[og].size() > 1) &&
		(obj_to_index.count(og.getObj()) > 0))
	    {
		int idx = obj_to_index[og.getObj()];
		++pe.nshared_objects;
		pe.shared_identifiers.push_back(idx);
	    }
	}
    }
}