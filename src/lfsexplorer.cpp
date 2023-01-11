#include "lfsexplorer.h"

std::map<String, cmdInfo> DEBUG::lfseCmdMap = {
	cmdMapEntry("help", cmdInfo(cmdHelp, "", "show help message")),
	cmdMapEntry("wipe", cmdInfo(cmdFormat, "-f", "delete all data from the filesystem")),
	cmdMapEntry("ls", cmdInfo(cmdLs, "[dirpath]", "list children files/directories")),
	cmdMapEntry("cd", cmdInfo(cmdCd, "[dirpath]", "change current working directory")),
	cmdMapEntry("pwd", cmdInfo(cmdPwd, "", "show current working directory")),
	cmdMapEntry("mkdir", cmdInfo(cmdMkdir, "[dirname]", "create directory (not recursive)")),
	cmdMapEntry("mv", cmdInfo(cmdMv, "[path_from] [path_to]", "move and/or rename file/directory")),
	cmdMapEntry("rm", cmdInfo(cmdRm, "[path]", "remove file/directory")),
	cmdMapEntry("cp", cmdInfo(cmdCp, "[path_src] [path_dst]", "copy file/directory")),
	cmdMapEntry("touch", cmdInfo(cmdTouch, "[filepath]", "create empty file")),
	cmdMapEntry("tee", cmdInfo(cmdWrite, "[\"content_args\"] [filepath]", "(over)write arguments' content to file")),
	cmdMapEntry("cat", cmdInfo(cmdCat, "[filepath]", "print content of the file")),
	cmdMapEntry("man", cmdInfo(cmdMan, "[command]", "show manual for command")),
};
LFSEPath DEBUG::lfsePath;
char DEBUG::lfseBuffer[LFSE_SERIAL_BUFFER_LENGTH];

