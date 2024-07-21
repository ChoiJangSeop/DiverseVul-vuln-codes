function_call(int argc, VALUE argv[], VALUE self)
{
    struct nogvl_ffi_call_args args = { 0 };
    fiddle_generic *generic_args;
    VALUE cfunc, types, cPointer;
    int i;
    VALUE alloc_buffer = 0;

    cfunc    = rb_iv_get(self, "@ptr");
    types    = rb_iv_get(self, "@args");
    cPointer = rb_const_get(mFiddle, rb_intern("Pointer"));

    Check_Max_Args("number of arguments", argc);
    if (argc != (i = RARRAY_LENINT(types))) {
	rb_error_arity(argc, i, i);
    }

    TypedData_Get_Struct(self, ffi_cif, &function_data_type, args.cif);

    if (rb_safe_level() >= 1) {
	for (i = 0; i < argc; i++) {
	    VALUE src = argv[i];
	    if (OBJ_TAINTED(src)) {
		rb_raise(rb_eSecurityError, "tainted parameter not allowed");
	    }
	}
    }

    generic_args = ALLOCV(alloc_buffer,
	(size_t)(argc + 1) * sizeof(void *) + (size_t)argc * sizeof(fiddle_generic));
    args.values = (void **)((char *)generic_args +
			    (size_t)argc * sizeof(fiddle_generic));

    for (i = 0; i < argc; i++) {
	VALUE type = RARRAY_AREF(types, i);
	VALUE src = argv[i];

	if(NUM2INT(type) == TYPE_VOIDP) {
	    if(NIL_P(src)) {
		src = INT2FIX(0);
	    } else if(cPointer != CLASS_OF(src)) {
		src = rb_funcall(cPointer, rb_intern("[]"), 1, src);
	    }
	    src = rb_Integer(src);
	}

	VALUE2GENERIC(NUM2INT(type), src, &generic_args[i]);
	args.values[i] = (void *)&generic_args[i];
    }
    args.values[argc] = NULL;
    args.fn = NUM2PTR(rb_Integer(cfunc));

    (void)rb_thread_call_without_gvl(nogvl_ffi_call, &args, 0, 0);

    rb_funcall(mFiddle, rb_intern("last_error="), 1, INT2NUM(errno));
#if defined(_WIN32)
    rb_funcall(mFiddle, rb_intern("win32_last_error="), 1, INT2NUM(errno));
#endif

    ALLOCV_END(alloc_buffer);

    return GENERIC2VALUE(rb_iv_get(self, "@return_type"), args.retval);
}