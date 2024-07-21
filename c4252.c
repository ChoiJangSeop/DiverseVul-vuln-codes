	TORRENT_EXTRA_EXPORT void inflate_gzip(
		char const* in
		, int size
		, std::vector<char>& buffer
		, int maximum_size
		, error_code& ec)
	{
		ec.clear();
		TORRENT_ASSERT(maximum_size > 0);

		int header_len = gzip_header(in, size);
		if (header_len < 0)
		{
			ec = gzip_errors::invalid_gzip_header;
			return;
		}

		// start off with 4 kilobytes and grow
		// if needed
		boost::uint32_t destlen = 4096;
		int ret = 0;
		boost::uint32_t srclen = size - header_len;
		in += header_len;

		do
		{
			TORRENT_TRY {
				buffer.resize(destlen);
			} TORRENT_CATCH(std::exception&) {
				ec = errors::no_memory;
				return;
			}

			ret = puff(reinterpret_cast<unsigned char*>(&buffer[0]), &destlen
				, reinterpret_cast<const unsigned char*>(in), &srclen);

			// if the destination buffer wasn't large enough, double its
			// size and try again. Unless it's already at its max, in which
			// case we fail
			if (ret == 1) // 1:  output space exhausted before completing inflate
			{
				if (destlen == boost::uint32_t(maximum_size))
				{
					ec = gzip_errors::inflated_data_too_large;
					return;
				}

				destlen *= 2;
				if (destlen > boost::uint32_t(maximum_size))
					destlen = maximum_size;
			}
		} while (ret == 1);

		if (ret != 0)
		{
			switch (ret)
			{
				case   2: ec = gzip_errors::data_did_not_terminate; return;
				case   1: ec = gzip_errors::space_exhausted; return;
				case  -1: ec = gzip_errors::invalid_block_type; return;
				case  -2: ec = gzip_errors::invalid_stored_block_length; return;
				case  -3: ec = gzip_errors::too_many_length_or_distance_codes; return;
				case  -4: ec = gzip_errors::code_lengths_codes_incomplete; return;
				case  -5: ec = gzip_errors::repeat_lengths_with_no_first_length; return;
				case  -6: ec = gzip_errors::repeat_more_than_specified_lengths; return;
				case  -7: ec = gzip_errors::invalid_literal_length_code_lengths; return;
				case  -8: ec = gzip_errors::invalid_distance_code_lengths; return;
				case  -9: ec = gzip_errors::invalid_literal_code_in_block; return;
				case -10: ec = gzip_errors::distance_too_far_back_in_block; return;
				default: ec = gzip_errors::unknown_gzip_error; return;
			}
		}

		if (destlen > buffer.size())
		{
			ec = gzip_errors::unknown_gzip_error;
			return;
		}

		buffer.resize(destlen);
	}