void DEBUG::cmdHelp(LFSECommand& cmd) {
	LOGLN(F("The following commands are available for execution:"));
	for (const cmdMapEntry& cmdEntry : lfseCmdMap) {
		LOG(cmdEntry.first);
		LOG(F(" "));
		LOG(std::get<1>(cmdEntry.second));
		LOG(F("\t"));
		LOGLN(std::get<2>(cmdEntry.second));
	}
}
void DEBUG::cmdFormat(LFSECommand& cmd) {
	if (!LittleFS.format()) {
		LOGLN(F("Formatting filesystem failed!"));
	}
}
void DEBUG::cmdPwd(LFSECommand& cmd) {
	LOGLN(lfsePath.toString());
}
void DEBUG::cmdLs(LFSECommand& cmd) {
	cmd.parseArgs();
	String userPath;
	if (cmd._args.size()) {
		userPath = cmd.getArgFirstFilenameOrLastArg();
		if (checkInvalidDirPath(userPath))
			return;
	}
	String dirPath(lfsePath.createAdjustedFromUserPath(userPath).toString());
	if (checkDoesntExist(dirPath))
		return;
	Dir dir = LittleFS.openDir(dirPath);
	while (dir.next()) {
		LOG(dir.isFile() ? F("f ") : (dir.isDirectory() ? F("d ") : F("- ")));
		const size_t nCharFS = LOG(dir.fileSize());
		for (uint8_t i = 0; i < max(6 - nCharFS, (unsigned int)0); ++i)
			LOG(' ');
		const size_t nCharCT = LOG(dir.fileCreationTime());
		for (uint8_t i = 0; i < max(10 - nCharCT, (unsigned int)0); ++i)
			LOG(' ');
		LOGLN(dir.fileName());
	}
}
void DEBUG::cmdCd(LFSECommand& cmd) {
	cmd.parseArgs();
	if (checkMissingOperand(cmd))
		return;
	String userPath(cmd.getArgFirstFilenameOrLastArg());
	if (checkInvalidDirPath(userPath) || checkDoesntExist(userPath))
		return;
	lfsePath.adjust(userPath);
}
void DEBUG::cmdMkdir(LFSECommand& cmd) {
	cmd.parseArgs();
	if (checkMissingOperand(cmd))
		return;
	String userPath(cmd.getArgFirstFilenameOrLastArg());
	if (checkInvalidDirPath(userPath)) 
		return;
	String dirPath(lfsePath.createAdjustedFromUserPath(userPath).toString());
	if (checkAlreadyExists(dirPath))
		return;
	if (!LittleFS.mkdir(dirPath)) {
		LOG(F("Failed to create directory "));
		LOGLN(dirPath);
		return;
	}
}
void DEBUG::cmdMv(LFSECommand& cmd) {
	// TODO: handle dot in path properly to keep the name
	cmd.parseArgs();
	if (checkMissingOperand(cmd, 2))
		return;
	uint8_t filePath1ArgIdx = cmd.getArgFirstFilenameOrLastArgIdx();
	String userPath1(cmd._args[filePath1ArgIdx]);
	String userPath2(cmd.getArgFirstFilenameOrLastArg(filePath1ArgIdx + 1));
	if (checkInvalidFilePath(userPath1) || checkInvalidFilePath(userPath2))
		return;
	String path1 = lfsePath.createAdjustedFromUserPath(userPath1).toString();
	String path2 = lfsePath.createAdjustedFromUserPath(userPath2).toString();
	if (checkDoesntExist(path1) || checkAlreadyExists(path2))
		return;
	if (!LittleFS.rename(path1, path2)) {
		LOG(F("Failed to move from "));
		LOG(path1);
		LOG(F(" to "));
		LOGLN(path2);
		return;
	}
}
void DEBUG::cmdCp(LFSECommand& cmd) {
	// TODO: handle dot in path properly to keep the name
	cmd.parseArgs();
	if (checkMissingOperand(cmd, 2))
		return;
	uint8_t filePath1ArgIdx = cmd.getArgFirstFilenameOrLastArgIdx();
	String userPath1(cmd._args[filePath1ArgIdx]);
	String userPath2(cmd.getArgFirstFilenameOrLastArg(filePath1ArgIdx + 1));
	
	bool copyDir = cmd.isSingleLetterFlagPresent('r');
	bool copyForce = cmd.isSingleLetterFlagPresent('f');

	if (copyDir) {
		if (checkInvalidFilePath(userPath1) || checkInvalidFilePath(userPath2))
			return;
	} else {
		if (checkInvalidDirPath(userPath1) || checkInvalidDirPath(userPath2))
			return;
	}

	String path1 = lfsePath.createAdjustedFromUserPath(userPath1).toString();
	String path2 = lfsePath.createAdjustedFromUserPath(userPath2).toString();
	if (checkDoesntExist(path1) || (copyForce && checkAlreadyExists(path2)))
		return;

	File f = LittleFS.open(path1, "r");
	bool isDir = f.isDirectory();
	f.close();
	
	// requested to copy dirs
	if (copyDir) {
		if (!isDir) {
			LOG(path1);
			LOGLN(F(" is not a directory"));
			return;
		}
		LOGLN(F("Not implemented yet!")); // TODO: implement
		return;
	}
	// requested to copy files
	if (isDir) {
		LOG(path1);
		LOGLN(F(" is not a file"));
		return;
	}

	File fsrc = LittleFS.open(path1, "r");
	if (!fsrc) {
		LOG(F("Failed to open file "));
		LOGLN(path1);
		return;
	}
	File fdst = LittleFS.open(path2, "w");
	if (!fsrc) {
		LOG(F("Failed to open file "));
		LOGLN(path2);
		return;
	}

	char buffer[LFSE_FILE_BUFFER_LENGTH];
	while (fsrc.available()) {
		size_t nBytes = fsrc.readBytes(buffer, LFSE_FILE_BUFFER_LENGTH);
		fdst.write(buffer, nBytes);
	}
}
void DEBUG::cmdTouch(LFSECommand& cmd) {
	cmd.parseArgs();
	if (checkMissingOperand(cmd))
		return;
	String userPath(cmd.getArgFirstFilenameOrLastArg());
	if (checkInvalidFilePath(userPath)) 
		return;
	String filePath(lfsePath.createAdjustedFromUserPath(userPath).toString());
	if (checkAlreadyExists(filePath))
		return;
	File f = LittleFS.open(filePath, "w");
	if (!f) {
		LOG(F("Failed to create file "));
		LOGLN(filePath);
		return;
	}
	f.close();
}
void DEBUG::cmdWrite(LFSECommand& cmd) {
	cmd.parseArgs();
	if (checkMissingOperand(cmd))
		return;
	uint8_t filePathArgIdx = cmd.getArgFirstFilenameOrLastArgIdx();
	String userPath(cmd._args[filePathArgIdx]);
	if (checkInvalidFilePath(userPath)) 
		return;
	String filePath(lfsePath.createAdjustedFromUserPath(userPath).toString());

	// get flags
	bool append = cmd.isSingleLetterFlagPresent('a');
	bool newLines = !cmd.isSingleLetterFlagPresent('n');

	File f = LittleFS.open(filePath, append ? "a" : "w+");
	if (!f) {
		LOG(F("Failed to open file "));
		LOGLN(filePath);
		return;
	}
	if (f.isDirectory()) {
		if (checkIsADir(f, filePath)) {
			f.close();
			return;
		}
	}

	// iterate over all string args that go before the filename arg
	bool dirty = false;
	for (uint8_t i = filePathArgIdx; i < cmd._args.size(); ++i) {
		LFSECommand::Arg& arg = cmd._args[i];
		if (arg.isTypeString()) {
			dirty = true;
			if (newLines)
				f.println(arg.value);
			else
				f.print(arg.value);
		}
	}
	f.close();
	if (!dirty) {
		LOGLN(F("Missing data to write"));
	}
}

