    Classifier (const char *&ptr, int size)
    {
        if (size <= 0) 
            throw IEX_NAMESPACE::InputExc("Error uncompressing DWA data"
                                " (truncated rule).");
            
        {
            char suffix[Name::SIZE];
            memset (suffix, 0, Name::SIZE);
            Xdr::read<CharPtrIO> (ptr, std::min(size, Name::SIZE-1), suffix);
            _suffix = std::string(suffix);
        }

        if (static_cast<size_t>(size) < _suffix.length() + 1 + 2*Xdr::size<char>()) 
            throw IEX_NAMESPACE::InputExc("Error uncompressing DWA data"
                                " (truncated rule).");

        char value;
        Xdr::read<CharPtrIO> (ptr, value);

        _cscIdx = (int)(value >> 4) - 1;
        if (_cscIdx < -1 || _cscIdx >= 3) 
            throw IEX_NAMESPACE::InputExc("Error uncompressing DWA data"
                                " (corrupt cscIdx rule).");

        _scheme = (CompressorScheme)((value >> 2) & 3);
        if (_scheme < 0 || _scheme >= NUM_COMPRESSOR_SCHEMES) 
            throw IEX_NAMESPACE::InputExc("Error uncompressing DWA data"
                                " (corrupt scheme rule).");

        _caseInsensitive = (value & 1 ? true : false);

        Xdr::read<CharPtrIO> (ptr, value);
        if (value < 0 || value >= NUM_PIXELTYPES) 
            throw IEX_NAMESPACE::InputExc("Error uncompressing DWA data"
                                " (corrupt rule).");
        _type = (PixelType)value;
    }