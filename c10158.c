void RegexMatchExpression::_init() {
    uassert(
        ErrorCodes::BadValue, "Regular expression is too long", _regex.size() <= kMaxPatternSize);

    uassert(ErrorCodes::BadValue,
            "Regular expression cannot contain an embedded null byte",
            _regex.find('\0') == std::string::npos);

    uassert(ErrorCodes::BadValue,
            "Regular expression options string cannot contain an embedded null byte",
            _flags.find('\0') == std::string::npos);
}