QPDF::dumpHPageOffset()
{
    HPageOffset& t = this->m->page_offset_hints;
    *this->m->out_stream
	<< "min_nobjects: " << t.min_nobjects
	<< std::endl
	<< "first_page_offset: " << adjusted_offset(t.first_page_offset)
	<< std::endl
	<< "nbits_delta_nobjects: " << t.nbits_delta_nobjects
	<< std::endl
	<< "min_page_length: " << t.min_page_length
	<< std::endl
	<< "nbits_delta_page_length: " << t.nbits_delta_page_length
	<< std::endl
	<< "min_content_offset: " << t.min_content_offset
	<< std::endl
	<< "nbits_delta_content_offset: " << t.nbits_delta_content_offset
	<< std::endl
	<< "min_content_length: " << t.min_content_length
	<< std::endl
	<< "nbits_delta_content_length: " << t.nbits_delta_content_length
	<< std::endl
	<< "nbits_nshared_objects: " << t.nbits_nshared_objects
	<< std::endl
	<< "nbits_shared_identifier: " << t.nbits_shared_identifier
	<< std::endl
	<< "nbits_shared_numerator: " << t.nbits_shared_numerator
	<< std::endl
	<< "shared_denominator: " << t.shared_denominator
	<< std::endl;

    for (int i1 = 0; i1 < this->m->linp.npages; ++i1)
    {
	HPageOffsetEntry& pe = t.entries.at(i1);
	*this->m->out_stream
	    << "Page " << i1 << ":" << std::endl
	    << "  nobjects: " << pe.delta_nobjects + t.min_nobjects
	    << std::endl
	    << "  length: " << pe.delta_page_length + t.min_page_length
	    << std::endl
	    // content offset is relative to page, not file
	    << "  content_offset: "
	    << pe.delta_content_offset + t.min_content_offset << std::endl
	    << "  content_length: "
	    << pe.delta_content_length + t.min_content_length << std::endl
	    << "  nshared_objects: " << pe.nshared_objects << std::endl;
	for (int i2 = 0; i2 < pe.nshared_objects; ++i2)
	{
	    *this->m->out_stream << "    identifier " << i2 << ": "
                                 << pe.shared_identifiers.at(i2) << std::endl;
	    *this->m->out_stream << "    numerator " << i2 << ": "
                                 << pe.shared_numerators.at(i2) << std::endl;
	}
    }
}