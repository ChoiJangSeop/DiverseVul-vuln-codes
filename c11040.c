ex_function(exarg_T *eap)
{
    char_u *line_to_free = NULL;

    (void)define_function(eap, NULL, &line_to_free);
    vim_free(line_to_free);
}