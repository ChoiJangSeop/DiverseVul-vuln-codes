dav_uri_match (SoupURI *a, SoupURI *b, gboolean relax)
{
  gboolean diff;

  diff = a->scheme != b->scheme || a->port != b->port  ||
    ! str_equal (a->host, b->host, TRUE)               ||
    ! path_equal (a->path, b->path, relax)             ||
    ! str_equal (a->query, b->query, FALSE)            ||
    ! str_equal (a->fragment, b->fragment, FALSE);

  return !diff;
}