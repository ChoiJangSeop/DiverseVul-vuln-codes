rb_f_rand(argc, argv, obj)
    int argc;
    VALUE *argv;
    VALUE obj;
{
    VALUE vmax;
    long val, max;

    rb_scan_args(argc, argv, "01", &vmax);
    if (first) {
	rand_init(random_seed());
    }
    switch (TYPE(vmax)) {
      case T_FLOAT:
	if (RFLOAT(vmax)->value <= LONG_MAX && RFLOAT(vmax)->value >= LONG_MIN) {
	    max = (long)RFLOAT(vmax)->value;
	    break;
	}
        if (RFLOAT(vmax)->value < 0)
            vmax = rb_dbl2big(-RFLOAT(vmax)->value);
        else
            vmax = rb_dbl2big(RFLOAT(vmax)->value);
	/* fall through */
      case T_BIGNUM:
      bignum:
        {
            struct RBignum *limit = (struct RBignum *)vmax;
            if (!limit->sign) {
                limit = (struct RBignum *)rb_big_clone(vmax);
                limit->sign = 1;
            }
            limit = (struct RBignum *)rb_big_minus((VALUE)limit, INT2FIX(1));
            if (FIXNUM_P((VALUE)limit)) {
                if (FIX2LONG((VALUE)limit) == -1)
                    return rb_float_new(genrand_real());
                return LONG2NUM(limited_rand(FIX2LONG((VALUE)limit)));
            }
            return limited_big_rand(limit);
	}
      case T_NIL:
	max = 0;
	break;
      default:
	vmax = rb_Integer(vmax);
	if (TYPE(vmax) == T_BIGNUM) goto bignum;
	/* fall through */
      case T_FIXNUM:
	max = FIX2LONG(vmax);
	break;
    }

    if (max == 0) {
	return rb_float_new(genrand_real());
    }
    if (max < 0) max = -max;
    val = limited_rand(max-1);
    return LONG2NUM(val);
}