QPDF::writeHGeneric(BitWriter& w, HGeneric& t)
{
    w.writeBits(t.first_object, 32);			    // 1
    w.writeBits(t.first_object_offset, 32);		    // 2
    w.writeBits(t.nobjects, 32);			    // 3
    w.writeBits(t.group_length, 32);			    // 4
}