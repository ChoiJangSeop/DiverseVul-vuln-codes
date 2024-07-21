static int xar_get_toc_data_values(xmlTextReaderPtr reader, long *length, long *offset, long *size, int *encoding,
                                   unsigned char ** a_cksum, int * a_hash, unsigned char ** e_cksum, int * e_hash)
{
    const xmlChar *name;
    int indata = 0, inea = 0;
    int rc, gotoffset=0, gotlength=0, gotsize=0;

    *a_cksum = NULL;
    *a_hash = XAR_CKSUM_NONE;
    *e_cksum = NULL;
    *e_hash = XAR_CKSUM_NONE;
    *encoding = CL_TYPE_ANY;

    rc = xmlTextReaderRead(reader);
    while (rc == 1) {
        name = xmlTextReaderConstLocalName(reader);
        if (indata || inea) {
            /*  cli_dbgmsg("cli_scanxar: xmlTextReaderRead read %s\n", name); */
            if (xmlStrEqual(name, (const xmlChar *)"offset") && 
                xmlTextReaderNodeType(reader) == XML_READER_TYPE_ELEMENT) {
                if (CL_SUCCESS == xar_get_numeric_from_xml_element(reader, offset))
                    gotoffset=1;

            } else if (xmlStrEqual(name, (const xmlChar *)"length") &&
                       xmlTextReaderNodeType(reader) == XML_READER_TYPE_ELEMENT) {
                if (CL_SUCCESS == xar_get_numeric_from_xml_element(reader, length))
                    gotlength=1;

            } else if (xmlStrEqual(name, (const xmlChar *)"size") &&
                       xmlTextReaderNodeType(reader) == XML_READER_TYPE_ELEMENT) {
                if (CL_SUCCESS == xar_get_numeric_from_xml_element(reader, size))
                    gotsize=1;

            } else if (xmlStrEqual(name, (const xmlChar *)"archived-checksum") &&
                       xmlTextReaderNodeType(reader) == XML_READER_TYPE_ELEMENT) {
                cli_dbgmsg("cli_scanxar: <archived-checksum>:\n");
                xar_get_checksum_values(reader, a_cksum, a_hash);
                
            } else if ((xmlStrEqual(name, (const xmlChar *)"extracted-checksum") ||
                        xmlStrEqual(name, (const xmlChar *)"unarchived-checksum")) &&
                       xmlTextReaderNodeType(reader) == XML_READER_TYPE_ELEMENT) {
                cli_dbgmsg("cli_scanxar: <extracted-checksum>:\n");
                xar_get_checksum_values(reader, e_cksum, e_hash);

            } else if (xmlStrEqual(name, (const xmlChar *)"encoding") &&
                       xmlTextReaderNodeType(reader) == XML_READER_TYPE_ELEMENT) {
                xmlChar * style = xmlTextReaderGetAttribute(reader, (const xmlChar *)"style");
                if (style == NULL) {
                    cli_dbgmsg("cli_scaxar: xmlTextReaderGetAttribute no style attribute "
                               "for encoding element\n");
                    *encoding = CL_TYPE_ANY;
                } else if (xmlStrEqual(style, (const xmlChar *)"application/x-gzip")) {
                    cli_dbgmsg("cli_scanxar: encoding = application/x-gzip.\n");
                    *encoding = CL_TYPE_GZ; 
                } else if (xmlStrEqual(style, (const xmlChar *)"application/octet-stream")) {
                    cli_dbgmsg("cli_scanxar: encoding = application/octet-stream.\n");
                    *encoding = CL_TYPE_ANY; 
                } else if (xmlStrEqual(style, (const xmlChar *)"application/x-bzip2")) {
                    cli_dbgmsg("cli_scanxar: encoding = application/x-bzip2.\n");
                    *encoding = CL_TYPE_BZ;
                } else if (xmlStrEqual(style, (const xmlChar *)"application/x-lzma")) {
                    cli_dbgmsg("cli_scanxar: encoding = application/x-lzma.\n");
                    *encoding = CL_TYPE_7Z;
                 } else if (xmlStrEqual(style, (const xmlChar *)"application/x-xz")) {
                    cli_dbgmsg("cli_scanxar: encoding = application/x-xz.\n");
                    *encoding = CL_TYPE_XZ;
                } else {
                    cli_dbgmsg("cli_scaxar: unknown style value=%s for encoding element\n", style);
                    *encoding = CL_TYPE_ANY;
                }
                if (style != NULL)
                    xmlFree(style);

           } else if (indata && xmlStrEqual(name, (const xmlChar *)"data") &&
                       xmlTextReaderNodeType(reader) == XML_READER_TYPE_END_ELEMENT) {
                break;

            } else if (inea && xmlStrEqual(name, (const xmlChar *)"ea") &&
                       xmlTextReaderNodeType(reader) == XML_READER_TYPE_END_ELEMENT) {
                break;
            }
            
        } else {
            if (xmlTextReaderNodeType(reader) == XML_READER_TYPE_ELEMENT) {
                if (xmlStrEqual(name, (const xmlChar *)"data")) {
                    cli_dbgmsg("cli_scanxar: xmlTextReaderRead read <data>\n");
                    indata = 1;
                } else if (xmlStrEqual(name, (const xmlChar *)"ea")) {
                    cli_dbgmsg("cli_scanxar: xmlTextReaderRead read <ea>\n");
                    inea = 1;
                }
            } else if ((xmlTextReaderNodeType(reader) == XML_READER_TYPE_END_ELEMENT) &&
                       xmlStrEqual(name, (const xmlChar *)"xar")) {
                cli_dbgmsg("cli_scanxar: finished parsing xar TOC.\n");   
                break;
            }
        }
        rc = xmlTextReaderRead(reader);
    }
    
    if (gotoffset && gotlength && gotsize) {
        rc = CL_SUCCESS;
    }
    else if (0 == gotoffset + gotlength + gotsize)
        rc = CL_BREAK;
    else
        rc = CL_EFORMAT;

    return rc;
}