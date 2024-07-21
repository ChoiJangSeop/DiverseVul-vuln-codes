	void startPreloader() {
		TRACE_POINT();
		this_thread::disable_interruption di;
		this_thread::disable_syscall_interruption dsi;
		assert(!preloaderStarted());
		checkChrootDirectories(options);
		
		shared_array<const char *> args;
		vector<string> command = createRealPreloaderCommand(options, args);
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
			execvp(command[0].c_str(), (char * const *) args.get());
			
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
			ScopeGuard guard(boost::bind(nonInterruptableKillAndWaitpid, pid));
			adminSocket.first.close();
			errorPipe.second.close();
			
			StartupDetails details;
			details.adminSocket = adminSocket.second;
			details.io = BufferedIO(adminSocket.second);
			details.stderrCapturer =
				make_shared<BackgroundIOCapturer>(
					errorPipe.first,
					config->forwardStderr ? config->forwardStderrTo : -1);
			details.stderrCapturer->start();
			details.debugDir = debugDir;
			details.options = &options;
			details.timeout = options.startTimeout * 1000;
			details.forwardStderr = config->forwardStderr;
			details.forwardStderrTo = config->forwardStderrTo;
			
			{
				this_thread::restore_interruption ri(di);
				this_thread::restore_syscall_interruption rsi(dsi);
				socketAddress = negotiatePreloaderStartup(details);
			}
			this->adminSocket = adminSocket.second;
			{
				lock_guard<boost::mutex> l(simpleFieldSyncher);
				this->pid = pid;
			}
			preloaderOutputWatcher.set(adminSocket.second, ev::READ);
			libev->start(preloaderOutputWatcher);
			setNonBlocking(errorPipe.first);
			preloaderErrorWatcher = make_shared<PipeWatcher>(libev,
				errorPipe.first,
				config->forwardStderr ? config->forwardStderrTo : -1);
			preloaderErrorWatcher->start();
			preloaderAnnotations = debugDir->readAll();
			P_DEBUG("Preloader for " << options.appRoot <<
				" started on PID " << pid <<
				", listening on " << socketAddress);
			guard.clear();
		}
	}