int main(int argc, char* argv[])
{
    if (argc != 4)
    {
	std::cerr << "Usage: pipeline {en,de}code filename columns" << std::endl;
	exit(2);
    }
    bool encode = (strcmp(argv[1], "encode") == 0);
    char* filename = argv[2];
    int columns = atoi(argv[3]);

    try
    {
	run(filename, encode, columns);
    }
    catch (std::exception& e)
    {
	std::cout << e.what() << std::endl;
    }
    return 0;
}