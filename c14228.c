dwg_encode_add_object (Dwg_Object *restrict obj, Bit_Chain *restrict dat,
                       unsigned long address)
{
  int error = 0;
  unsigned long oldpos;
  unsigned long end_address = address + obj->size;

  oldpos = bit_position (dat);
  assert (address);
  dat->byte = address;
  dat->bit = 0;

  LOG_INFO ("Object number: %lu", (unsigned long)obj->index);
  if (obj->size > 0x100000)
    {
      LOG_ERROR ("Object size %u overflow", obj->size);
      return DWG_ERR_VALUEOUTOFBOUNDS;
    }
  while (dat->byte + obj->size >= dat->size)
    bit_chain_alloc (dat);

  // TODO: calculate size from the fields. either <0x7fff or more
  // patch it afterwards and check old<>new size if enough space allocated.
  bit_write_MS (dat, obj->size);
  obj->address = dat->byte;
  PRE (R_2010) {
    bit_write_BS (dat, obj->type);
    LOG_INFO (", Size: %d [MS], Type: %d [BS]\n", obj->size, obj->type)
  }
  LATER_VERSIONS
  {
    if (!obj->handlestream_size && obj->bitsize)
      obj->handlestream_size = obj->size * 8 - obj->bitsize;
    bit_write_UMC (dat, obj->handlestream_size);
    obj->address = dat->byte;
    bit_write_BOT (dat, obj->type);
    LOG_INFO (", Size: %d [MS], Hdlsize: %lu [UMC], Type: %d [BOT]\n",
              obj->size, (unsigned long)obj->handlestream_size, obj->type)
  }

  /* Write the specific type to dat */
  switch (obj->type)
    {
    case DWG_TYPE_TEXT:
      error = dwg_encode_TEXT (dat, obj);
      break;
    case DWG_TYPE_ATTRIB:
      error = dwg_encode_ATTRIB (dat, obj);
      break;
    case DWG_TYPE_ATTDEF:
      error = dwg_encode_ATTDEF (dat, obj);
      break;
    case DWG_TYPE_BLOCK:
      error = dwg_encode_BLOCK (dat, obj);
      break;
    case DWG_TYPE_ENDBLK:
      error = dwg_encode_ENDBLK (dat, obj);
      break;
    case DWG_TYPE_SEQEND:
      error = dwg_encode_SEQEND (dat, obj);
      break;
    case DWG_TYPE_INSERT:
      error = dwg_encode_INSERT (dat, obj);
      break;
    case DWG_TYPE_MINSERT:
      error = dwg_encode_MINSERT (dat, obj);
      break;
    case DWG_TYPE_VERTEX_2D:
      error = dwg_encode_VERTEX_2D (dat, obj);
      break;
    case DWG_TYPE_VERTEX_3D:
      error = dwg_encode_VERTEX_3D (dat, obj);
      break;
    case DWG_TYPE_VERTEX_MESH:
      error = dwg_encode_VERTEX_MESH (dat, obj);
      break;
    case DWG_TYPE_VERTEX_PFACE:
      error = dwg_encode_VERTEX_PFACE (dat, obj);
      break;
    case DWG_TYPE_VERTEX_PFACE_FACE:
      error = dwg_encode_VERTEX_PFACE_FACE (dat, obj);
      break;
    case DWG_TYPE_POLYLINE_2D:
      error = dwg_encode_POLYLINE_2D (dat, obj);
      break;
    case DWG_TYPE_POLYLINE_3D:
      error = dwg_encode_POLYLINE_3D (dat, obj);
      break;
    case DWG_TYPE_ARC:
      error = dwg_encode_ARC (dat, obj);
      break;
    case DWG_TYPE_CIRCLE:
      error = dwg_encode_CIRCLE (dat, obj);
      break;
    case DWG_TYPE_LINE:
      error = dwg_encode_LINE (dat, obj);
      break;
    case DWG_TYPE_DIMENSION_ORDINATE:
      error = dwg_encode_DIMENSION_ORDINATE (dat, obj);
      break;
    case DWG_TYPE_DIMENSION_LINEAR:
      error = dwg_encode_DIMENSION_LINEAR (dat, obj);
      break;
    case DWG_TYPE_DIMENSION_ALIGNED:
      error = dwg_encode_DIMENSION_ALIGNED (dat, obj);
      break;
    case DWG_TYPE_DIMENSION_ANG3PT:
      error = dwg_encode_DIMENSION_ANG3PT (dat, obj);
      break;
    case DWG_TYPE_DIMENSION_ANG2LN:
      error = dwg_encode_DIMENSION_ANG2LN (dat, obj);
      break;
    case DWG_TYPE_DIMENSION_RADIUS:
      error = dwg_encode_DIMENSION_RADIUS (dat, obj);
      break;
    case DWG_TYPE_DIMENSION_DIAMETER:
      error = dwg_encode_DIMENSION_DIAMETER (dat, obj);
      break;
    case DWG_TYPE_POINT:
      error = dwg_encode_POINT (dat, obj);
      break;
    case DWG_TYPE__3DFACE:
      error = dwg_encode__3DFACE (dat, obj);
      break;
    case DWG_TYPE_POLYLINE_PFACE:
      error = dwg_encode_POLYLINE_PFACE (dat, obj);
      break;
    case DWG_TYPE_POLYLINE_MESH:
      error = dwg_encode_POLYLINE_MESH (dat, obj);
      break;
    case DWG_TYPE_SOLID:
      error = dwg_encode_SOLID (dat, obj);
      break;
    case DWG_TYPE_TRACE:
      error = dwg_encode_TRACE (dat, obj);
      break;
    case DWG_TYPE_SHAPE:
      error = dwg_encode_SHAPE (dat, obj);
      break;
    case DWG_TYPE_VIEWPORT:
      error = dwg_encode_VIEWPORT (dat, obj);
      break;
    case DWG_TYPE_ELLIPSE:
      error = dwg_encode_ELLIPSE (dat, obj);
      break;
    case DWG_TYPE_SPLINE:
      error = dwg_encode_SPLINE (dat, obj);
      break;
    case DWG_TYPE_REGION:
      error = dwg_encode_REGION (dat, obj);
      break;
    case DWG_TYPE__3DSOLID:
      error = dwg_encode__3DSOLID (dat, obj);
      break;
    case DWG_TYPE_BODY:
      error = dwg_encode_BODY (dat, obj);
      break;
    case DWG_TYPE_RAY:
      error = dwg_encode_RAY (dat, obj);
      break;
    case DWG_TYPE_XLINE:
      error = dwg_encode_XLINE (dat, obj);
      break;
    case DWG_TYPE_DICTIONARY:
      error = dwg_encode_DICTIONARY (dat, obj);
      break;
    case DWG_TYPE_MTEXT:
      error = dwg_encode_MTEXT (dat, obj);
      break;
    case DWG_TYPE_LEADER:
      error = dwg_encode_LEADER (dat, obj);
      break;
    case DWG_TYPE_TOLERANCE:
      error = dwg_encode_TOLERANCE (dat, obj);
      break;
    case DWG_TYPE_MLINE:
      error = dwg_encode_MLINE (dat, obj);
      break;
    case DWG_TYPE_BLOCK_CONTROL:
      error = dwg_encode_BLOCK_CONTROL (dat, obj);
      break;
    case DWG_TYPE_BLOCK_HEADER:
      error = dwg_encode_BLOCK_HEADER (dat, obj);
      break;
    case DWG_TYPE_LAYER_CONTROL:
      error = dwg_encode_LAYER_CONTROL (dat, obj);
      break;
    case DWG_TYPE_LAYER:
      error = dwg_encode_LAYER (dat, obj);
      break;
    case DWG_TYPE_STYLE_CONTROL:
      error = dwg_encode_STYLE_CONTROL (dat, obj);
      break;
    case DWG_TYPE_STYLE:
      error = dwg_encode_STYLE (dat, obj);
      break;
    case DWG_TYPE_LTYPE_CONTROL:
      error = dwg_encode_LTYPE_CONTROL (dat, obj);
      break;
    case DWG_TYPE_LTYPE:
      error = dwg_encode_LTYPE (dat, obj);
      break;
    case DWG_TYPE_VIEW_CONTROL:
      error = dwg_encode_VIEW_CONTROL (dat, obj);
      break;
    case DWG_TYPE_VIEW:
      error = dwg_encode_VIEW (dat, obj);
      break;
    case DWG_TYPE_UCS_CONTROL:
      error = dwg_encode_UCS_CONTROL (dat, obj);
      break;
    case DWG_TYPE_UCS:
      error = dwg_encode_UCS (dat, obj);
      break;
    case DWG_TYPE_VPORT_CONTROL:
      error = dwg_encode_VPORT_CONTROL (dat, obj);
      break;
    case DWG_TYPE_VPORT:
      error = dwg_encode_VPORT (dat, obj);
      break;
    case DWG_TYPE_APPID_CONTROL:
      error = dwg_encode_APPID_CONTROL (dat, obj);
      break;
    case DWG_TYPE_APPID:
      error = dwg_encode_APPID (dat, obj);
      break;
    case DWG_TYPE_DIMSTYLE_CONTROL:
      error = dwg_encode_DIMSTYLE_CONTROL (dat, obj);
      break;
    case DWG_TYPE_DIMSTYLE:
      error = dwg_encode_DIMSTYLE (dat, obj);
      break;
    case DWG_TYPE_VPORT_ENTITY_CONTROL:
      error = dwg_encode_VPORT_ENTITY_CONTROL (dat, obj);
      break;
    case DWG_TYPE_VPORT_ENTITY_HEADER:
      error = dwg_encode_VPORT_ENTITY_HEADER (dat, obj);
      break;
    case DWG_TYPE_GROUP:
      error = dwg_encode_GROUP (dat, obj);
      break;
    case DWG_TYPE_MLINESTYLE:
      error = dwg_encode_MLINESTYLE (dat, obj);
      (void)dwg_encode_get_class (obj->parent, obj);
      break;
    case DWG_TYPE_OLE2FRAME:
      error = dwg_encode_OLE2FRAME (dat, obj);
      (void)dwg_encode_get_class (obj->parent, obj);
      break;
    case DWG_TYPE_DUMMY:
      error = dwg_encode_DUMMY (dat, obj);
      break;
    case DWG_TYPE_LONG_TRANSACTION:
      error = dwg_encode_LONG_TRANSACTION (dat, obj);
      break;
    case DWG_TYPE_LWPOLYLINE:
      error = dwg_encode_LWPOLYLINE (dat, obj);
      (void)dwg_encode_get_class (obj->parent, obj);
      break;
    case DWG_TYPE_HATCH:
      error = dwg_encode_HATCH (dat, obj);
      (void)dwg_encode_get_class (obj->parent, obj);
      break;
    case DWG_TYPE_XRECORD:
      error = dwg_encode_XRECORD (dat, obj);
      (void)dwg_encode_get_class (obj->parent, obj);
      break;
    case DWG_TYPE_PLACEHOLDER:
      error = dwg_encode_PLACEHOLDER (dat, obj);
      (void)dwg_encode_get_class (obj->parent, obj);
      break;
    case DWG_TYPE_OLEFRAME:
      error = dwg_encode_OLEFRAME (dat, obj);
      (void)dwg_encode_get_class (obj->parent, obj);
      break;
    case DWG_TYPE_VBA_PROJECT:
      LOG_ERROR ("Unhandled Object VBA_PROJECT. Has its own section");
      // dwg_encode_VBA_PROJECT(dat, obj);
      break;
    case DWG_TYPE_LAYOUT:
      error |= dwg_encode_LAYOUT (dat, obj);
      (void)dwg_encode_get_class (obj->parent, obj);
      break;
    case DWG_TYPE_PROXY_ENTITY:
      error = dwg_encode_PROXY_ENTITY (dat, obj);
      break;
    case DWG_TYPE_PROXY_OBJECT:
      error = dwg_encode_PROXY_OBJECT (dat, obj);
      break;
    default:
      if (obj->type == obj->parent->layout_type
          && obj->fixedtype == DWG_TYPE_LAYOUT)
        {
          error = dwg_encode_LAYOUT (dat, obj);
          (void)dwg_encode_get_class (obj->parent, obj);
        }
      else if ((error = dwg_encode_variable_type (obj->parent, dat, obj))
               & DWG_ERR_UNHANDLEDCLASS)
        {
          Dwg_Data *dwg = obj->parent;
          int is_entity;
          int i = obj->type - 500;
          Dwg_Class *klass = dwg_encode_get_class (dwg, obj);

          assert (address);
          dat->byte = address; // restart and write into the UNKNOWN_OBJ object
          dat->bit = 0;
          bit_write_MS (dat, obj->size); // unknown blobs have a known size
          if (dat->version >= R_2010)
            {
              bit_write_UMC (dat, obj->handlestream_size);
              bit_write_BOT (dat, obj->type);
            }
          else
            bit_write_BS (dat, obj->type);

          if (klass && obj->supertype == DWG_SUPERTYPE_UNKNOWN)
            is_entity = dwg_class_is_entity (klass);
          else
            is_entity = obj->supertype == DWG_SUPERTYPE_ENTITY;
          // properly dwg_decode_object/_entity for eed, reactors, xdic
          if (klass && !is_entity)
            error = dwg_encode_UNKNOWN_OBJ (dat, obj);
          else if (klass)
            error = dwg_encode_UNKNOWN_ENT (dat, obj);
          else // not a class
            {
              LOG_WARN ("Unknown object, skipping eed/reactors/xdic");
              error = DWG_ERR_UNHANDLEDCLASS;
              SINCE (R_2000)
              {
                bit_write_RL (dat, obj->bitsize);
                LOG_INFO ("bitsize: " FORMAT_RL " [RL] (@%lu.%u)\n", obj->bitsize,
                          dat->byte - 4, dat->bit);
              }
              bit_write_H (dat, &obj->handle);
              LOG_INFO ("handle: " FORMAT_H " [H 5]\n", ARGS_H (obj->handle));
              // write obj->size bytes, excl. bitsize and handle.
              // overshoot the bitsize and handle size.
              bit_write_TF (dat, obj->tio.unknown, obj->size);
            }
        }
    }

  /* DXF: patchup size and bitsize */
  if (!obj->size)
    {
      BITCODE_BL pos = bit_position (dat);
      assert (address);
      obj->size = dat->byte - address - 2; // excludes the CRC
      if (dat->bit)
        obj->size++;
      //assert (obj->bitsize); // on errors
      if (!obj->bitsize)
        {
          LOG_TRACE ("-bitsize calc from address (no handle) @%lu.%u\n",
                     dat->byte, dat->bit);
          obj->bitsize = pos - (obj->address * 8);
        }
      bit_set_position (dat, address * 8);
      if (obj->size > 0x7fff)
        // TODO: with overlarge sizes >0x7fff memmove dat
        LOG_ERROR ("Unhandled size %u > 0x7fff", (unsigned)obj->size);
      bit_write_MS (dat, obj->size);
      LOG_TRACE ("-size: %u [MS] @%lu\n", obj->size, address);
      SINCE (R_2013)
      {
        if (!obj->handlestream_size && obj->bitsize)
          obj->handlestream_size = obj->size * 8 - obj->bitsize;
        bit_write_UMC (dat, obj->handlestream_size);
        LOG_TRACE ("-handlestream_size: %lu [UMC]\n", obj->handlestream_size);
      }
      SINCE (R_2000)
      {
        if (obj->bitsize_pos && obj->bitsize)
          {
            bit_set_position (dat, obj->bitsize_pos);
            bit_write_RL (dat, obj->bitsize);
            LOG_TRACE ("-bitsize: %u [RL] @%lu.%lu\n", obj->bitsize,
                       obj->bitsize_pos / 8, obj->bitsize_pos % 8);
          }
      }
      bit_set_position (dat, pos);
    }

  /*
   if (obj->supertype != DWG_SUPERTYPE_UNKNOWN)
   {
     fprintf (stderr, "Begin address:\t%10lu\n", address);
     fprintf (stderr, "Last address:\t%10lu\tSize: %10lu\n", dat->byte,
   obj->size); fprintf (stderr, "End address:\t%10lu (calculated)\n", address +
   2 + obj->size);
   }
   */

  /* Now 1 padding bits until next byte, and then a RS CRC */
  if (dat->bit)
    LOG_TRACE ("padding: +%d [*B]\n", 8 - dat->bit)
  while (dat->bit)
    bit_write_B (dat, 1);
  end_address = obj->address + obj->size;
  if (end_address != dat->byte)
    {
      if (obj->size)
        LOG_WARN ("Wrong object size: %lu + %u = %lu != %lu: %ld off", obj->address,
                  obj->size, end_address, dat->byte, (long)(end_address - dat->byte));
      dat->byte = end_address;
    }
  assert (!dat->bit);
  bit_write_CRC (dat, address, 0xC0C1);
  return error;
}