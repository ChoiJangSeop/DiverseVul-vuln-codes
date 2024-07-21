ldns_rr_new_frm_str_internal(ldns_rr **newrr, const char *str,
                             uint32_t default_ttl, ldns_rdf *origin,
		             ldns_rdf **prev, bool question)
{
	ldns_rr *new;
	const ldns_rr_descriptor *desc;
	ldns_rr_type rr_type;
	ldns_buffer *rr_buf = NULL;
	ldns_buffer *rd_buf = NULL;
	uint32_t ttl_val;
	char  *owner = NULL;
	char  *ttl = NULL;
	ldns_rr_class clas_val;
	char  *clas = NULL;
	char  *type = NULL;
	char  *rdata = NULL;
	char  *rd = NULL;
	char  *	b64 = NULL;
	size_t rd_strlen;
	const char *delimiters;
	ssize_t c;
	ldns_rdf *owner_dname;
        const char* endptr;
        int was_unknown_rr_format = 0;
	ldns_status status = LDNS_STATUS_OK;

	/* used for types with unknown number of rdatas */
	bool done;
	bool quoted;

	ldns_rdf *r = NULL;
	uint16_t r_cnt;
	uint16_t r_min;
	uint16_t r_max;
        size_t pre_data_pos;

	new = ldns_rr_new();

	owner = LDNS_XMALLOC(char, LDNS_MAX_DOMAINLEN + 1);
	ttl = LDNS_XMALLOC(char, LDNS_TTL_DATALEN);
	clas = LDNS_XMALLOC(char, LDNS_SYNTAX_DATALEN);
	rdata = LDNS_XMALLOC(char, LDNS_MAX_PACKETLEN + 1);
	rr_buf = LDNS_MALLOC(ldns_buffer);
	rd_buf = LDNS_MALLOC(ldns_buffer);
	rd = LDNS_XMALLOC(char, LDNS_MAX_RDFLEN);
	b64 = LDNS_XMALLOC(char, LDNS_MAX_RDFLEN);
	if (!new || !owner || !ttl || !clas || !rdata || !rr_buf || !rd_buf || !rd || !b64 ) {
		status = LDNS_STATUS_MEM_ERR;
		LDNS_FREE(rr_buf);
		goto ldnserror;
	}

	ldns_buffer_new_frm_data(rr_buf, (char*)str, strlen(str));

	/* split the rr in its parts -1 signals trouble */
	if (ldns_bget_token(rr_buf, owner, "\t\n ", LDNS_MAX_DOMAINLEN) == -1) {
		status = LDNS_STATUS_SYNTAX_ERR;
		ldns_buffer_free(rr_buf);
		goto ldnserror;
	}

	if (ldns_bget_token(rr_buf, ttl, "\t\n ", LDNS_TTL_DATALEN) == -1) {
		status = LDNS_STATUS_SYNTAX_TTL_ERR;
		ldns_buffer_free(rr_buf);
		goto ldnserror;
	}
	ttl_val = (uint32_t) ldns_str2period(ttl, &endptr);

	if (strlen(ttl) > 0 && !isdigit((int) ttl[0])) {
		/* ah, it's not there or something */
		if (default_ttl == 0) {
			ttl_val = LDNS_DEFAULT_TTL;
		} else {
			ttl_val = default_ttl;
		}
		/* we not ASSUMING the TTL is missing and that
		 * the rest of the RR is still there. That is
		 * CLASS TYPE RDATA
		 * so ttl value we read is actually the class
		 */
		clas_val = ldns_get_rr_class_by_name(ttl);
		/* class can be left out too, assume IN, current
		 * token must be type
		 */
		if (clas_val == 0) {
			clas_val = LDNS_RR_CLASS_IN;
			type = LDNS_XMALLOC(char, strlen(ttl) + 1);
			if(!type) {
				status = LDNS_STATUS_MEM_ERR;
				ldns_buffer_free(rr_buf);
				goto ldnserror;
			}
			strncpy(type, ttl, strlen(ttl) + 1);
		}
	} else {
		if (ldns_bget_token(rr_buf, clas, "\t\n ", LDNS_SYNTAX_DATALEN) == -1) {
			status = LDNS_STATUS_SYNTAX_CLASS_ERR;
			ldns_buffer_free(rr_buf);
			goto ldnserror;
		}
		clas_val = ldns_get_rr_class_by_name(clas);
		/* class can be left out too, assume IN, current
		 * token must be type
		 */
		if (clas_val == 0) {
			clas_val = LDNS_RR_CLASS_IN;
			type = LDNS_XMALLOC(char, strlen(clas) + 1);
			if(!type) {
				status = LDNS_STATUS_MEM_ERR;
				ldns_buffer_free(rr_buf);
				goto ldnserror;
			}
			strncpy(type, clas, strlen(clas) + 1);
		}
	}
	/* the rest should still be waiting for us */

	if (!type) {
		type = LDNS_XMALLOC(char, LDNS_SYNTAX_DATALEN);
		if(!type) {
			status = LDNS_STATUS_MEM_ERR;
			ldns_buffer_free(rr_buf);
			goto ldnserror;
		}
		if (ldns_bget_token(rr_buf, type, "\t\n ", LDNS_SYNTAX_DATALEN) == -1) {
			status = LDNS_STATUS_SYNTAX_TYPE_ERR;
			ldns_buffer_free(rr_buf);
			goto ldnserror;
		}
	}

	if (ldns_bget_token(rr_buf, rdata, "\0", LDNS_MAX_PACKETLEN) == -1) {
		/* apparently we are done, and it's only a question RR
		 * so do not set status and go to ldnserror here
		*/
	}

	ldns_buffer_new_frm_data(rd_buf, rdata, strlen(rdata));

	if (strlen(owner) <= 1 && strncmp(owner, "@", 1) == 0) {
		if (origin) {
			ldns_rr_set_owner(new, ldns_rdf_clone(origin));
		} else if (prev && *prev) {
			ldns_rr_set_owner(new, ldns_rdf_clone(*prev));
		} else {
			/* default to root */
			ldns_rr_set_owner(new, ldns_dname_new_frm_str("."));
		}

		/* @ also overrides prev */
		if (prev) {
			ldns_rdf_deep_free(*prev);
			*prev = ldns_rdf_clone(ldns_rr_owner(new));
			if (!*prev) {
				status = LDNS_STATUS_MEM_ERR;
				ldns_buffer_free(rr_buf);
				goto ldnserror;
			}
		}
	} else {
		if (strlen(owner) == 0) {
			/* no ownername was given, try prev, if that fails
			 * origin, else default to root */
			if (prev && *prev) {
				ldns_rr_set_owner(new, ldns_rdf_clone(*prev));
			} else if (origin) {
				ldns_rr_set_owner(new, ldns_rdf_clone(origin));
			} else {
				ldns_rr_set_owner(new, ldns_dname_new_frm_str("."));
			}
			if(!ldns_rr_owner(new)) {
				status = LDNS_STATUS_MEM_ERR;
				ldns_buffer_free(rr_buf);
				goto ldnserror;
			}
		} else {
			owner_dname = ldns_dname_new_frm_str(owner);
			if (!owner_dname) {
				status = LDNS_STATUS_SYNTAX_ERR;
				ldns_buffer_free(rr_buf);
				goto ldnserror;
			}

			ldns_rr_set_owner(new, owner_dname);
			if (!ldns_dname_str_absolute(owner) && origin) {
				if(ldns_dname_cat(ldns_rr_owner(new),
							origin) != LDNS_STATUS_OK) {
					status = LDNS_STATUS_SYNTAX_ERR;
					ldns_buffer_free(rr_buf);
					goto ldnserror;
				}
			}
			if (prev) {
				ldns_rdf_deep_free(*prev);
				*prev = ldns_rdf_clone(ldns_rr_owner(new));
				if(!*prev) {
					status = LDNS_STATUS_MEM_ERR;
					ldns_buffer_free(rr_buf);
					goto ldnserror;
				}
			}
		}
	}
	LDNS_FREE(owner);
	owner = NULL;

	ldns_rr_set_question(new, question);

	ldns_rr_set_ttl(new, ttl_val);
	LDNS_FREE(ttl);
	ttl = NULL;

	ldns_rr_set_class(new, clas_val);
	LDNS_FREE(clas);
	clas = NULL;

	rr_type = ldns_get_rr_type_by_name(type);
	LDNS_FREE(type);
	type = NULL;

	desc = ldns_rr_descript((uint16_t)rr_type);
	ldns_rr_set_type(new, rr_type);
	if (desc) {
		/* only the rdata remains */
		r_max = ldns_rr_descriptor_maximum(desc);
		r_min = ldns_rr_descriptor_minimum(desc);
	} else {
		r_min = 0;
		r_max = 1;
	}

	/* depending on the rr_type we need to extract
	 * the rdata differently, e.g. NSEC/NSEC3 */
	switch(rr_type) {
		default:
			done = false;

			for (r_cnt = 0; !done && r_cnt < r_max; r_cnt++) {
				quoted = false;
				/* if type = B64, the field may contain spaces */
				if (ldns_rr_descriptor_field_type(desc,
					    r_cnt) == LDNS_RDF_TYPE_B64 ||
				    ldns_rr_descriptor_field_type(desc,
					    r_cnt) == LDNS_RDF_TYPE_HEX ||
				    ldns_rr_descriptor_field_type(desc,
					    r_cnt) == LDNS_RDF_TYPE_LOC ||
				    ldns_rr_descriptor_field_type(desc,
					    r_cnt) == LDNS_RDF_TYPE_WKS ||
				    ldns_rr_descriptor_field_type(desc,
					    r_cnt) == LDNS_RDF_TYPE_IPSECKEY ||
				    ldns_rr_descriptor_field_type(desc,
					    r_cnt) == LDNS_RDF_TYPE_NSEC) {
					delimiters = "\n\t";
				} else {
					delimiters = "\n\t ";
				}

				if (ldns_rr_descriptor_field_type(desc,
							r_cnt) == LDNS_RDF_TYPE_STR &&
							ldns_buffer_remaining(rd_buf) > 0) {
					/* skip spaces */
					while (*(ldns_buffer_current(rd_buf)) == ' ') {
						ldns_buffer_skip(rd_buf, 1);
					}

					if (*(ldns_buffer_current(rd_buf)) == '\"') {
						delimiters = "\"\0";
						ldns_buffer_skip(rd_buf, 1);
						quoted = true;
					}
				}

				/* because number of fields can be variable, we can't
				   rely on _maximum() only */
				/* skip spaces */
				while (ldns_buffer_position(rd_buf) < ldns_buffer_limit(rd_buf) &&
					*(ldns_buffer_current(rd_buf)) == ' ' && !quoted
				      ) {
					ldns_buffer_skip(rd_buf, 1);
				}

				pre_data_pos = ldns_buffer_position(rd_buf);
				if ((c = ldns_bget_token(rd_buf, rd, delimiters,
							LDNS_MAX_RDFLEN)) != -1) {
					/* hmmz, rfc3597 specifies that any type can be represented with
					 * \# method, which can contain spaces...
					 * it does specify size though...
					 */
					rd_strlen = strlen(rd);

					/* unknown RR data */
					if (strncmp(rd, "\\#", 2) == 0 && !quoted && (rd_strlen == 2 || rd[2]==' ')) {
                                        	uint16_t hex_data_size;
                                                char *hex_data_str;
                                                uint16_t cur_hex_data_size;

                                                was_unknown_rr_format = 1;
                                                /* go back to before \# and skip it while setting delimiters better */
                                                ldns_buffer_set_position(rd_buf, pre_data_pos);
					        delimiters = "\n\t ";
                                                (void)ldns_bget_token(rd_buf, rd, delimiters, LDNS_MAX_RDFLEN);
                                                /* read rdata octet length */
						c = ldns_bget_token(rd_buf, rd, delimiters, LDNS_MAX_RDFLEN);
						if (c == -1) {
							/* something goes very wrong here */
                                                        LDNS_FREE(rd);
                                                        LDNS_FREE(b64);
                                                        ldns_buffer_free(rd_buf);
                                                        ldns_buffer_free(rr_buf);
                                                        LDNS_FREE(rdata);
                                                        ldns_rr_free(new);
							return LDNS_STATUS_SYNTAX_RDATA_ERR;
						}
						hex_data_size = (uint16_t) atoi(rd);
						/* copy the hex chars into hex str (which is 2 chars per byte) */
						hex_data_str = LDNS_XMALLOC(char, 2 * hex_data_size + 1);
						if (!hex_data_str) {
							/* malloc error */
                                                        LDNS_FREE(rd);
                                                        LDNS_FREE(b64);
                                                        ldns_buffer_free(rd_buf);
                                                        ldns_buffer_free(rr_buf);
                                                        LDNS_FREE(rdata);
                                                        ldns_rr_free(new);
							return LDNS_STATUS_SYNTAX_RDATA_ERR;
						}
						cur_hex_data_size = 0;
						while(cur_hex_data_size < 2 * hex_data_size) {
							c = ldns_bget_token(rd_buf, rd, delimiters, LDNS_MAX_RDFLEN);
							if (c != -1) {
								rd_strlen = strlen(rd);
							}
							if (c == -1 || (size_t)cur_hex_data_size + rd_strlen > 2 * (size_t)hex_data_size) {
								LDNS_FREE(hex_data_str);
								LDNS_FREE(rd);
								LDNS_FREE(b64);
								ldns_buffer_free(rd_buf);
								ldns_buffer_free(rr_buf);
								LDNS_FREE(rdata);
								ldns_rr_free(new);
								return LDNS_STATUS_SYNTAX_RDATA_ERR;
							}
							strncpy(hex_data_str + cur_hex_data_size, rd, rd_strlen);
							cur_hex_data_size += rd_strlen;
						}
						hex_data_str[cur_hex_data_size] = '\0';

						/* correct the rdf type */
						/* if *we* know the type, interpret it as wireformat */
						if (desc) {
							size_t hex_pos = 0;
							uint8_t *hex_data = LDNS_XMALLOC(uint8_t, hex_data_size + 2);
                                                        ldns_status s;
                                                        if(!hex_data) {
                                                                LDNS_FREE(hex_data_str);
                                                                LDNS_FREE(rd);
                                                                LDNS_FREE(b64);
                                                                ldns_buffer_free(rd_buf);
                                                                ldns_buffer_free(rr_buf);
                                                                LDNS_FREE(rdata);
                                                                ldns_rr_free(new);
                                                                return LDNS_STATUS_MEM_ERR;
                                                        }
							ldns_write_uint16(hex_data, hex_data_size);
							ldns_hexstring_to_data(hex_data + 2, hex_data_str);
							s = ldns_wire2rdf(new, hex_data,
							                 hex_data_size+2, &hex_pos);
                                                        if(s != LDNS_STATUS_OK) {
                                                                LDNS_FREE(hex_data_str);
                                                                LDNS_FREE(rd);
                                                                LDNS_FREE(b64);
                                                                ldns_buffer_free(rd_buf);
                                                                ldns_buffer_free(rr_buf);
                                                                LDNS_FREE(rdata);
                                                                ldns_rr_free(new);
								LDNS_FREE(hex_data);
                                                                return s;
                                                        }
							LDNS_FREE(hex_data);
						} else {
							r = ldns_rdf_new_frm_str(LDNS_RDF_TYPE_HEX, hex_data_str);
                                                        if(!r) {
                                                                LDNS_FREE(hex_data_str);
                                                                LDNS_FREE(rd);
                                                                LDNS_FREE(b64);
                                                                ldns_buffer_free(rd_buf);
                                                                ldns_buffer_free(rr_buf);
                                                                LDNS_FREE(rdata);
                                                                ldns_rr_free(new);
                                                                return LDNS_STATUS_MEM_ERR;
                                                        }
							ldns_rdf_set_type(r, LDNS_RDF_TYPE_UNKNOWN);
							if(!ldns_rr_push_rdf(new, r)) {
                                                                LDNS_FREE(hex_data_str);
                                                                LDNS_FREE(rd);
                                                                LDNS_FREE(b64);
                                                                ldns_buffer_free(rd_buf);
                                                                ldns_buffer_free(rr_buf);
                                                                LDNS_FREE(rdata);
                                                                ldns_rr_free(new);
                                                                return LDNS_STATUS_MEM_ERR;
                                                        }
						}
						LDNS_FREE(hex_data_str);
					} else {
						/* Normal RR */
						switch(ldns_rr_descriptor_field_type(desc, r_cnt)) {
						case LDNS_RDF_TYPE_HEX:
						case LDNS_RDF_TYPE_B64:
							/* can have spaces, and will always be the last
							 * record of the rrdata. Read in the rest */
							if ((c = ldns_bget_token(rd_buf,
												b64,
												"\n",
												LDNS_MAX_RDFLEN))
							    != -1) {
								rd = strncat(rd,
										   b64,
										   LDNS_MAX_RDFLEN
										   - strlen(rd) - 1);
							}
							r = ldns_rdf_new_frm_str(
									ldns_rr_descriptor_field_type(desc, r_cnt),
									rd);
							break;
						case LDNS_RDF_TYPE_DNAME:
							r = ldns_rdf_new_frm_str(
									ldns_rr_descriptor_field_type(desc, r_cnt),
									rd);

							/* check if the origin should be used or concatenated */
							if (r && ldns_rdf_size(r) > 1 && ldns_rdf_data(r)[0] == 1
								&& ldns_rdf_data(r)[1] == '@') {
								ldns_rdf_deep_free(r);
								if (origin) {
									r = ldns_rdf_clone(origin);
								} else {
								     /* if this is the SOA, use its own owner name */
									if (rr_type == LDNS_RR_TYPE_SOA) {
										r = ldns_rdf_clone(ldns_rr_owner(new));
									} else {
										r = ldns_rdf_new_frm_str(LDNS_RDF_TYPE_DNAME, ".");
									}
								}
							} else if (r && rd_strlen >= 1 && !ldns_dname_str_absolute(rd) && origin) {
								if (ldns_dname_cat(r, origin) != LDNS_STATUS_OK) {
							                LDNS_FREE(rd);
							                LDNS_FREE(b64);
							                ldns_buffer_free(rd_buf);
							                ldns_buffer_free(rr_buf);
							                LDNS_FREE(rdata);
							                ldns_rr_free(new);
									return LDNS_STATUS_ERR;
								}
							}
							break;
						default:
							r = ldns_rdf_new_frm_str(
									ldns_rr_descriptor_field_type(desc, r_cnt),
									rd);
							break;
						}
						if (r) {
							ldns_rr_push_rdf(new, r);
						} else {
							LDNS_FREE(rd);
							LDNS_FREE(b64);
							ldns_buffer_free(rd_buf);
							ldns_buffer_free(rr_buf);
							LDNS_FREE(rdata);
							ldns_rr_free(new);
							return LDNS_STATUS_SYNTAX_RDATA_ERR;
						}
					}
					if (quoted) {
						if (ldns_buffer_available(rd_buf, 1)) {
							ldns_buffer_skip(rd_buf, 1);
						} else {
							done = true;
						}
					}
				} else {
					done = true;
				}
			}
	}
	LDNS_FREE(rd);
	LDNS_FREE(b64);
	ldns_buffer_free(rd_buf);
	ldns_buffer_free(rr_buf);
	LDNS_FREE(rdata);

	if (!question && desc && !was_unknown_rr_format && ldns_rr_rd_count(new) < r_min) {
		ldns_rr_free(new);
		return LDNS_STATUS_SYNTAX_MISSING_VALUE_ERR;
	}

	if (newrr) {
		*newrr = new;
	} else {
		/* Maybe the caller just wanted to see if it would parse? */
		ldns_rr_free(new);
	}
	return LDNS_STATUS_OK;

ldnserror:
	LDNS_FREE(type);
	LDNS_FREE(owner);
	LDNS_FREE(ttl);
	LDNS_FREE(clas);
	LDNS_FREE(rdata);
	LDNS_FREE(rd);
	LDNS_FREE(rd_buf);
	LDNS_FREE(b64);
	ldns_rr_free(new);
    return status;
}