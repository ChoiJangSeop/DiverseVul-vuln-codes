static int handle_sid_request(enum request_types request_type, const char *sid,
                              struct berval **berval)
{
    int ret;
    struct passwd pwd;
    struct passwd *pwd_result = NULL;
    struct group grp;
    struct group *grp_result = NULL;
    char *domain_name = NULL;
    char *fq_name = NULL;
    char *object_name = NULL;
    char *sep;
    size_t buf_len;
    char *buf = NULL;
    enum sss_id_type id_type;
    struct sss_nss_kv *kv_list = NULL;

    ret = sss_nss_getnamebysid(sid, &fq_name, &id_type);
    if (ret != 0) {
        if (ret == ENOENT) {
            ret = LDAP_NO_SUCH_OBJECT;
        } else {
            ret = LDAP_OPERATIONS_ERROR;
        }
        goto done;
    }

    sep = strchr(fq_name, SSSD_DOMAIN_SEPARATOR);
    if (sep == NULL) {
        ret = LDAP_OPERATIONS_ERROR;
        goto done;
    }

    object_name = strndup(fq_name, (sep - fq_name));
    domain_name = strdup(sep + 1);
    if (object_name == NULL || domain_name == NULL) {
        ret = LDAP_OPERATIONS_ERROR;
        goto done;
    }

    if (request_type == REQ_SIMPLE) {
        ret = pack_ber_name(domain_name, object_name, berval);
        goto done;
    }

    ret = get_buffer(&buf_len, &buf);
    if (ret != LDAP_SUCCESS) {
        goto done;
    }

    switch(id_type) {
    case SSS_ID_TYPE_UID:
    case SSS_ID_TYPE_BOTH:
        ret = getpwnam_r(fq_name, &pwd, buf, buf_len, &pwd_result);
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
        break;
    case SSS_ID_TYPE_GID:
        ret = getgrnam_r(fq_name, &grp, buf, buf_len, &grp_result);
        if (ret != 0) {
            ret = LDAP_NO_SUCH_OBJECT;
            goto done;
        }

        if (grp_result == NULL) {
            ret = LDAP_NO_SUCH_OBJECT;
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
        break;
    default:
        ret = LDAP_OPERATIONS_ERROR;
        goto done;
    }

done:
    sss_nss_free_kv(kv_list);
    free(fq_name);
    free(object_name);
    free(domain_name);
    free(buf);

    return ret;
}