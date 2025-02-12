int Exiv2::http(Exiv2::Dictionary& request, Exiv2::Dictionary& response, std::string& errors)
{
    if (!request.count("verb"))
        request["verb"] = "GET";
    if (!request.count("header"))
        request["header"] = "";
    if (!request.count("version"))
        request["version"] = "1.0";
    if (!request.count("port"))
        request["port"] = "";

    std::string file;
    errors = "";
    int result = 0;

    ////////////////////////////////////
    // Windows specific code
#ifdef WIN32
    WSADATA wsaData;
    WSAStartup(MAKEWORD(2, 2), &wsaData);
#endif

    const char* servername = request["server"].c_str();
    const char* page = request["page"].c_str();
    const char* verb = request["verb"].c_str();
    const char* header = request["header"].c_str();
    const char* version = request["version"].c_str();
    const char* port = request["port"].c_str();

    const char* servername_p = servername;
    const char* port_p = port;
    std::string url = std::string("http://") + request["server"] + request["page"];

    // parse and change server if using a proxy
    const char* PROXI = "HTTP_PROXY";
    const char* proxi = "http_proxy";
    const char* PROXY = getenv(PROXI);
    const char* proxy = getenv(proxi);
    bool bProx = PROXY || proxy;
    const char* prox = bProx ? (proxy ? proxy : PROXY) : "";
    Exiv2::Uri Proxy = Exiv2::Uri::Parse(prox);

    // find the dictionary of no_proxy servers
    const char* NO_PROXI = "NO_PROXY";
    const char* no_proxi = "no_proxy";
    const char* NO_PROXY = getenv(NO_PROXI);
    const char* no_proxy = getenv(no_proxi);
    bool bNoProxy = NO_PROXY || no_proxy;
    std::string no_prox = std::string(bNoProxy ? (no_proxy ? no_proxy : NO_PROXY) : "");
    Exiv2::Dictionary noProxy = stringToDict(no_prox + ",localhost,127.0.0.1");

    // if the server is on the no_proxy list ... ignore the proxy!
    if (noProxy.count(servername))
        bProx = false;

    if (bProx) {
        servername_p = Proxy.Host.c_str();
        port_p = Proxy.Port.c_str();
        page = url.c_str();
        std::string p(proxy ? proxi : PROXI);
        //  std::cerr << p << '=' << prox << " page = " << page << std::endl;
    }
    if (!port[0])
        port = "80";
    if (!port_p[0])
        port_p = "80";

    ////////////////////////////////////
    // open the socket
    int sockfd = (int)socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (sockfd < 0)
        return error(errors, "unable to create socket\n", nullptr, nullptr, 0);

    // fill in the address
    struct sockaddr_in serv_addr;
    int serv_len = sizeof(serv_addr);
    memset((char*)&serv_addr, 0, serv_len);

    serv_addr.sin_addr.s_addr = inet_addr(servername_p);
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(atoi(port_p));

    // convert unknown servername into IP address
    // http://publib.boulder.ibm.com/infocenter/iseries/v5r3/index.jsp?topic=/rzab6/rzab6uafinet.htm
    if (serv_addr.sin_addr.s_addr == (unsigned long)INADDR_NONE) {
        struct hostent* host = gethostbyname(servername_p);
        if (!host) {
            closesocket(sockfd);
            return error(errors, "no such host", servername_p);
        }
        memcpy(&serv_addr.sin_addr, host->h_addr, sizeof(serv_addr.sin_addr));
    }

    makeNonBlocking(sockfd);

    ////////////////////////////////////
    // connect the socket to the server
    auto server = connect(sockfd, (const struct sockaddr*)&serv_addr, serv_len);
    if (server == SOCKET_ERROR && WSAGetLastError() != WSAEWOULDBLOCK) {
        auto errorCode = WSAGetLastError();
        closesocket(sockfd);
        return error(errors, "error - unable to connect to server = %s port = %s wsa_error = %d", servername_p, port_p,
                     errorCode);
    }

    char buffer[32 * 1024 + 1];
    size_t buff_l = sizeof buffer - 1;

    ////////////////////////////////////
    // format the request
    int n = snprintf(buffer, buff_l, httpTemplate, verb, page, version, servername, header);
    buffer[n] = 0;
    response["requestheaders"] = std::string(buffer, n);

    ////////////////////////////////////
    // send the header (we'll have to wait for the connection by the non-blocking socket)
    while (sleep_ >= 0 && send(sockfd, buffer, n, 0) == SOCKET_ERROR /* && WSAGetLastError() == WSAENOTCONN */) {
        Sleep(snooze);
        sleep_ -= snooze;
    }

    if (sleep_ < 0) {
        auto errorCode = WSAGetLastError();
        closesocket(server);
        closesocket(sockfd);
        return error(errors, "error - timeout connecting to server = %s port = %s wsa_error = %d", servername, port,
                     errorCode);
    }

    int end = 0;             // write position in buffer
    bool bSearching = true;  // looking for headers in the response
    int status = 200;        // assume happiness

    ////////////////////////////////////
    // read and process the response
    int err;
    n = forgive(recv(sockfd, buffer, (int)buff_l, 0), err);
    while (n >= 0 && OK(status)) {
        if (n) {
            end += n;
            buffer[end] = 0;

            size_t body = 0;  // start of body
            if (bSearching) {
                // search for the body
                for (size_t b = 0; bSearching && b < lengthof(blankLines); b++) {
                    if (strstr(buffer, blankLines[b])) {
                        bSearching = false;
                        body = (int)(strstr(buffer, blankLines[b]) - buffer) + strlen(blankLines[b]);
                        status = atoi(strchr(buffer, ' '));
                    }
                }

                // parse response headers
                char* h = buffer;
                char C = ':';
                char N = '\n';
                int i = 0;  // initial byte in buffer
                while (buffer[i] == N)
                    i++;
                h = strchr(h + i, N) + 1;
                response[""] = std::string(buffer + i).substr(0, h - buffer - 2);
                result = atoi(strchr(buffer, ' '));
                char* c = strchr(h, C);
                char* first_newline = strchr(h, N);
                while (c && first_newline && c < first_newline && h < buffer + body) {
                    std::string key(h);
                    std::string value(c + 1);
                    key = key.substr(0, c - h);
                    value = value.substr(0, first_newline - c - 1);
                    response[key] = value;
                    h = first_newline + 1;
                    c = strchr(h, C);
                    first_newline = strchr(h, N);
                }
            }

            // if the buffer's full and we're still searching - give up!
            // this handles the possibility that there are no headers
            if (bSearching && buff_l - end < 10) {
                bSearching = false;
                body = 0;
            }
            if (!bSearching && OK(status)) {
                flushBuffer(buffer, body, end, file);
            }
        }
        n = forgive(recv(sockfd, buffer + end, (int)(buff_l - end), 0), err);
        if (!n) {
            Sleep(snooze);
            sleep_ -= snooze;
            if (sleep_ < 0)
                n = FINISH;
        }
    }

    if (n != FINISH || !OK(status)) {
        snprintf(buffer, sizeof buffer, "wsa_error = %d,n = %d,sleep_ = %d status = %d", WSAGetLastError(), n, sleep_,
                 status);
        error(errors, buffer, nullptr, nullptr, 0);
    } else if (bSearching && OK(status)) {
        if (end) {
            //  we finished OK without finding headers, flush the buffer
            flushBuffer(buffer, 0, end, file);
        } else {
            auto errorCode = WSAGetLastError();
            closesocket(server);
            closesocket(sockfd);
            return error(errors, "error - no response from server = %s port = %s wsa_error = %d", servername, port,
                         errorCode);
        }
    }

    ////////////////////////////////////
    // close sockets
    closesocket(server);
    closesocket(sockfd);
    response["body"] = file;
    return result;
}