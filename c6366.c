MpdCantataMounterInterface * RemoteFsDevice::mounter()
{
    if (!mounterIface) {
        if (!QDBusConnection::systemBus().interface()->isServiceRegistered(MpdCantataMounterInterface::staticInterfaceName())) {
            QDBusConnection::systemBus().interface()->startService(MpdCantataMounterInterface::staticInterfaceName());
        }
        mounterIface=new MpdCantataMounterInterface(MpdCantataMounterInterface::staticInterfaceName(),
                                                              "/Mounter", QDBusConnection::systemBus(), this);
        connect(mounterIface, SIGNAL(mountStatus(const QString &, int, int)), SLOT(mountStatus(const QString &, int, int)));
        connect(mounterIface, SIGNAL(umountStatus(const QString &, int, int)), SLOT(umountStatus(const QString &, int, int)));
    }
    return mounterIface;
}