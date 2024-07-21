read_block( IOBUF a, PACKET **pending_pkt, KBNODE *ret_root )
{
    int rc;
    PACKET *pkt;
    KBNODE root = NULL;
    int in_cert;

    if( *pending_pkt ) {
	root = new_kbnode( *pending_pkt );
	*pending_pkt = NULL;
	in_cert = 1;
    }
    else
	in_cert = 0;
    pkt = xmalloc( sizeof *pkt );
    init_packet(pkt);
    while( (rc=parse_packet(a, pkt)) != -1 ) {
	if( rc ) {  /* ignore errors */
	    if( rc != G10ERR_UNKNOWN_PACKET ) {
		log_error("read_block: read error: %s\n", g10_errstr(rc) );
		rc = G10ERR_INV_KEYRING;
		goto ready;
	    }
	    free_packet( pkt );
	    init_packet(pkt);
	    continue;
	}

	if( !root && pkt->pkttype == PKT_SIGNATURE
		  && pkt->pkt.signature->sig_class == 0x20 ) {
	    /* this is a revocation certificate which is handled
	     * in a special way */
	    root = new_kbnode( pkt );
	    pkt = NULL;
	    goto ready;
	}

	/* make a linked list of all packets */
	switch( pkt->pkttype ) {
	  case PKT_COMPRESSED:
	    if(check_compress_algo(pkt->pkt.compressed->algorithm))
	      {
		rc = G10ERR_COMPR_ALGO;
		goto ready;
	      }
	    else
	      {
		compress_filter_context_t *cfx = xmalloc_clear( sizeof *cfx );
		pkt->pkt.compressed->buf = NULL;
		push_compress_filter2(a,cfx,pkt->pkt.compressed->algorithm,1);
	      }
	    free_packet( pkt );
	    init_packet(pkt);
	    break;

          case PKT_RING_TRUST:
            /* skip those packets */
	    free_packet( pkt );
	    init_packet(pkt);
            break;

	  case PKT_PUBLIC_KEY:
	  case PKT_SECRET_KEY:
	    if( in_cert ) { /* store this packet */
		*pending_pkt = pkt;
		pkt = NULL;
		goto ready;
	    }
	    in_cert = 1;
	  default:
	    if( in_cert ) {
		if( !root )
		    root = new_kbnode( pkt );
		else
		    add_kbnode( root, new_kbnode( pkt ) );
		pkt = xmalloc( sizeof *pkt );
	    }
	    init_packet(pkt);
	    break;
	}
    }
  ready:
    if( rc == -1 && root )
	rc = 0;

    if( rc )
	release_kbnode( root );
    else
	*ret_root = root;
    free_packet( pkt );
    xfree( pkt );
    return rc;
}