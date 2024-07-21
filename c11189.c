copyString(const XML_Char *s, const XML_Memory_Handling_Suite *memsuite) {
  int charsRequired = 0;
  XML_Char *result;

  /* First determine how long the string is */
  while (s[charsRequired] != 0) {
    charsRequired++;
  }
  /* Include the terminator */
  charsRequired++;

  /* Now allocate space for the copy */
  result = memsuite->malloc_fcn(charsRequired * sizeof(XML_Char));
  if (result == NULL)
    return NULL;
  /* Copy the original into place */
  memcpy(result, s, charsRequired * sizeof(XML_Char));
  return result;
}