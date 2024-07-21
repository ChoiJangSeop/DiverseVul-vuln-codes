int Server::authenticate(QString &name, const QString &pw, const QStringList &emails, const QString &certhash, bool bStrongCert, const QList<QSslCertificate> &certs) {
	int res = -2;

	emit authenticateSig(res, name, certs, certhash, bStrongCert, pw);

	if (res != -2) {
		// External authentication handled it. Ignore certificate completely.
		if (res != -1) {
			TransactionHolder th;
			QSqlQuery &query = *th.qsqQuery;

			int lchan=readLastChannel(res);
			if (lchan < 0)
				lchan = 0;

			SQLPREP("REPLACE INTO `%1users` (`server_id`, `user_id`, `name`, `lastchannel`) VALUES (?,?,?,?)");
			query.addBindValue(iServerNum);
			query.addBindValue(res);
			query.addBindValue(name);
			query.addBindValue(lchan);
			SQLEXEC();
		}
		if (res >= 0) {
			qhUserNameCache.remove(res);
			qhUserIDCache.remove(name);
		}
		return res;
	}

	TransactionHolder th;
	QSqlQuery &query = *th.qsqQuery;

	SQLPREP("SELECT `user_id`,`name`,`pw` FROM `%1users` WHERE `server_id` = ? AND `name` like ?");
	query.addBindValue(iServerNum);
	query.addBindValue(name);
	SQLEXEC();
	if (query.next()) {
		res = -1;
		QString storedpw = query.value(2).toString();
		QString hashedpw = QString::fromLatin1(sha1(pw).toHex());

		if (! storedpw.isEmpty() && (storedpw == hashedpw)) {
			name = query.value(1).toString();
			res = query.value(0).toInt();
		} else if (query.value(0).toInt() == 0) {
			return -1;
		}
	}

	// No password match. Try cert or email match, but only for non-SuperUser.
	if (!certhash.isEmpty() && (res < 0)) {
		SQLPREP("SELECT `user_id` FROM `%1user_info` WHERE `server_id` = ? AND `key` = ? AND `value` = ?");
		query.addBindValue(iServerNum);
		query.addBindValue(ServerDB::User_Hash);
		query.addBindValue(certhash);
		SQLEXEC();
		if (query.next()) {
			res = query.value(0).toInt();
		} else if (bStrongCert) {
			foreach(const QString &email, emails) {
				if (! email.isEmpty()) {
					query.addBindValue(iServerNum);
					query.addBindValue(ServerDB::User_Email);
					query.addBindValue(email);
					SQLEXEC();
					if (query.next()) {
						res = query.value(0).toInt();
						break;
					}
				}
			}
		}
		if (res > 0) {
			SQLPREP("SELECT `name` FROM `%1users` WHERE `server_id` = ? AND `user_id` = ?");
			query.addBindValue(iServerNum);
			query.addBindValue(res);
			SQLEXEC();
			if (! query.next()) {
				res = -1;
			} else {
				name = query.value(0).toString();
			}
		}
	}
	if (! certhash.isEmpty() && (res > 0)) {
		SQLPREP("REPLACE INTO `%1user_info` (`server_id`, `user_id`, `key`, `value`) VALUES (?, ?, ?, ?)");
		query.addBindValue(iServerNum);
		query.addBindValue(res);
		query.addBindValue(ServerDB::User_Hash);
		query.addBindValue(certhash);
		SQLEXEC();
		if (! emails.isEmpty()) {
			query.addBindValue(iServerNum);
			query.addBindValue(res);
			query.addBindValue(ServerDB::User_Email);
			query.addBindValue(emails.at(0));
			SQLEXEC();
		}
	}
	if (res >= 0) {
		qhUserNameCache.remove(res);
		qhUserIDCache.remove(name);
	}
	return res;
}