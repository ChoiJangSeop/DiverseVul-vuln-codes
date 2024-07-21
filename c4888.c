compare_vote_rs(const vote_routerstatus_t *a, const vote_routerstatus_t *b)
{
  int r;
  tor_assert(a);
  tor_assert(b);

  if ((r = fast_memcmp(a->status.identity_digest, b->status.identity_digest,
                  DIGEST_LEN)))
    return r;
  if ((r = fast_memcmp(a->status.descriptor_digest,
                       b->status.descriptor_digest,
                       DIGEST_LEN)))
    return r;
  if ((r = (int)(b->status.published_on - a->status.published_on)))
    return r;
  if ((r = strcmp(b->status.nickname, a->status.nickname)))
    return r;
  if ((r = (((int)b->status.addr) - ((int)a->status.addr))))
    return r;
  if ((r = (((int)b->status.or_port) - ((int)a->status.or_port))))
    return r;
  if ((r = (((int)b->status.dir_port) - ((int)a->status.dir_port))))
    return r;
  return 0;
}