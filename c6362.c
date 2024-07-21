void AvahiService::stop()
{
    if (resolver) {
        resolver->Free();
        resolver->deleteLater();
        disconnect(resolver, SIGNAL(Found(int,int,const QString &,const QString &,const QString &,const QString &, int, const QString &,ushort,const QList<QByteArray>&, uint)),
                   this, SLOT(resolved(int,int,const QString &,const QString &,const QString &,const QString &, int, const QString &,ushort, const QList<QByteArray>&, uint)));
        disconnect(resolver, SIGNAL(Failure(QString)), this, SLOT(error(QString)));
        resolver=0;
    }
}