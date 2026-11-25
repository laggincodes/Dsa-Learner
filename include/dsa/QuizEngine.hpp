#pragma once

#include "dsa/Types.hpp"

#include <cstddef>
#include <map>
#include <mutex>
#include <string>
#include <vector>

namespace dsa {

class QuizEngine {
public:
    QuizEngine();

    struct StartSessionResult {
        std::string sessionId;
        std::vector<QuizQuestion> questions;
    };

    StartSessionResult startSession(std::size_t count = 8, const std::string& category = "all");
    QuizAnswerResult submitAnswer(const std::string& sessionId, int questionId, int selectedIndex);
    QuizRecord finishSession(const std::string& sessionId);

private:
    struct QuizSession {
        std::string category;
        std::vector<int> questionIds;
        std::map<int, int> answers;
        std::size_t score = 0;
        bool finished = false;
    };

    const QuizQuestion& findQuestion(int id) const;

    std::vector<QuizQuestion> bank_;
    std::map<std::string, QuizSession> sessions_;
    std::size_t nextSessionSeq_ = 1;
    mutable std::mutex mutex_;
};

} // namespace dsa
