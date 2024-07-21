int ndpi_netbios_name_interpret(char *in, size_t in_len, char *out, u_int out_len) {
  u_int ret = 0, len, idx = in_len, out_idx = 0;

  len = (*in++)/2;
  out_len--;
  out[out_idx] = 0;

  if((len > out_len) || (len < 1) || ((2*len) > in_len))
    return(-1);

  while((len--) && (out_idx < out_len)) {
    if((idx < 2) || (in[0] < 'A') || (in[0] > 'P') || (in[1] < 'A') || (in[1] > 'P')) {
      out[out_idx] = 0;
      break;
    }

    out[out_idx] = ((in[0] - 'A') << 4) + (in[1] - 'A');
    in += 2, idx -= 2;

    if(isprint(out[out_idx]))
      out_idx++, ret++;
  }

  /* Trim trailing whitespace from the returned string */
  if(out_idx > 0) {
    out[out_idx] = 0;
    out_idx--;

    while((out_idx > 0) && (out[out_idx] == ' ')) {
      out[out_idx] = 0;
      out_idx--;
    }

  }

  return(ret);
}