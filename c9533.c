static void do_agent_file_xfer_status(UdscsConnection             *conn,
                                      struct udscs_message_header *header,
                                      guint8                      *data)
{
    gpointer task_id = GUINT_TO_POINTER(GUINT32_TO_LE(header->arg1));
    const gchar *log_msg = NULL;
    guint data_size = 0;

    /* header->arg1 = file xfer task id, header->arg2 = file xfer status */
    switch (header->arg2) {
        case VD_AGENT_FILE_XFER_STATUS_NOT_ENOUGH_SPACE:
            *((guint64 *)data) = GUINT64_TO_LE(*((guint64 *)data));
            log_msg = "Not enough free space. Cancelling file-xfer %u";
            data_size = sizeof(guint64);
            break;
        case VD_AGENT_FILE_XFER_STATUS_DISABLED:
            log_msg = "File-xfer is disabled. Cancelling file-xfer %u";
            break;
    }
    send_file_xfer_status(virtio_port, log_msg, header->arg1, header->arg2,
                          data, data_size);

    if (header->arg2 == VD_AGENT_FILE_XFER_STATUS_CAN_SEND_DATA)
        g_hash_table_insert(active_xfers, task_id, conn);
    else
        g_hash_table_remove(active_xfers, task_id);
}