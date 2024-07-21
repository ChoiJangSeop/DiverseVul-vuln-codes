identify_system_timezone(void)
{
	int			i;
	char		tzname[128];
	char		localtzname[256];
	time_t		t = time(NULL);
	struct tm  *tm = localtime(&t);
	HKEY		rootKey;
	int			idx;

	if (!tm)
	{
#ifdef DEBUG_IDENTIFY_TIMEZONE
		fprintf(stderr, "could not identify system time zone: localtime() failed\n");
#endif
		return NULL;			/* go to GMT */
	}

	memset(tzname, 0, sizeof(tzname));
	strftime(tzname, sizeof(tzname) - 1, "%Z", tm);

	for (i = 0; win32_tzmap[i].stdname != NULL; i++)
	{
		if (strcmp(tzname, win32_tzmap[i].stdname) == 0 ||
			strcmp(tzname, win32_tzmap[i].dstname) == 0)
		{
#ifdef DEBUG_IDENTIFY_TIMEZONE
			fprintf(stderr, "TZ \"%s\" matches system time zone \"%s\"\n",
					win32_tzmap[i].pgtzname, tzname);
#endif
			return win32_tzmap[i].pgtzname;
		}
	}

	/*
	 * Localized Windows versions return localized names for the timezone.
	 * Scan the registry to find the English name, and then try matching
	 * against our table again.
	 */
	memset(localtzname, 0, sizeof(localtzname));
	if (RegOpenKeyEx(HKEY_LOCAL_MACHINE,
			   "SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Time Zones",
					 0,
					 KEY_READ,
					 &rootKey) != ERROR_SUCCESS)
	{
#ifdef DEBUG_IDENTIFY_TIMEZONE
		fprintf(stderr, "could not open registry key to identify system time zone: error code %lu\n",
				GetLastError());
#endif
		return NULL;			/* go to GMT */
	}

	for (idx = 0;; idx++)
	{
		char		keyname[256];
		char		zonename[256];
		DWORD		namesize;
		FILETIME	lastwrite;
		HKEY		key;
		LONG		r;

		memset(keyname, 0, sizeof(keyname));
		namesize = sizeof(keyname);
		if ((r = RegEnumKeyEx(rootKey,
							  idx,
							  keyname,
							  &namesize,
							  NULL,
							  NULL,
							  NULL,
							  &lastwrite)) != ERROR_SUCCESS)
		{
			if (r == ERROR_NO_MORE_ITEMS)
				break;
#ifdef DEBUG_IDENTIFY_TIMEZONE
			fprintf(stderr, "could not enumerate registry subkeys to identify system time zone: %d\n",
					(int) r);
#endif
			break;
		}

		if ((r = RegOpenKeyEx(rootKey, keyname, 0, KEY_READ, &key)) != ERROR_SUCCESS)
		{
#ifdef DEBUG_IDENTIFY_TIMEZONE
			fprintf(stderr, "could not open registry subkey to identify system time zone: %d\n",
					(int) r);
#endif
			break;
		}

		memset(zonename, 0, sizeof(zonename));
		namesize = sizeof(zonename);
		if ((r = RegQueryValueEx(key, "Std", NULL, NULL, (unsigned char *) zonename, &namesize)) != ERROR_SUCCESS)
		{
#ifdef DEBUG_IDENTIFY_TIMEZONE
			fprintf(stderr, "could not query value for key \"std\" to identify system time zone \"%s\": %d\n",
					keyname, (int) r);
#endif
			RegCloseKey(key);
			continue;			/* Proceed to look at the next timezone */
		}
		if (strcmp(tzname, zonename) == 0)
		{
			/* Matched zone */
			strcpy(localtzname, keyname);
			RegCloseKey(key);
			break;
		}
		memset(zonename, 0, sizeof(zonename));
		namesize = sizeof(zonename);
		if ((r = RegQueryValueEx(key, "Dlt", NULL, NULL, (unsigned char *) zonename, &namesize)) != ERROR_SUCCESS)
		{
#ifdef DEBUG_IDENTIFY_TIMEZONE
			fprintf(stderr, "could not query value for key \"dlt\" to identify system time zone \"%s\": %d\n",
					keyname, (int) r);
#endif
			RegCloseKey(key);
			continue;			/* Proceed to look at the next timezone */
		}
		if (strcmp(tzname, zonename) == 0)
		{
			/* Matched DST zone */
			strcpy(localtzname, keyname);
			RegCloseKey(key);
			break;
		}

		RegCloseKey(key);
	}

	RegCloseKey(rootKey);

	if (localtzname[0])
	{
		/* Found a localized name, so scan for that one too */
		for (i = 0; win32_tzmap[i].stdname != NULL; i++)
		{
			if (strcmp(localtzname, win32_tzmap[i].stdname) == 0 ||
				strcmp(localtzname, win32_tzmap[i].dstname) == 0)
			{
#ifdef DEBUG_IDENTIFY_TIMEZONE
				fprintf(stderr, "TZ \"%s\" matches localized system time zone \"%s\" (\"%s\")\n",
						win32_tzmap[i].pgtzname, tzname, localtzname);
#endif
				return win32_tzmap[i].pgtzname;
			}
		}
	}

#ifdef DEBUG_IDENTIFY_TIMEZONE
	fprintf(stderr, "could not find a match for system time zone \"%s\"\n",
			tzname);
#endif
	return NULL;				/* go to GMT */
}