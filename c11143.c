static char **shell_name_completion(const char *text,
				    int start __attribute__((__unused__)),
				    int end __attribute__((__unused__)))
{
	rl_attempted_completion_over = 1;
	return rl_completion_matches(text, shell_name_generator);
}