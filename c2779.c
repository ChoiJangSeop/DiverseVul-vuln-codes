static void pcntl_sigwaitinfo(INTERNAL_FUNCTION_PARAMETERS, int timedwait) /* {{{ */
{
	zval            *user_set, **user_signo, *user_siginfo = NULL;
	long             tv_sec = 0, tv_nsec = 0;
	sigset_t         set;
	HashPosition     pos;
	int              signo;
	siginfo_t        siginfo;
	struct timespec  timeout;

	if (timedwait) {
		if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "a|zll", &user_set, &user_siginfo, &tv_sec, &tv_nsec) == FAILURE) {
			return;
		}
	} else {
		if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "a|z", &user_set, &user_siginfo) == FAILURE) {
			return;
		}
	}

	if (sigemptyset(&set) != 0) {
		PCNTL_G(last_error) = errno;
		php_error_docref(NULL TSRMLS_CC, E_WARNING, "%s", strerror(errno));
		RETURN_FALSE;
	}

	zend_hash_internal_pointer_reset_ex(Z_ARRVAL_P(user_set), &pos);
	while (zend_hash_get_current_data_ex(Z_ARRVAL_P(user_set), (void **)&user_signo, &pos) == SUCCESS)
	{
		if (Z_TYPE_PP(user_signo) != IS_LONG) {
			SEPARATE_ZVAL(user_signo);
			convert_to_long_ex(user_signo);
		}
		signo = Z_LVAL_PP(user_signo);
		if (sigaddset(&set, signo) != 0) {
			PCNTL_G(last_error) = errno;
			php_error_docref(NULL TSRMLS_CC, E_WARNING, "%s", strerror(errno));
			RETURN_FALSE;
		}
		zend_hash_move_forward_ex(Z_ARRVAL_P(user_set), &pos);
	}

	if (timedwait) {
		timeout.tv_sec  = (time_t) tv_sec;
		timeout.tv_nsec = tv_nsec;
		signo = sigtimedwait(&set, &siginfo, &timeout);
	} else {
		signo = sigwaitinfo(&set, &siginfo);
	}
	if (signo == -1 && errno != EAGAIN) {
		PCNTL_G(last_error) = errno;
		php_error_docref(NULL TSRMLS_CC, E_WARNING, "%s", strerror(errno));
	}

	/*
	 * sigtimedwait and sigwaitinfo can return 0 on success on some 
	 * platforms, e.g. NetBSD
	 */
	if (!signo && siginfo.si_signo) {
		signo = siginfo.si_signo;
	}

	if (signo > 0 && user_siginfo) {
		if (Z_TYPE_P(user_siginfo) != IS_ARRAY) {
			zval_dtor(user_siginfo);
			array_init(user_siginfo);
		} else {
			zend_hash_clean(Z_ARRVAL_P(user_siginfo));
		}
		add_assoc_long_ex(user_siginfo, "signo", sizeof("signo"), siginfo.si_signo);
		add_assoc_long_ex(user_siginfo, "errno", sizeof("errno"), siginfo.si_errno);
		add_assoc_long_ex(user_siginfo, "code",  sizeof("code"),  siginfo.si_code);
		switch(signo) {
#ifdef SIGCHLD
			case SIGCHLD:
				add_assoc_long_ex(user_siginfo,   "status", sizeof("status"), siginfo.si_status);
# ifdef si_utime
				add_assoc_double_ex(user_siginfo, "utime",  sizeof("utime"),  siginfo.si_utime);
# endif
# ifdef si_stime
				add_assoc_double_ex(user_siginfo, "stime",  sizeof("stime"),  siginfo.si_stime);
# endif
				add_assoc_long_ex(user_siginfo,   "pid",    sizeof("pid"),    siginfo.si_pid);
				add_assoc_long_ex(user_siginfo,   "uid",    sizeof("uid"),    siginfo.si_uid);
				break;
#endif
			case SIGILL:
			case SIGFPE:
			case SIGSEGV:
			case SIGBUS:
				add_assoc_double_ex(user_siginfo, "addr", sizeof("addr"), (long)siginfo.si_addr);
				break;
#ifdef SIGPOLL
			case SIGPOLL:
				add_assoc_long_ex(user_siginfo, "band", sizeof("band"), siginfo.si_band);
# ifdef si_fd
				add_assoc_long_ex(user_siginfo, "fd",   sizeof("fd"),   siginfo.si_fd);
# endif
				break;
#endif
			EMPTY_SWITCH_DEFAULT_CASE();
		}
	}
	
	RETURN_LONG(signo);
}