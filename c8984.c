void dname_print(FILE* out, struct sldns_buffer* pkt, uint8_t* dname)
{
	uint8_t lablen;
	if(!out) out = stdout;
	if(!dname) return;

	lablen = *dname++;
	if(!lablen) 
		fputc('.', out);
	while(lablen) {
		if(LABEL_IS_PTR(lablen)) {
			/* follow pointer */
			if(!pkt) {
				fputs("??compressionptr??", out);
				return;
			}
			dname = sldns_buffer_at(pkt, PTR_OFFSET(lablen, *dname));
			lablen = *dname++;
			continue;
		}
		if(lablen > LDNS_MAX_LABELLEN) {
			fputs("??extendedlabel??", out);
			return;
		}
		while(lablen--)
			fputc((int)*dname++, out);
		fputc('.', out);
		lablen = *dname++;
	}
}