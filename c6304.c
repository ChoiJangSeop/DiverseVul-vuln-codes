includeFile(FileInfo *nested, CharsString *includedFile,
		CharacterClass **characterClasses,
		TranslationTableCharacterAttributes *characterClassAttribute,
		short opcodeLengths[], TranslationTableOffset *newRuleOffset,
		TranslationTableRule **newRule, RuleName **ruleNames,
		TranslationTableHeader **table) {
	int k;
	char includeThis[MAXSTRING];
	char **tableFiles;
	int rv;
	for (k = 0; k < includedFile->length; k++)
		includeThis[k] = (char)includedFile->chars[k];
	includeThis[k] = 0;
	tableFiles = _lou_resolveTable(includeThis, nested->fileName);
	if (tableFiles == NULL) {
		errorCount++;
		return 0;
	}
	if (tableFiles[1] != NULL) {
		errorCount++;
		free_tablefiles(tableFiles);
		_lou_logMessage(LOG_ERROR,
				"Table list not supported in include statement: 'include %s'",
				includeThis);
		return 0;
	}
	rv = compileFile(*tableFiles, characterClasses, characterClassAttribute,
			opcodeLengths, newRuleOffset, newRule, ruleNames, table);
	free_tablefiles(tableFiles);
	return rv;
}