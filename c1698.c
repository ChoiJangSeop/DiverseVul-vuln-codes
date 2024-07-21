void AudioOutputSpeech::addFrameToBuffer(const QByteArray &qbaPacket, unsigned int iSeq) {
	QMutexLocker lock(&qmJitter);

	if (qbaPacket.size() < 2)
		return;

	PacketDataStream pds(qbaPacket);

	// skip flags
	pds.next();

	int samples = 0;
	if (umtType == MessageHandler::UDPVoiceOpus) {
		int size;
		pds >> size;
		size &= 0x1fff;

		const QByteArray &qba = pds.dataBlock(size);
		const unsigned char *packet = reinterpret_cast<const unsigned char*>(qba.constData());

#ifdef USE_OPUS
		int frames = opus_packet_get_nb_frames(packet, size);
		samples = frames * opus_packet_get_samples_per_frame(packet, SAMPLE_RATE);
#else
		return;
#endif

		// We can't handle frames which are not a multiple of 10ms.
		Q_ASSERT(samples % iFrameSize == 0);
	} else {
		unsigned int header = 0;

		do {
			header = static_cast<unsigned char>(pds.next());
			samples += iFrameSize;
			pds.skip(header & 0x7f);
		} while ((header & 0x80) && pds.isValid());
	}

	if (pds.isValid()) {
		JitterBufferPacket jbp;
		jbp.data = const_cast<char *>(qbaPacket.constData());
		jbp.len = qbaPacket.size();
		jbp.span = samples;
		jbp.timestamp = iFrameSize * iSeq;

#ifdef REPORT_JITTER
		if (g.s.bUsage && (umtType != MessageHandler::UDPVoiceSpeex) && p && ! p->qsHash.isEmpty() && (p->qlTiming.count() < 3000)) {
			QMutexLocker qml(& p->qmTiming);

			ClientUser::JitterRecord jr;
			jr.iSequence = iSeq;
			jr.iFrames = frames;
			jr.uiElapsed = p->tTiming.restart();

			if (! p->qlTiming.isEmpty()) {
				jr.iFrames -= p->iFrames;
				jr.iSequence -= p->iSequence + p->iFrames;
			}
			p->iFrames = frames;
			p->iSequence = iSeq;

			p->qlTiming.append(jr);
		}
#endif

		jitter_buffer_put(jbJitter, &jbp);
	}
}