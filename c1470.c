	FileDescriptor connectToHelperAgent() {
		TRACE_POINT();
		FileDescriptor conn;
		
		try {
			conn = connectToUnixServer(agentsStarter.getRequestSocketFilename());
			writeExact(conn, agentsStarter.getRequestSocketPassword());
		} catch (const SystemException &e) {
			if (e.code() == EPIPE || e.code() == ECONNREFUSED || e.code() == ENOENT) {
				UPDATE_TRACE_POINT();
				bool connected = false;
				
				// Maybe the helper agent crashed. First wait 50 ms.
				usleep(50000);
				
				// Then try to reconnect to the helper agent for the
				// next 5 seconds.
				time_t deadline = time(NULL) + 5;
				while (!connected && time(NULL) < deadline) {
					try {
						conn = connectToUnixServer(agentsStarter.getRequestSocketFilename());
						writeExact(conn, agentsStarter.getRequestSocketPassword());
						connected = true;
					} catch (const SystemException &e) {
						if (e.code() == EPIPE || e.code() == ECONNREFUSED || e.code() == ENOENT) {
							// Looks like the helper agent hasn't been
							// restarted yet. Wait between 20 and 100 ms.
							usleep(20000 + rand() % 80000);
							// Don't care about thread-safety of rand()
						} else {
							throw;
						}
					}
				}
				
				if (!connected) {
					UPDATE_TRACE_POINT();
					throw IOException("Cannot connect to the helper agent");
				}
			} else {
				throw;
			}
		}
		return conn;
	}