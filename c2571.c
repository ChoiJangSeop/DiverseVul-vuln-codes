static int handle_gid_request(enum request_types request_type, gid_t gid,
                              const char *domain_name, struct berval **berval)
{
    int ret;
    struct group grp;
    char *sid_str = NULL;
    enum sss_id_type id_type;
    size_t buf_len;
    char *buf = NULL;
    struct sss_nss_kv *kv_list = NULL;

    ret = get_buffer(&buf_len, &buf);
    if (ret != LDAP_SUCCESS) {
        return ret;
    }

    if (request_type == REQ_SIMPLE) {
        ret = sss_nss_getsidbyid(gid, &sid_str, &id_type);
        if (ret != 0 || id_type != SSS_ID_TYPE_GID) {
            if (ret == ENOENT) {
                ret = LDAP_NO_SUCH_OBJECT;
            } else {
                ret = LDAP_OPERATIONS_ERROR;
            }
            goto done;
        }

        ret = pack_ber_sid(sid_str, berval);
    } else {
        ret = getgrgid_r_wrapper(MAX_BUF, gid, &grp, &buf, &buf_len);
        if (ret != 0) {
            if (ret == ENOMEM || ret == ERANGE) {
                ret = LDAP_OPERATIONS_ERROR;
            } else {
                ret = LDAP_NO_SUCH_OBJECT;
            }
            goto done;
        }

        if (request_type == REQ_FULL_WITH_GROUPS) {
            ret = sss_nss_getorigbyname(grp.gr_name, &kv_list, &id_type);
            if (ret != 0 || !(id_type == SSS_ID_TYPE_GID
                                || id_type == SSS_ID_TYPE_BOTH)) {
                if (ret == ENOENT) {
                    ret = LDAP_NO_SUCH_OBJECT;
                } else {
                    ret = LDAP_OPERATIONS_ERROR;
                }
                goto done;
            }
        }

        ret = pack_ber_group((request_type == REQ_FULL ? RESP_GROUP
                                                       : RESP_GROUP_MEMBERS),
                             domain_name, grp.gr_name, grp.gr_gid,
                             grp.gr_mem, kv_list, berval);
    }

done:
    sss_nss_free_kv(kv_list);
    free(sid_str);
    free(buf);
    return ret;
}