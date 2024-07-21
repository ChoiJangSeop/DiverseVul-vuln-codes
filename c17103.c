static int parse_next_header(struct message_parser_ctx *ctx,
			     struct message_block *block_r)
{
	struct message_part *part = ctx->part;
	struct message_header_line *hdr;
	struct message_boundary *boundary;
	bool full;
	int ret;

	if ((ret = message_parser_read_more(ctx, block_r, &full)) == 0)
		return ret;

	if (ret > 0 && block_is_at_eoh(block_r) &&
	    ctx->last_boundary != NULL &&
	    (part->flags & MESSAGE_PART_FLAG_IS_MIME) != 0) {
		/* we are at the end of headers and we've determined that we're
		   going to start a multipart. add the boundary already here
		   at this point so we can reliably determine whether the
		   "\n--boundary" belongs to us or to a previous boundary.
		   this is a problem if the boundary prefixes are identical,
		   because MIME requires only the prefix to match. */
		if (!parse_too_many_nested_mime_parts(ctx)) {
			parse_next_body_multipart_init(ctx);
			ctx->multipart = TRUE;
		} else {
			part->flags &= ENUM_NEGATE(MESSAGE_PART_FLAG_MULTIPART);
		}
	}

	/* before parsing the header see if we can find a --boundary from here.
	   we're guaranteed to be at the beginning of the line here. */
	if (ret > 0) {
		ret = ctx->boundaries == NULL ? -1 :
			boundary_line_find(ctx, block_r->data,
					   block_r->size, full, &boundary);
		if (ret > 0 && boundary->part == ctx->part) {
			/* our own body begins with our own --boundary.
			   we don't want to handle that yet. */
			ret = -1;
		}
	}
	if (ret < 0) {
		/* no boundary */
		ret = message_parse_header_next(ctx->hdr_parser_ctx, &hdr);
		if (ret == 0 || (ret < 0 && ctx->input->stream_errno != 0)) {
			ctx->want_count = i_stream_get_data_size(ctx->input) + 1;
			return ret;
		}
	} else if (ret == 0) {
		/* need more data */
		return 0;
	} else {
		/* boundary found. stop parsing headers here. The previous
		   [CR]LF belongs to the MIME boundary though. */
		if (ctx->prev_hdr_newline_size > 0) {
			i_assert(ctx->part->header_size.lines > 0);
			/* remove the newline size from the MIME header */
			ctx->part->header_size.lines--;
			ctx->part->header_size.physical_size -=
				ctx->prev_hdr_newline_size;
			ctx->part->header_size.virtual_size -= 2;
			/* add the newline size to the parent's body */
			ctx->part->parent->body_size.lines++;
			ctx->part->parent->body_size.physical_size +=
				ctx->prev_hdr_newline_size;
			ctx->part->parent->body_size.virtual_size += 2;
		}
		hdr = NULL;
	}

	if (hdr != NULL) {
		if (hdr->eoh)
			;
		else if (strcasecmp(hdr->name, "Mime-Version") == 0) {
			/* it's MIME. Content-* headers are valid */
			part->flags |= MESSAGE_PART_FLAG_IS_MIME;
		} else if (strcasecmp(hdr->name, "Content-Type") == 0) {
			if ((ctx->flags &
			     MESSAGE_PARSER_FLAG_MIME_VERSION_STRICT) == 0)
				part->flags |= MESSAGE_PART_FLAG_IS_MIME;

			if (hdr->continues)
				hdr->use_full_value = TRUE;
			else T_BEGIN {
				parse_content_type(ctx, hdr);
			} T_END;
		}

		block_r->hdr = hdr;
		block_r->size = 0;
		ctx->prev_hdr_newline_size = hdr->no_newline ? 0 :
			(hdr->crlf_newline ? 2 : 1);
		return 1;
	}

	/* end of headers */
	if ((part->flags & MESSAGE_PART_FLAG_IS_MIME) == 0) {
		/* It's not MIME. Reset everything we found from
		   Content-Type. */
		i_assert(!ctx->multipart);
		part->flags = 0;
	}
	i_free(ctx->last_boundary);

	if (!ctx->part_seen_content_type ||
	    (part->flags & MESSAGE_PART_FLAG_IS_MIME) == 0) {
		if (part->parent != NULL &&
		    (part->parent->flags &
		     MESSAGE_PART_FLAG_MULTIPART_DIGEST) != 0) {
			/* when there's no content-type specified and we're
			   below multipart/digest, assume message/rfc822
			   content-type */
			part->flags |= MESSAGE_PART_FLAG_MESSAGE_RFC822;
		} else {
			/* otherwise we default to text/plain */
			part->flags |= MESSAGE_PART_FLAG_TEXT;
		}
	}

	if (message_parse_header_has_nuls(ctx->hdr_parser_ctx))
		part->flags |= MESSAGE_PART_FLAG_HAS_NULS;
	message_parse_header_deinit(&ctx->hdr_parser_ctx);

	i_assert((part->flags & MUTEX_FLAGS) != MUTEX_FLAGS);

	ctx->last_chr = '\n';
	if (ctx->multipart) {
		i_assert(ctx->last_boundary == NULL);
		ctx->multipart = FALSE;
		ctx->parse_next_block = parse_next_body_to_boundary;
	} else if ((part->flags & MESSAGE_PART_FLAG_MESSAGE_RFC822) != 0 &&
		   !parse_too_many_nested_mime_parts(ctx)) {
		ctx->parse_next_block = parse_next_body_message_rfc822_init;
	} else {
		part->flags &= ENUM_NEGATE(MESSAGE_PART_FLAG_MESSAGE_RFC822);
		if (ctx->boundaries != NULL)
			ctx->parse_next_block = parse_next_body_to_boundary;
		else
			ctx->parse_next_block = parse_next_body_to_eof;
	}

	ctx->want_count = 1;

	/* return empty block as end of headers */
	block_r->hdr = NULL;
	block_r->size = 0;
	return 1;
}