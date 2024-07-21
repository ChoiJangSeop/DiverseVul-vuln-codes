Status CreateTempFile(Env* env, float value, uint64 size, string* filename) {
  const string dir = testing::TmpDir();
  *filename = io::JoinPath(dir, strings::StrCat("file_", value));
  std::unique_ptr<WritableFile> file;
  TF_RETURN_IF_ERROR(env->NewWritableFile(*filename, &file));
  for (uint64 i = 0; i < size; ++i) {
    StringPiece sp(static_cast<char*>(static_cast<void*>(&value)),
                   sizeof(value));
    TF_RETURN_IF_ERROR(file->Append(sp));
  }
  TF_RETURN_IF_ERROR(file->Close());
  return Status::OK();
}