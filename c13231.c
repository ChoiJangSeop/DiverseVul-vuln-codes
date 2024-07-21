void FilterManager::maybeEndEncode(bool end_stream) {
  if (end_stream) {
    filter_manager_callbacks_.endStream();
  }
}