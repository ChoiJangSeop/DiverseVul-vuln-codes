bool TLSWrap::InvokeQueued(int status, const char* error_str) {
  Debug(this, "InvokeQueued(%d, %s)", status, error_str);
  if (!write_callback_scheduled_)
    return false;

  if (current_write_ != nullptr) {
    WriteWrap* w = current_write_;
    current_write_ = nullptr;
    w->Done(status, error_str);
  }

  return true;
}