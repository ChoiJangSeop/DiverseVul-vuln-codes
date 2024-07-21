QPDF::calculateHPageOffset(
    std::map<int, QPDFXRefEntry> const& xref,
    std::map<int, qpdf_offset_t> const& lengths,
    std::map<int, int> const& obj_renumber)
{
    // Page Offset Hint Table

    // We are purposely leaving some values set to their initial zero
    // values.

    std::vector<QPDFObjectHandle> const& pages = getAllPages();
    unsigned int npages = pages.size();
    CHPageOffset& cph = this->m->c_page_offset_data;
    std::vector<CHPageOffsetEntry>& cphe = cph.entries;

    // Calculate minimum and maximum values for number of objects per
    // page and page length.

    int min_nobjects = cphe.at(0).nobjects;
    int max_nobjects = min_nobjects;
    int min_length = outputLengthNextN(
	pages.at(0).getObjectID(), min_nobjects, lengths, obj_renumber);
    int max_length = min_length;
    int max_shared = cphe.at(0).nshared_objects;

    HPageOffset& ph = this->m->page_offset_hints;
    std::vector<HPageOffsetEntry>& phe = ph.entries;
    // npages is the size of the existing pages array.
    phe = std::vector<HPageOffsetEntry>(npages);

    for (unsigned int i = 0; i < npages; ++i)
    {
	// Calculate values for each page, assigning full values to
	// the delta items.  They will be adjusted later.

	// Repeat calculations for page 0 so we can assign to phe[i]
	// without duplicating those assignments.

	int nobjects = cphe.at(i).nobjects;
	int length = outputLengthNextN(
	    pages.at(i).getObjectID(), nobjects, lengths, obj_renumber);
	int nshared = cphe.at(i).nshared_objects;

	min_nobjects = std::min(min_nobjects, nobjects);
	max_nobjects = std::max(max_nobjects, nobjects);
	min_length = std::min(min_length, length);
	max_length = std::max(max_length, length);
	max_shared = std::max(max_shared, nshared);

	phe.at(i).delta_nobjects = nobjects;
	phe.at(i).delta_page_length = length;
	phe.at(i).nshared_objects = nshared;
    }

    ph.min_nobjects = min_nobjects;
    int in_page0_id = pages.at(0).getObjectID();
    int out_page0_id = (*(obj_renumber.find(in_page0_id))).second;
    ph.first_page_offset = (*(xref.find(out_page0_id))).second.getOffset();
    ph.nbits_delta_nobjects = nbits(max_nobjects - min_nobjects);
    ph.min_page_length = min_length;
    ph.nbits_delta_page_length = nbits(max_length - min_length);
    ph.nbits_nshared_objects = nbits(max_shared);
    ph.nbits_shared_identifier =
	nbits(this->m->c_shared_object_data.nshared_total);
    ph.shared_denominator = 4;	// doesn't matter

    // It isn't clear how to compute content offset and content
    // length.  Since we are not interleaving page objects with the
    // content stream, we'll use the same values for content length as
    // page length.  We will use 0 as content offset because this is
    // what Adobe does (implementation note 127) and pdlin as well.
    ph.nbits_delta_content_length = ph.nbits_delta_page_length;
    ph.min_content_length = ph.min_page_length;

    for (unsigned int i = 0; i < npages; ++i)
    {
	// Adjust delta entries
	if ((phe.at(i).delta_nobjects < min_nobjects) ||
            (phe.at(i).delta_page_length < min_length))
        {
            stopOnError("found too small delta nobjects or delta page length"
                        " while writing linearization data");
        }
	phe.at(i).delta_nobjects -= min_nobjects;
	phe.at(i).delta_page_length -= min_length;
	phe.at(i).delta_content_length = phe.at(i).delta_page_length;

	for (int j = 0; j < cphe.at(i).nshared_objects; ++j)
	{
	    phe.at(i).shared_identifiers.push_back(
		cphe.at(i).shared_identifiers.at(j));
	    phe.at(i).shared_numerators.push_back(0);
	}
    }
}