void Hints::readTables(BaseStream *str, Linearization *linearization, XRef *xref, SecurityHandler *secHdlr)
{
    hintsOffset = linearization->getHintsOffset();
    hintsLength = linearization->getHintsLength();
    hintsOffset2 = linearization->getHintsOffset2();
    hintsLength2 = linearization->getHintsLength2();

    const unsigned int bufLength = hintsLength + hintsLength2;

    if (bufLength == 0) {
        ok = false;
        return;
    }

    std::vector<char> buf(bufLength);
    char *p = &buf[0];

    if (hintsOffset && hintsLength) {
        Stream *s = str->makeSubStream(hintsOffset, false, hintsLength, Object(objNull));
        s->reset();
        for (unsigned int i = 0; i < hintsLength; i++) {
            *p++ = s->getChar();
        }
        delete s;
    }

    if (hintsOffset2 && hintsLength2) {
        Stream *s = str->makeSubStream(hintsOffset2, false, hintsLength2, Object(objNull));
        s->reset();
        for (unsigned int i = 0; i < hintsLength2; i++) {
            *p++ = s->getChar();
        }
        delete s;
    }

    MemStream *memStream = new MemStream(&buf[0], 0, bufLength, Object(objNull));

    Parser *parser = new Parser(xref, memStream, true);

    int num, gen;
    Object obj;
    if ((obj = parser->getObj(), obj.isInt()) && (num = obj.getInt(), obj = parser->getObj(), obj.isInt()) && (gen = obj.getInt(), obj = parser->getObj(), obj.isCmd("obj"))
        && (obj = parser->getObj(false, secHdlr ? secHdlr->getFileKey() : nullptr, secHdlr ? secHdlr->getEncAlgorithm() : cryptRC4, secHdlr ? secHdlr->getFileKeyLength() : 0, num, gen, 0, true), obj.isStream())) {
        Stream *hintsStream = obj.getStream();
        Dict *hintsDict = obj.streamGetDict();

        int sharedStreamOffset = 0;
        if (hintsDict->lookupInt("S", nullptr, &sharedStreamOffset) && sharedStreamOffset > 0) {

            hintsStream->reset();
            ok = readPageOffsetTable(hintsStream);

            if (ok) {
                hintsStream->reset();
                for (int i = 0; i < sharedStreamOffset; i++) {
                    hintsStream->getChar();
                }
                ok = readSharedObjectsTable(hintsStream);
            }
        } else {
            error(errSyntaxWarning, -1, "Invalid shared object hint table offset");
            ok = false;
        }
    } else {
        error(errSyntaxWarning, -1, "Failed parsing hints table object");
        ok = false;
    }

    delete parser;
}