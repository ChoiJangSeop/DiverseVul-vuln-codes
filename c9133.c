void LogRecordingSink::log(absl::string_view msg) {
  previous_delegate()->log(msg);
  messages_.push_back(std::string(msg));
}