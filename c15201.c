QPDF::calculateHSharedObject(
    std::map<int, QPDFXRefEntry> const& xref,
    std::map<int, qpdf_offset_t> const& lengths,
    std::map<int, int> const& obj_renumber)
{
    CHSharedObject& cso = this->m->c_shared_object_data;
    std::vector<CHSharedObjectEntry>& csoe = cso.entries;
    HSharedObject& so = this->m->shared_object_hints;
    std::vector<HSharedObjectEntry>& soe = so.entries;
    soe.clear();

    int min_length = outputLengthNextN(
	csoe.at(0).object, 1, lengths, obj_renumber);
    int max_length = min_length;

    for (int i = 0; i < cso.nshared_total; ++i)
    {
	// Assign absolute numbers to deltas; adjust later
	int length = outputLengthNextN(
	    csoe.at(i).object, 1, lengths, obj_renumber);
	min_length = std::min(min_length, length);
	max_length = std::max(max_length, length);
        soe.push_back(HSharedObjectEntry());
	soe.at(i).delta_group_length = length;
    }
    if (soe.size() != static_cast<size_t>(cso.nshared_total))
    {
        throw std::logic_error("soe has wrong size after initialization");
    }

    so.nshared_total = cso.nshared_total;
    so.nshared_first_page = cso.nshared_first_page;
    if (so.nshared_total > so.nshared_first_page)
    {
	so.first_shared_obj =
	    (*(obj_renumber.find(cso.first_shared_obj))).second;
	so.first_shared_offset =
	    (*(xref.find(so.first_shared_obj))).second.getOffset();
    }
    so.min_group_length = min_length;
    so.nbits_delta_group_length = nbits(max_length - min_length);

    for (int i = 0; i < cso.nshared_total; ++i)
    {
	// Adjust deltas
	if (soe.at(i).delta_group_length < min_length)
        {
            stopOnError("found too small group length while"
                        " writing linearization data");
        }
	soe.at(i).delta_group_length -= min_length;
    }
}