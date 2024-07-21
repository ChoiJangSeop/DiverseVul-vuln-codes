static int ehci_execute(EHCIPacket *p, const char *action)
{
    USBEndpoint *ep;
    int endp;
    bool spd;

    assert(p->async == EHCI_ASYNC_NONE ||
           p->async == EHCI_ASYNC_INITIALIZED);

    if (!(p->qtd.token & QTD_TOKEN_ACTIVE)) {
        fprintf(stderr, "Attempting to execute inactive qtd\n");
        return -1;
    }

    if (get_field(p->qtd.token, QTD_TOKEN_TBYTES) > BUFF_SIZE) {
        ehci_trace_guest_bug(p->queue->ehci,
                             "guest requested more bytes than allowed");
        return -1;
    }

    if (!ehci_verify_pid(p->queue, &p->qtd)) {
        ehci_queue_stopped(p->queue); /* Mark the ep in the prev dir stopped */
    }
    p->pid = ehci_get_pid(&p->qtd);
    p->queue->last_pid = p->pid;
    endp = get_field(p->queue->qh.epchar, QH_EPCHAR_EP);
    ep = usb_ep_get(p->queue->dev, p->pid, endp);

    if (p->async == EHCI_ASYNC_NONE) {
        if (ehci_init_transfer(p) != 0) {
            return -1;
        }

        spd = (p->pid == USB_TOKEN_IN && NLPTR_TBIT(p->qtd.altnext) == 0);
        usb_packet_setup(&p->packet, p->pid, ep, 0, p->qtdaddr, spd,
                         (p->qtd.token & QTD_TOKEN_IOC) != 0);
        usb_packet_map(&p->packet, &p->sgl);
        p->async = EHCI_ASYNC_INITIALIZED;
    }

    trace_usb_ehci_packet_action(p->queue, p, action);
    usb_handle_packet(p->queue->dev, &p->packet);
    DPRINTF("submit: qh 0x%x next 0x%x qtd 0x%x pid 0x%x len %zd endp 0x%x "
            "status %d actual_length %d\n", p->queue->qhaddr, p->qtd.next,
            p->qtdaddr, p->pid, p->packet.iov.size, endp, p->packet.status,
            p->packet.actual_length);

    if (p->packet.actual_length > BUFF_SIZE) {
        fprintf(stderr, "ret from usb_handle_packet > BUFF_SIZE\n");
        return -1;
    }

    return 1;
}