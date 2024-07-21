Database::Database() {
	QSqlDatabase db = QSqlDatabase::addDatabase(QLatin1String("QSQLITE"));
	QSettings qs;
	QStringList datapaths;
	int i;

	datapaths << g.qdBasePath.absolutePath();
	datapaths << QDesktopServices::storageLocation(QDesktopServices::DataLocation);
#if defined(Q_OS_UNIX) && ! defined(Q_OS_MAC)
	datapaths << QDir::homePath() + QLatin1String("/.config/Mumble");
#endif
	datapaths << QDir::homePath();
	datapaths << QDir::currentPath();
	datapaths << qApp->applicationDirPath();
	datapaths << qs.value(QLatin1String("InstPath")).toString();
	bool found = false;

	for (i = 0; (i < datapaths.size()) && ! found; i++) {
		if (!datapaths[i].isEmpty()) {
			QFile f(datapaths[i] + QLatin1String("/mumble.sqlite"));
			if (f.exists()) {
				db.setDatabaseName(f.fileName());
				found = db.open();
			}

			QFile f2(datapaths[i] + QLatin1String("/.mumble.sqlite"));
			if (f2.exists()) {
				db.setDatabaseName(f2.fileName());
				found = db.open();
			}
		}
	}

	if (! found) {
		for (i = 0; (i < datapaths.size()) && ! found; i++) {
			if (!datapaths[i].isEmpty()) {
				QDir::root().mkpath(datapaths[i]);
#ifdef Q_OS_WIN
				QFile f(datapaths[i] + QLatin1String("/mumble.sqlite"));
#else
				QFile f(datapaths[i] + QLatin1String("/.mumble.sqlite"));
#endif
				db.setDatabaseName(f.fileName());
				found = db.open();
			}
		}
	}

	if (! found) {
		QMessageBox::critical(NULL, QLatin1String("Mumble"), tr("Mumble failed to initialize a database in any\nof the possible locations."), QMessageBox::Ok | QMessageBox::Default, QMessageBox::NoButton);
		qFatal("Database: Failed initialization");
	}

	QFileInfo fi(db.databaseName());

	if (! fi.isWritable()) {
		QMessageBox::critical(NULL, QLatin1String("Mumble"), tr("The database '%1' is read-only. Mumble cannot store server settings (i.e. SSL certificates) until you fix this problem.").arg(fi.filePath()), QMessageBox::Ok | QMessageBox::Default, QMessageBox::NoButton);
		qWarning("Database: Database is read-only");
	}

	QSqlQuery query;

	query.exec(QLatin1String("CREATE TABLE IF NOT EXISTS `servers` (`id` INTEGER PRIMARY KEY AUTOINCREMENT, `name` TEXT, `hostname` TEXT, `port` INTEGER DEFAULT " MUMTEXT(DEFAULT_MUMBLE_PORT) ", `username` TEXT, `password` TEXT)"));
	query.exec(QLatin1String("ALTER TABLE `servers` ADD COLUMN `url` TEXT"));

	query.exec(QLatin1String("CREATE TABLE IF NOT EXISTS `comments` (`who` TEXT, `comment` BLOB, `seen` DATE)"));
	query.exec(QLatin1String("CREATE UNIQUE INDEX IF NOT EXISTS `comments_comment` ON `comments`(`who`, `comment`)"));
	query.exec(QLatin1String("CREATE INDEX IF NOT EXISTS `comments_seen` ON `comments`(`seen`)"));

	query.exec(QLatin1String("CREATE TABLE IF NOT EXISTS `blobs` (`hash` TEXT, `data` BLOB, `seen` DATE)"));
	query.exec(QLatin1String("CREATE UNIQUE INDEX IF NOT EXISTS `blobs_hash` ON `blobs`(`hash`)"));
	query.exec(QLatin1String("CREATE INDEX IF NOT EXISTS `blobs_seen` ON `blobs`(`seen`)"));

	query.exec(QLatin1String("CREATE TABLE IF NOT EXISTS `tokens` (`id` INTEGER PRIMARY KEY AUTOINCREMENT, `digest` BLOB, `token` TEXT)"));
	query.exec(QLatin1String("CREATE INDEX IF NOT EXISTS `tokens_host_port` ON `tokens`(`digest`)"));

	query.exec(QLatin1String("CREATE TABLE IF NOT EXISTS `shortcut` (`id` INTEGER PRIMARY KEY AUTOINCREMENT, `digest` BLOB, `shortcut` BLOB, `target` BLOB, `suppress` INTEGER)"));
	query.exec(QLatin1String("CREATE INDEX IF NOT EXISTS `shortcut_host_port` ON `shortcut`(`digest`)"));

	query.exec(QLatin1String("CREATE TABLE IF NOT EXISTS `udp` (`id` INTEGER PRIMARY KEY AUTOINCREMENT, `digest` BLOB)"));
	query.exec(QLatin1String("CREATE UNIQUE INDEX IF NOT EXISTS `udp_host_port` ON `udp`(`digest`)"));

	query.exec(QLatin1String("CREATE TABLE IF NOT EXISTS `cert` (`id` INTEGER PRIMARY KEY AUTOINCREMENT, `hostname` TEXT, `port` INTEGER, `digest` TEXT)"));
	query.exec(QLatin1String("CREATE UNIQUE INDEX IF NOT EXISTS `cert_host_port` ON `cert`(`hostname`,`port`)"));

	query.exec(QLatin1String("CREATE TABLE IF NOT EXISTS `friends` (`id` INTEGER PRIMARY KEY AUTOINCREMENT, `name` TEXT, `hash` TEXT)"));
	query.exec(QLatin1String("CREATE UNIQUE INDEX IF NOT EXISTS `friends_name` ON `friends`(`name`)"));
	query.exec(QLatin1String("CREATE UNIQUE INDEX IF NOT EXISTS `friends_hash` ON `friends`(`hash`)"));

	query.exec(QLatin1String("CREATE TABLE IF NOT EXISTS `muted` (`id` INTEGER PRIMARY KEY AUTOINCREMENT, `hash` TEXT)"));
	query.exec(QLatin1String("CREATE UNIQUE INDEX IF NOT EXISTS `muted_hash` ON `muted`(`hash`)"));

	query.exec(QLatin1String("CREATE TABLE IF NOT EXISTS `pingcache` (`id` INTEGER PRIMARY KEY AUTOINCREMENT, `hostname` TEXT, `port` INTEGER, `ping` INTEGER)"));
	query.exec(QLatin1String("CREATE UNIQUE INDEX IF NOT EXISTS `pingcache_host_port` ON `pingcache`(`hostname`,`port`)"));

	query.exec(QLatin1String("DELETE FROM `comments` WHERE `seen` < datetime('now', '-1 years')"));
	query.exec(QLatin1String("DELETE FROM `blobs` WHERE `seen` < datetime('now', '-1 months')"));

	query.exec(QLatin1String("VACUUM"));

	query.exec(QLatin1String("PRAGMA synchronous = OFF"));
	query.exec(QLatin1String("PRAGMA journal_mode = TRUNCATE"));

	query.exec(QLatin1String("SELECT sqlite_version()"));
	while (query.next())
		qWarning() << "Database SQLite:" << query.value(0).toString();
}