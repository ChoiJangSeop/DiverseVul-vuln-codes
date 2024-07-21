static std::string getComment(const std::string &fulltext, int line)
{
	if (line < 1) return "";

	// Locate line
	unsigned int start = 0;
	for (; start<fulltext.length() ; ++start) {
		if (line <= 1) break;
		if (fulltext[start] == '\n') line--;
	}

	int end = start + 1;
	while (fulltext[end] != '\n') end++;

	std::string comment = fulltext.substr(start, end - start);

	// Locate comment
	unsigned int startText = 0;
	int noOfSemicolon = 0;
	bool inString = false;
	for (; startText < comment.length() - 1; ++startText) {
		if (inString && comment.compare(startText, 2, "\\\"") == 0) {
			startText++;
			continue;
		}
		if (comment[startText] == '"') inString = !inString;
		if (!inString) {
			if (comment.compare(startText, 2, "//") == 0) break;
			if (comment[startText] == ';' && noOfSemicolon > 0) return "";
			if (comment[startText] == ';') noOfSemicolon++;
		}
	}

	if (startText + 2 > comment.length()) return "";

	std::string result = comment.substr(startText + 2);
	return result;
}