void ModuleSQL::init()
{
	Dispatcher = new DispatcherThread(this);
	ServerInstance->Threads.Start(Dispatcher);
}