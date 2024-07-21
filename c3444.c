print_arrays_for(char *set)
{
  FILE *f;

  sprintf(buf, "%s.conf", set);

  if ((f = fopen(buf, "r")) == NULL) {
    fprintf(stderr, "%s: can't read conf file for charset %s\n", prog, set);
    exit(EXIT_FAILURE);
  }

  printf("\
/* The %s character set.  Generated automatically by configure and\n\
 * the %s program\n\
 */\n\n",
	 set, prog);

  /* it would be nice if this used the code in mysys/charset.c, but... */
  print_array(f, set, "ctype",      CTYPE_TABLE_SIZE);
  print_array(f, set, "to_lower",   TO_LOWER_TABLE_SIZE);
  print_array(f, set, "to_upper",   TO_UPPER_TABLE_SIZE);
  print_array(f, set, "sort_order", SORT_ORDER_TABLE_SIZE);
  printf("\n");

  fclose(f);

  return;
}