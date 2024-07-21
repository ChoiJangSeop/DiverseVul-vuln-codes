QPDFObjectHandle::getUniqueResourceName(std::string const& prefix,
                                        int& min_suffix)
{
    std::set<std::string> names = getResourceNames();
    int max_suffix = min_suffix + names.size();
    while (min_suffix <= max_suffix)
    {
        std::string candidate = prefix + QUtil::int_to_string(min_suffix);
        if (names.count(candidate) == 0)
        {
            return candidate;
        }
        // Increment after return; min_suffix should be the value
        // used, not the next value.
        ++min_suffix;
    }
    // This could only happen if there is a coding error.
    // The number of candidates we test is more than the
    // number of keys we're checking against.
    throw std::logic_error("unable to find unconflicting name in"
                           " QPDFObjectHandle::getUniqueResourceName");
}