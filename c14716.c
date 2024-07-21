void svhandler_flash_erase_sector(void) {
  uint32_t sector = _param_1;

  // Do not allow firmware to erase bootstrap or bootloader sectors.
  if ((sector == FLASH_BOOTSTRAP_SECTOR) ||
      (sector >= FLASH_BOOT_SECTOR_FIRST && sector <= FLASH_BOOT_SECTOR_LAST)) {
    return;
  }

  // Unlock flash.
  flash_clear_status_flags();
  flash_unlock();

  // Erase the sector.
  flash_erase_sector(sector, FLASH_CR_PROGRAM_X32);

  // Return flash status.
  _param_1 = !!flash_chk_status();
  _param_2 = 0;
  _param_3 = 0;

  // Wait for any write operation to complete.
  flash_wait_for_last_operation();

  // Disable writes to flash.
  FLASH_CR &= ~FLASH_CR_PG;

  // lock flash register
  FLASH_CR |= FLASH_CR_LOCK;
}