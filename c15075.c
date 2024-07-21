GF_Err gf_bifs_flush_command_list(GF_BifsDecoder *codec)
{
	GF_BitStream *bs;
	GF_Err e;
	CommandBufferItem *cbi;
	GF_SceneGraph *prev_root = codec->current_graph;
	u32 NbPass = gf_list_count(codec->command_buffers);
	GF_List *nextPass = gf_list_new();
	while (NbPass) {
		while (gf_list_count(codec->command_buffers)) {
			cbi = (CommandBufferItem *)gf_list_get(codec->command_buffers, 0);
			gf_list_rem(codec->command_buffers, 0);

			codec->current_graph = gf_node_get_graph(cbi->node);
			e = GF_OK;
			if (cbi->cb->bufferSize) {
				bs = gf_bs_new((char*)cbi->cb->buffer, cbi->cb->bufferSize, GF_BITSTREAM_READ);
				gf_bs_set_eos_callback(bs, BM_EndOfStream, codec);
				e = BM_ParseCommand(codec, bs, cbi->cb->commandList);
				gf_bs_del(bs);
			}
			if (!e) {
				gf_node_unregister(cbi->node, NULL);
				gf_free(cbi);
				continue;
			}
			/*this may be an error or a dependency pb - reset coimmand list and move to next pass*/
			while (gf_list_count(cbi->cb->commandList)) {
				u32 i;
				GF_CommandField *cf;
				GF_Command *com = (GF_Command *)gf_list_get(cbi->cb->commandList, 0);
				gf_list_rem(cbi->cb->commandList, 0);
				cf = (GF_CommandField *) gf_list_get(com->command_fields, 0);
				if (cf && cf->fieldType==GF_SG_VRML_SFCOMMANDBUFFER) {
					for (i=0; i<gf_list_count(codec->command_buffers); i++) {
						CommandBufferItem *cbi2 = (CommandBufferItem *)gf_list_get(codec->command_buffers, i);
						if (cbi2->cb == cf->field_ptr) {
							gf_node_unregister(cbi2->node, NULL);
							gf_free(cbi2);
							gf_list_rem(codec->command_buffers, i);
							i--;
						}
					}
				}
				gf_sg_command_del(com);
			}
			gf_list_add(nextPass, cbi);
		}
		if (!gf_list_count(nextPass)) break;
		/*prepare next pass*/
		while (gf_list_count(nextPass)) {
			cbi = (CommandBufferItem *)gf_list_get(nextPass, 0);
			gf_list_rem(nextPass, 0);
			gf_list_add(codec->command_buffers, cbi);
		}
		NbPass --;
		if (NbPass > gf_list_count(codec->command_buffers)) NbPass = gf_list_count(codec->command_buffers);
		codec->LastError = GF_OK;
	}
	gf_list_del(nextPass);
	codec->current_graph = prev_root;
	return GF_OK;
}