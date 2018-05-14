#include <stdio.h>
#include "libernet/Logger.h"

#ifdef __Tracer_h__
	#define RunCount 10
#else
	#define RunCount 250
#endif

#define sleep(n) exec::ThreadId::sleep(n, exec::ThreadId::Seconds)

class LogThread : public exec::Thread {
	public:
		LogThread(const std::string &name, logger::Logger &log):exec::Thread(exec::Thread::KeepAroundAfterFinish), _name(name),_log(log) {start();}
		~LogThread() {}
	protected:
		virtual void *run() {
			for (int i = 0; i < RunCount; ++i) {
				logDetail(_log, _name + " Detail");
				sleep(0.001);
				logInfo(_log, _name + " Info");
				sleep(0.001);
				logWarn(_log, _name + " Warn");
				sleep(0.001);
				logError(_log, _name + " Error");
				sleep(0.001);
				try {
					AssertMessageException(false);
				} catch(const std::exception &exception) {
					logException(_log, exception, _name + " assert false");
				}
			}
			return NULL;
		}
	private:
		std::string _name;
		logger::Logger &_log;

};

int main(int /*argc*/, const char */*argv*/[]) {
	io::Path logFile = io::Path("bin") + "scratch" + "Logger" + "log.txt";

	if (logFile.isFile()) {
		logFile.remove();
	}

	logger::Logger log(io::Path("bin") + "scratch" + "Logger" + "log.txt");
	LogThread t1("one", log), t2("two", log), t3("three", log), t4("four", log), t5("five", log);

	sleep(0.001);
	logDetail(log, "Detail");
	sleep(0.001);
	logInfo(log, "Info");
	sleep(0.001);
	logWarn(log, "Warn");
	sleep(0.001);
	logError(log, "Error");
	sleep(0.001);
	try {
		AssertMessageException(false);
	} catch(const std::exception &exception) {
		logException(log, exception, "assert false");
	}
	t1.join();
	t2.join();
	t3.join();
	t4.join();
	t5.join();
	return 0;
}
