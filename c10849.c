static int scsi_disk_check_mode_select(SCSIDiskState *s, int page,
                                       uint8_t *inbuf, int inlen)
{
    uint8_t mode_current[SCSI_MAX_MODE_LEN];
    uint8_t mode_changeable[SCSI_MAX_MODE_LEN];
    uint8_t *p;
    int len, expected_len, changeable_len, i;

    /* The input buffer does not include the page header, so it is
     * off by 2 bytes.
     */
    expected_len = inlen + 2;
    if (expected_len > SCSI_MAX_MODE_LEN) {
        return -1;
    }

    p = mode_current;
    memset(mode_current, 0, inlen + 2);
    len = mode_sense_page(s, page, &p, 0);
    if (len < 0 || len != expected_len) {
        return -1;
    }

    p = mode_changeable;
    memset(mode_changeable, 0, inlen + 2);
    changeable_len = mode_sense_page(s, page, &p, 1);
    assert(changeable_len == len);

    /* Check that unchangeable bits are the same as what MODE SENSE
     * would return.
     */
    for (i = 2; i < len; i++) {
        if (((mode_current[i] ^ inbuf[i - 2]) & ~mode_changeable[i]) != 0) {
            return -1;
        }
    }
    return 0;
}