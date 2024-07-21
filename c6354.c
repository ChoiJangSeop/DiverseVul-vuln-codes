static QString fixPath(const QString &dir)
{
    QString d(dir);

    if (!d.isEmpty() && !d.startsWith(QLatin1String("http://"))) {
        d.replace(QLatin1String("//"), QChar('/'));
    }
    d.replace(QLatin1String("/./"), QChar('/'));
    if (!d.isEmpty() && !d.endsWith('/')) {
        d+='/';
    }
    return d;
}