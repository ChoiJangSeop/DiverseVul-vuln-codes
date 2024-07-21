static int handle_uid_request(enum request_types request_type, uid_t uid,
                              const char *domain_name, struct berval **berval)
{
    int ret;
    struct passwd pwd;
    struct passwd *pwd_result = NULL;
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
        ret = sss_nss_getsidbyid(uid, &sid_str, &id_type);
        if (ret != 0 || !(id_type == SSS_ID_TYPE_UID
                            || id_type == SSS_ID_TYPE_BOTH)) {
            if (ret == ENOENT) {
                ret = LDAP_NO_SUCH_OBJECT;
            } else {
                ret = LDAP_OPERATIONS_ERROR;
            }
            goto done;
        }

        ret = pack_ber_sid(sid_str, berval);
    } else {
        ret = getpwuid_r(uid, &pwd, buf, buf_len, &pwd_result);
        if (ret != 0) {
            ret = LDAP_NO_SUCH_OBJECT;
            goto done;
        }
        if (pwd_result == NULL) {
            ret = LDAP_NO_SUCH_OBJECT;
            goto done;
        }

        if (request_type == REQ_FULL_WITH_GROUPS) {
            ret = sss_nss_getorigbyname(pwd.pw_name, &kv_list, &id_type);
            if (ret != 0 || !(id_type == SSS_ID_TYPE_UID
                                || id_type == SSS_ID_TYPE_BOTH)) {
                if (ret == ENOENT) {
                    ret = LDAP_NO_SUCH_OBJECT;
                } else {
                    ret = LDAP_OPERATIONS_ERROR;
                }
                goto done;
            }
        }

        ret = pack_ber_user((request_type == REQ_FULL ? RESP_USER
                                                      : RESP_USER_GROUPLIST),
                            domain_name, pwd.pw_name, pwd.pw_uid,
                            pwd.pw_gid, pwd.pw_gecos, pwd.pw_dir,
                            pwd.pw_shell, kv_list, berval);
    }

done:
    sss_nss_free_kv(kv_list);
    free(sid_str);
    free(buf);
    return ret;
}