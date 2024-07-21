void ZrtpStateClass::processEvent(Event_t *ev) {

    char *msg, first, middle, last;
    uint8_t *pkt;

    parent->synchEnter();

    event = ev;
    if (event->type == ZrtpPacket) {
        pkt = event->packet;
        msg = (char *)pkt + 4;
        first = tolower(*msg);
        middle = tolower(*(msg+4));
        last = tolower(*(msg+7));

        // Sanity check of packet size for all states except WaitErrorAck.
        if (!inState(WaitErrorAck)) {
            uint16_t totalLength = *(uint16_t*)(pkt+2);
            totalLength = zrtpNtohs(totalLength) * ZRTP_WORD_SIZE;
            totalLength += 12 + sizeof(uint32_t);           // 12 bytes is fixed header, uint32_t is CRC

            if (totalLength != ev->length) {
                fprintf(stderr, "Total length does not match received length: %d - %ld\n", totalLength, ev->length);
                sendErrorPacket(MalformedPacket);
                parent->synchLeave();
                return;
            }
        }

        // Check if this is an Error packet.
        if (first == 'e' && middle =='r' && last == ' ') {
            /*
             * Process a received Error packet.
             *
             * In any case stop timer to prevent resending packets.
             * Use callback method to prepare and get an ErrorAck packet.
             * Modify event type to "ErrorPkt" and hand it over to current
             * state for further processing.
             */
            cancelTimer();
            ZrtpPacketError epkt(pkt);
            ZrtpPacketErrorAck* eapkt = parent->prepareErrorAck(&epkt);
            parent->sendPacketZRTP(static_cast<ZrtpPacketBase *>(eapkt));
            event->type = ErrorPkt;
        }
        else if (first == 'p' && middle == ' ' && last == ' ') {
            ZrtpPacketPing ppkt(pkt);
            ZrtpPacketPingAck* ppktAck = parent->preparePingAck(&ppkt);
            parent->sendPacketZRTP(static_cast<ZrtpPacketBase *>(ppktAck));
            parent->synchLeave();
            return;
        }
        else if (first == 's' && last == 'y') {
            uint32_t errorCode = 0;
            ZrtpPacketSASrelay* srly = new ZrtpPacketSASrelay(pkt);
            ZrtpPacketRelayAck* rapkt = parent->prepareRelayAck(srly, &errorCode);
            parent->sendPacketZRTP(static_cast<ZrtpPacketBase *>(rapkt));
            parent->synchLeave();
            return;
        }
    }
    /*
     * Shut down protocol state engine: cancel outstanding timer, further
     * processing in current state.
     */
    else if (event->type == ZrtpClose) {
        cancelTimer();
    }
    engine->processEvent(*this);
    parent->synchLeave();
}