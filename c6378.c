void RemoteDevicePropertiesWidget::update(const RemoteFsDevice::Details &d, bool create, bool isConnected)
{
    int t=create
            ? Type_Samba
            : d.isLocalFile()
                ? Type_File
                : d.url.scheme()==RemoteFsDevice::constSshfsProtocol
                    ? Type_SshFs
                    : d.url.scheme()==RemoteFsDevice::constSambaProtocol
                        ? Type_Samba
                        : Type_SambaAvahi;
    setEnabled(d.isLocalFile() || !isConnected);
    infoLabel->setVisible(create);
    orig=d;
    name->setText(d.name);
    sshPort->setValue(22);
    smbPort->setValue(445);

    connectionNote->setVisible(!d.isLocalFile() && isConnected);
    sshFolder->setText(QString());
    sshHost->setText(QString());
    sshUser->setText(QString());
    fileFolder->setText(QString());

    switch (t) {
    case Type_SshFs: {
        sshFolder->setText(d.url.path());
        if (0!=d.url.port()) {
            sshPort->setValue(d.url.port());
        }
        sshHost->setText(d.url.host());
        sshUser->setText(d.url.userName());
        sshExtra->setText(d.extraOptions);
        break;
    }
    case Type_File:
        fileFolder->setText(d.url.path());
        break;
    case Type_Samba: {
        smbShare->setText(d.url.path());
        if (0!=d.url.port()) {
            smbPort->setValue(d.url.port());
        }
        smbHost->setText(d.url.host());
        smbUser->setText(d.url.userName());
        smbPassword->setText(d.url.password());
        QUrlQuery q(d.url);
        if (q.hasQueryItem(RemoteFsDevice::constDomainQuery)) {
            smbDomain->setText(q.queryItemValue(RemoteFsDevice::constDomainQuery));
        } else {
            smbDomain->setText(QString());
        }
        break;
    }
    case Type_SambaAvahi: {
        smbAvahiShare->setText(d.url.path());
        smbAvahiUser->setText(d.url.userName());
        smbAvahiPassword->setText(d.url.password());
        QUrlQuery q(d.url);
        if (q.hasQueryItem(RemoteFsDevice::constDomainQuery)) {
            smbAvahiDomain->setText(q.queryItemValue(RemoteFsDevice::constDomainQuery));
        } else {
            smbAvahiDomain->setText(QString());
        }
        if (q.hasQueryItem(RemoteFsDevice::constServiceNameQuery)) {
            smbAvahiName->setText(q.queryItemValue(RemoteFsDevice::constServiceNameQuery));
        } else {
            smbAvahiName->setText(QString());
        }
        break;
    }
    }

    name->setEnabled(d.isLocalFile() || !isConnected);

    connect(type, SIGNAL(currentIndexChanged(int)), this, SLOT(setType()));
    for (int i=1; i<type->count(); ++i) {
        if (type->itemData(i).toInt()==t) {
            type->setCurrentIndex(i);
            stackedWidget->setCurrentIndex(i);
            break;
        }
    }
    connect(name, SIGNAL(textChanged(const QString &)), this, SLOT(checkSaveable()));
    connect(sshHost, SIGNAL(textChanged(const QString &)), this, SLOT(checkSaveable()));
    connect(sshUser, SIGNAL(textChanged(const QString &)), this, SLOT(checkSaveable()));
    connect(sshFolder, SIGNAL(textChanged(const QString &)), this, SLOT(checkSaveable()));
    connect(sshPort, SIGNAL(valueChanged(int)), this, SLOT(checkSaveable()));
    connect(sshExtra, SIGNAL(textChanged(const QString &)), this, SLOT(checkSaveable()));
    connect(fileFolder, SIGNAL(textChanged(const QString &)), this, SLOT(checkSaveable()));
    connect(smbHost, SIGNAL(textChanged(const QString &)), this, SLOT(checkSaveable()));
    connect(smbUser, SIGNAL(textChanged(const QString &)), this, SLOT(checkSaveable()));
    connect(smbPassword, SIGNAL(textChanged(const QString &)), this, SLOT(checkSaveable()));
    connect(smbDomain, SIGNAL(textChanged(const QString &)), this, SLOT(checkSaveable()));
    connect(smbShare, SIGNAL(textChanged(const QString &)), this, SLOT(checkSaveable()));
    connect(smbPort, SIGNAL(valueChanged(int)), this, SLOT(checkSaveable()));
    connect(smbAvahiName, SIGNAL(textChanged(const QString &)), this, SLOT(checkSaveable()));
    connect(smbAvahiUser, SIGNAL(textChanged(const QString &)), this, SLOT(checkSaveable()));
    connect(smbAvahiPassword, SIGNAL(textChanged(const QString &)), this, SLOT(checkSaveable()));
    connect(smbAvahiDomain, SIGNAL(textChanged(const QString &)), this, SLOT(checkSaveable()));
    connect(smbAvahiShare, SIGNAL(textChanged(const QString &)), this, SLOT(checkSaveable()));
    modified=false;
    setType();
    checkSaveable();
}