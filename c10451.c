Mgr::QueryParams::Parse(const String& aParamsStr, QueryParams& aParams)
{
    if (aParamsStr.size() != 0) {
        Param param;
        size_t n = 0;
        size_t len = aParamsStr.size();
        for (size_t i = n; i < len; ++i) {
            if (aParamsStr[i] == '&') {
                if (!ParseParam(aParamsStr.substr(n, i), param))
                    return false;
                aParams.params.push_back(param);
                n = i + 1;
            }
        }
        if (n < len) {
            if (!ParseParam(aParamsStr.substr(n, len), param))
                return false;
            aParams.params.push_back(param);
        }
    }
    return true;
}