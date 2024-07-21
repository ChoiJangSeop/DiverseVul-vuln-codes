static void svg_process_media_href(GF_SVG_Parser *parser, GF_Node *elt, XMLRI *iri)
{
	u32 tag = gf_node_get_tag(elt);

	if ((tag==TAG_SVG_image) || (tag==TAG_SVG_video) || (tag==TAG_SVG_audio)) {
		SVG_SAFExternalStream *st = svg_saf_get_stream(parser, 0, iri->string+1);
		if (!st && !strnicmp(iri->string, "stream:", 7))
			st = svg_saf_get_stream(parser, 0, iri->string+7);
		if (st) {
			gf_free(iri->string);
			iri->string = NULL;
			iri->lsr_stream_id = st->id;
			iri->type = XMLRI_STREAMID;
			return;
		}
	}
	if ((parser->load->flags & GF_SM_LOAD_EMBEDS_RES) && (iri->type==XMLRI_STRING) ) {
		u32 size;
		char *buffer;

		GF_Err e = gf_file_load_data(iri->string, (u8 **) &buffer, &size);
		if (e) return;

		if (tag==TAG_SVG_script) {
			GF_DOMText *dtext;
			GF_DOMAttribute *att, *prev;
			buffer[size]=0;
			dtext = gf_dom_add_text_node(elt, buffer);
			dtext->type = GF_DOM_TEXT_CDATA;

			gf_free(iri->string);
			iri->string=NULL;

			/*delete attribute*/
			att = ((GF_DOMNode*)elt)->attributes;
			prev = NULL;
			while(att) {
				if (att->tag!=TAG_XLINK_ATT_href) {
					prev = att;
					att = att->next;
					continue;
				}
				gf_svg_delete_attribute_value(att->data_type, att->data, elt->sgprivate->scenegraph);
				if (prev) prev->next = att->next;
				else ((GF_DOMNode*)elt)->attributes = att->next;
				gf_free(att);
				break;
			}
		} else {
			char *mtype;
			char *buf64;
			u64 size64;
			char *ext;
			buf64 = (char *)gf_malloc((size_t)size*2);
			size64 = gf_base64_encode(buffer, (u32)size, buf64, (u32)size*2);
			buf64[size64] = 0;
			mtype = "application/data";
			ext = strchr(iri->string, '.');
			if (ext) {
				if (!stricmp(ext, ".png")) mtype = "image/png";
				if (!stricmp(ext, ".jpg") || !stricmp(ext, ".jpeg")) mtype = "image/jpg";
			}
			gf_free(iri->string);
			iri->string = (char *)gf_malloc(sizeof(char)*(40+(size_t)size64));
			sprintf(iri->string, "data:%s;base64,%s", mtype, buf64);
			gf_free(buf64);
			gf_free(buffer);
		}
	}
}