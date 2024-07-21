rpmRC headerVerifyRegion(rpmTagVal regionTag,
			struct indexEntry_s *entry, int il, int dl,
			entryInfo pe, unsigned char *dataStart,
			int *rilp, int *rdlp, char **buf)
{
    rpmRC rc = RPMRC_FAIL;
    struct entryInfo_s info;
    unsigned char * regionEnd = NULL;
    int32_t ril = 0;
    int32_t rdl = 0;

    memset(entry, 0, sizeof(*entry));

    /* Check (and convert) the 1st tag element. */
    if (headerVerifyInfo(1, dl, pe, &entry->info, 0) != -1) {
	rasprintf(buf, _("tag[%d]: BAD, tag %d type %d offset %d count %d"),
		0, entry->info.tag, entry->info.type,
		entry->info.offset, entry->info.count);
	goto exit;
    }

    /* Is there an immutable header region tag? */
    if (!(entry->info.tag == regionTag)) {
	rc = RPMRC_NOTFOUND;
	goto exit;
    }

    /* Is the region tag sane? */
    if (!(entry->info.type == REGION_TAG_TYPE &&
	  entry->info.count == REGION_TAG_COUNT)) {
	rasprintf(buf,
		_("region tag: BAD, tag %d type %d offset %d count %d"),
		entry->info.tag, entry->info.type,
		entry->info.offset, entry->info.count);
	goto exit;
    }

    /* Is the trailer within the data area? */
    if (entry->info.offset + REGION_TAG_COUNT > dl) {
	rasprintf(buf, 
		_("region offset: BAD, tag %d type %d offset %d count %d"),
		entry->info.tag, entry->info.type,
		entry->info.offset, entry->info.count);
	goto exit;
    }

    /* Is there an immutable header region tag trailer? */
    memset(&info, 0, sizeof(info));
    regionEnd = dataStart + entry->info.offset;
    (void) memcpy(&info, regionEnd, REGION_TAG_COUNT);
    regionEnd += REGION_TAG_COUNT;
    rdl = regionEnd - dataStart;

    if (headerVerifyInfo(1, il * sizeof(*pe), &info, &entry->info, 1) != -1 ||
	!(entry->info.tag == regionTag
       && entry->info.type == REGION_TAG_TYPE
       && entry->info.count == REGION_TAG_COUNT))
    {
	rasprintf(buf, 
		_("region trailer: BAD, tag %d type %d offset %d count %d"),
		entry->info.tag, entry->info.type,
		entry->info.offset, entry->info.count);
	goto exit;
    }

    /* Is the no. of tags in the region less than the total no. of tags? */
    ril = entry->info.offset/sizeof(*pe);
    if ((entry->info.offset % sizeof(*pe)) || ril > il) {
	rasprintf(buf, _("region size: BAD, ril(%d) > il(%d)"), ril, il);
	goto exit;
    }

    rc = RPMRC_OK;
    if (rilp)
	*rilp = ril;
    if (rdlp)
	*rdlp = rdl;

exit:
    return rc;
}