lib_merge_tklist(argc, argv, obj)
    int argc;
    VALUE *argv;
    VALUE obj;
{
    int  num, len;
    int  *flagPtr;
    char *dst, *result;
    volatile VALUE str;
    int taint_flag = 0;
    int thr_crit_bup;
    VALUE old_gc;

    if (argc == 0) return rb_str_new2("");

    tcl_stubs_check();

    thr_crit_bup = rb_thread_critical;
    rb_thread_critical = Qtrue;
    old_gc = rb_gc_disable();

    /* based on Tcl/Tk's Tcl_Merge() */
    /* flagPtr = ALLOC_N(int, argc); */
    flagPtr = RbTk_ALLOC_N(int, argc);
#if 0 /* use Tcl_Preserve/Release */
    Tcl_Preserve((ClientData)flagPtr); /* XXXXXXXXXX */
#endif

    /* pass 1 */
    len = 1;
    for(num = 0; num < argc; num++) {
        if (OBJ_TAINTED(argv[num])) taint_flag = 1;
        dst = StringValuePtr(argv[num]);
#if TCL_MAJOR_VERSION >= 8
        len += Tcl_ScanCountedElement(dst, RSTRING_LENINT(argv[num]),
                                      &flagPtr[num]) + 1;
#else /* TCL_MAJOR_VERSION < 8 */
        len += Tcl_ScanElement(dst, &flagPtr[num]) + 1;
#endif
    }

    /* pass 2 */
    /* result = (char *)Tcl_Alloc(len); */
    result = (char *)ckalloc(len);
#if 0 /* use Tcl_Preserve/Release */
    Tcl_Preserve((ClientData)result);
#endif
    dst = result;
    for(num = 0; num < argc; num++) {
#if TCL_MAJOR_VERSION >= 8
        len = Tcl_ConvertCountedElement(RSTRING_PTR(argv[num]),
                                        RSTRING_LENINT(argv[num]),
                                        dst, flagPtr[num]);
#else /* TCL_MAJOR_VERSION < 8 */
        len = Tcl_ConvertElement(RSTRING_PTR(argv[num]), dst, flagPtr[num]);
#endif
        dst += len;
        *dst = ' ';
        dst++;
    }
    if (dst == result) {
        *dst = 0;
    } else {
        dst[-1] = 0;
    }

#if 0 /* use Tcl_EventuallyFree */
    Tcl_EventuallyFree((ClientData)flagPtr, TCL_DYNAMIC); /* XXXXXXXX */
#else
#if 0 /* use Tcl_Preserve/Release */
    Tcl_Release((ClientData)flagPtr);
#else
    /* free(flagPtr); */
    ckfree((char*)flagPtr);
#endif
#endif

    /* create object */
    str = rb_str_new(result, dst - result - 1);
    if (taint_flag) RbTk_OBJ_UNTRUST(str);
#if 0 /* use Tcl_EventuallyFree */
    Tcl_EventuallyFree((ClientData)result, TCL_DYNAMIC); /* XXXXXXXX */
#else
#if 0 /* use Tcl_Preserve/Release */
    Tcl_Release((ClientData)result); /* XXXXXXXXXXX */
#else
    /* Tcl_Free(result); */
    ckfree(result);
#endif
#endif

    if (old_gc == Qfalse) rb_gc_enable();
    rb_thread_critical = thr_crit_bup;

    return str;
}