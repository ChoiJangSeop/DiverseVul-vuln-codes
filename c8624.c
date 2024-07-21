ipmi_sdr_print_name_from_rawentry(uint16_t id,
                                  uint8_t type, uint8_t *raw)
{
   union {
      struct sdr_record_full_sensor *full;
      struct sdr_record_compact_sensor *compact;
      struct sdr_record_eventonly_sensor *eventonly;
      struct sdr_record_generic_locator *genloc;
      struct sdr_record_fru_locator *fruloc;
      struct sdr_record_mc_locator *mcloc;
      struct sdr_record_entity_assoc *entassoc;
      struct sdr_record_oem *oem;
   } record;

   int rc =0;
   char desc[17];
   memset(desc, ' ', sizeof (desc));

   switch ( type) {
      case SDR_RECORD_TYPE_FULL_SENSOR:
      record.full = (struct sdr_record_full_sensor *) raw;
      snprintf(desc, (record.full->id_code & 0x1f) +1, "%s",
               (const char *)record.full->id_string);
      break;
      case SDR_RECORD_TYPE_COMPACT_SENSOR:
      record.compact = (struct sdr_record_compact_sensor *) raw	;
      snprintf(desc, (record.compact->id_code & 0x1f)  +1, "%s",
               (const char *)record.compact->id_string);
      break;
      case SDR_RECORD_TYPE_EVENTONLY_SENSOR:
      record.eventonly  = (struct sdr_record_eventonly_sensor *) raw ;
      snprintf(desc, (record.eventonly->id_code & 0x1f)  +1, "%s",
               (const char *)record.eventonly->id_string);
      break;            
      case SDR_RECORD_TYPE_MC_DEVICE_LOCATOR:
      record.mcloc  = (struct sdr_record_mc_locator *) raw ;
      snprintf(desc, (record.mcloc->id_code & 0x1f)  +1, "%s",
               (const char *)record.mcloc->id_string);		
      break;
      default:
      rc = -1;
      break;
   }   

      lprintf(LOG_INFO, "ID: 0x%04x , NAME: %-16s", id, desc);
   return rc;
}