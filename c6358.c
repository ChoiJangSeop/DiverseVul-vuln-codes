void RemoteFsDevice::mount()
{
    if (details.isLocalFile()) {
        return;
    }
    if (isConnected() || proc) {
        return;
    }

    if (messageSent) {
        return;
    }
    if (constSambaAvahiProtocol==details.url.scheme()) {
        Details det=details;
        AvahiService *srv=Avahi::self()->getService(det.serviceName);
        if (!srv || srv->getHost().isEmpty() || 0==srv->getPort()) {
            emit error(tr("Failed to resolve connection details for %1").arg(details.name));
            return;
        }
        if (constPromptPassword==det.url.password()) {
            bool ok=false;
            QString passwd=InputDialog::getPassword(QString(), &ok, QApplication::activeWindow());
            if (!ok) {
                return;
            }
            det.url.setPassword(passwd);
        }
        det.url.setScheme(constSambaProtocol);
        det.url.setHost(srv->getHost());
        det.url.setPort(srv->getPort());
        mounter()->mount(det.url.toString(), mountPoint(details, true), getuid(), getgid(), getpid());
        setStatusMessage(tr("Connecting..."));
        messageSent=true;
        return;
    }
    if (constSambaProtocol==details.url.scheme()) {
        Details det=details;
        if (constPromptPassword==det.url.password()) {
            bool ok=false;
            QString passwd=InputDialog::getPassword(QString(), &ok, QApplication::activeWindow());
            if (!ok) {
                return;
            }
            det.url.setPassword(passwd);
        }
        mounter()->mount(det.url.toString(), mountPoint(details, true), getuid(), getgid(), getpid());
        setStatusMessage(tr("Connecting..."));
        messageSent=true;
        return;
    }

    QString cmd;
    QStringList args;
    QString askPass;
    if (!details.isLocalFile() && !details.isEmpty()) {
        // If user has added 'IdentityFile' to extra options, then no password prompting is required...
        bool needAskPass=!details.extraOptions.contains("IdentityFile=");

        if (needAskPass) {
            QStringList askPassList;
            if (Utils::KDE==Utils::currentDe()) {
                askPassList << QLatin1String("ksshaskpass") << QLatin1String("ssh-askpass") << QLatin1String("ssh-askpass-gnome");
            } else {
                askPassList << QLatin1String("ssh-askpass-gnome") << QLatin1String("ssh-askpass") << QLatin1String("ksshaskpass");
            }

            for (const QString &ap: askPassList) {
                askPass=Utils::findExe(ap);
                if (!askPass.isEmpty()) {
                    break;
                }
            }

            if (askPass.isEmpty()) {
                emit error(tr("No suitable ssh-askpass application installed! This is required for entering passwords."));
                return;
            }
        }
        QString sshfs=Utils::findExe("sshfs");
        if (sshfs.isEmpty()) {
            emit error(tr("\"sshfs\" is not installed!"));
            return;
        }
        cmd=Utils::findExe("setsid");
        if (!cmd.isEmpty()) {
            QString mp=mountPoint(details, true);
            if (mp.isEmpty()) {
                emit error("Failed to determine mount point"); // TODO: 2.4 make translatable. For now, error should never happen!
            }
            if (!QDir(mp).entryList(QDir::NoDot|QDir::NoDotDot|QDir::AllEntries|QDir::Hidden).isEmpty()) {
                emit error(tr("Mount point (\"%1\") is not empty!").arg(mp));
                return;
            }

            args << sshfs << details.url.userName()+QChar('@')+details.url.host()+QChar(':')+details.url.path()<< QLatin1String("-p")
                 << QString::number(details.url.port()) << mountPoint(details, true)
                 << QLatin1String("-o") << QLatin1String("ServerAliveInterval=15");
            //<< QLatin1String("-o") << QLatin1String("Ciphers=arcfour");
            if (!details.extraOptions.isEmpty()) {
                args << details.extraOptions.split(' ', QString::SkipEmptyParts);
            }
        } else {
            emit error(tr("\"sshfs\" is not installed!").replace("sshfs", "setsid")); // TODO: 2.4 use correct string!
        }
    }

    if (!cmd.isEmpty()) {
        setStatusMessage(tr("Connecting..."));
        proc=new QProcess(this);
        proc->setProperty("mount", true);

        if (!askPass.isEmpty()) {
            QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
            env.insert("SSH_ASKPASS", askPass);
            proc->setProcessEnvironment(env);
        }
        connect(proc, SIGNAL(finished(int)), SLOT(procFinished(int)));
        proc->start(cmd, args, QIODevice::ReadOnly);
    }
}