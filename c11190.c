storeRawNames(XML_Parser parser) {
  TAG *tag = parser->m_tagStack;
  while (tag) {
    int bufSize;
    int nameLen = sizeof(XML_Char) * (tag->name.strLen + 1);
    char *rawNameBuf = tag->buf + nameLen;
    /* Stop if already stored.  Since m_tagStack is a stack, we can stop
       at the first entry that has already been copied; everything
       below it in the stack is already been accounted for in a
       previous call to this function.
    */
    if (tag->rawName == rawNameBuf)
      break;
    /* For re-use purposes we need to ensure that the
       size of tag->buf is a multiple of sizeof(XML_Char).
    */
    bufSize = nameLen + ROUND_UP(tag->rawNameLength, sizeof(XML_Char));
    if (bufSize > tag->bufEnd - tag->buf) {
      char *temp = (char *)REALLOC(parser, tag->buf, bufSize);
      if (temp == NULL)
        return XML_FALSE;
      /* if tag->name.str points to tag->buf (only when namespace
         processing is off) then we have to update it
      */
      if (tag->name.str == (XML_Char *)tag->buf)
        tag->name.str = (XML_Char *)temp;
      /* if tag->name.localPart is set (when namespace processing is on)
         then update it as well, since it will always point into tag->buf
      */
      if (tag->name.localPart)
        tag->name.localPart
            = (XML_Char *)temp + (tag->name.localPart - (XML_Char *)tag->buf);
      tag->buf = temp;
      tag->bufEnd = temp + bufSize;
      rawNameBuf = temp + nameLen;
    }
    memcpy(rawNameBuf, tag->rawName, tag->rawNameLength);
    tag->rawName = rawNameBuf;
    tag = tag->parent;
  }
  return XML_TRUE;
}