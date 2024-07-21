QPDF::dumpHSharedObject()
{
    HSharedObject& t = this->m->shared_object_hints;
    *this->m->out_stream
	<< "first_shared_obj: " << t.first_shared_obj
	<< std::endl
	<< "first_shared_offset: " << adjusted_offset(t.first_shared_offset)
	<< std::endl
	<< "nshared_first_page: " << t.nshared_first_page
	<< std::endl
	<< "nshared_total: " << t.nshared_total
	<< std::endl
	<< "nbits_nobjects: " << t.nbits_nobjects
	<< std::endl
	<< "min_group_length: " << t.min_group_length
	<< std::endl
	<< "nbits_delta_group_length: " << t.nbits_delta_group_length
	<< std::endl;

    for (int i = 0; i < t.nshared_total; ++i)
    {
	HSharedObjectEntry& se = t.entries.at(i);
	*this->m->out_stream
            << "Shared Object " << i << ":" << std::endl
            << "  group length: "
            << se.delta_group_length + t.min_group_length << std::endl;
	// PDF spec says signature present nobjects_minus_one are
	// always 0, so print them only if they have a non-zero value.
	if (se.signature_present)
	{
	    *this->m->out_stream << "  signature present" << std::endl;
	}
	if (se.nobjects_minus_one != 0)
	{
	    *this->m->out_stream << "  nobjects: "
                                 << se.nobjects_minus_one + 1 << std::endl;
	}
    }
}