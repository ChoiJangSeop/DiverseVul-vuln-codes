static DnsServer* dns_server_free(DnsServer *s)  {
        assert(s);

        dns_stream_unref(s->stream);

#if ENABLE_DNS_OVER_TLS
        dnstls_server_free(s);
#endif

        free(s->server_string);
        return mfree(s);
}