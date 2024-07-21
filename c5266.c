void t_cpp_generator::generate_struct_writer(ofstream& out, t_struct* tstruct, bool pointers) {
  string name = tstruct->get_name();
  const vector<t_field*>& fields = tstruct->get_sorted_members();
  vector<t_field*>::const_iterator f_iter;

  if (gen_templates_) {
    out << indent() << "template <class Protocol_>" << endl << indent() << "uint32_t "
        << tstruct->get_name() << "::write(Protocol_* oprot) const {" << endl;
  } else {
    indent(out) << "uint32_t " << tstruct->get_name()
                << "::write(::apache::thrift::protocol::TProtocol* oprot) const {" << endl;
  }
  indent_up();

  out << indent() << "uint32_t xfer = 0;" << endl;

  indent(out) << "oprot->incrementRecursionDepth();" << endl;
  indent(out) << "xfer += oprot->writeStructBegin(\"" << name << "\");" << endl;

  for (f_iter = fields.begin(); f_iter != fields.end(); ++f_iter) {
    bool check_if_set = (*f_iter)->get_req() == t_field::T_OPTIONAL
                        || (*f_iter)->get_type()->is_xception();
    if (check_if_set) {
      out << endl << indent() << "if (this->__isset." << (*f_iter)->get_name() << ") {" << endl;
      indent_up();
    } else {
      out << endl;
    }

    // Write field header
    out << indent() << "xfer += oprot->writeFieldBegin("
        << "\"" << (*f_iter)->get_name() << "\", " << type_to_enum((*f_iter)->get_type()) << ", "
        << (*f_iter)->get_key() << ");" << endl;
    // Write field contents
    if (pointers && !(*f_iter)->get_type()->is_xception()) {
      generate_serialize_field(out, *f_iter, "(*(this->", "))");
    } else {
      generate_serialize_field(out, *f_iter, "this->");
    }
    // Write field closer
    indent(out) << "xfer += oprot->writeFieldEnd();" << endl;
    if (check_if_set) {
      indent_down();
      indent(out) << '}';
    }
  }

  out << endl;

  // Write the struct map
  out << indent() << "xfer += oprot->writeFieldStop();" << endl << indent()
      << "xfer += oprot->writeStructEnd();" << endl << indent()
      << "oprot->decrementRecursionDepth();" << endl << indent() << "return xfer;" << endl;

  indent_down();
  indent(out) << "}" << endl << endl;
}