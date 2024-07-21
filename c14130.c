void MainWindow::showUpgradePrompt()
{
    if (Settings.checkUpgradeAutomatic()) {
        showStatusMessage("Checking for upgrade...");
        QNetworkRequest request(QUrl("https://check.shotcut.org/version.json"));
        QSslConfiguration sslConfig = request.sslConfiguration();
        sslConfig.setPeerVerifyMode(QSslSocket::VerifyNone);
        request.setSslConfiguration(sslConfig);
        m_network.get(request);
    } else {
        m_network.setStrictTransportSecurityEnabled(false);
        QAction* action = new QAction(tr("Click here to check for a new version of Shotcut."), 0);
        connect(action, SIGNAL(triggered(bool)), SLOT(on_actionUpgrade_triggered()));
        showStatusMessage(action, 15 /* seconds */);
    }
}