void init_xml_schema()
{
  VALUE nokogiri = rb_define_module("Nokogiri");
  VALUE xml = rb_define_module_under(nokogiri, "XML");
  VALUE klass = rb_define_class_under(xml, "Schema", rb_cObject);

  cNokogiriXmlSchema = klass;

  rb_define_singleton_method(klass, "read_memory", read_memory, 1);
  rb_define_singleton_method(klass, "from_document", from_document, 1);

  rb_define_private_method(klass, "validate_document", validate_document, 1);
  rb_define_private_method(klass, "validate_file",     validate_file, 1);
}