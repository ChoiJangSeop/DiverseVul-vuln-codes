fixExec2Error(int action,
             u_char * var_val,
             u_char var_val_type,
             size_t var_val_len,
             u_char * statP, oid * name, size_t name_len)
{
    netsnmp_old_extend *exten = NULL;
    unsigned int idx;

    idx = name[name_len-1] -1;
    exten = &compatability_entries[ idx ];

#ifndef NETSNMP_NO_WRITE_SUPPORT
    switch (action) {
    case MODE_SET_RESERVE1:
        if (var_val_type != ASN_INTEGER) {
            snmp_log(LOG_ERR, "Wrong type != int\n");
            return SNMP_ERR_WRONGTYPE;
        }
        idx = *((long *) var_val);
        if (idx != 1) {
            snmp_log(LOG_ERR, "Wrong value != 1\n");
            return SNMP_ERR_WRONGVALUE;
        }
        if (!exten || !exten->efix_entry) {
            snmp_log(LOG_ERR, "No command to run\n");
            return SNMP_ERR_GENERR;
        }
        return SNMP_ERR_NOERROR;

    case MODE_SET_COMMIT:
        netsnmp_cache_check_and_reload( exten->efix_entry->cache );
    }
#endif /* !NETSNMP_NO_WRITE_SUPPORT */
    return SNMP_ERR_NOERROR;
}