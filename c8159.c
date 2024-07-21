one_one_mapping(const char *mapping)
{
    bool result = TRUE;

    if (VALID_STRING(mapping)) {
	int n = 0;
	while (mapping[n] != '\0') {
	    if (isLine(mapping[n]) &&
		mapping[n] != mapping[n + 1]) {
		result = FALSE;
		break;
	    }
	    n += 2;
	}
    }
    return result;
}