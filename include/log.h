#include<fstream>
#include<string>
#include"helpers.h"
#include<iomanip>
#include<sstream>
class Log {
public:
    enum class Severity {
                         LOG_FATAL = 0,
                         LOG_ERROR = 1,
                         LOG_WARNING = 2,
                         LOG_DEBUG = 3,
                         LOG_INFO = 4
    };
    static void init_log(Severity severity, const string & logFileName);
    static void log_dbg(Severity level, const string & msg);
    static void log_destroy();
private:
    static string getSeverityString(Severity severity);
    static fstream logStream;
    static Severity sev;
};
