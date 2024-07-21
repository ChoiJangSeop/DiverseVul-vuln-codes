void RemoteDevicePropertiesWidget::setType()
{
    if (Type_SshFs==type->itemData(type->currentIndex()).toInt() && 0==sshPort->value()) {
        sshPort->setValue(22);
    }
    if (Type_Samba==type->itemData(type->currentIndex()).toInt() && 0==smbPort->value()) {
        smbPort->setValue(445);
    }
}