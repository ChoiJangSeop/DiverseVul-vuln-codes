rpmRC hdrblobRead(FD_t fd, int magic, int exact_size, rpmTagVal regionTag, hdrblob blob, char **emsg)
{
    int32_t block[4];
    int32_t *bs = (magic != 0) ? &block[0] : &block[2];
    int blen = (magic != 0) ? sizeof(block) : sizeof(block) / 2;
    int32_t il;
    int32_t dl;
    int32_t * ei = NULL;
    size_t uc;
    size_t nb;
    rpmRC rc = RPMRC_FAIL;		/* assume failure */
    int xx;
    int32_t il_max = HEADER_TAGS_MAX;
    int32_t dl_max = HEADER_DATA_MAX;

    if (regionTag == RPMTAG_HEADERSIGNATURES) {
	il_max = 32;
	dl_max = 64 * 1024 * 1024;
    }

    memset(block, 0, sizeof(block));
    if ((xx = Freadall(fd, bs, blen)) != blen) {
	rasprintf(emsg,
		_("hdr size(%d): BAD, read returned %d"), blen, xx);
	goto exit;
    }
    if (magic && memcmp(block, rpm_header_magic, sizeof(rpm_header_magic))) {
	rasprintf(emsg, _("hdr magic: BAD"));
	goto exit;
    }
    il = ntohl(block[2]);
    if (hdrchkRange(il_max, il)) {
	rasprintf(emsg, _("hdr tags: BAD, no. of tags(%d) out of range"), il);
	goto exit;
    }
    dl = ntohl(block[3]);
    if (hdrchkRange(dl_max, dl)) {
	rasprintf(emsg, _("hdr data: BAD, no. of bytes(%d) out of range"), dl);
	goto exit;
    }

    nb = (il * sizeof(struct entryInfo_s)) + dl;
    uc = sizeof(il) + sizeof(dl) + nb;
    ei = xmalloc(uc);
    ei[0] = block[2];
    ei[1] = block[3];
    if ((xx = Freadall(fd, (char *)&ei[2], nb)) != nb) {
	rasprintf(emsg, _("hdr blob(%zd): BAD, read returned %d"), nb, xx);
	goto exit;
    }

    if (regionTag == RPMTAG_HEADERSIGNATURES) {
	size_t sigSize = uc + sizeof(rpm_header_magic);
	size_t pad = (8 - (sigSize % 8)) % 8;
	size_t trc;
	if (pad && (trc = Freadall(fd, block, pad)) != pad) {
	    rasprintf(emsg, _("sigh pad(%zd): BAD, read %zd bytes"), pad, trc);
	    goto exit;
	}
    }

    rc = hdrblobInit(ei, uc, regionTag, exact_size, blob, emsg);

exit:
    if (rc != RPMRC_OK) {
	free(ei);
	blob->ei = NULL;
	if (emsg && *emsg && regionTag == RPMTAG_HEADERSIGNATURES) {
	    /* rstrscat() cannot handle overlap even if it claims so */
	    char *tmp = rstrscat(NULL, _("signature "), *emsg, NULL);
	    free(*emsg);
	    *emsg = tmp;
	}
    }

    return rc;
}