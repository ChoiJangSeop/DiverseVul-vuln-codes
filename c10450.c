Mgr::QueryParams::ParseParam(const String& paramStr, Param& param)
{
    bool parsed = false;
    regmatch_t pmatch[3];
    regex_t intExpr;
    regcomp(&intExpr, "^([a-z][a-z0-9_]*)=([0-9]+((,[0-9]+))*)$", REG_EXTENDED | REG_ICASE);
    regex_t stringExpr;
    regcomp(&stringExpr, "^([a-z][a-z0-9_]*)=([^&= ]+)$", REG_EXTENDED | REG_ICASE);
    if (regexec(&intExpr, paramStr.termedBuf(), 3, pmatch, 0) == 0) {
        param.first = paramStr.substr(pmatch[1].rm_so, pmatch[1].rm_eo);
        std::vector<int> array;
        int n = pmatch[2].rm_so;
        for (int i = n; i < pmatch[2].rm_eo; ++i) {
            if (paramStr[i] == ',') {
                array.push_back(atoi(paramStr.substr(n, i).termedBuf()));
                n = i + 1;
            }
        }
        if (n < pmatch[2].rm_eo)
            array.push_back(atoi(paramStr.substr(n, pmatch[2].rm_eo).termedBuf()));
        param.second = new IntParam(array);
        parsed = true;
    } else if (regexec(&stringExpr, paramStr.termedBuf(), 3, pmatch, 0) == 0) {
        param.first = paramStr.substr(pmatch[1].rm_so, pmatch[1].rm_eo);
        param.second = new StringParam(paramStr.substr(pmatch[2].rm_so, pmatch[2].rm_eo));
        parsed = true;
    }
    regfree(&stringExpr);
    regfree(&intExpr);
    return parsed;
}