void Mounter::timeout()
{
    if (procCount!=0) {
        startTimer();
        return;
    }

    QSet<int> running;

    for (int p: pids) {
        if (0==kill(p, 0)) {
            running.insert(p);
        }
    }

    pids=running;

    if (pids.isEmpty()) {
        qApp->exit();
        QMap<QObject *, QTemporaryFile *>::ConstIterator it(tempFiles.constBegin());
        QMap<QObject *, QTemporaryFile *>::ConstIterator end(tempFiles.constEnd());
        for (; it!=end; ++it) {
            it.value()->close();
            delete it.value();
        }
        tempFiles.clear();
    } else {
        startTimer();
    }
}