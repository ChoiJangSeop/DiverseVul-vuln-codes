int X509_cmp_time(ASN1_TIME *ctm, time_t *cmp_time)
{
    char *str;
    ASN1_TIME atm;
    long offset;
    char buff1[24], buff2[24], *p;
    int i, j;

    p = buff1;
    i = ctm->length;
    str = (char *)ctm->data;
    if (ctm->type == V_ASN1_UTCTIME) {
        if ((i < 11) || (i > 17))
            return 0;
        memcpy(p, str, 10);
        p += 10;
        str += 10;
    } else {
        if (i < 13)
            return 0;
        memcpy(p, str, 12);
        p += 12;
        str += 12;
    }

    if ((*str == 'Z') || (*str == '-') || (*str == '+')) {
        *(p++) = '0';
        *(p++) = '0';
    } else {
        *(p++) = *(str++);
        *(p++) = *(str++);
        /* Skip any fractional seconds... */
        if (*str == '.') {
            str++;
            while ((*str >= '0') && (*str <= '9'))
                str++;
        }

    }
    *(p++) = 'Z';
    *(p++) = '\0';

    if (*str == 'Z')
        offset = 0;
    else {
        if ((*str != '+') && (*str != '-'))
            return 0;
        offset = ((str[1] - '0') * 10 + (str[2] - '0')) * 60;
        offset += (str[3] - '0') * 10 + (str[4] - '0');
        if (*str == '-')
            offset = -offset;
    }
    atm.type = ctm->type;
    atm.length = sizeof(buff2);
    atm.data = (unsigned char *)buff2;

    if (X509_time_adj(&atm, offset * 60, cmp_time) == NULL)
        return 0;

    if (ctm->type == V_ASN1_UTCTIME) {
        i = (buff1[0] - '0') * 10 + (buff1[1] - '0');
        if (i < 50)
            i += 100;           /* cf. RFC 2459 */
        j = (buff2[0] - '0') * 10 + (buff2[1] - '0');
        if (j < 50)
            j += 100;

        if (i < j)
            return -1;
        if (i > j)
            return 1;
    }
    i = strcmp(buff1, buff2);
    if (i == 0)                 /* wait a second then return younger :-) */
        return -1;
    else
        return i;
}