png_write_info(png_structrp png_ptr, png_const_inforp info_ptr)
{
#if defined(PNG_WRITE_TEXT_SUPPORTED) || defined(PNG_WRITE_sPLT_SUPPORTED)
   int i;
#endif

   png_debug(1, "in png_write_info");

   if (png_ptr == NULL || info_ptr == NULL)
      return;

   png_write_info_before_PLTE(png_ptr, info_ptr);

   if ((info_ptr->valid & PNG_INFO_PLTE) != 0)
      png_write_PLTE(png_ptr, info_ptr->palette,
          (png_uint_32)info_ptr->num_palette);

   else if ((info_ptr->color_type == PNG_COLOR_TYPE_PALETTE) !=0)
      png_error(png_ptr, "Valid palette required for paletted images");

#ifdef PNG_WRITE_tRNS_SUPPORTED
   if ((info_ptr->valid & PNG_INFO_tRNS) !=0)
   {
#ifdef PNG_WRITE_INVERT_ALPHA_SUPPORTED
      /* Invert the alpha channel (in tRNS) */
      if ((png_ptr->transformations & PNG_INVERT_ALPHA) != 0 &&
          info_ptr->color_type == PNG_COLOR_TYPE_PALETTE)
      {
         int j, jend;

         jend = info_ptr->num_trans;
         if (jend > PNG_MAX_PALETTE_LENGTH)
            jend = PNG_MAX_PALETTE_LENGTH;

         for (j = 0; j<jend; ++j)
            info_ptr->trans_alpha[j] =
               (png_byte)(255 - info_ptr->trans_alpha[j]);
      }
#endif
      png_write_tRNS(png_ptr, info_ptr->trans_alpha, &(info_ptr->trans_color),
          info_ptr->num_trans, info_ptr->color_type);
   }
#endif
#ifdef PNG_WRITE_bKGD_SUPPORTED
   if ((info_ptr->valid & PNG_INFO_bKGD) != 0)
      png_write_bKGD(png_ptr, &(info_ptr->background), info_ptr->color_type);
#endif

#ifdef PNG_WRITE_hIST_SUPPORTED
   if ((info_ptr->valid & PNG_INFO_hIST) != 0)
      png_write_hIST(png_ptr, info_ptr->hist, info_ptr->num_palette);
#endif

#ifdef PNG_WRITE_oFFs_SUPPORTED
   if ((info_ptr->valid & PNG_INFO_oFFs) != 0)
      png_write_oFFs(png_ptr, info_ptr->x_offset, info_ptr->y_offset,
          info_ptr->offset_unit_type);
#endif

#ifdef PNG_WRITE_pCAL_SUPPORTED
   if ((info_ptr->valid & PNG_INFO_pCAL) != 0)
      png_write_pCAL(png_ptr, info_ptr->pcal_purpose, info_ptr->pcal_X0,
          info_ptr->pcal_X1, info_ptr->pcal_type, info_ptr->pcal_nparams,
          info_ptr->pcal_units, info_ptr->pcal_params);
#endif

#ifdef PNG_WRITE_sCAL_SUPPORTED
   if ((info_ptr->valid & PNG_INFO_sCAL) != 0)
      png_write_sCAL_s(png_ptr, (int)info_ptr->scal_unit,
          info_ptr->scal_s_width, info_ptr->scal_s_height);
#endif /* sCAL */

#ifdef PNG_WRITE_pHYs_SUPPORTED
   if ((info_ptr->valid & PNG_INFO_pHYs) != 0)
      png_write_pHYs(png_ptr, info_ptr->x_pixels_per_unit,
          info_ptr->y_pixels_per_unit, info_ptr->phys_unit_type);
#endif /* pHYs */

#ifdef PNG_WRITE_tIME_SUPPORTED
   if ((info_ptr->valid & PNG_INFO_tIME) != 0)
   {
      png_write_tIME(png_ptr, &(info_ptr->mod_time));
      png_ptr->mode |= PNG_WROTE_tIME;
   }
#endif /* tIME */

#ifdef PNG_WRITE_sPLT_SUPPORTED
   if ((info_ptr->valid & PNG_INFO_sPLT) != 0)
      for (i = 0; i < (int)info_ptr->splt_palettes_num; i++)
         png_write_sPLT(png_ptr, info_ptr->splt_palettes + i);
#endif /* sPLT */

#ifdef PNG_WRITE_TEXT_SUPPORTED
   /* Check to see if we need to write text chunks */
   for (i = 0; i < info_ptr->num_text; i++)
   {
      png_debug2(2, "Writing header text chunk %d, type %d", i,
          info_ptr->text[i].compression);
      /* An internationalized chunk? */
      if (info_ptr->text[i].compression > 0)
      {
#ifdef PNG_WRITE_iTXt_SUPPORTED
         /* Write international chunk */
         png_write_iTXt(png_ptr,
             info_ptr->text[i].compression,
             info_ptr->text[i].key,
             info_ptr->text[i].lang,
             info_ptr->text[i].lang_key,
             info_ptr->text[i].text);
         /* Mark this chunk as written */
         if (info_ptr->text[i].compression == PNG_TEXT_COMPRESSION_NONE)
            info_ptr->text[i].compression = PNG_TEXT_COMPRESSION_NONE_WR;
         else
            info_ptr->text[i].compression = PNG_TEXT_COMPRESSION_zTXt_WR;
#else
         png_warning(png_ptr, "Unable to write international text");
#endif
      }

      /* If we want a compressed text chunk */
      else if (info_ptr->text[i].compression == PNG_TEXT_COMPRESSION_zTXt)
      {
#ifdef PNG_WRITE_zTXt_SUPPORTED
         /* Write compressed chunk */
         png_write_zTXt(png_ptr, info_ptr->text[i].key,
             info_ptr->text[i].text, info_ptr->text[i].compression);
         /* Mark this chunk as written */
         info_ptr->text[i].compression = PNG_TEXT_COMPRESSION_zTXt_WR;
#else
         png_warning(png_ptr, "Unable to write compressed text");
#endif
      }

      else if (info_ptr->text[i].compression == PNG_TEXT_COMPRESSION_NONE)
      {
#ifdef PNG_WRITE_tEXt_SUPPORTED
         /* Write uncompressed chunk */
         png_write_tEXt(png_ptr, info_ptr->text[i].key,
             info_ptr->text[i].text,
             0);
         /* Mark this chunk as written */
         info_ptr->text[i].compression = PNG_TEXT_COMPRESSION_NONE_WR;
#else
         /* Can't get here */
         png_warning(png_ptr, "Unable to write uncompressed text");
#endif
      }
   }
#endif /* tEXt */

#ifdef PNG_WRITE_UNKNOWN_CHUNKS_SUPPORTED
   write_unknown_chunks(png_ptr, info_ptr, PNG_HAVE_PLTE);
#endif
}