void DEBUG::cmdRm(LFSECommand& cmd) {
	cmd.parseArgs();
	if (checkMissingOperand(cmd))
		return;
	
	bool removeDir = cmd.isSingleLetterFlagPresent('r');
	// if these flags are present, command removes lines in file
	int16_t lastIdx = cmd.getNumericalFlagValue('l', -1);
	int16_t firstIdx = cmd.getNumericalFlagValue('f', -1);
	if (lastIdx > -1) {
		if (removeDir) {
			LOGLN(F("Flags l and r are incompatible"));
			return;
		}
		if (firstIdx > -1 && firstIdx > lastIdx) {
			LOGLN(F("-l should be >= than -f"));
			return;
		}
	}

	String userPath(cmd.getArgFirstFilenameOrLastArg());
	if (removeDir ? checkInvalidDirPath(userPath) : checkInvalidFilePath(userPath))
		return;

	String path = lfsePath.createAdjustedFromUserPath(userPath).toString();
	if (checkDoesntExist(path))
		return;

	File file = LittleFS.open(path, "r+");
	bool isDir = file.isDirectory();
	file.close();

	// Removing dir
	if (isDir) {
		if (!removeDir) {
			LOG(path);
			LOGLN(F(" is not a file"));
			return;
		}
		if (!LittleFS.rmdir(path)) {
			LOG(F("Failed to remove directory "));
			LOGLN(path);
		}
		return;
	}
	if (removeDir) {
		LOG(path);
		LOGLN(F(" is not a directory"));
		return;
	}

	// Removing lines from file
	if (lastIdx > -1) {
		// LOGLN("Not implemented yet!");
		uint16_t lineFirstIdx = firstIdx > -1 ? firstIdx : lastIdx;
		uint16_t lineLastIdx = lastIdx;

		DLOG(lineFirstIdx);
		LOG(" ");
		LOGLN(lineLastIdx);

		File f = LittleFS.open(path, "r+");		
		// first determine cursor pos where the second part should be copied to
		uint16_t lineIdx = 0;
		while (f.available() && lineIdx < lineFirstIdx) {
			DLOGLN(lineIdx);
			// char c = '\0';
			// while (c != '\n') {
			// 	c = (char)f.read();
			// 	DLOGLN(c);
			// }
			f.readStringUntil('\n');
			++lineIdx;
		}
		size_t insertCursor = f.position();
		DLOGLN(insertCursor);
		while (f.available() && lineIdx <= lineLastIdx) {
			String line = f.readStringUntil('\n');
			DLOGLN(line);

			size_t curPos = f.position();
			DLOGLN(curPos);

			f.seek(insertCursor);

			insertCursor += f.println(line);
			DLOGLN(insertCursor);

			f.seek(curPos);
			++lineIdx;
		}
		// f.truncate(insertCursor);
		f.close();
		return;
	}

	// Removing file
	if (!LittleFS.remove(path)) {
		LOG(F("Failed to remove file "));
		LOGLN(path);
	}
	return;
}
void DEBUG::cmdCat(LFSECommand& cmd) {
	// TODO: too long function, better split into semantic parts!
	cmd.parseArgs();
	if (checkMissingOperand(cmd))
		return;
	String userPath(cmd.getArgFirstFilenameOrLastArg());
	if (checkInvalidFilePath(userPath)) 
		return;
	String filePath(lfsePath.createAdjustedFromUserPath(userPath).toString());
	if (checkDoesntExist(filePath))
		return;

	File f = LittleFS.open(filePath, "r");
	if (!f) {
		LOG(F("Failed to open file "));
		LOGLN(filePath);
		return;
	}
	if (checkIsADir(f, filePath)) {
		f.close();
		return;
	}
	
	// get flags
	// TODO: it'd be nice to have numerical flags and distinguish them positionally
	bool lineNumbers = cmd.isSingleLetterFlagPresent('n');
	bool byteView = cmd.isSingleLetterFlagPresent('b');
	bool plainMode = cmd.isSingleLetterFlagPresent('p');
	uint16_t limitColumn = cmd.getNumericalFlagValue('c', byteView ? 16 : 128);
	bool flagF = cmd.isSingleLetterFlagPresent('f');
	bool flagL = cmd.isSingleLetterFlagPresent('l');
	uint16_t rowIdxFirst = cmd.getNumericalFlagValue('f', 0);
	uint16_t rowIdxLast = cmd.getNumericalFlagValue('l', 0);

	if (!limitColumn) {
		LOGLN(F("cat: -c cannot be 0"));
		return;
	}
	if (!byteView && plainMode) {
		LOGLN(F("cat: -p ignored because -b is missing"));	
	}
	if (byteView && plainMode && lineNumbers) {
		LOGLN(F("cat: -n ignored because -bp are present"));	
	}
	if (flagF && flagL) {
		if (rowIdxFirst > rowIdxLast) {
			LOGLN(F("cat: -l cannot be smaller than -f"));
			return;
		}
		if (rowIdxFirst == rowIdxLast) {
			++rowIdxLast;
		}
	}


	String bufString;
	if (byteView && plainMode) { // here we don't care about line breaks
		uint16_t byteIdxFirst = rowIdxFirst;
		uint16_t byteIdxLast = rowIdxLast;
		if (byteIdxFirst) {
			f.seek(byteIdxFirst);
			LOGLN(F("...>>"));
		}
		size_t byteCursor = byteIdxFirst;
		while (f.available() && (!byteIdxLast || (byteCursor < byteIdxLast))) {
			readChars(f, &bufString, limitColumn);
			for (const char& c : bufString) {
				if (byteIdxLast && (byteCursor >= byteIdxLast)) { // subtraction is safe as (bool)byteIdxLast == true
					LOGLN("");
					byteCursor = byteIdxLast; // break while loop
					break;
				}
				LOGF("%02x", c);
				++byteCursor;
				LOG(byteCursor % limitColumn ? F(" ") : F("\r\n"));
			}
		}
	} else { // here we count lines
		uint16_t lineIdx = 0;
		while (f.available() && (!rowIdxLast || lineIdx < rowIdxLast)) {
			if (lineIdx < rowIdxFirst) {
				readLine(f, nullptr, 0); // simply skip the line
				if (lineIdx == 0)
					LOGLN(F("...>>"));
				++lineIdx;
				continue;
			}
			if (lineNumbers) {
				LOG(lineIdx);
				LOG(F("\t"));
			}
			bool completeLine = readLine(f, &bufString, limitColumn, nullptr, byteView);
			if (byteView) {
				for (uint16_t i = 0; i < limitColumn && i < bufString.length(); ++i) {
					LOGF("%02x", bufString[i]);
					LOG(F(" "));
				}
			} else {
				LOG(bufString);
			}
			if ((!completeLine && f.available()) || (bufString.length() > limitColumn)) {
				LOG(F(" ->..."));
			}
			if (!completeLine)
				readLine(f, nullptr, 0); // skip the line
			++lineIdx;
			LOGLN("");
		}
	}
	if (f.available()) {
		LOGLN(F("<<..."));
	}
	f.close();
}
void DEBUG::cmdMan(LFSECommand& cmd) {
	cmd.parseArgs();
	if (checkMissingOperand(cmd))
		return;
}


