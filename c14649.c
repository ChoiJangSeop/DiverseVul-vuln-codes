bool DDeviceDiskInfoPrivate::openDataStream(int index)
{
    if (process) {
        process->deleteLater();
    }

    process = new QProcess();

    QObject::connect(process, static_cast<void (QProcess::*)(int, QProcess::ExitStatus)>(&QProcess::finished),
                     process, [this] (int code, QProcess::ExitStatus status) {
        if (isClosing())
            return;

        if (status == QProcess::CrashExit) {
            setErrorString(QObject::tr("process \"%1 %2\" crashed").arg(process->program()).arg(process->arguments().join(" ")));
        } else if (code != 0) {
            setErrorString(QObject::tr("Failed to perform process \"%1 %2\", error: %3").arg(process->program()).arg(process->arguments().join(" ")).arg(QString::fromUtf8(process->readAllStandardError())));
        }
    });

    switch (currentScope) {
    case DDiskInfo::Headgear: {
        if (type != DDiskInfo::Disk) {
            setErrorString(QObject::tr("\"%1\" is not a disk device").arg(filePath()));

            return false;
        }

        if (currentMode == DDiskInfo::Read) {
            process->start(QStringLiteral("dd if=%1 bs=512 count=2048 status=none").arg(filePath()), QIODevice::ReadOnly);
        } else {
            process->start(QStringLiteral("dd of=%1 bs=512 status=none conv=fsync").arg(filePath()));
        }

        break;
    }
    case DDiskInfo::PartitionTable: {
        if (type != DDiskInfo::Disk) {
            setErrorString(QObject::tr("\"%1\" is not a disk device").arg(filePath()));

            return false;
        }

        if (currentMode == DDiskInfo::Read)
            process->start(QStringLiteral("sfdisk -d %1").arg(filePath()), QIODevice::ReadOnly);
        else
            process->start(QStringLiteral("sfdisk %1 --no-reread").arg(filePath()));

        break;
    }
    case DDiskInfo::Partition: {
        const DPartInfo &part = (index == 0 && currentMode == DDiskInfo::Write) ? DDevicePartInfo(filePath()) : q->getPartByNumber(index);

        if (!part) {
            dCDebug("Part is null(index: %d)", index);

            return false;
        }

        dCDebug("Try open device: %s, mode: %s", qPrintable(part.filePath()), currentMode == DDiskInfo::Read ? "Read" : "Write");

        if (Helper::isMounted(part.filePath())) {
            if (Helper::umountDevice(part.filePath())) {
                part.d->mountPoint.clear();
            } else {
                setErrorString(QObject::tr("\"%1\" is busy").arg(part.filePath()));

                return false;
            }
        }

        if (currentMode == DDiskInfo::Read) {
            const QString &executer = Helper::getPartcloneExecuter(part);
            process->start(QStringLiteral("%1 -s %2 -o - -c -z %3 -L /tmp/partclone.log").arg(executer).arg(part.filePath()).arg(Global::bufferSize), QIODevice::ReadOnly);
        } else {
            process->start(QStringLiteral("partclone.restore -s - -o %2 -z %3 -L /tmp/partclone.log").arg(part.filePath()).arg(Global::bufferSize));
        }

        break;
    }
    case DDiskInfo::JsonInfo: {
        process->deleteLater();
        process = 0;
        buffer.setData(q->toJson());
        break;
    }
    default:
        return false;
    }

    if (process) {
        if (!process->waitForStarted()) {
            setErrorString(QObject::tr("Failed to start \"%1 %2\", error: %3").arg(process->program()).arg(process->arguments().join(" ")).arg(process->errorString()));

            return false;
        }

        dCDebug("The \"%s %s\" command start finished", qPrintable(process->program()), qPrintable(process->arguments().join(" ")));
    }

    bool ok = process ? process->isOpen() : buffer.open(QIODevice::ReadOnly);

    if (!ok) {
        setErrorString(QObject::tr("Failed to open process, error: %1").arg(process ? process->errorString(): buffer.errorString()));
    }

    return ok;
}