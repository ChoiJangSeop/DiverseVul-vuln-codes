usage(const char * cmd, int exit_status, const char * devtype)
{
	FILE *stream;

	stream = exit_status ? stderr : stdout;

	/* non-NULL devtype indicates help for specific device, so no usage */
	if (devtype == NULL) {
		fprintf(stream, "usage:\n");
		fprintf(stream, "\t %s [-svh] "
		"-L\n"
		, cmd);

		fprintf(stream, "\t %s [-svh] "
		"-t stonith-device-type "
		"-n\n"
		, cmd);

		fprintf(stream, "\t %s [-svh] "
		"-t stonith-device-type "
		"-m\n"
		, cmd);

		fprintf(stream, "\t %s [-svh] "
		"-t stonith-device-type "
		"{-p stonith-device-parameters | "
		"-F stonith-device-parameters-file | "
		"name=value...} "
		"[-c count] "
		"-lS\n"
		, cmd);

		fprintf(stream, "\t %s [-svh] "
		"-t stonith-device-type "
		"{-p stonith-device-parameters | "
		"-F stonith-device-parameters-file | "
		"name=value...} "
		"[-c count] "
		"-T {reset|on|off} nodename\n"
		, cmd);

		fprintf(stream, "\nwhere:\n");
		fprintf(stream, "\t-L\tlist supported stonith device types\n");
		fprintf(stream, "\t-l\tlist hosts controlled by this stonith device\n");
		fprintf(stream, "\t-S\treport stonith device status\n");
		fprintf(stream, "\t-s\tsilent\n");
		fprintf(stream, "\t-v\tverbose\n");
		fprintf(stream, "\t-n\toutput the config names of stonith-device-parameters\n");
		fprintf(stream, "\t-m\tdisplay meta-data of the stonith device type\n");
		fprintf(stream, "\t-h\tdisplay detailed help message with stonith device description(s)\n");
	}

	if (exit_status == 0) {
		confhelp(cmd, stream, devtype);
	}

	exit(exit_status);
}