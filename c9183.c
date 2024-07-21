virDomainDiskSourceFormatNetworkCookies(virBufferPtr buf,
                                        virStorageSourcePtr src)
{
    g_auto(virBuffer) childBuf = VIR_BUFFER_INIT_CHILD(buf);
    size_t i;

    for (i = 0; i < src->ncookies; i++) {
        virBufferEscapeString(&childBuf, "<cookie name='%s'>", src->cookies[i]->name);
        virBufferEscapeString(&childBuf, "%s</cookie>\n", src->cookies[i]->value);
    }

    virXMLFormatElement(buf, "cookies", NULL, &childBuf);
}