bool StringMatching::matchString(const char* str1, const char* str2)
{
	std::string path(str1);
	std::string spec(str2);

	std::string base_path(path);
	std::string base_spec(spec);

	replace_all(base_spec, "*", ".*");
	replace_all(base_spec, "?", ".");

	std::regex base_spec_regex(base_spec);
	std::smatch base_spec_match;
	if (std::regex_match(path, base_spec_match, base_spec_regex))
	{
		return true;
	}

	replace_all(base_path, "*", ".*");
	replace_all(base_path, "?", ".");

	std::regex base_path_regex(base_path);
	std::smatch base_path_match;

	if (std::regex_match(spec, base_path_match, base_path_regex))
	{
		return true;
	}

	return false;
}