static int ssl_ctx_make_profiles(const char *profiles_string,STACK_OF(SRTP_PROTECTION_PROFILE) **out)
	{
	STACK_OF(SRTP_PROTECTION_PROFILE) *profiles;

	char *col;
	char *ptr=(char *)profiles_string;
    
	SRTP_PROTECTION_PROFILE *p;

	if(!(profiles=sk_SRTP_PROTECTION_PROFILE_new_null()))
		{
		SSLerr(SSL_F_SSL_CTX_MAKE_PROFILES, SSL_R_SRTP_COULD_NOT_ALLOCATE_PROFILES);
		return 1;
		}
    
	do
		{
		col=strchr(ptr,':');

		if(!find_profile_by_name(ptr,&p,
					 col ? col-ptr : (int)strlen(ptr)))
			{
			sk_SRTP_PROTECTION_PROFILE_push(profiles,p);
			}
		else
			{
			SSLerr(SSL_F_SSL_CTX_MAKE_PROFILES,SSL_R_SRTP_UNKNOWN_PROTECTION_PROFILE);
			return 1;
			}

		if(col) ptr=col+1;
		} while (col);

	*out=profiles;
    
	return 0;
	}