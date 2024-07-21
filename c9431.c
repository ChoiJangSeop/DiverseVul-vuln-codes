clientProcessRequest(ConnStateData *conn, const Http1::RequestParserPointer &hp, Http::Stream *context)
{
    ClientHttpRequest *http = context->http;
    bool chunked = false;
    bool mustReplyToOptions = false;
    bool unsupportedTe = false;
    bool expectBody = false;

    // We already have the request parsed and checked, so we
    // only need to go through the final body/conn setup to doCallouts().
    assert(http->request);
    HttpRequest::Pointer request = http->request;

    // temporary hack to avoid splitting this huge function with sensitive code
    const bool isFtp = !hp;

    // Some blobs below are still HTTP-specific, but we would have to rewrite
    // this entire function to remove them from the FTP code path. Connection
    // setup and body_pipe preparation blobs are needed for FTP.

    request->manager(conn, http->al);

    request->flags.accelerated = http->flags.accel;
    request->flags.sslBumped=conn->switchedToHttps();
    // TODO: decouple http->flags.accel from request->flags.sslBumped
    request->flags.noDirect = (request->flags.accelerated && !request->flags.sslBumped) ?
                              !conn->port->allow_direct : 0;
    request->sources |= isFtp ? HttpMsg::srcFtp :
                        ((request->flags.sslBumped || conn->port->transport.protocol == AnyP::PROTO_HTTPS) ? HttpMsg::srcHttps : HttpMsg::srcHttp);
#if USE_AUTH
    if (request->flags.sslBumped) {
        if (conn->getAuth() != NULL)
            request->auth_user_request = conn->getAuth();
    }
#endif

    if (internalCheck(request->url.path())) {
        if (internalHostnameIs(request->url.host()) && request->url.port() == getMyPort()) {
            debugs(33, 2, "internal URL found: " << request->url.getScheme() << "://" << request->url.authority(true));
            http->flags.internal = true;
        } else if (Config.onoff.global_internal_static && internalStaticCheck(request->url.path())) {
            debugs(33, 2, "internal URL found: " << request->url.getScheme() << "://" << request->url.authority(true) << " (global_internal_static on)");
            request->url.setScheme(AnyP::PROTO_HTTP, "http");
            request->url.host(internalHostname());
            request->url.port(getMyPort());
            http->flags.internal = true;
            http->setLogUriToRequestUri();
        } else
            debugs(33, 2, "internal URL found: " << request->url.getScheme() << "://" << request->url.authority(true) << " (not this proxy)");
    }

    request->flags.internal = http->flags.internal;

    if (!isFtp) {
        // XXX: for non-HTTP messages instantiate a different HttpMsg child type
        // for now Squid only supports HTTP requests
        const AnyP::ProtocolVersion &http_ver = hp->messageProtocol();
        assert(request->http_ver.protocol == http_ver.protocol);
        request->http_ver.major = http_ver.major;
        request->http_ver.minor = http_ver.minor;
    }

    if (request->header.chunked()) {
        chunked = true;
    } else if (request->header.has(Http::HdrType::TRANSFER_ENCODING)) {
        const String te = request->header.getList(Http::HdrType::TRANSFER_ENCODING);
        // HTTP/1.1 requires chunking to be the last encoding if there is one
        unsupportedTe = te.size() && te != "identity";
    } // else implied identity coding

    mustReplyToOptions = (request->method == Http::METHOD_OPTIONS) &&
                         (request->header.getInt64(Http::HdrType::MAX_FORWARDS) == 0);
    if (!urlCheckRequest(request.getRaw()) || mustReplyToOptions || unsupportedTe) {
        clientStreamNode *node = context->getClientReplyContext();
        conn->quitAfterError(request.getRaw());
        clientReplyContext *repContext = dynamic_cast<clientReplyContext *>(node->data.getRaw());
        assert (repContext);
        repContext->setReplyToError(ERR_UNSUP_REQ, Http::scNotImplemented, request->method, NULL,
                                    conn->clientConnection->remote, request.getRaw(), NULL, NULL);
        assert(context->http->out.offset == 0);
        context->pullData();
        clientProcessRequestFinished(conn, request);
        return;
    }

    if (!chunked && !clientIsContentLengthValid(request.getRaw())) {
        clientStreamNode *node = context->getClientReplyContext();
        clientReplyContext *repContext = dynamic_cast<clientReplyContext *>(node->data.getRaw());
        assert (repContext);
        conn->quitAfterError(request.getRaw());
        repContext->setReplyToError(ERR_INVALID_REQ,
                                    Http::scLengthRequired, request->method, NULL,
                                    conn->clientConnection->remote, request.getRaw(), NULL, NULL);
        assert(context->http->out.offset == 0);
        context->pullData();
        clientProcessRequestFinished(conn, request);
        return;
    }

    clientSetKeepaliveFlag(http);
    // Let tunneling code be fully responsible for CONNECT requests
    if (http->request->method == Http::METHOD_CONNECT) {
        context->mayUseConnection(true);
        conn->flags.readMore = false;
    }

#if USE_OPENSSL
    if (conn->switchedToHttps() && conn->serveDelayedError(context)) {
        clientProcessRequestFinished(conn, request);
        return;
    }
#endif

    /* Do we expect a request-body? */
    expectBody = chunked || request->content_length > 0;
    if (!context->mayUseConnection() && expectBody) {
        request->body_pipe = conn->expectRequestBody(
                                 chunked ? -1 : request->content_length);

        /* Is it too large? */
        if (!chunked && // if chunked, we will check as we accumulate
                clientIsRequestBodyTooLargeForPolicy(request->content_length)) {
            clientStreamNode *node = context->getClientReplyContext();
            clientReplyContext *repContext = dynamic_cast<clientReplyContext *>(node->data.getRaw());
            assert (repContext);
            conn->quitAfterError(request.getRaw());
            repContext->setReplyToError(ERR_TOO_BIG,
                                        Http::scPayloadTooLarge, Http::METHOD_NONE, NULL,
                                        conn->clientConnection->remote, http->request, NULL, NULL);
            assert(context->http->out.offset == 0);
            context->pullData();
            clientProcessRequestFinished(conn, request);
            return;
        }

        if (!isFtp) {
            // We may stop producing, comm_close, and/or call setReplyToError()
            // below, so quit on errors to avoid http->doCallouts()
            if (!conn->handleRequestBodyData()) {
                clientProcessRequestFinished(conn, request);
                return;
            }

            if (!request->body_pipe->productionEnded()) {
                debugs(33, 5, "need more request body");
                context->mayUseConnection(true);
                assert(conn->flags.readMore);
            }
        }
    }

    http->calloutContext = new ClientRequestContext(http);

    http->doCallouts();

    clientProcessRequestFinished(conn, request);
}