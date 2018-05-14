#ifndef __Logger_h__
#define __Logger_h__

#include "os/Thread.h"
#include "os/Queue.h"
#include "os/Path.h"
#include <string>

#define logDetail(logObject, message) logObject.log(logger::Logger::Detail, message, __FILE__, __LINE__)
#define logInfo(logObject, message) logObject.log(logger::Logger::Info, message, __FILE__, __LINE__)
#define logWarn(logObject, message) logObject.log(logger::Logger::Warn, message, __FILE__, __LINE__)
#define logError(logObject, message) logObject.log(logger::Logger::Error, message, __FILE__, __LINE__)
#define logException(logObject, exception, message) logObject.exception(exception, message, __FILE__, __LINE__)

namespace logger {

	class Logger : public exec::Thread {
		public:
			enum Level {Detail, Info, Warn, Error};
			Logger(FILE *file=NULL);
			Logger(const io::Path &path);
			~Logger();
			void log(Level level, const std::string &message, const char *file, int line);
			void exception(const std::exception &exception, const std::string &message, const char *file, int line);
		protected:
			virtual void *run();
		private:
			exec::Queue<std::string>	_logs;
			FILE 						*_file;
			io::Path					_path;
			bool						_gone;
	};

	Logger::Logger(FILE *file):_logs(), _file(NULL == file ? stderr : file), _path(), _gone(false) {
		start();
	}
	Logger::Logger(const io::Path &path):_logs(), _file(NULL), _path(path), _gone(false) {
		if (!_path.parent().isDirectory()) {
			_path.parent().mkdirs();
		}
		start();
	}
	Logger::~Logger() {
		_gone = true;
	}
	/**
		@todo We need to split up multiline messages
	*/
	void Logger::log(Level level, const std::string &message, const char *file, int line) {
		dt::DateTime	now;
		int				milliseconds = int(1000.0 * (now.seconds() - int(now.seconds())));
		std::string 	msString = std::to_string(milliseconds);
		intptr_t		thread = reinterpret_cast<intptr_t>(exec::ThreadId::current().thread());
		std::string		thString = std::to_string(thread);
		std::string		typeString;

		switch(level) {
			case Detail: typeString = "DTAL"; break;
			case Info: typeString = "INFO"; break;
			case Warn: typeString = "WARN"; break;
			case Error: typeString = "EROR"; break;
			default: typeString = "????"; break;
		}
		while(msString.length() < 3) {msString = "0" + msString;}
		while(thString.length() < 16) {thString = "0" + thString;}
		std::string	line_prefix = "[" + typeString + " " + now.format("%Y/%m/%d %H:%M:%S") + "." + msString + "  Thread: " + thString + " " + file + ":" + std::to_string(line) + "] ";
		_logs.enqueue(line_prefix + message);
	}
	void Logger::exception(const std::exception &exception, const std::string &message, const char *file, int line) {
		log(Error, exception.what() + std::string("\n") + message, file, line);
	}
	void *Logger::run() {

		while(true) {
			std::string next;

			try {
				next = _logs.dequeue();
			} catch(const std::exception &exception) {
				if (_gone) {
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
