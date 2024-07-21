	ProcessPtr handleSpawnResponse(NegotiationDetails &details) {
		TRACE_POINT();
		SocketListPtr sockets = make_shared<SocketList>();
		while (true) {
			string line;
			
			try {
				line = readMessageLine(details);
			} catch (const SystemException &e) {
				throwAppSpawnException("An error occurred while starting the "
					"web application. There was an I/O error while reading its "
					"startup response: " + e.sys(),
					SpawnException::APP_STARTUP_PROTOCOL_ERROR,
					details);
			} catch (const TimeoutException &) {
				throwAppSpawnException("An error occurred while starting the "
					"web application: it did not write a startup response in time.",
					SpawnException::APP_STARTUP_TIMEOUT,
					details);
			}
			
			if (line.empty()) {
				throwAppSpawnException("An error occurred while starting the "
					"web application. It unexpected closed the connection while "
					"sending its startup response.",
					SpawnException::APP_STARTUP_PROTOCOL_ERROR,
					details);
			} else if (line[line.size() - 1] != '\n') {
				throwAppSpawnException("An error occurred while starting the "
					"web application. It sent a line without a newline character "
					"in its startup response.",
					SpawnException::APP_STARTUP_PROTOCOL_ERROR,
					details);
			} else if (line == "\n") {
				break;
			}
			
			string::size_type pos = line.find(": ");
			if (pos == string::npos) {
				throwAppSpawnException("An error occurred while starting the "
					"web application. It sent a startup response line without "
					"separator.",
					SpawnException::APP_STARTUP_PROTOCOL_ERROR,
					details);
			}
			
			string key = line.substr(0, pos);
			string value = line.substr(pos + 2, line.size() - pos - 3);
			if (key == "socket") {
				// socket: <name>;<address>;<protocol>;<concurrency>
				// TODO: in case of TCP sockets, check whether it points to localhost
				// TODO: in case of unix sockets, check whether filename is absolute
				// and whether owner is correct
				vector<string> args;
				split(value, ';', args);
				if (args.size() == 4) {
					sockets->add(args[0],
						fixupSocketAddress(*details.options, args[1]),
						args[2],
						atoi(args[3]));
				} else {
					throwAppSpawnException("An error occurred while starting the "
						"web application. It reported a wrongly formatted 'socket'"
						"response value: '" + value + "'",
						SpawnException::APP_STARTUP_PROTOCOL_ERROR,
						details);
				}
			} else {
				throwAppSpawnException("An error occurred while starting the "
					"web application. It sent an unknown startup response line "
					"called '" + key + "'.",
					SpawnException::APP_STARTUP_PROTOCOL_ERROR,
					details);
			}
		}

		if (sockets->hasSessionSockets() == 0) {
			throwAppSpawnException("An error occured while starting the web "
				"application. It did not advertise any session sockets.",
				SpawnException::APP_STARTUP_PROTOCOL_ERROR,
				details);
		}
		
		return make_shared<Process>(details.libev, details.pid,
			details.gupid, details.connectPassword,
			details.adminSocket, details.errorPipe,
			sockets, creationTime, details.spawnStartTime,
			config);
	}