static int am_handle_repost(request_rec *r)
{
    am_mod_cfg_rec *mod_cfg;
    const char *query;
    const char *enctype;
    char *charset;
    char *psf_id;
    char *cp;
    char *psf_filename;
    char *post_data;
    const char *post_form;
    char *output;
    char *return_url;
    const char *(*post_mkform)(request_rec *, const char *);

    if (am_cookie_get(r) == NULL) {
        ap_log_rerror(APLOG_MARK, APLOG_ERR, 0, r, 
                      "Repost query without a session");
        return HTTP_FORBIDDEN;
    }

    mod_cfg = am_get_mod_cfg(r->server);

    if (!mod_cfg->post_dir) {
        ap_log_rerror(APLOG_MARK, APLOG_ERR, 0, r,
                      "Repost query without MellonPostDirectory.");
        return HTTP_NOT_FOUND;
    }

    query = r->parsed_uri.query;

    enctype = am_extract_query_parameter(r->pool, query, "enctype");
    if (enctype == NULL) {
        ap_log_rerror(APLOG_MARK, APLOG_ERR, 0, r, 
                      "Bad repost query: missing enctype");
        return HTTP_BAD_REQUEST;
    }
    if (strcmp(enctype, "urlencoded") == 0) {
        enctype = "application/x-www-form-urlencoded";
        post_mkform = am_post_mkform_urlencoded;
    } else if (strcmp(enctype, "multipart") == 0) {
        enctype = "multipart/form-data";
        post_mkform = am_post_mkform_multipart;
    } else {
        ap_log_rerror(APLOG_MARK, APLOG_ERR, 0, r, 
                      "Bad repost query: invalid enctype \"%s\".", enctype);
        return HTTP_BAD_REQUEST;
    }

    charset = am_extract_query_parameter(r->pool, query, "charset");
    if (charset != NULL) {
        if (am_urldecode(charset) != OK) {
            ap_log_rerror(APLOG_MARK, APLOG_ERR, 0, r, 
                          "Bad repost query: invalid charset \"%s\"", charset);
            return HTTP_BAD_REQUEST;
        }
    
        /* Check that charset is sane */
        for (cp = charset; *cp; cp++) {
            if (!apr_isalnum(*cp) && (*cp != '-') && (*cp != '_')) {
                ap_log_rerror(APLOG_MARK, APLOG_ERR, 0, r, 
                              "Bad repost query: invalid charset \"%s\"", charset);
                return HTTP_BAD_REQUEST;
            }
        }
    }

    psf_id = am_extract_query_parameter(r->pool, query, "id");
    if (psf_id == NULL) {
        ap_log_rerror(APLOG_MARK, APLOG_ERR, 0, r, 
                      "Bad repost query: missing id");
        return HTTP_BAD_REQUEST;
    }

    /* Check that Id is sane */
    for (cp = psf_id; *cp; cp++) {
        if (!apr_isalnum(*cp)) {
            ap_log_rerror(APLOG_MARK, APLOG_ERR, 0, r, 
                          "Bad repost query: invalid id \"%s\"", psf_id);
            return HTTP_BAD_REQUEST;
        }
    }
    
    
    return_url = am_extract_query_parameter(r->pool, query, "ReturnTo");
    if (return_url == NULL) {
        ap_log_rerror(APLOG_MARK, APLOG_ERR, 0, r,
                      "Invalid or missing query ReturnTo parameter.");
        return HTTP_BAD_REQUEST;
    }

    if (am_urldecode(return_url) != OK) {
        ap_log_rerror(APLOG_MARK, APLOG_ERR, 0, r, "Bad repost query: return");
        return HTTP_BAD_REQUEST;
    }

    psf_filename = apr_psprintf(r->pool, "%s/%s", mod_cfg->post_dir, psf_id);
    post_data = am_getfile(r->pool, r->server, psf_filename);
    if (post_data == NULL) {
        /* Unable to load repost data. Just redirect us instead. */
        ap_log_rerror(APLOG_MARK, APLOG_WARNING, 0, r,
                      "Bad repost query: cannot find \"%s\"", psf_filename);
        apr_table_setn(r->headers_out, "Location", return_url);
        return HTTP_SEE_OTHER;
    }

    if ((post_form = (*post_mkform)(r, post_data)) == NULL) {
        ap_log_rerror(APLOG_MARK, APLOG_ERR, 0, r, "am_post_mkform() failed");
        return HTTP_INTERNAL_SERVER_ERROR;
    }

    if (charset != NULL) {
         ap_set_content_type(r, apr_psprintf(r->pool,
                             "text/html; charset=\"%s\"", charset));
         charset = apr_psprintf(r->pool, " accept-charset=\"%s\"", charset);
    } else {
         ap_set_content_type(r, "text/html");
         charset = (char *)"";
    }

    output = apr_psprintf(r->pool,
      "<!DOCTYPE HTML PUBLIC \"-//W3C//DTD HTML 4.01 Transitional//EN\">\n"
      "<html>\n"
      " <head>\n" 
      "  <title>SAML rePOST request</title>\n" 
      " </head>\n" 
      " <body onload=\"document.getElementById('form').submit();\">\n" 
      "  <noscript>\n"
      "   Your browser does not support Javascript, \n"
      "   you must click the button below to proceed.\n"
      "  </noscript>\n"
      "   <form id=\"form\" method=\"POST\" action=\"%s\" enctype=\"%s\"%s>\n%s"
      "    <noscript>\n"
      "     <input type=\"submit\">\n"
      "    </noscript>\n"
      "   </form>\n"
      " </body>\n" 
      "</html>\n",
      am_htmlencode(r, return_url), enctype, charset, post_form);

    ap_rputs(output, r);
    return OK;
}