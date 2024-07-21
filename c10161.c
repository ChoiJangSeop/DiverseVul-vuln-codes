Status RegexMatchExpression::init(StringData path, StringData regex, StringData options) {
    if (regex.size() > MaxPatternSize) {
        return Status(ErrorCodes::BadValue, "Regular expression is too long");
    }

    if (regex.find('\0') != std::string::npos) {
        return Status(ErrorCodes::BadValue,
                      "Regular expression cannot contain an embedded null byte");
    }

    if (options.find('\0') != std::string::npos) {
        return Status(ErrorCodes::BadValue,
                      "Regular expression options string cannot contain an embedded null byte");
    }

    _regex = regex.toString();
    _flags = options.toString();

    // isValidUTF8() checks for UTF-8 which does not map to a series of codepoints but does not
    // check the validity of the code points themselves. These situations do not cause problems
    // downstream so we do not do additional work to enforce that the code points are valid.
    uassert(
        5108300, "Regular expression is invalid UTF-8", isValidUTF8(_regex) && isValidUTF8(_flags));

    _re.reset(new pcrecpp::RE(_regex.c_str(), flags2options(_flags.c_str())));

    return setPath(path);
}