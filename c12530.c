void FileBody::Dump(std::ostream& os, const std::string& prefix) const {
  os << prefix << "<file: " << path_.string() << ">" << std::endl;
}