#include <stdio.h>
#include "libernet/Logger.h"

int main(int argc, const char *argv[]) {
#ifndef __Tracer_h__
exec::ThreadId::sleep(1.05, exec::ThreadId::Seconds);
#endif
	log::Logger log(io::Path("bin") + "scratch" + "Logger");

	logDetail(log, "Detail");
	logInfo(log, "Info");
	logWarn(log, "Warn");
	logError(log, "Error");
	try {
		AssertMessageException(false);
	} catch(const std::exception &exception) {
		logException(log, exception, "test false");
	}
	return 0;
}
