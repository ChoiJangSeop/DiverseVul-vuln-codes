Client::haveParsedReplyHeaders()
{
    Must(theFinalReply);
    maybePurgeOthers();

    // adaptation may overwrite old offset computed using the virgin response
    const bool partial = theFinalReply->contentRange();
    currentOffset = partial ? theFinalReply->contentRange()->spec.offset : 0;
}