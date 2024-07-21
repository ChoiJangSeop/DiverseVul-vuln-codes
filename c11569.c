find_sig8_target_as_global_offset(Dwarf_Attribute attr,
    Dwarf_Sig8  *sig8,
    Dwarf_Bool  *is_info,
    Dwarf_Off   *targoffset,
    Dwarf_Error *error)
{
    Dwarf_Die  targdie = 0;
    Dwarf_Bool targ_is_info = 0;
    Dwarf_Off  localoff = 0;
    int res = 0;

    targ_is_info = attr->ar_cu_context->cc_is_info;
    memcpy(sig8,attr->ar_debug_ptr,sizeof(*sig8));
    res = dwarf_find_die_given_sig8(attr->ar_dbg,
        sig8,&targdie,&targ_is_info,error);
    if (res != DW_DLV_OK) {
        return res;
    }
    res = dwarf_die_offsets(targdie,targoffset,&localoff,error);
    if (res != DW_DLV_OK) {
        dwarf_dealloc_die(targdie);
        return res;
    }
    *is_info = targdie->di_cu_context->cc_is_info;
    dwarf_dealloc_die(targdie);
    return DW_DLV_OK;
}