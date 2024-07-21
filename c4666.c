static void CopyRectangle(rfbClient* client, uint8_t* buffer, int x, int y, int w, int h) {
  int j;

  if (client->frameBuffer == NULL) {
      return;
  }

#define COPY_RECT(BPP) \
  { \
    int rs = w * BPP / 8, rs2 = client->width * BPP / 8; \
    for (j = ((x * (BPP / 8)) + (y * rs2)); j < (y + h) * rs2; j += rs2) { \
      memcpy(client->frameBuffer + j, buffer, rs); \
      buffer += rs; \
    } \
  }

  switch(client->format.bitsPerPixel) {
  case  8: COPY_RECT(8);  break;
  case 16: COPY_RECT(16); break;
  case 32: COPY_RECT(32); break;
  default:
    rfbClientLog("Unsupported bitsPerPixel: %d\n",client->format.bitsPerPixel);
  }
}