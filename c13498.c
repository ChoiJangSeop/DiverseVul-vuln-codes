static int udf_translate_to_linux(uint8_t *newName, uint8_t *udfName,
				  int udfLen, uint8_t *fidName,
				  int fidNameLen)
{
	int index, newIndex = 0, needsCRC = 0;
	int extIndex = 0, newExtIndex = 0, hasExt = 0;
	unsigned short valueCRC;
	uint8_t curr;

	if (udfName[0] == '.' &&
	    (udfLen == 1 || (udfLen == 2 && udfName[1] == '.'))) {
		needsCRC = 1;
		newIndex = udfLen;
		memcpy(newName, udfName, udfLen);
	} else {
		for (index = 0; index < udfLen; index++) {
			curr = udfName[index];
			if (curr == '/' || curr == 0) {
				needsCRC = 1;
				curr = ILLEGAL_CHAR_MARK;
				while (index + 1 < udfLen &&
						(udfName[index + 1] == '/' ||
						 udfName[index + 1] == 0))
					index++;
			}
			if (curr == EXT_MARK &&
					(udfLen - index - 1) <= EXT_SIZE) {
				if (udfLen == index + 1)
					hasExt = 0;
				else {
					hasExt = 1;
					extIndex = index;
					newExtIndex = newIndex;
				}
			}
			if (newIndex < 256)
				newName[newIndex++] = curr;
			else
				needsCRC = 1;
		}
	}
	if (needsCRC) {
		uint8_t ext[EXT_SIZE];
		int localExtIndex = 0;

		if (hasExt) {
			int maxFilenameLen;
			for (index = 0;
			     index < EXT_SIZE && extIndex + index + 1 < udfLen;
			     index++) {
				curr = udfName[extIndex + index + 1];

				if (curr == '/' || curr == 0) {
					needsCRC = 1;
					curr = ILLEGAL_CHAR_MARK;
					while (extIndex + index + 2 < udfLen &&
					      (index + 1 < EXT_SIZE &&
						(udfName[extIndex + index + 2] == '/' ||
						 udfName[extIndex + index + 2] == 0)))
						index++;
				}
				ext[localExtIndex++] = curr;
			}
			maxFilenameLen = 250 - localExtIndex;
			if (newIndex > maxFilenameLen)
				newIndex = maxFilenameLen;
			else
				newIndex = newExtIndex;
		} else if (newIndex > 250)
			newIndex = 250;
		newName[newIndex++] = CRC_MARK;
		valueCRC = crc_itu_t(0, fidName, fidNameLen);
		newName[newIndex++] = hex_asc_upper_hi(valueCRC >> 8);
		newName[newIndex++] = hex_asc_upper_lo(valueCRC >> 8);
		newName[newIndex++] = hex_asc_upper_hi(valueCRC);
		newName[newIndex++] = hex_asc_upper_lo(valueCRC);

		if (hasExt) {
			newName[newIndex++] = EXT_MARK;
			for (index = 0; index < localExtIndex; index++)
				newName[newIndex++] = ext[index];
		}
	}

	return newIndex;
}