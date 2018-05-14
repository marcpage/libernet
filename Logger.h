#ifndef __Logger_h__
#define __Logger_h__

#include "os/Thread.h"
#include "os/Queue.h"
#include "os/Path.h"
#include "os/Environment.h"

#include <string>
#include <map>
#include <vector>
#include <regex>
#include <algorithm>
#include <limits>

#define logDetail(logObject, message) if (logObject.canLog(logger::Logger::Detail, __FILE__, __LINE__)) {logObject.log(logger::Logger::Detail, message, __FILE__, __LINE__);} else {logger::Logger::noop();}
#define logInfo(logObject, message) if (logObject.canLog(logger::Logger::Info, __FILE__, __LINE__)) {logObject.log(logger::Logger::Info, message, __FILE__, __LINE__);} else {logger::Logger::noop();}
#define logWarn(logObject, message) if (logObject.canLog(logger::Logger::Warn, __FILE__, __LINE__)) {logObject.log(logger::Logger::Warn, message, __FILE__, __LINE__);} else {logger::Logger::noop();}
#define logError(logObject, message) if (logObject.canLog(logger::Logger::Error, __FILE__, __LINE__)) {logObject.log(logger::Logger::Error, message, __FILE__, __LINE__);} else {logger::Logger::noop();}
#define logException(logObject, exception, message) if (logObject.canLog(logger::Logger::Error, __FILE__, __LINE__)) {logObject.exception(exception, message, __FILE__, __LINE__);} else {logger::Logger::noop();}

namespace logger {

	class Logger : public exec::Thread {
		public:
			enum Level {Detail, Info, Warn, Error, None};
			Logger(FILE *file=NULL);
			Logger(const io::Path &path);
			~Logger();
			void log(Level level, const std::string &message, const char *file, int line);
			void exception(const std::exception &exception, const std::string &message, const char *file, int line);
			void finish();
			Level logLevel(const char *file, int line) const;
			bool canLog(Level level, const char *file, int line) const;
			static void noop() {}
		protected:
			virtual void *run();
		private:
			struct _Range {
				_Range(int s, int e, Level l):start(s),end(e),level(l) {}
				_Range(int line, Level l):start(line),end(line),level(l) {}
				int start;
				int end;
				Level level;
			};
			typedef std::vector<_Range> _RangeList;
			typedef std::map<std::string,_RangeList> _FileRanges;
			exec::Queue<std::string>	_logs;
			FILE 						*_file;
			io::Path					_path;
			bool						_done;
			Level						_defaultLevel;
			_FileRanges					_fileLevels;
			std::string &_tolower(std::string &s);
			void _loadSettings(const io::Path &path);
			void _init();
	};

