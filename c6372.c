void Mounter::umount(const QString &mountPoint, int pid)
{
    if (calledFromDBus()) {
        registerPid(pid);
    }

    if (mpOk(mountPoint)) {
        QProcess *proc=new QProcess(this);
        connect(proc, SIGNAL(finished(int)), SLOT(umountResult(int)));
        proc->start("umount", QStringList() << mountPoint);
        proc->setProperty("mp", mountPoint);
        proc->setProperty("pid", pid);
        procCount++;
    } else {
        emit umountStatus(mountPoint, pid, -1);
    }
}