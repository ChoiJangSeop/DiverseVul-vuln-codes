void CoreUserInputHandler::putPrivmsg(const QByteArray &target, const QByteArray &message, Cipher *cipher)
{
    // Encrypted messages need special care. There's no clear relation between cleartext and encrypted message length,
    // so we can't just compute the maxSplitPos. Instead, we need to loop through the splitpoints until the crypted
    // version is short enough...
    // TODO: check out how the various possible encryption methods behave length-wise and make
    //       this clean by predicting the length of the crypted msg.
    //       For example, blowfish-ebc seems to create 8-char chunks.

    static const char *cmd = "PRIVMSG";
    static const char *splitter = " .,-!?";

    int maxSplitPos = message.count();
    int splitPos = maxSplitPos;
    forever {
        QByteArray crypted = message.left(splitPos);
        bool isEncrypted = false;
#ifdef HAVE_QCA2
        if (cipher && !cipher->key().isEmpty() && !message.isEmpty()) {
            isEncrypted = cipher->encrypt(crypted);
        }
#endif
        int overrun = lastParamOverrun(cmd, QList<QByteArray>() << target << crypted);
        if (overrun) {
            // In case this is not an encrypted msg, we can just cut off at the end
            if (!isEncrypted)
                maxSplitPos = message.count() - overrun;

            splitPos = -1;
            for (const char *splitChar = splitter; *splitChar != 0; splitChar++) {
                splitPos = qMax(splitPos, message.lastIndexOf(*splitChar, maxSplitPos) + 1); // keep split char on old line
            }
            if (splitPos <= 0 || splitPos > maxSplitPos)
                splitPos = maxSplitPos;

            maxSplitPos = splitPos - 1;
            if (maxSplitPos <= 0) { // this should never happen, but who knows...
                qWarning() << tr("[Error] Could not encrypt your message: %1").arg(message.data());
                return;
            }
            continue; // we never come back here for !encrypted!
        }

        // now we have found a valid splitpos (or didn't need to split to begin with)
        putCmd(cmd, QList<QByteArray>() << target << crypted);
        if (splitPos < message.count())
            putPrivmsg(target, message.mid(splitPos), cipher);

        return;
    }
}