	virtual ProcessPtr spawn(const Options &options) {
		TRACE_POINT();
		assert(options.appType == this->options.appType);
		assert(options.appRoot == this->options.appRoot);
		
		P_DEBUG("Spawning new process: appRoot=" << options.appRoot);
		possiblyRaiseInternalError(options);

		{
			lock_guard<boost::mutex> l(simpleFieldSyncher);
			m_lastUsed = SystemTime::getUsec();
		}
		if (!preloaderStarted()) {
			UPDATE_TRACE_POINT();
			startPreloader();
		}
		
		UPDATE_TRACE_POINT();
		SpawnResult result;
		try {
			result = sendSpawnCommand(options);
		} catch (const SystemException &e) {
			result = sendSpawnCommandAgain(e, options);
		} catch (const IOException &e) {
			result = sendSpawnCommandAgain(e, options);
		} catch (const SpawnException &e) {
			result = sendSpawnCommandAgain(e, options);
		}
		
		UPDATE_TRACE_POINT();
		NegotiationDetails details;
		details.libev = libev;
		details.pid = result.pid;
		details.adminSocket = result.adminSocket;
		details.io = result.io;
		details.options = &options;
		details.forwardStderr = config->forwardStderr;
		details.forwardStderrTo = config->forwardStderrTo;
		ProcessPtr process = negotiateSpawn(details);
		P_DEBUG("Process spawning done: appRoot=" << options.appRoot <<
			", pid=" << process->pid);
		return process;
	}