inline static void _checkIsA(const String& path, const String& type) {
	LOG(path);
	LOG(F(" is a "));
	LOGLN(type);
}
bool DEBUG::checkIsAFile(const File& f, const String& path) {
	if (f.isDirectory())
		return false;
	_checkIsA(path, F("file"));
	return true;
}
bool DEBUG::checkIsADir(const File& f, const String& path) {
	if (f.isFile())
		return false;
	_checkIsA(path, F("directory"));
	return true;
}
bool DEBUG::checkMissingOperand(LFSECommand& cmd, uint8_t nRequiredArgs, LFSECommand::Arg::Type requiredType) {
	if (cmd._args.size() >= nRequiredArgs) {
		uint8_t correctArgs = 0;
		for (const LFSECommand::Arg& arg : cmd._args) {
			if (arg.type == requiredType)
				++correctArgs;
		}
		if (correctArgs >= nRequiredArgs)
			return false;
	}
	LOGLN(F("Missing operand"));
	return true;
}
inline static void _checkInvalidPathLog(const String& path, const String& type) {
	LOG(F("Invalid "));
	LOG(type);
	LOG(F(" path: "));
	LOGLN(path);
}
bool DEBUG::checkInvalidDirPath(const String& path) {
	if (isValidFSPath(path))
		return false;
	_checkInvalidPathLog(path, F("directory"));
	return true;
}
bool DEBUG::checkInvalidFilePath(const String& path) {
	if (isValidFSPath(path))
		return false;
	_checkInvalidPathLog(path, F("file"));
	return true;
}
bool DEBUG::checkAlreadyExists(const String& path) {
	if (!LittleFS.exists(path))
		return false;
	LOG(path);
	LOGLN(F(" already exists"));
	return true;
}
bool DEBUG::checkDoesntExist(const String& path) {
	if (LittleFS.exists(path))
		return false;
	LOG(path);
	LOGLN(F(" doesn't exist"));
	return true;
}


