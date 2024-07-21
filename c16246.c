	void stopPreloader() {
		TRACE_POINT();
		this_thread::disable_interruption di;
		this_thread::disable_syscall_interruption dsi;
		
		if (!preloaderStarted()) {
			return;
		}
		adminSocket.close();
		if (timedWaitpid(pid, NULL, 5000) == 0) {
			P_TRACE(2, "Spawn server did not exit in time, killing it...");
			syscalls::kill(pid, SIGKILL);
			syscalls::waitpid(pid, NULL, 0);
		}
		libev->stop(preloaderOutputWatcher);
		// Detach the error pipe; it will truly be closed after the error
		// pipe has reached EOF.
		preloaderErrorWatcher.reset();
		// Delete socket after the process has exited so that it
		// doesn't crash upon deleting a nonexistant file.
		// TODO: in Passenger 4 we must check whether the file really was
		// owned by the preloader, otherwise this is a potential security flaw.
		if (getSocketAddressType(socketAddress) == SAT_UNIX) {
			string filename = parseUnixSocketAddress(socketAddress);
			syscalls::unlink(filename.c_str());
		}
		{
			lock_guard<boost::mutex> l(simpleFieldSyncher);
			pid = -1;
		}
		socketAddress.clear();
	}