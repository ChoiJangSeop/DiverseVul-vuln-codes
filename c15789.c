static char *ask_new_shell(char *question, char *oldshell)
{
	int len;
	char *ans = NULL;
#ifdef HAVE_LIBREADLINE
	rl_attempted_completion_function = shell_name_completion;
#else
	size_t dummy = 0;
#endif
	if (!oldshell)
		oldshell = "";
	printf("%s [%s]:", question, oldshell);
#ifdef HAVE_LIBREADLINE
	if ((ans = readline(" ")) == NULL)
#else
	putchar(' ');
	if (getline(&ans, &dummy, stdin) < 0)
#endif
		return NULL;
	/* remove the newline at the end of ans. */
	ltrim_whitespace((unsigned char *) ans);
	len = rtrim_whitespace((unsigned char *) ans);
	if (len == 0)
		return NULL;
	return ans;
}