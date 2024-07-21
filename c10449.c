CacheManager::ParseUrl(const char *url)
{
    int t;
    LOCAL_ARRAY(char, host, MAX_URL);
    LOCAL_ARRAY(char, request, MAX_URL);
    LOCAL_ARRAY(char, password, MAX_URL);
    LOCAL_ARRAY(char, params, MAX_URL);
    host[0] = 0;
    request[0] = 0;
    password[0] = 0;
    params[0] = 0;
    int pos = -1;
    int len = strlen(url);
    Must(len > 0);
    t = sscanf(url, "cache_object://%[^/]/%[^@?]%n@%[^?]?%s", host, request, &pos, password, params);
    if (t < 3) {
        t = sscanf(url, "cache_object://%[^/]/%[^?]%n?%s", host, request, &pos, params);
    }
    if (t < 1) {
        t = sscanf(url, "http://%[^/]/squid-internal-mgr/%[^?]%n?%s", host, request, &pos, params);
    }
    if (t < 1) {
        t = sscanf(url, "https://%[^/]/squid-internal-mgr/%[^?]%n?%s", host, request, &pos, params);
    }
    if (t < 2) {
        if (strncmp("cache_object://",url,15)==0)
            xstrncpy(request, "menu", MAX_URL);
        else
            xstrncpy(request, "index", MAX_URL);
    }

#if _SQUID_OS2_
    if (t == 2 && request[0] == '\0') {
        /*
         * emx's sscanf insists of returning 2 because it sets request
         * to null
         */
        if (strncmp("cache_object://",url,15)==0)
            xstrncpy(request, "menu", MAX_URL);
        else
            xstrncpy(request, "index", MAX_URL);
    }
#endif

    debugs(16, 3, HERE << "MGR request: t=" << t << ", host='" << host << "', request='" << request << "', pos=" << pos <<
           ", password='" << password << "', params='" << params << "'");

    Mgr::ActionProfile::Pointer profile = findAction(request);
    if (!profile) {
        debugs(16, DBG_IMPORTANT, "CacheManager::ParseUrl: action '" << request << "' not found");
        return NULL;
    }

    const char *prot = ActionProtection(profile);
    if (!strcmp(prot, "disabled") || !strcmp(prot, "hidden")) {
        debugs(16, DBG_IMPORTANT, "CacheManager::ParseUrl: action '" << request << "' is " << prot);
        return NULL;
    }

    Mgr::Command::Pointer cmd = new Mgr::Command;
    if (!Mgr::QueryParams::Parse(params, cmd->params.queryParams))
        return NULL;
    cmd->profile = profile;
    cmd->params.httpUri = url;
    cmd->params.userName = String();
    cmd->params.password = password;
    cmd->params.actionName = request;
    return cmd;
}