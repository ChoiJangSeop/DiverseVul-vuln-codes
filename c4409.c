int main(int argc, char **argv)
{
	char *origpath;
	char *reconpath;
	int verbose;
	char *metricname;
	int metric;

	int id;
	jas_image_t *origimage;
	jas_image_t *reconimage;
	jas_matrix_t *origdata;
	jas_matrix_t *recondata;
	jas_image_t *diffimage;
	jas_stream_t *diffstream;
	int width;
	int height;
	int depth;
	int numcomps;
	double d;
	double maxdist;
	double mindist;
	int compno;
	jas_stream_t *origstream;
	jas_stream_t *reconstream;
	char *diffpath;
	int maxonly;
	int minonly;
	int fmtid;

	verbose = 0;
	origpath = 0;
	reconpath = 0;
	metricname = 0;
	metric = metricid_none;
	diffpath = 0;
	maxonly = 0;
	minonly = 0;

	if (jas_init()) {
		abort();
	}

	cmdname = argv[0];

	/* Parse the command line options. */
	while ((id = jas_getopt(argc, argv, opts)) >= 0) {
		switch (id) {
		case OPT_MAXONLY:
			maxonly = 1;
			break;
		case OPT_MINONLY:
			minonly = 1;
			break;
		case OPT_METRIC:
			metricname = jas_optarg;
			break;
		case OPT_ORIG:
			origpath = jas_optarg;
			break;
		case OPT_RECON:
			reconpath = jas_optarg;
			break;
		case OPT_VERBOSE:
			verbose = 1;
			break;
		case OPT_DIFFIMAGE:
			diffpath = jas_optarg;
			break;
		case OPT_VERSION:
			printf("%s\n", JAS_VERSION);
			exit(EXIT_SUCCESS);
			break;
		case OPT_HELP:
		default:
			usage();
			break;
		}
	}

	if (verbose) {
		cmdinfo();
	}

	/* Ensure that files are given for both the original and reconstructed
	  images. */
	if (!origpath || !reconpath) {
		usage();
	}

	/* If a metric was specified, process it. */
	if (metricname) {
		if ((metric = (jas_taginfo_nonull(jas_taginfos_lookup(metrictab,
		  metricname))->id)) < 0) {
			usage();
		}
	}

	/* Open the original image file. */
	if (!(origstream = jas_stream_fopen(origpath, "rb"))) {
		fprintf(stderr, "cannot open %s\n", origpath);
		return EXIT_FAILURE;
	}

	/* Open the reconstructed image file. */
	if (!(reconstream = jas_stream_fopen(reconpath, "rb"))) {
		fprintf(stderr, "cannot open %s\n", reconpath);
		return EXIT_FAILURE;
	}

	/* Decode the original image. */
	if (!(origimage = jas_image_decode(origstream, -1, 0))) {
		fprintf(stderr, "cannot load original image\n");
		return EXIT_FAILURE;
	}

	/* Decoder the reconstructed image. */
	if (!(reconimage = jas_image_decode(reconstream, -1, 0))) {
		fprintf(stderr, "cannot load reconstructed image\n");
		return EXIT_FAILURE;
	}

	/* Close the original image file. */
	jas_stream_close(origstream);

	/* Close the reconstructed image file. */
	jas_stream_close(reconstream);

	/* Ensure that both images have the same number of components. */
	numcomps = jas_image_numcmpts(origimage);
	if (jas_image_numcmpts(reconimage) != numcomps) {
		fprintf(stderr, "number of components differ\n");
		return EXIT_FAILURE;
	}

	/* Compute the difference for each component. */
	maxdist = 0;
	mindist = FLT_MAX;
	for (compno = 0; compno < numcomps; ++compno) {
		width = jas_image_cmptwidth(origimage, compno);
		height = jas_image_cmptheight(origimage, compno);
		depth = jas_image_cmptprec(origimage, compno);
		if (jas_image_cmptwidth(reconimage, compno) != width ||
		 jas_image_cmptheight(reconimage, compno) != height) {
			fprintf(stderr, "image dimensions differ\n");
			return EXIT_FAILURE;
		}
		if (jas_image_cmptprec(reconimage, compno) != depth) {
			fprintf(stderr, "precisions differ\n");
			return EXIT_FAILURE;
		}

		if (!(origdata = jas_matrix_create(height, width))) {
			fprintf(stderr, "internal error\n");
			return EXIT_FAILURE;
		}
		if (!(recondata = jas_matrix_create(height, width))) {
			fprintf(stderr, "internal error\n");
			return EXIT_FAILURE;
		}
		if (jas_image_readcmpt(origimage, compno, 0, 0, width, height,
		  origdata)) {
			fprintf(stderr, "cannot read component data\n");
			return EXIT_FAILURE;
		}
		if (jas_image_readcmpt(reconimage, compno, 0, 0, width, height,
		  recondata)) {
			fprintf(stderr, "cannot read component data\n");
			return EXIT_FAILURE;
		}

		if (diffpath) {
			if (!(diffstream = jas_stream_fopen(diffpath, "rwb"))) {
				fprintf(stderr, "cannot open diff stream\n");
				return EXIT_FAILURE;
			}
			if (!(diffimage = makediffimage(origdata, recondata))) {
				fprintf(stderr, "cannot make diff image\n");
				return EXIT_FAILURE;
			}
			fmtid = jas_image_strtofmt("pnm");
			if (jas_image_encode(diffimage, diffstream, fmtid, 0)) {
				fprintf(stderr, "cannot save\n");
				return EXIT_FAILURE;
			}
			jas_stream_close(diffstream);
			jas_image_destroy(diffimage);
		}

		if (metric != metricid_none) {
			d = getdistortion(origdata, recondata, depth, metric);
			if (d > maxdist) {
				maxdist = d;
			}
			if (d < mindist) {
				mindist = d;
			}
			if (!maxonly && !minonly) {
				if (metric == metricid_pae || metric == metricid_equal) {
					printf("%ld\n", (long) ceil(d));
				} else {
					printf("%f\n", d);
				}
			}
		}
		jas_matrix_destroy(origdata);
		jas_matrix_destroy(recondata);
	}

	if (metric != metricid_none && (maxonly || minonly)) {
		if (maxonly) {
			d = maxdist;
		} else if (minonly) {
			d = mindist;
		} else {
			abort();
		}
		
		if (metric == metricid_pae || metric == metricid_equal) {
			printf("%ld\n", (long) ceil(d));
		} else {
			printf("%f\n", d);
		}
	}

	jas_image_destroy(origimage);
	jas_image_destroy(reconimage);
	jas_image_clearfmts();

	return EXIT_SUCCESS;
}