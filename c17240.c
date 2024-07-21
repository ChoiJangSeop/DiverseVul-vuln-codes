static void lsi_do_msgout(LSIState *s)
{
    uint8_t msg;
    int len;
    uint32_t current_tag;
    lsi_request *current_req, *p, *p_next;

    if (s->current) {
        current_tag = s->current->tag;
        current_req = s->current;
    } else {
        current_tag = s->select_tag;
        current_req = lsi_find_by_tag(s, current_tag);
    }

    trace_lsi_do_msgout(s->dbc);
    while (s->dbc) {
        msg = lsi_get_msgbyte(s);
        s->sfbr = msg;

        switch (msg) {
        case 0x04:
            trace_lsi_do_msgout_disconnect();
            lsi_disconnect(s);
            break;
        case 0x08:
            trace_lsi_do_msgout_noop();
            lsi_set_phase(s, PHASE_CMD);
            break;
        case 0x01:
            len = lsi_get_msgbyte(s);
            msg = lsi_get_msgbyte(s);
            (void)len; /* avoid a warning about unused variable*/
            trace_lsi_do_msgout_extended(msg, len);
            switch (msg) {
            case 1:
                trace_lsi_do_msgout_ignored("SDTR");
                lsi_skip_msgbytes(s, 2);
                break;
            case 3:
                trace_lsi_do_msgout_ignored("WDTR");
                lsi_skip_msgbytes(s, 1);
                break;
            case 4:
                trace_lsi_do_msgout_ignored("PPR");
                lsi_skip_msgbytes(s, 5);
                break;
            default:
                goto bad;
            }
            break;
        case 0x20: /* SIMPLE queue */
            s->select_tag |= lsi_get_msgbyte(s) | LSI_TAG_VALID;
            trace_lsi_do_msgout_simplequeue(s->select_tag & 0xff);
            break;
        case 0x21: /* HEAD of queue */
            qemu_log_mask(LOG_UNIMP, "lsi_scsi: HEAD queue not implemented\n");
            s->select_tag |= lsi_get_msgbyte(s) | LSI_TAG_VALID;
            break;
        case 0x22: /* ORDERED queue */
            qemu_log_mask(LOG_UNIMP,
                          "lsi_scsi: ORDERED queue not implemented\n");
            s->select_tag |= lsi_get_msgbyte(s) | LSI_TAG_VALID;
            break;
        case 0x0d:
            /* The ABORT TAG message clears the current I/O process only. */
            trace_lsi_do_msgout_abort(current_tag);
            if (current_req && current_req->req) {
                scsi_req_cancel(current_req->req);
                current_req->req = NULL;
            }
            lsi_disconnect(s);
            break;
        case 0x06:
        case 0x0e:
        case 0x0c:
            /* The ABORT message clears all I/O processes for the selecting
               initiator on the specified logical unit of the target. */
            if (msg == 0x06) {
                trace_lsi_do_msgout_abort(current_tag);
            }
            /* The CLEAR QUEUE message clears all I/O processes for all
               initiators on the specified logical unit of the target. */
            if (msg == 0x0e) {
                trace_lsi_do_msgout_clearqueue(current_tag);
            }
            /* The BUS DEVICE RESET message clears all I/O processes for all
               initiators on all logical units of the target. */
            if (msg == 0x0c) {
                trace_lsi_do_msgout_busdevicereset(current_tag);
            }

            /* clear the current I/O process */
            if (s->current) {
                scsi_req_cancel(s->current->req);
            }

            /* As the current implemented devices scsi_disk and scsi_generic
               only support one LUN, we don't need to keep track of LUNs.
               Clearing I/O processes for other initiators could be possible
               for scsi_generic by sending a SG_SCSI_RESET to the /dev/sgX
               device, but this is currently not implemented (and seems not
               to be really necessary). So let's simply clear all queued
               commands for the current device: */
            QTAILQ_FOREACH_SAFE(p, &s->queue, next, p_next) {
                if ((p->tag & 0x0000ff00) == (current_tag & 0x0000ff00)) {
                    scsi_req_cancel(p->req);
                }
            }

            lsi_disconnect(s);
            break;
        default:
            if ((msg & 0x80) == 0) {
                goto bad;
            }
            s->current_lun = msg & 7;
            trace_lsi_do_msgout_select(s->current_lun);
            lsi_set_phase(s, PHASE_CMD);
            break;
        }
    }
    return;
bad:
    qemu_log_mask(LOG_UNIMP, "Unimplemented message 0x%02x\n", msg);
    lsi_set_phase(s, PHASE_MI);
    lsi_add_msg_byte(s, 7); /* MESSAGE REJECT */
    s->msg_action = LSI_MSG_ACTION_COMMAND;
}