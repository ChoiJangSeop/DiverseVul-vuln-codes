static void tiny_dispatch(const MessagesMap_t *entry, uint8_t *msg, uint32_t msg_size)
{
    if (!pb_parse(entry, msg, msg_size, msg_tiny)) {
        call_msg_failure_handler(FailureType_Failure_UnexpectedMessage,
                                 "Could not parse tiny protocol buffer message");
        return;
    }

    msg_tiny_id = entry->msg_id;
}