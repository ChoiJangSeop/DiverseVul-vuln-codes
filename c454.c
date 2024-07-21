int cli_pdf(const char *dir, cli_ctx *ctx, off_t offset)
{
    struct pdf_struct pdf;
    fmap_t *map = *ctx->fmap;
    size_t size = map->len - offset;
    off_t versize = size > 1032 ? 1032 : size;
    off_t map_off, bytesleft;
    long xref;
    const char *pdfver, *start, *eofmap, *q, *eof;
    int rc;
    unsigned i;

    cli_dbgmsg("in cli_pdf(%s)\n", dir);
    memset(&pdf, 0, sizeof(pdf));
    pdf.ctx = ctx;
    pdf.dir = dir;

    pdfver = start = fmap_need_off_once(map, offset, versize);

    /* Check PDF version */
    if (!pdfver) {
	cli_errmsg("cli_pdf: mmap() failed (1)\n");
	return CL_EMAP;
    }
    /* offset is 0 when coming from filetype2 */
    pdfver = cli_memstr(pdfver, versize, "%PDF-", 5);
    if (!pdfver) {
	cli_dbgmsg("cli_pdf: no PDF- header found\n");
	return CL_SUCCESS;
    }
    /* Check for PDF-1.[0-9]. Although 1.7 is highest now, allow for future
     * versions */
    if (pdfver[5] != '1' || pdfver[6] != '.' ||
	pdfver[7] < '1' || pdfver[7] > '9') {
	pdf.flags |= 1 << BAD_PDF_VERSION;
	cli_dbgmsg("cli_pdf: bad pdf version: %.8s\n", pdfver);
    }
    if (pdfver != start || offset) {
	pdf.flags |= 1 << BAD_PDF_HEADERPOS;
	cli_dbgmsg("cli_pdf: PDF header is not at position 0: %ld\n",pdfver-start+offset);
    }
    offset += pdfver - start;

    /* find trailer and xref, don't fail if not found */
    map_off = map->len - 2048;
    if (map_off < 0)
	map_off = 0;
    bytesleft = map->len - map_off;
    eofmap = fmap_need_off_once(map, map_off, bytesleft);
    if (!eofmap) {
	cli_errmsg("cli_pdf: mmap() failed (2)\n");
	return CL_EMAP;
    }
    eof = eofmap + bytesleft;
    for (q=&eofmap[bytesleft-5]; q > eofmap; q--) {
	if (memcmp(q, "%%EOF", 5) == 0)
	    break;
    }
    if (q <= eofmap) {
	pdf.flags |= 1 << BAD_PDF_TRAILER;
	cli_dbgmsg("cli_pdf: %%%%EOF not found\n");
    } else {
	size = q - eofmap + map_off;
	for (;q > eofmap;q--) {
	    if (memcmp(q, "startxref", 9) == 0)
		break;
	}
	if (q <= eofmap) {
	    pdf.flags |= 1 << BAD_PDF_TRAILER;
	    cli_dbgmsg("cli_pdf: startxref not found\n");
	}
	q += 9;
	while (q < eof && (*q == ' ' || *q == '\n' || *q == '\r')) { q++; }
	xref = atol(q);
	bytesleft = map->len - offset - xref;
	if (bytesleft > 4096)
	    bytesleft = 4096;
	q = fmap_need_off_once(map, offset + xref, bytesleft);
	if (!q || xrefCheck(q, q+bytesleft) == -1) {
	    cli_dbgmsg("cli_pdf: did not find valid xref\n");
	    pdf.flags |= 1 << BAD_PDF_TRAILER;
	}
    }
    size -= offset;

    pdf.size = size;
    pdf.map = fmap_need_off_once(map, offset, size);
    if (!pdf.map) {
	cli_errmsg("cli_pdf: mmap() failed (3)\n");
	return CL_EMAP;
    }
    /* parse PDF and find obj offsets */
    while ((rc = pdf_findobj(&pdf)) > 0) {
	struct pdf_obj *obj = &pdf.objs[pdf.nobjs-1];
	cli_dbgmsg("found %d %d obj @%ld\n", obj->id >> 8, obj->id&0xff, obj->start + offset);
	pdf_parseobj(&pdf, obj);
    }
    if (rc == -1)
	pdf.flags |= 1 << BAD_PDF_TOOMANYOBJS;

    /* extract PDF objs */
    for (i=0;i<pdf.nobjs;i++) {
	struct pdf_obj *obj = &pdf.objs[i];
	rc = pdf_extract_obj(&pdf, obj);
	if (rc != CL_SUCCESS)
	    break;
    }

    if (pdf.flags) {
	cli_dbgmsg("cli_pdf: flags 0x%02x\n", pdf.flags);
	if (pdf.flags & (1 << ESCAPED_COMMON_PDFNAME)) {
	    /* for example /Fl#61te#44#65#63#6f#64#65 instead of /FlateDecode */
	    *ctx->virname = "Heuristics.PDF.ObfuscatedNameObject";
	    rc = CL_VIRUS;
	}
    }
    cli_dbgmsg("cli_pdf: returning %d\n", rc);
    free(pdf.objs);
    return rc;
}