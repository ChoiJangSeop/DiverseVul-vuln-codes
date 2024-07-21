void fli_read_lc_2(FILE *f, s_fli_header *fli_header, unsigned char *old_framebuf, unsigned char *framebuf)
{
	unsigned short yc, lc, numline;
	unsigned char *pos;
	memcpy(framebuf, old_framebuf, fli_header->width * fli_header->height);
	yc=0;
	numline = fli_read_short(f);
	for (lc=0; lc < numline; lc++) {
		unsigned short xc, pc, pcnt, lpf, lpn;
		pc=fli_read_short(f);
		lpf=0; lpn=0;
		while (pc & 0x8000) {
			if (pc & 0x4000) {
				yc+=-(signed short)pc;
			} else {
				lpf=1;lpn=pc&0xFF;
			}
			pc=fli_read_short(f);
		}
		xc=0;
		pos=framebuf+(fli_header->width * yc);
		for (pcnt=pc; pcnt>0; pcnt--) {
			unsigned short ps,skip;
			skip=fli_read_char(f);
			ps=fli_read_char(f);
			xc+=skip;
			if (ps & 0x80) {
				unsigned char v1,v2;
				ps=-(signed char)ps;
				v1=fli_read_char(f);
				v2=fli_read_char(f);
				while (ps>0) {
					pos[xc++]=v1;
					pos[xc++]=v2;
					ps--;
				}
			} else {
				fread(&(pos[xc]), ps, 2, f);
				xc+=ps << 1;
			}
		}
		if (lpf) pos[xc]=lpn;
		yc++;
	}
}