AvahiService * Avahi::getService(const QString &name)
{
    return services.contains(name) ? services[name] : 0;
}