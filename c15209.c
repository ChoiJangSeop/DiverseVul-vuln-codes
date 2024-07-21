QPDF::readHGeneric(BitStream h, HGeneric& t)
{
    t.first_object = h.getBits(32);			    // 1
    t.first_object_offset = h.getBits(32);		    // 2
    t.nobjects = h.getBits(32);				    // 3
    t.group_length = h.getBits(32);			    // 4
}