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
    _re.reset(new pcrecpp::RE(_regex.c_str(), flags2options(_flags.c_str())));

    return setPath(path);
}