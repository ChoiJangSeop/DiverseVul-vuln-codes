QPDF::optimize(std::map<int, int> const& object_stream_data,
	       bool allow_changes)
{
    if (! this->m->obj_user_to_objects.empty())
    {
	// already optimized
	return;
    }

    // The PDF specification indicates that /Outlines is supposed to
    // be an indirect reference.  Force it to be so if it exists and
    // is direct.  (This has been seen in the wild.)
    QPDFObjectHandle root = getRoot();
    if (root.getKey("/Outlines").isDictionary())
    {
        QPDFObjectHandle outlines = root.getKey("/Outlines");
        if (! outlines.isIndirect())
        {
            QTC::TC("qpdf", "QPDF_optimization indirect outlines");
            root.replaceKey("/Outlines", makeIndirectObject(outlines));
        }
    }

    // Traverse pages tree pushing all inherited resources down to the
    // page level.  This also initializes this->m->all_pages.
    pushInheritedAttributesToPage(allow_changes, false);

    // Traverse pages
    int n = this->m->all_pages.size();
    for (int pageno = 0; pageno < n; ++pageno)
    {
        updateObjectMaps(ObjUser(ObjUser::ou_page, pageno),
                         this->m->all_pages.at(pageno));
    }

    // Traverse document-level items
    std::set<std::string> keys = this->m->trailer.getKeys();
    for (std::set<std::string>::iterator iter = keys.begin();
	 iter != keys.end(); ++iter)
    {
	std::string const& key = *iter;
	if (key == "/Root")
	{
	    // handled separately
	}
	else
	{
	    updateObjectMaps(ObjUser(ObjUser::ou_trailer_key, key),
			     this->m->trailer.getKey(key));
	}
    }

    keys = root.getKeys();
    for (std::set<std::string>::iterator iter = keys.begin();
	 iter != keys.end(); ++iter)
    {
	// Technically, /I keys from /Thread dictionaries are supposed
	// to be handled separately, but we are going to disregard
	// that specification for now.  There is loads of evidence
	// that pdlin and Acrobat both disregard things like this from
	// time to time, so this is almost certain not to cause any
	// problems.

	std::string const& key = *iter;
	updateObjectMaps(ObjUser(ObjUser::ou_root_key, key),
			 root.getKey(key));
    }

    ObjUser root_ou = ObjUser(ObjUser::ou_root);
    QPDFObjGen root_og = QPDFObjGen(root.getObjGen());
    this->m->obj_user_to_objects[root_ou].insert(root_og);
    this->m->object_to_obj_users[root_og].insert(root_ou);

    filterCompressedObjects(object_stream_data);
}