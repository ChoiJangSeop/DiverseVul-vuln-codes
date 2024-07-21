static VALUE read_memory(VALUE klass, VALUE content)
{
  xmlRelaxNGParserCtxtPtr ctx = xmlRelaxNGNewMemParserCtxt(
      (const char *)StringValuePtr(content),
      (int)RSTRING_LEN(content)
  );
  xmlRelaxNGPtr schema;
  VALUE errors = rb_ary_new();
  VALUE rb_schema;

  xmlSetStructuredErrorFunc((void *)errors, Nokogiri_error_array_pusher);

#ifdef HAVE_XMLRELAXNGSETPARSERSTRUCTUREDERRORS
  xmlRelaxNGSetParserStructuredErrors(
    ctx,
    Nokogiri_error_array_pusher,
    (void *)errors
  );
#endif

  schema = xmlRelaxNGParse(ctx);

  xmlSetStructuredErrorFunc(NULL, NULL);
  xmlRelaxNGFreeParserCtxt(ctx);

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