int mszip_decompress(struct mszip_stream *zip, off_t out_bytes) {
  /* for the bit buffer */
  register unsigned int bit_buffer;
  register int bits_left;
  unsigned char *i_ptr, *i_end;

  int i, ret, state, error;

  /* easy answers */
  if (!zip || (out_bytes < 0)) return CL_ENULLARG;
  if (zip->error) return zip->error;

  /* flush out any stored-up bytes before we begin */
  i = zip->o_end - zip->o_ptr;
  if ((off_t) i > out_bytes) i = (int) out_bytes;
  if (i) {
    if (zip->wflag && (ret = mspack_write(zip->ofd, zip->o_ptr, i, zip->file)) != CL_SUCCESS) {
      return zip->error = ret;
    }
    zip->o_ptr  += i;
    out_bytes   -= i;
  }
  if (out_bytes == 0) return CL_SUCCESS;

  while (out_bytes > 0) {
    /* unpack another block */
    MSZIP_RESTORE_BITS;

    /* skip to next read 'CK' header */
    i = bits_left & 7; MSZIP_REMOVE_BITS(i); /* align to bytestream */
    state = 0;
    do {
      MSZIP_READ_BITS(i, 8);
      if (i == 'C') state = 1;
      else if ((state == 1) && (i == 'K')) state = 2;
      else state = 0;
    } while (state != 2);

    /* inflate a block, repair and realign if necessary */
    zip->window_posn = 0;
    zip->bytes_output = 0;
    MSZIP_STORE_BITS;
    if ((error = mszip_inflate(zip))) {
      cli_dbgmsg("mszip_decompress: inflate error %d\n", error);
      if (zip->repair_mode) {
	cli_dbgmsg("mszip_decompress: MSZIP error, %u bytes of data lost\n",
			  MSZIP_FRAME_SIZE - zip->bytes_output);
	for (i = zip->bytes_output; i < MSZIP_FRAME_SIZE; i++) {
	  zip->window[i] = '\0';
	}
	zip->bytes_output = MSZIP_FRAME_SIZE;
      }
      else {
	return zip->error = (error > 0) ? error : CL_EFORMAT;
      }
    }
    zip->o_ptr = &zip->window[0];
    zip->o_end = &zip->o_ptr[zip->bytes_output];

    /* write a frame */
    i = (out_bytes < (off_t)zip->bytes_output) ?
      (int)out_bytes : zip->bytes_output;
    if (zip->wflag && (ret = mspack_write(zip->ofd, zip->o_ptr, i, zip->file)) != CL_SUCCESS) {
      return zip->error = ret;
    }

    /* mspack errors (i.e. read errors) are fatal and can't be recovered */
    if ((error > 0) && zip->repair_mode) return error;

    zip->o_ptr  += i;
    out_bytes   -= i;
  }

  if (out_bytes) {
    cli_dbgmsg("mszip_decompress: bytes left to output\n");
    return zip->error = CL_EFORMAT;
  }
  return CL_SUCCESS;
}