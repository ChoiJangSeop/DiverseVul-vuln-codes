	TEST_METHOD(5) {
		// The destructor doesnn't remove the server instance directory if it
		// wasn't created with the ownership flag or if it's been detached.
		string path, path2;
		{
			ServerInstanceDir dir(parentDir + "/passenger-test.1234", false);
			ServerInstanceDir dir2(parentDir + "/passenger-test.5678", false);
			dir2.detach();
			path = dir.getPath();
			path2 = dir2.getPath();
		}
		ensure_equals(getFileType(path), FT_DIRECTORY);
		ensure_equals(getFileType(path2), FT_DIRECTORY);
	}