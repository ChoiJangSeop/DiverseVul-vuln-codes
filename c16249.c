	ProcessPtr negotiateSpawn(NegotiationDetails &details) {
		TRACE_POINT();
		details.spawnStartTime = SystemTime::getUsec();
		details.gupid = integerToHex(SystemTime::get() / 60) + "-" +
			randomGenerator->generateAsciiString(11);
		details.connectPassword = randomGenerator->generateAsciiString(43);
		details.timeout = details.options->startTimeout * 1000;
		
		string result;
		try {
			result = readMessageLine(details);
		} catch (const SystemException &e) {
			throwAppSpawnException("An error occurred while starting the "
				"web application. There was an I/O error while reading its "
				"handshake message: " + e.sys(),
				SpawnException::APP_STARTUP_PROTOCOL_ERROR,
				details);
		} catch (const TimeoutException &) {
			throwAppSpawnException("An error occurred while starting the "
				"web application: it did not write a handshake message in time.",
				SpawnException::APP_STARTUP_TIMEOUT,
				details);
		}
		
		if (result == "I have control 1.0\n") {
			UPDATE_TRACE_POINT();
			sendSpawnRequest(details);
			try {
				result = readMessageLine(details);
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
			if (result == "Ready\n") {
				return handleSpawnResponse(details);
			} else if (result == "Error\n") {
				handleSpawnErrorResponse(details);
			} else {
				handleInvalidSpawnResponseType(result, details);
			}
		} else {
			UPDATE_TRACE_POINT();
			if (result == "Error\n") {
				handleSpawnErrorResponse(details);
			} else {
				handleInvalidSpawnResponseType(result, details);
			}
		}
		return ProcessPtr(); // Never reached.
	}