void DEBUG::logExecutedCommand(const LFSECommand& cmd) {
	LOG(lfsePath.toString());
	LOG(F("$ "));
	LOGLN(cmd.toString());
}
void DEBUG::handleCommand(uint16_t length) {
	if (!length || length >= LFSE_SERIAL_BUFFER_LENGTH)
		return;

	LFSECommand cmd(lfseBuffer, length);
	auto search = lfseCmdMap.find(cmd._cmd);
	logExecutedCommand(cmd);
	if (search == lfseCmdMap.end()) {
		LOG(F("Error: command "));
		LOG(cmd._cmd);
		LOGLN(F(" not found!"));
		return;
	}
	std::get<0>(search->second)(cmd);
}

void DEBUG::LittleFSExplorer(const String& cmd) {
	while (!cmd.isEmpty() || _UART_.available() > 0) {
		size_t nBytesGot = cmd.isEmpty() ? _UART_.readBytesUntil('\n', lfseBuffer, LFSE_SERIAL_BUFFER_LENGTH) : 0;
		if (!cmd.isEmpty()) {
			for (const char& c : cmd)
				lfseBuffer[nBytesGot++] = c;
		}
		if (nBytesGot == 0) {
			LOGLN(F("Error: Could not read serial data: no valid data found!"));
			return;
		}
		if (nBytesGot == LFSE_SERIAL_BUFFER_LENGTH) {
			LOGLN(F("Error: Too big command!"));
			return;
		}
		handleCommand(nBytesGot);
		if (!cmd.isEmpty()) {
			break;
		}
	}
}

