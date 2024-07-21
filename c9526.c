get_multipart_info(packet_info *pinfo, http_message_info_t *message_info)
{
    char *start_boundary, *start_protocol = NULL;
    multipart_info_t *m_info = NULL;
    const char *type = pinfo->match_string;
    char *parameters;
    gint dummy;

    /*
     * We need both a content type AND parameters
     * for multipart dissection.
     */
    if (type == NULL) {
        return NULL;
    }
    if (message_info == NULL) {
        return NULL;
    }
    if (message_info->media_str == NULL) {
        return NULL;
    }

    /* Clean up the parameters */
    parameters = unfold_and_compact_mime_header(message_info->media_str, &dummy);

    start_boundary = ws_find_media_type_parameter(wmem_packet_scope(), parameters, "boundary");

    if(!start_boundary) {
        return NULL;
    }
    if(strncmp(type, "multipart/encrypted", sizeof("multipart/encrypted")-1) == 0) {
        start_protocol = ws_find_media_type_parameter(wmem_packet_scope(), parameters, "protocol");
        if(!start_protocol) {
            g_free(start_boundary);
            return NULL;
        }
    }

    /*
     * There is a value for the boundary string
     */
    m_info = wmem_new(wmem_packet_scope(), multipart_info_t);
    m_info->type = type;
    m_info->boundary = start_boundary;
    m_info->boundary_length = (guint)strlen(start_boundary);
    if(start_protocol) {
        m_info->protocol = start_protocol;
        m_info->protocol_length = (guint)strlen(start_protocol);
    } else {
        m_info->protocol = NULL;
        m_info->protocol_length = -1;
    }
    m_info->orig_content_type = NULL;
    m_info->orig_parameters = NULL;

    return m_info;
}