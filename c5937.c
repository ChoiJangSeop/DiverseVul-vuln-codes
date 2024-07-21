static void parse_options(int argc, char* argv[], Options& o)
{
    for (int i = 1; i < argc; ++i)
    {
        char const* arg = argv[i];
        if ((arg[0] == '-') && (strcmp(arg, "-") != 0))
        {
            ++arg;
            if (arg[0] == '-')
            {
                // Be lax about -arg vs --arg
                ++arg;
            }
            char* parameter = const_cast<char*>(strchr(arg, '='));
            if (parameter)
            {
                *parameter++ = 0;
            }

            // Arguments that start with space are undocumented and
            // are for use by the test suite.
            if (strcmp(arg, " test-numrange") == 0)
            {
                test_numrange(parameter);
                exit(0);
            }
            else if (strcmp(arg, "password") == 0)
            {
                if (parameter == 0)
                {
                    usage("--password must be given as --password=pass");
                }
                o.password = parameter;
            }
            else if (strcmp(arg, "empty") == 0)
            {
                o.infilename = "";
            }
            else if (strcmp(arg, "linearize") == 0)
            {
                o.linearize = true;
            }
            else if (strcmp(arg, "encrypt") == 0)
            {
                parse_encrypt_options(
                    argc, argv, ++i,
                    o.user_password, o.owner_password, o.keylen,
                    o.r2_print, o.r2_modify, o.r2_extract, o.r2_annotate,
                    o.r3_accessibility, o.r3_extract, o.r3_print, o.r3_modify,
                    o.force_V4, o.cleartext_metadata, o.use_aes, o.force_R5);
                o.encrypt = true;
                o.decrypt = false;
                o.copy_encryption = false;
            }
            else if (strcmp(arg, "decrypt") == 0)
            {
                o.decrypt = true;
                o.encrypt = false;
                o.copy_encryption = false;
            }
            else if (strcmp(arg, "copy-encryption") == 0)
            {
                if (parameter == 0)
                {
                    usage("--copy-encryption must be given as"
                          "--copy_encryption=file");
                }
                o.encryption_file = parameter;
                o.copy_encryption = true;
                o.encrypt = false;
                o.decrypt = false;
            }
            else if (strcmp(arg, "encryption-file-password") == 0)
            {
                if (parameter == 0)
                {
                    usage("--encryption-file-password must be given as"
                          "--encryption-file-password=password");
                }
                o.encryption_file_password = parameter;
            }
            else if (strcmp(arg, "pages") == 0)
            {
                o.page_specs = parse_pages_options(argc, argv, ++i);
                if (o.page_specs.empty())
                {
                    usage("--pages: no page specifications given");
                }
            }
            else if (strcmp(arg, "rotate") == 0)
            {
                parse_rotation_parameter(o, parameter);
            }
            else if (strcmp(arg, "stream-data") == 0)
            {
                if (parameter == 0)
                {
                    usage("--stream-data must be given as"
                          "--stream-data=option");
                }
                o.stream_data_set = true;
                if (strcmp(parameter, "compress") == 0)
                {
                    o.stream_data_mode = qpdf_s_compress;
                }
                else if (strcmp(parameter, "preserve") == 0)
                {
                    o.stream_data_mode = qpdf_s_preserve;
                }
                else if (strcmp(parameter, "uncompress") == 0)
                {
                    o.stream_data_mode = qpdf_s_uncompress;
                }
                else
                {
                    usage("invalid stream-data option");
                }
            }
            else if (strcmp(arg, "compress-streams") == 0)
            {
                o.compress_streams_set = true;
                if (parameter && (strcmp(parameter, "y") == 0))
                {
                    o.compress_streams = true;
                }
                else if (parameter && (strcmp(parameter, "n") == 0))
                {
                    o.compress_streams = false;
                }
                else
                {
                    usage("--compress-streams must be given as"
                          " --compress-streams=[yn]");
                }
            }
            else if (strcmp(arg, "decode-level") == 0)
            {
                if (parameter == 0)
                {
                    usage("--decode-level must be given as"
                          "--decode-level=option");
                }
                o.decode_level_set = true;
                if (strcmp(parameter, "none") == 0)
                {
                    o.decode_level = qpdf_dl_none;
                }
                else if (strcmp(parameter, "generalized") == 0)
                {
                    o.decode_level = qpdf_dl_generalized;
                }
                else if (strcmp(parameter, "specialized") == 0)
                {
                    o.decode_level = qpdf_dl_specialized;
                }
                else if (strcmp(parameter, "all") == 0)
                {
                    o.decode_level = qpdf_dl_all;
                }
                else
                {
                    usage("invalid stream-data option");
                }
            }
            else if (strcmp(arg, "normalize-content") == 0)
            {
                o.normalize_set = true;
                if (parameter && (strcmp(parameter, "y") == 0))
                {
                    o.normalize = true;
                }
                else if (parameter && (strcmp(parameter, "n") == 0))
                {
                    o.normalize = false;
                }
                else
                {
                    usage("--normalize-content must be given as"
                          " --normalize-content=[yn]");
                }
            }
            else if (strcmp(arg, "suppress-recovery") == 0)
            {
                o.suppress_recovery = true;
            }
            else if (strcmp(arg, "object-streams") == 0)
            {
                if (parameter == 0)
                {
                    usage("--object-streams must be given as"
                          " --object-streams=option");
                }
                o.object_stream_set = true;
                if (strcmp(parameter, "disable") == 0)
                {
                    o.object_stream_mode = qpdf_o_disable;
                }
                else if (strcmp(parameter, "preserve") == 0)
                {
                    o.object_stream_mode = qpdf_o_preserve;
                }
                else if (strcmp(parameter, "generate") == 0)
                {
                    o.object_stream_mode = qpdf_o_generate;
                }
                else
                {
                    usage("invalid object stream mode");
                }
            }
            else if (strcmp(arg, "ignore-xref-streams") == 0)
            {
                o.ignore_xref_streams = true;
            }
            else if (strcmp(arg, "qdf") == 0)
            {
                o.qdf_mode = true;
            }
            else if (strcmp(arg, "preserve-unreferenced") == 0)
            {
                o.preserve_unreferenced_objects = true;
            }
            else if (strcmp(arg, "newline-before-endstream") == 0)
            {
                o.newline_before_endstream = true;
            }
            else if (strcmp(arg, "min-version") == 0)
            {
                if (parameter == 0)
                {
                    usage("--min-version be given as"
                          "--min-version=version");
                }
                o.min_version = parameter;
            }
            else if (strcmp(arg, "force-version") == 0)
            {
                if (parameter == 0)
                {
                    usage("--force-version be given as"
                          "--force-version=version");
                }
                o.force_version = parameter;
            }
            else if (strcmp(arg, "split-pages") == 0)
            {
                int n = ((parameter == 0) ? 1 : atoi(parameter));
                o.split_pages = n;
            }
            else if (strcmp(arg, "verbose") == 0)
            {
                o.verbose = true;
            }
            else if (strcmp(arg, "deterministic-id") == 0)
            {
                o.deterministic_id = true;
            }
            else if (strcmp(arg, "static-id") == 0)
            {
                o.static_id = true;
            }
            else if (strcmp(arg, "static-aes-iv") == 0)
            {
                o.static_aes_iv = true;
            }
            else if (strcmp(arg, "no-original-object-ids") == 0)
            {
                o.suppress_original_object_id = true;
            }
            else if (strcmp(arg, "show-encryption") == 0)
            {
                o.show_encryption = true;
                o.require_outfile = false;
            }
            else if (strcmp(arg, "check-linearization") == 0)
            {
                o.check_linearization = true;
                o.require_outfile = false;
            }
            else if (strcmp(arg, "show-linearization") == 0)
            {
                o.show_linearization = true;
                o.require_outfile = false;
            }
            else if (strcmp(arg, "show-xref") == 0)
            {
                o.show_xref = true;
                o.require_outfile = false;
            }
            else if (strcmp(arg, "show-object") == 0)
            {
                if (parameter == 0)
                {
                    usage("--show-object must be given as"
                          " --show-object=obj[,gen]");
                }
                char* obj = parameter;
                char* gen = obj;
                if ((gen = strchr(obj, ',')) != 0)
                {
                    *gen++ = 0;
                    o.show_gen = atoi(gen);
                }
                o.show_obj = atoi(obj);
                o.require_outfile = false;
            }
            else if (strcmp(arg, "raw-stream-data") == 0)
            {
                o.show_raw_stream_data = true;
            }
            else if (strcmp(arg, "filtered-stream-data") == 0)
            {
                o.show_filtered_stream_data = true;
            }
            else if (strcmp(arg, "show-npages") == 0)
            {
                o.show_npages = true;
                o.require_outfile = false;
            }
            else if (strcmp(arg, "show-pages") == 0)
            {
                o.show_pages = true;
                o.require_outfile = false;
            }
            else if (strcmp(arg, "with-images") == 0)
            {
                o.show_page_images = true;
            }
            else if (strcmp(arg, "check") == 0)
            {
                o.check = true;
                o.require_outfile = false;
            }
            else
            {
                usage(std::string("unknown option --") + arg);
            }
        }
        else if (o.infilename == 0)
        {
            o.infilename = arg;
        }
        else if (o.outfilename == 0)
        {
            o.outfilename = arg;
        }
        else
        {
            usage(std::string("unknown argument ") + arg);
        }
    }

    if (o.infilename == 0)
    {
        usage("an input file name is required");
    }
    else if (o.require_outfile && (o.outfilename == 0))
    {
        usage("an output file name is required; use - for standard output");
    }
    else if ((! o.require_outfile) && (o.outfilename != 0))
    {
        usage("no output file may be given for this option");
    }

    if (o.require_outfile && (strcmp(o.outfilename, "-") == 0) &&
        o.split_pages)
    {
        usage("--split-pages may not be used when writing to standard output");
    }

    if (QUtil::same_file(o.infilename, o.outfilename))
    {
        QTC::TC("qpdf", "qpdf same file error");
        usage("input file and output file are the same; this would cause input file to be lost");
    }
}