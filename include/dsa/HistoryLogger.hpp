#pragma once

#include "dsa/Types.hpp"

#include <cstddef>
#include <mutex>
#include <string>
#include <vector>

namespace dsa {

class HistoryLogger {
public:
    explicit HistoryLogger(std::string filePath = "dsa_history.log");
    ~HistoryLogger();

    // Function overloading for logging
    void log(const std::string& category, const std::string& operation, const std::string& summary);
    void log(const OperationRecord& record);

    std::vector<OperationRecord> readRecent(std::size_t limit = 25) const;
    void clear();
    const std::string& filePath() const noexcept { return filePath_; }

private:
    static std::string currentTimestamp();
    static std::string sanitizeField(const std::string& input);

    std::string filePath_;
    mutable std::mutex mutex_;
};

} // namespace dsa
