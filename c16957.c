check_redir (ti)
     int ti;
{
  register int this_char, prev_char;

  /* Handle the two character tokens `>&', `<&', and `>|'.
     We are not in a command position after one of these. */
  this_char = rl_line_buffer[ti];
  prev_char = rl_line_buffer[ti - 1];

  if ((this_char == '&' && (prev_char == '<' || prev_char == '>')) ||
      (this_char == '|' && prev_char == '>'))
    return (1);
  else if (this_char == '{' && prev_char == '$') /*}*/
    return (1);
#if 0	/* Not yet */
  else if (this_char == '(' && prev_char == '$') /*)*/
    return (1);
  else if (this_char == '(' && prev_char == '<') /*)*/
    return (1);
#if defined (EXTENDED_GLOB)
  else if (extended_glob && this_char == '(' && prev_char == '!') /*)*/
    return (1);
#endif
#endif
  else if (char_is_quoted (rl_line_buffer, ti))
    return (1);
  return (0);
}