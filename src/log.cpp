#include"log.h"
void Log::init_log(Severity severity) {
    // Check if the file name is valid
    stringstream s;
    time_t t = time(NULL);
    s << put_time(localtime(&t), "log/%Y-%m-%e-server-log");
    if (logStream.is_open()) logStream.close();
    logStream.open(s.str(), fstream::out | fstream::app);
    if (!logStream) {
        perror("Stream failed");
    }
    sev = severity;
}

string Log::getSeverityString(Severity severity) {
    switch (severity) {
    case Severity::LOG_FATAL:
        return "Fatal";
    case Severity::LOG_ERROR:
        return "Error";
    case Severity::LOG_WARNING:
        return "Warning";
    case Severity::LOG_DEBUG:
        return "Debug";
    case Severity::LOG_INFO:
        return "Info";
    }
    return "Not Recognized";
}

void Log::log_dbg(Severity level, const string & msg) {
    // If right severity level to display message
    if (level <= sev) {
        time_t t = time(NULL);
        logStream << put_time(localtime(&t), "%c") << "    " << getSeverityString(level) << "    " << msg << endl; // Display time in standard format
    }
}

void Log::log_destroy() {
    if (logStream.is_open()) logStream.close();
}
fstream Log::logStream = fstream();
Log::Severity Log::sev = Log::Severity::LOG_INFO;
