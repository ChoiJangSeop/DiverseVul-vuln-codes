static int pdf_extract_obj(struct pdf_struct *pdf, struct pdf_obj *obj)
{
    char fullname[NAME_MAX + 1];
    int fout;
    off_t sum = 0;
    int rc = CL_SUCCESS;
    char *ascii_decoded = NULL;
    int dump = 1;

    /* TODO: call bytecode hook here, allow override dumpability */
    if ((!(obj->flags & (1 << OBJ_STREAM)) ||
	(obj->flags & (1 << OBJ_HASFILTERS)))
	&& !(obj->flags & DUMP_MASK)) {
	/* don't dump all streams */
	dump = 0;
    }
    if ((obj->flags & (1 << OBJ_IMAGE)) &&
	!(obj->flags & (1 << OBJ_FILTER_DCT))) {
	/* don't dump / scan non-JPG images */
	dump = 0;
    }
    if (obj->flags & (1 << OBJ_FORCEDUMP)) {
	/* bytecode can force dump by setting this flag */
	dump = 1;
    }
    if (!dump)
	return CL_CLEAN;
    cli_dbgmsg("cli_pdf: dumping obj %u %u\n", obj->id>>8, obj->id);
    snprintf(fullname, sizeof(fullname), "%s"PATHSEP"pdf%02u", pdf->dir, pdf->files++);
    fout = open(fullname,O_RDWR|O_CREAT|O_EXCL|O_TRUNC|O_BINARY, 0600);
    if (fout < 0) {
	char err[128];
	cli_errmsg("cli_pdf: can't create temporary file %s: %s\n", fullname, cli_strerror(errno, err, sizeof(err)));
	free(ascii_decoded);
	return CL_ETMPFILE;
    }

    do {
    if (obj->flags & (1 << OBJ_STREAM)) {
	const char *start = pdf->map + obj->start;
	off_t p_stream = 0, p_endstream = 0;
	off_t length;
	find_stream_bounds(start, pdf->size - obj->start,
			   pdf->size - obj->start,
			   &p_stream, &p_endstream);
	if (p_stream && p_endstream) {
	    const char *flate_in;
	    long ascii_decoded_size = 0;
	    size_t size = p_endstream - p_stream;
	    off_t orig_length;

	    length = find_length(pdf, obj, start, p_stream);
	    if (length < 0)
		length = 0;
	    orig_length = length;
	    if (length > pdf->size || obj->start + p_stream + length > pdf->size) {
		cli_dbgmsg("cli_pdf: length out of file: %ld + %ld > %ld\n",
			   p_stream, length, pdf->size);
		length = pdf->size - (obj->start + p_stream);
	    }
	    if (!(obj->flags & (1 << OBJ_FILTER_FLATE)) && length <= 0) {
		const char *q = start + p_endstream;
		length = size;
		q--;
		if (*q == '\n') {
		    q--;
		    length--;
		    if (*q == '\r')
			length--;
		} else if (*q == '\r') {
		    length--;
		}
		if (length < 0)
		    length = 0;
		cli_dbgmsg("cli_pdf: calculated length %ld\n", length);
	    } else {
		if (size > length+2) {
		    cli_dbgmsg("cli_pdf: calculated length %ld < %ld\n",
			       length, size);
		    length = size;
		}
	    }
	    if (orig_length && size > orig_length + 20) {
		cli_dbgmsg("cli_pdf: orig length: %ld, length: %ld, size: %ld\n",
			   orig_length, length, size);
		pdfobj_flag(pdf, obj, BAD_STREAMLEN);
	    }
	    if (!length)
		length = size;

	    if (obj->flags & (1 << OBJ_FILTER_AH)) {
		ascii_decoded = cli_malloc(length/2 + 1);
		if (!ascii_decoded) {
		    cli_errmsg("Cannot allocate memory for asciidecode\n");
		    rc = CL_EMEM;
		    break;
		}
		ascii_decoded_size = asciihexdecode(start + p_stream,
						    length,
						    ascii_decoded);
	    } else if (obj->flags & (1 << OBJ_FILTER_A85)) {
		ascii_decoded = cli_malloc(length*5);
		if (!ascii_decoded) {
		    cli_errmsg("Cannot allocate memory for asciidecode\n");
		    rc = CL_EMEM;
		    break;
		}
		ascii_decoded_size = ascii85decode(start+p_stream,
						   length,
						   (unsigned char*)ascii_decoded);
	    }
	    if (ascii_decoded_size < 0) {
		/* don't flag for images or truncated objs*/
		if (!(obj->flags &
		      ((1 << OBJ_IMAGE) | (1 << OBJ_TRUNCATED))))
		    pdfobj_flag(pdf, obj, BAD_ASCIIDECODE);
		cli_dbgmsg("cli_pdf: failed to asciidecode in %u %u obj\n", obj->id>>8,obj->id&0xff);
		free(ascii_decoded);
		ascii_decoded = NULL;
		/* attempt to directly flatedecode it */
	    }
	    /* either direct or ascii-decoded input */
	    if (!ascii_decoded)
		ascii_decoded_size = length;
	    flate_in = ascii_decoded ? ascii_decoded : start+p_stream;

	    if (obj->flags & (1 << OBJ_FILTER_FLATE)) {
		cli_dbgmsg("cli_pdf: deflate len %ld (orig %ld)\n", ascii_decoded_size, (long)orig_length);
		rc = filter_flatedecode(pdf, obj, flate_in, ascii_decoded_size, fout, &sum);
	    } else {
		if (filter_writen(pdf, obj, fout, flate_in, ascii_decoded_size, &sum) != ascii_decoded_size)
		    rc = CL_EWRITE;
	    }
	}
    } else if (obj->flags & (1 << OBJ_JAVASCRIPT)) {
	const char *q2;
	const char *q = pdf->map+obj->start;
	/* TODO: get obj-endobj size */
	off_t bytesleft = obj_size(pdf, obj, 0);
	if (bytesleft < 0)
	    break;

	q2 = cli_memstr(q, bytesleft, "/JavaScript", 11);
	if (!q2)
	    break;
	bytesleft -= q2 - q;
	do {
	q2++;
	bytesleft--;
	q = pdf_nextobject(q2, bytesleft);
	if (!q)
	    break;
	bytesleft -= q - q2;
	q2 = q;
	} while (*q == '/');
	if (!q)
	    break;
	if (*q == '(') {
	    if (filter_writen(pdf, obj, fout, q+1, bytesleft-1, &sum) != (bytesleft-1)) {
		rc = CL_EWRITE;
		break;
	    }
	} else if (*q == '<') {
	    char *decoded;
	    q2 = memchr(q+1, '>', bytesleft);
	    if (!q2) q2 = q + bytesleft;
	    decoded = cli_malloc(q2 - q);
	    if (!decoded) {
		rc = CL_EMEM;
		break;
	    }
	    cli_hex2str_to(q2, decoded, q2-q-1);
	    decoded[q2-q-1] = '\0';
	    cli_dbgmsg("cli_pdf: found hexadecimal encoded javascript in %u %u obj\n",
		       obj->id>>8, obj->id&0xff);
	    pdfobj_flag(pdf, obj, HEX_JAVASCRIPT);
	    filter_writen(pdf, obj, fout, decoded, q2-q-1, &sum);
	    free(decoded);
	}
    } else {
	off_t bytesleft = obj_size(pdf, obj, 0);
	if (filter_writen(pdf, obj, fout , pdf->map + obj->start, bytesleft,&sum) != bytesleft)
	    rc = CL_EWRITE;
    }
    } while (0);
    cli_dbgmsg("cli_pdf: extracted %ld bytes %u %u obj to %s\n", sum, obj->id>>8, obj->id&0xff, fullname);
    if (sum) {
	int rc2;
	cli_updatelimits(pdf->ctx, sum);
	/* TODO: invoke bytecode on this pdf obj with metainformation associated
	 * */
	lseek(fout, 0, SEEK_SET);
	rc2 = cli_magic_scandesc(fout, pdf->ctx);
	if (rc2 == CL_VIRUS || rc == CL_SUCCESS)
	    rc = rc2;
	if (rc == CL_CLEAN) {
	    rc2 = run_pdf_hooks(pdf, PDF_PHASE_POSTDUMP, fout, obj - pdf->objs);
	    if (rc2 == CL_VIRUS)
		rc = rc2;
	}
    }
    close(fout);
    free(ascii_decoded);
    if (!pdf->ctx->engine->keeptmp)
	if (cli_unlink(fullname) && rc != CL_VIRUS)
	    rc = CL_EUNLINK;
    return rc;
}