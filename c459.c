void Server::msgQueryUsers(ServerUser *uSource, MumbleProto::QueryUsers &msg) {
	MSG_SETUP(ServerUser::Authenticated);

	MumbleProto::QueryUsers reply;

	for (int i=0;i<msg.ids_size();++i) {
		int id = msg.ids(i);
		if (id >= 0) {
			const QString &name = getUserName(id);
			if (! name.isEmpty()) {
				reply.add_ids(id);
				reply.add_names(u8(name));
			}
		}
	}

	for (int i=0;i<msg.names_size();++i) {
		QString name = u8(msg.names(i));
		int id = getUserID(name);
		if (id >= 0) {
			name = getUserName(id);
			reply.add_ids(id);
			reply.add_names(u8(name));
		}
	}

	sendMessage(uSource, reply);
}