void init_xml_relax_ng()
{
  VALUE nokogiri = rb_define_module("Nokogiri");
  VALUE xml = rb_define_module_under(nokogiri, "XML");
  VALUE klass = rb_define_class_under(xml, "RelaxNG", cNokogiriXmlSchema);

  cNokogiriXmlRelaxNG = klass;

  rb_define_singleton_method(klass, "read_memory", read_memory, 1);
  rb_define_singleton_method(klass, "from_document", from_document, 1);
  rb_define_private_method(klass, "validate_document", validate_document, 1);
}