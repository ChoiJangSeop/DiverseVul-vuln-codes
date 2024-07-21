absl::optional<std::string> canonicalizePath(absl::string_view original_path) {
  std::string canonical_path;
  url::Component in_component(0, original_path.size());
  url::Component out_component;
  url::StdStringCanonOutput output(&canonical_path);
  if (!CanonicalizePath(original_path.data(), in_component, &output, &out_component)) {
    return absl::nullopt;
  } else {
    output.Complete();
    return absl::make_optional(std::move(canonical_path));
  }
}