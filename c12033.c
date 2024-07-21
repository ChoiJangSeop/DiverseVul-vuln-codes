void dwgCompressor::copyCompBytes21(duint8 *cbuf, duint8 *dbuf, duint32 l, duint32 si, duint32 di){
    duint32 length =l;
    duint32 dix = di;
    duint32 six = si;

    while (length > 31){
        //in doc: 16-31, 0-15
        for (duint32 i = six+24; i<six+32; i++)
            dbuf[dix++] = cbuf[i];
        for (duint32 i = six+16; i<six+24; i++)
            dbuf[dix++] = cbuf[i];
        for (duint32 i = six+8; i<six+16; i++)
            dbuf[dix++] = cbuf[i];
        for (duint32 i = six; i<six+8; i++)
            dbuf[dix++] = cbuf[i];
        six = six + 32;
        length = length -32;
    }

    switch (length) {
    case 0:
        break;
    case 1: //Ok
        dbuf[dix] = cbuf[six];
        break;
    case 2: //Ok
        dbuf[dix++] = cbuf[six+1];
        dbuf[dix] = cbuf[six];
        break;
    case 3: //Ok
        dbuf[dix++] = cbuf[six+2];
        dbuf[dix++] = cbuf[six+1];
        dbuf[dix] = cbuf[six];
        break;
    case 4: //Ok
        for (int i = 0; i<4;i++)
            dbuf[dix++] = cbuf[six++];
        break;
    case 5: //Ok
        dbuf[dix++] = cbuf[six+4];
        for (int i = 0; i<4;i++)
            dbuf[dix++] = cbuf[six++];
        break;
    case 6: //Ok
        dbuf[dix++] = cbuf[six+5];
        for (int i = 1; i<5;i++)
            dbuf[dix++] = cbuf[six+i];
        dbuf[dix] = cbuf[six];
        break;
    case 7:
        //in doc: six+5, six+6, 1-5, six+0
        dbuf[dix++] = cbuf[six+6];
        dbuf[dix++] = cbuf[six+5];
        for (int i = 1; i<5;i++)
            dbuf[dix++] = cbuf[six+i];
        dbuf[dix] = cbuf[six];
        break;
    case 8: //Ok
        for (int i = 0; i<8;i++)
            dbuf[dix++] = cbuf[six++];
        break;
    case 9: //Ok
        dbuf[dix++] = cbuf[six+8];
        for (int i = 0; i<8;i++)
            dbuf[dix++] = cbuf[six++];
        break;
    case 10: //Ok
        dbuf[dix++] = cbuf[six+9];
        for (int i = 1; i<9;i++)
            dbuf[dix++] = cbuf[six+i];
        dbuf[dix] = cbuf[six];
        break;
    case 11:
        //in doc: six+9, six+10, 1-9, six+0
        dbuf[dix++] = cbuf[six+10];
        dbuf[dix++] = cbuf[six+9];
        for (int i = 1; i<9;i++)
            dbuf[dix++] = cbuf[six+i];
        dbuf[dix] = cbuf[six];
        break;
    case 12: //Ok
        for (int i = 8; i<12;i++)
            dbuf[dix++] = cbuf[six+i];
        for (int i = 0; i<8;i++)
            dbuf[dix++] = cbuf[six++];
        break;
    case 13: //Ok
        dbuf[dix++] = cbuf[six+12];
        for (int i = 8; i<12;i++)
            dbuf[dix++] = cbuf[six+i];
        for (int i = 0; i<8;i++)
            dbuf[dix++] = cbuf[six++];
        break;
    case 14: //Ok
        dbuf[dix++] = cbuf[six+13];
        for (int i = 9; i<13; i++)
            dbuf[dix++] = cbuf[six+i];
        for (int i = 1; i<9; i++)
            dbuf[dix++] = cbuf[six+i];
        dbuf[dix] = cbuf[six];
        break;
    case 15:
        //in doc: six+13, six+14, 9-12, 1-8, six+0
        dbuf[dix++] = cbuf[six+14];
        dbuf[dix++] = cbuf[six+13];
        for (int i = 9; i<13; i++)
            dbuf[dix++] = cbuf[six+i];
        for (int i = 1; i<9; i++)
            dbuf[dix++] = cbuf[six+i];
        dbuf[dix] = cbuf[six];
        break;
    case 16: //Ok
        for (int i = 8; i<16;i++)
            dbuf[dix++] = cbuf[six+i];
        for (int i = 0; i<8;i++)
            dbuf[dix++] = cbuf[six++];
        break;
    case 17: //Ok
        for (int i = 9; i<17;i++)
            dbuf[dix++] = cbuf[six+i];
        dbuf[dix++] = cbuf[six+8];
        for (int i = 0; i<8;i++)
            dbuf[dix++] = cbuf[six++];
        break;
    case 18:
        //in doc: six+17, 1-16, six+0
        dbuf[dix++] = cbuf[six+17];
        for (int i = 9; i<17;i++)
            dbuf[dix++] = cbuf[six+i];
        for (int i = 1; i<9;i++)
            dbuf[dix++] = cbuf[six+i];
        dbuf[dix] = cbuf[six];
        break;
    case 19:
        //in doc: 16-18, 0-15
        dbuf[dix++] = cbuf[six+18];
        dbuf[dix++] = cbuf[six+17];
        dbuf[dix++] = cbuf[six+16];
        for (int i = 8; i<16;i++)
            dbuf[dix++] = cbuf[six+i];
        for (int i = 0; i<8;i++)
            dbuf[dix++] = cbuf[six++];
        break;
    case 20:
        //in doc: 16-19, 0-15
        for (int i = 16; i<20;i++)
            dbuf[dix++] = cbuf[six+i];
        for (int i = 8; i<16;i++)
            dbuf[dix++] = cbuf[six+i];
        for (int i = 0; i<8;i++)
            dbuf[dix++] = cbuf[six++];
        break;
    case 21:
        //in doc: six+20, 16-19, 0-15
        dbuf[dix++] = cbuf[six+20];
        for (int i = 16; i<20;i++)
            dbuf[dix++] = cbuf[six+i];
        for (int i = 8; i<16;i++)
            dbuf[dix++] = cbuf[six+i];
        for (int i = 0; i<8;i++)
            dbuf[dix++] = cbuf[six+i];
        break;
    case 22:
        //in doc: six+20, six+21, 16-19, 0-15
        dbuf[dix++] = cbuf[six+21];
        dbuf[dix++] = cbuf[six+20];
        for (int i = 16; i<20;i++)
            dbuf[dix++] = cbuf[six+i];
        for (int i = 8; i<16;i++)
            dbuf[dix++] = cbuf[six+i];
        for (int i = 0; i<8;i++)
            dbuf[dix++] = cbuf[six++];
        break;
    case 23:
        //in doc: six+20, six+21, six+22, 16-19, 0-15
        dbuf[dix++] = cbuf[six+22];
        dbuf[dix++] = cbuf[six+21];
        dbuf[dix++] = cbuf[six+20];
        for (int i = 16; i<20;i++)
            dbuf[dix++] = cbuf[six+i];
        for (int i = 8; i<16;i++)
            dbuf[dix++] = cbuf[six+i];
        for (int i = 0; i<8;i++)
            dbuf[dix++] = cbuf[six+i];
        break;
    case 24:
        //in doc: 16-23, 0-15
        for (int i = 16; i<24;i++)
            dbuf[dix++] = cbuf[six+i];
        for (int i = 8; i<16;i++)
            dbuf[dix++] = cbuf[six+i];
        for (int i = 0; i<8; i++)
            dbuf[dix++] = cbuf[six++];
        break;
    case 25:
        //in doc: 17-24, six+16, 0-15
        for (int i = 17; i<25;i++)
            dbuf[dix++] = cbuf[six+i];
        dbuf[dix++] = cbuf[six+16];
        for (int i = 8; i<16; i++)
            dbuf[dix++] = cbuf[six+i];
        for (int i = 0; i<8; i++)
            dbuf[dix++] = cbuf[six++];
        break;
    case 26:
        //in doc: six+25, 17-24, six+16, 0-15
        dbuf[dix++] = cbuf[six+25];
        for (int i = 17; i<25;i++)
            dbuf[dix++] = cbuf[six+i];
        dbuf[dix++] = cbuf[six+16];
        for (int i = 8; i<16;i++)
            dbuf[dix++] = cbuf[six+i];
        for (int i = 0; i<8; i++)
            dbuf[dix++] = cbuf[six++];
        break;
    case 27:
        //in doc: six+25, six+26, 17-24, six+16, 0-15
        dbuf[dix++] = cbuf[six+26];
        dbuf[dix++] = cbuf[six+25];
        for (int i = 17; i<25;i++)
            dbuf[dix++] = cbuf[six+i];
        dbuf[dix++] = cbuf[six+16];
        for (int i = 8; i<16;i++)
            dbuf[dix++] = cbuf[six+i];
        for (int i = 0; i<8; i++)
            dbuf[dix++] = cbuf[six++];
        break;
    case 28:
        //in doc: 24-27, 16-23, 0-15
        for (int i = 24; i<28; i++)
            dbuf[dix++] = cbuf[six+i];
        for (int i = 16; i<24;i++)
            dbuf[dix++] = cbuf[six+i];
        for (int i = 8; i<16; i++)
            dbuf[dix++] = cbuf[six+i];
        for (int i = 0; i<8; i++)
            dbuf[dix++] = cbuf[six++];
        break;
    case 29:
        //in doc: six+28, 24-27, 16-23, 0-15
        dbuf[dix++] = cbuf[six+28];
        for (int i = 24; i<28; i++)
            dbuf[dix++] = cbuf[six+i];
        for (int i = 16; i<24;i++)
            dbuf[dix++] = cbuf[six+i];
        for (int i = 8; i<16;i++)
            dbuf[dix++] = cbuf[six+i];
        for (int i = 0; i<8; i++)
            dbuf[dix++] = cbuf[six++];
        break;
    case 30:
        //in doc: six+28, six+29, 24-27, 16-23, 0-15
        dbuf[dix++] = cbuf[six+29];
        dbuf[dix++] = cbuf[six+28];
        for (int i = 24; i<28; i++)
            dbuf[dix++] = cbuf[six+i];
        for (int i = 16; i<24;i++)
            dbuf[dix++] = cbuf[six+i];
        for (int i = 8; i<16;i++)
            dbuf[dix++] = cbuf[six+i];
        for (int i = 0; i<8; i++)
            dbuf[dix++] = cbuf[six++];
        break;
    case 31:
        //in doc: six+30, 26-29, 18-25, 2-17, 0-1
        dbuf[dix++] = cbuf[six+30];
        for (int i = 26; i<30;i++)
            dbuf[dix++] = cbuf[six+i];
        for (int i = 18; i<26;i++)
            dbuf[dix++] = cbuf[six+i];
/*        for (int i = 2; i<18; i++)
            dbuf[dix++] = cbuf[six+i];*/
        for (int i = 10; i<18; i++)
            dbuf[dix++] = cbuf[six+i];
        for (int i = 2; i<10; i++)
            dbuf[dix++] = cbuf[six+i];
        dbuf[dix++] = cbuf[six+1];
        dbuf[dix] = cbuf[six];
        break;
    default:
        DRW_DBG("WARNING dwgCompressor::copyCompBytes21, bad output.\n");
        break;
    }
}