////////////////////////////////////////

LFSEPath::LFSEPath(const LFSEPath& obj) {
	for (const String& token : obj._tokens)
		_tokens.push_back(token);
}
LFSEPath::LFSEPath(const String& root) {
	tokenize(root);
}

// replaces if path is absolute, adds if relative
void LFSEPath::adjust(const String& path) {
	if (path.isEmpty() || !isValidFSPath(path)) {
		return;
	}
	tokenize(path, path[0] != '/');
}

String LFSEPath::toString(bool trailingSlash) const {
	String res;
	for (const String& token : _tokens) {
		res += "/";
		res += token;
	}
	if (trailingSlash || res.isEmpty())
		res += "/";
	return res;
}

void LFSEPath::tokenize(const String& s, bool appendToExisting) {
	if (!appendToExisting)
		_tokens.clear();
	String token;
	for (uint16_t i = 0; i < s.length(); ++i) {
		char c = s[i];
		if (isValidFSNameChar(c)) {
			token += c;
			continue;
		}
		if (c == '/') {
			if (token.length()) {
				_tokens.push_back(token);
				token.clear();
			}
		}
	}
	if (token.length()) {
		_tokens.push_back(std::move(token));
	}
	
	// get rid of '.'s and empties
	_tokens.erase(std::remove_if(_tokens.begin(), _tokens.end(), [](const String& token) {
		return token.isEmpty() || token.equals(F("."));
	}), _tokens.end());

	// resolve '..'s
	{
		auto it = _tokens.rbegin();
		while(it != _tokens.rend()) {
			auto token = *it;
			if (token.equals(F(".."))) {
				it = decltype(it)(_tokens.erase(std::next(it).base()));
				if (it != _tokens.rend())
					it = decltype(it)(_tokens.erase(std::next(it).base()));
				continue;
			}
			++it;
		}
	}
}

LFSEPath LFSEPath::createAdjustedFromUserPath(String userPath) {
	LFSEPath path(*this);
	path.adjust(userPath);
	return path;
}

////////////

void LFSECommand::parseCmd() {
	String token;
	uint16_t i = 0;
	for (; i < _bufferLength; ++i) {
		char c = _buffer[i];
		if (!isValidFSNameChar(c))
			break;
		token += c;
	}
	_cmdBufferCursor = i;
	_cmd = std::move(token);
	_cmd.toLowerCase();
}


bool LFSECommand::isSingleLetterFlagPresent(char f) const {
	for (const Arg& arg : _args) {
		if (!arg.isTypeFlag() || arg.value.isEmpty())
			continue;
		if (arg.value.indexOf(f) != -1)
			return true;
	}
	return false;
}

template <typename T> static T CastStringToNum(const String& s) { return static_cast<T>(s); }
template <> int CastStringToNum<int>(const String& s) { return s.toInt(); }
template <> float CastStringToNum<float>(const String& s) { return s.toFloat(); }
template <typename T>
T LFSECommand::getNumericalFlagValue(char f, const T& fallback) const {
	for (const Arg& arg : _args) {
		if (arg.isFlagAndStartsWith(f) && arg.value.length() > 1 && (isDigit(arg.value[1]) || arg.value[1] == '-')) {
			return CastStringToNum<T>(arg.value.substring(1));
		}
	}
	return fallback;
}

