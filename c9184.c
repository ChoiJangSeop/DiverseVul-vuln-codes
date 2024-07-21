virDomainDiskSourceFormatNetwork(virBufferPtr attrBuf,
                                 virBufferPtr childBuf,
                                 virStorageSourcePtr src,
                                 unsigned int flags)
{
    size_t n;
    g_autofree char *path = NULL;

    virBufferAsprintf(attrBuf, " protocol='%s'",
                      virStorageNetProtocolTypeToString(src->protocol));

    if (src->volume)
        path = g_strdup_printf("%s/%s", src->volume, src->path);

    virBufferEscapeString(attrBuf, " name='%s'", path ? path : src->path);
    virBufferEscapeString(attrBuf, " query='%s'", src->query);

    if (src->haveTLS != VIR_TRISTATE_BOOL_ABSENT &&
        !(flags & VIR_DOMAIN_DEF_FORMAT_MIGRATABLE &&
          src->tlsFromConfig))
        virBufferAsprintf(attrBuf, " tls='%s'",
                          virTristateBoolTypeToString(src->haveTLS));
    if (flags & VIR_DOMAIN_DEF_FORMAT_STATUS)
        virBufferAsprintf(attrBuf, " tlsFromConfig='%d'", src->tlsFromConfig);

    for (n = 0; n < src->nhosts; n++) {
        virBufferAddLit(childBuf, "<host");
        virBufferEscapeString(childBuf, " name='%s'", src->hosts[n].name);

        if (src->hosts[n].port)
            virBufferAsprintf(childBuf, " port='%u'", src->hosts[n].port);

        if (src->hosts[n].transport)
            virBufferAsprintf(childBuf, " transport='%s'",
                              virStorageNetHostTransportTypeToString(src->hosts[n].transport));

        virBufferEscapeString(childBuf, " socket='%s'", src->hosts[n].socket);
        virBufferAddLit(childBuf, "/>\n");
    }

    virBufferEscapeString(childBuf, "<snapshot name='%s'/>\n", src->snapshot);
    virBufferEscapeString(childBuf, "<config file='%s'/>\n", src->configFile);

    virStorageSourceInitiatorFormatXML(&src->initiator, childBuf);

    if (src->sslverify != VIR_TRISTATE_BOOL_ABSENT) {
        virBufferAsprintf(childBuf, "<ssl verify='%s'/>\n",
                          virTristateBoolTypeToString(src->sslverify));
    }

    virDomainDiskSourceFormatNetworkCookies(childBuf, src);

    if (src->readahead)
        virBufferAsprintf(childBuf, "<readahead size='%llu'/>\n", src->readahead);

    if (src->timeout)
        virBufferAsprintf(childBuf, "<timeout seconds='%llu'/>\n", src->timeout);

    return 0;
}