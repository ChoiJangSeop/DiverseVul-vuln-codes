static void xml_element_serialize(xml_element *el, int (*fptr)(void *data, const char *text, int size), void *data, XML_ELEM_OUTPUT_OPTIONS options, int depth)
{
   int i;
   static STRUCT_XML_ELEM_OUTPUT_OPTIONS default_opts = {xml_elem_pretty, xml_elem_markup_escaping | xml_elem_non_print_escaping, XML_DECL_ENCODING_DEFAULT};
   static char whitespace[] = "                                                                                               "
                              "                                                                                               "
                              "                                                                                               ";
   depth++;

   if(!el) {
      fprintf(stderr, "Nothing to write\n");
      return;
   }
   if(!options) {
      options = &default_opts;
   }

   /* print xml declaration if at root level */
   if(depth == 1) {
      xml_elem_writefunc(fptr, XML_DECL_START, data, XML_DECL_START_LEN);
      xml_elem_writefunc(fptr, WHITESPACE, data, WHITESPACE_LEN);
      xml_elem_writefunc(fptr, XML_DECL_VERSION, data, XML_DECL_VERSION_LEN);
      if(options->encoding && *options->encoding) {
          xml_elem_writefunc(fptr, WHITESPACE, data, WHITESPACE_LEN);
          xml_elem_writefunc(fptr, XML_DECL_ENCODING_ATTR, data, XML_DECL_ENCODING_ATTR_LEN);
          xml_elem_writefunc(fptr, EQUALS, data, EQUALS_LEN);
          xml_elem_writefunc(fptr, ATTR_DELIMITER, data, ATTR_DELIMITER_LEN);
          xml_elem_writefunc(fptr, options->encoding, data, 0);
          xml_elem_writefunc(fptr, ATTR_DELIMITER, data, ATTR_DELIMITER_LEN);
      }
      xml_elem_writefunc(fptr, XML_DECL_END, data, XML_DECL_END_LEN);
      if(options->verbosity != xml_elem_no_white_space) {
         xml_elem_writefunc(fptr, NEWLINE, data, NEWLINE_LEN);
      }
   }

   if(options->verbosity == xml_elem_pretty && depth > 2) {
         xml_elem_writefunc(fptr, whitespace, data, depth - 2);
   }
   /* begin element */
   xml_elem_writefunc(fptr,START_TOKEN_BEGIN, data, START_TOKEN_BEGIN_LEN);
   if(el->name) {
      xml_elem_writefunc(fptr, el->name, data, 0);

      /* write attrs, if any */
      if(Q_Size(&el->attrs)) {
         xml_element_attr* iter = Q_Head(&el->attrs);
         while( iter ) {
            xml_elem_writefunc(fptr, WHITESPACE, data, WHITESPACE_LEN);
            xml_elem_writefunc(fptr, iter->key, data, 0);
            xml_elem_writefunc(fptr, EQUALS, data, EQUALS_LEN);
            xml_elem_writefunc(fptr, ATTR_DELIMITER, data, ATTR_DELIMITER_LEN);
            xml_elem_writefunc(fptr, iter->val, data, 0);
            xml_elem_writefunc(fptr, ATTR_DELIMITER, data, ATTR_DELIMITER_LEN);

            iter = Q_Next(&el->attrs);
         }
      }
   }
   else {
      xml_elem_writefunc(fptr, "None", data, 0);
   }
   /* if no text and no children, use abbreviated form, eg: <foo/> */
   if(!el->text.len && !Q_Size(&el->children)) {
       xml_elem_writefunc(fptr, EMPTY_START_TOKEN_END, data, EMPTY_START_TOKEN_END_LEN);
   }
   /* otherwise, print element contents */
   else {
       xml_elem_writefunc(fptr, START_TOKEN_END, data, START_TOKEN_END_LEN);

       /* print text, if any */
       if(el->text.len) {
          char* escaped_str = el->text.str;
          int buflen = el->text.len;

          if(options->escaping && options->escaping != xml_elem_cdata_escaping) {
             escaped_str = xml_elem_entity_escape(el->text.str, buflen, &buflen, options->escaping );
             if(!escaped_str) {
                escaped_str = el->text.str;
             }
          }

          if(options->escaping & xml_elem_cdata_escaping) {
             xml_elem_writefunc(fptr, CDATA_BEGIN, data, CDATA_BEGIN_LEN);
          }

          xml_elem_writefunc(fptr, escaped_str, data, buflen);

          if(escaped_str != el->text.str) {
             my_free(escaped_str);
          }

          if(options->escaping & xml_elem_cdata_escaping) {
             xml_elem_writefunc(fptr, CDATA_END, data, CDATA_END_LEN);
          }
       }
       /* no text, so print child elems */
       else {
          xml_element *kids = Q_Head(&el->children);
          i = 0;
          while( kids ) {
             if(i++ == 0) {
                if(options->verbosity != xml_elem_no_white_space) {
                   xml_elem_writefunc(fptr, NEWLINE, data, NEWLINE_LEN);
                }
             }
             xml_element_serialize(kids, fptr, data, options, depth);
             kids = Q_Next(&el->children);
          }
          if(i) {
             if(options->verbosity == xml_elem_pretty && depth > 2) {
                   xml_elem_writefunc(fptr, whitespace, data, depth - 2);
             }
          }
       }

       xml_elem_writefunc(fptr, END_TOKEN_BEGIN, data, END_TOKEN_BEGIN_LEN);
       xml_elem_writefunc(fptr,el->name ? el->name : "None", data, 0);
       xml_elem_writefunc(fptr, END_TOKEN_END, data, END_TOKEN_END_LEN);
   }
   if(options->verbosity != xml_elem_no_white_space) {
      xml_elem_writefunc(fptr, NEWLINE, data, NEWLINE_LEN);
   }
}