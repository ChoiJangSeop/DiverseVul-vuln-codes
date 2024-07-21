ModuleLoader::Module* ModuleLoader::loadModule(const Firebird::PathName& modPath)
{
	void* module = dlopen(modPath.nullStr(), FB_RTLD_MODE);
	if (module == NULL)
	{
#ifdef DEV_BUILD
//		gds__log("loadModule failed loading %s: %s", modPath.c_str(), dlerror());
#endif
		return 0;
	}

#ifdef DEBUG_THREAD_IN_UNLOADED_LIBRARY
	Firebird::string command;
	command.printf("echo +++ %s +++ >>/tmp/fbmaps;date >> /tmp/fbmaps;cat /proc/%d/maps >>/tmp/fbmaps",
		modPath.c_str(), getpid());
	system(command.c_str());
#endif

	return FB_NEW_POOL(*getDefaultMemoryPool()) DlfcnModule(module);
}