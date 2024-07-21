static bool is_legal_file(const std::string &filename)
{
	DBG_FS << "Looking for '" << filename << "'.\n";

	if (filename.empty()) {
		LOG_FS << "  invalid filename\n";
		return false;
	}

	if (filename.find("..") != std::string::npos) {
		ERR_FS << "Illegal path '" << filename << "' (\"..\" not allowed).\n";
		return false;
	}

	return true;
}