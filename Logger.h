#ifndef __Logger_h__
#define __Logger_h__

#include "os/Thread.h"
#include "os./Queue.h"

#define logDetail(logger, message) logger.log(log::Logger::Detail, message, __FILE__, __LINE__)
#define logInfo(logger, message) logger.log(log::Logger::Info, message, __FILE__, __LINE__)
#define logWarn(logger, message) logger.log(log::Logger::Warn, message, __FILE__, __LINE__)
#define logError(logger, message) logger.log(log::Logger::Error, message, __FILE__, __LINE__)
#define logException(logger, exception, message) logger.log(exception, message, __FILE__, __LINE__)

namespace log {

	class Logger : public exec::Thread {
		public:
			enum Level {Detail, Info, Warn, Error};
			Logger(FILE file=NULL);
			Logger(const io::Path &path);
			~Logger();
			void log(Level level, const std::string &message, const char *file, int line);
			void exception(const std::exception &exception, const std::string &message, const char *file, int line);
		protected:
			virtual void *run();
		private:
			Queue<std::string>	_logs;
			FILE 				*_file;
			io::Path			_path;
	};

	Logger::Logger(FILE file):_logs(), _file(NULL == file ? stderr : file), _path() {
		start();
	}
	Logger::Logger(const io::Path &path):logs(), _file(NULL), _path(path) {
		start();
	}
	Logger::~Logger() {}
	/**
		@todo We need to split up multiline messages
	*/
	void Logger::log(Level level, const std::string &message, const char *file, int line) {
		dt::DateTime	now;
		int				milliseconds = int(1000.0 * (now.seconds() - int(now.seconds())));
		std::string 	msString = std::to_string(milliseconds);
		intptr_t		thread = exec::ThreadId::current().thread();
		std::string		thString = std::to_string(thread, 16);

		while(msString.length() < 3) {msString = "0" + msString;}
		while(thString.length() < 16) {thString = "0" + thString;}
		std::string	line_prefix = "[" + now.format("%Y/%m/%d %H:%M:%S") + "." + msString + "  Thread: " + thString + " " + file + ":" + std::to_string(line) + "] ";
		_logs.enqueue(line_prefix + message;
	}
	void Logger::exception(const std::exception &exception, const std::string &message, const char *file, int line) {
		log(Error, exception.what() + std::string("\n") + message, file, line);
	}
	void *Logger::run() {
		while(true) {
			std::string next = _logs.dequeue();
			if (NULL != _file) {
				fprintf(_file, "%s\n", next.c_str());
			}
		}
	}


}


#endif // __Logger_h__
