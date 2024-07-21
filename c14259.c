static CURLcode hsts_create(struct hsts *h,
                            const char *hostname,
                            bool subdomains,
                            curl_off_t expires)
{
  struct stsentry *sts = hsts_entry();
  if(!sts)
    return CURLE_OUT_OF_MEMORY;

  sts->expires = expires;
  sts->includeSubDomains = subdomains;
  sts->host = strdup(hostname);
  if(!sts->host) {
    free(sts);
    return CURLE_OUT_OF_MEMORY;
  }
  Curl_llist_insert_next(&h->list, h->list.tail, sts, &sts->node);
  return CURLE_OK;
}