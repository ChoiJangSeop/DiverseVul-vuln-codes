static VALUE from_document(VALUE klass, VALUE document)
{
  xmlDocPtr doc;
  xmlSchemaParserCtxtPtr ctx;
  xmlSchemaPtr schema;
  VALUE errors;
  VALUE rb_schema;

  Data_Get_Struct(document, xmlDoc, doc);

  /* In case someone passes us a node. ugh. */
  doc = doc->doc;

  if (has_blank_nodes_p(DOC_NODE_CACHE(doc))) {
    rb_raise(rb_eArgError, "Creating a schema from a document that has blank nodes exposed to Ruby is dangerous");
  }

  ctx = xmlSchemaNewDocParserCtxt(doc);

  errors = rb_ary_new();
  xmlSetStructuredErrorFunc((void *)errors, Nokogiri_error_array_pusher);

#ifdef HAVE_XMLSCHEMASETPARSERSTRUCTUREDERRORS
  xmlSchemaSetParserStructuredErrors(
    ctx,
    Nokogiri_error_array_pusher,
    (void *)errors
  );
#endif

  schema = xmlSchemaParse(ctx);

  xmlSetStructuredErrorFunc(NULL, NULL);
  xmlSchemaFreeParserCtxt(ctx);

  if(NULL == schema) {
    xmlErrorPtr error = xmlGetLastError();
    if(error)
      Nokogiri_error_raise(NULL, error);
    else
      rb_raise(rb_eRuntimeError, "Could not parse document");

    return Qnil;
  }

  rb_schema = Data_Wrap_Struct(klass, 0, dealloc, schema);
  rb_iv_set(rb_schema, "@errors", errors);

  return rb_schema;

  return Qnil;
}