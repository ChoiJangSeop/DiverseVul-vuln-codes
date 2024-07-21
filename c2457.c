b64enc_write (struct b64state *state, const void *buffer, size_t nbytes)
{
  unsigned char radbuf[4];
  int idx, quad_count;
  const unsigned char *p;

  if (state->lasterr)
    return state->lasterr;

  if (!nbytes)
    {
      if (buffer)
        if (state->stream? es_fflush (state->stream) : fflush (state->fp))
          goto write_error;
      return 0;
    }

  if (!(state->flags & B64ENC_DID_HEADER))
    {
      if (state->title)
        {
          if ( my_fputs ("-----BEGIN ", state) == EOF
               || my_fputs (state->title, state) == EOF
               || my_fputs ("-----\n", state) == EOF)
            goto write_error;
          if ( (state->flags & B64ENC_USE_PGPCRC)
               && my_fputs ("\n", state) == EOF)
            goto write_error;
        }

      state->flags |= B64ENC_DID_HEADER;
    }

  idx = state->idx;
  quad_count = state->quad_count;
  assert (idx < 4);
  memcpy (radbuf, state->radbuf, idx);

  if ( (state->flags & B64ENC_USE_PGPCRC) )
    {
      size_t n;
      u32 crc = state->crc;

      for (p=buffer, n=nbytes; n; p++, n-- )
        crc = (crc << 8) ^ crc_table[((crc >> 16)&0xff) ^ *p];
      state->crc = (crc & 0x00ffffff);
    }

  for (p=buffer; nbytes; p++, nbytes--)
    {
      radbuf[idx++] = *p;
      if (idx > 2)
        {
          char tmp[4];

          tmp[0] = bintoasc[(*radbuf >> 2) & 077];
          tmp[1] = bintoasc[(((*radbuf<<4)&060)|((radbuf[1] >> 4)&017))&077];
          tmp[2] = bintoasc[(((radbuf[1]<<2)&074)|((radbuf[2]>>6)&03))&077];
          tmp[3] = bintoasc[radbuf[2]&077];
          if (state->stream)
            {
              for (idx=0; idx < 4; idx++)
                es_putc (tmp[idx], state->stream);
              idx = 0;
              if (es_ferror (state->stream))
                goto write_error;
            }
          else
            {
              for (idx=0; idx < 4; idx++)
                putc (tmp[idx], state->fp);
              idx = 0;
              if (ferror (state->fp))
                goto write_error;
            }
          if (++quad_count >= (64/4))
            {
              quad_count = 0;
              if (!(state->flags & B64ENC_NO_LINEFEEDS)
                  && my_fputs ("\n", state) == EOF)
                goto write_error;
            }
        }
    }
  memcpy (state->radbuf, radbuf, idx);
  state->idx = idx;
  state->quad_count = quad_count;
  return 0;

 write_error:
  state->lasterr = gpg_error_from_syserror ();
  if (state->title)
    {
      xfree (state->title);
      state->title = NULL;
    }
  return state->lasterr;
}