bool f_libxml_disable_entity_loader(bool disable /* = true */) {
  xmlParserInputBufferCreateFilenameFunc old;

  if (disable) {
    old = xmlParserInputBufferCreateFilenameDefault(hphp_libxml_input_buffer_noload);
  } else {
    old = xmlParserInputBufferCreateFilenameDefault(nullptr);
  }
  return (old == hphp_libxml_input_buffer_noload);
}