bool ParseAttrValue(StringPiece type, StringPiece text, AttrValue* out) {
  // Parse type.
  string field_name;
  bool is_list = absl::ConsumePrefix(&type, "list(");
  if (absl::ConsumePrefix(&type, "string")) {
    field_name = "s";
  } else if (absl::ConsumePrefix(&type, "int")) {
    field_name = "i";
  } else if (absl::ConsumePrefix(&type, "float")) {
    field_name = "f";
  } else if (absl::ConsumePrefix(&type, "bool")) {
    field_name = "b";
  } else if (absl::ConsumePrefix(&type, "type")) {
    field_name = "type";
  } else if (absl::ConsumePrefix(&type, "shape")) {
    field_name = "shape";
  } else if (absl::ConsumePrefix(&type, "tensor")) {
    field_name = "tensor";
  } else if (absl::ConsumePrefix(&type, "func")) {
    field_name = "func";
  } else if (absl::ConsumePrefix(&type, "placeholder")) {
    field_name = "placeholder";
  } else {
    return false;
  }
  if (is_list && !absl::ConsumePrefix(&type, ")")) {
    return false;
  }

  // Construct a valid text proto message to parse.
  string to_parse;
  if (is_list) {
    // TextFormat parser considers "i: 7" to be the same as "i: [7]",
    // but we only want to allow list values with [].
    StringPiece cleaned = text;
    str_util::RemoveLeadingWhitespace(&cleaned);
    str_util::RemoveTrailingWhitespace(&cleaned);
    if (cleaned.size() < 2 || cleaned[0] != '[' ||
        cleaned[cleaned.size() - 1] != ']') {
      return false;
    }
    cleaned.remove_prefix(1);
    str_util::RemoveLeadingWhitespace(&cleaned);
    if (cleaned.size() == 1) {
      // User wrote "[]", so return empty list without invoking the TextFormat
      // parse which returns an error for "i: []".
      out->Clear();
      out->mutable_list();
      return true;
    }
    to_parse = strings::StrCat("list { ", field_name, ": ", text, " }");
  } else {
    to_parse = strings::StrCat(field_name, ": ", text);
  }

  return ProtoParseFromString(to_parse, out);
}