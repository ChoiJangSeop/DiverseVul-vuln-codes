Perl_my_setenv(pTHX_ const char *nam, const char *val)
{
  dVAR;
#ifdef __amigaos4__
  amigaos4_obtain_environ(__FUNCTION__);
#endif
#ifdef USE_ITHREADS
  /* only parent thread can modify process environment */
  if (PL_curinterp == aTHX)
#endif
  {
#ifndef PERL_USE_SAFE_PUTENV
    if (!PL_use_safe_putenv) {
        /* most putenv()s leak, so we manipulate environ directly */
        I32 i;
        const I32 len = strlen(nam);
        int nlen, vlen;

        /* where does it go? */
        for (i = 0; environ[i]; i++) {
            if (strnEQ(environ[i],nam,len) && environ[i][len] == '=')
                break;
        }

        if (environ == PL_origenviron) {   /* need we copy environment? */
            I32 j;
            I32 max;
            char **tmpenv;

            max = i;
            while (environ[max])
                max++;
            tmpenv = (char**)safesysmalloc((max+2) * sizeof(char*));
            for (j=0; j<max; j++) {         /* copy environment */
                const int len = strlen(environ[j]);
                tmpenv[j] = (char*)safesysmalloc((len+1)*sizeof(char));
                Copy(environ[j], tmpenv[j], len+1, char);
            }
            tmpenv[max] = NULL;
            environ = tmpenv;               /* tell exec where it is now */
        }
        if (!val) {
            safesysfree(environ[i]);
            while (environ[i]) {
                environ[i] = environ[i+1];
                i++;
            }
#ifdef __amigaos4__
            goto my_setenv_out;
#else
            return;
#endif
        }
        if (!environ[i]) {                 /* does not exist yet */
            environ = (char**)safesysrealloc(environ, (i+2) * sizeof(char*));
            environ[i+1] = NULL;    /* make sure it's null terminated */
        }
        else
            safesysfree(environ[i]);
        nlen = strlen(nam);
        vlen = strlen(val);

        environ[i] = (char*)safesysmalloc((nlen+vlen+2) * sizeof(char));
        /* all that work just for this */
        my_setenv_format(environ[i], nam, nlen, val, vlen);
    } else {
# endif
    /* This next branch should only be called #if defined(HAS_SETENV), but
       Configure doesn't test for that yet.  For Solaris, setenv() and unsetenv()
       were introduced in Solaris 9, so testing for HAS UNSETENV is sufficient.
    */
#   if defined(__CYGWIN__)|| defined(__SYMBIAN32__) || defined(__riscos__) || (defined(__sun) && defined(HAS_UNSETENV)) || defined(PERL_DARWIN)
#       if defined(HAS_UNSETENV)
        if (val == NULL) {
            (void)unsetenv(nam);
        } else {
            (void)setenv(nam, val, 1);
        }
#       else /* ! HAS_UNSETENV */
        (void)setenv(nam, val, 1);
#       endif /* HAS_UNSETENV */
#   elif defined(HAS_UNSETENV)
        if (val == NULL) {
            if (environ) /* old glibc can crash with null environ */
                (void)unsetenv(nam);
        } else {
	    const int nlen = strlen(nam);
	    const int vlen = strlen(val);
	    char * const new_env =
                (char*)safesysmalloc((nlen + vlen + 2) * sizeof(char));
            my_setenv_format(new_env, nam, nlen, val, vlen);
            (void)putenv(new_env);
        }
#   else /* ! HAS_UNSETENV */
        char *new_env;
	const int nlen = strlen(nam);
	int vlen;
        if (!val) {
	   val = "";
        }
        vlen = strlen(val);
        new_env = (char*)safesysmalloc((nlen + vlen + 2) * sizeof(char));
        /* all that work just for this */
        my_setenv_format(new_env, nam, nlen, val, vlen);
        (void)putenv(new_env);
#   endif /* __CYGWIN__ */
#ifndef PERL_USE_SAFE_PUTENV
    }
#endif
  }
#ifdef __amigaos4__
my_setenv_out:
  amigaos4_release_environ(__FUNCTION__);
#endif
}