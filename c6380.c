void AvahiService::resolved(int, int, const QString &name, const QString &, const QString &, const QString &h, int, const QString &, ushort p, const QList<QByteArray> &, uint)
{
    Q_UNUSED(name)
    port=p;
    host=h;
    stop();
    emit serviceResolved(name);
}