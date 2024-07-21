int	tfm_load_file(const char *filename, TFMInfo *info)
{
	int	lf, lh, bc, ec, nw, nh, nd, ne;
	int	i, n;
	Uchar	*tfm;
	Uchar	*ptr;
	struct stat st;
	int	size;
	FILE	*in;
	Int32	*cb;
	Int32	*charinfo;
	Int32	*widths;
	Int32	*heights;
	Int32	*depths;
	Uint32	checksum;

	in = fopen(filename, "rb");
	if(in == NULL)
		return -1;
	tfm = NULL;

	DEBUG((DBG_FONTS, "(mt) reading TFM file `%s'\n",
		filename));
	/* We read the entire TFM file into core */
	if(fstat(fileno(in), &st) < 0)
		return -1;
	if(st.st_size == 0)
		goto bad_tfm;

	/* allocate a word-aligned buffer to hold the file */
	size = 4 * ROUND(st.st_size, 4);
	if(size != st.st_size)
		mdvi_warning(_("Warning: TFM file `%s' has suspicious size\n"), 
			     filename);
	tfm = (Uchar *)mdvi_malloc(size);
	if(fread(tfm, st.st_size, 1, in) != 1)
		goto error;
	/* we don't need this anymore */
	fclose(in);
	in = NULL;

	/* not a checksum, but serves a similar purpose */
	checksum = 0;
	
	ptr = tfm;
	/* get the counters */
	lf = muget2(ptr);
	lh = muget2(ptr); checksum += 6 + lh;
	bc = muget2(ptr); 
	ec = muget2(ptr); checksum += ec - bc + 1;
	nw = muget2(ptr); checksum += nw;
	nh = muget2(ptr); checksum += nh;
	nd = muget2(ptr); checksum += nd;
	checksum += muget2(ptr); /* skip italics correction count */
	checksum += muget2(ptr); /* skip lig/kern table size */
	checksum += muget2(ptr); /* skip kern table size */
	ne = muget2(ptr); checksum += ne;
	checksum += muget2(ptr); /* skip # of font parameters */

	size = ec - bc + 1;
	cb = (Int32 *)tfm; cb += 6 + lh;
	charinfo    = cb;  cb += size;
	widths      = cb;  cb += nw;
	heights     = cb;  cb += nh;
	depths      = cb;

	if(widths[0] || heights[0] || depths[0] || 
	   checksum != lf || bc - 1 > ec || ec > 255 || ne > 256)
		goto bad_tfm;

	/* from this point on, no error checking is done */

	/* now we're at the header */
	/* get the checksum */
	info->checksum = muget4(ptr);
	/* get the design size */
	info->design = muget4(ptr);
	/* get the coding scheme */
	if(lh > 2) {
		/* get the coding scheme */
		i = n = msget1(ptr);
		if(n < 0 || n > 39) {
			mdvi_warning(_("%s: font coding scheme truncated to 40 bytes\n"),
				     filename);
			n = 39;
		}
		memcpy(info->coding, ptr, n);
		info->coding[n] = 0;
		ptr += i;
	} else
		strcpy(info->coding, "FontSpecific");
	/* get the font family */
	if(lh > 12) {
		n = msget1(ptr);
		if(n > 0) {
			i = Max(n, 63);
			memcpy(info->family, ptr, i);
			info->family[i] = 0;
		} else
			strcpy(info->family, "unspecified");
		ptr += n;
	}
	/* now we don't read from `ptr' anymore */
	
	info->loc = bc;
	info->hic = ec;
	info->type = DviFontTFM;

	/* allocate characters */
	info->chars = xnalloc(TFMChar, size);


#ifdef WORD_LITTLE_ENDIAN
	/* byte-swap the three arrays at once (they are consecutive in memory) */
	swap_array((Uint32 *)widths, nw + nh + nd);
#endif

	/* get the relevant data */
	ptr = (Uchar *)charinfo;
	for(i = bc; i <= ec; ptr += 3, i++) {
		int	ndx;

		ndx = (int)*ptr; ptr++;
		info->chars[i-bc].advance = widths[ndx];
		/* TFM files lack this information */
		info->chars[i-bc].left = 0;
		info->chars[i-bc].right = widths[ndx];
		info->chars[i-bc].present = (ndx != 0);
		if(ndx) {
			ndx = ((*ptr >> 4) & 0xf);
			info->chars[i-bc].height = heights[ndx];
			ndx = (*ptr & 0xf);
			info->chars[i-bc].depth = depths[ndx];
		}
	}

	/* free everything */
	mdvi_free(tfm);
	
	return 0;

bad_tfm:
	mdvi_error(_("%s: File corrupted, or not a TFM file\n"), filename);
error:
	if(tfm) mdvi_free(tfm);
	if(in)  fclose(in);
	return -1;	
}