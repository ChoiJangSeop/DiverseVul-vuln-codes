_gnutls_recv_handshake_header (gnutls_session_t session,
			       gnutls_handshake_description_t type,
			       gnutls_handshake_description_t * recv_type)
{
  int ret;
  uint32_t length32 = 0;
  uint8_t *dataptr = NULL;	/* for realloc */
  size_t handshake_header_size = HANDSHAKE_HEADER_SIZE;

  /* if we have data into the buffer then return them, do not read the next packet.
   * In order to return we need a full TLS handshake header, or in case of a version 2
   * packet, then we return the first byte.
   */
  if (session->internals.handshake_header_buffer.header_size ==
      handshake_header_size || (session->internals.v2_hello != 0
				&& type == GNUTLS_HANDSHAKE_CLIENT_HELLO
				&& session->internals.
				handshake_header_buffer.packet_length > 0))
    {

      *recv_type = session->internals.handshake_header_buffer.recv_type;

      return session->internals.handshake_header_buffer.packet_length;
    }

  /* Note: SSL2_HEADERS == 1 */

  dataptr = session->internals.handshake_header_buffer.header;

  /* If we haven't already read the handshake headers.
   */
  if (session->internals.handshake_header_buffer.header_size < SSL2_HEADERS)
    {
      ret =
	_gnutls_handshake_io_recv_int (session, GNUTLS_HANDSHAKE,
				       type, dataptr, SSL2_HEADERS);

      if (ret < 0)
	{
	  gnutls_assert ();
	  return ret;
	}

      /* The case ret==0 is caught here.
       */
      if (ret != SSL2_HEADERS)
	{
	  gnutls_assert ();
	  return GNUTLS_E_UNEXPECTED_PACKET_LENGTH;
	}
      session->internals.handshake_header_buffer.header_size = SSL2_HEADERS;
    }

  if (session->internals.v2_hello == 0
      || type != GNUTLS_HANDSHAKE_CLIENT_HELLO)
    {
      ret =
	_gnutls_handshake_io_recv_int (session, GNUTLS_HANDSHAKE,
				       type,
				       &dataptr[session->
						internals.
						handshake_header_buffer.
						header_size],
				       HANDSHAKE_HEADER_SIZE -
				       session->internals.
				       handshake_header_buffer.header_size);
      if (ret <= 0)
	{
	  gnutls_assert ();
	  return (ret < 0) ? ret : GNUTLS_E_UNEXPECTED_PACKET_LENGTH;
	}
      if ((size_t) ret !=
	  HANDSHAKE_HEADER_SIZE -
	  session->internals.handshake_header_buffer.header_size)
	{
	  gnutls_assert ();
	  return GNUTLS_E_UNEXPECTED_PACKET_LENGTH;
	}
      *recv_type = dataptr[0];

      /* we do not use DECR_LEN because we know
       * that the packet has enough data.
       */
      length32 = _gnutls_read_uint24 (&dataptr[1]);
      handshake_header_size = HANDSHAKE_HEADER_SIZE;

      _gnutls_handshake_log ("HSK[%x]: %s was received [%ld bytes]\n",
			     session, _gnutls_handshake2str (dataptr[0]),
			     length32 + HANDSHAKE_HEADER_SIZE);

    }
  else
    {				/* v2 hello */
      length32 = session->internals.v2_hello - SSL2_HEADERS;	/* we've read the first byte */

      handshake_header_size = SSL2_HEADERS;	/* we've already read one byte */

      *recv_type = dataptr[0];

      _gnutls_handshake_log ("HSK[%x]: %s(v2) was received [%ld bytes]\n",
			     session, _gnutls_handshake2str (*recv_type),
			     length32 + handshake_header_size);

      if (*recv_type != GNUTLS_HANDSHAKE_CLIENT_HELLO)
	{			/* it should be one or nothing */
	  gnutls_assert ();
	  return GNUTLS_E_UNEXPECTED_HANDSHAKE_PACKET;
	}
    }

  /* put the packet into the buffer */
  session->internals.handshake_header_buffer.header_size =
    handshake_header_size;
  session->internals.handshake_header_buffer.packet_length = length32;
  session->internals.handshake_header_buffer.recv_type = *recv_type;

  if (*recv_type != type)
    {
      gnutls_assert ();
      return GNUTLS_E_UNEXPECTED_HANDSHAKE_PACKET;
    }

  return length32;
}