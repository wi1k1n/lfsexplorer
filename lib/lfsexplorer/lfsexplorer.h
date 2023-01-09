#ifndef LFSEXPLORER_H__
#define LFSEXPLORER_H__

#ifndef __PRIVATE_LOG_PREAMBULE
#define __PRIVATE_LOG_PREAMBULE	   (Serial.print(millis())+\
									Serial.print(" | ")+\
									Serial.print(__FILE__)+\
									Serial.print(F(":"))+\
									Serial.print(__LINE__)+\
									Serial.print(F(":"))+\
									Serial.print(__func__)+\
									Serial.print(F("() - ")))
#endif
#ifndef DLOGLN
#define DLOGLN(txt)		(__PRIVATE_LOG_PREAMBULE+Serial.println(txt))
#endif
#ifndef DLOGF
#define DLOGF(fmt, ...)	(__PRIVATE_LOG_PREAMBULE+Serial.printf(fmt, __VA_ARGS__))
#endif
#ifndef DLOG
#define DLOG(txt)    	(__PRIVATE_LOG_PREAMBULE+Serial.print(txt))
#endif
#ifndef LOG
#define LOG(txt)    	(Serial.print(txt))
#endif
#ifndef LOGF
#define LOGF(fmt, ...)	(Serial.printf(fmt, __VA_ARGS__))
#endif
#ifndef LOGLN
#define LOGLN(txt)		(Serial.println(txt))
#endif

#include <map>
#include <unordered_set>
#include <vector>
#include <functional>
#include "LittleFS.h"

// #define LFSE_COMMAND_MAX_LENGTH 10
#define LFSE_SERIAL_BUFFER_LENGTH 64

inline static bool isValidFSNameChar(char c) {
	return isAlphaNumeric(c) || c == '-' || c == '.';
}
inline static bool isValidFSPathChar(char c) {
	return isValidFSNameChar(c) || c == '/';
}
static bool isValidFSName(const String& name) {
	if (name.isEmpty())
		return false;
	for (const char& c : name)
		if (!isValidFSNameChar(c))
			return false;
	return true;
}
static bool isValidFSPath(const String& path) {
	if (path.isEmpty())
		return false;
	for (const char& c : path)
		if (!isValidFSNameChar(c) && c != '/')
			return false;
	return true;
}

struct LFSEPath {
	std::vector<String> _tokens;

	LFSEPath() = default;
	LFSEPath(const LFSEPath& obj);
	LFSEPath(const String& root);

	// replaces if path is absolute, adds if relative
	void adjust(const String& path);
	bool isEmpty() const { return _tokens.size(); }
	String toString(bool trailingSlash = false) const;

	LFSEPath createAdjustedFromUserPath(String userPath);
private:
	void tokenize(const String& s, bool appendToExisting = false);
};

struct LFSECommand {
	struct Arg {
		enum class Type {
			NONE = -1,
			FILENAME = 0,
			FLAG,
			STRING
		} type = Type::NONE;
		String value;
		uint16_t idx = 0;
		
		inline bool isFlagAndStartsWith(char c) const { return isTypeFlag() && !value.isEmpty() && value[0] == c; }

		inline bool isTypeFlag() const { return type == Type::FLAG; }
		inline bool isTypeString() const { return type == Type::STRING; }
		inline bool isTypeFilename() const { return type == Type::FILENAME; }

		String toString(bool onlyContent = true) const {
			if (onlyContent)
				return value;
			return (isTypeFlag() ? "-" : (type == Type::STRING ? "\"" : "")) + value + (type == Type::STRING ? "\"" : "");
		}
		operator String() const { return toString(); }

		bool operator<(const Arg& rhs) const { return idx < rhs.idx; }
		bool operator==(const Arg& x) const { return idx == x.idx && value.equals(x.value) && type == x.type; }
	};

	String _cmd;
	std::vector<Arg> _args; // TODO: should be hashset?
	char* _buffer = nullptr;
	uint16_t _bufferLength = 0;
	bool _argsParsed = false;
	uint8_t _cmdBufferCursor = 0;

	LFSECommand() = default;
	LFSECommand(char* buffer, uint16_t len) : _buffer(buffer), _bufferLength(len) { parseCmd(); }

	// Flags
	bool isSingleLetterFlagPresent(char f) const;
	template <typename T>
	T getNumericalFlagValue(char f, const T& fallback = T()) const;
	// Filenames
	uint8_t getArgFirstFilenameOrLastArgIdx(uint8_t startIdx = 0) const;
	Arg getArgFirstFilenameOrLastArg(uint8_t startIdx = 0) const;

	void parseCmd();
	void parseArgs();

	String toString() const;
	operator String() const { return toString(); }
};

namespace std {
	template <> struct hash<LFSECommand::Arg> {
		size_t operator()(const LFSECommand::Arg& arg) const {
			return std::hash<const char*>()(arg.value.begin()) ^ std::hash<uint16_t>()(arg.idx) ^ std::hash<size_t>()(static_cast<size_t>(arg.type));
		}
	};
}

typedef std::function<void(LFSECommand&)> cmdFunc;
typedef std::tuple<cmdFunc, String, String> cmdInfo; // function, arguments description, command description
typedef std::pair<String, cmdInfo> cmdMapEntry;

class DEBUG {
public:
	static void LittleFSExplorer();
private:
	static std::map<String, cmdInfo> lfseCmdMap;
	static char lfseBuffer[];
	static LFSEPath lfsePath;

	static void logExecutedCommand(const LFSECommand& cmd);
	static void handleCommand(uint16_t length);

	static void cmdHelp(LFSECommand& cmd);
	static void cmdFormat(LFSECommand& cmd);
	static void cmdLs(LFSECommand& cmd);
	static void cmdCd(LFSECommand& cmd);
	static void cmdPwd(LFSECommand& cmd);
	static void cmdMkdir(LFSECommand& cmd);
	static void cmdMv(LFSECommand& cmd);
	static void cmdRm(LFSECommand& cmd);
	static void cmdTouch(LFSECommand& cmd);
	static void cmdWrite(LFSECommand& cmd);
	static void cmdCat(LFSECommand& cmd);
	static void cmdMan(LFSECommand& cmd);

	static bool checkIsAFile(const File& f, const String& path);
	static bool checkIsADir(const File& f, const String& path);
	static bool checkMissingOperand(LFSECommand& cmd, uint8_t nRequiredArgs = 1, LFSECommand::Arg::Type requiredType = LFSECommand::Arg::Type::FILENAME);
	static bool checkInvalidDirPath(const String& path);
	static bool checkInvalidFilePath(const String& path);
	static bool checkAlreadyExists(const String& path);
	static bool checkDoesntExist(const String& path);
};

#endif // LFSEXPLORER_H__