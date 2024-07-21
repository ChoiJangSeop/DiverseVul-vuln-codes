tdbio_read_record( ulong recnum, TRUSTREC *rec, int expected )
{
    byte readbuf[TRUST_RECORD_LEN];
    const byte *buf, *p;
    gpg_error_t err = 0;
    int n, i;

    if( db_fd == -1 )
	open_db();
    buf = get_record_from_cache( recnum );
    if( !buf ) {
	if( lseek( db_fd, recnum * TRUST_RECORD_LEN, SEEK_SET ) == -1 ) {
            err = gpg_error_from_syserror ();
	    log_error(_("trustdb: lseek failed: %s\n"), strerror(errno) );
	    return err;
	}
	n = read( db_fd, readbuf, TRUST_RECORD_LEN);
	if( !n ) {
	    return -1; /* eof */
	}
	else if( n != TRUST_RECORD_LEN ) {
            err = gpg_error_from_syserror ();
	    log_error(_("trustdb: read failed (n=%d): %s\n"), n,
							strerror(errno) );
	    return err;
	}
	buf = readbuf;
    }
    rec->recnum = recnum;
    rec->dirty = 0;
    p = buf;
    rec->rectype = *p++;
    if( expected && rec->rectype != expected ) {
	log_error("%lu: read expected rec type %d, got %d\n",
		    recnum, expected, rec->rectype );
	return gpg_error (GPG_ERR_TRUSTDB);
    }
    p++;    /* skip reserved byte */
    switch( rec->rectype ) {
      case 0:  /* unused (free) record */
	break;
      case RECTYPE_VER: /* version record */
	if( memcmp(buf+1, GPGEXT_GPG, 3 ) ) {
	    log_error( _("%s: not a trustdb file\n"), db_name );
	    err = gpg_error (GPG_ERR_TRUSTDB);
	}
	p += 2; /* skip "gpg" */
	rec->r.ver.version  = *p++;
	rec->r.ver.marginals = *p++;
	rec->r.ver.completes = *p++;
	rec->r.ver.cert_depth = *p++;
	rec->r.ver.trust_model = *p++;
	rec->r.ver.min_cert_level = *p++;
	p += 2;
	rec->r.ver.created  = buftoulong(p); p += 4;
	rec->r.ver.nextcheck = buftoulong(p); p += 4;
	p += 4;
	p += 4;
	rec->r.ver.firstfree =buftoulong(p); p += 4;
	p += 4;
	rec->r.ver.trusthashtbl =buftoulong(p); p += 4;
	if( recnum ) {
	    log_error( _("%s: version record with recnum %lu\n"), db_name,
							     (ulong)recnum );
	    err = gpg_error (GPG_ERR_TRUSTDB);
	}
	else if( rec->r.ver.version != 3 ) {
	    log_error( _("%s: invalid file version %d\n"), db_name,
							rec->r.ver.version );
	    err = gpg_error (GPG_ERR_TRUSTDB);
	}
	break;
      case RECTYPE_FREE:
	rec->r.free.next  = buftoulong(p); p += 4;
	break;
      case RECTYPE_HTBL:
	for(i=0; i < ITEMS_PER_HTBL_RECORD; i++ ) {
	    rec->r.htbl.item[i] = buftoulong(p); p += 4;
	}
	break;
      case RECTYPE_HLST:
	rec->r.hlst.next = buftoulong(p); p += 4;
	for(i=0; i < ITEMS_PER_HLST_RECORD; i++ ) {
	    rec->r.hlst.rnum[i] = buftoulong(p); p += 4;
	}
	break;
      case RECTYPE_TRUST:
	memcpy( rec->r.trust.fingerprint, p, 20); p+=20;
        rec->r.trust.ownertrust = *p++;
        rec->r.trust.depth = *p++;
        rec->r.trust.min_ownertrust = *p++;
        p++;
	rec->r.trust.validlist = buftoulong(p); p += 4;
	break;
      case RECTYPE_VALID:
	memcpy( rec->r.valid.namehash, p, 20); p+=20;
        rec->r.valid.validity = *p++;
	rec->r.valid.next = buftoulong(p); p += 4;
	rec->r.valid.full_count = *p++;
	rec->r.valid.marginal_count = *p++;
	break;
      default:
	log_error( "%s: invalid record type %d at recnum %lu\n",
				   db_name, rec->rectype, (ulong)recnum );
	err = gpg_error (GPG_ERR_TRUSTDB);
	break;
    }

    return err;
}