uint8_t LFSECommand::getArgFirstFilenameOrLastArgIdx(uint8_t startIdx) const {
	for (uint8_t i = startIdx; i < _args.size(); ++i) {
		if (_args[i].isTypeFilename()) {
			return i;
		}
	}
	return 0xFF;
}

LFSECommand::Arg LFSECommand::getArgFirstFilenameOrLastArg(uint8_t startIdx) const {
	return _args[getArgFirstFilenameOrLastArgIdx(startIdx)];
}

// String str contains all chars before CRLF, but at most maxLen chars
// String str doesn't contain CRLF unless addCRLF == true if returning true
// String str can be nullptr, then number of bytes that've been read can be accessed via strLenOut
// f.position() points to the char right after the CRLF sequence (if found), o/w to the (s.length()-1)th
// maxLen can be 0, then there's no limit
// ATTENTION! maxLen == 0 recommended only with str == nullptr
// returns true if CRLF follows the (s.length()-1)th char, false o/w
bool DEBUG::readLine(File& f, String* str, uint16_t maxLen, size_t* strLenOut, bool addCRLF) {
	// str can be nullptr, StringLike class handles it properly
	// this allows cheaply skip lines using readLine(f, nullptr, 0)
	StringLike s(str);
	s.clear();

	// problem with file
	if (!f || !f.available()) {
		if (strLenOut)
			*strLenOut = 0;
		return false;
	}

	char lastChar = '\0';
	bool brokeByLen = false; // TODO: use byte flags here
	bool brokenByCRLF = false;
	size_t originalPosition = f.position();
	while (f.available()) {
		if (maxLen && (s.length() >= maxLen + 2)) {
			brokeByLen = true;
			break;
		}
		char c = (char)f.read();
		s += c;
		if (c == '\n' && lastChar == '\r') {
			brokenByCRLF = true;
			break;
		}
		lastChar = c;
	}

	// s.length() == maxLen + 2
	if (brokeByLen) {
		// str cannot end with crlf
		// even if str.last == cr, str.length is already too big,
		// so in any case return false
		s.cut(0, s.length() - 2);
		f.seek(originalPosition + s.length());
		if (strLenOut)
			*strLenOut = s.length();
		return false;
	}

	int16_t d = s.length() - maxLen;

	// crlf are last 2 chars of s
	if (brokenByCRLF) {
		if (!addCRLF)
			s.cut(0, s.length() - 2);
		if (strLenOut)
			*strLenOut = s.length();
		return true;
	}

	// got to the end of file without seeing crlf
	if (d > 0) { // exceeded maxLen
		s.cut(0, maxLen);
		f.seek(originalPosition + maxLen);
	}
	if (strLenOut)
		*strLenOut = s.length();
	return false;
}
// Same as readLine, but doesn't stop on CRLF
// returns true if got to the end of file before exceeding maxLen
bool DEBUG::readChars(File& f, String* str, uint16_t maxLen, size_t* strLenOut) {
	// str can be nullptr, StringLike class handles it properly
	StringLike s(str);
	s.clear();
	// problem with file
	if (!f) {
		return false;
	}
	
	bool brokeByLen = false;
	size_t originalPosition = f.position();
	while (f.available()) {
		if (maxLen && (s.length() >= maxLen)) {
			brokeByLen = true;
			break;
		}
		char c = (char)f.read();
		s += c;
	}

	if (strLenOut)
		*strLenOut = s.length();

	// s.length() == maxLen
	if (brokeByLen) {
		return false;
	}
	// went to the end of file
	return true;
}

