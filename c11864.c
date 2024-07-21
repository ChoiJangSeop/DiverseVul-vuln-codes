mes_lookup (struct message *meslist, int max, int index)
{
  /* first check for best case: index is in range and matches the key
     value in that slot */
  if ((index >= 0) && (index < max) && (meslist[index].key == index))
    return meslist[index].str;

  /* fall back to linear search */
  {
    int i;

    for (i = 0; i < max; i++, meslist++)
      {
	if (meslist->key == index)
	  {
	    zlog_warn("message index %d [%s] found in position %d (max is %d)",
		      index, meslist->str, i, max);
	    return meslist->str;
	  }
      }
  }
  zlog_err("message index %d not found (max is %d)", index, max);
  return NULL;
}