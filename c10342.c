bool ConnectionManagerUtility::maybeNormalizePath(RequestHeaderMap& request_headers,
                                                  const ConnectionManagerConfig& config) {
  if (!request_headers.Path()) {
    return true; // It's as valid as it is going to get.
  }
  bool is_valid_path = true;
  if (config.shouldNormalizePath()) {
    is_valid_path = PathUtil::canonicalPath(request_headers);
  }
  // Merge slashes after path normalization to catch potential edge cases with percent encoding.
  if (is_valid_path && config.shouldMergeSlashes()) {
    PathUtil::mergeSlashes(request_headers);
  }
  return is_valid_path;
}