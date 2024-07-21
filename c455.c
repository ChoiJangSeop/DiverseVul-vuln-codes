retrieve_url (struct url * orig_parsed, const char *origurl, char **file,
              char **newloc, const char *refurl, int *dt, bool recursive,
              struct iri *iri, bool register_status)
{
  uerr_t result;
  char *url;
  bool location_changed;
  bool iri_fallbacked = 0;
  int dummy;
  char *mynewloc, *proxy;
  struct url *u = orig_parsed, *proxy_url;
  int up_error_code;            /* url parse error code */
  char *local_file;
  int redirection_count = 0;

  bool post_data_suspended = false;
  char *saved_post_data = NULL;
  char *saved_post_file_name = NULL;

  /* If dt is NULL, use local storage.  */
  if (!dt)
    {
      dt = &dummy;
      dummy = 0;
    }
  url = xstrdup (origurl);
  if (newloc)
    *newloc = NULL;
  if (file)
    *file = NULL;

  if (!refurl)
    refurl = opt.referer;

 redirected:
  /* (also for IRI fallbacking) */

  result = NOCONERROR;
  mynewloc = NULL;
  local_file = NULL;
  proxy_url = NULL;

  proxy = getproxy (u);
  if (proxy)
    {
      struct iri *pi = iri_new ();
      set_uri_encoding (pi, opt.locale, true);
      pi->utf8_encode = false;

      /* Parse the proxy URL.  */
      proxy_url = url_parse (proxy, &up_error_code, NULL, true);
      if (!proxy_url)
        {
          char *error = url_error (proxy, up_error_code);
          logprintf (LOG_NOTQUIET, _("Error parsing proxy URL %s: %s.\n"),
                     proxy, error);
          xfree (url);
          xfree (error);
          RESTORE_POST_DATA;
          result = PROXERR;
          goto bail;
        }
      if (proxy_url->scheme != SCHEME_HTTP && proxy_url->scheme != u->scheme)
        {
          logprintf (LOG_NOTQUIET, _("Error in proxy URL %s: Must be HTTP.\n"), proxy);
          url_free (proxy_url);
          xfree (url);
          RESTORE_POST_DATA;
          result = PROXERR;
          goto bail;
        }
    }

  if (u->scheme == SCHEME_HTTP
#ifdef HAVE_SSL
      || u->scheme == SCHEME_HTTPS
#endif
      || (proxy_url && proxy_url->scheme == SCHEME_HTTP))
    {
      result = http_loop (u, &mynewloc, &local_file, refurl, dt, proxy_url, iri);
    }
  else if (u->scheme == SCHEME_FTP)
    {
      /* If this is a redirection, temporarily turn off opt.ftp_glob
         and opt.recursive, both being undesirable when following
         redirects.  */
      bool oldrec = recursive, glob = opt.ftp_glob;
      if (redirection_count)
        oldrec = glob = false;

      result = ftp_loop (u, &local_file, dt, proxy_url, recursive, glob);
      recursive = oldrec;

      /* There is a possibility of having HTTP being redirected to
         FTP.  In these cases we must decide whether the text is HTML
         according to the suffix.  The HTML suffixes are `.html',
         `.htm' and a few others, case-insensitive.  */
      if (redirection_count && local_file && u->scheme == SCHEME_FTP)
        {
          if (has_html_suffix_p (local_file))
            *dt |= TEXTHTML;
        }
    }

  if (proxy_url)
    {
      url_free (proxy_url);
      proxy_url = NULL;
    }

