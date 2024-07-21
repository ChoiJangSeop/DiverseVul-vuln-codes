date_s_httpdate(int argc, VALUE *argv, VALUE klass)
{
    VALUE str, sg;

    rb_scan_args(argc, argv, "02", &str, &sg);

    switch (argc) {
      case 0:
	str = rb_str_new2("Mon, 01 Jan -4712 00:00:00 GMT");
      case 1:
	sg = INT2FIX(DEFAULT_SG);
    }

    {
	VALUE hash = date_s__httpdate(klass, str);
	return d_new_by_frags(klass, hash, sg);
    }
}