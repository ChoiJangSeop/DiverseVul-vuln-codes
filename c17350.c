int main(int argc, char * * argv) {
	THIS LINE WILL LEAD TO A COMPILE ERROR. THIS TOOL SHOULD NOT BE USED, SEE README.

	try {
		ArgParser argp(argc, argv);
		bool bHelp = false;
		bool bModeBenchmark = false;
		bool bModeZeros = false;
		bool bModeLetters = false;
		bool bModeNumbers = false;
		std::string strModeLeading;
		std::string strModeMatching;
		bool bModeLeadingRange = false;
		bool bModeRange = false;
		bool bModeMirror = false;
		bool bModeDoubles = false;
		int rangeMin = 0;
		int rangeMax = 0;
		std::vector<size_t> vDeviceSkipIndex;
		size_t worksizeLocal = 64;
		size_t worksizeMax = 0; // Will be automatically determined later if not overriden by user
		bool bNoCache = false;
		size_t inverseSize = 255;
		size_t inverseMultiple = 16384;
		bool bMineContract = false;

		argp.addSwitch('h', "help", bHelp);
		argp.addSwitch('0', "benchmark", bModeBenchmark);
		argp.addSwitch('1', "zeros", bModeZeros);
		argp.addSwitch('2', "letters", bModeLetters);
		argp.addSwitch('3', "numbers", bModeNumbers);
		argp.addSwitch('4', "leading", strModeLeading);
		argp.addSwitch('5', "matching", strModeMatching);
		argp.addSwitch('6', "leading-range", bModeLeadingRange);
		argp.addSwitch('7', "range", bModeRange);
		argp.addSwitch('8', "mirror", bModeMirror);
		argp.addSwitch('9', "leading-doubles", bModeDoubles);
		argp.addSwitch('m', "min", rangeMin);
		argp.addSwitch('M', "max", rangeMax);
		argp.addMultiSwitch('s', "skip", vDeviceSkipIndex);
		argp.addSwitch('w', "work", worksizeLocal);
		argp.addSwitch('W', "work-max", worksizeMax);
		argp.addSwitch('n', "no-cache", bNoCache);
		argp.addSwitch('i', "inverse-size", inverseSize);
		argp.addSwitch('I', "inverse-multiple", inverseMultiple);
		argp.addSwitch('c', "contract", bMineContract);

		if (!argp.parse()) {
			std::cout << "error: bad arguments, try again :<" << std::endl;
			return 1;
		}

		if (bHelp) {
			std::cout << g_strHelp << std::endl;
			return 0;
		}

		Mode mode = Mode::benchmark();
		if (bModeBenchmark) {
			mode = Mode::benchmark();
		} else if (bModeZeros) {
			mode = Mode::zeros();
		} else if (bModeLetters) {
			mode = Mode::letters();
		} else if (bModeNumbers) {
			mode = Mode::numbers();
		} else if (!strModeLeading.empty()) {
			mode = Mode::leading(strModeLeading.front());
		} else if (!strModeMatching.empty()) {
			mode = Mode::matching(strModeMatching);
		} else if (bModeLeadingRange) {
			mode = Mode::leadingRange(rangeMin, rangeMax);
		} else if (bModeRange) {
			mode = Mode::range(rangeMin, rangeMax);
		} else if(bModeMirror) {
			mode = Mode::mirror();
		} else if (bModeDoubles) {
			mode = Mode::doubles();
		} else {
			std::cout << g_strHelp << std::endl;
			return 0;
		}
		std::cout << "Mode: " << mode.name << std::endl;

		if (bMineContract) {
			mode.target = CONTRACT;
		} else {
			mode.target = ADDRESS;
		}
		std::cout << "Target: " << mode.transformName() << std:: endl;

		std::vector<cl_device_id> vFoundDevices = getAllDevices();
		std::vector<cl_device_id> vDevices;
		std::map<cl_device_id, size_t> mDeviceIndex;

		std::vector<std::string> vDeviceBinary;
		std::vector<size_t> vDeviceBinarySize;
		cl_int errorCode;
		bool bUsedCache = false;

		std::cout << "Devices:" << std::endl;
		for (size_t i = 0; i < vFoundDevices.size(); ++i) {
			// Ignore devices in skip index
			if (std::find(vDeviceSkipIndex.begin(), vDeviceSkipIndex.end(), i) != vDeviceSkipIndex.end()) {
				continue;
			}

			cl_device_id & deviceId = vFoundDevices[i];

			const auto strName = clGetWrapperString(clGetDeviceInfo, deviceId, CL_DEVICE_NAME);
			const auto computeUnits = clGetWrapper<cl_uint>(clGetDeviceInfo, deviceId, CL_DEVICE_MAX_COMPUTE_UNITS);
			const auto globalMemSize = clGetWrapper<cl_ulong>(clGetDeviceInfo, deviceId, CL_DEVICE_GLOBAL_MEM_SIZE);
			bool precompiled = false;

			// Check if there's a prebuilt binary for this device and load it
			if(!bNoCache) {
				std::ifstream fileIn(getDeviceCacheFilename(deviceId, inverseSize), std::ios::binary);
				if (fileIn.is_open()) {
					vDeviceBinary.push_back(std::string((std::istreambuf_iterator<char>(fileIn)), std::istreambuf_iterator<char>()));
					vDeviceBinarySize.push_back(vDeviceBinary.back().size());
					precompiled = true;
				}
			}

			std::cout << "  GPU" << i << ": " << strName << ", " << globalMemSize << " bytes available, " << computeUnits << " compute units (precompiled = " << (precompiled ? "yes" : "no") << ")" << std::endl;
			vDevices.push_back(vFoundDevices[i]);
			mDeviceIndex[vFoundDevices[i]] = i;
		}

		if (vDevices.empty()) {
			return 1;
		}

		std::cout << std::endl;
		std::cout << "Initializing OpenCL..." << std::endl;
		std::cout << "  Creating context..." << std::flush;
		auto clContext = clCreateContext( NULL, vDevices.size(), vDevices.data(), NULL, NULL, &errorCode);
		if (printResult(clContext, errorCode)) {
			return 1;
		}

		cl_program clProgram;
		if (vDeviceBinary.size() == vDevices.size()) {
			// Create program from binaries
			bUsedCache = true;

			std::cout << "  Loading kernel from binary..." << std::flush;
			const unsigned char * * pKernels = new const unsigned char *[vDevices.size()];
			for (size_t i = 0; i < vDeviceBinary.size(); ++i) {
				pKernels[i] = reinterpret_cast<const unsigned char *>(vDeviceBinary[i].data());
			}

			cl_int * pStatus = new cl_int[vDevices.size()];

			clProgram = clCreateProgramWithBinary(clContext, vDevices.size(), vDevices.data(), vDeviceBinarySize.data(), pKernels, pStatus, &errorCode);
			if(printResult(clProgram, errorCode)) {
				return 1;
			}
		} else {
			// Create a program from the kernel source
			std::cout << "  Compiling kernel..." << std::flush;
			const std::string strKeccak = readFile("keccak.cl");
			const std::string strVanity = readFile("profanity.cl");
			const char * szKernels[] = { strKeccak.c_str(), strVanity.c_str() };

			clProgram = clCreateProgramWithSource(clContext, sizeof(szKernels) / sizeof(char *), szKernels, NULL, &errorCode);
			if (printResult(clProgram, errorCode)) {
				return 1;
			}
		}

		// Build the program
		std::cout << "  Building program..." << std::flush;
		const std::string strBuildOptions = "-D PROFANITY_INVERSE_SIZE=" + toString(inverseSize) + " -D PROFANITY_MAX_SCORE=" + toString(PROFANITY_MAX_SCORE);
		if (printResult(clBuildProgram(clProgram, vDevices.size(), vDevices.data(), strBuildOptions.c_str(), NULL, NULL))) {
#ifdef PROFANITY_DEBUG
			std::cout << std::endl;
			std::cout << "build log:" << std::endl;

			size_t sizeLog;
			clGetProgramBuildInfo(clProgram, vDevices[0], CL_PROGRAM_BUILD_LOG, 0, NULL, &sizeLog);
			char * const szLog = new char[sizeLog];
			clGetProgramBuildInfo(clProgram, vDevices[0], CL_PROGRAM_BUILD_LOG, sizeLog, szLog, NULL);

			std::cout << szLog << std::endl;
			delete[] szLog;
#endif
			return 1;
		}

		// Save binary to improve future start times
		if( !bUsedCache && !bNoCache ) {
			std::cout << "  Saving program..." << std::flush;
			auto binaries = getBinaries(clProgram);
			for (size_t i = 0; i < binaries.size(); ++i) {
				std::ofstream fileOut(getDeviceCacheFilename(vDevices[i], inverseSize), std::ios::binary);
				fileOut.write(binaries[i].data(), binaries[i].size());
			}
			std::cout << "OK" << std::endl;
		}

		std::cout << std::endl;

		Dispatcher d(clContext, clProgram, mode, worksizeMax == 0 ? inverseSize * inverseMultiple : worksizeMax, inverseSize, inverseMultiple, 0);
		for (auto & i : vDevices) {
			d.addDevice(i, worksizeLocal, mDeviceIndex[i]);
		}

		d.run();
		clReleaseContext(clContext);
		return 0;
	} catch (std::runtime_error & e) {
		std::cout << "std::runtime_error - " << e.what() << std::endl;
	} catch (...) {
		std::cout << "unknown exception occured" << std::endl;
	}

	return 1;
}