ip_cancel_eval_core(interp, msg, flag)
    Tcl_Interp *interp;
    VALUE msg;
    int flag;
{
#if TCL_MAJOR_VERSION < 8 || (TCL_MAJOR_VERSION == 8 && TCL_MINOR_VERSION < 6)
    rb_raise(rb_eNotImpError,
	     "cancel_eval is supported Tcl/Tk8.6 or later.");

    UNREACHABLE;
#else
    Tcl_Obj *msg_obj;

    if (NIL_P(msg)) {
      msg_obj = NULL;
    } else {
      msg_obj = Tcl_NewStringObj(RSTRING_PTR(msg), RSTRING_LEN(msg));
      Tcl_IncrRefCount(msg_obj);
    }

    return Tcl_CancelEval(interp, msg_obj, 0, flag);
#endif
}