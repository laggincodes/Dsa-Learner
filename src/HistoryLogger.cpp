#include "dsa/HistoryLogger.hpp"

#include <algorithm>
#include <chrono>
#include <ctime>
#include <fstream>
#include <iomanip>
#include <sstream>

namespace dsa {

HistoryLogger::HistoryLogger(std::string filePath)
    : filePath_(std::move(filePath)) {}

HistoryLogger::~HistoryLogger() = default;

std::string HistoryLogger::currentTimestamp() {
    const auto now = std::chrono::system_clock::now();
    const std::time_t t = std::chrono::system_clock::to_time_t(now);
    std::tm tmBuf{};
    localtime_r(&t, &tmBuf);
    std::ostringstream oss;
    oss << std::put_time(&tmBuf, "%Y-%m-%d %H:%M:%S");
    return oss.str();
}

std::string HistoryLogger::sanitizeField(const std::string& input) {
    std::string clean = input;
    std::replace(clean.begin(), clean.end(), '|', '/');
    std::replace(clean.begin(), clean.end(), '\n', ' ');
    std::replace(clean.begin(), clean.end(), '\r', ' ');
    return clean;
}

void HistoryLogger::log(const std::string& category, const std::string& operation, const std::string& summary) {
    log(OperationRecord{currentTimestamp(), category, operation, summary});
}

void HistoryLogger::log(const OperationRecord& record) {
    std::lock_guard<std::mutex> lock(mutex_);
    std::ofstream out(filePath_, std::ios::app);
    if (!out.is_open()) {
        throw FileIOException("Failed to open log file for writing: " + filePath_);
    }
    const std::string ts = record.timestamp.empty() ? currentTimestamp() : sanitizeField(record.timestamp);
    out << ts << "|"
        << sanitizeField(record.category) << "|"
        << sanitizeField(record.operation) << "|"
        << sanitizeField(record.summary) << "\n";
    out.flush();
}

std::vector<OperationRecord> HistoryLogger::readRecent(std::size_t limit) const {
    std::lock_guard<std::mutex> lock(mutex_);
    std::ifstream in(filePath_);
    if (!in.is_open()) {
        return {};
    }

    std::vector<OperationRecord> records;
    std::string line;
    while (std::getline(in, line)) {
        if (line.empty()) {
            continue;
        }
        std::vector<std::string> parts;
        std::stringstream ss(line);
        std::string item;
        while (std::getline(ss, item, '|')) {
            parts.push_back(item);
        }
        if (parts.size() >= 4) {
            records.push_back(OperationRecord{parts[0], parts[1], parts[2], parts[3]});
        }
    }

    std::reverse(records.begin(), records.end());
    if (records.size() > limit) {
        records.resize(limit);
    }
    return records;
}

void HistoryLogger::clear() {
    std::lock_guard<std::mutex> lock(mutex_);
    std::ofstream out(filePath_, std::ios::trunc);
    if (!out.is_open()) {
        throw FileIOException("Failed to clear log file: " + filePath_);
    }
}

} // namespace dsa
