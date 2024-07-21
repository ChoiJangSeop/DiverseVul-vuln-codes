tor_version_compare(tor_version_t *a, tor_version_t *b)
{
  int i;
  tor_assert(a);
  tor_assert(b);
  if ((i = a->major - b->major))
    return i;
  else if ((i = a->minor - b->minor))
    return i;
  else if ((i = a->micro - b->micro))
    return i;
  else if ((i = a->status - b->status))
    return i;
  else if ((i = a->patchlevel - b->patchlevel))
    return i;
  else if ((i = strcmp(a->status_tag, b->status_tag)))
    return i;
  else if ((i = a->svn_revision - b->svn_revision))
    return i;
  else if ((i = a->git_tag_len - b->git_tag_len))
    return i;
  else if (a->git_tag_len)
    return fast_memcmp(a->git_tag, b->git_tag, a->git_tag_len);
  else
    return 0;
}