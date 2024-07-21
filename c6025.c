static void xar_get_checksum_values(xmlTextReaderPtr reader, unsigned char ** cksum, int * hash)
{
    xmlChar * style = xmlTextReaderGetAttribute(reader, (const xmlChar *)"style");
    const xmlChar * xmlval;

    *hash = XAR_CKSUM_NONE;
    if (style == NULL) {
        cli_dbgmsg("cli_scaxar: xmlTextReaderGetAttribute no style attribute "
                   "for checksum element\n");
    } else {
        cli_dbgmsg("cli_scanxar: checksum algorithm is %s.\n", style);        
        if (0 == xmlStrcasecmp(style, (const xmlChar *)"sha1")) {
            *hash = XAR_CKSUM_SHA1;
        } else if (0 == xmlStrcasecmp(style, (const xmlChar *)"md5")) {
            *hash = XAR_CKSUM_MD5;
        } else {
            cli_dbgmsg("cli_scanxar: checksum algorithm %s is unsupported.\n", style);
            *hash = XAR_CKSUM_OTHER;
        }
    }
    if (style != NULL)
        xmlFree(style);

    if (xmlTextReaderRead(reader) == 1 && xmlTextReaderNodeType(reader) == XML_READER_TYPE_TEXT) {
        xmlval = xmlTextReaderConstValue(reader);
        if (xmlval) {
            *cksum = xmlStrdup(xmlval); 
            cli_dbgmsg("cli_scanxar: checksum value is %s.\n", *cksum);
        } else {
            *cksum = NULL;
            cli_dbgmsg("cli_scanxar: xmlTextReaderConstValue() returns NULL for checksum value.\n");           
        }
    }
    else
        cli_dbgmsg("cli_scanxar: No text for XML checksum element.\n");
}