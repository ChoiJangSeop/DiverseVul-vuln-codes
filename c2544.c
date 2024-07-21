void CoreUserInputHandler::handleSay(const BufferInfo &bufferInfo, const QString &msg)
{
    if (bufferInfo.bufferName().isEmpty() || !bufferInfo.acceptsRegularMessages())
        return;  // server buffer

    QByteArray encMsg = channelEncode(bufferInfo.bufferName(), msg);
#ifdef HAVE_QCA2
    putPrivmsg(serverEncode(bufferInfo.bufferName()), encMsg, network()->cipher(bufferInfo.bufferName()));
#else
    putPrivmsg(serverEncode(bufferInfo.bufferName()), encMsg);
#endif
    emit displayMsg(Message::Plain, bufferInfo.type(), bufferInfo.bufferName(), msg, network()->myNick(), Message::Self);
}