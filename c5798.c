void fli_read_lc(FILE *f, s_fli_header *fli_header, unsigned char *old_framebuf, unsigned char *framebuf)
{
	unsigned short yc, firstline, numline;
	unsigned char *pos;
	memcpy(framebuf, old_framebuf, fli_header->width * fli_header->height);
	firstline = fli_read_short(f);
	numline = fli_read_short(f);
	for (yc=0; yc < numline; yc++) {
		unsigned short xc, pc, pcnt;
		pc=fli_read_char(f);
		xc=0;
		pos=framebuf+(fli_header->width * (firstline+yc));
		for (pcnt=pc; pcnt>0; pcnt--) {
			unsigned short ps,skip;
			skip=fli_read_char(f);
			ps=fli_read_char(f);
			xc+=skip;
			if (ps & 0x80) {
				unsigned char val;
				ps=-(signed char)ps;
				val=fli_read_char(f);
				memset(&(pos[xc]), val, ps);
				xc+=ps;
			} else {
				fread(&(pos[xc]), ps, 1, f);
				xc+=ps;
			}
		}
	}
}