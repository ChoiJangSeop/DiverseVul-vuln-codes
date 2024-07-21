Macho_Binary_t** macho_parse(const char *file) {
  FatBinary* fat = Parser::parse(file).release();

  auto** c_macho_binaries = static_cast<Macho_Binary_t**>(
      malloc((fat->size() + 1) * sizeof(Macho_Binary_t**)));

  for (size_t i = 0; i < fat->size(); ++i) {
    Binary* binary = fat->at(i);
    if (binary != nullptr) {
      c_macho_binaries[i] = static_cast<Macho_Binary_t*>(malloc(sizeof(Macho_Binary_t)));
      init_c_binary(c_macho_binaries[i], binary);
    }
  }

  c_macho_binaries[fat->size()] = nullptr;

  return c_macho_binaries;
}