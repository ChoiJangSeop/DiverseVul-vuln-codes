void RemoteFsDevice::unmount()
{
    if (details.isLocalFile()) {
        return;
    }

    if (!isConnected() || proc) {
        return;
    }

    if (messageSent) {
        return;
    }
    if (constSambaProtocol==details.url.scheme() || constSambaAvahiProtocol==details.url.scheme()) {
        mounter()->umount(mountPoint(details, false), getpid());
        setStatusMessage(tr("Disconnecting..."));
        messageSent=true;
        return;
    }

    QString cmd;
    QStringList args;
    if (!details.isLocalFile()) {
        QString mp=mountPoint(details, false);
        if (!mp.isEmpty()) {
            cmd=Utils::findExe("fusermount");
            if (!cmd.isEmpty()) {
                args << QLatin1String("-u") << QLatin1String("-z") << mp;
            } else {
                emit error(tr("\"fusermount\" is not installed!"));
            }
        }
    }

    if (!cmd.isEmpty()) {
        setStatusMessage(tr("Disconnecting..."));
        proc=new QProcess(this);
        proc->setProperty("unmount", true);
        connect(proc, SIGNAL(finished(int)), SLOT(procFinished(int)));
        proc->start(cmd, args, QIODevice::ReadOnly);
    }
}