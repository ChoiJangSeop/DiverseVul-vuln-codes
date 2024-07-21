CURLcode Curl_dupset(struct SessionHandle *dst, struct SessionHandle *src)
{
  CURLcode result = CURLE_OK;
  enum dupstring i;

  /* Copy src->set into dst->set first, then deal with the strings
     afterwards */
  dst->set = src->set;

  /* clear all string pointers first */
  memset(dst->set.str, 0, STRING_LAST * sizeof(char *));

  /* duplicate all strings */
  for(i=(enum dupstring)0; i< STRING_LAST; i++) {
    result = setstropt(&dst->set.str[i], src->set.str[i]);
    if(result)
      break;
  }

  /* If a failure occurred, freeing has to be performed externally. */
  return result;
}