void MainWindow::on_actionUpgrade_triggered()
{
    if (Settings.askUpgradeAutmatic()) {
        QMessageBox dialog(QMessageBox::Question,
           qApp->applicationName(),
           tr("Do you want to automatically check for updates in the future?"),
           QMessageBox::No |
           QMessageBox::Yes,
           this);
        dialog.setWindowModality(QmlApplication::dialogModality());
        dialog.setDefaultButton(QMessageBox::Yes);
        dialog.setEscapeButton(QMessageBox::No);
        dialog.setCheckBox(new QCheckBox(tr("Do not show this anymore.", "Automatic upgrade check dialog")));
        Settings.setCheckUpgradeAutomatic(dialog.exec() == QMessageBox::Yes);
        if (dialog.checkBox()->isChecked())
            Settings.setAskUpgradeAutomatic(false);
    }
    showStatusMessage("Checking for upgrade...");
    m_network.get(QNetworkRequest(QUrl("http://check.shotcut.org/version.json")));
}