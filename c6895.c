zfont_info(gs_font *font, const gs_point *pscale, int members,
           gs_font_info_t *info)
{
    int code = gs_default_font_info(font, pscale, members &
                    ~(FONT_INFO_COPYRIGHT | FONT_INFO_NOTICE |
                      FONT_INFO_FAMILY_NAME | FONT_INFO_FULL_NAME),
                                    info);
    const ref *pfdict;
    ref *pfontinfo, *pvalue;

    if (code < 0)
        return code;
    pfdict = &pfont_data(font)->dict;
    if (dict_find_string(pfdict, "FontInfo", &pfontinfo) <= 0 ||
        !r_has_type(pfontinfo, t_dictionary))
        return 0;
    if ((members & FONT_INFO_COPYRIGHT) &&
        zfont_info_has(pfontinfo, "Copyright", &info->Copyright))
        info->members |= FONT_INFO_COPYRIGHT;
    if ((members & FONT_INFO_NOTICE) &&
        zfont_info_has(pfontinfo, "Notice", &info->Notice))
        info->members |= FONT_INFO_NOTICE;
    if ((members & FONT_INFO_FAMILY_NAME) &&
        zfont_info_has(pfontinfo, "FamilyName", &info->FamilyName))
        info->members |= FONT_INFO_FAMILY_NAME;
    if ((members & FONT_INFO_FULL_NAME) &&
        zfont_info_has(pfontinfo, "FullName", &info->FullName))
        info->members |= FONT_INFO_FULL_NAME;
    if ((members & FONT_INFO_EMBEDDING_RIGHTS)
        && (dict_find_string(pfontinfo, "FSType", &pvalue) > 0)) {
        info->EmbeddingRights = pvalue->value.intval;
        info->members |= FONT_INFO_EMBEDDING_RIGHTS;
    }
    return code;
}