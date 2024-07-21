static int iccompareproc(i_ctx_t *i_ctx_p, ref *space, ref *testspace)
{
    int code1, code2;
    ref ICCdict1, ICCdict2, *tempref1, *tempref2;
    int buff_size;

    code1 = array_get(imemory, space, 1, &ICCdict1);
    if (code1 < 0)
        return 0;
    code2 = array_get(imemory, testspace, 1, &ICCdict2);
    if (code2 < 0)
        return 0;

    /* As a quick check see if current is same as new */
    if (ICCdict1.value.bytes == ICCdict2.value.bytes)
         return 1;

    /* Need to check all the various parts */
    code1 = dict_find_string(&ICCdict1, "N", &tempref1);
    code2 = dict_find_string(&ICCdict2, "N", &tempref2);
    if (code1 != code2)
        return 0;
    if (tempref1->value.intval != tempref2->value.intval)
        return 0;

    if (!comparedictkey(i_ctx_p, &ICCdict1, &ICCdict2, (char *)"Range"))
        return 0;

    code1 = dict_find_string(&ICCdict1, "DataSource", &tempref1);
    if (code1 <= 0)
        return 0;
    code2 = dict_find_string(&ICCdict2, "DataSource", &tempref2);
    if (code2 <= 0)
        return 0;
    if (r_size(tempref1) != r_size(tempref2))
        return 0;

    buff_size = r_size(tempref1);
    if (memcmp(tempref1->value.const_bytes, tempref2->value.const_bytes, buff_size) == 0)
        return 1;
    else
        return 0;
}