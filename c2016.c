int to_pkcs8(char* in_path, char* out_path, char* file_pin)
{
	FILE* file_pointer = NULL;
	char line[MAX_LINE], data[MAX_LINE];
	char* value_pointer = NULL;
	int lineno = 0, m, n, error = 0, found, algorithm = DNS_KEYALG_ERROR, data_length;
	uint32_t bitfield = 0;
	key_material_t pkey[TAG_MAX];

	if (in_path == NULL)
	{
		fprintf(stderr, "ERROR: A path to the input file must be supplied. Use --in <path>\n");
		return 1;
	}

	if (out_path == NULL)
	{
		fprintf(stderr, "ERROR: A path to the output file must be supplied. Use --out <path>\n");
		return 1;
	}

	file_pointer = fopen(in_path, "r");
	if (file_pointer == NULL)
	{
		fprintf(stderr, "ERROR: Could not open input file %.100s for reading.\n", in_path);
		return 1;
	}

	// Loop over all of the lines
	while (fgets(line, MAX_LINE, file_pointer) != NULL)
	{
		lineno++;

		// Find the current text field in the BIND file.
		for (m = 0, found = -1; found == -1 && file_tags[m]; m++)
		{
			if (strncasecmp(line, file_tags[m], strlen(file_tags[m])) == 0)
			{
				found = m;
			}
		}

		// The text files is not recognized.
		if (found == -1)
		{
			fprintf(stderr, "ERROR: Unrecognized input line %i\n", lineno);
			fprintf(stderr, "ERROR: --> %s", line);
			continue;
		}

		// Point to the data for this text field.
		value_pointer = line + strlen(file_tags[found]) + 1;

		// Continue if we are at the end of the string
		if (*value_pointer == 0)
		{
			continue;
		}

		// Check that we do not get duplicates.
		if (bitfield & (1 << found))
		{
			fprintf(stderr, "ERROR: Duplicate \"%s\" field, line %i - ignored\n",
					file_tags[found], lineno);
			continue;
		}
		bitfield |= (1 << found);

		// Handle the data for this text field.
		switch (found)
		{
			case TAG_VERSION:
				if (sscanf(value_pointer, "v%i.%i", &m, &n) != 2)
				{
					fprintf(stderr, "ERROR: Invalid/unknown version string "
							"(%.100s).\n", value_pointer);
					error = 1;
					break;
				}
				if (m > FILE_MAJOR_VERSION || (m == FILE_MAJOR_VERSION && n > FILE_MINOR_VERSION))
				{
					fprintf(stderr, "ERROR: Cannot parse this version of file format, "
							"v%i.%i.\n", m, n);
					error = 1;
				}
				break;
			case TAG_ALGORITHM:
				algorithm = strtol(value_pointer, NULL, 10);
				break;
			// RSA
			case TAG_MODULUS:
			case TAG_PUBEXP:
			case TAG_PRIVEXP:
			case TAG_PRIME1:
			case TAG_PRIME2:
			case TAG_EXP1:
			case TAG_EXP2:
			case TAG_COEFF:
			// DSA
			case TAG_PRIME:
			case TAG_SUBPRIME:
			case TAG_BASE:
			case TAG_PRIVVAL:
			case TAG_PUBVAL:
				data_length = b64_pton(value_pointer, (unsigned char*)data, MAX_LINE);
				if (data_length == -1)
				{
					error = 1;
					fprintf(stderr, "ERROR: Could not parse the base64 string on line %i.\n", lineno);
				}
				else
				{
					pkey[found].big = malloc(data_length);
					if (!pkey[found].big)
					{
						fprintf(stderr, "ERROR: Could not allocate memory.\n");
						error = 1;
						break;
					}
					memcpy(pkey[found].big, data, data_length);
					pkey[found].size = data_length;
				}
				break;
			// Do not need these
			case TAG_CREATED:
			case TAG_PUBLISH:
			case TAG_ACTIVATE:
			default:
				break;
		}
	}

	fclose(file_pointer);

	// Something went wrong. Clean up and quit.
	if (error)
	{
		free_key_material(pkey);
		return error;
	}

	crypto_init();

	// Save the the key to the disk
	switch (algorithm)
	{
		case DNS_KEYALG_ERROR:
			fprintf(stderr, "ERROR: The algorithm %i was not given in the file.\n",
					algorithm);
			error = 1;
			break;
		case DNS_KEYALG_RSAMD5:
		case DNS_KEYALG_RSASHA1:
		case DNS_KEYALG_RSASHA1_NSEC3_SHA1:
		case DNS_KEYALG_RSASHA256:
		case DNS_KEYALG_RSASHA512:
			error = save_rsa_pkcs8(out_path, file_pin, pkey);
			break;
		case DNS_KEYALG_DSA:
		case DNS_KEYALG_DSA_NSEC3_SHA1:
			error = save_dsa_pkcs8(out_path, file_pin, pkey);
			break;
		case DNS_KEYALG_ECC:
		case DNS_KEYALG_ECC_GOST:
		default:
			fprintf(stderr, "ERROR: The algorithm %i is not supported.\n",
					algorithm);
			error = 1;
			break;
	}

	crypto_final();
	free_key_material(pkey);

	return error;
}