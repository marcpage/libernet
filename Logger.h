#ifndef __Logger_h__
#define __Logger_h__

#include "os/Thread.h"
#include "os/Queue.h"
#include "os/Path.h"

#include <string>
#include <map>
#include <vector>

#define logDetail(logObject, message) logObject.log(logger::Logger::Detail, message, __FILE__, __LINE__)
#define logInfo(logObject, message) logObject.log(logger::Logger::Info, message, __FILE__, __LINE__)
#define logWarn(logObject, message) logObject.log(logger::Logger::Warn, message, __FILE__, __LINE__)
#define logError(logObject, message) logObject.log(logger::Logger::Error, message, __FILE__, __LINE__)
#define logException(logObject, exception, message) logObject.exception(exception, message, __FILE__, __LINE__)

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

	};

	inline Logger::Logger(FILE *file)
			:exec::Thread(exec::Thread::KeepAroundAfterFinish),_logs(), _file(NULL == file ? stderr : file), _path(), _done(false),_defaultLevel(Warn),_fileLevels() {
		start();
	}
	inline Logger::Logger(const io::Path &path)
			:exec::Thread(exec::Thread::KeepAroundAfterFinish),_logs(), _file(NULL), _path(path), _done(false),_defaultLevel(Warn),_fileLevels() {
		if (!_path.parent().isDirectory()) {
			_path.parent().mkdirs();
		}
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
		_done = true;
		log(Detail, "finish()", __FILE__, __LINE__);
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
}


#endif // __Logger_h__
