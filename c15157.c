QPDF::writeHSharedObject(BitWriter& w)
{
    HSharedObject& t = this->m->shared_object_hints;

    w.writeBits(t.first_shared_obj, 32);		    // 1
    w.writeBits(t.first_shared_offset, 32);		    // 2
    w.writeBits(t.nshared_first_page, 32);		    // 3
    w.writeBits(t.nshared_total, 32);			    // 4
    w.writeBits(t.nbits_nobjects, 16);			    // 5
    w.writeBits(t.min_group_length, 32);		    // 6
    w.writeBits(t.nbits_delta_group_length, 16);	    // 7

    QTC::TC("qpdf", "QPDF lin write nshared_total > nshared_first_page",
	    (t.nshared_total > t.nshared_first_page) ? 1 : 0);

    int nitems = t.nshared_total;
    std::vector<HSharedObjectEntry>& entries = t.entries;

    write_vector_int(w, nitems, entries,
		     t.nbits_delta_group_length,
		     &HSharedObjectEntry::delta_group_length);
    write_vector_int(w, nitems, entries,
		     1, &HSharedObjectEntry::signature_present);
    for (int i = 0; i < nitems; ++i)
    {
	// If signature were present, we'd have to write a 128-bit hash.
	if (entries.at(i).signature_present != 0)
        {
            stopOnError("found unexpected signature present"
                        " while writing linearization data");
        }
    }
    write_vector_int(w, nitems, entries,
		     t.nbits_nobjects,
		     &HSharedObjectEntry::nobjects_minus_one);
}