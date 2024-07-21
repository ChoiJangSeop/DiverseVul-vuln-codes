f_mzeval(typval_T *argvars, typval_T *rettv)
{
    char_u	*str;
    char_u	buf[NUMBUFLEN];

    str = tv_get_string_buf(&argvars[0], buf);
    do_mzeval(str, rettv);
}