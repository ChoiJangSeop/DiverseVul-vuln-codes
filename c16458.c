static int find_profile_by_num(unsigned profile_num,
			       SRTP_PROTECTION_PROFILE **pptr)
	{
	SRTP_PROTECTION_PROFILE *p;

	p=srtp_known_profiles;
	while(p->name)
		{
		if(p->id == profile_num)
			{
			*pptr=p;
			return 0;
			}
		p++;
		}

	return 1;
	}