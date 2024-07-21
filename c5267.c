void t_cpp_generator::generate_struct_reader(ofstream& out, t_struct* tstruct, bool pointers) {
  if (gen_templates_) {
    out << indent() << "template <class Protocol_>" << endl << indent() << "uint32_t "
        << tstruct->get_name() << "::read(Protocol_* iprot) {" << endl;
  } else {
    indent(out) << "uint32_t " << tstruct->get_name()
                << "::read(::apache::thrift::protocol::TProtocol* iprot) {" << endl;
  }
  indent_up();

  const vector<t_field*>& fields = tstruct->get_members();
  vector<t_field*>::const_iterator f_iter;

  // Declare stack tmp variables
  out << endl << indent() << "uint32_t xfer = 0;" << endl << indent() << "std::string fname;"
      << endl << indent() << "::apache::thrift::protocol::TType ftype;" << endl << indent()
      << "int16_t fid;" << endl << endl << indent() << "xfer += iprot->readStructBegin(fname);"
      << endl << endl << indent() << "using ::apache::thrift::protocol::TProtocolException;" << endl
      << endl;

  // Required variables aren't in __isset, so we need tmp vars to check them.
  for (f_iter = fields.begin(); f_iter != fields.end(); ++f_iter) {
    if ((*f_iter)->get_req() == t_field::T_REQUIRED)
      indent(out) << "bool isset_" << (*f_iter)->get_name() << " = false;" << endl;
  }
  out << endl;

  // Loop over reading in fields
  indent(out) << "while (true)" << endl;
  scope_up(out);

  // Read beginning field marker
  indent(out) << "xfer += iprot->readFieldBegin(fname, ftype, fid);" << endl;

  // Check for field STOP marker
  out << indent() << "if (ftype == ::apache::thrift::protocol::T_STOP) {" << endl << indent()
      << "  break;" << endl << indent() << "}" << endl;

  if (fields.empty()) {
    out << indent() << "xfer += iprot->skip(ftype);" << endl;
  } else {
    // Switch statement on the field we are reading
    indent(out) << "switch (fid)" << endl;

    scope_up(out);

    // Generate deserialization code for known cases
    for (f_iter = fields.begin(); f_iter != fields.end(); ++f_iter) {
      indent(out) << "case " << (*f_iter)->get_key() << ":" << endl;
      indent_up();
      indent(out) << "if (ftype == " << type_to_enum((*f_iter)->get_type()) << ") {" << endl;
      indent_up();

      const char* isset_prefix = ((*f_iter)->get_req() != t_field::T_REQUIRED) ? "this->__isset."
                                                                               : "isset_";

#if 0
          // This code throws an exception if the same field is encountered twice.
          // We've decided to leave it out for performance reasons.
          // TODO(dreiss): Generate this code and "if" it out to make it easier
          // for people recompiling thrift to include it.
          out <<
            indent() << "if (" << isset_prefix << (*f_iter)->get_name() << ")" << endl <<
            indent() << "  throw TProtocolException(TProtocolException::INVALID_DATA);" << endl;
#endif

      if (pointers && !(*f_iter)->get_type()->is_xception()) {
        generate_deserialize_field(out, *f_iter, "(*(this->", "))");
      } else {
        generate_deserialize_field(out, *f_iter, "this->");
      }
      out << indent() << isset_prefix << (*f_iter)->get_name() << " = true;" << endl;
      indent_down();
      out << indent() << "} else {" << endl << indent() << "  xfer += iprot->skip(ftype);" << endl
          <<
          // TODO(dreiss): Make this an option when thrift structs
          // have a common base class.
          // indent() << "  throw TProtocolException(TProtocolException::INVALID_DATA);" << endl <<
          indent() << "}" << endl << indent() << "break;" << endl;
      indent_down();
    }

    // In the default case we skip the field
    out << indent() << "default:" << endl << indent() << "  xfer += iprot->skip(ftype);" << endl
        << indent() << "  break;" << endl;

    scope_down(out);
  } //!fields.empty()
  // Read field end marker
  indent(out) << "xfer += iprot->readFieldEnd();" << endl;

  scope_down(out);

  out << endl << indent() << "xfer += iprot->readStructEnd();" << endl;

  // Throw if any required fields are missing.
  // We do this after reading the struct end so that
  // there might possibly be a chance of continuing.
  out << endl;
  for (f_iter = fields.begin(); f_iter != fields.end(); ++f_iter) {
    if ((*f_iter)->get_req() == t_field::T_REQUIRED)
      out << indent() << "if (!isset_" << (*f_iter)->get_name() << ')' << endl << indent()
          << "  throw TProtocolException(TProtocolException::INVALID_DATA);" << endl;
  }

  indent(out) << "return xfer;" << endl;

  indent_down();
  indent(out) << "}" << endl << endl;
}