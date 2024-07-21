static int handle_name_request(enum request_types request_type,
                               const char *name, const char *domain_name,
                               struct berval **berval)
{
    int ret;
    char *fq_name = NULL;
    struct passwd pwd;
    struct passwd *pwd_result = NULL;
    struct group grp;
    struct group *grp_result = NULL;
    char *sid_str = NULL;
    enum sss_id_type id_type;
    size_t buf_len;
    char *buf = NULL;
    struct sss_nss_kv *kv_list = NULL;

    ret = asprintf(&fq_name, "%s%c%s", name, SSSD_DOMAIN_SEPARATOR,
                                       domain_name);
    if (ret == -1) {
        ret = LDAP_OPERATIONS_ERROR;
        fq_name = NULL; /* content is undefined according to
                           asprintf(3) */
        goto done;
    }

    if (request_type == REQ_SIMPLE) {
        ret = sss_nss_getsidbyname(fq_name, &sid_str, &id_type);
        if (ret != 0) {
            if (ret == ENOENT) {
                ret = LDAP_NO_SUCH_OBJECT;
            } else {
                ret = LDAP_OPERATIONS_ERROR;
            }
            goto done;
        }

        ret = pack_ber_sid(sid_str, berval);
    } else {
        ret = get_buffer(&buf_len, &buf);
        if (ret != LDAP_SUCCESS) {
            goto done;
        }

        ret = getpwnam_r(fq_name, &pwd, buf, buf_len, &pwd_result);
        if (ret != 0) {
            /* according to the man page there are a couple of error codes
             * which can indicate that the user was not found. To be on the
             * safe side we fail back to the group lookup on all errors. */
            pwd_result = NULL;
        }

        if (pwd_result != NULL) {
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
        } else { /* no user entry found */
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
        }
    }

done:
    sss_nss_free_kv(kv_list);
    free(fq_name);
    free(sid_str);
    free(buf);

    return ret;
}