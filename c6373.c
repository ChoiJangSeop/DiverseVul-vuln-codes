void RemoteFsDevice::serviceAdded(const QString &name)
{
    if (name==details.serviceName && constSambaAvahiProtocol==details.url.scheme()) {
        sub=tr("Available");
        updateStatus();
    }
}