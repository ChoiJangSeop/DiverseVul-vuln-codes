ignore_error_for_testing(char_u *error)
{
    if (ignore_error_list.ga_itemsize == 0)
	ga_init2(&ignore_error_list, sizeof(char_u *), 1);

    if (STRCMP("RESET", error) == 0)
	ga_clear_strings(&ignore_error_list);
    else
	ga_add_string(&ignore_error_list, error);
}