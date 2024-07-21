keyring_get_keyblock (KEYRING_HANDLE hd, KBNODE *ret_kb)
{
    PACKET *pkt;
    int rc;
    KBNODE keyblock = NULL, node, lastnode;
    IOBUF a;
    int in_cert = 0;
    int pk_no = 0;
    int uid_no = 0;
    int save_mode;

    if (ret_kb)
        *ret_kb = NULL;

    if (!hd->found.kr)
        return -1; /* no successful search */

    a = iobuf_open (hd->found.kr->fname);
    if (!a)
      {
	log_error(_("can't open '%s'\n"), hd->found.kr->fname);
	return GPG_ERR_KEYRING_OPEN;
      }

    if (iobuf_seek (a, hd->found.offset) ) {
        log_error ("can't seek '%s'\n", hd->found.kr->fname);
	iobuf_close(a);
	return GPG_ERR_KEYRING_OPEN;
    }

    pkt = xmalloc (sizeof *pkt);
    init_packet (pkt);
    hd->found.n_packets = 0;;
    lastnode = NULL;
    save_mode = set_packet_list_mode(0);
    while ((rc=parse_packet (a, pkt)) != -1) {
        hd->found.n_packets++;
        if (gpg_err_code (rc) == GPG_ERR_UNKNOWN_PACKET) {
	    free_packet (pkt);
	    init_packet (pkt);
	    continue;
	}
        if (gpg_err_code (rc) == GPG_ERR_LEGACY_KEY)
          break;  /* Upper layer needs to handle this.  */
	if (rc) {
            log_error ("keyring_get_keyblock: read error: %s\n",
                       gpg_strerror (rc) );
            rc = GPG_ERR_INV_KEYRING;
            break;
        }
	if (pkt->pkttype == PKT_COMPRESSED) {
	    log_error ("skipped compressed packet in keyring\n");
	    free_packet(pkt);
	    init_packet(pkt);
	    continue;
        }

        if (in_cert && (pkt->pkttype == PKT_PUBLIC_KEY
                        || pkt->pkttype == PKT_SECRET_KEY)) {
            hd->found.n_packets--; /* fix counter */
            break; /* ready */
        }

        in_cert = 1;
        if (pkt->pkttype == PKT_RING_TRUST)
          {
            /*(this code is duplicated after the loop)*/
            if ( lastnode
                 && lastnode->pkt->pkttype == PKT_SIGNATURE
                 && (pkt->pkt.ring_trust->sigcache & 1) ) {
                /* This is a ring trust packet with a checked signature
                 * status cache following directly a signature paket.
                 * Set the cache status into that signature packet.  */
                PKT_signature *sig = lastnode->pkt->pkt.signature;

                sig->flags.checked = 1;
                sig->flags.valid = !!(pkt->pkt.ring_trust->sigcache & 2);
            }
            /* Reset LASTNODE, so that we set the cache status only from
             * the ring trust packet immediately following a signature. */
            lastnode = NULL;
	    free_packet(pkt);
	    init_packet(pkt);
	    continue;
          }


        node = lastnode = new_kbnode (pkt);
        if (!keyblock)
          keyblock = node;
        else
          add_kbnode (keyblock, node);
        switch (pkt->pkttype)
          {
          case PKT_PUBLIC_KEY:
          case PKT_PUBLIC_SUBKEY:
          case PKT_SECRET_KEY:
          case PKT_SECRET_SUBKEY:
            if (++pk_no == hd->found.pk_no)
              node->flag |= 1;
            break;

          case PKT_USER_ID:
            if (++uid_no == hd->found.uid_no)
              node->flag |= 2;
            break;

          default:
            break;
          }

        pkt = xmalloc (sizeof *pkt);
        init_packet(pkt);
    }
    set_packet_list_mode(save_mode);

    if (rc == -1 && keyblock)
	rc = 0; /* got the entire keyblock */

    if (rc || !ret_kb)
	release_kbnode (keyblock);
    else {
        /*(duplicated form the loop body)*/
        if ( pkt && pkt->pkttype == PKT_RING_TRUST
             && lastnode
             && lastnode->pkt->pkttype == PKT_SIGNATURE
             && (pkt->pkt.ring_trust->sigcache & 1) ) {
            PKT_signature *sig = lastnode->pkt->pkt.signature;
            sig->flags.checked = 1;
            sig->flags.valid = !!(pkt->pkt.ring_trust->sigcache & 2);
        }
	*ret_kb = keyblock;
    }
    free_packet (pkt);
    xfree (pkt);
    iobuf_close(a);

    /* Make sure that future search operations fail immediately when
     * we know that we are working on a invalid keyring
     */
    if (gpg_err_code (rc) == GPG_ERR_INV_KEYRING)
        hd->current.error = rc;

    return rc;
}