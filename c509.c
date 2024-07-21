static int pk_load_font(DviParams *unused, DviFont *font)
{
	int	i;
	int	flag_byte;
	int	loc, hic, maxch;
	Int32	checksum;
	FILE	*p;
#ifndef NODEBUG
	char	s[256];
#endif
	long	alpha, beta, z;

	font->chars = xnalloc(DviFontChar, 256);
	p = font->in;
	memzero(font->chars, 256 * sizeof(DviFontChar));
	for(i = 0; i < 256; i++)
		font->chars[i].offset = 0;

	/* check the preamble */
	loc = fuget1(p); hic = fuget1(p);
	if(loc != PK_PRE || hic != PK_ID)
		goto badpk;
	i = fuget1(p);
#ifndef NODEBUG
	for(loc = 0; loc < i; loc++)
		s[loc] = fuget1(p);
	s[loc] = 0;
	DEBUG((DBG_FONTS, "(pk) %s: %s\n", font->fontname, s));
#else
	fseek(in, (long)i, SEEK_CUR);
#endif
	/* get the design size */
	font->design = fuget4(p);
	/* get the checksum */
	checksum = fuget4(p);
	if(checksum && font->checksum && font->checksum != checksum) {
		mdvi_warning(_("%s: checksum mismatch (expected %u, got %u)\n"),
			     font->fontname, font->checksum, checksum);
	} else if(!font->checksum)
		font->checksum = checksum;
	/* skip pixel per point ratios */
	fuget4(p);
	fuget4(p);
	if(feof(p))
		goto badpk;	

	/* now start reading the font */
	loc = 256; hic = -1; maxch = 256;
	
	/* initialize alpha and beta for TFM width computation */
	TFMPREPARE(font->scale, z, alpha, beta);

	while((flag_byte = fuget1(p)) != PK_POST) {
		if(feof(p))
			break;
		if(flag_byte >= PK_CMD_START) {
			switch(flag_byte) {
			case PK_X1:
			case PK_X2:
			case PK_X3:
			case PK_X4: {
#ifndef NODEBUG
				char	*t;
				int	n;
				
				i = fugetn(p, flag_byte - PK_X1 + 1);
				if(i < 256)
					t = &s[0];
				else
					t = mdvi_malloc(i + 1);
				for(n = 0; n < i; n++)
					t[n] = fuget1(p);
				t[n] = 0;
				DEBUG((DBG_SPECIAL, "(pk) %s: Special \"%s\"\n",
					font->fontname, t));
				if(t != &s[0])
					mdvi_free(t);
#else
				i = fugetn(p, flag_byte - PK_X1 + 1);
				while(i-- > 0)
					fuget1(p);
#endif
				break;
			}
			case PK_Y:
				i = fuget4(p);
				DEBUG((DBG_SPECIAL, "(pk) %s: MF special %u\n",
					font->fontname, (unsigned)i));
				break;
			case PK_POST:
			case PK_NOOP:
				break;
			case PK_PRE:
				mdvi_error(_("%s: unexpected preamble\n"), font->fontname);
				goto error;
			}
		} else {
			int	pl;
			int	cc;
			int	w, h;
			int	x, y;
			int	offset;
			long	tfm;
			
			switch(flag_byte & 0x7) {
			case 7:
				pl = fuget4(p);
				cc = fuget4(p);
				offset = ftell(p) + pl;
				tfm = fuget4(p);
				fsget4(p); /* skip dx */
				fsget4(p); /* skip dy */
				w  = fuget4(p);
				h  = fuget4(p); 
				x  = fsget4(p);
				y  = fsget4(p);
				break;
			case 4:
			case 5:
			case 6:				
				pl = (flag_byte % 4) * 65536 + fuget2(p);
				cc = fuget1(p);
				offset = ftell(p) + pl;
				tfm = fuget3(p);
				fsget2(p); /* skip dx */
				           /* dy assumed 0 */
				w = fuget2(p);
				h = fuget2(p);
				x = fsget2(p);
				y = fsget2(p);
				break;
			default:
				pl = (flag_byte % 4) * 256 + fuget1(p);
				cc = fuget1(p);
				offset = ftell(p) + pl;
				tfm = fuget3(p);
				fsget1(p); /* skip dx */
				           /* dy assumed 0 */
				w = fuget1(p);
				h = fuget1(p);
				x = fsget1(p);
				y = fsget1(p);
			}
			if(feof(p))
				break;
			if(cc < loc)
				loc = cc;
			if(cc > hic)
				hic = cc;
			if(cc > maxch) {
				font->chars = xresize(font->chars, 
					DviFontChar, cc + 16);
				for(i = maxch; i < cc + 16; i++)
					font->chars[i].offset = 0;
				maxch = cc + 16;
			}
			font->chars[cc].code = cc;
			font->chars[cc].flags = flag_byte;
			font->chars[cc].offset = ftell(p);
			font->chars[cc].width = w;
			font->chars[cc].height = h;
			font->chars[cc].glyph.data = NULL;
			font->chars[cc].x = x;
			font->chars[cc].y = y;
			font->chars[cc].glyph.x = x;
			font->chars[cc].glyph.y = y;
			font->chars[cc].glyph.w = w;
			font->chars[cc].glyph.h = h;
			font->chars[cc].grey.data = NULL;
			font->chars[cc].shrunk.data = NULL;
			font->chars[cc].tfmwidth = TFMSCALE(z, tfm, alpha, beta);
			font->chars[cc].loaded = 0;
			fseek(p, (long)offset, SEEK_SET);
		}
	}
	if(flag_byte != PK_POST) {
		mdvi_error(_("%s: unexpected end of file (no postamble)\n"),
			   font->fontname);
		goto error;
	}
	while((flag_byte = fuget1(p)) != EOF) {
		if(flag_byte != PK_NOOP) {
			mdvi_error(_("invalid PK file! (junk in postamble)\n"));
			goto error;
		}
	}

	/* resize font char data */
	if(loc > 0 || hic < maxch-1) {
		memmove(font->chars, font->chars + loc, 
			(hic - loc + 1) * sizeof(DviFontChar));
		font->chars = xresize(font->chars,
			DviFontChar, hic - loc + 1);
	}
	font->loc = loc;
	font->hic = hic;		
	return 0;

badpk:
	mdvi_error(_("%s: File corrupted, or not a PK file\n"), font->fontname);
error:
	mdvi_free(font->chars);
	font->chars = NULL;
	font->loc = font->hic = 0;
	return -1;
}