void LFSECommand::parseArgs() {
	// TODO: double check how the unpaired quotes are handled
	if (_argsParsed) {
		return;
	}
	_args.clear();
	if (!_cmdBufferCursor || _cmdBufferCursor >= _bufferLength) {
		return;
	}
	Arg token;
	bool prevCharEscape = false; // helps to detect escaped chars in string args
	auto addToken = [&](bool ignoreEmpty = true){
		if (!token.value.isEmpty() || !ignoreEmpty) {
			token.idx = _args.size();
			_args.push_back(std::move(token));
			token = Arg();
			return true;
		}
		return false;
	};
	auto strArgAddChar = [&](char c) {
		token.value += c;
		prevCharEscape = false;
	};
	for (uint16_t i = _cmdBufferCursor; i < _bufferLength; ++i) {
		char c = _buffer[i];
		if (token.isTypeFilename()) { // already filling filename arg
			if (isValidFSPathChar(c)) {
				token.value += c;
				continue;
			}
			addToken(); // if not valid filename char -> save token
			continue;
		}
		if (token.isTypeFlag()) { // already filling flag arg
			if (isAlphaNumeric(c) || c == '-' || c == '.') { // flag args are alphanumeric or '-' or '.'
				token.value += c;
				continue;
			}
			addToken(); // if not valid flag char -> save token
			continue;
		}
		if (token.isTypeString()) { // already filling string filename arg
			if (c == '"') { // potentially end of string arg
				if (prevCharEscape) { // no, it was escaped
					strArgAddChar(c);
					continue;
				}
				addToken(false); // was not escaped -> string ended -> save token
				continue;
			}
			if (prevCharEscape) { // the char was escaped -> add correct symbol to token
				switch(c) {
					case '"':
					case '\\':
						strArgAddChar(c);
						break;
					case 'n':
						strArgAddChar('\n');
						break;
					case 'r':
						strArgAddChar('\r');
						break;
					case 't':
						strArgAddChar('\t');
						break;
					default:
						strArgAddChar('\\');
						strArgAddChar(c);
						break;
				}
				prevCharEscape = false;
			} else {
				if (c == '\\') { // escape symbol
					prevCharEscape = true;
					continue;
				}
				strArgAddChar(c);
			}
			continue;
		}
		// if none of above -> just started filling token
		if (c == '"') { // only string args start with '"'
			token.type = Arg::Type::STRING;
			continue;
		}
		if (c == '-') { // flags start with '-'
			token.type = Arg::Type::FLAG;
			continue;
		}
		if (isValidFSPathChar(c)) {
			token.type = Arg::Type::FILENAME;
			token.value += c;
		}
		continue; // if no conditions met -> bullshit symbol
	}
	if (prevCharEscape) { // if '\' was the last symbol, add it
		strArgAddChar('\\');
	}
	addToken(!token.isTypeString());
	_argsParsed = true;
}

String LFSECommand::toString() const {
	String res(_cmd);
	if (_argsParsed) {
		for (const Arg& arg : _args) {
			res += " ";
			res += arg.toString(false);
		}
	} else if (_cmdBufferCursor && _cmdBufferCursor < _bufferLength) {
		for (uint16_t i = _cmdBufferCursor; i < _bufferLength; ++i)
			res += _buffer[i];
	}
	return res;
}

// Some debugging code
void DEBUG::customDebugCode(const String& l) {
	// File f = LittleFS.open(l.substring(1), "r");
	// if (!f) {
	// 	DLOGLN("!f");
	// 	f.close();
	// 	return;
	// }
	// String l1 = f.readStringUntil('\n');
	// DLOGLN(l1);
	// f.close();
	// File fn = LittleFS.open(l.substring(1), "r");
	// String s;
	// size_t nb;
	// bool cmpl = readLine(fn, &s, 5, &nb, false);
	// DLOGLN(s.length());
	// DLOGLN(s);
	// DLOGLN(cmpl);
	// fn.close();
}

template<std::size_t N, class T>
constexpr std::size_t countof(T(&)[N]) { return N; }
uint8_t DEBUG::_debugIdx = 0;
void DEBUG::_debug() {
	String cmds[] = {
		"",
		"ls",
		"cat f",
		"!f"
	};
	uint8_t cmdsCount = countof(cmds);
	while (_debugIdx < cmdsCount || _UART_.available()) {
		String l = (_debugIdx < cmdsCount) ? cmds[_debugIdx++] : _UART_.readStringUntil('\n');
		if (l.startsWith("!")) {
			customDebugCode(l);
			continue;
		}
		LittleFSExplorer(l);
	}
}