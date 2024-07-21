rb_fiddle_handle_sym(VALUE self, VALUE sym)
{
    struct dl_handle *fiddle_handle;

    TypedData_Get_Struct(self, struct dl_handle, &fiddle_handle_data_type, fiddle_handle);
    if( ! fiddle_handle->open ){
	rb_raise(rb_eFiddleError, "closed handle");
    }

    return fiddle_handle_sym(fiddle_handle->ptr, StringValueCStr(sym));
}