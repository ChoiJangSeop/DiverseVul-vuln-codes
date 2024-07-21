void CoreNetwork::putCmd(const QString &cmd, const QList<QByteArray> &params, const QByteArray &prefix)
{
    QByteArray msg;

    if (!prefix.isEmpty())
        msg += ":" + prefix + " ";
    msg += cmd.toUpper().toLatin1();

    for (int i = 0; i < params.size(); i++) {
        msg += " ";

        if (i == params.size() - 1 && (params[i].contains(' ') || (!params[i].isEmpty() && params[i][0] == ':')))
            msg += ":";

        msg += params[i];
    }

    putRawLine(msg);
}