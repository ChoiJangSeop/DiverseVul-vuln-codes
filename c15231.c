int main(int argc, char* argv[])
{
    if (argc != 7)
    {
	std::cerr << "Usage: predictor {png|tiff} {en,de}code filename"
                  << " columns samples-per-pixel bits-per-sample"
                  << std::endl;
	exit(2);
    }
    char* filter = argv[1];
    bool encode = (strcmp(argv[2], "encode") == 0);
    char* filename = argv[3];
    int columns = QUtil::string_to_int(argv[4]);
    int samples_per_pixel = QUtil::string_to_int(argv[5]);
    int bits_per_sample = QUtil::string_to_int(argv[6]);

    try
    {
	run(filename, filter, encode,
            columns, bits_per_sample, samples_per_pixel);
    }
    catch (std::exception& e)
    {
	std::cout << e.what() << std::endl;
    }
    return 0;
}