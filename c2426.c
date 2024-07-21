parse_keyblock_image (iobuf_t iobuf, int pk_no, int uid_no,
                      const u32 *sigstatus, kbnode_t *r_keyblock)
{
  gpg_error_t err;
  PACKET *pkt;
  kbnode_t keyblock = NULL;
  kbnode_t node, *tail;
  int in_cert, save_mode;
  u32 n_sigs;
  int pk_count, uid_count;

  *r_keyblock = NULL;

  pkt = xtrymalloc (sizeof *pkt);
  if (!pkt)
    return gpg_error_from_syserror ();
  init_packet (pkt);
  save_mode = set_packet_list_mode (0);
  in_cert = 0;
  n_sigs = 0;
  tail = NULL;
  pk_count = uid_count = 0;
  while ((err = parse_packet (iobuf, pkt)) != -1)
    {
      if (gpg_err_code (err) == GPG_ERR_UNKNOWN_PACKET)
        {
          free_packet (pkt);
          init_packet (pkt);
          continue;
	}
      if (err)
        {
          log_error ("parse_keyblock_image: read error: %s\n",
                     gpg_strerror (err));
          err = gpg_error (GPG_ERR_INV_KEYRING);
          break;
        }
      if (pkt->pkttype == PKT_COMPRESSED)
        {
          log_error ("skipped compressed packet in keybox blob\n");
          free_packet(pkt);
          init_packet(pkt);
          continue;
        }
      if (pkt->pkttype == PKT_RING_TRUST)
        {
          log_info ("skipped ring trust packet in keybox blob\n");
          free_packet(pkt);
          init_packet(pkt);
          continue;
        }

      if (!in_cert && pkt->pkttype != PKT_PUBLIC_KEY)
        {
          log_error ("parse_keyblock_image: first packet in a keybox blob "
                     "is not a public key packet\n");
          err = gpg_error (GPG_ERR_INV_KEYRING);
          break;
        }
      if (in_cert && (pkt->pkttype == PKT_PUBLIC_KEY
                      || pkt->pkttype == PKT_SECRET_KEY))
        {
          log_error ("parse_keyblock_image: "
                     "multiple keyblocks in a keybox blob\n");
          err = gpg_error (GPG_ERR_INV_KEYRING);
          break;
        }
      in_cert = 1;

      if (pkt->pkttype == PKT_SIGNATURE && sigstatus)
        {
          PKT_signature *sig = pkt->pkt.signature;

          n_sigs++;
          if (n_sigs > sigstatus[0])
            {
              log_error ("parse_keyblock_image: "
                         "more signatures than found in the meta data\n");
              err = gpg_error (GPG_ERR_INV_KEYRING);
              break;

            }
          if (sigstatus[n_sigs])
            {
              sig->flags.checked = 1;
              if (sigstatus[n_sigs] == 1 )
                ; /* missing key */
              else if (sigstatus[n_sigs] == 2 )
                ; /* bad signature */
              else if (sigstatus[n_sigs] < 0x10000000)
                ; /* bad flag */
              else
                {
                  sig->flags.valid = 1;
                  /* Fixme: Shall we set the expired flag here?  */
                }
            }
        }

      node = new_kbnode (pkt);

      switch (pkt->pkttype)
        {
        case PKT_PUBLIC_KEY:
        case PKT_PUBLIC_SUBKEY:
        case PKT_SECRET_KEY:
        case PKT_SECRET_SUBKEY:
          if (++pk_count == pk_no)
            node->flag |= 1;
          break;

        case PKT_USER_ID:
          if (++uid_count == uid_no)
            node->flag |= 2;
          break;

        default:
          break;
        }

      if (!keyblock)
        keyblock = node;
      else
        *tail = node;
      tail = &node->next;
      pkt = xtrymalloc (sizeof *pkt);
      if (!pkt)
        {
          err = gpg_error_from_syserror ();
          break;
        }
      init_packet (pkt);
    }
  set_packet_list_mode (save_mode);

  if (err == -1 && keyblock)
    err = 0; /* Got the entire keyblock.  */

  if (!err && sigstatus && n_sigs != sigstatus[0])
    {
      log_error ("parse_keyblock_image: signature count does not match\n");
      err = gpg_error (GPG_ERR_INV_KEYRING);
    }

  if (err)
    release_kbnode (keyblock);
  else
    *r_keyblock = keyblock;
  free_packet (pkt);
  xfree (pkt);
  return err;
}