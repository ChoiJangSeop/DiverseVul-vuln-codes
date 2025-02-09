auth_shadow (
  /* PARAMETERS */
  const char *login,			/* I: plaintext authenticator */
  const char *password,			/* I: plaintext password */
  const char *service __attribute__((unused)),
  const char *realm __attribute__((unused))
  /* END PARAMETERS */
  )
{

/************************************************************************
 *									*
 * This is gross. Everyone wants to do this differently, thus we have   *
 * to #ifdef the whole mess for each system type.                       *
 *									*
 ***********************************************************************/

# ifdef HAVE_GETSPNAM

 /***************
 * getspnam_r() *
 ***************/

    /* VARIABLES */
    long today;				/* the current time */
    char *cpw;				/* pointer to crypt() result */
    struct passwd	*pw;		/* return from getpwnam_r() */
    struct spwd   	*sp;		/* return from getspnam_r() */
    int errnum;
#  ifdef _REENTRANT
    struct passwd pwbuf;
    char pwdata[PWBUFSZ];		/* pwbuf indirect data goes in here */

    struct spwd spbuf;
    char spdata[PWBUFSZ];		/* spbuf indirect data goes in here */
#  endif /* _REENTRANT */
    /* END VARIABLES */

#  define RETURN(x) return strdup(x)

    /*
     * "Magic" password field entries for SunOS/SysV
     *
     * "*LK*" is defined at in the shadow(4) man page, but of course any string
     * inserted in front of the password will prevent the strings from matching
     *
     * *NP* is documented in getspnam(3) and indicates the caller had
     * insufficient permission to read the shadow password database
     * (generally this is a NIS error).
     */

#  define SHADOW_PW_LOCKED "*LK*"	/* account locked (not used by us) */
#  define SHADOW_PW_EPERM  "*NP*"	/* insufficient database perms */

#  ifdef _REENTRANT
#    ifdef GETXXNAM_R_5ARG
	(void) getpwnam_r(login, &pwbuf, pwdata, sizeof(pwdata), &pw);
#    else
    pw = getpwnam_r(login, &pwbuf, pwdata, sizeof(pwdata));
#    endif /* GETXXNAM_R_5ARG */
#  else
    pw = getpwnam(login);
#  endif /* _REENTRANT */
    errnum = errno;
    endpwent();

    if (pw == NULL) {
	if (errnum != 0) {
	    char *errstr;

	    if (flags & VERBOSE) {
		syslog(LOG_DEBUG, "DEBUG: auth_shadow: getpwnam(%s) failure: %m", login);
	    }
	    if (asprintf(&errstr, "NO Username lookup failure: %s", strerror(errno)) == -1) {
		/* XXX the hidden strdup() will likely fail and return NULL here.... */
		RETURN("NO Username lookup failure: unknown error (ENOMEM formatting strerror())");
	    }
	    return errstr;
	} else {
	    if (flags & VERBOSE) {
		syslog(LOG_DEBUG, "DEBUG: auth_shadow: getpwnam(%s): invalid username", login);
	    }
	    RETURN("NO Invalid username");
	}
    }

    today = (long)time(NULL)/(24L*60*60);

#  ifdef _REENTRANT
#    ifdef GETXXNAM_R_5ARG
	(void) getspnam_r(login, &spbuf, spdata, sizeof(spdata), &sp);
#    else
    sp = getspnam_r(login, &spbuf, spdata, sizeof(spdata));
#    endif /* GETXXNAM_R_5ARG */
#  else
    sp = getspnam(login);
#  endif /* _REENTRANT */
    errnum = errno;
    endspent();

    if (sp == NULL) {
	if (errnum != 0) {
	    char *errstr;

	    if (flags & VERBOSE) {
		syslog(LOG_DEBUG, "DEBUG: auth_shadow: getspnam(%s) failure: %m", login);
	    }
	    if (asprintf(&errstr, "NO Username shadow lookup failure: %s", strerror(errno)) == -1) {
		/* XXX the hidden strdup() will likely fail and return NULL here.... */
		RETURN("NO Username shadow lookup failure: unknown error (ENOMEM formatting strerror())");
	    }
	    return errstr;
	} else {
	    if (flags & VERBOSE) {
		syslog(LOG_DEBUG, "DEBUG: auth_shadow: getspnam(%s): invalid shadow username", login);
	    }
	    RETURN("NO Invalid shadow username");
	}
    }

    if (!strcmp(sp->sp_pwdp, SHADOW_PW_EPERM)) {
	if (flags & VERBOSE) {
	    syslog(LOG_DEBUG, "DEBUG: auth_shadow: sp->sp_pwdp == SHADOW_PW_EPERM");
	}
	RETURN("NO Insufficient permission to access NIS authentication database (saslauthd)");
    }

    cpw = strdup((const char *)crypt(password, sp->sp_pwdp));
    if (strcmp(sp->sp_pwdp, cpw)) {
	if (flags & VERBOSE) {
	    /*
	     * This _should_ reveal the SHADOW_PW_LOCKED prefix to an
	     * administrator trying to debug the situation, though maybe we
	     * should do the check here and be less obtuse about it....
	     */
	    syslog(LOG_DEBUG, "DEBUG: auth_shadow: pw mismatch: '%s' != '%s'",
		   sp->sp_pwdp, cpw);
	}
	free(cpw);
	RETURN("NO Incorrect password");
    }
    free(cpw);

    /*
     * The following fields will be set to -1 if:
     *
     *	1) They are not specified in the shadow database, or
     *	2) The database is being served up by NIS.
     */

    if ((sp->sp_expire != -1) && (today > sp->sp_expire)) {
	if (flags & VERBOSE) {
	    syslog(LOG_DEBUG, "DEBUG: auth_shadow: account expired: %dl > %dl",
		   today, sp->sp_expire);
	}
	RETURN("NO Account expired");
    }

    /* Remaining tests are relative to the last change date for the password */

    if (sp->sp_lstchg != -1) {

	if ((sp->sp_max != -1) && ((sp->sp_lstchg + sp->sp_max) < today)) {
	    if (flags & VERBOSE) {
		syslog(LOG_DEBUG,
		       "DEBUG: auth_shadow: password expired: %ld + %ld < %ld",
		       sp->sp_lstchg, sp->sp_max, today);
	    }
	    RETURN("NO Password expired");
	}
    }
    if (flags & VERBOSE) {
	syslog(LOG_DEBUG, "DEBUG: auth_shadow: OK: %s", login);
    }
    RETURN("OK");


# elif defined(HAVE_GETUSERPW)

/*************
 * AIX 4.1.4 *
 ************/
    /* VARIABLES */
    struct userpw *upw;				/* return from getuserpw() */
    /* END VARIABLES */

#  define RETURN(x) { endpwdb(); return strdup(x); }

    if (setpwdb(S_READ) == -1) {
	syslog(LOG_ERR, "setpwdb: %m");
	RETURN("NO setpwdb() internal failure (saslauthd)");
    }
  
    upw = getuserpw(login);
  
    if (upw == 0) {
	if (flags & VERBOSE) {
	    syslog(LOG_DEBUG, "auth_shadow: getuserpw(%s) failed: %m",
		   login);
	}
	RETURN("NO Invalid username");
    }
  
    if (strcmp(upw->upw_passwd, crypt(password, upw->upw_passwd)) != 0) {
	if (flags & VERBOSE) {
	    syslog(LOG_DEBUG, "auth_shadow: pw mismatch: %s != %s",
		   password, upw->upw_passwd);
	}
	RETURN("NO Incorrect password");
    }

    RETURN("OK");
  
# else /* HAVE_GETUSERPW */

#  error "unknown shadow authentication type"

# endif /* ! HAVE_GETUSERPW */
}