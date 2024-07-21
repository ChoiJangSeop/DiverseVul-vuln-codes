void Mounter::mount(const QString &url, const QString &mountPoint, int uid, int gid, int pid)
{
    if (calledFromDBus()) {
        registerPid(pid);
    }

    qWarning() << url << mountPoint << uid << gid;
    QUrl u(url);
    int st=-1;

    if (u.scheme()=="smb" && mpOk(mountPoint)) {
        QString user=u.userName();
        QString domain;
        QString password=u.password();
        int port=u.port();

        #if QT_VERSION < 0x050000
        if (u.hasQueryItem("domain")) {
            domain=u.queryItemValue("domain");
        }
        #else
        QUrlQuery q(u);
        if (q.hasQueryItem("domain")) {
            domain=q.queryItemValue("domain");
        }
        #endif

        QTemporaryFile *temp=0;

        if (!password.isEmpty()) {
            temp=new QTemporaryFile();
            if (temp && temp->open()) {
                QTextStream str(temp);
                if (!user.isEmpty()) {
                    str << "username=" << user << endl;
                }
                str << "password=" << password << endl;
                if (!domain.isEmpty()) {
                    str << "domain=" << domain << endl;
                }
            }
        }

        QString path=fixPath(u.host()+"/"+u.path());
        while (!path.startsWith("//")) {
            path="/"+path;
        }

//        qWarning() << "EXEC" << "mount.cifs" << path << mountPoint
//                                                         << "-o" <<
//                                                         (temp ? ("credentials="+temp->fileName()+",") : QString())+
//                                                         "uid="+QString::number(uid)+",gid="+QString::number(gid)+
//                                                         (445==port || port<1 ? QString() : ",port="+QString::number(port))+
//                                                         (temp || user.isEmpty() ? QString() : (",username="+user))+
//                                                         (temp || domain.isEmpty() ? QString() : (",domain="+domain))+
//                                                         (temp ? QString() : ",password=");
        QProcess *proc=new QProcess(this);
        connect(proc, SIGNAL(finished(int)), SLOT(mountResult(int)));
        proc->setProperty("mp", mountPoint);
        proc->setProperty("pid", pid);
        proc->start(QLatin1String(INSTALL_PREFIX"/share/cantata/scripts/mount.cifs.wrapper"),
                    QStringList() << path << mountPoint
                    << "-o" <<
                    (temp ? ("credentials="+temp->fileName()+",") : QString())+
                    "uid="+QString::number(uid)+",gid="+QString::number(gid)+
                    (445==port || port<1 ? QString() : ",port="+QString::number(port))+
                    (temp || user.isEmpty() ? QString() : (",username="+user))+
                    (temp || domain.isEmpty() ? QString() : (",domain="+domain))+
                    (temp ? QString() : ",password="), QIODevice::WriteOnly);
        if (temp) {
            tempFiles.insert(proc, temp);
        }
        procCount++;
        return;
    }
    emit mountStatus(mountPoint, pid, st);
}