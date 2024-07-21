dwg_decode_add_object (Dwg_Data *restrict dwg, Bit_Chain *dat,
                       Bit_Chain *hdl_dat, long unsigned int address)
{
  long unsigned int objpos, restartpos;
  Bit_Chain abs_dat = { NULL };
  unsigned char previous_bit;
  Dwg_Object *restrict obj;
  BITCODE_BL num = dwg->num_objects;
  int error = 0;
  int realloced = 0;

  /* Keep the previous full chain  */
  abs_dat = *dat;

  /* Use the indicated address for the object
   */
  dat->byte = address;
  dat->bit = 0;

  // DEBUG_HERE;
  /*
   * Reserve memory space for objects. A realloc violates all internal
   * pointers.
   */
  realloced = dwg_add_object (dwg);
  if (realloced > 0)
    {
      *dat = abs_dat;
      return realloced; // i.e. DWG_ERR_OUTOFMEM
    }
  obj = &dwg->object[num];
  LOG_INFO ("==========================================\n"
            "Object number: %lu/%lX",
            (unsigned long)num, (unsigned long)num)

  obj->size = bit_read_MS (dat);
  LOG_INFO (", Size: %d [MS]", obj->size)
  SINCE (R_2010)
  {
    /* This is not counted in the object size */
    obj->handlestream_size = bit_read_UMC (dat);
    LOG_INFO (", Hdlsize: " FORMAT_UMC " [UMC] ", obj->handlestream_size);
    obj->bitsize = obj->size * 8 - obj->handlestream_size;
    // TODO boundscheck
  }

  objpos = bit_position (dat); // absolute
  obj->address = dat->byte;

  /* Until here dat is absolute. now restrict it */
  bit_reset_chain (dat);
  if (obj->size > dat->size)
    {
      LOG_ERROR ("\nInvalid object size. Would overflow");
      *dat = abs_dat;
      return DWG_ERR_VALUEOUTOFBOUNDS;
    }
  dat->size = obj->size;

  SINCE (R_2010) { obj->type = bit_read_BOT (dat); }
  else { obj->type = bit_read_BS (dat); }
  LOG_INFO (", Type: %d [%s]\n", obj->type, dat->version >= R_2010 ? "BOT" : "BS");
  restartpos = bit_position (dat); // relative

  /* Check the type of the object
   */
  switch (obj->type)
    {
    case DWG_TYPE_TEXT:
      error = dwg_decode_TEXT (dat, obj);
      break;
    case DWG_TYPE_ATTRIB:
      error = dwg_decode_ATTRIB (dat, obj);
      break;
    case DWG_TYPE_ATTDEF:
      error = dwg_decode_ATTDEF (dat, obj);
      break;
    case DWG_TYPE_BLOCK:
      error = dwg_decode_BLOCK (dat, obj);
      break;
    case DWG_TYPE_ENDBLK:
      error = dwg_decode_ENDBLK (dat, obj);
      break;
    case DWG_TYPE_SEQEND:
      error = dwg_decode_SEQEND (dat, obj);
      if (dat->version >= R_13 && obj->tio.entity->ownerhandle)
        {
          Dwg_Object *restrict owner = dwg_resolve_handle (
              dwg, obj->tio.entity->ownerhandle->absolute_ref);
          if (!owner)
            {
              LOG_WARN ("no SEQEND.ownerhandle")
            }
          else if (owner->fixedtype == DWG_TYPE_INSERT
                   || owner->fixedtype == DWG_TYPE_MINSERT)
            {
              /* SEQEND handle for the owner needed in validate_INSERT */
              hash_set (dwg->object_map, obj->handle.value, (uint32_t)num);
              (void)dwg_validate_INSERT (owner);
            }
          else if (owner->fixedtype == DWG_TYPE_POLYLINE_2D
                   || owner->fixedtype == DWG_TYPE_POLYLINE_3D
                   || owner->fixedtype == DWG_TYPE_POLYLINE_PFACE
                   || owner->fixedtype == DWG_TYPE_POLYLINE_MESH)
            {
              Dwg_Entity_POLYLINE_2D *restrict _obj
                  = owner->tio.entity->tio.POLYLINE_2D;
              if (!_obj->seqend)
                /* SEQEND handle for the owner needed in validate_POLYLINE */
                hash_set (dwg->object_map, obj->handle.value, (uint32_t)num);
              (void)dwg_validate_POLYLINE (owner);
            }
        }
      break;
    case DWG_TYPE_INSERT:
      error = dwg_decode_INSERT (dat, obj);
      break;
    case DWG_TYPE_MINSERT:
      error = dwg_decode_MINSERT (dat, obj);
      break;
    case DWG_TYPE_VERTEX_2D:
      error = dwg_decode_VERTEX_2D (dat, obj);
      break;
    case DWG_TYPE_VERTEX_3D:
      error = dwg_decode_VERTEX_3D (dat, obj);
      break;
    case DWG_TYPE_VERTEX_MESH:
      error = dwg_decode_VERTEX_MESH (dat, obj);
      break;
    case DWG_TYPE_VERTEX_PFACE:
      error = dwg_decode_VERTEX_PFACE (dat, obj);
      break;
    case DWG_TYPE_VERTEX_PFACE_FACE:
      error = dwg_decode_VERTEX_PFACE_FACE (dat, obj);
      break;
    case DWG_TYPE_POLYLINE_2D:
      error = dwg_decode_POLYLINE_2D (dat, obj);
      if (dat->version >= R_2010)
        check_POLYLINE_handles (obj);
      break;
    case DWG_TYPE_POLYLINE_3D:
      error = dwg_decode_POLYLINE_3D (dat, obj);
      if (dat->version >= R_2010)
        check_POLYLINE_handles (obj);
      break;
    case DWG_TYPE_ARC:
      error = dwg_decode_ARC (dat, obj);
      break;
    case DWG_TYPE_CIRCLE:
      error = dwg_decode_CIRCLE (dat, obj);
      break;
    case DWG_TYPE_LINE:
      error = dwg_decode_LINE (dat, obj);
      break;
    case DWG_TYPE_DIMENSION_ORDINATE:
      error = dwg_decode_DIMENSION_ORDINATE (dat, obj);
      break;
    case DWG_TYPE_DIMENSION_LINEAR:
      error = dwg_decode_DIMENSION_LINEAR (dat, obj);
      break;
    case DWG_TYPE_DIMENSION_ALIGNED:
      error = dwg_decode_DIMENSION_ALIGNED (dat, obj);
      break;
    case DWG_TYPE_DIMENSION_ANG3PT:
      error = dwg_decode_DIMENSION_ANG3PT (dat, obj);
      break;
    case DWG_TYPE_DIMENSION_ANG2LN:
      error = dwg_decode_DIMENSION_ANG2LN (dat, obj);
      break;
    case DWG_TYPE_DIMENSION_RADIUS:
      error = dwg_decode_DIMENSION_RADIUS (dat, obj);
      break;
    case DWG_TYPE_DIMENSION_DIAMETER:
      error = dwg_decode_DIMENSION_DIAMETER (dat, obj);
      break;
    case DWG_TYPE_POINT:
      error = dwg_decode_POINT (dat, obj);
      break;
    case DWG_TYPE__3DFACE:
      error = dwg_decode__3DFACE (dat, obj);
      break;
    case DWG_TYPE_POLYLINE_PFACE:
      error = dwg_decode_POLYLINE_PFACE (dat, obj);
      if (dat->version >= R_2010)
        check_POLYLINE_handles (obj);
      break;
    case DWG_TYPE_POLYLINE_MESH:
      error = dwg_decode_POLYLINE_MESH (dat, obj);
      if (dat->version >= R_2010)
        check_POLYLINE_handles (obj);
      break;
    case DWG_TYPE_SOLID:
      error = dwg_decode_SOLID (dat, obj);
      break;
    case DWG_TYPE_TRACE:
      error = dwg_decode_TRACE (dat, obj);
      break;
    case DWG_TYPE_SHAPE:
      error = dwg_decode_SHAPE (dat, obj);
      break;
    case DWG_TYPE_VIEWPORT:
      error = dwg_decode_VIEWPORT (dat, obj);
      break;
    case DWG_TYPE_ELLIPSE:
      error = dwg_decode_ELLIPSE (dat, obj);
      break;
    case DWG_TYPE_SPLINE:
      error = dwg_decode_SPLINE (dat, obj);
      break;
    case DWG_TYPE_REGION:
      error = dwg_decode_REGION (dat, obj);
      break;
    case DWG_TYPE__3DSOLID:
      error = dwg_decode__3DSOLID (dat, obj);
      break;
    case DWG_TYPE_BODY:
      error = dwg_decode_BODY (dat, obj);
      break;
    case DWG_TYPE_RAY:
      error = dwg_decode_RAY (dat, obj);
      break;
    case DWG_TYPE_XLINE:
      error = dwg_decode_XLINE (dat, obj);
      break;
    case DWG_TYPE_DICTIONARY:
      error = dwg_decode_DICTIONARY (dat, obj);
      break;
    case DWG_TYPE_MTEXT:
      error = dwg_decode_MTEXT (dat, obj);
      break;
    case DWG_TYPE_LEADER:
      error = dwg_decode_LEADER (dat, obj);
      break;
    case DWG_TYPE_TOLERANCE:
      error = dwg_decode_TOLERANCE (dat, obj);
      break;
    case DWG_TYPE_MLINE:
      error = dwg_decode_MLINE (dat, obj);
      break;
    case DWG_TYPE_BLOCK_CONTROL:
      error = dwg_decode_BLOCK_CONTROL (dat, obj);
      if (!error && obj->tio.object->tio.BLOCK_CONTROL)
        {
          obj->tio.object->tio.BLOCK_CONTROL->objid = num;
          if (!dwg->block_control.parent) // only once
            dwg->block_control = *obj->tio.object->tio.BLOCK_CONTROL;
          else
            LOG_WARN ("Second BLOCK_CONTROL object ignored");
        }
      break;
    case DWG_TYPE_BLOCK_HEADER:
      error = dwg_decode_BLOCK_HEADER (dat, obj);
      /*
       * We cannot cache dwg->*space_block here as dwg->objects might get
       * realloc'ed. See dwg_model_space_object() and dwg_paper_space_object()
       * instead.
       */
      break;
    case DWG_TYPE_LAYER_CONTROL:
      error = dwg_decode_LAYER_CONTROL (dat, obj);
      if (!error && obj->tio.object->tio.LAYER_CONTROL)
        {
          obj->tio.object->tio.LAYER_CONTROL->objid = num;
          dwg->layer_control = *obj->tio.object->tio.LAYER_CONTROL;
        }
      break;
    case DWG_TYPE_LAYER:
      error = dwg_decode_LAYER (dat, obj);
      break;
    case DWG_TYPE_STYLE_CONTROL:
      error = dwg_decode_STYLE_CONTROL (dat, obj);
      if (!error && obj->tio.object->tio.STYLE_CONTROL)
        {
          obj->tio.object->tio.STYLE_CONTROL->objid = num;
          dwg->style_control = *obj->tio.object->tio.STYLE_CONTROL;
        }
      break;
    case DWG_TYPE_STYLE:
      error = dwg_decode_STYLE (dat, obj);
      break;
    case DWG_TYPE_LTYPE_CONTROL:
      error = dwg_decode_LTYPE_CONTROL (dat, obj);
      if (!error && obj->tio.object->tio.LTYPE_CONTROL)
        {
          obj->tio.object->tio.LTYPE_CONTROL->objid = num;
          dwg->ltype_control = *obj->tio.object->tio.LTYPE_CONTROL;
        }
      break;
    case DWG_TYPE_LTYPE:
      error = dwg_decode_LTYPE (dat, obj);
      break;
    case DWG_TYPE_VIEW_CONTROL:
      error = dwg_decode_VIEW_CONTROL (dat, obj);
      if (!error && obj->tio.object->tio.VIEW_CONTROL)
        {
          obj->tio.object->tio.VIEW_CONTROL->objid = num;
          dwg->view_control = *obj->tio.object->tio.VIEW_CONTROL;
        }
      break;
    case DWG_TYPE_VIEW:
      error = dwg_decode_VIEW (dat, obj);
      break;
    case DWG_TYPE_UCS_CONTROL:
      error = dwg_decode_UCS_CONTROL (dat, obj);
      if (!error && obj->tio.object->tio.UCS_CONTROL)
        {
          obj->tio.object->tio.UCS_CONTROL->objid = num;
          dwg->ucs_control = *obj->tio.object->tio.UCS_CONTROL;
        }
      break;
    case DWG_TYPE_UCS:
      error = dwg_decode_UCS (dat, obj);
      break;
    case DWG_TYPE_VPORT_CONTROL:
      error = dwg_decode_VPORT_CONTROL (dat, obj);
      if (!error && obj->tio.object->tio.VPORT_CONTROL)
        {
          obj->tio.object->tio.VPORT_CONTROL->objid = num;
          dwg->vport_control = *obj->tio.object->tio.VPORT_CONTROL;
        }
      break;
    case DWG_TYPE_VPORT:
      error = dwg_decode_VPORT (dat, obj);
      break;
    case DWG_TYPE_APPID_CONTROL:
      error = dwg_decode_APPID_CONTROL (dat, obj);
      if (!error && obj->tio.object->tio.APPID_CONTROL)
        {
          obj->tio.object->tio.APPID_CONTROL->objid = num;
          dwg->appid_control = *obj->tio.object->tio.APPID_CONTROL;
        }
      break;
    case DWG_TYPE_APPID:
      error = dwg_decode_APPID (dat, obj);
      break;
    case DWG_TYPE_DIMSTYLE_CONTROL:
      error = dwg_decode_DIMSTYLE_CONTROL (dat, obj);
      if (!error && obj->tio.object->tio.DIMSTYLE_CONTROL)
        {
          obj->tio.object->tio.DIMSTYLE_CONTROL->objid = num;
          dwg->dimstyle_control = *obj->tio.object->tio.DIMSTYLE_CONTROL;
        }
      break;
    case DWG_TYPE_DIMSTYLE:
      error = dwg_decode_DIMSTYLE (dat, obj);
      break;
    case DWG_TYPE_VPORT_ENTITY_CONTROL:
      error = dwg_decode_VPORT_ENTITY_CONTROL (dat, obj);
      if (!error && obj->tio.object->tio.VPORT_ENTITY_CONTROL)
        {
          obj->tio.object->tio.VPORT_ENTITY_CONTROL->objid = num;
          dwg->vport_entity_control
              = *obj->tio.object->tio.VPORT_ENTITY_CONTROL;
        }
      break;
    case DWG_TYPE_VPORT_ENTITY_HEADER:
      error = dwg_decode_VPORT_ENTITY_HEADER (dat, obj);
      break;
    case DWG_TYPE_GROUP:
      error = dwg_decode_GROUP (dat, obj);
      break;
    case DWG_TYPE_MLINESTYLE:
      error = dwg_decode_MLINESTYLE (dat, obj);
      break;
    case DWG_TYPE_OLE2FRAME:
      error = dwg_decode_OLE2FRAME (dat, obj);
      break;
    case DWG_TYPE_DUMMY:
      error = dwg_decode_DUMMY (dat, obj);
      break;
    case DWG_TYPE_LONG_TRANSACTION:
      error = dwg_decode_LONG_TRANSACTION (dat, obj);
      break;
    case DWG_TYPE_LWPOLYLINE:
      error = dwg_decode_LWPOLYLINE (dat, obj);
      break;
    case DWG_TYPE_HATCH:
      error = dwg_decode_HATCH (dat, obj);
      break;
    case DWG_TYPE_XRECORD:
      error = dwg_decode_XRECORD (dat, obj);
      break;
    case DWG_TYPE_PLACEHOLDER:
      error = dwg_decode_PLACEHOLDER (dat, obj);
      break;
    case DWG_TYPE_OLEFRAME:
      error = dwg_decode_OLEFRAME (dat, obj);
      break;
    case DWG_TYPE_VBA_PROJECT:
      LOG_ERROR ("Unhandled Object VBA_PROJECT. Has its own section");
      // dwg_decode_VBA_PROJECT(dat, obj);
      error = DWG_ERR_UNHANDLEDCLASS;
      break;
    case DWG_TYPE_LAYOUT:
      error = dwg_decode_LAYOUT (dat, obj);
      break;
    case DWG_TYPE_PROXY_ENTITY:
      error = dwg_decode_PROXY_ENTITY (dat, obj);
      break;
    case DWG_TYPE_PROXY_OBJECT:
      error = dwg_decode_PROXY_OBJECT (dat, obj);
      break;
    default:
      if (obj->type == dwg->layout_type)
        error = dwg_decode_LAYOUT (dat, obj);
      /* > 500 */
      else if ((error = dwg_decode_variable_type (dwg, dat, hdl_dat, obj))
               & DWG_ERR_UNHANDLEDCLASS)
        {
          int is_entity = 0;
          int i = obj->type - 500;
          Dwg_Class *klass = NULL;

          /* restart and read into the UNKNOWN_OBJ object */
          /* the relative offset from type after common_entity_data */
          // obj->common_size = bit_position(dat) - restartpos;
          // LOG_HANDLE("common_size: %lu\n", obj->common_size); // needed for
          // unknown
          bit_set_position (dat, restartpos);

          // obj->unknown_off = obj->unknown_pos - restartpos;
          // LOG_TRACE("Unknown pos %lu, offset %lu\n", obj->unknown_pos,
          // obj->unknown_off);

          if (i >= 0 && i < (int)dwg->num_classes)
            {
              klass = &dwg->dwg_class[i];
              is_entity = dwg_class_is_entity (klass);
            }
          else
            {
              if (i < 0)
                {
                  LOG_ERROR ("Invalid class index %d <0", i);
                }
              else
                {
                  LOG_ERROR ("Invalid class index %d >%d", i,
                             (int)dwg->num_classes);
                }
              obj->supertype = DWG_SUPERTYPE_UNKNOWN;
              obj->type = 0;
              *dat = abs_dat;
              return error | DWG_ERR_VALUEOUTOFBOUNDS;
            }
          // properly dwg_decode_object/_entity for eed, reactors, xdic
          if (klass && !is_entity)
            {
              int err = dwg_decode_UNKNOWN_OBJ (dat, obj);
              error |= err;
              obj->supertype = DWG_SUPERTYPE_UNKNOWN;
              if (!dat)
                return error;
              if (err >= DWG_ERR_CRITICAL)
                *dat = abs_dat;
            }
          else if (klass) // is_entity
            {
              int err;
#if 0 && !defined(IS_RELEASE)
              if (strEQc(klass->dxfname, "MULTILEADER")) { //debug CED
                char *mleader = bit_read_TF(dat, obj->size);
                LOG_INSANE_TF(mleader, (int)obj->size)
                bit_set_position(dat, restartpos);
                free (mleader);
              }
#endif
              err = dwg_decode_UNKNOWN_ENT (dat, obj);
              error |= err;
              obj->supertype = DWG_SUPERTYPE_UNKNOWN;
              if (!dat)
                return error;
              if (err >= DWG_ERR_CRITICAL)
                *dat = abs_dat;
            }
          else // not a class
            {
              LOG_WARN ("Unknown object, skipping eed/reactors/xdic");
              SINCE (R_2000)
              {
                obj->bitsize = bit_read_RL (dat);
                LOG_TRACE ("bitsize: " FORMAT_RL " [RL] @%lu.%u\n",
                           obj->bitsize, dat->byte-2, dat->bit);
                if (obj->bitsize > obj->size * 8)
                  {
                    LOG_ERROR ("Invalid bitsize " FORMAT_RL " => " FORMAT_RL,
                               obj->bitsize, obj->size * 8);
                    obj->bitsize = obj->size * 8;
                    error |= DWG_ERR_VALUEOUTOFBOUNDS;
                  }
              }
              if (!bit_read_H (dat, &obj->handle))
                {
                  LOG_TRACE ("handle: " FORMAT_H " [H 5]\n",
                             ARGS_H (obj->handle));
                }
              restartpos = dat->byte;
              obj->supertype = DWG_SUPERTYPE_UNKNOWN;
              obj->tio.unknown = bit_read_TF (dat, obj->size);
              dat->byte = restartpos;
            }
        }
    }

  if (obj->handle.value)
    { // empty only with UNKNOWN
      LOG_HANDLE (" object_map{%lX} = %lu\n", obj->handle.value,
                  (unsigned long)num);
      hash_set (dwg->object_map, obj->handle.value, (uint32_t)num);
    }

  if (dat->byte > 8 * dat->size)
    {
      LOG_ERROR ("Invalid object address (overflow): %lu > %lu", dat->byte,
                 8 * dat->size);
      *dat = abs_dat;
      return error | DWG_ERR_INVALIDDWG;
    }

  /* Restore the old absolute chain.
     CRC needs to be calculated from address, which is before our 0 position.
   */
  restartpos = bit_position (dat);
  *dat = abs_dat;
  bit_set_position (dat, objpos + restartpos);

  /* Now 1 padding bits until next byte, and then a RS CRC */
  if (dat->bit)
    {
      unsigned char r = 8 - dat->bit;
      LOG_HANDLE (" padding: %X/%X (%d bits)\n", dat->chain[dat->byte],
                  dat->chain[dat->byte] & ((1 << r) - 1), r);
      bit_advance_position (dat, r);
    }
  bit_set_position (dat, (obj->address + obj->size) * 8 - 2);
  if (!bit_check_CRC (dat, address, 0xC0C1))
    error |= DWG_ERR_WRONGCRC;

  /* Reset to previous addresses for return */
  *dat = abs_dat;

  return realloced ? -1 : error; // re-alloced or not
}