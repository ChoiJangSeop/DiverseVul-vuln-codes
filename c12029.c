duint32 dwgCompressor::litLength18(){
    duint32 cont=0;
    duint8 ll = bufC[pos++];
    //no literal length, this byte is next opCode
    if (ll > 0x0F) {
        pos--;
        return 0;
    }

    if (ll == 0x00) {
        cont = 0x0F;
        ll = bufC[pos++];
        while (ll == 0x00){//repeat until ll != 0x00
            cont +=0xFF;
            ll = bufC[pos++];
        }
    }
    cont +=ll;
    cont +=3; //already sum 3
    return cont;
}