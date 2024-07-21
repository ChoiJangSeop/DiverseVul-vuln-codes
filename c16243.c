	virtual ProcessPtr spawn(const Options &options) {
		TRACE_POINT();
		this_thread::disable_interruption di;
		this_thread::disable_syscall_interruption dsi;
		P_DEBUG("Spawning new process: appRoot=" << options.appRoot);
		possiblyRaiseInternalError(options);

		shared_array<const char *> args;
		vector<string> command = createCommand(options, args);
		SpawnPreparationInfo preparation = prepareSpawn(options);
		SocketPair adminSocket = createUnixSocketPair();
		Pipe errorPipe = createPipe();
		DebugDirPtr debugDir = make_shared<DebugDir>(preparation.uid, preparation.gid);
		pid_t pid;
		
		pid = syscalls::fork();
		if (pid == 0) {
			setenv("PASSENGER_DEBUG_DIR", debugDir->getPath().c_str(), 1);
			purgeStdio(stdout);
			purgeStdio(stderr);
			resetSignalHandlersAndMask();
			disableMallocDebugging();
			int adminSocketCopy = dup2(adminSocket.first, 3);
			int errorPipeCopy = dup2(errorPipe.second, 4);
			dup2(adminSocketCopy, 0);
			dup2(adminSocketCopy, 1);
			dup2(errorPipeCopy, 2);
			closeAllFileDescriptors(2);
			setChroot(preparation);
			switchUser(preparation);
			setWorkingDirectory(preparation);
			execvp(args[0], (char * const *) args.get());
			
			int e = errno;
			printf("!> Error\n");
			printf("!> \n");
			printf("Cannot execute \"%s\": %s (errno=%d)\n", command[0].c_str(),
				strerror(e), e);
			fprintf(stderr, "Cannot execute \"%s\": %s (errno=%d)\n",
				command[0].c_str(), strerror(e), e);
			fflush(stdout);
			fflush(stderr);
			_exit(1);
			
		} else if (pid == -1) {
			int e = errno;
			throw SystemException("Cannot fork a new process", e);
			
		} else {
			UPDATE_TRACE_POINT();
			ScopeGuard guard(boost::bind(nonInterruptableKillAndWaitpid, pid));
			adminSocket.first.close();
			errorPipe.second.close();
			
			NegotiationDetails details;
			details.libev = libev;
			details.stderrCapturer =
				make_shared<BackgroundIOCapturer>(
					errorPipe.first,
					config->forwardStderr ? config->forwardStderrTo : -1);
			details.stderrCapturer->start();
			details.pid = pid;
			details.adminSocket = adminSocket.second;
			details.io = BufferedIO(adminSocket.second);
			details.errorPipe = errorPipe.first;
			details.options = &options;
			details.forwardStderr = config->forwardStderr;
			details.forwardStderrTo = config->forwardStderrTo;
			details.debugDir = debugDir;
			
			ProcessPtr process;
			{
				this_thread::restore_interruption ri(di);
				this_thread::restore_syscall_interruption rsi(dsi);
				process = negotiateSpawn(details);
			}
			detachProcess(process->pid);
			guard.clear();
			P_DEBUG("Process spawning done: appRoot=" << options.appRoot <<
				", pid=" << process->pid);
			return process;
		}
	}