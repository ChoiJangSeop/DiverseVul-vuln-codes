datetime_s_xmlschema(int argc, VALUE *argv, VALUE klass)
{
    VALUE str, sg;

    rb_scan_args(argc, argv, "02", &str, &sg);

    switch (argc) {
      case 0:
	str = rb_str_new2("-4712-01-01T00:00:00+00:00");
      case 1:
	sg = INT2FIX(DEFAULT_SG);
    }

    {
	VALUE hash = date_s__xmlschema(klass, str);
	return dt_new_by_frags(klass, hash, sg);
    }
}