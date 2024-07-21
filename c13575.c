static std::string GetBaseFilename(const std::string &filepath) {
  auto idx = filepath.find_last_of("/\\");
  if (idx != std::string::npos)
    return filepath.substr(idx + 1);
  return filepath;
}