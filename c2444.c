make_backsig (PKT_signature *sig, PKT_public_key *pk,
              PKT_public_key *sub_pk, PKT_public_key *sub_psk,
              u32 timestamp, const char *cache_nonce)
{
  gpg_error_t err;
  PKT_signature *backsig;

  cache_public_key (sub_pk);

  err = make_keysig_packet (&backsig, pk, NULL, sub_pk, sub_psk, 0x19,
                            0, timestamp, 0, NULL, NULL, cache_nonce);
  if (err)
    log_error ("make_keysig_packet failed for backsig: %s\n",
               gpg_strerror (err));
  else
    {
      /* Get it into a binary packed form. */
      IOBUF backsig_out = iobuf_temp();
      PACKET backsig_pkt;

      init_packet (&backsig_pkt);
      backsig_pkt.pkttype = PKT_SIGNATURE;
      backsig_pkt.pkt.signature = backsig;
      err = build_packet (backsig_out, &backsig_pkt);
      free_packet (&backsig_pkt);
      if (err)
	log_error ("build_packet failed for backsig: %s\n", gpg_strerror (err));
      else
	{
	  size_t pktlen = 0;
	  byte *buf = iobuf_get_temp_buffer (backsig_out);

	  /* Remove the packet header. */
	  if(buf[0]&0x40)
	    {
	      if (buf[1] < 192)
		{
		  pktlen = buf[1];
		  buf += 2;
		}
	      else if(buf[1] < 224)
		{
		  pktlen = (buf[1]-192)*256;
		  pktlen += buf[2]+192;
		  buf += 3;
		}
	      else if (buf[1] == 255)
		{
		  pktlen  = buf[2] << 24;
		  pktlen |= buf[3] << 16;
		  pktlen |= buf[4] << 8;
		  pktlen |= buf[5];
		  buf += 6;
		}
	      else
		BUG ();
	    }
	  else
	    {
	      int mark = 1;

	      switch (buf[0]&3)
		{
		case 3:
		  BUG ();
		  break;

		case 2:
		  pktlen  = buf[mark++] << 24;
		  pktlen |= buf[mark++] << 16;

		case 1:
		  pktlen |= buf[mark++] << 8;

		case 0:
		  pktlen |= buf[mark++];
		}

	      buf += mark;
	    }

	  /* Now make the binary blob into a subpacket.  */
	  build_sig_subpkt (sig, SIGSUBPKT_SIGNATURE, buf, pktlen);

	  iobuf_close (backsig_out);
	}
    }

  return err;
}