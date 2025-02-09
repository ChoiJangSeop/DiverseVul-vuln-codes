cdf_read_property_info(const cdf_stream_t *sst, const cdf_header_t *h,
    uint32_t offs, cdf_property_info_t **info, size_t *count, size_t *maxcount)
{
	const cdf_section_header_t *shp;
	cdf_section_header_t sh;
	const uint8_t *p, *q, *e;
	size_t i, o4, nelements, j, slen, left;
	cdf_property_info_t *inp;

	if (offs > UINT32_MAX / 4) {
		errno = EFTYPE;
		goto out;
	}
	shp = CAST(const cdf_section_header_t *,
	    cdf_offset(sst->sst_tab, offs));
	if (cdf_check_stream_offset(sst, h, shp, sizeof(*shp), __LINE__) == -1)
		goto out;
	sh.sh_len = CDF_TOLE4(shp->sh_len);
	if (sh.sh_len > CDF_SHLEN_LIMIT) {
		errno = EFTYPE;
		goto out;
	}

	if (cdf_check_stream_offset(sst, h, shp, sh.sh_len, __LINE__) == -1)
		goto out;

	sh.sh_properties = CDF_TOLE4(shp->sh_properties);
	DPRINTF(("section len: %u properties %u\n", sh.sh_len,
	    sh.sh_properties));
	if (sh.sh_properties > CDF_PROP_LIMIT)
		goto out;
	inp = cdf_grow_info(info, maxcount, sh.sh_properties);
	if (inp == NULL)
		goto out;
	inp += *count;
	*count += sh.sh_properties;
	p = CAST(const uint8_t *, cdf_offset(sst->sst_tab, offs + sizeof(sh)));
	e = CAST(const uint8_t *, cdf_offset(shp, sh.sh_len));
	if (p >= e || cdf_check_stream_offset(sst, h, e, 0, __LINE__) == -1)
		goto out;

	for (i = 0; i < sh.sh_properties; i++) {
		if ((q = cdf_get_property_info_pos(sst, h, p, e, i)) == NULL)
			goto out;
		inp[i].pi_id = CDF_GETUINT32(p, i << 1);
		left = CAST(size_t, e - q);
		if (left < sizeof(uint32_t)) {
			DPRINTF(("short info (no type)_\n"));
			goto out;
		}
		inp[i].pi_type = CDF_GETUINT32(q, 0);
		DPRINTF(("%" SIZE_T_FORMAT "u) id=%#x type=%#x offs=%#tx,%#x\n",
		    i, inp[i].pi_id, inp[i].pi_type, q - p, offs));
		if (inp[i].pi_type & CDF_VECTOR) {
			if (left < sizeof(uint32_t) * 2) {
				DPRINTF(("missing CDF_VECTOR length\n"));
				goto out;
			}
			nelements = CDF_GETUINT32(q, 1);
			if (nelements == 0) {
				DPRINTF(("CDF_VECTOR with nelements == 0\n"));
				goto out;
			}
			slen = 2;
		} else {
			nelements = 1;
			slen = 1;
		}
		o4 = slen * sizeof(uint32_t);
		if (inp[i].pi_type & (CDF_ARRAY|CDF_BYREF|CDF_RESERVED))
			goto unknown;
		switch (inp[i].pi_type & CDF_TYPEMASK) {
		case CDF_NULL:
		case CDF_EMPTY:
			break;
		case CDF_SIGNED16:
			if (!cdf_copy_info(&inp[i], &q[o4], e, sizeof(int16_t)))
				goto unknown;
			break;
		case CDF_SIGNED32:
		case CDF_BOOL:
		case CDF_UNSIGNED32:
		case CDF_FLOAT:
			if (!cdf_copy_info(&inp[i], &q[o4], e, sizeof(int32_t)))
				goto unknown;
			break;
		case CDF_SIGNED64:
		case CDF_UNSIGNED64:
		case CDF_DOUBLE:
		case CDF_FILETIME:
			if (!cdf_copy_info(&inp[i], &q[o4], e, sizeof(int64_t)))
				goto unknown;
			break;
		case CDF_LENGTH32_STRING:
		case CDF_LENGTH32_WSTRING:
			if (nelements > 1) {
				size_t nelem = inp - *info;
				inp = cdf_grow_info(info, maxcount, nelements);
				if (inp == NULL)
					goto out;
				inp += nelem;
			}
			DPRINTF(("nelements = %" SIZE_T_FORMAT "u\n",
			    nelements));
			for (j = 0; j < nelements && i < sh.sh_properties;
			    j++, i++)
			{
				uint32_t l;

				if (o4 + sizeof(uint32_t) > left)
					goto out;

				l = CDF_GETUINT32(q, slen);
				o4 += sizeof(uint32_t);
				if (o4 + l > left)
					goto out;

				inp[i].pi_str.s_len = l;
				inp[i].pi_str.s_buf = CAST(const char *,
				    CAST(const void *, &q[o4]));

				DPRINTF(("o=%" SIZE_T_FORMAT "u l=%d(%"
				    SIZE_T_FORMAT "u), t=%" SIZE_T_FORMAT
				    "u s=%s\n", o4, l, CDF_ROUND(l, sizeof(l)),
				    left, inp[i].pi_str.s_buf));

				if (l & 1)
					l++;

				slen += l >> 1;
				o4 = slen * sizeof(uint32_t);
			}
			i--;
			break;
		case CDF_CLIPBOARD:
			if (inp[i].pi_type & CDF_VECTOR)
				goto unknown;
			break;
		default:
		unknown:
			memset(&inp[i].pi_val, 0, sizeof(inp[i].pi_val));
			DPRINTF(("Don't know how to deal with %#x\n",
			    inp[i].pi_type));
			break;
		}
	}
	return 0;
out:
	free(*info);
	*info = NULL;
	*count = 0;
	*maxcount = 0;
	errno = EFTYPE;
	return -1;
}