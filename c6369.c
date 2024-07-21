RemoteFsDevice::Details RemoteDevicePropertiesWidget::details()
{
    int t=type->itemData(type->currentIndex()).toInt();
    RemoteFsDevice::Details det;

    det.name=name->text().trimmed();
    switch (t) {
    case Type_SshFs: {
        det.url.setHost(sshHost->text().trimmed());
        det.url.setUserName(sshUser->text().trimmed());
        det.url.setPath(sshFolder->text().trimmed());
        det.url.setPort(sshPort->value());
        det.url.setScheme(RemoteFsDevice::constSshfsProtocol);
        det.extraOptions=sshExtra->text().trimmed();
        break;
    }
    case Type_File: {
        QString path=fileFolder->text().trimmed();
        if (path.isEmpty()) {
            path="/";
        }
        det.url.setPath(path);
        det.url.setScheme(RemoteFsDevice::constFileProtocol);
        break;
    }
    case Type_Samba:
        det.url.setHost(smbHost->text().trimmed());
        det.url.setUserName(smbUser->text().trimmed());
        det.url.setPath(smbShare->text().trimmed());
        det.url.setPort(smbPort->value());
        det.url.setScheme(RemoteFsDevice::constSambaProtocol);
        det.url.setPassword(smbPassword->text().trimmed());
        if (!smbDomain->text().trimmed().isEmpty()) {
            QUrlQuery q;
            q.addQueryItem(RemoteFsDevice::constDomainQuery, smbDomain->text().trimmed());
            det.url.setQuery(q);
        }
        break;
    case Type_SambaAvahi:
        det.url.setUserName(smbAvahiUser->text().trimmed());
        det.url.setPath(smbAvahiShare->text().trimmed());
        det.url.setPort(0);
        det.url.setScheme(RemoteFsDevice::constSambaAvahiProtocol);
        det.url.setPassword(smbAvahiPassword->text().trimmed());
        if (!smbDomain->text().trimmed().isEmpty() || !smbAvahiName->text().trimmed().isEmpty()) {
            QUrlQuery q;
            if (!smbDomain->text().trimmed().isEmpty()) {
                q.addQueryItem(RemoteFsDevice::constDomainQuery, smbAvahiDomain->text().trimmed());
            }
            if (!smbAvahiName->text().trimmed().isEmpty()) {
                det.serviceName=smbAvahiName->text().trimmed();
                q.addQueryItem(RemoteFsDevice::constServiceNameQuery, det.serviceName);
            }
            det.url.setQuery(q);
        }
        break;
    }
    return det;
}