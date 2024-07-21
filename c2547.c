void CoreUserInputHandler::handleMsg(const BufferInfo &bufferInfo, const QString &msg)
{
    Q_UNUSED(bufferInfo);
    if (!msg.contains(' '))
        return;

    QString target = msg.section(' ', 0, 0);
    QByteArray encMsg = userEncode(target, msg.section(' ', 1));

#ifdef HAVE_QCA2
    putPrivmsg(serverEncode(target), encMsg, network()->cipher(target));
#else
    putPrivmsg(serverEncode(target), encMsg);
#endif
}