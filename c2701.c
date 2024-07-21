QSqlDatabase AbstractSqlStorage::logDb()
{
    if (!_connectionPool.contains(QThread::currentThread()))
        addConnectionToPool();

    return QSqlDatabase::database(_connectionPool[QThread::currentThread()]->name());
}