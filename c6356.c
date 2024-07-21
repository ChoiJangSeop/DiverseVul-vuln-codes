void Avahi::removeService(int, int, const QString &name, const QString &, const QString &domain, uint)
{
    if (isLocalDomain(domain) && services.contains(name)) {
        services[name]->deleteLater();
        disconnect(services[name], SIGNAL(serviceResolved(QString)), this, SIGNAL(serviceAdded(QString)));
        services.remove(name);
        emit serviceRemoved(name);
    }
}