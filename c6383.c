static bool isLocalDomain(const QString &d)
{
    return QLatin1String("local")==d.section('.', -1, -1).toLower();
}