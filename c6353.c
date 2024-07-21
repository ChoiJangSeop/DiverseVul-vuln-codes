QString Avahi::domainToDNS(const QString &domain)
{
    return isLocalDomain(domain) ? domain : QUrl::toAce(domain);
}