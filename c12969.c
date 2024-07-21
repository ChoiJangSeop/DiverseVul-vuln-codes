static int ensure_valid_ownership(const char *path)
{
	struct safe_directory_data data = { .path = path };

	if (!git_env_bool("GIT_TEST_ASSUME_DIFFERENT_OWNER", 0) &&
	    is_path_owned_by_current_user(path))
		return 1;

	read_very_early_config(safe_directory_cb, &data);

	return data.is_safe;
}