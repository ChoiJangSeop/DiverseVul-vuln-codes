void RemoteFsDevice::serviceRemoved(const QString &name)
{
    if (name==details.serviceName && constSambaAvahiProtocol==details.url.scheme()) {
        sub=tr("Not Available");
        updateStatus();
    }
}