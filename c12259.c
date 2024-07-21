main(int argc, char** argv)
{
    if (argc < 2) {
        PrintUsageAndExit();
    }
    
    // default options
    Options.input                          = NULL;
    Options.verbose                        = false;
    Options.hls_version                    = 0;
    Options.pmt_pid                        = 0x100;
    Options.audio_pid                      = 0x101;
    Options.video_pid                      = 0x102;
    Options.audio_track_id                 = -1;
    Options.video_track_id                 = -1;
    Options.audio_format                   = AUDIO_FORMAT_TS;
    Options.output_single_file             = false;
    Options.show_info                      = false;
    Options.index_filename                 = "stream.m3u8";
    Options.iframe_index_filename          = NULL;
    Options.segment_filename_template      = NULL;
    Options.segment_url_template           = NULL;
    Options.segment_duration               = 6;
    Options.segment_duration_threshold     = DefaultSegmentDurationThreshold;
    Options.allow_cache                    = NULL;
    Options.encryption_key_hex             = NULL;
    Options.encryption_mode                = ENCRYPTION_MODE_NONE;
    Options.encryption_iv_mode             = ENCRYPTION_IV_MODE_NONE;
    Options.encryption_key_uri             = "key.bin";
    Options.encryption_key_format          = NULL;
    Options.encryption_key_format_versions = NULL;
    Options.pcr_offset                     = AP4_MPEG2_TS_DEFAULT_PCR_OFFSET;
    AP4_SetMemory(Options.encryption_key, 0, sizeof(Options.encryption_key));
    AP4_SetMemory(Options.encryption_iv,  0, sizeof(Options.encryption_iv));
    AP4_SetMemory(&Stats, 0, sizeof(Stats));

    // parse command line
    AP4_Result result;
    char** args = argv+1;
    while (const char* arg = *args++) {
        if (!strcmp(arg, "--verbose")) {
            Options.verbose = true;
        } else if (!strcmp(arg, "--hls-version")) {
            if (*args == NULL) {
                fprintf(stderr, "ERROR: --hls-version requires a number\n");
                return 1;
            }
            Options.hls_version = (unsigned int)strtoul(*args++, NULL, 10);
            if (Options.hls_version ==0) {
                fprintf(stderr, "ERROR: --hls-version requires number > 0\n");
                return 1;
            }
        } else if (!strcmp(arg, "--segment-duration")) {
            if (*args == NULL) {
                fprintf(stderr, "ERROR: --segment-duration requires a number\n");
                return 1;
            }
            Options.segment_duration = (unsigned int)strtoul(*args++, NULL, 10);
        } else if (!strcmp(arg, "--segment-duration-threshold")) {
            if (*args == NULL) {
                fprintf(stderr, "ERROR: --segment-duration-threshold requires a number\n");
                return 1;
            }
            Options.segment_duration_threshold = (unsigned int)strtoul(*args++, NULL, 10);
        } else if (!strcmp(arg, "--segment-filename-template")) {
            if (*args == NULL) {
                fprintf(stderr, "ERROR: --segment-filename-template requires an argument\n");
                return 1;
            }
            Options.segment_filename_template = *args++;
        } else if (!strcmp(arg, "--segment-url-template")) {
            if (*args == NULL) {
                fprintf(stderr, "ERROR: --segment-url-template requires an argument\n");
                return 1;
            }
            Options.segment_url_template = *args++;
        } else if (!strcmp(arg, "--allow-cache")) {
            if (*args == NULL || (strcmp(*args, "NO") && strcmp(*args, "YES"))) {
                fprintf(stderr, "ERROR: --allow-cache requires a YES or NO argument\n");
                return 1;
            }
            Options.allow_cache = *args++;
        } else if (!strcmp(arg, "--pmt-pid")) {
            if (*args == NULL) {
                fprintf(stderr, "ERROR: --pmt-pid requires a number\n");
                return 1;
            }
            Options.pmt_pid = (unsigned int)strtoul(*args++, NULL, 10);
        } else if (!strcmp(arg, "--audio-pid")) {
            if (*args == NULL) {
                fprintf(stderr, "ERROR: --audio-pid requires a number\n");
                return 1;
            }
            Options.audio_pid = (unsigned int)strtoul(*args++, NULL, 10);
        } else if (!strcmp(arg, "--video-pid")) {
            if (*args == NULL) {
                fprintf(stderr, "ERROR: --video-pid requires a number\n");
                return 1;
            }
            Options.video_pid = (unsigned int)strtoul(*args++, NULL, 10);
        } else if (!strcmp(arg, "--audio-track-id")) {
            if (*args == NULL) {
                fprintf(stderr, "ERROR: --audio-track-id requires a number\n");
                return 1;
            }
            Options.audio_track_id = (unsigned int)strtoul(*args++, NULL, 10);
        } else if (!strcmp(arg, "--audio-format")) {
            if (*args == NULL) {
                fprintf(stderr, "ERROR: --audio-format requires an argument\n");
                return 1;
            }
            const char* format = *args++;
            if (!strcmp(format, "ts")) {
                Options.audio_format = AUDIO_FORMAT_TS;
            } else if (!strcmp(format, "packed")) {
                Options.audio_format = AUDIO_FORMAT_PACKED;
            } else {
                fprintf(stderr, "ERROR: unknown audio format\n");
                return 1;
            }
        } else if (!strcmp(arg, "--video-track-id")) {
            if (*args == NULL) {
                fprintf(stderr, "ERROR: --video-track-id requires a number\n");
                return 1;
            }
            Options.video_track_id = (unsigned int)strtoul(*args++, NULL, 10);
        } else if (!strcmp(arg, "--pcr-offset")) {
            if (*args == NULL) {
                fprintf(stderr, "ERROR: --pcr-offset requires a number\n");
                return 1;
            }
            Options.pcr_offset = (unsigned int)strtoul(*args++, NULL, 10);
        } else if (!strcmp(arg, "--output-single-file")) {
            Options.output_single_file = true;
        } else if (!strcmp(arg, "--index-filename")) {
            if (*args == NULL) {
                fprintf(stderr, "ERROR: --index-filename requires a filename\n");
                return 1;
            }
            Options.index_filename = *args++;
        } else if (!strcmp(arg, "--iframe-index-filename")) {
            if (*args == NULL) {
                fprintf(stderr, "ERROR: --iframe-index-filename requires a filename\n");
                return 1;
            }
            Options.iframe_index_filename = *args++;
        } else if (!strcmp(arg, "--show-info")) {
            Options.show_info = true;
        } else if (!strcmp(arg, "--encryption-key")) {
            if (*args == NULL) {
                fprintf(stderr, "ERROR: --encryption-key requires an argument\n");
                return 1;
            }
            Options.encryption_key_hex = *args++;
            result = AP4_ParseHex(Options.encryption_key_hex, Options.encryption_key, 16);
            if (AP4_FAILED(result)) {
                fprintf(stderr, "ERROR: invalid hex key\n");
                return 1;
            }
            if (Options.encryption_mode == ENCRYPTION_MODE_NONE) {
                Options.encryption_mode = ENCRYPTION_MODE_AES_128;
            }
        } else if (!strcmp(arg, "--encryption-mode")) {
            if (*args == NULL) {
                fprintf(stderr, "ERROR: --encryption-mode requires an argument\n");
                return 1;
            }
            if (strncmp(*args, "AES-128", 7) == 0) {
                Options.encryption_mode = ENCRYPTION_MODE_AES_128;
            } else if (strncmp(*args, "SAMPLE-AES", 10) == 0) {
                Options.encryption_mode = ENCRYPTION_MODE_SAMPLE_AES;
            } else {
                fprintf(stderr, "ERROR: unknown encryption mode\n");
                return 1;
            }
            ++args;
        } else if (!strcmp(arg, "--encryption-iv-mode")) {
            if (*args == NULL) {
                fprintf(stderr, "ERROR: --encryption-iv-mode requires an argument\n");
                return 1;
            }
            if (strncmp(*args, "sequence", 8) == 0) {
                Options.encryption_iv_mode = ENCRYPTION_IV_MODE_SEQUENCE;
            } else if (strncmp(*args, "random", 6) == 0) {
                Options.encryption_iv_mode = ENCRYPTION_IV_MODE_RANDOM;
            } else if (strncmp(*args, "fps", 3) == 0) {
                Options.encryption_iv_mode = ENCRYPTION_IV_MODE_FPS;
            } else {
                fprintf(stderr, "ERROR: unknown encryption IV mode\n");
                return 1;
            }
            ++args;
        } else if (!strcmp(arg, "--encryption-key-uri")) {
            if (*args == NULL) {
                fprintf(stderr, "ERROR: --encryption-key-uri requires an argument\n");
                return 1;
            }
            Options.encryption_key_uri = *args++;
        } else if (!strcmp(arg, "--encryption-key-format")) {
            if (*args == NULL) {
                fprintf(stderr, "ERROR: --encryption-key-format requires an argument\n");
                return 1;
            }
            Options.encryption_key_format = *args++;
        } else if (!strcmp(arg, "--encryption-key-format-versions")) {
            if (*args == NULL) {
                fprintf(stderr, "ERROR: --encryption-key-format-versions requires an argument\n");
                return 1;
            }
            Options.encryption_key_format_versions = *args++;
        } else if (!strcmp(arg, "--encryption-key-line")) {
            if (*args == NULL) {
                fprintf(stderr, "ERROR: --encryption-key-line requires an argument\n");
                return 1;
            }
            Options.encryption_key_lines.Append(*args++);
        } else if (Options.input == NULL) {
            Options.input = arg;
        } else {
            fprintf(stderr, "ERROR: unexpected argument: %s\n", arg);
            return 1;
        }
    }

    // check args
    if (Options.input == NULL) {
        fprintf(stderr, "ERROR: missing input file name\n");
        return 1;
    }
    if (Options.encryption_mode == ENCRYPTION_MODE_NONE && Options.encryption_key_lines.ItemCount() != 0) {
        fprintf(stderr, "ERROR: --encryption-key-line requires --encryption-key and --encryption-key-mode\n");
        return 1;
    }
    if (Options.encryption_mode != ENCRYPTION_MODE_NONE && Options.encryption_key_hex == NULL) {
        fprintf(stderr, "ERROR: no encryption key specified\n");
        return 1;
    }
    if (Options.encryption_mode == ENCRYPTION_MODE_SAMPLE_AES && Options.hls_version > 0 && Options.hls_version < 5) {
        Options.hls_version = 5;
        fprintf(stderr, "WARNING: forcing version to 5 in order to support SAMPLE-AES encryption\n");
    }
    if (Options.iframe_index_filename && Options.hls_version > 0 && Options.hls_version < 4) {
        fprintf(stderr, "WARNING: forcing version to 4 in order to support I-FRAME-ONLY playlists\n");
        Options.hls_version = 4;
    }
    if (Options.encryption_iv_mode == ENCRYPTION_IV_MODE_NONE && Options.encryption_mode != ENCRYPTION_MODE_NONE) {
        if (Options.encryption_mode == ENCRYPTION_MODE_SAMPLE_AES) {
            // sequence-mode IVs don't work well with i-frame only playlists, use random instead
            Options.encryption_iv_mode = ENCRYPTION_IV_MODE_RANDOM;
        } else {
            Options.encryption_iv_mode = ENCRYPTION_IV_MODE_SEQUENCE;
        }
    }
    if ((Options.encryption_key_format || Options.encryption_key_format_versions) && Options.hls_version > 0 && Options.hls_version < 5) {
        Options.hls_version = 5;
        fprintf(stderr, "WARNING: forcing version to 5 in order to support KEYFORMAT and/or KEYFORMATVERSIONS\n");
    }
    if (Options.output_single_file && Options.hls_version > 0 && Options.hls_version < 4) {
        Options.hls_version = 4;
        fprintf(stderr, "WARNING: forcing version to 4 in order to support single file output\n");
    }
    if (Options.hls_version == 0) {
        // default version is 3 for cleartext or AES-128 encryption, and 5 for SAMPLE-AES
        if (Options.encryption_mode == ENCRYPTION_MODE_SAMPLE_AES) {
            Options.hls_version = 5;
        } else if (Options.output_single_file || Options.iframe_index_filename) {
            Options.hls_version = 4;
        } else {
            Options.hls_version = 3;
        }
    }
    
    if (Options.verbose && Options.show_info) {
        fprintf(stderr, "WARNING: --verbose will be ignored because --show-info is selected\n");
        Options.verbose = false;
    }

    // compute some derived values
    if (Options.iframe_index_filename == NULL) {
        if (Options.hls_version >= 4) {
            Options.iframe_index_filename = "iframes.m3u8";
        }
    }
    if (Options.audio_format == AUDIO_FORMAT_TS) {
        if (Options.segment_filename_template == NULL) {
            if (Options.output_single_file) {
                Options.segment_filename_template = "stream.ts";
            } else {
                Options.segment_filename_template = "segment-%d.ts";
            }
        }
        if (Options.segment_url_template == NULL) {
            if (Options.output_single_file) {
                Options.segment_url_template = "stream.ts";
            } else {
                Options.segment_url_template = "segment-%d.ts";
            }
        }
    }
    
    if (Options.encryption_iv_mode == ENCRYPTION_IV_MODE_FPS) {
        if (AP4_StringLength(Options.encryption_key_hex) != 64) {
            fprintf(stderr, "ERROR: 'fps' IV mode requires a 32 byte key value (64 characters in hex)\n");
            return 1;
        }
        result = AP4_ParseHex(Options.encryption_key_hex+32, Options.encryption_iv, 16);
        if (AP4_FAILED(result)) {
            fprintf(stderr, "ERROR: invalid hex IV\n");
            return 1;
        }
    } else if (Options.encryption_iv_mode == ENCRYPTION_IV_MODE_RANDOM) {
        result = AP4_System_GenerateRandomBytes(Options.encryption_iv, sizeof(Options.encryption_iv));
        if (AP4_FAILED(result)) {
            fprintf(stderr, "ERROR: failed to get random IV (%d)\n", result);
            return 1;
        }
    }
    
	// create the input stream
    AP4_ByteStream* input = NULL;
    result = AP4_FileByteStream::Create(Options.input, AP4_FileByteStream::STREAM_MODE_READ, input);
    if (AP4_FAILED(result)) {
        fprintf(stderr, "ERROR: cannot open input (%d)\n", result);
        return 1;
    }
    
	// open the file
    AP4_File* input_file = new AP4_File(*input, true);

    // get the movie
    AP4_SampleDescription* sample_description;
    AP4_Movie* movie = input_file->GetMovie();
    if (movie == NULL) {
        fprintf(stderr, "ERROR: no movie in file\n");
        return 1;
    }

    // get the audio and video tracks
    AP4_Track* audio_track = NULL;
    if (Options.audio_track_id == -1) {
        audio_track = movie->GetTrack(AP4_Track::TYPE_AUDIO);
    } else if (Options.audio_track_id > 0) {
        audio_track = movie->GetTrack((AP4_UI32)Options.audio_track_id);
        if (audio_track == NULL) {
            fprintf(stderr, "ERROR: audio track ID %d not found\n", Options.audio_track_id);
            return 1;
        }
        if (audio_track->GetType() != AP4_Track::TYPE_AUDIO) {
            fprintf(stderr, "ERROR: track ID %d is not an audio track\n", Options.audio_track_id);
            return 1;
        }
    }
    AP4_Track* video_track = NULL;
    if (Options.video_track_id == -1) {
        video_track = movie->GetTrack(AP4_Track::TYPE_VIDEO);
    } else if (Options.video_track_id > 0) {
        video_track = movie->GetTrack((AP4_UI32)Options.video_track_id);
        if (video_track == NULL) {
            fprintf(stderr, "ERROR: video track ID %d not found\n", Options.video_track_id);
            return 1;
        }
        if (video_track->GetType() != AP4_Track::TYPE_VIDEO) {
            fprintf(stderr, "ERROR: track ID %d is not a video track\n", Options.video_track_id);
            return 1;
        }
    }
    if (audio_track == NULL && video_track == NULL) {
        fprintf(stderr, "ERROR: no suitable tracks found\n");
        delete input_file;
        input->Release();
        return 1;
    }
    if (Options.audio_format == AUDIO_FORMAT_PACKED && video_track != NULL) {
        if (audio_track == NULL) {
            fprintf(stderr, "ERROR: packed audio format requires an audio track\n");
            return 1;
        }
        fprintf(stderr, "WARNING: ignoring video track because of the packed audio format\n");
        video_track = NULL;
    }
    if (video_track == NULL) {
        Options.segment_duration_threshold = 0;
    }
    
    // create the appropriate readers
    AP4_LinearReader* linear_reader = NULL;
    SampleReader*     audio_reader  = NULL;
    SampleReader*     video_reader  = NULL;
    if (movie->HasFragments()) {
        // create a linear reader to get the samples
        linear_reader = new AP4_LinearReader(*movie, input);
    
        if (audio_track) {
            linear_reader->EnableTrack(audio_track->GetId());
            audio_reader = new FragmentedSampleReader(*linear_reader, audio_track->GetId());
        }
        if (video_track) {
            linear_reader->EnableTrack(video_track->GetId());
            video_reader = new FragmentedSampleReader(*linear_reader, video_track->GetId());
        }
    } else {
        if (audio_track) {
            audio_reader = new TrackSampleReader(*audio_track);
        }
        if (video_track) {
            video_reader = new TrackSampleReader(*video_track);
        }
    }
    
    AP4_Mpeg2TsWriter*               ts_writer = NULL;
    AP4_Mpeg2TsWriter::SampleStream* audio_stream = NULL;
    AP4_Mpeg2TsWriter::SampleStream* video_stream = NULL;
    AP4_UI08                         nalu_length_size = 0;
    PackedAudioWriter*               packed_writer = NULL;
    if (Options.audio_format == AUDIO_FORMAT_PACKED) {
        packed_writer = new PackedAudioWriter();
    
        // figure out the file extensions if needed
        sample_description = audio_track->GetSampleDescription(0);
        if (sample_description == NULL) {
            fprintf(stderr, "ERROR: unable to parse audio sample description\n");
            goto end;
        }
        if (Options.segment_filename_template == NULL || Options.segment_url_template == NULL) {
            const char* default_stream_name    = "stream.es";
            const char* default_stream_pattern = "segment-%d.es";
            if (sample_description->GetFormat() == AP4_SAMPLE_FORMAT_MP4A) {
                AP4_MpegAudioSampleDescription* mpeg_audio_desc = AP4_DYNAMIC_CAST(AP4_MpegAudioSampleDescription, sample_description);
                if (mpeg_audio_desc == NULL ||
                    !(mpeg_audio_desc->GetObjectTypeId() == AP4_OTI_MPEG4_AUDIO          ||
                      mpeg_audio_desc->GetObjectTypeId() == AP4_OTI_MPEG2_AAC_AUDIO_LC   ||
                      mpeg_audio_desc->GetObjectTypeId() == AP4_OTI_MPEG2_AAC_AUDIO_MAIN)) {
                    fprintf(stderr, "ERROR: only AAC audio is supported\n");
                    return 1;
                }
                default_stream_name    = "stream.aac";
                default_stream_pattern = "segment-%d.aac";
            } else if (sample_description->GetFormat() == AP4_SAMPLE_FORMAT_AC_3) {
                default_stream_name    = "stream.ac3";
                default_stream_pattern = "segment-%d.ac3";
            } else if (sample_description->GetFormat() == AP4_SAMPLE_FORMAT_EC_3) {
                default_stream_name    = "stream.ec3";
                default_stream_pattern = "segment-%d.ec3";
            } else if (sample_description->GetFormat() == AP4_SAMPLE_FORMAT_AC_4) {
                default_stream_name    = "stream.ac4";
                default_stream_pattern = "segment-%d.ac4";
            }

            // override the segment names
            if (Options.segment_filename_template == NULL) {
                if (Options.output_single_file) {
                    Options.segment_filename_template = default_stream_name;
                } else {
                    Options.segment_filename_template = default_stream_pattern;
                }
            }
            if (Options.segment_url_template == NULL) {
                if (Options.output_single_file) {
                    Options.segment_url_template = default_stream_name;
                } else {
                    Options.segment_url_template = default_stream_pattern;
                }
            }
        }
    } else {
        // create an MPEG2 TS Writer
        ts_writer = new AP4_Mpeg2TsWriter(Options.pmt_pid);

        // add the audio stream
        if (audio_track) {
            sample_description = audio_track->GetSampleDescription(0);
            if (sample_description == NULL) {
                fprintf(stderr, "ERROR: unable to parse audio sample description\n");
                goto end;
            }

            unsigned int stream_type = 0;
            unsigned int stream_id   = 0;
            if (sample_description->GetFormat() == AP4_SAMPLE_FORMAT_MP4A) {
                if (Options.encryption_mode == ENCRYPTION_MODE_SAMPLE_AES) {
                    stream_type = AP4_MPEG2_STREAM_TYPE_SAMPLE_AES_ISO_IEC_13818_7;
                } else {
                    stream_type = AP4_MPEG2_STREAM_TYPE_ISO_IEC_13818_7;
                }
                stream_id   = AP4_MPEG2_TS_DEFAULT_STREAM_ID_AUDIO;
            } else if (sample_description->GetFormat() == AP4_SAMPLE_FORMAT_AC_3) {
                if (Options.encryption_mode == ENCRYPTION_MODE_SAMPLE_AES) {
                    stream_type = AP4_MPEG2_STREAM_TYPE_SAMPLE_AES_ATSC_AC3;
                } else {
                    stream_type = AP4_MPEG2_STREAM_TYPE_ATSC_AC3;
                }
                stream_id   = AP4_MPEG2_TS_STREAM_ID_PRIVATE_STREAM_1;
            } else if (sample_description->GetFormat() == AP4_SAMPLE_FORMAT_EC_3) {
                if (Options.encryption_mode == ENCRYPTION_MODE_SAMPLE_AES) {
                    stream_type = AP4_MPEG2_STREAM_TYPE_SAMPLE_AES_ATSC_EAC3;
                } else {
                    stream_type = AP4_MPEG2_STREAM_TYPE_ATSC_EAC3;
                }
                stream_id   = AP4_MPEG2_TS_STREAM_ID_PRIVATE_STREAM_1;
            } else {
                fprintf(stderr, "ERROR: audio codec not supported\n");
                return 1;
            }
            if (stream_type == AP4_MPEG2_STREAM_TYPE_ATSC_EAC3) {
                // E-AC-3 descriptor
                unsigned int number_of_channels = 0;
                AP4_String track_language;
                AP4_Dec3Atom* dec3 = AP4_DYNAMIC_CAST(AP4_Dec3Atom, sample_description->GetDetails().GetChild(AP4_ATOM_TYPE_DEC3));
                AP4_BitWriter bits(8);
                bits.Write(0xCC, 8);
                bits.Write(0x06, 8);    // fixed value
                bits.Write(0xC0, 8);    // reserved, bsid_flag, mainid_flag, asvc_flag, mixinfoexists, substream1_flag, substream2_flag and substream3_flag 
                bits.Write(24, 5);      // reserved, full_service_flag and service_type
                if (dec3->GetSubStreams()[0].acmod == 0) {
                    number_of_channels = 1;
                } else if (dec3->GetSubStreams()[0].acmod == 1) {
                    number_of_channels = 0;
                } else if (dec3->GetSubStreams()[0].acmod == 2) {
                    number_of_channels = 2;
                } else {
                    number_of_channels = 4;
                }
                if (dec3->GetSubStreams()[0].num_dep_sub > 0) {
                    number_of_channels = 5;
                }
                bits.Write(number_of_channels, 3);              // number_of_channels
                bits.Write(4, 3);                               // language_flag, language_flag_2, reserved
                bits.Write(dec3->GetSubStreams()[0].bsid, 5);   // bsid
                track_language = audio_track->GetTrackLanguage();
                if (track_language.GetLength() == 3) {
                    bits.Write(track_language.GetChars()[0], 8);
                    bits.Write(track_language.GetChars()[1], 8);
                    bits.Write(track_language.GetChars()[2], 8);
                } else {
                    bits.Write(0x75, 8);
                    bits.Write(0x6E, 8);
                    bits.Write(0x64, 8);
                }
                 // setup the audio stream
                result = ts_writer->SetAudioStream(audio_track->GetMediaTimeScale(),
                                                   stream_type,
                                                   stream_id,
                                                   audio_stream,
                                                   Options.audio_pid,
                                                   bits.GetData(), 8,
                                                   Options.pcr_offset);
            } else {
            // setup the audio stream
            result = ts_writer->SetAudioStream(audio_track->GetMediaTimeScale(),
                                               stream_type,
                                               stream_id,
                                               audio_stream,
                                               Options.audio_pid,
                                               NULL, 0,
                                               Options.pcr_offset);
            }
            if (AP4_FAILED(result)) {
                fprintf(stderr, "could not create audio stream (%d)\n", result);
                goto end;
            }
        }
        
        // add the video stream
        if (video_track) {
            sample_description = video_track->GetSampleDescription(0);
            if (sample_description == NULL) {
                fprintf(stderr, "ERROR: unable to parse video sample description\n");
                goto end;
            }
            
            // decide on the stream type
            unsigned int stream_type = 0;
            unsigned int stream_id   = AP4_MPEG2_TS_DEFAULT_STREAM_ID_VIDEO;
            if (sample_description->GetFormat() == AP4_SAMPLE_FORMAT_AVC1 ||
                sample_description->GetFormat() == AP4_SAMPLE_FORMAT_AVC2 ||
                sample_description->GetFormat() == AP4_SAMPLE_FORMAT_AVC3 ||
                sample_description->GetFormat() == AP4_SAMPLE_FORMAT_AVC4 ||
                sample_description->GetFormat() == AP4_SAMPLE_FORMAT_DVAV ||
                sample_description->GetFormat() == AP4_SAMPLE_FORMAT_DVA1) {
                if (Options.encryption_mode == ENCRYPTION_MODE_SAMPLE_AES) {
                    stream_type = AP4_MPEG2_STREAM_TYPE_SAMPLE_AES_AVC;
                    AP4_AvcSampleDescription* avc_desc = AP4_DYNAMIC_CAST(AP4_AvcSampleDescription, sample_description);
                    if (avc_desc == NULL) {
                        fprintf(stderr, "ERROR: not a proper AVC track\n");
                        return 1;
                    }
                    nalu_length_size = avc_desc->GetNaluLengthSize();
                } else {
                    stream_type = AP4_MPEG2_STREAM_TYPE_AVC;
                }
            } else if (sample_description->GetFormat() == AP4_SAMPLE_FORMAT_HEV1 ||
                       sample_description->GetFormat() == AP4_SAMPLE_FORMAT_HVC1 ||
                       sample_description->GetFormat() == AP4_SAMPLE_FORMAT_DVHE ||
                       sample_description->GetFormat() == AP4_SAMPLE_FORMAT_DVH1) {
                stream_type = AP4_MPEG2_STREAM_TYPE_HEVC;
            } else {
                fprintf(stderr, "ERROR: video codec not supported\n");
                return 1;
            }
            if (Options.encryption_mode == ENCRYPTION_MODE_SAMPLE_AES) {
                if (stream_type != AP4_MPEG2_STREAM_TYPE_SAMPLE_AES_AVC) {
                    fprintf(stderr, "ERROR: AES-SAMPLE encryption can only be used with H.264 video\n");
                    return 1;
                }
            }
            
            // setup the video stream
            result = ts_writer->SetVideoStream(video_track->GetMediaTimeScale(),
                                               stream_type,
                                               stream_id,
                                               video_stream,
                                               Options.video_pid,
                                               NULL, 0,
                                               Options.pcr_offset);
            if (AP4_FAILED(result)) {
                fprintf(stderr, "could not create video stream (%d)\n", result);
                goto end;
            }
        }
    }
    
    result = WriteSamples(ts_writer, packed_writer,
                          audio_track, audio_reader, audio_stream,
                          video_track, video_reader, video_stream,
                          Options.segment_duration_threshold,
                          nalu_length_size);
    if (AP4_FAILED(result)) {
        fprintf(stderr, "ERROR: failed to write samples (%d)\n", result);
    }

    if (Options.show_info) {
        double average_segment_bitrate = 0.0;
        if (Stats.segments_total_duration != 0.0) {
            average_segment_bitrate = 8.0*(double)Stats.segments_total_size/Stats.segments_total_duration;
        }
        double average_iframe_bitrate = 0.0;
        if (Stats.segments_total_duration != 0.0) {
            average_iframe_bitrate = 8.0*(double)Stats.iframes_total_size/Stats.segments_total_duration;
        }

        double frame_rate = 0.0;
        if (video_track && (Stats.segments_total_duration != 0.0)) {
            double sample_count = (double)video_track->GetSampleCount();
            double media_duration = (double)video_track->GetMediaDuration();
            double timescale = (double)video_track->GetMediaTimeScale();
            if (media_duration > 0.0) {
                frame_rate = sample_count/(media_duration/timescale);
            }
        }

        printf(
            "{\n"
        );
        printf(
            "  \"stats\": {\n"
            "    \"duration\": %f,\n"
            "    \"avg_segment_bitrate\": %f,\n"
            "    \"max_segment_bitrate\": %f,\n"
            "    \"avg_iframe_bitrate\": %f,\n"
            "    \"max_iframe_bitrate\": %f,\n"
            "    \"frame_rate\": %f\n"
            "  }",
            (double)movie->GetDurationMs()/1000.0,
            average_segment_bitrate,
            Stats.max_segment_bitrate,
            average_iframe_bitrate,
            Stats.max_iframe_bitrate,
            frame_rate
        );
        if (audio_track) {
            AP4_String codec;
            AP4_SampleDescription* sdesc = audio_track->GetSampleDescription(0);
            if (sdesc) {
                sdesc->GetCodecString(codec);
            }
            printf(
                ",\n"
                "  \"audio\": {\n"
                "    \"codec\": \"%s\"\n"
                "  }",
                codec.GetChars()
            );
        }
        if (video_track) {
            AP4_String codec;
            AP4_UI16 width = (AP4_UI16)(video_track->GetWidth()/65536.0);
            AP4_UI16 height = (AP4_UI16)(video_track->GetHeight()/65536.0);
            AP4_SampleDescription* sdesc = video_track->GetSampleDescription(0);
            if (sdesc) {
                sdesc->GetCodecString(codec);
                AP4_VideoSampleDescription* video_desc = AP4_DYNAMIC_CAST(AP4_VideoSampleDescription, sdesc);
                if (video_desc) {
                    width = video_desc->GetWidth();
                    height = video_desc->GetHeight();
                }
            }
            printf(
                ",\n"
                "  \"video\": {\n"
                "    \"codec\": \"%s\",\n"
                "    \"width\": %d,\n"
                "    \"height\": %d\n"
                "  }",
                codec.GetChars(),
                width,
                height
            );
        }
        printf(
            "\n"
            "}\n"
        );
    }
    
end:
    delete ts_writer;
    delete packed_writer;
    delete input_file;
    input->Release();
    delete linear_reader;
    delete audio_reader;
    delete video_reader;
    
    return result == AP4_SUCCESS?0:1;
}