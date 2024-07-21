MP4::Properties::Properties(File *file, MP4::Atoms *atoms, ReadStyle style)
  : AudioProperties(style)
{
  d = new PropertiesPrivate;

  MP4::Atom *moov = atoms->find("moov");
  if(!moov) {
    debug("MP4: Atom 'moov' not found");
    return;
  }

  MP4::Atom *trak = 0;
  ByteVector data;

  MP4::AtomList trakList = moov->findall("trak");
  for (unsigned int i = 0; i < trakList.size(); i++) {
    trak = trakList[i];
    MP4::Atom *hdlr = trak->find("mdia", "hdlr");
    if(!hdlr) {
      debug("MP4: Atom 'trak.mdia.hdlr' not found");
      return;
    }
    file->seek(hdlr->offset);
    data = file->readBlock(hdlr->length);
    if(data.mid(16, 4) == "soun") {
      break;
    }
    trak = 0;
  }
  if (!trak) {
    debug("MP4: No audio tracks");
    return;
  }

  MP4::Atom *mdhd = trak->find("mdia", "mdhd");
  if(!mdhd) {
    debug("MP4: Atom 'trak.mdia.mdhd' not found");
    return;
  }

  file->seek(mdhd->offset);
  data = file->readBlock(mdhd->length);
  if(data[8] == 0) {
    unsigned int unit = data.mid(20, 4).toUInt();
    unsigned int length = data.mid(24, 4).toUInt();
    d->length = length / unit;
  }
  else {
    long long unit = data.mid(28, 8).toLongLong();
    long long length = data.mid(36, 8).toLongLong();
    d->length = int(length / unit);
  }

  MP4::Atom *atom = trak->find("mdia", "minf", "stbl", "stsd");
  if(!atom) {
    return;
  }

  file->seek(atom->offset);
  data = file->readBlock(atom->length);
  if(data.mid(20, 4) == "mp4a") {
    d->channels = data.mid(40, 2).toShort();
    d->bitsPerSample = data.mid(42, 2).toShort();
    d->sampleRate = data.mid(46, 4).toUInt();
    if(data.mid(56, 4) == "esds" && data[64] == 0x03) {
      long pos = 65;
      if(data.mid(pos, 3) == "\x80\x80\x80") {
        pos += 3;
      }
      pos += 4;
      if(data[pos] == 0x04) {
        pos += 1;
        if(data.mid(pos, 3) == "\x80\x80\x80") {
          pos += 3;
        }
        pos += 10;
        d->bitrate = (data.mid(pos, 4).toUInt() + 500) / 1000;
      }
    }
  }
}