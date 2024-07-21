set_attribute_9(TERMTYPE2 *tp, int flag)
{
    const char *value;
    char *result;

    value = tparm(set_attributes, 0, 0, 0, 0, 0, 0, 0, 0, flag);
    if (PRESENT(value))
	result = strdup(value);
    else
	result = 0;
    return result;
}