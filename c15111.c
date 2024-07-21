QPDF::readHPageOffset(BitStream h)
{
    // All comments referring to the PDF spec refer to the spec for
    // version 1.4.

    HPageOffset& t = this->m->page_offset_hints;

    t.min_nobjects = h.getBits(32);		  	    // 1
    t.first_page_offset = h.getBits(32);		    // 2
    t.nbits_delta_nobjects = h.getBits(16);		    // 3
    t.min_page_length = h.getBits(32);			    // 4
    t.nbits_delta_page_length = h.getBits(16);		    // 5
    t.min_content_offset = h.getBits(32);		    // 6
    t.nbits_delta_content_offset = h.getBits(16);	    // 7
    t.min_content_length = h.getBits(32);		    // 8
    t.nbits_delta_content_length = h.getBits(16);	    // 9
    t.nbits_nshared_objects = h.getBits(16);		    // 10
    t.nbits_shared_identifier = h.getBits(16);		    // 11
    t.nbits_shared_numerator = h.getBits(16);		    // 12
    t.shared_denominator = h.getBits(16);		    // 13

    std::vector<HPageOffsetEntry>& entries = t.entries;
    entries.clear();
    unsigned int nitems = this->m->linp.npages;
    load_vector_int(h, nitems, entries,
		    t.nbits_delta_nobjects,
		    &HPageOffsetEntry::delta_nobjects);
    load_vector_int(h, nitems, entries,
		    t.nbits_delta_page_length,
		    &HPageOffsetEntry::delta_page_length);
    load_vector_int(h, nitems, entries,
		    t.nbits_nshared_objects,
		    &HPageOffsetEntry::nshared_objects);
    load_vector_vector(h, nitems, entries,
		       &HPageOffsetEntry::nshared_objects,
		       t.nbits_shared_identifier,
		       &HPageOffsetEntry::shared_identifiers);
    load_vector_vector(h, nitems, entries,
		       &HPageOffsetEntry::nshared_objects,
		       t.nbits_shared_numerator,
		       &HPageOffsetEntry::shared_numerators);
    load_vector_int(h, nitems, entries,
		    t.nbits_delta_content_offset,
		    &HPageOffsetEntry::delta_content_offset);
    load_vector_int(h, nitems, entries,
		    t.nbits_delta_content_length,
		    &HPageOffsetEntry::delta_content_length);
}