ResponsePtr Server::ServeStatic(RequestPtr request) {
  assert(request->method() == methods::kGet);

  if (doc_root_.empty()) {
    LOG_INFO("The doc root was not specified");
    return {};
  }

  fs::path path = doc_root_ / request->url().path();

  try {
    // NOTE: FileBody might throw Error::kFileError.
    auto body = std::make_shared<FileBody>(path, file_chunk_size_);

    auto response = std::make_shared<Response>(Status::kOK);

    std::string extension = path.extension().string();
    response->SetContentType(media_types::FromExtension(extension), "");

    // NOTE: Gzip compression is not supported.
    response->SetBody(body, true);

    return response;

  } catch (const Error& error) {
    LOG_ERRO("File error: %s", error.message().c_str());
    return {};
  }
}