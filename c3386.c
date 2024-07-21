PHP_MINIT_FUNCTION(wddx)
{
	le_wddx = zend_register_list_destructors_ex(release_wddx_packet_rsrc, NULL, "wddx", module_number);

#if HAVE_PHP_SESSION && !defined(COMPILE_DL_SESSION)
	php_session_register_serializer("wddx",
									PS_SERIALIZER_ENCODE_NAME(wddx),
									PS_SERIALIZER_DECODE_NAME(wddx));
#endif	

	return SUCCESS;
}