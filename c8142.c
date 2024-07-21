static bool check_transfer_bounds(struct vrend_resource *res,
                                  const struct vrend_transfer_info *info)
{
   int lwidth, lheight;

   /* check mipmap level is in bounds */
   if (info->level > res->base.last_level)
      return false;
   if (info->box->x < 0 || info->box->y < 0)
      return false;
   /* these will catch bad y/z/w/d with 1D textures etc */
   lwidth = u_minify(res->base.width0, info->level);
   if (info->box->width > lwidth)
      return false;
   if (info->box->x > lwidth)
      return false;
   if (info->box->width + info->box->x > lwidth)
      return false;

   lheight = u_minify(res->base.height0, info->level);
   if (info->box->height > lheight)
      return false;
   if (info->box->y > lheight)
      return false;
   if (info->box->height + info->box->y > lheight)
      return false;

   if (res->base.target == PIPE_TEXTURE_3D) {
      int ldepth = u_minify(res->base.depth0, info->level);
      if (info->box->depth > ldepth)
         return false;
      if (info->box->z > ldepth)
         return false;
      if (info->box->z + info->box->depth > ldepth)
         return false;
   } else {
      if (info->box->depth > (int)res->base.array_size)
         return false;
      if (info->box->z > (int)res->base.array_size)
         return false;
      if (info->box->z + info->box->depth > (int)res->base.array_size)
         return false;
   }

   return true;
}