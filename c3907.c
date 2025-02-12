dissect_u3v(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree, void* data)
{
    gint offset = 0;
    proto_tree *u3v_tree = NULL, *ccd_tree_flag, *u3v_telegram_tree = NULL, *ccd_tree = NULL;
    gint data_length = 0;
    gint req_id = 0;
    gint command_id = -1;
    gint status = 0;
    guint prefix = 0;
    proto_item *ti = NULL;
    proto_item *item = NULL;
    const char *command_string;
    usb_conv_info_t *usb_conv_info;
    gint stream_detected = FALSE;
    gint control_detected = FALSE;
    u3v_conv_info_t *u3v_conv_info = NULL;
    gencp_transaction_t *gencp_trans = NULL;

    usb_conv_info = (usb_conv_info_t *)data;

    /* decide if this packet belongs to U3V protocol */
    u3v_conv_info = (u3v_conv_info_t *)usb_conv_info->class_data;

    if (!u3v_conv_info) {
        u3v_conv_info = wmem_new0(wmem_file_scope(), u3v_conv_info_t);
        usb_conv_info->class_data = u3v_conv_info;
    }

    prefix = tvb_get_letohl(tvb, 0);
    if ((tvb_reported_length(tvb) >= 4) && ( ( U3V_CONTROL_PREFIX == prefix ) || ( U3V_EVENT_PREFIX == prefix ) ) ) {
        control_detected = TRUE;
    }

    if (((tvb_reported_length(tvb) >= 4) && (( U3V_STREAM_LEADER_PREFIX == prefix ) || ( U3V_STREAM_TRAILER_PREFIX == prefix )))
         || (usb_conv_info->endpoint == u3v_conv_info->ep_stream)) {
        stream_detected = TRUE;
    }

    /* initialize interface class/subclass in case no descriptors have been dissected yet */
    if ( control_detected || stream_detected){
        if ( usb_conv_info->interfaceClass  == IF_CLASS_UNKNOWN &&
             usb_conv_info->interfaceSubclass  == IF_SUBCLASS_UNKNOWN){
            usb_conv_info->interfaceClass = IF_CLASS_MISCELLANEOUS;
            usb_conv_info->interfaceSubclass = IF_SUBCLASS_MISC_U3V;
        }
    }

    if ( control_detected ) {
        /* Set the protocol column */
        col_set_str(pinfo->cinfo, COL_PROTOCOL, "U3V");

        /* Clear out stuff in the info column */
        col_clear(pinfo->cinfo, COL_INFO);

        /* Adds "USB3Vision" heading to protocol tree */
        /* We will add fields to this using the u3v_tree pointer */
        ti = proto_tree_add_item(tree, proto_u3v, tvb, offset, -1, ENC_NA);
        u3v_tree = proto_item_add_subtree(ti, ett_u3v);

        prefix = tvb_get_letohl(tvb, offset);
        command_id = tvb_get_letohs(tvb, offset+6);

        /* decode CCD ( DCI/DCE command data layout) */
        if ((prefix == U3V_CONTROL_PREFIX || prefix == U3V_EVENT_PREFIX) && ((command_id % 2) == 0)) {
            command_string = val_to_str(command_id,command_names,"Unknown Command (0x%x)");
            item = proto_tree_add_item(u3v_tree, hf_u3v_ccd_cmd, tvb, offset, 8, ENC_NA);
            proto_item_append_text(item, ": %s", command_string);
            ccd_tree = proto_item_add_subtree(item, ett_u3v_cmd);

            /* Add the prefix code: */
            proto_tree_add_item(ccd_tree, hf_u3v_gencp_prefix, tvb, offset, 4, ENC_LITTLE_ENDIAN);
            offset += 4;

            /* Add the flags */
            item = proto_tree_add_item(ccd_tree, hf_u3v_flag, tvb, offset, 2, ENC_LITTLE_ENDIAN);
            ccd_tree_flag  = proto_item_add_subtree(item, ett_u3v_flags);
            proto_tree_add_item(ccd_tree_flag, hf_u3v_acknowledge_required_flag, tvb, offset, 2, ENC_LITTLE_ENDIAN);

            offset += 2;
            col_append_fstr(pinfo->cinfo, COL_INFO, "> %s ", command_string);
        } else if (prefix == U3V_CONTROL_PREFIX && ((command_id % 2) == 1)) {
            command_string = val_to_str(command_id,command_names,"Unknown Acknowledge (0x%x)");
            item = proto_tree_add_item(u3v_tree, hf_u3v_ccd_ack, tvb, offset, 8, ENC_NA);
            proto_item_append_text(item, ": %s", command_string);
            ccd_tree = proto_item_add_subtree(item, ett_u3v_ack);

            /* Add the prefix code: */
            proto_tree_add_item(ccd_tree, hf_u3v_gencp_prefix, tvb, offset, 4, ENC_LITTLE_ENDIAN);
            offset += 4;

            /* Add the status: */
            proto_tree_add_item(ccd_tree, hf_u3v_status, tvb, offset, 2,ENC_LITTLE_ENDIAN);
            status = tvb_get_letohs(tvb, offset);
            offset += 2;
            col_append_fstr(pinfo->cinfo, COL_INFO, "< %s %s",
                    command_string,
                    val_to_str(status, status_names_short, "Unknown status (0x%04X)"));
        } else {
            return 0;
        }

        /* Add the command id*/
        proto_tree_add_item(ccd_tree, hf_u3v_command_id, tvb, offset, 2,ENC_LITTLE_ENDIAN);
        offset += 2;

        /* Parse the second part of both the command and the acknowledge header:
        0          15 16         31
        -------- -------- -------- --------
        |     status      |   acknowledge   |
        -------- -------- -------- --------
        |     length      |      req_id     |
        -------- -------- -------- --------

        Add the data length
        Number of valid data bytes in this message, not including this header. This
        represents the number of bytes of payload appended after this header */

        proto_tree_add_item(ccd_tree, hf_u3v_length, tvb, offset, 2, ENC_LITTLE_ENDIAN);
        data_length = tvb_get_letohs(tvb, offset);
        offset += 2;

        /* Add the request ID */
        proto_tree_add_item(ccd_tree, hf_u3v_request_id, tvb, offset, 2, ENC_LITTLE_ENDIAN);
        req_id = tvb_get_letohs(tvb, offset);
        offset += 2;

        /* Add telegram subtree */
        u3v_telegram_tree = proto_item_add_subtree(u3v_tree, ett_u3v);

        if (!PINFO_FD_VISITED(pinfo)) {
              if ((command_id % 2) == 0) {
                    /* This is a command */
                    gencp_trans = wmem_new(wmem_file_scope(), gencp_transaction_t);
                    gencp_trans->cmd_frame = pinfo->fd->num;
                    gencp_trans->ack_frame = 0;
                    gencp_trans->cmd_time = pinfo->fd->abs_ts;
                    /* add reference to current packet */
                    p_add_proto_data(wmem_file_scope(), pinfo, proto_u3v, req_id, gencp_trans);
                    /* add reference to current */
                    u3v_conv_info->trans_info = gencp_trans;
                } else {
                    gencp_trans = u3v_conv_info->trans_info;
                    if (gencp_trans) {
                        gencp_trans->ack_frame = pinfo->fd->num;
                        /* add reference to current packet */
                        p_add_proto_data(wmem_file_scope(), pinfo, proto_u3v, req_id, gencp_trans);
                    }
                }
         } else {
            gencp_trans = (gencp_transaction_t*)p_get_proto_data(wmem_file_scope(),pinfo, proto_u3v, req_id);
         }

        if (!gencp_trans) {
            /* create a "fake" gencp_trans structure */
            gencp_trans = wmem_new(wmem_packet_scope(), gencp_transaction_t);
            gencp_trans->cmd_frame = 0;
            gencp_trans->ack_frame = 0;
            gencp_trans->cmd_time = pinfo->fd->abs_ts;
        }

        /* dissect depending on command? */
        switch (command_id) {
        case U3V_READMEM_CMD:
            dissect_u3v_read_mem_cmd(u3v_telegram_tree, tvb, pinfo, offset, data_length,u3v_conv_info,gencp_trans);
            break;
        case U3V_WRITEMEM_CMD:
            dissect_u3v_write_mem_cmd(u3v_telegram_tree, tvb, pinfo, offset, data_length,u3v_conv_info,gencp_trans);
            break;
        case U3V_EVENT_CMD:
            dissect_u3v_event_cmd(u3v_telegram_tree, tvb, pinfo, offset, data_length);
            break;
        case U3V_READMEM_ACK:
            if ( U3V_STATUS_GENCP_SUCCESS == status ) {
                dissect_u3v_read_mem_ack(u3v_telegram_tree, tvb, pinfo, offset, data_length,u3v_conv_info,gencp_trans);
            }
            break;
        case U3V_WRITEMEM_ACK:
            dissect_u3v_write_mem_ack(u3v_telegram_tree, tvb, pinfo, offset, data_length, u3v_conv_info,gencp_trans);
            break;
        case U3V_PENDING_ACK:
            dissect_u3v_pending_ack(u3v_telegram_tree, tvb, pinfo, offset, data_length, u3v_conv_info,gencp_trans);
            break;
        default:
            proto_tree_add_item(u3v_telegram_tree, hf_u3v_payloaddata, tvb, offset, data_length, ENC_NA);
            break;
        }
        return data_length + 12;
    } else if ( stream_detected ) {
        /* this is streaming data */

        /* init this stream configuration */
        u3v_conv_info = (u3v_conv_info_t *)usb_conv_info->class_data;
        u3v_conv_info->ep_stream = usb_conv_info->endpoint;

        /* Set the protocol column */
        col_set_str(pinfo->cinfo, COL_PROTOCOL, "U3V");

        /* Clear out stuff in the info column */
        col_clear(pinfo->cinfo, COL_INFO);

        /* Adds "USB3Vision" heading to protocol tree */
        /* We will add fields to this using the u3v_tree pointer */
        ti = proto_tree_add_item(tree, proto_u3v, tvb, offset, -1, ENC_NA);
        u3v_tree = proto_item_add_subtree(ti, ett_u3v);

        if(tvb_captured_length(tvb) >=4) {
            prefix = tvb_get_letohl(tvb, offset);
            switch (prefix) {
            case U3V_STREAM_LEADER_PREFIX:
                dissect_u3v_stream_leader(u3v_tree, tvb, pinfo, usb_conv_info);
                break;
            case U3V_STREAM_TRAILER_PREFIX:
                dissect_u3v_stream_trailer(u3v_tree, tvb, pinfo, usb_conv_info);
                break;
            default:
                dissect_u3v_stream_payload(u3v_tree, tvb, pinfo, usb_conv_info);
                break;
            }
        }
        return tvb_captured_length(tvb);
    }
    return 0;
}