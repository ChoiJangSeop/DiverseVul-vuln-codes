RemoteFsDevice::RemoteFsDevice(MusicLibraryModel *m, const DeviceOptions &options, const Details &d)
    : FsDevice(m, d.name, createUdi(d.name))
    , mountToken(0)
    , currentMountStatus(false)
    , details(d)
    , proc(0)
    , mounterIface(0)
    , messageSent(false)
{
    opts=options;
//    details.path=Utils::fixPath(details.path);
    load();
    mount();
    icn=MonoIcon::icon(details.isLocalFile()
                       ? FontAwesome::foldero
                       : constSshfsProtocol==details.url.scheme()
                         ? FontAwesome::linux_os
                         : FontAwesome::windows, Utils::monoIconColor());
}