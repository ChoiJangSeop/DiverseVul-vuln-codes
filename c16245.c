	string handleStartupResponse(StartupDetails &details) {
		TRACE_POINT();
		string socketAddress;
		
		while (true) {
			string line;
			
			try {
				line = readMessageLine(details);
			} catch (const SystemException &e) {
				throwPreloaderSpawnException("An error occurred while starting up "
					"the preloader. There was an I/O error while reading its "
					"startup response: " + e.sys(),
					SpawnException::PRELOADER_STARTUP_PROTOCOL_ERROR,
					details);
			} catch (const TimeoutException &) {
				throwPreloaderSpawnException("An error occurred while starting up "
					"the preloader: it did not write a startup response in time.",
					SpawnException::PRELOADER_STARTUP_TIMEOUT,
					details);
			}
			
			if (line.empty()) {
				throwPreloaderSpawnException("An error occurred while starting up "
					"the preloader. It unexpected closed the connection while "
					"sending its startup response.",
					SpawnException::PRELOADER_STARTUP_PROTOCOL_ERROR,
					details);
			} else if (line[line.size() - 1] != '\n') {
				throwPreloaderSpawnException("An error occurred while starting up "
					"the preloader. It sent a line without a newline character "
					"in its startup response.",
					SpawnException::PRELOADER_STARTUP_PROTOCOL_ERROR,
					details);
			} else if (line == "\n") {
				break;
			}
			
			string::size_type pos = line.find(": ");
			if (pos == string::npos) {
				throwPreloaderSpawnException("An error occurred while starting up "
					"the preloader. It sent a startup response line without "
					"separator.",
					SpawnException::PRELOADER_STARTUP_PROTOCOL_ERROR,
					details);
			}
			
			string key = line.substr(0, pos);
			string value = line.substr(pos + 2, line.size() - pos - 3);
			if (key == "socket") {
				socketAddress = fixupSocketAddress(options, value);
			} else {
				throwPreloaderSpawnException("An error occurred while starting up "
					"the preloader. It sent an unknown startup response line "
					"called '" + key + "'.",
					SpawnException::PRELOADER_STARTUP_PROTOCOL_ERROR,
					details);
			}
		}
		
		if (socketAddress.empty()) {
			throwPreloaderSpawnException("An error occurred while starting up "
				"the preloader. It did not report a socket address in its "
				"startup response.",
				SpawnException::PRELOADER_STARTUP_PROTOCOL_ERROR,
				details);
		}
		
		return socketAddress;
	}