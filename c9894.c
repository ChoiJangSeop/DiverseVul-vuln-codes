static VALUE read_memory(VALUE klass, VALUE content)
{
  xmlSchemaPtr schema;
  xmlSchemaParserCtxtPtr ctx = xmlSchemaNewMemParserCtxt(
      (const char *)StringValuePtr(content),
      (int)RSTRING_LEN(content)
  );
  VALUE rb_schema;
  VALUE errors = rb_ary_new();
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
}