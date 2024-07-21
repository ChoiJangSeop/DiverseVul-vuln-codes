static void basic_globals_ctor(php_basic_globals *basic_globals_p TSRMLS_DC) /* {{{ */
{
	BG(rand_is_seeded) = 0;
	BG(mt_rand_is_seeded) = 0;
	BG(umask) = -1;
	BG(next) = NULL;
	BG(left) = -1;
	BG(user_tick_functions) = NULL;
	BG(user_filter_map) = NULL;
	BG(serialize_lock) = 0;
	
	memset(&BG(serialize), 0, sizeof(BG(serialize)));
	memset(&BG(unserialize), 0, sizeof(BG(unserialize)));

	memset(&BG(url_adapt_state_ex), 0, sizeof(BG(url_adapt_state_ex)));

#if defined(_REENTRANT) && defined(HAVE_MBRLEN) && defined(HAVE_MBSTATE_T)
	memset(&BG(mblen_state), 0, sizeof(BG(mblen_state)));
#endif

	BG(incomplete_class) = incomplete_class_entry;
	BG(page_uid) = -1;
	BG(page_gid) = -1;
}