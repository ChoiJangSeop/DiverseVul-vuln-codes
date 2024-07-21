AvahiService::AvahiService(const QString &n, const QString &type, const QString &domain)
    : name(n)
    , port(0)
{
    static bool registeredTypes=false;
    if (!registeredTypes) {
        qDBusRegisterMetaType<QList<QByteArray> >();
        registeredTypes=true;
    }

    org::freedesktop::Avahi::Server server("org.freedesktop.Avahi", "/", QDBusConnection::systemBus());
    QDBusReply<QDBusObjectPath> reply=server.ServiceResolverNew(-1, -1, name, type, Avahi::domainToDNS(domain), -1, 8 /*AVAHI_LOOKUP_NO_ADDRESS|AVAHI_LOOKUP_NO_TXT*/);

    if (reply.isValid()) {
        resolver=new OrgFreedesktopAvahiServiceResolverInterface("org.freedesktop.Avahi", reply.value().path(), QDBusConnection::systemBus());
        connect(resolver, SIGNAL(Found(int,int,const QString &,const QString &,const QString &,const QString &, int, const QString &,ushort,const QList<QByteArray>&, uint)),
                this, SLOT(resolved(int,int,const QString &,const QString &,const QString &,const QString &, int, const QString &,ushort, const QList<QByteArray>&, uint)));
        connect(resolver, SIGNAL(Failure(QString)), this, SLOT(error(QString)));
    }
}