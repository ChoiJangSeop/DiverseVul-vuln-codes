static int icccomponents(i_ctx_t * i_ctx_p, ref *space, int *n)
{
    int code = 0;
    ref *tempref, ICCdict;

    code = array_get(imemory, space, 1, &ICCdict);
    if (code < 0)
        return code;

    code = dict_find_string(&ICCdict, "N", &tempref);
    if (code < 0)
        return code;
    if (code == 0)
        return gs_note_error(gs_error_undefined);
    *n = tempref->value.intval;
    return 0;
}