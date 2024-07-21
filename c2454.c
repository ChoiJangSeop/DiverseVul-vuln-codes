delete_sig_subpkt (subpktarea_t *area, sigsubpkttype_t reqtype )
{
    int buflen;
    sigsubpkttype_t type;
    byte *buffer, *bufstart;
    size_t n;
    size_t unused = 0;
    int okay = 0;

    if( !area )
	return 0;
    buflen = area->len;
    buffer = area->data;
    for(;;) {
	if( !buflen ) {
            okay = 1;
            break;
        }
	bufstart = buffer;
	n = *buffer++; buflen--;
	if( n == 255 ) {
	    if( buflen < 4 )
		break;
	    n = (((size_t)buffer[0] << 24)
                 | (buffer[1] << 16)
                 | (buffer[2] << 8)
                 | buffer[3]);
	    buffer += 4;
	    buflen -= 4;
	}
	else if( n >= 192 ) {
	    if( buflen < 2 )
		break;
	    n = (( n - 192 ) << 8) + *buffer + 192;
	    buffer++;
	    buflen--;
	}
	if( buflen < n )
	    break;

	type = *buffer & 0x7f;
	if( type == reqtype ) {
	    buffer++;
            buflen--;
	    n--;
	    if( n > buflen )
		break;
            buffer += n; /* point to next subpkt */
            buflen -= n;
            memmove (bufstart, buffer, buflen); /* shift */
            unused +=  buffer - bufstart;
            buffer = bufstart;
	}
        else {
            buffer += n; buflen -=n;
        }
    }

    if (!okay)
        log_error ("delete_subpkt: buffer shorter than subpacket\n");
    assert (unused <= area->len);
    area->len -= unused;
    return !!unused;
}