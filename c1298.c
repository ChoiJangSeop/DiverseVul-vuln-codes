ZrtpPacketPingAck* ZRtp::preparePingAck(ZrtpPacketPing* ppkt) {

    // Because we do not support ZRTP proxy mode use the truncated ZID.
    // If this code shall be used in ZRTP proxy implementation the computation
    // of the endpoint hash must be enhanced (see chaps 5.15ff and 5.16)
    zrtpPingAck.setLocalEpHash(ownZid);
    zrtpPingAck.setRemoteEpHash(ppkt->getEpHash());
    zrtpPingAck.setSSRC(peerSSRC);
    return &zrtpPingAck;
}