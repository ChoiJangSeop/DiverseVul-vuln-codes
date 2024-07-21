static void CopyRectangleFromRectangle(rfbClient* client, int src_x, int src_y, int w, int h, int dest_x, int dest_y) {
  int i,j;

  if (client->frameBuffer == NULL) {
      return;
  }

#define COPY_RECT_FROM_RECT(BPP) \
  { \
    uint##BPP##_t* _buffer=((uint##BPP##_t*)client->frameBuffer)+(src_y-dest_y)*client->width+src_x-dest_x; \
    if (dest_y < src_y) { \
      for(j = dest_y*client->width; j < (dest_y+h)*client->width; j += client->width) { \
        if (dest_x < src_x) { \
          for(i = dest_x; i < dest_x+w; i++) { \
            ((uint##BPP##_t*)client->frameBuffer)[j+i]=_buffer[j+i]; \
          } \
        } else { \
          for(i = dest_x+w-1; i >= dest_x; i--) { \
            ((uint##BPP##_t*)client->frameBuffer)[j+i]=_buffer[j+i]; \
          } \
        } \
      } \
    } else { \
      for(j = (dest_y+h-1)*client->width; j >= dest_y*client->width; j-=client->width) { \
        if (dest_x < src_x) { \
          for(i = dest_x; i < dest_x+w; i++) { \
            ((uint##BPP##_t*)client->frameBuffer)[j+i]=_buffer[j+i]; \
          } \
        } else { \
          for(i = dest_x+w-1; i >= dest_x; i--) { \
            ((uint##BPP##_t*)client->frameBuffer)[j+i]=_buffer[j+i]; \
          } \
        } \
      } \
    } \
  }

  switch(client->format.bitsPerPixel) {
  case  8: COPY_RECT_FROM_RECT(8);  break;
  case 16: COPY_RECT_FROM_RECT(16); break;
  case 32: COPY_RECT_FROM_RECT(32); break;
  default:
    rfbClientLog("Unsupported bitsPerPixel: %d\n",client->format.bitsPerPixel);
  }
}