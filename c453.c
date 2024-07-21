cli_pdf(const char *dir, cli_ctx *ctx, off_t offset)
{
	off_t size;	/* total number of bytes in the file */
	off_t bytesleft, trailerlength;
	char *buf;	/* start of memory mapped area */
	const char *p, *q, *trailerstart;
	const char *xrefstart;	/* cross reference table */
	/*size_t xreflength;*/
	int printed_predictor_message, printed_embedded_font_message, rc;
	unsigned int files;
	fmap_t *map = *ctx->fmap;
	int opt_failed = 0;

	cli_dbgmsg("in cli_pdf(%s)\n", dir);
	size = map->len - offset;

	if(size <= 7)	/* doesn't even include the file header */
		return CL_CLEAN;

	p = buf = fmap_need_off_once(map, 0, size); /* FIXME: really port to fmap */
	if(!buf) {
		cli_errmsg("cli_pdf: mmap() failed\n");
		return CL_EMAP;
	}

	cli_dbgmsg("cli_pdf: scanning %lu bytes\n", (unsigned long)size);

	/* Lines are terminated by \r, \n or both */

	/* File Header */
	bytesleft = size - 5;
	for(q = p; bytesleft; bytesleft--, q++) {
	    if(!strncasecmp(q, "%PDF-", 5)) {
		bytesleft = size - (off_t) (q - p);
		p = q;
		break;
	    }
	}

	if(!bytesleft) {
	    cli_dbgmsg("cli_pdf: file header not found\n");
	    return CL_CLEAN;
	}

	/* Find the file trailer */
	for(q = &p[bytesleft - 5]; q > p; --q)
		if(strncasecmp(q, "%%EOF", 5) == 0)
			break;

	if(q <= p) {
		cli_dbgmsg("cli_pdf: trailer not found\n");
		return CL_CLEAN;
	}

	for(trailerstart = &q[-7]; trailerstart > p; --trailerstart)
		if(memcmp(trailerstart, "trailer", 7) == 0)
			break;

	/*
	 * q points to the end of the trailer section
	 */
	trailerlength = (long)(q - trailerstart);
	if(cli_memstr(trailerstart, trailerlength, "Encrypt", 7)) {
		/*
		 * This tends to mean that the file is, in effect, read-only
		 * http://www.cs.cmu.edu/~dst/Adobe/Gallery/anon21jul01-pdf-encryption.txt
		 * http://www.adobe.com/devnet/pdf/
		 */
		cli_dbgmsg("cli_pdf: Encrypted PDF files not yet supported\n");
		return CL_CLEAN;
	}

	/*
	 * not true, since edits may put data after the trailer
	bytesleft -= trailerlength;
	 */

	/*
	 * FIXME: Handle more than one xref section in the xref table
	 */
	for(xrefstart = trailerstart; xrefstart > p; --xrefstart)
		if(memcmp(xrefstart, "xref", 4) == 0)
			/*
			 * Make sure it's the start of the line, not a startxref
			 * token
			 */
			if((xrefstart[-1] == '\n') || (xrefstart[-1] == '\r'))
				break;

	if(xrefstart == p) {
		cli_dbgmsg("cli_pdf: xref not found\n");
		return CL_CLEAN;
	}

	printed_predictor_message = printed_embedded_font_message = 0;

	/*
	 * not true, since edits may put data after the trailer
	xreflength = (size_t)(trailerstart - xrefstart);
	bytesleft -= xreflength;
	 */

	files = 0;

	rc = CL_CLEAN;

	/*
	 * The body section consists of a sequence of indirect objects
	 */
	while((p < xrefstart) && (cli_checklimits("cli_pdf", ctx, 0, 0, 0)==CL_CLEAN) &&
	      ((q = pdf_nextobject(p, bytesleft)) != NULL)) {
		int is_ascii85decode, is_flatedecode, fout, len, has_cr;
		/*int object_number, generation_number;*/
		const char *objstart, *objend, *streamstart, *streamend;
		unsigned long length, objlen, real_streamlen, calculated_streamlen;
		int is_embedded_font, predictor;
		char fullname[NAME_MAX + 1];

		rc = CL_CLEAN;
		if(q == xrefstart)
			break;
		if(memcmp(q, "xref", 4) == 0)
			break;

		/*object_number = atoi(q);*/
		bytesleft -= (off_t)(q - p);
		p = q;

		if(memcmp(q, "endobj", 6) == 0)
			continue;
		if(!isdigit(*q)) {
			cli_dbgmsg("cli_pdf: Object number missing\n");
			break;
		}
		q = pdf_nextobject(p, bytesleft);
		if((q == NULL) || !isdigit(*q)) {
			cli_dbgmsg("cli_pdf: Generation number missing\n");
			break;
		}
		/*generation_number = atoi(q);*/
		bytesleft -= (off_t)(q - p);
		p = q;

		q = pdf_nextobject(p, bytesleft);
		if((q == NULL) || (memcmp(q, "obj", 3) != 0)) {
			cli_dbgmsg("cli_pdf: Indirect object missing \"obj\"\n");
			break;
		}

		bytesleft -= (off_t)((q - p) + 3);
		objstart = p = &q[3];
		objend = cli_memstr(p, bytesleft, "endobj", 6);
		if(objend == NULL) {
			cli_dbgmsg("cli_pdf: No matching endobj\n");
			break;
		}
		bytesleft -= (off_t)((objend - p) + 6);
		p = &objend[6];
		objlen = (unsigned long)(objend - objstart);

		/* Is this object a stream? */
		streamstart = cli_memstr(objstart, objlen, "stream", 6);
		if(streamstart == NULL)
			continue;

		is_embedded_font = length = is_ascii85decode =
			is_flatedecode = 0;
		predictor = 1;

		/*
		 * TODO: handle F and FFilter?
		 */
		q = objstart;
		while(q < streamstart) {
			if(*q == '/') {	/* name object */
				/*cli_dbgmsg("Name object %8.8s\n", q+1, q+1);*/
				if(strncmp(++q, "Length ", 7) == 0) {
					q += 7;
					length = atoi(q);
					while(isdigit(*q))
						q++;
					/*
					 * Note: incremental updates are not
					 *	supported
					 */
					if((bytesleft > 11) && strncmp(q, " 0 R", 4) == 0) {
						const char *r, *nq;
						char b[14];

						q += 4;
						cli_dbgmsg("cli_pdf: Length is in indirect obj %lu\n",
							length);
						snprintf(b, sizeof(b),
							"%lu 0 obj", length);
						length = (unsigned long)strlen(b);
						/* optimization: assume objects
						 * are sequential */
						if(!opt_failed) {
						    nq = q;
						    len = buf + size - q;
						} else {
						    nq = buf;
						    len = q - buf;
						}
						do {
							r = cli_memstr(nq, len, b, length);
							if (r > nq) {
								const char x = *(r-1);
								if (x == '\n' || x=='\r') {
									--r;
									break;
								}
							}
							if (r) {
								len -= r + length - nq;
								nq = r + length;
							} else if (!opt_failed) {
								/* we failed optimized match,
								 * try matching from the beginning
								 */
								len = q - buf;
								r = nq = buf;
								/* prevent
								 * infloop */
								opt_failed = 1;
							}
						} while (r);
						if(r) {
							r += length - 1;
							r = pdf_nextobject(r, bytesleft - (r - q));
							if(r) {
								length = atoi(r);
								while(isdigit(*r))
									r++;
								cli_dbgmsg("cli_pdf: length in '%s' %lu\n",
									&b[1],
									length);
							}
						} else
							cli_dbgmsg("cli_pdf: Couldn't find '%s'\n",
								&b[1]);
					}
					q--;
				} else if(strncmp(q, "Length2 ", 8) == 0)
					is_embedded_font = 1;
				else if(strncmp(q, "Predictor ", 10) == 0) {
					q += 10;
					predictor = atoi(q);
					while(isdigit(*q))
						q++;
					q--;
				} else if(strncmp(q, "FlateDecode", 11) == 0) {
					is_flatedecode = 1;
					q += 11;
				} else if(strncmp(q, "ASCII85Decode", 13) == 0) {
					is_ascii85decode = 1;
					q += 13;
				}
			}
			q = pdf_nextobject(q, (size_t)(streamstart - q));
			if(q == NULL)
				break;
		}

		if(is_embedded_font) {
			/*
			 * Need some documentation, the only I can find a
			 * reference to is not free, if some kind soul wishes
			 * to donate a copy, please contact me!
			 * (http://safari.adobepress.com/0321304748)
			 */
			if(!printed_embedded_font_message) {
				cli_dbgmsg("cli_pdf: Embedded fonts not yet supported\n");
				printed_embedded_font_message = 1;
			}
			continue;
		}
		if(predictor > 1) {
			/*
			 * Needs some thought
			 */
			if(!printed_predictor_message) {
				cli_dbgmsg("cli_pdf: Predictor %d not honoured for embedded image\n",
					predictor);
				printed_predictor_message = 1;
			}
			continue;
		}

		/* objend points to the end of the object (start of "endobj") */
		streamstart += 6;	/* go past the word "stream" */
		len = (int)(objend - streamstart);
		q = pdf_nextlinestart(streamstart, len);
		if(q == NULL)
			break;
		len -= (int)(q - streamstart);
		streamstart = q;
		streamend = cli_memstr(streamstart, len, "endstream\n", 10);
		if(streamend == NULL) {
			streamend = cli_memstr(streamstart, len, "endstream\r", 10);
			if(streamend == NULL) {
				cli_dbgmsg("cli_pdf: No endstream\n");
				break;
			}
			has_cr = 1;
		} else
			has_cr = 0;
		snprintf(fullname, sizeof(fullname), "%s"PATHSEP"pdf%02u", dir, files);
		fout = open(fullname, O_RDWR|O_CREAT|O_EXCL|O_TRUNC|O_BINARY, 0600);
		if(fout < 0) {
			char err[128];
			cli_errmsg("cli_pdf: can't create temporary file %s: %s\n", fullname, cli_strerror(errno, err, sizeof(err)));
			rc = CL_ETMPFILE;
			break;
		}

		/*
		 * Calculate the length ourself, the Length parameter is often
		 * wrong
		 */
		if((*--streamend != '\n') && (*streamend != '\r'))
			streamend++;
		else if(has_cr && (*--streamend != '\r'))
			streamend++;

		if(streamend <= streamstart) {
			close(fout);
			cli_dbgmsg("cli_pdf: Empty stream\n");
			if (cli_unlink(fullname)) {
				rc = CL_EUNLINK;
				break;
			}
			continue;
		}
		calculated_streamlen = (int)(streamend - streamstart);
		real_streamlen = length;

		cli_dbgmsg("cli_pdf: length %lu, calculated_streamlen %lu isFlate %d isASCII85 %d\n",
			length, calculated_streamlen,
			is_flatedecode, is_ascii85decode);

		if(calculated_streamlen != real_streamlen) {
			cli_dbgmsg("cli_pdf: Incorrect Length field in file attempting to recover\n");
			if(real_streamlen > calculated_streamlen)
				real_streamlen = calculated_streamlen;
		}
#if	0
		/* FIXME: this isn't right... */
		if(length)
			/*streamlen = (is_flatedecode) ? length : MIN(length, streamlen);*/
			streamlen = MIN(length, streamlen);
#endif

		if(is_ascii85decode) {
			unsigned char *tmpbuf;
			int ret = cli_checklimits("cli_pdf", ctx, calculated_streamlen * 5, calculated_streamlen, real_streamlen);

			if(ret != CL_CLEAN) {
				close(fout);
				if (cli_unlink(fullname)) {
					rc = CL_EUNLINK;
					break;
				}
				continue;
			}

			tmpbuf = cli_malloc(calculated_streamlen * 5);

			if(tmpbuf == NULL) {
				close(fout);
				if (cli_unlink(fullname)) {
					rc = CL_EUNLINK;
					break;
				}
				continue;
			}

			ret = ascii85decode(streamstart, calculated_streamlen, tmpbuf);

			if(ret == -1) {
				free(tmpbuf);
				close(fout);
				if (cli_unlink(fullname)) {
					rc = CL_EUNLINK;
					break;
				}
				continue;
			}
			if(ret) {
				unsigned char *t;

				real_streamlen = ret;
				/* free unused trailing bytes */
				t = (unsigned char *)cli_realloc(tmpbuf,calculated_streamlen);
				if(t == NULL) {
					free(tmpbuf);
					close(fout);
					if (cli_unlink(fullname)) {
						rc = CL_EUNLINK;
						break;
					}
					continue;
				}
				tmpbuf = t;
				/*
				 * Note that it will probably be both
				 * ascii85encoded and flateencoded
				 */

				if(is_flatedecode)
					rc = try_flatedecode((unsigned char *)tmpbuf, real_streamlen, real_streamlen, fout, ctx);
				else
				  rc = (unsigned long)cli_writen(fout, (const char *)streamstart, real_streamlen)==real_streamlen ? CL_CLEAN : CL_EWRITE;
			}
			free(tmpbuf);
		} else if(is_flatedecode) {
			rc = try_flatedecode((unsigned char *)streamstart, real_streamlen, calculated_streamlen, fout, ctx);

		} else {
			cli_dbgmsg("cli_pdf: writing %lu bytes from the stream\n",
				(unsigned long)real_streamlen);
			if((rc = cli_checklimits("cli_pdf", ctx, real_streamlen, 0, 0))==CL_CLEAN)
				rc = (unsigned long)cli_writen(fout, (const char *)streamstart, real_streamlen) == real_streamlen ? CL_CLEAN : CL_EWRITE;
		}

		if (rc == CL_CLEAN) {
			cli_dbgmsg("cli_pdf: extracted file %u to %s\n", files, fullname);
			files++;
	
			lseek(fout, 0, SEEK_SET);
			rc = cli_magic_scandesc(fout, ctx);
		}
		close(fout);
		if(!ctx->engine->keeptmp)
			if (cli_unlink(fullname)) rc = CL_EUNLINK;
		if(rc != CL_CLEAN) break;
	}


	cli_dbgmsg("cli_pdf: returning %d\n", rc);
	return rc;
}