void bitmap_cache_free(rdpBitmapCache* bitmapCache)
{
	int i, j;
	rdpBitmap* bitmap;

	if (bitmapCache)
	{
		for (i = 0; i < (int)bitmapCache->maxCells; i++)
		{
			for (j = 0; j < (int)bitmapCache->cells[i].number + 1; j++)
			{
				bitmap = bitmapCache->cells[i].entries[j];
				Bitmap_Free(bitmapCache->context, bitmap);
			}

			free(bitmapCache->cells[i].entries);
		}

		free(bitmapCache->cells);
		free(bitmapCache);
	}
}