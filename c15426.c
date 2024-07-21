GF_Err stbl_AddChunkOffset(GF_MediaBox *mdia, u32 sampleNumber, u32 StreamDescIndex, u64 offset, u32 nb_pack_samples)
{
	GF_SampleTableBox *stbl;
	GF_ChunkOffsetBox *stco;
	GF_SampleToChunkBox *stsc;
	GF_ChunkLargeOffsetBox *co64;
	GF_StscEntry *ent;
	u32 i, k, *newOff, new_chunk_idx=0;
	u64 *newLarge;
	s32 insert_idx = -1;

	stbl = mdia->information->sampleTable;
	stsc = stbl->SampleToChunk;

//	if (stsc->w_lastSampleNumber + 1 < sampleNumber ) return GF_BAD_PARAM;
	if (!nb_pack_samples)
		nb_pack_samples = 1;

	if (!stsc->nb_entries || (stsc->nb_entries + 2 >= stsc->alloc_size)) {
		if (!stsc->alloc_size) stsc->alloc_size = 1;
		ALLOC_INC(stsc->alloc_size);
		stsc->entries = gf_realloc(stsc->entries, sizeof(GF_StscEntry)*stsc->alloc_size);
		if (!stsc->entries) return GF_OUT_OF_MEM;
		memset(&stsc->entries[stsc->nb_entries], 0, sizeof(GF_StscEntry)*(stsc->alloc_size-stsc->nb_entries) );
	}
	if (sampleNumber == stsc->w_lastSampleNumber + 1) {
		ent = &stsc->entries[stsc->nb_entries];
		stsc->w_lastChunkNumber ++;
		ent->firstChunk = stsc->w_lastChunkNumber;
		if (stsc->nb_entries) stsc->entries[stsc->nb_entries-1].nextChunk = stsc->w_lastChunkNumber;

		new_chunk_idx = stsc->w_lastChunkNumber;
		stsc->w_lastSampleNumber = sampleNumber + nb_pack_samples-1;
		stsc->nb_entries += 1;
	} else {
		u32 cur_samp = 1;
		u32 samples_in_next_entry = 0;
		u32 next_entry_first_chunk = 1;
		for (i=0; i<stsc->nb_entries; i++) {
			u32 nb_chunks = 1;
			ent = &stsc->entries[i];
			if (i+1<stsc->nb_entries) nb_chunks = stsc->entries[i+1].firstChunk - ent->firstChunk;
			for (k=0; k<nb_chunks; k++) {
				if ((cur_samp <= sampleNumber) && (ent->samplesPerChunk + cur_samp > sampleNumber)) {
					insert_idx = i;
					//stsc entry has samples before inserted sample, split
					if (sampleNumber>cur_samp) {
						samples_in_next_entry = ent->samplesPerChunk - (sampleNumber-cur_samp);
						ent->samplesPerChunk = sampleNumber-cur_samp;
					}
					break;
				}
				cur_samp += ent->samplesPerChunk;
				next_entry_first_chunk++;
			}
			if (insert_idx>=0) break;
		}
		//we need to split the entry
		if (samples_in_next_entry) {
			memmove(&stsc->entries[insert_idx+3], &stsc->entries[insert_idx+1], sizeof(GF_StscEntry)*(stsc->nb_entries - insert_idx - 1));
			//copy over original entry
			ent = &stsc->entries[insert_idx];
			stsc->entries[insert_idx+2] = *ent;
			stsc->entries[insert_idx+2].samplesPerChunk = samples_in_next_entry;
			stsc->entries[insert_idx+2].firstChunk = next_entry_first_chunk + 1;

			//setup new entry
			ent = &stsc->entries[insert_idx+1];
			ent->firstChunk = next_entry_first_chunk;

			stsc->nb_entries += 2;
		} else {
			if (insert_idx<0) {
				ent = &stsc->entries[stsc->nb_entries];
				insert_idx = stsc->nb_entries;
			} else {
				memmove(&stsc->entries[insert_idx+1], &stsc->entries[insert_idx], sizeof(GF_StscEntry)*(stsc->nb_entries+1-insert_idx));
				ent = &stsc->entries[insert_idx+1];
			}

			ent->firstChunk = next_entry_first_chunk;
			stsc->nb_entries += 1;
		}
		new_chunk_idx = next_entry_first_chunk;
	}
	ent->isEdited = (Media_IsSelfContained(mdia, StreamDescIndex)) ? 1 : 0;
	ent->sampleDescriptionIndex = StreamDescIndex;
	ent->samplesPerChunk = nb_pack_samples;
	ent->nextChunk = ent->firstChunk+1;

	//OK, now if we've inserted a chunk, update the sample to chunk info...
	if (sampleNumber + nb_pack_samples - 1 == stsc->w_lastSampleNumber) {
		if (stsc->nb_entries)
			stsc->entries[stsc->nb_entries-1].nextChunk = ent->firstChunk;

		stbl->SampleToChunk->currentIndex = stsc->nb_entries-1;
		stbl->SampleToChunk->firstSampleInCurrentChunk = sampleNumber;
		//write - edit mode: sample number = chunk number
		stbl->SampleToChunk->currentChunk = stsc->w_lastChunkNumber;
		stbl->SampleToChunk->ghostNumber = 1;
	} else {
		/*offset remaining entries*/
		for (i = insert_idx+1; i<stsc->nb_entries+1; i++) {
			stsc->entries[i].firstChunk++;
			if (i+1<stsc->nb_entries)
				stsc->entries[i-1].nextChunk = stsc->entries[i].firstChunk;
		}
	}

	//add the offset to the chunk...
	//and we change our offset
	if (stbl->ChunkOffset->type == GF_ISOM_BOX_TYPE_STCO) {
		stco = (GF_ChunkOffsetBox *)stbl->ChunkOffset;
		//if the new offset is a large one, we have to rewrite our table entry by entry (32->64 bit conv)...
		if (offset > 0xFFFFFFFF) {
			co64 = (GF_ChunkLargeOffsetBox *) gf_isom_box_new_parent(&stbl->child_boxes, GF_ISOM_BOX_TYPE_CO64);
			if (!co64) return GF_OUT_OF_MEM;
			co64->nb_entries = stco->nb_entries + 1;
			co64->alloc_size = co64->nb_entries;
			co64->offsets = (u64*)gf_malloc(sizeof(u64) * co64->nb_entries);
			if (!co64->offsets) return GF_OUT_OF_MEM;
			k = 0;
			for (i=0; i<stco->nb_entries; i++) {
				if (i + 1 == new_chunk_idx) {
					co64->offsets[i] = offset;
					k = 1;
				}
				co64->offsets[i+k] = (u64) stco->offsets[i];
			}
			if (!k) co64->offsets[co64->nb_entries - 1] = offset;
			gf_isom_box_del_parent(&stbl->child_boxes, stbl->ChunkOffset);
			stbl->ChunkOffset = (GF_Box *) co64;
		} else {
			//no, we can use this one.
			if (new_chunk_idx > stco->nb_entries) {
				if (!stco->alloc_size) stco->alloc_size = stco->nb_entries;
				if (stco->nb_entries == stco->alloc_size) {
					ALLOC_INC(stco->alloc_size);
					stco->offsets = (u32*)gf_realloc(stco->offsets, sizeof(u32) * stco->alloc_size);
					if (!stco->offsets) return GF_OUT_OF_MEM;
					memset(&stco->offsets[stco->nb_entries], 0, sizeof(u32) * (stco->alloc_size-stco->nb_entries) );
				}
				stco->offsets[stco->nb_entries] = (u32) offset;
				stco->nb_entries += 1;
			} else {
				//nope. we're inserting
				newOff = (u32*)gf_malloc(sizeof(u32) * (stco->nb_entries + 1));
				if (!newOff) return GF_OUT_OF_MEM;
				k=0;
				for (i=0; i<stco->nb_entries; i++) {
					if (i+1 == new_chunk_idx) {
						newOff[i] = (u32) offset;
						k=1;
					}
					newOff[i+k] = stco->offsets[i];
				}
				gf_free(stco->offsets);
				stco->offsets = newOff;
				stco->nb_entries ++;
				stco->alloc_size = stco->nb_entries;
			}
		}
	} else {
		//use large offset...
		co64 = (GF_ChunkLargeOffsetBox *)stbl->ChunkOffset;
		if (sampleNumber > co64->nb_entries) {
			if (!co64->alloc_size) co64->alloc_size = co64->nb_entries;
			if (co64->nb_entries == co64->alloc_size) {
				ALLOC_INC(co64->alloc_size);
				co64->offsets = (u64*)gf_realloc(co64->offsets, sizeof(u64) * co64->alloc_size);
				if (!co64->offsets) return GF_OUT_OF_MEM;
				memset(&co64->offsets[co64->nb_entries], 0, sizeof(u64) * (co64->alloc_size - co64->nb_entries) );
			}
			co64->offsets[co64->nb_entries] = offset;
			co64->nb_entries += 1;
		} else {
			//nope. we're inserting
			newLarge = (u64*)gf_malloc(sizeof(u64) * (co64->nb_entries + 1));
			if (!newLarge) return GF_OUT_OF_MEM;
			k=0;
			for (i=0; i<co64->nb_entries; i++) {
				if (i+1 == new_chunk_idx) {
					newLarge[i] = offset;
					k=1;
				}
				newLarge[i+k] = co64->offsets[i];
			}
			gf_free(co64->offsets);
			co64->offsets = newLarge;
			co64->nb_entries++;
			co64->alloc_size++;
		}
	}

	return GF_OK;
}