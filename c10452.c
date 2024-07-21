CacheManager::start(const Comm::ConnectionPointer &client, HttpRequest *request, StoreEntry *entry, const AccessLogEntry::Pointer &ale)
{
    debugs(16, 3, "CacheManager::Start: '" << entry->url() << "'" );

    Mgr::Command::Pointer cmd = ParseUrl(entry->url());
    if (!cmd) {
        const auto err = new ErrorState(ERR_INVALID_URL, Http::scNotFound, request, ale);
        err->url = xstrdup(entry->url());
        errorAppendEntry(entry, err);
        entry->expires = squid_curtime;
        return;
    }

    const char *actionName = cmd->profile->name;

    entry->expires = squid_curtime;

    debugs(16, 5, "CacheManager: " << client << " requesting '" << actionName << "'");

    /* get additional info from request headers */
    ParseHeaders(request, cmd->params);

    const char *userName = cmd->params.userName.size() ?
                           cmd->params.userName.termedBuf() : "unknown";

    /* Check password */

    if (CheckPassword(*cmd) != 0) {
        /* build error message */
        ErrorState errState(ERR_CACHE_MGR_ACCESS_DENIED, Http::scUnauthorized, request, ale);
        /* warn if user specified incorrect password */

        if (cmd->params.password.size()) {
            debugs(16, DBG_IMPORTANT, "CacheManager: " <<
                   userName << "@" <<
                   client << ": incorrect password for '" <<
                   actionName << "'" );
        } else {
            debugs(16, DBG_IMPORTANT, "CacheManager: " <<
                   userName << "@" <<
                   client << ": password needed for '" <<
                   actionName << "'" );
        }

        HttpReply *rep = errState.BuildHttpReply();

#if HAVE_AUTH_MODULE_BASIC
        /*
         * add Authenticate header using action name as a realm because
         * password depends on the action
         */
        rep->header.putAuth("Basic", actionName);
#endif
        // Allow cachemgr and other XHR scripts access to our version string
        if (request->header.has(Http::HdrType::ORIGIN)) {
            rep->header.putExt("Access-Control-Allow-Origin",request->header.getStr(Http::HdrType::ORIGIN));
#if HAVE_AUTH_MODULE_BASIC
            rep->header.putExt("Access-Control-Allow-Credentials","true");
#endif
            rep->header.putExt("Access-Control-Expose-Headers","Server");
        }

        /* store the reply */
        entry->replaceHttpReply(rep);

        entry->expires = squid_curtime;

        entry->complete();

        return;
    }

    if (request->header.has(Http::HdrType::ORIGIN)) {
        cmd->params.httpOrigin = request->header.getStr(Http::HdrType::ORIGIN);
    }

    debugs(16, 2, "CacheManager: " <<
           userName << "@" <<
           client << " requesting '" <<
           actionName << "'" );

    // special case: /squid-internal-mgr/ index page
    if (!strcmp(cmd->profile->name, "index")) {
        ErrorState err(MGR_INDEX, Http::scOkay, request, ale);
        err.url = xstrdup(entry->url());
        HttpReply *rep = err.BuildHttpReply();
        if (strncmp(rep->body.content(),"Internal Error:", 15) == 0)
            rep->sline.set(Http::ProtocolVersion(1,1), Http::scNotFound);
        // Allow cachemgr and other XHR scripts access to our version string
        if (request->header.has(Http::HdrType::ORIGIN)) {
            rep->header.putExt("Access-Control-Allow-Origin",request->header.getStr(Http::HdrType::ORIGIN));
#if HAVE_AUTH_MODULE_BASIC
            rep->header.putExt("Access-Control-Allow-Credentials","true");
#endif
            rep->header.putExt("Access-Control-Expose-Headers","Server");
        }
        entry->replaceHttpReply(rep);
        entry->complete();
        return;
    }

    if (UsingSmp() && IamWorkerProcess()) {
        // is client the right connection to pass here?
        AsyncJob::Start(new Mgr::Forwarder(client, cmd->params, request, entry, ale));
        return;
    }

    Mgr::Action::Pointer action = cmd->profile->creator->create(cmd);
    Must(action != NULL);
    action->run(entry, true);
}