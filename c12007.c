mp_sint32 LoaderS3M::load(XMFileBase& f, XModule* module)
{	
	module->cleanUp();

	// this will make code much easier to read
	TXMHeader*		header = &module->header;
	TXMInstrument*	instr  = module->instr;
	TXMSample*		smp	   = module->smp;
	TXMPattern*		phead  = module->phead;	

	// we're already out of memory here
	if (!phead || !instr || !smp)
		return MP_OUT_OF_MEMORY;
	
	f.read(&header->name,1,28);
	header->whythis1a = f.readByte();
	
	if (f.readByte() != 16) 
		return MP_LOADER_FAILED;	// no ST3 module
	
	f.readByte(); // skip something
	f.readByte(); // skip something
	
	header->ordnum = f.readWord(); // number of positions in order list (songlength)
	
	mp_ubyte* orders = new mp_ubyte[header->ordnum];
	if (orders == NULL) 
		return MP_OUT_OF_MEMORY;
	
	header->insnum = f.readWord(); // number of instruments
	header->patnum = f.readWord(); // number of patterns	
	
	mp_sint32 flags = f.readWord(); // st3 flags	

	mp_sint32 Cvt = f.readWord();

	header->flags = XModule::MODULE_ST3NEWINSTRUMENT | XModule::MODULE_ST3DUALCOMMANDS;

	if (Cvt == 0x1300 || (flags & 64))
		header->flags |= module->MODULE_OLDS3MVOLSLIDES;
		
	header->flags |= module->MODULE_ST3NOTECUT;
	
	/*mp_uword Ffi = */f.readWord();
	
	f.read(header->sig,1,4);
	
	header->mainvol = module->vol64to255(f.readByte()); // initial main volume
	
	header->tempo = f.readByte(); // tempo
	
	header->speed = f.readByte(); // speed
	
	f.readByte(); // global volume? skipped...
	
	f.readByte(); // ignore GUS click removal
	
	/*mp_ubyte dp = */f.readByte();
	
	f.readDword();	// skip something
	f.readDword();	// skip something
	f.readWord();	// skip some more...
	
	mp_ubyte channelSettings[32];
	f.read(channelSettings,1,32);
	
	mp_sint32 numChannels = 0;
	
	for (numChannels = 0; numChannels < 32; numChannels++)
		if (channelSettings[numChannels] == 255)
			break;
	
	header->channum = numChannels; // number of channels
	
	f.read(orders,1,header->ordnum);
	
	mp_sint32 j = 0, i = 0;
	for (i = 0; i < header->ordnum; i++)
	{
		if (orders[i] == 255) 
			break;
		
		header->ord[j++] = orders[i];		
	}
	
	header->ordnum = j; // final songlength
	
	delete[] orders;
	
	mp_uword* insParaPtrs = new mp_uword[header->insnum];
	
	if (insParaPtrs == NULL)
		return MP_OUT_OF_MEMORY;
	
	f.readWords(insParaPtrs,header->insnum);
	
	mp_uword* patParaPtrs = new mp_uword[header->patnum];
	
	if (patParaPtrs == NULL)
	{
		delete[] insParaPtrs;
		return MP_OUT_OF_MEMORY;
	}
	
	f.readWords(patParaPtrs,header->patnum);
	
	//for (i = 0; i < header->insnum; i++)
	//{
	//	printf("%x\n",insParaPtrs[i]*16);
	//}
		
	//////////////////////
	// read instruments //
	//////////////////////
	mp_uint32* samplePtrs = new mp_uint32[header->insnum];
	if (samplePtrs == NULL)
	{
		delete[] insParaPtrs;
		delete[] patParaPtrs;
		return MP_OUT_OF_MEMORY;
	}
	
	memset(samplePtrs,0,sizeof(mp_uint32)*header->insnum);
	
	mp_sint32 s = 0;
	for (i = 0; i < header->insnum; i++)
	{
		mp_uint32 insOffs = insParaPtrs[i]*16;

		if (insOffs)
		{
			f.seekWithBaseOffset(insOffs);
		
			// We can only read that if it's a sample
			mp_ubyte type = f.readByte();
			
			if (type == 1)
			{
				f.read(smp[s].name,1,12);	// read dos filename
		
				mp_ubyte bOffs = f.readByte();
				mp_uword wOffs = f.readWord();
				
				// stupid fileoffsets
				samplePtrs[i] = (((mp_uint32)bOffs<<16)+(mp_uint32)wOffs)*16;
				
				smp[s].flags = 1;
				smp[s].pan = 0x80;
				
				smp[s].samplen = f.readDword();
				smp[s].loopstart = f.readDword();
				mp_sint32 looplen = ((mp_sint32)f.readDword() - (mp_sint32)smp[s].loopstart);
				if (looplen < 0) 
					looplen = 0;
				smp[s].looplen = looplen;
				
				smp[s].vol = module->vol64to255(f.readByte());
				
				f.readByte(); // skip something
				
				smp[s].res = f.readByte() == 0x04 ? 0xAD : 0; // packing
				
				mp_ubyte flags = f.readByte();
				
				// looping
				if (flags & 1)
				{
					smp[s].type = 1;	
				}
				
				// 16 bit sample
				if (flags & 4)
				{
					smp[s].type |= 16;
					smp[s].samplen >>= 1;
					smp[s].loopstart >>= 1;
					smp[s].looplen >>= 1;
				}
				
				mp_uint32 c4spd = f.readDword();
				
				XModule::convertc4spd(c4spd,&smp[s].finetune,&smp[s].relnote);

#ifdef VERBOSE
				printf("%i, %i\n",c4spd,module->getc4spd(smp[s].relnote,smp[s].finetune));				
#endif

				f.readDword(); // skip something
				
				f.readDword(); // skip two internal words
				
				f.readDword(); // skip internal dword

				f.read(instr[i].name,1,28); // instrument name
				
				f.readDword(); // skip signature
				
				if (samplePtrs[i] && smp[s].samplen)
				{
					instr[i].samp=1;
					for (j=0;j<120;j++) 
						instr[i].snum[j] = s;
					s++;
				}
			}
			else if (type == 0)
			{
				samplePtrs[i] = 0;
			
				mp_ubyte buffer[12];
				f.read(buffer,1,12);	// read dos filename
		
				f.readByte();
				f.readWord();
				
				f.readDword();
				f.readDword();
				f.readDword();
				f.readByte();
				f.readByte(); // skip something
				f.readByte(); // skip packing
				
				f.readByte();
				
				f.readDword();
				
				f.readDword(); // skip something
				
				f.readDword(); // skip two internal words
				
				f.readDword(); // skip internal dword

				f.read(instr[i].name,1,28); // instrument name
				
				f.readDword(); // skip signature				
			}
			else 
			{
				samplePtrs[i] = 0;
			}
			
		}

	}
	
	//////////////////////
	// read patterns	//
	//////////////////////
	mp_ubyte* pattern = new mp_ubyte[64*32*5];
	if (pattern == NULL)
	{
		delete[] insParaPtrs;
		delete[] patParaPtrs;
		delete[] samplePtrs;
		return MP_OUT_OF_MEMORY;
	}
	
	mp_uint32 songMaxChannels = 1;
	
	for (i = 0; i < header->patnum; i++)
	{
		for (j = 0; j < 32*64; j++)
		{
			pattern[j*5] = 0xFF;
			pattern[j*5+1] = 0;
			pattern[j*5+2] = 0xFF;
			pattern[j*5+3] = 0xFF;
			pattern[j*5+4] = 0;
		}
		
		mp_uint32 patOffs = patParaPtrs[i]*16;
		
		mp_uint32 maxChannels = 1;			
		
		if (patOffs)
		{
			f.seekWithBaseOffset(patOffs);
			
			mp_uint32 size = f.readWord();
			
			if (size > 2)
			{
				size-=2;
				
				mp_ubyte* packed = new mp_ubyte[size+5];
				if (packed == NULL)
				{
					delete[] insParaPtrs;
					delete[] patParaPtrs;
					delete[] samplePtrs;
					delete[] pattern;
					return MP_OUT_OF_MEMORY;				
				}
				
				memset(packed, 0, size);
				f.read(packed, 1, size);
				
				mp_uint32 index = 0;
				mp_uint32 row = 0;
				
				while (index<size)
				{
					
					mp_ubyte pi = safeRead(packed, index, size);
					
					if (pi == 0) 
					{
						row++;
						// one more safety net for incorrectly saved pattern sizes
						if (row >= 64)
						{
							int i = 0;
							i++;
							i--;
							break;
						}
						continue;
					}
					
					mp_uint32 chn = pi&31;
					
					if (chn>maxChannels && (pi & (32+64+128)))
					{
						maxChannels = chn;
					}
					
					mp_ubyte* slot = pattern+(row*32*5)+chn*5;
					
					if (pi & 32)
					{
						slot[0] = safeRead(packed, index, size, 0xFF);
						slot[1] = safeRead(packed, index, size);
					}
					if (pi & 64)
					{
						slot[2] = safeRead(packed, index, size, 0xFF);
					}
					if (pi & 128)
					{
						slot[3] = safeRead(packed, index, size, 0xFF);
						slot[4] = safeRead(packed, index, size);
					}
					
				}
				
				maxChannels++;
				
				if (maxChannels > header->channum)
					maxChannels = header->channum;
				
				delete[] packed;
			}
			
			if (maxChannels > songMaxChannels)
				songMaxChannels = maxChannels;
			
		}
		
		convertS3MPattern(&phead[i], pattern, maxChannels, i);
		
		
	}
	
	if (header->channum > songMaxChannels)
		header->channum = songMaxChannels;
	
	delete[] pattern;
	delete[] insParaPtrs;
	delete[] patParaPtrs;
	
	s = 0;
	for (i = 0; i < header->insnum; i++)
	{
		mp_uint32 smpOffs = samplePtrs[i];

		if (smpOffs)
		{
			f.seekWithBaseOffset(smpOffs);
			
			if (!smp[s].samplen)
				continue;

			bool adpcm = (smp[s].res == 0xAD);

			mp_sint32 result = module->loadModuleSample(f, s, 
										  adpcm ? XModule::ST_PACKING_ADPCM : XModule::ST_UNSIGNED, 
										  adpcm ? (XModule::ST_16BIT | XModule::ST_PACKING_ADPCM) : (XModule::ST_16BIT | XModule::ST_UNSIGNED));
			if (result != MP_OK)
			{
				delete[] samplePtrs;
				return result;
			}
			
			if (adpcm)
				// no longer needed
				smp[s].res = 0;			
							
			s++;
			
		}

	}
	
	delete[] samplePtrs;
	
	header->smpnum = s;
	
	strcpy(header->tracker,"Screamtracker 3");
	
	module->setDefaultPanning();
	
	module->postProcessSamples();
	
	return MP_OK;	
}