int main(int argc, char* argv[])
{
    QUtil::setLineBuf(stdout);
    if ((whoami = strrchr(argv[0], '/')) == NULL)
    {
	whoami = argv[0];
    }
    else
    {
	++whoami;
    }
    // For libtool's sake....
    if (strncmp(whoami, "lt-", 3) == 0)
    {
	whoami += 3;
    }

    if ((argc < 3) || (argc > 4))
    {
	usage();
    }

    try
    {
	int n = atoi(argv[1]);
	char const* filename1 = argv[2];
        char const* arg2 = argv[3];
	runtest(n, filename1, arg2);
    }
    catch (std::exception& e)
    {
	std::cerr << e.what() << std::endl;
	exit(2);
    }

    return 0;
}