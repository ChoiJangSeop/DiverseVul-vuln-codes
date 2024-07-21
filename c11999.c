void* DlfcnModule::findSymbol(const Firebird::string& symName)
{
	void* result = dlsym(module, symName.c_str());
	if (!result)
	{
		Firebird::string newSym = '_' + symName;

		result = dlsym(module, newSym.c_str());
	}
	return result;
}