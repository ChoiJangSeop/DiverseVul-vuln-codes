runMainLoop(WorkingObjects &wo) {
	ev::io feedbackFdWatcher(eventLoop);
	ev::sig sigintWatcher(eventLoop);
	ev::sig sigtermWatcher(eventLoop);
	ev::sig sigquitWatcher(eventLoop);
	
	if (feedbackFdAvailable()) {
		feedbackFdWatcher.set<&feedbackFdBecameReadable>();
		feedbackFdWatcher.start(FEEDBACK_FD, ev::READ);
		writeArrayMessage(FEEDBACK_FD, "initialized", NULL);
	}
	sigintWatcher.set<&caughtExitSignal>();
	sigintWatcher.start(SIGINT);
	sigtermWatcher.set<&caughtExitSignal>();
	sigtermWatcher.start(SIGTERM);
	sigquitWatcher.set<&printInfo>();
	sigquitWatcher.start(SIGQUIT);
	
	P_WARN("PassengerLoggingAgent online, listening at " << socketAddress);
	ev_run(eventLoop, 0);
}