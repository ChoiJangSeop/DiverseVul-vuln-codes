int_ossl_asn1_decode0_cons(unsigned char **pp, long max_len, long length,
			   long *offset, int depth, int yield, int j,
			   int tag, VALUE tc, long *num_read)
{
    VALUE value, asn1data, ary;
    int infinite;
    long off = *offset;

    infinite = (j == 0x21);
    ary = rb_ary_new();

    while (length > 0 || infinite) {
	long inner_read = 0;
	value = ossl_asn1_decode0(pp, max_len, &off, depth + 1, yield, &inner_read);
	*num_read += inner_read;
	max_len -= inner_read;
	rb_ary_push(ary, value);
	if (length > 0)
	    length -= inner_read;

	if (infinite &&
	    NUM2INT(ossl_asn1_get_tag(value)) == V_ASN1_EOC &&
	    ossl_asn1_get_tag_class(value) == sym_UNIVERSAL) {
	    break;
	}
    }

    if (tc == sym_UNIVERSAL) {
	VALUE args[4];
	int not_sequence_or_set;

	not_sequence_or_set = tag != V_ASN1_SEQUENCE && tag != V_ASN1_SET;

	if (not_sequence_or_set) {
	    if (infinite) {
		asn1data = rb_obj_alloc(cASN1Constructive);
	    }
	    else {
		ossl_raise(eASN1Error, "invalid non-infinite tag");
		return Qnil;
	    }
	}
	else {
	    VALUE klass = *ossl_asn1_info[tag].klass;
	    asn1data = rb_obj_alloc(klass);
	}
	args[0] = ary;
	args[1] = INT2NUM(tag);
	args[2] = Qnil;
	args[3] = tc;
	ossl_asn1_initialize(4, args, asn1data);
    }
    else {
	asn1data = rb_obj_alloc(cASN1Data);
	ossl_asn1data_initialize(asn1data, ary, INT2NUM(tag), tc);
    }

    if (infinite)
	ossl_asn1_set_infinite_length(asn1data, Qtrue);
    else
	ossl_asn1_set_infinite_length(asn1data, Qfalse);

    *offset = off;
    return asn1data;
}