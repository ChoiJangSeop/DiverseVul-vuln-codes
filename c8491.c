std::string GetTempFileName() {
#if !defined _MSC_VER && !defined __MINGW32__
  std::string temp_file_name_template_str =
      std::string(std::getenv("TEST_TMPDIR") ? std::getenv("TEST_TMPDIR") :
                                               ".") +
      "/libwebm_temp.XXXXXX";
  char* temp_file_name_template =
      new char[temp_file_name_template_str.length() + 1];
  memset(temp_file_name_template, 0, temp_file_name_template_str.length() + 1);
  temp_file_name_template_str.copy(temp_file_name_template,
                                   temp_file_name_template_str.length(), 0);
  int fd = mkstemp(temp_file_name_template);
  std::string temp_file_name =
      (fd != -1) ? std::string(temp_file_name_template) : std::string();
  delete[] temp_file_name_template;
  if (fd != -1) {
    close(fd);
  }
  return temp_file_name;
#else
  char tmp_file_name[_MAX_PATH];
#if defined _MSC_VER || defined MINGW_HAS_SECURE_API
  errno_t err = tmpnam_s(tmp_file_name);
#else
  char* fname_pointer = tmpnam(tmp_file_name);
  errno_t err = (fname_pointer == &tmp_file_name[0]) ? 0 : -1;
#endif
  if (err == 0) {
    return std::string(tmp_file_name);
  }
  return std::string();
#endif
}