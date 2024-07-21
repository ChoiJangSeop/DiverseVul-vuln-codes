static void test_http_server(void) {
  struct mg_mgr mgr;
  const char *url = "http://127.0.0.1:12346";
  char buf[FETCH_BUF_SIZE];

  mg_mgr_init(&mgr);
  mg_http_listen(&mgr, url, eh1, NULL);

  ASSERT(fetch(&mgr, buf, url, "GET /a.txt HTTP/1.0\n\n") == 200);
  ASSERT(cmpbody(buf, "hello\n") == 0);

  ASSERT(fetch(&mgr, buf, url, "GET /%%61.txt HTTP/1.0\n\n") == 200);
  ASSERT(cmpbody(buf, "hello\n") == 0);

  // Responses with missing reason phrase must also work
  ASSERT(fetch(&mgr, buf, url, "GET /no_reason HTTP/1.0\n\n") == 200);
  ASSERT(cmpbody(buf, "ok") == 0);

  // Fetch file with unicode chars in filename
  ASSERT(fetch(&mgr, buf, url, "GET /київ.txt HTTP/1.0\n\n") == 200);
  ASSERT(cmpbody(buf, "є\n") == 0);

  ASSERT(fetch(&mgr, buf, url, "GET /../fuzz.c HTTP/1.0\n\n") == 404);
  ASSERT(fetch(&mgr, buf, url, "GET /.%%2e/fuzz.c HTTP/1.0\n\n") == 404);
  ASSERT(fetch(&mgr, buf, url, "GET /.%%2e%%2ffuzz.c HTTP/1.0\n\n") == 404);
  ASSERT(fetch(&mgr, buf, url, "GET /..%%2f%%20fuzz.c HTTP/1.0\n\n") == 404);
  ASSERT(fetch(&mgr, buf, url, "GET /..%%2ffuzz.c%%20 HTTP/1.0\n\n") == 404);

  ASSERT(fetch(&mgr, buf, url, "GET /dredir HTTP/1.0\n\n") == 301);
  ASSERT(cmpheader(buf, "Location", "/dredir/"));

  ASSERT(fetch(&mgr, buf, url, "GET /dredir/ HTTP/1.0\n\n") == 200);
  ASSERT(cmpbody(buf, "hi\n") == 0);

  ASSERT(fetch(&mgr, buf, url, "GET /..ddot HTTP/1.0\n\n") == 301);
  ASSERT(fetch(&mgr, buf, url, "GET /..ddot/ HTTP/1.0\n\n") == 200);
  ASSERT(cmpbody(buf, "hi\n") == 0);

  {
    extern char *mg_http_etag(char *, size_t, size_t, time_t);
    char etag[100];
    size_t size = 0;
    time_t mtime = 0;
    ASSERT(mg_fs_posix.stat("./test/data/a.txt", &size, &mtime) != 0);
    ASSERT(mg_http_etag(etag, sizeof(etag), size, mtime) == etag);
    ASSERT(fetch(&mgr, buf, url, "GET /a.txt HTTP/1.0\nIf-None-Match: %s\n\n",
                 etag) == 304);
  }

  // Text mime type override
  ASSERT(fetch(&mgr, buf, url, "GET /servefile HTTP/1.0\n\n") == 200);
  ASSERT(cmpbody(buf, "hello\n") == 0);
  {
    struct mg_http_message hm;
    mg_http_parse(buf, strlen(buf), &hm);
    ASSERT(mg_http_get_header(&hm, "Content-Type") != NULL);
    ASSERT(mg_strcmp(*mg_http_get_header(&hm, "Content-Type"), mg_str("c/d")) ==
           0);
  }

  ASSERT(fetch(&mgr, buf, url, "GET /foo/1 HTTP/1.0\r\n\n") == 200);
  // LOG(LL_INFO, ("%d %.*s", (int) hm.len, (int) hm.len, hm.buf));
  ASSERT(cmpbody(buf, "uri: 1") == 0);

  ASSERT(fetch(&mgr, buf, url, "%s",
               "POST /body HTTP/1.1\r\n"
               "Content-Length: 4\r\n\r\nkuku") == 200);
  ASSERT(cmpbody(buf, "kuku") == 0);

  ASSERT(fetch(&mgr, buf, url, "GET /ssi HTTP/1.1\r\n\r\n") == 301);
  ASSERT(fetch(&mgr, buf, url, "GET /ssi/ HTTP/1.1\r\n\r\n") == 200);
  ASSERT(cmpbody(buf,
                 "this is index\n"
                 "this is nested\n\n"
                 "this is f1\n\n\n\n"
                 "recurse\n\n"
                 "recurse\n\n"
                 "recurse\n\n"
                 "recurse\n\n"
                 "recurse\n\n") == 0);
  {
    struct mg_http_message hm;
    mg_http_parse(buf, strlen(buf), &hm);
    ASSERT(mg_http_get_header(&hm, "Content-Length") != NULL);
    ASSERT(mg_http_get_header(&hm, "Content-Type") != NULL);
    ASSERT(mg_strcmp(*mg_http_get_header(&hm, "Content-Type"),
                     mg_str("text/html; charset=utf-8")) == 0);
  }

  ASSERT(fetch(&mgr, buf, url, "GET /badroot HTTP/1.0\r\n\n") == 400);
  ASSERT(cmpbody(buf, "Invalid web root [/BAAADDD!]\n") == 0);

  {
    char *data = mg_file_read("./test/data/ca.pem", NULL);
    ASSERT(fetch(&mgr, buf, url, "GET /ca.pem HTTP/1.0\r\n\n") == 200);
    ASSERT(cmpbody(buf, data) == 0);
    free(data);
  }

  {
    // Test mime type
    struct mg_http_message hm;
    ASSERT(fetch(&mgr, buf, url, "GET /empty.js HTTP/1.0\r\n\n") == 200);
    mg_http_parse(buf, strlen(buf), &hm);
    ASSERT(mg_http_get_header(&hm, "Content-Type") != NULL);
    ASSERT(mg_strcmp(*mg_http_get_header(&hm, "Content-Type"),
                     mg_str("text/javascript; charset=utf-8")) == 0);
  }

  {
    // Test connection refused
    int i, errored = 0;
    mg_connect(&mgr, "tcp://127.0.0.1:55117", eh9, &errored);
    for (i = 0; i < 10 && errored == 0; i++) mg_mgr_poll(&mgr, 1);
    ASSERT(errored == 7);
  }

  // Directory listing
  fetch(&mgr, buf, url, "GET /test/ HTTP/1.0\n\n");
  ASSERT(fetch(&mgr, buf, url, "GET /test/ HTTP/1.0\n\n") == 200);
  ASSERT(mg_strstr(mg_str(buf), mg_str(">Index of /test/<")) != NULL);
  ASSERT(mg_strstr(mg_str(buf), mg_str(">fuzz.c<")) != NULL);

  {
    // Credentials
    struct mg_http_message hm;
    ASSERT(fetch(&mgr, buf, url, "%s",
                 "GET /creds?access_token=x HTTP/1.0\r\n\r\n") == 200);
    mg_http_parse(buf, strlen(buf), &hm);
    ASSERT(mg_strcmp(hm.body, mg_str("[]:[x]")) == 0);

    ASSERT(fetch(&mgr, buf, url, "%s",
                 "GET /creds HTTP/1.0\r\n"
                 "Authorization: Bearer x\r\n\r\n") == 200);
    mg_http_parse(buf, strlen(buf), &hm);
    ASSERT(mg_strcmp(hm.body, mg_str("[]:[x]")) == 0);

    ASSERT(fetch(&mgr, buf, url, "%s",
                 "GET /creds HTTP/1.0\r\n"
                 "Authorization: Basic Zm9vOmJhcg==\r\n\r\n") == 200);
    mg_http_parse(buf, strlen(buf), &hm);
    ASSERT(mg_strcmp(hm.body, mg_str("[foo]:[bar]")) == 0);

    ASSERT(fetch(&mgr, buf, url, "%s",
                 "GET /creds HTTP/1.0\r\n"
                 "Cookie: blah; access_token=hello\r\n\r\n") == 200);
    mg_http_parse(buf, strlen(buf), &hm);
    ASSERT(mg_strcmp(hm.body, mg_str("[]:[hello]")) == 0);
  }

  {
    // Test upload
    char *p;
    remove("uploaded.txt");
    ASSERT((p = mg_file_read("uploaded.txt", NULL)) == NULL);

    ASSERT(fetch(&mgr, buf, url,
                 "POST /upload HTTP/1.0\n"
                 "Content-Length: 1\n\nx") == 400);

    ASSERT(fetch(&mgr, buf, url,
                 "POST /upload?name=uploaded.txt HTTP/1.0\r\n"
                 "Content-Length: 5\r\n"
                 "\r\nhello") == 200);
    ASSERT(fetch(&mgr, buf, url,
                 "POST /upload?name=uploaded.txt&offset=5 HTTP/1.0\r\n"
                 "Content-Length: 6\r\n"
                 "\r\n\nworld") == 200);
    ASSERT((p = mg_file_read("uploaded.txt", NULL)) != NULL);
    ASSERT(strcmp(p, "hello\nworld") == 0);
    free(p);
    remove("uploaded.txt");
  }

  // HEAD request
  ASSERT(fetch(&mgr, buf, url, "GET /a.txt HTTP/1.0\n\n") == 200);
  ASSERT(fetch(&mgr, buf, url, "HEAD /a.txt HTTP/1.0\n\n") == 200);

#if MG_ENABLE_IPV6
  {
    const char *url6 = "http://[::1]:12346";
    ASSERT(mg_http_listen(&mgr, url6, eh1, NULL) != NULL);
    ASSERT(fetch(&mgr, buf, url6, "GET /a.txt HTTP/1.0\n\n") == 200);
    ASSERT(cmpbody(buf, "hello\n") == 0);
  }
#endif

  mg_mgr_free(&mgr);
  ASSERT(mgr.conns == NULL);
}