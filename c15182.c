int main(int argc, char* argv[])
{
    if (argc != 6)
    {
        usage();
    }

    char* infilename = argv[1];
    char* outfilename = argv[2];
    int width = QUtil::string_to_int(argv[3]);
    int height = QUtil::string_to_int(argv[4]);
    char* colorspace = argv[5];
    J_COLOR_SPACE cs =
        ((strcmp(colorspace, "rgb") == 0) ? JCS_RGB :
         (strcmp(colorspace, "cmyk") == 0) ? JCS_CMYK :
         (strcmp(colorspace, "gray") == 0) ? JCS_GRAYSCALE :
         JCS_UNKNOWN);
    int components = 0;
    switch (cs)
    {
      case JCS_RGB:
        components = 3;
        break;
      case JCS_CMYK:
        components = 4;
        break;
      case JCS_GRAYSCALE:
        components = 1;
        break;
      default:
        usage();
        break;
    }

    FILE* infile = QUtil::safe_fopen(infilename, "rb");
    FILE* outfile = QUtil::safe_fopen(outfilename, "wb");
    Pl_StdioFile out("stdout", outfile);
    unsigned char buf[100];
    bool done = false;
    Callback callback;
    Pl_DCT dct("dct", &out, width, height, components, cs, &callback);
    while (! done)
    {
	size_t len = fread(buf, 1, sizeof(buf), infile);
	if (len <= 0)
	{
	    done = true;
	}
	else
	{
	    dct.write(buf, len);
	}
    }
    dct.finish();
    if (! callback.called)
    {
        std::cout << "Callback was not called" << std::endl;
    }
    fclose(infile);
    fclose(outfile);
    return 0;
}