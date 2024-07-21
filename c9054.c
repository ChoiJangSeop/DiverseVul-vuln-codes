f_py3eval(typval_T *argvars, typval_T *rettv)
{
    char_u	*str;
    char_u	buf[NUMBUFLEN];

    if (p_pyx == 0)
	p_pyx = 3;

    str = tv_get_string_buf(&argvars[0], buf);
    do_py3eval(str, rettv);
}