PHP_MINIT_FUNCTION(com_dotnet)
{
	zend_class_entry ce, *tmp;

	php_com_wrapper_minit(INIT_FUNC_ARGS_PASSTHRU);
	php_com_persist_minit(INIT_FUNC_ARGS_PASSTHRU);

	INIT_CLASS_ENTRY(ce, "com_exception", NULL);
	php_com_exception_class_entry = zend_register_internal_class_ex(&ce, zend_ce_exception);
	php_com_exception_class_entry->ce_flags |= ZEND_ACC_FINAL;
/*	php_com_exception_class_entry->constructor->common.fn_flags |= ZEND_ACC_PROTECTED; */

	INIT_CLASS_ENTRY(ce, "com_safearray_proxy", NULL);
	php_com_saproxy_class_entry = zend_register_internal_class(&ce);
	php_com_saproxy_class_entry->ce_flags |= ZEND_ACC_FINAL;
/*	php_com_saproxy_class_entry->constructor->common.fn_flags |= ZEND_ACC_PROTECTED; */
	php_com_saproxy_class_entry->get_iterator = php_com_saproxy_iter_get;

	INIT_CLASS_ENTRY(ce, "variant", NULL);
	ce.create_object = php_com_object_new;
	php_com_variant_class_entry = zend_register_internal_class(&ce);
	php_com_variant_class_entry->get_iterator = php_com_iter_get;

	INIT_CLASS_ENTRY(ce, "com", NULL);
	ce.create_object = php_com_object_new;
	tmp = zend_register_internal_class_ex(&ce, php_com_variant_class_entry);
	tmp->get_iterator = php_com_iter_get;

	zend_ts_hash_init(&php_com_typelibraries, 0, NULL, php_com_typelibrary_dtor, 1);

#if HAVE_MSCOREE_H
	INIT_CLASS_ENTRY(ce, "dotnet", NULL);
	ce.create_object = php_com_object_new;
	tmp = zend_register_internal_class_ex(&ce, php_com_variant_class_entry);
	tmp->get_iterator = php_com_iter_get;
#endif

	REGISTER_INI_ENTRIES();

#define COM_CONST(x) REGISTER_LONG_CONSTANT(#x, x, CONST_CS|CONST_PERSISTENT)

#define COM_ERR_CONST(x) { \
	zend_long __tmp; \
	ULongToIntPtr(x, &__tmp); \
	REGISTER_LONG_CONSTANT(#x, __tmp, CONST_CS|CONST_PERSISTENT); \
}

	COM_CONST(CLSCTX_INPROC_SERVER);
	COM_CONST(CLSCTX_INPROC_HANDLER);
	COM_CONST(CLSCTX_LOCAL_SERVER);
	COM_CONST(CLSCTX_REMOTE_SERVER);
	COM_CONST(CLSCTX_SERVER);
	COM_CONST(CLSCTX_ALL);

#if 0
	COM_CONST(DISPATCH_METHOD);
	COM_CONST(DISPATCH_PROPERTYGET);
	COM_CONST(DISPATCH_PROPERTYPUT);
#endif

	COM_CONST(VT_NULL);
	COM_CONST(VT_EMPTY);
	COM_CONST(VT_UI1);
	COM_CONST(VT_I1);
	COM_CONST(VT_UI2);
	COM_CONST(VT_I2);
	COM_CONST(VT_UI4);
	COM_CONST(VT_I4);
	COM_CONST(VT_R4);
	COM_CONST(VT_R8);
	COM_CONST(VT_BOOL);
	COM_CONST(VT_ERROR);
	COM_CONST(VT_CY);
	COM_CONST(VT_DATE);
	COM_CONST(VT_BSTR);
	COM_CONST(VT_DECIMAL);
	COM_CONST(VT_UNKNOWN);
	COM_CONST(VT_DISPATCH);
	COM_CONST(VT_VARIANT);
	COM_CONST(VT_INT);
	COM_CONST(VT_UINT);
	COM_CONST(VT_ARRAY);
	COM_CONST(VT_BYREF);

	COM_CONST(CP_ACP);
	COM_CONST(CP_MACCP);
	COM_CONST(CP_OEMCP);
	COM_CONST(CP_UTF7);
	COM_CONST(CP_UTF8);
	COM_CONST(CP_SYMBOL);
	COM_CONST(CP_THREAD_ACP);

	COM_CONST(VARCMP_LT);
	COM_CONST(VARCMP_EQ);
	COM_CONST(VARCMP_GT);
	COM_CONST(VARCMP_NULL);

	COM_CONST(NORM_IGNORECASE);
	COM_CONST(NORM_IGNORENONSPACE);
	COM_CONST(NORM_IGNORESYMBOLS);
	COM_CONST(NORM_IGNOREWIDTH);
	COM_CONST(NORM_IGNOREKANATYPE);
#ifdef NORM_IGNOREKASHIDA
	COM_CONST(NORM_IGNOREKASHIDA);
#endif
	COM_ERR_CONST(DISP_E_DIVBYZERO);
	COM_ERR_CONST(DISP_E_OVERFLOW);
	COM_ERR_CONST(DISP_E_BADINDEX);
	COM_ERR_CONST(MK_E_UNAVAILABLE);

#if SIZEOF_ZEND_LONG == 8
	COM_CONST(VT_UI8);
	COM_CONST(VT_I8);
#endif
	return SUCCESS;
}