	inline Logger::Logger(FILE *file)
			:exec::Thread(exec::Thread::KeepAroundAfterFinish),_logs(), _file(NULL == file ? stderr : file), _path(), _done(false),_defaultLevel(Warn),_fileLevels() {
		_init();
		start();
	}
	inline Logger::Logger(const io::Path &path)
			:exec::Thread(exec::Thread::KeepAroundAfterFinish),_logs(), _file(NULL), _path(path), _done(false),_defaultLevel(Warn),_fileLevels() {
		_init();
		logDetail((*this), "Logger Started");
		start();
	}
	inline Logger::~Logger() {
		_done = true;
	}
	/**
		@todo We need to split up multiline messages
	*/
	inline void Logger::log(Level level, const std::string &message, const char *file, int line) {
		if (canLog(level, file, line)) {
			dt::DateTime	now;
			int				milliseconds = int(1000.0 * (now.seconds() - int(now.seconds())));
			std::string 	msString = std::to_string(milliseconds);
			intptr_t		thread = reinterpret_cast<intptr_t>(exec::ThreadId::current().thread());
			std::string		thString = std::to_string(thread);
			std::string		typeString;

			switch(level) {
				case Detail:	typeString = "DTAL"; break;
				case Info:		typeString = "INFO"; break;
				case Warn:		typeString = "WARN"; break;
				case Error:		typeString = "EROR"; break;
				case None:		typeString = "NONE"; break;
				default:		typeString = "????"; break;
			}
			while(msString.length() < 3) {msString = "0" + msString;}
			while(thString.length() < 16) {thString = "0" + thString;}
			std::string	line_prefix = "[" + typeString + " " + now.format("%Y/%m/%d %H:%M:%S") + "." + msString + "  Thread: " + thString + " " + file + ":" + std::to_string(line) + "] ";
			_logs.enqueue(line_prefix + message);
		}
	}
	inline void Logger::exception(const std::exception &exception, const std::string &message, const char *file, int line) {
		log(Error, exception.what() + std::string("\n") + message, file, line);
	}
	inline void Logger::finish() {
		log(Detail, "finish()", __FILE__, __LINE__);
		_done = true;
		join();
	}
	Logger::Level Logger::logLevel(const char *file, int line) const {
		Level	level = _defaultLevel;
		_FileRanges::const_iterator fileRanges = _fileLevels.find(file);

		if (fileRanges != _fileLevels.end()) {
			for (_RangeList::const_iterator range = fileRanges->second.begin(); range != fileRanges->second.end(); ++range) {
				if ( (line >= range->start) && (line <= range->end) ) {
					return range->level;
				}
			}
		}

		return level;
	}
	inline bool Logger::canLog(Level level, const char *file, int line) const {
		Level canLevel = logLevel(file, line);

		if (Detail == canLevel) {
			return true;
		} else if (Detail == level) {
			return false;
		}
		if (Info == canLevel) {
			return true;
		} else if (Info == level) {
			return false;
		}
		if (Warn == canLevel) {
			return true;
		} else if (Warn == level) {
			return false;
		}
		if (Error == canLevel) {
			return true;
		}
		return false;
	}
	inline void *Logger::run() {
		while(!_done || !_logs.empty()) {
			std::string next;

			try {
				next = _logs.dequeue();
			} catch(const std::exception &exception) {
				if (_done) {
					break;
				}
				next = exception.what();
			}

			FILE	*output = NULL == _file ? fopen(std::string(_path).c_str(), "a") : _file;

			if (NULL != output) {
				fprintf(output, "%s\n", next.c_str());
			}

			if ( (NULL == _file) && (NULL != output) ) {
				fclose(output);
			}
		}
		return NULL;
	}
	inline std::string &Logger::_tolower(std::string &s) {
		std::transform(s.begin(), s.end(), s.begin(), ::tolower);
		return s;
	}
	inline void Logger::_loadSettings(const io::Path &path) {
		if (path.isFile()) {
			std::string::size_type endOfLine = -1;
			std::string contents = std::regex_replace(std::regex_replace(path.contents(), std::regex("\\r\\n"), "\n"), std::regex("\\r"), "\n");
			int lineNumber = 0;

			do {
				std::string::size_type startOfLine = endOfLine + 1;

				endOfLine = contents.find("\n", startOfLine);

				std::string	line = contents.substr(startOfLine, endOfLine - startOfLine);
				lineNumber += 1;
				while ( (line.length() > 0) && (std::isspace(line[0])) ) {line.erase(0, 1);}
				while ( (line.length() > 0) && (std::isspace(line[line.length() - 1])) ) {line.erase(line.length() - 1, 1);}
				if ( (line.length() > 0) && (line[0] != '#') ) {
					std::string lowercased = line;
					std::string::size_type equalsPos = line.find('=');

					if (equalsPos == std::string::npos) {
						log(Warn, "Unable to interpret " + std::string(path) + ":" + std::to_string(lineNumber) + ": '" + line + "'", __FILE__, __LINE__);
					} else {
						std::string key = line.substr(0,equalsPos);
						std::string value = line.substr(equalsPos + 1);
						std::string lower = key;

						if (_tolower(lower) == "output") {
							lower = value;
							if (_tolower(lower) == "stderr") {
								_file = stderr;
							} else if (_tolower(lower) == "stdout") {
								_file = stdout;
							} else {
								_file = NULL;
								_path = value;
							}
						} else {
							_Range	range(0, std::numeric_limits<int>::max(), Detail);
							std::string::size_type colonPos = key.find(':');
							std::string file = key.substr(0, colonPos);

							lower = value;
							switch(_tolower(lower)[0]) {
								case 'd': range.level = Detail; break;
								case 'i': range.level = Info; break;
								case 'w': range.level = Warn; break;
								case 'e': range.level = Error; break;
								case 'n': range.level = None; break;
								default:
									log(Warn, "Unable to interpret " + std::string(path) + ":" + std::to_string(lineNumber) + ": Level = '" + value + "'", __FILE__, __LINE__);
									break;
							}

							lower = key;
							if (_tolower(lower) == "all") {
								_defaultLevel = range.level;
							} else {
								if (colonPos != std::string::npos) {
									std::string::size_type	minusPos = key.find('-');

									range.start = std::stoi(key.substr(colonPos + 1, minusPos - colonPos - 1));
									range.end = (std::string::npos == minusPos) ? range.start : std::stoi(key.substr(minusPos + 1));
								}
								_fileLevels[file].push_back(range);
							}
						}
					}
				}
			} while(endOfLine != std::string::npos);
		}
	}
	inline void Logger::_init() {
		_loadSettings(io::Path(env::get("HOME")) + "Library" + "Preferences" + "Logger.txt");
		_loadSettings(io::Path("Logger.txt"));
		if ( (NULL == _file) && (std::string(_path).length() == 0) ) {
			_path = io::Path(env::get("HOME")) + "Library" + "Logs" + "Logger.txt";
		}
		if ( (NULL == _file) && (!_path.parent().isDirectory()) ) {
			_path.parent().mkdirs();
		}
	}

}


#endif // __Logger_h__
