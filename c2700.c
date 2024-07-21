void AbstractSqlStorage::addConnectionToPool()
{
    QMutexLocker locker(&_connectionPoolMutex);
    // we have to recheck if the connection pool already contains a connection for
    // this thread. Since now (after the lock) we can only tell for sure
    if (_connectionPool.contains(QThread::currentThread()))
        return;

    QThread *currentThread = QThread::currentThread();

    int connectionId = _nextConnectionId++;

    Connection *connection = new Connection(QLatin1String(QString("quassel_%1_con_%2").arg(driverName()).arg(connectionId).toLatin1()));
    connection->moveToThread(currentThread);
    connect(this, SIGNAL(destroyed()), connection, SLOT(deleteLater()));
    connect(currentThread, SIGNAL(destroyed()), connection, SLOT(deleteLater()));
    connect(connection, SIGNAL(destroyed()), this, SLOT(connectionDestroyed()));
    _connectionPool[currentThread] = connection;

    QSqlDatabase db = QSqlDatabase::addDatabase(driverName(), connection->name());
    db.setDatabaseName(databaseName());

    if (!hostName().isEmpty())
        db.setHostName(hostName());

    if (port() != -1)
        db.setPort(port());

    if (!userName().isEmpty()) {
        db.setUserName(userName());
        db.setPassword(password());
    }

    if (!db.open()) {
        quWarning() << "Unable to open database" << displayName() << "for thread" << QThread::currentThread();
        quWarning() << "-" << db.lastError().text();
    }
    else {
        if (!initDbSession(db)) {
            quWarning() << "Unable to initialize database" << displayName() << "for thread" << QThread::currentThread();
            db.close();
        }
    }
}