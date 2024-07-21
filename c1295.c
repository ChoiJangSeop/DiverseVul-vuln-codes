ZrtpQueue::takeInDataPacket(void)
{
    InetHostAddress network_address;
    tpport_t transport_port;

    uint32 nextSize = (uint32)getNextDataPacketSize();
    unsigned char* buffer = new unsigned char[nextSize];
    int32 rtn = (int32)recvData(buffer, nextSize, network_address, transport_port);
    if ( (rtn < 0) || ((uint32)rtn > getMaxRecvPacketSize()) ){
        delete buffer;
        return 0;
    }

    IncomingZRTPPkt* packet = NULL;
    // check if this could be a real RTP/SRTP packet.
    if ((*buffer & 0xf0) != 0x10) {
        return (rtpDataPacket(buffer, rtn, network_address, transport_port));
    }

    // We assume all other packets are ZRTP packets here. Process
    // if ZRTP processing is enabled. Because valid RTP packets are
    // already handled we delete any packets here after processing.
    if (enableZrtp && zrtpEngine != NULL) {
        // Get CRC value into crc (see above how to compute the offset)
        uint16_t temp = rtn - CRC_SIZE;
        uint32_t crc = *(uint32_t*)(buffer + temp);
        crc = ntohl(crc);

        if (!zrtpCheckCksum(buffer, temp, crc)) {
            delete buffer;
            if (zrtpUserCallback != NULL)
                zrtpUserCallback->showMessage(Warning, WarningCRCmismatch);
            return 0;
        }

        packet = new IncomingZRTPPkt(buffer,rtn);

        uint32 magic = packet->getZrtpMagic();

        // Check if it is really a ZRTP packet, if not delete it and return 0
        if (magic != ZRTP_MAGIC || zrtpEngine == NULL) {
            delete packet;
            return 0;
        }
        // cover the case if the other party sends _only_ ZRTP packets at the
        // beginning of a session. Start ZRTP in this case as well.
        if (!started) {
            startZrtp();
         }
        // this now points beyond the undefined and length field.
        // We need them, thus adjust
        unsigned char* extHeader =
                const_cast<unsigned char*>(packet->getHdrExtContent());
        extHeader -= 4;

        // store peer's SSRC, used when creating the CryptoContext
        peerSSRC = packet->getSSRC();
        zrtpEngine->processZrtpMessage(extHeader, peerSSRC, rtn);
    }
    delete packet;
    return 0;
}