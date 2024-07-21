static void cgi_web_auth(void)
{
	const char *user = getenv("REMOTE_USER");
	struct passwd *pwd;
	const char *head = "Content-Type: text/html\r\n\r\n<HTML><BODY><H1>SWAT installation Error</H1>\n";
	const char *tail = "</BODY></HTML>\r\n";

	if (!user) {
		printf("%sREMOTE_USER not set. Not authenticated by web server.<br>%s\n",
		       head, tail);
		exit(0);
	}

	pwd = Get_Pwnam_alloc(talloc_tos(), user);
	if (!pwd) {
		printf("%sCannot find user %s<br>%s\n", head, user, tail);
		exit(0);
	}

	C_user = SMB_STRDUP(user);

	if (!setuid(0)) {
		C_pass = secrets_fetch_generic("root", "SWAT");
		if (C_pass == NULL) {
			char *tmp_pass = NULL;
			tmp_pass = generate_random_password(talloc_tos(),
							    16, 16);
			if (tmp_pass == NULL) {
				printf("%sFailed to create random nonce for "
				       "SWAT session\n<br>%s\n", head, tail);
				exit(0);
			}
			secrets_store_generic("root", "SWAT", tmp_pass);
			C_pass = SMB_STRDUP(tmp_pass);
			TALLOC_FREE(tmp_pass);
		}
	}
	setuid(pwd->pw_uid);
	if (geteuid() != pwd->pw_uid || getuid() != pwd->pw_uid) {
		printf("%sFailed to become user %s - uid=%d/%d<br>%s\n", 
		       head, user, (int)geteuid(), (int)getuid(), tail);
		exit(0);
	}
	TALLOC_FREE(pwd);
}