dwg_free_object (Dwg_Object *obj)
{
  int error = 0;
  long unsigned int j;
  Dwg_Data *dwg;
  Bit_Chain *dat = &pdat;

  if (obj && obj->parent)
    {
      dwg = obj->parent;
      dat->version = dwg->header.version;
    }
  else
    return;
  if (obj->type == DWG_TYPE_FREED || obj->tio.object == NULL)
    return;
  dat->from_version = dat->version;
  if (obj->supertype == DWG_SUPERTYPE_UNKNOWN)
    goto unhandled;

  switch (obj->type)
    {
    case DWG_TYPE_TEXT:
      dwg_free_TEXT (dat, obj);
      break;
    case DWG_TYPE_ATTRIB:
      dwg_free_ATTRIB (dat, obj);
      break;
    case DWG_TYPE_ATTDEF:
      dwg_free_ATTDEF (dat, obj);
      break;
    case DWG_TYPE_BLOCK:
      dwg_free_BLOCK (dat, obj);
      break;
    case DWG_TYPE_ENDBLK:
      dwg_free_ENDBLK (dat, obj);
      break;
    case DWG_TYPE_SEQEND:
      dwg_free_SEQEND (dat, obj);
      break;
    case DWG_TYPE_INSERT:
      dwg_free_INSERT (dat, obj);
      break;
    case DWG_TYPE_MINSERT:
      dwg_free_MINSERT (dat, obj);
      break;
    case DWG_TYPE_VERTEX_2D:
      dwg_free_VERTEX_2D (dat, obj);
      break;
    case DWG_TYPE_VERTEX_3D:
      dwg_free_VERTEX_3D (dat, obj);
      break;
    case DWG_TYPE_VERTEX_MESH:
      dwg_free_VERTEX_MESH (dat, obj);
      break;
    case DWG_TYPE_VERTEX_PFACE:
      dwg_free_VERTEX_PFACE (dat, obj);
      break;
    case DWG_TYPE_VERTEX_PFACE_FACE:
      dwg_free_VERTEX_PFACE_FACE (dat, obj);
      break;
    case DWG_TYPE_POLYLINE_2D:
      dwg_free_POLYLINE_2D (dat, obj);
      break;
    case DWG_TYPE_POLYLINE_3D:
      dwg_free_POLYLINE_3D (dat, obj);
      break;
    case DWG_TYPE_ARC:
      dwg_free_ARC (dat, obj);
      break;
    case DWG_TYPE_CIRCLE:
      dwg_free_CIRCLE (dat, obj);
      break;
    case DWG_TYPE_LINE:
      dwg_free_LINE (dat, obj);
      break;
    case DWG_TYPE_DIMENSION_ORDINATE:
      dwg_free_DIMENSION_ORDINATE (dat, obj);
      break;
    case DWG_TYPE_DIMENSION_LINEAR:
      dwg_free_DIMENSION_LINEAR (dat, obj);
      break;
    case DWG_TYPE_DIMENSION_ALIGNED:
      dwg_free_DIMENSION_ALIGNED (dat, obj);
      break;
    case DWG_TYPE_DIMENSION_ANG3PT:
      dwg_free_DIMENSION_ANG3PT (dat, obj);
      break;
    case DWG_TYPE_DIMENSION_ANG2LN:
      dwg_free_DIMENSION_ANG2LN (dat, obj);
      break;
    case DWG_TYPE_DIMENSION_RADIUS:
      dwg_free_DIMENSION_RADIUS (dat, obj);
      break;
    case DWG_TYPE_DIMENSION_DIAMETER:
      dwg_free_DIMENSION_DIAMETER (dat, obj);
      break;
    case DWG_TYPE_POINT:
      dwg_free_POINT (dat, obj);
      break;
    case DWG_TYPE__3DFACE:
      dwg_free__3DFACE (dat, obj);
      break;
    case DWG_TYPE_POLYLINE_PFACE:
      dwg_free_POLYLINE_PFACE (dat, obj);
      break;
    case DWG_TYPE_POLYLINE_MESH:
      dwg_free_POLYLINE_MESH (dat, obj);
      break;
    case DWG_TYPE_SOLID:
      dwg_free_SOLID (dat, obj);
      break;
    case DWG_TYPE_TRACE:
      dwg_free_TRACE (dat, obj);
      break;
    case DWG_TYPE_SHAPE:
      dwg_free_SHAPE (dat, obj);
      break;
    case DWG_TYPE_VIEWPORT:
      dwg_free_VIEWPORT (dat, obj);
      break;
    case DWG_TYPE_ELLIPSE:
      dwg_free_ELLIPSE (dat, obj);
      break;
    case DWG_TYPE_SPLINE:
      dwg_free_SPLINE (dat, obj);
      break;
    case DWG_TYPE_REGION:
      dwg_free_REGION (dat, obj);
      break;
    case DWG_TYPE__3DSOLID:
      dwg_free__3DSOLID (dat, obj);
      break; /* Check the type of the object */
    case DWG_TYPE_BODY:
      dwg_free_BODY (dat, obj);
      break;
    case DWG_TYPE_RAY:
      dwg_free_RAY (dat, obj);
      break;
    case DWG_TYPE_XLINE:
      dwg_free_XLINE (dat, obj);
      break;
    case DWG_TYPE_DICTIONARY:
      dwg_free_DICTIONARY (dat, obj);
      break;
    case DWG_TYPE_MTEXT:
      dwg_free_MTEXT (dat, obj);
      break;
    case DWG_TYPE_LEADER:
      dwg_free_LEADER (dat, obj);
      break;
    case DWG_TYPE_TOLERANCE:
      dwg_free_TOLERANCE (dat, obj);
      break;
    case DWG_TYPE_MLINE:
      dwg_free_MLINE (dat, obj);
      break;
    case DWG_TYPE_BLOCK_CONTROL:
      dwg_free_BLOCK_CONTROL (dat, obj);
      break;
    case DWG_TYPE_BLOCK_HEADER:
      dwg_free_BLOCK_HEADER (dat, obj);
      break;
    case DWG_TYPE_LAYER_CONTROL:
      dwg_free_LAYER_CONTROL (dat, obj);
      break;
    case DWG_TYPE_LAYER:
      dwg_free_LAYER (dat, obj);
      break;
    case DWG_TYPE_STYLE_CONTROL:
      dwg_free_STYLE_CONTROL (dat, obj);
      break;
    case DWG_TYPE_STYLE:
      dwg_free_STYLE (dat, obj);
      break;
    case DWG_TYPE_LTYPE_CONTROL:
      dwg_free_LTYPE_CONTROL (dat, obj);
      break;
    case DWG_TYPE_LTYPE:
      dwg_free_LTYPE (dat, obj);
      break;
    case DWG_TYPE_VIEW_CONTROL:
      dwg_free_VIEW_CONTROL (dat, obj);
      break;
    case DWG_TYPE_VIEW:
      dwg_free_VIEW (dat, obj);
      break;
    case DWG_TYPE_UCS_CONTROL:
      dwg_free_UCS_CONTROL (dat, obj);
      break;
    case DWG_TYPE_UCS:
      dwg_free_UCS (dat, obj);
      break;
    case DWG_TYPE_VPORT_CONTROL:
      dwg_free_VPORT_CONTROL (dat, obj);
      break;
    case DWG_TYPE_VPORT:
      dwg_free_VPORT (dat, obj);
      break;
    case DWG_TYPE_APPID_CONTROL:
      dwg_free_APPID_CONTROL (dat, obj);
      break;
    case DWG_TYPE_APPID:
      dwg_free_APPID (dat, obj);
      break;
    case DWG_TYPE_DIMSTYLE_CONTROL:
      dwg_free_DIMSTYLE_CONTROL (dat, obj);
      break;
    case DWG_TYPE_DIMSTYLE:
      dwg_free_DIMSTYLE (dat, obj);
      break;
    case DWG_TYPE_VPORT_ENTITY_CONTROL:
      dwg_free_VPORT_ENTITY_CONTROL (dat, obj);
      break;
    case DWG_TYPE_VPORT_ENTITY_HEADER:
      dwg_free_VPORT_ENTITY_HEADER (dat, obj);
      break;
    case DWG_TYPE_GROUP:
      dwg_free_GROUP (dat, obj);
      break;
    case DWG_TYPE_MLINESTYLE:
      dwg_free_MLINESTYLE (dat, obj);
      break;
    case DWG_TYPE_OLE2FRAME:
      dwg_free_OLE2FRAME (dat, obj);
      break;
    case DWG_TYPE_DUMMY:
      dwg_free_DUMMY (dat, obj);
      break;
    case DWG_TYPE_LONG_TRANSACTION:
      dwg_free_LONG_TRANSACTION (dat, obj);
      break;
    case DWG_TYPE_LWPOLYLINE:
      dwg_free_LWPOLYLINE (dat, obj);
      break;
    case DWG_TYPE_HATCH:
      dwg_free_HATCH (dat, obj);
      break;
    case DWG_TYPE_XRECORD:
      dwg_free_XRECORD (dat, obj);
      break;
    case DWG_TYPE_PLACEHOLDER:
      dwg_free_PLACEHOLDER (dat, obj);
      break;
    case DWG_TYPE_OLEFRAME:
      dwg_free_OLEFRAME (dat, obj);
      break;
#ifdef DEBUG_VBA_PROJECT
    case DWG_TYPE_VBA_PROJECT:
      dwg_free_VBA_PROJECT (dat, obj);
      break;
#endif
    case DWG_TYPE_LAYOUT:
      dwg_free_LAYOUT (dat, obj);
      break;
    case DWG_TYPE_PROXY_ENTITY:
      dwg_free_PROXY_ENTITY (dat, obj);
      break;
    case DWG_TYPE_PROXY_OBJECT:
      dwg_free_PROXY_OBJECT (dat, obj);
      break;
    default:
      if (obj->type == obj->parent->layout_type)
        {
          SINCE (R_13)
          {
            dwg_free_LAYOUT (dat, obj); // XXX avoid double-free, esp. in eed
          }
        }

      else if ((error = dwg_free_variable_type (obj->parent, obj))
               & DWG_ERR_UNHANDLEDCLASS)
        {
          int is_entity;
          int i;
          Dwg_Class *klass;

        unhandled:
          is_entity = 0;
          i = obj->type - 500;
          klass = NULL;

          dwg = obj->parent;
          if (dwg->dwg_class && i >= 0 && i < (int)dwg->num_classes)
            {
              klass = &dwg->dwg_class[i];
              is_entity = klass ? dwg_class_is_entity (klass) : 0;
            }
          // indxf (and later injson) already creates some DEBUGGING classes
          if (obj->fixedtype == DWG_TYPE_TABLE)
            {
              // just the preview, i.e. common. plus some colors: leak
              dwg_free_UNKNOWN_ENT (dat, obj);
            }
          else if (obj->fixedtype == DWG_TYPE_DATATABLE)
            {
              dwg_free_UNKNOWN_OBJ (dat, obj);
            }
          else if (klass && !is_entity)
            {
              dwg_free_UNKNOWN_OBJ (dat, obj);
            }
          else if (klass && is_entity)
            {
              dwg_free_UNKNOWN_ENT (dat, obj);
            }
          else // not a class
            {
              FREE_IF (obj->tio.unknown);
            }
        }
    }
  /* With this importer the dxfname is dynamic, just the name is const */
  if (dwg->opts & DWG_OPTS_INDXF)
    FREE_IF (obj->dxfname);
  obj->type = DWG_TYPE_FREED;
}