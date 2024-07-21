int main(int argc, char **argv)
{
	int c;

	init();

	/* Determine the base name of this command. */
	if ((cmdname = strrchr(argv[0], '/'))) {
		++cmdname;
	} else {
		cmdname = argv[0];
	}

	/* Initialize the JasPer library. */
	if (jas_init()) {
		abort();
	}

	glutInit(&argc, argv);
	glutInitDisplayMode(GLUT_RGBA | GLUT_DOUBLE);
	glutCreateWindow(cmdname);
	glutReshapeFunc(reshapefunc);
	glutDisplayFunc(displayfunc);
	glutSpecialFunc(specialfunc);
	glutKeyboardFunc(keyboardfunc);

	cmdopts.numfiles = 0;
	cmdopts.filenames = 0;
	cmdopts.title = 0;
	cmdopts.tmout = 0;
	cmdopts.loop = 0;
	cmdopts.verbose = 0;

	while ((c = jas_getopt(argc, argv, opts)) != EOF) {
		switch (c) {
		case 'w':
			cmdopts.tmout = atof(jas_optarg) * 1000;
			break;
		case 'l':
			cmdopts.loop = 1;
			break;
		case 't':
			cmdopts.title = jas_optarg;
			break;
		case 'v':
			cmdopts.verbose = 1;
			break;
		case 'V':
			printf("%s\n", JAS_VERSION);
			fprintf(stderr, "libjasper %s\n", jas_getversion());
			cleanupandexit(EXIT_SUCCESS);
			break;
		default:
		case 'h':
			usage();
			break;
		}
	}

	if (jas_optind < argc) {
		/* The images are to be read from one or more explicitly named
		  files. */
		cmdopts.numfiles = argc - jas_optind;
		cmdopts.filenames = &argv[jas_optind];
	} else {
		/* The images are to be read from standard input. */
		static char *null = 0;
		cmdopts.filenames = &null;
		cmdopts.numfiles = 1;
	}

	streamin = jas_stream_fdopen(0, "rb");

	/* Load the next image. */
	nextimage();

	/* Start the GLUT main event handler loop. */
	glutMainLoop();

	return EXIT_SUCCESS;
}