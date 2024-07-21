void RemoteDevicePropertiesWidget::checkSaveable()
{
    RemoteFsDevice::Details det=details();
    modified=det!=orig;
    saveable=!det.isEmpty();
    if (saveable && Type_SambaAvahi==type->itemData(type->currentIndex()).toInt()) {
        saveable=!smbAvahiName->text().trimmed().isEmpty();
    }
    emit updated();
}