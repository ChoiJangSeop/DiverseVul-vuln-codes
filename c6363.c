void Mounter::mountResult(int st)
{
    QProcess *proc=dynamic_cast<QProcess *>(sender());
    qWarning() << "MOUNT RESULT" << st << (void *)proc;
    if (proc) {
        procCount--;
        proc->close();
        proc->deleteLater();
        if (tempFiles.contains(proc)) {
            tempFiles[proc]->close();
            tempFiles[proc]->deleteLater();
            tempFiles.remove(proc);
        }
        emit mountStatus(proc->property("mp").toString(), proc->property("pid").toInt(), st);
    }
    startTimer();
}