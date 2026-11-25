#pragma once

#include "dsa/Types.hpp"

#include <map>
#include <mutex>
#include <string>
#include <vector>

namespace dsa {

class ProgressManager {
public:
    explicit ProgressManager(std::string filePath = "dsa_progress.dat");

    void markModule(const std::string& moduleId, bool completed = true);
    void recordQuiz(const QuizRecord& record);
    ProgressSnapshot snapshot() const;
    void reset();
    const std::string& filePath() const noexcept { return filePath_; }

private:
    void initDefaultModules();
    void loadFromDisk();
    void saveToDisk() const;
    static std::string sanitize(const std::string& s);

    std::string filePath_;
    std::vector<ModuleProgress> modules_;
    std::vector<QuizRecord> quizHistory_;
    mutable std::mutex mutex_;
};

} // namespace dsa
