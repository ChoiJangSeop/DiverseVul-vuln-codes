cupsdSendError(cupsd_client_t *con,	/* I - Connection */
               http_status_t  code,	/* I - Error code */
	       int            auth_type)/* I - Authentication type */
{
  char	location[HTTP_MAX_VALUE];	/* Location field */


  cupsdLogClient(con, CUPSD_LOG_DEBUG2, "cupsdSendError code=%d, auth_type=%d", code, auth_type);

#ifdef HAVE_SSL
 /*
  * Force client to upgrade for authentication if that is how the
  * server is configured...
  */

  if (code == HTTP_STATUS_UNAUTHORIZED &&
      DefaultEncryption == HTTP_ENCRYPTION_REQUIRED &&
      _cups_strcasecmp(httpGetHostname(con->http, NULL, 0), "localhost") &&
      !httpIsEncrypted(con->http))
  {
    code = HTTP_STATUS_UPGRADE_REQUIRED;
  }
#endif /* HAVE_SSL */

 /*
  * Put the request in the access_log file...
  */

  cupsdLogRequest(con, code);

 /*
  * To work around bugs in some proxies, don't use Keep-Alive for some
  * error messages...
  *
  * Kerberos authentication doesn't work without Keep-Alive, so
  * never disable it in that case.
  */

  strlcpy(location, httpGetField(con->http, HTTP_FIELD_LOCATION), sizeof(location));

  httpClearFields(con->http);

  httpSetField(con->http, HTTP_FIELD_LOCATION, location);

  if (code >= HTTP_STATUS_BAD_REQUEST && con->type != CUPSD_AUTH_NEGOTIATE)
    httpSetKeepAlive(con->http, HTTP_KEEPALIVE_OFF);

  if (httpGetVersion(con->http) >= HTTP_VERSION_1_1 &&
      httpGetKeepAlive(con->http) == HTTP_KEEPALIVE_OFF)
    httpSetField(con->http, HTTP_FIELD_CONNECTION, "close");

  if (code >= HTTP_STATUS_BAD_REQUEST)
  {
   /*
    * Send a human-readable error message.
    */

    char	message[4096],		/* Message for user */
		urltext[1024],		/* URL redirection text */
		redirect[1024];		/* Redirection link */
    const char	*text;			/* Status-specific text */


    redirect[0] = '\0';

    if (code == HTTP_STATUS_UNAUTHORIZED)
      text = _cupsLangString(con->language,
                             _("Enter your username and password or the "
			       "root username and password to access this "
			       "page. If you are using Kerberos authentication, "
			       "make sure you have a valid Kerberos ticket."));
    else if (code == HTTP_STATUS_UPGRADE_REQUIRED)
    {
      text = urltext;

      snprintf(urltext, sizeof(urltext),
               _cupsLangString(con->language,
                               _("You must access this page using the URL "
			         "<A HREF=\"https://%s:%d%s\">"
				 "https://%s:%d%s</A>.")),
               con->servername, con->serverport, con->uri,
	       con->servername, con->serverport, con->uri);

      snprintf(redirect, sizeof(redirect),
               "<META HTTP-EQUIV=\"Refresh\" "
	       "CONTENT=\"3;URL=https://%s:%d%s\">\n",
	       con->servername, con->serverport, con->uri);
    }
    else if (code == HTTP_STATUS_CUPS_WEBIF_DISABLED)
      text = _cupsLangString(con->language,
                             _("The web interface is currently disabled. Run "
			       "\"cupsctl WebInterface=yes\" to enable it."));
    else
      text = "";

    snprintf(message, sizeof(message),
             "<!DOCTYPE HTML PUBLIC \"-//W3C//DTD HTML 4.01 Transitional//EN\" "
	     "\"http://www.w3.org/TR/html4/loose.dtd\">\n"
	     "<HTML>\n"
	     "<HEAD>\n"
             "\t<META HTTP-EQUIV=\"Content-Type\" "
	     "CONTENT=\"text/html; charset=utf-8\">\n"
	     "\t<TITLE>%s - " CUPS_SVERSION "</TITLE>\n"
	     "\t<LINK REL=\"STYLESHEET\" TYPE=\"text/css\" "
	     "HREF=\"/cups.css\">\n"
	     "%s"
	     "</HEAD>\n"
             "<BODY>\n"
	     "<H1>%s</H1>\n"
	     "<P>%s</P>\n"
	     "</BODY>\n"
	     "</HTML>\n",
	     _httpStatus(con->language, code), redirect,
	     _httpStatus(con->language, code), text);

   /*
    * Send an error message back to the client.  If the error code is a
    * 400 or 500 series, make sure the message contains some text, too!
    */

    size_t length = strlen(message);	/* Length of message */

    httpSetLength(con->http, length);

    if (!cupsdSendHeader(con, code, "text/html", auth_type))
      return (0);

    if (httpWrite2(con->http, message, length) < 0)
      return (0);

    if (httpFlushWrite(con->http) < 0)
      return (0);
  }
  else
  {
    httpSetField(con->http, HTTP_FIELD_CONTENT_LENGTH, "0");

    if (!cupsdSendHeader(con, code, NULL, auth_type))
      return (0);
  }

  return (1);
}