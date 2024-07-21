static int iccdomain(i_ctx_t * i_ctx_p, ref *space, float *ptr)
{
    int components, i, code = 0;
    ref *tempref, ICCdict, valref;

    code = array_get(imemory, space, 1, &ICCdict);
    if (code < 0)
        return code;
    code = dict_find_string(&ICCdict, "N", &tempref);
    if (code < 0)
        return code;
    if (code == 0)
        return gs_note_error(gs_error_undefined);
    components = tempref->value.intval;
    code = dict_find_string(&ICCdict, "Range", &tempref);
    if (code > 0 && !r_has_type(tempref, t_null)) {
        for (i=0;i<components * 2;i++) {
            code = array_get(imemory, tempref, i, &valref);
            if (code < 0)
                return code;
            if (r_has_type(&valref, t_integer))
                ptr[i * 2] = (float)valref.value.intval;
            else
                ptr[i * 2] = valref.value.realval;
        }
    } else {
        for (i=0;i<components;i++) {
            ptr[i * 2] = 0;
            ptr[(i * 2) + 1] = 1;
        }
    }
    return 0;
}