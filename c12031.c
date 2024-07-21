duint32 dwgCompressor::longCompressionOffset(){
    duint32 cont = 0;
    duint8 ll = bufC[pos++];
    while (ll == 0x00){
        cont += 0xFF;
        ll = bufC[pos++];
    }
    cont += ll;
    return cont;
}