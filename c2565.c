static int pack_ber_user(enum response_types response_type,
                         const char *domain_name, const char *user_name,
                         uid_t uid, gid_t gid,
                         const char *gecos, const char *homedir,
                         const char *shell, struct sss_nss_kv *kv_list,
                         struct berval **berval)
{
    BerElement *ber = NULL;
    int ret;
    size_t ngroups;
    gid_t *groups = NULL;
    size_t buf_len;
    char *buf = NULL;
    struct group grp;
    struct group *grp_result;
    size_t c;
    char *locat;
    char *short_user_name = NULL;

    short_user_name = strdup(user_name);
    if ((locat = strchr(short_user_name, SSSD_DOMAIN_SEPARATOR)) != NULL) {
        if (strcasecmp(locat+1, domain_name) == 0  ) {
            locat[0] = '\0';
        } else {
            ret = LDAP_NO_SUCH_OBJECT;
            goto done;
        }
    }

    ber = ber_alloc_t( LBER_USE_DER );
    if (ber == NULL) {
        ret = LDAP_OPERATIONS_ERROR;
        goto done;
    }

    ret = ber_printf(ber,"{e{ssii", response_type, domain_name, short_user_name,
                                      uid, gid);
    if (ret == -1) {
        ret = LDAP_OPERATIONS_ERROR;
        goto done;
    }

    if (response_type == RESP_USER_GROUPLIST) {
        ret = get_user_grouplist(user_name, gid, &ngroups, &groups);
        if (ret != LDAP_SUCCESS) {
            goto done;
        }

        ret = get_buffer(&buf_len, &buf);
        if (ret != LDAP_SUCCESS) {
            goto done;
        }

        ret = ber_printf(ber,"sss", gecos, homedir, shell);
        if (ret == -1) {
            ret = LDAP_OPERATIONS_ERROR;
            goto done;
        }

        ret = ber_printf(ber,"{");
        if (ret == -1) {
            ret = LDAP_OPERATIONS_ERROR;
            goto done;
        }

        for (c = 0; c < ngroups; c++) {
            ret = getgrgid_r(groups[c], &grp, buf, buf_len, &grp_result);
            if (ret != 0) {
                ret = LDAP_NO_SUCH_OBJECT;
                goto done;
            }
            if (grp_result == NULL) {
                ret = LDAP_NO_SUCH_OBJECT;
                goto done;
            }

            ret = ber_printf(ber, "s", grp.gr_name);
            if (ret == -1) {
                ret = LDAP_OPERATIONS_ERROR;
                goto done;
            }
        }

        ret = ber_printf(ber,"}");
        if (ret == -1) {
            ret = LDAP_OPERATIONS_ERROR;
            goto done;
        }

        if (kv_list != NULL) {
            ret = add_kv_list(ber, kv_list);
            if (ret != LDAP_SUCCESS) {
                goto done;
            }
        }
    }

    ret = ber_printf(ber,"}}");
    if (ret == -1) {
        ret = LDAP_OPERATIONS_ERROR;
        goto done;
    }

    ret = ber_flatten(ber, berval);
    if (ret == -1) {
        ret = LDAP_OPERATIONS_ERROR;
        goto done;
    }

    ret = LDAP_SUCCESS;
done:
    free(short_user_name);
    free(groups);
    free(buf);
    ber_free(ber, 1);
    return ret;
}