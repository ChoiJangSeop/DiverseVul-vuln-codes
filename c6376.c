void Avahi::addService(int, int, const QString &name, const QString &type, const QString &domain, uint)
{
    if (isLocalDomain(domain) && !services.contains(name)) {
        AvahiService *srv=new AvahiService(name, type, domain);
        services.insert(name, srv);
        connect(srv, SIGNAL(serviceResolved(QString)), this, SIGNAL(serviceAdded(QString)));
    }
}