  location_changed = (result == NEWLOCATION);
  if (location_changed)
    {
      char *construced_newloc;
      struct url *newloc_parsed;

      assert (mynewloc != NULL);

      if (local_file)
        xfree (local_file);

      /* The HTTP specs only allow absolute URLs to appear in
         redirects, but a ton of boneheaded webservers and CGIs out
         there break the rules and use relative URLs, and popular
         browsers are lenient about this, so wget should be too. */
      construced_newloc = uri_merge (url, mynewloc);
      xfree (mynewloc);
      mynewloc = construced_newloc;

      /* Reset UTF-8 encoding state, keep the URI encoding and reset
         the content encoding. */
      iri->utf8_encode = opt.enable_iri;
      set_content_encoding (iri, NULL);
      xfree_null (iri->orig_url);

      /* Now, see if this new location makes sense. */
      newloc_parsed = url_parse (mynewloc, &up_error_code, iri, true);
      if (!newloc_parsed)
        {
          char *error = url_error (mynewloc, up_error_code);
          logprintf (LOG_NOTQUIET, "%s: %s.\n", escnonprint_uri (mynewloc),
                     error);
          if (orig_parsed != u)
            {
              url_free (u);
            }
          xfree (url);
          xfree (mynewloc);
          xfree (error);
          RESTORE_POST_DATA;
          goto bail;
        }

      /* Now mynewloc will become newloc_parsed->url, because if the
         Location contained relative paths like .././something, we
         don't want that propagating as url.  */
      xfree (mynewloc);
      mynewloc = xstrdup (newloc_parsed->url);

      /* Check for max. number of redirections.  */
      if (++redirection_count > opt.max_redirect)
        {
          logprintf (LOG_NOTQUIET, _("%d redirections exceeded.\n"),
                     opt.max_redirect);
          url_free (newloc_parsed);
          if (orig_parsed != u)
            {
              url_free (u);
            }
          xfree (url);
          xfree (mynewloc);
          RESTORE_POST_DATA;
          result = WRONGCODE;
          goto bail;
        }

      xfree (url);
      url = mynewloc;
      if (orig_parsed != u)
        {
          url_free (u);
        }
      u = newloc_parsed;

      /* If we're being redirected from POST, we don't want to POST
         again.  Many requests answer POST with a redirection to an
         index page; that redirection is clearly a GET.  We "suspend"
         POST data for the duration of the redirections, and restore
         it when we're done. */
      if (!post_data_suspended)
        SUSPEND_POST_DATA;

      goto redirected;
    }

  /* Try to not encode in UTF-8 if fetching failed */
  if (!(*dt & RETROKF) && iri->utf8_encode)
    {
      iri->utf8_encode = false;
      if (orig_parsed != u)
        {
          url_free (u);
        }
      u = url_parse (origurl, NULL, iri, true);
      if (u)
        {
          DEBUGP (("[IRI fallbacking to non-utf8 for %s\n", quote (url)));
          url = xstrdup (u->url);
          iri_fallbacked = 1;
          goto redirected;
        }
      else
          DEBUGP (("[Couldn't fallback to non-utf8 for %s\n", quote (url)));
    }

  if (local_file && *dt & RETROKF)
    {
      register_download (u->url, local_file);
      if (redirection_count && 0 != strcmp (origurl, u->url))
        register_redirection (origurl, u->url);
      if (*dt & TEXTHTML)
        register_html (u->url, local_file);
      if (*dt & RETROKF)
        {
          register_download (u->url, local_file);
          if (redirection_count && 0 != strcmp (origurl, u->url))
            register_redirection (origurl, u->url);
          if (*dt & TEXTHTML)
            register_html (u->url, local_file);
          if (*dt & TEXTCSS)
            register_css (u->url, local_file);
        }
    }

  if (file)
    *file = local_file ? local_file : NULL;
  else
    xfree_null (local_file);

  if (orig_parsed != u)
    {
      url_free (u);
    }

  if (redirection_count || iri_fallbacked)
    {
      if (newloc)
        *newloc = url;
      else
        xfree (url);
    }
  else
    {
      if (newloc)
        *newloc = NULL;
      xfree (url);
    }

  RESTORE_POST_DATA;

bail:
  if (register_status)
    inform_exit_status (result);
  return result;
}