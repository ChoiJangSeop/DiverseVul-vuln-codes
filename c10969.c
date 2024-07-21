static uint32_t get_requirement_by_name(const char *requirement)
{
	int i;

	for (i = 0; requirements_flags[i].description; i++)
		if (!strcmp(requirement, requirements_flags[i].description))
			return requirements_flags[i].flag;

	return CRYPT_REQUIREMENT_UNKNOWN;
}