#ifndef LFSEXPLORER_H__
#define LFSEXPLORER_H__

#include <map>
#include <unordered_set>
#include <vector>
#include <functional>
#include "LittleFS.h"

#define LFSE_SERIAL_BUFFER_LENGTH 256
#define LFSE_FILE_BUFFER_LENGTH 64

////////////////////////////////////////////////////////////////////////////////

#ifdef CUSTOM_UART
#include "CustomUART.h"
#define _UART_ uart
#else
#define _UART_ Serial
#endif

#ifndef __PRIVATE_LOG_PREAMBULE
#define __PRIVATE_LOG_PREAMBULE	   (_UART_.print(millis())+\
									_UART_.print(" | ")+\
									_UART_.print(__FILE__)+\
									_UART_.print(F(":"))+\
									_UART_.print(__LINE__)+\
									_UART_.print(F(":"))+\
									_UART_.print(__func__)+\
									_UART_.print(F("() - ")))
#endif
#ifndef DLOGLN
#define DLOGLN(txt)		(__PRIVATE_LOG_PREAMBULE+_UART_.println(txt))
#endif
#ifndef DLOGF
#define DLOGF(fmt, ...)	(__PRIVATE_LOG_PREAMBULE+_UART_.printf(fmt, __VA_ARGS__))
#endif
#ifndef DLOG
#define DLOG(txt)    	(__PRIVATE_LOG_PREAMBULE+_UART_.print(txt))
#endif
#ifndef LOG
#define LOG(txt)    	(_UART_.print(txt))
#endif
#ifndef LOGF
#define LOGF(fmt, ...)	(_UART_.printf(fmt, __VA_ARGS__))
#endif
#ifndef LOGFLN
#define LOGFLN(fmt, ...)	(_UART_.printf(fmt, __VA_ARGS__)+_UART_.println())
#endif
#ifndef LOGLN
#define LOGLN(txt)		(_UART_.println(txt))
#endif
////////////////////////////////////////////////////////////////////////////////

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
	static void LittleFSExplorer(const String& cmd);
	static void _debug();
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
	static void cmdCp(LFSECommand& cmd);
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

	static bool readLine(File& f, String* s, uint16_t maxLen, size_t* strLenOut = nullptr, bool addCRLF = false);
	static bool readChars(File& f, String* s, uint16_t maxLen, size_t* strLenOut = nullptr);
	static uint8_t _debugIdx;
	static void customDebugCode(const String&);
};

// A small helping struct for readLine function
// Behaves as string if constructed with a valid string pointer
// o/w very cheaply imitates string behavior
struct StringLike {
	String* _s;
	size_t _sLength = 0;
	StringLike(String* sPtr) : _s(sPtr) { }

	size_t length() const { return _s ? _s->length() : _sLength; }
	StringLike& operator+=(const char& rhs) {
		if (_s) {
			*_s += rhs;
		} else {
			++_sLength;
		}
		return *this;
	}
	char operator[](size_t idx) const {
		if (_s)
			return (*_s)[idx];
		return '\0';
	}
	void clear() {
		if (_s)
			return _s->clear();
		_sLength = 0;
	}
	void cut(size_t left, size_t right) { // .substring without creating new obj
		if (_s) {
			*_s = _s->substring(left, right);
			return;
		}
		_sLength = right - left;
	}
};

#endif // LFSEXPLORER_H__