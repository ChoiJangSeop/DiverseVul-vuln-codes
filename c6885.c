static int comparedictkey(i_ctx_t * i_ctx_p, ref *CIEdict1, ref *CIEdict2, char *key)
{
    int code, code1;
    ref *tempref1, *tempref2;

    code = dict_find_string(CIEdict1, key, &tempref1);
    code1 = dict_find_string(CIEdict2, key, &tempref2);
    if (code != code1)
        return 0;

    if (code <= 0)
        return 1;

    if (r_type(tempref1) != r_type(tempref2))
        return 0;

    if (r_type(tempref1) == t_null)
        return 1;

    return comparearrays(i_ctx_p, tempref1, tempref2);
}