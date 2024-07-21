cdf_file_property_info(struct magic_set *ms, const cdf_property_info_t *info,
    size_t count)
{
        size_t i;
        cdf_timestamp_t tp;
        struct timespec ts;
        char buf[64];
        const char *str = NULL;
        const char *s;
        int len;

        for (i = 0; i < count; i++) {
                cdf_print_property_name(buf, sizeof(buf), info[i].pi_id);
                switch (info[i].pi_type) {
                case CDF_NULL:
                        break;
                case CDF_SIGNED16:
                        if (NOTMIME(ms) && file_printf(ms, ", %s: %hd", buf,
                            info[i].pi_s16) == -1)
                                return -1;
                        break;
                case CDF_SIGNED32:
                        if (NOTMIME(ms) && file_printf(ms, ", %s: %d", buf,
                            info[i].pi_s32) == -1)
                                return -1;
                        break;
                case CDF_UNSIGNED32:
                        if (NOTMIME(ms) && file_printf(ms, ", %s: %u", buf,
                            info[i].pi_u32) == -1)
                                return -1;
                        break;
                case CDF_LENGTH32_STRING:
                case CDF_LENGTH32_WSTRING:
                        len = info[i].pi_str.s_len;
                        if (len > 1) {
                                char vbuf[1024];
                                size_t j, k = 1;

                                if (info[i].pi_type == CDF_LENGTH32_WSTRING)
                                    k++;
                                s = info[i].pi_str.s_buf;
                                for (j = 0; j < sizeof(vbuf) && len--;
                                    j++, s += k) {
                                        if (*s == '\0')
                                                break;
                                        if (isprint((unsigned char)*s))
                                                vbuf[j] = *s;
                                }
                                if (j == sizeof(vbuf))
                                        --j;
                                vbuf[j] = '\0';
                                if (NOTMIME(ms)) {
                                        if (vbuf[0]) {
                                                if (file_printf(ms, ", %s: %s",
                                                    buf, vbuf) == -1)
                                                        return -1;
                                        }
                                } else if (info[i].pi_id ==
                                        CDF_PROPERTY_NAME_OF_APPLICATION) {
                                        if (strstr(vbuf, "Word"))
                                                str = "msword";
                                        else if (strstr(vbuf, "Excel"))
                                                str = "vnd.ms-excel";
                                        else if (strstr(vbuf, "Powerpoint"))
                                                str = "vnd.ms-powerpoint";
                                        else if (strstr(vbuf,
                                            "Crystal Reports"))
                                                str = "x-rpt";
                                }
                        }
                        break;
                case CDF_FILETIME:
                        tp = info[i].pi_tp;
                        if (tp != 0) {
                                if (tp < 1000000000000000LL) {
                                        char tbuf[64];
                                        cdf_print_elapsed_time(tbuf,
                                            sizeof(tbuf), tp);
                                        if (NOTMIME(ms) && file_printf(ms,
                                            ", %s: %s", buf, tbuf) == -1)
                                                return -1;
                                } else {
                                        char *c, *ec;
                                        cdf_timestamp_to_timespec(&ts, tp);
                                        c = cdf_ctime(&ts.tv_sec);
                                        if ((ec = strchr(c, '\n')) != NULL)
                                                *ec = '\0';

                                        if (NOTMIME(ms) && file_printf(ms,
                                            ", %s: %s", buf, c) == -1)
                                                return -1;
                                }
                        }
                        break;
                case CDF_CLIPBOARD:
                        break;
                default:
                        return -1;
                }
        }
        if (!NOTMIME(ms)) {
		if (str == NULL)
			return 0;
                if (file_printf(ms, "application/%s", str) == -1)
                        return -1;
        }
        return 1;
}