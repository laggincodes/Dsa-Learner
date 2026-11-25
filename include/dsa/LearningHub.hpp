#pragma once

#include "dsa/HistoryLogger.hpp"
#include "dsa/ProgressManager.hpp"
#include "dsa/Types.hpp"

#include <cstddef>
#include <map>
#include <mutex>
#include <string>
#include <vector>

namespace dsa {

struct UserPreferences {
    std::string leetcodeUsername;
    std::string learningGoal = "Complete Foundations & Arrays practice";
};

struct RoadmapTopic {
    std::string id;
    std::string stage; // "FOUNDATIONS", "DATA STRUCTURES", "ALGORITHMS"
    std::string title;
    std::string explanation;
    std::string difficulty; // "Beginner", "Intermediate", "Advanced"
    std::vector<std::string> prerequisites;
    std::string relatedVisualizer; // view id or view:tab
    std::vector<std::string> relatedProblemIds;
    bool completed = false;
};

struct PracticeProblem {
    PracticeProblem() = default;
    PracticeProblem(std::string id_, std::string title_, std::string topic_,
                    std::string difficulty_, std::string prompt_,
                    std::string hint_, std::string defaultApproach_)
        : id(std::move(id_)),
          title(std::move(title_)),
          topic(std::move(topic_)),
          difficulty(std::move(difficulty_)),
          prompt(std::move(prompt_)),
          hint(std::move(hint_)),
          defaultApproach(std::move(defaultApproach_)) {}

    std::string id;
    std::string title;
    std::string topic;
    std::string difficulty; // "Easy", "Medium", "Hard"
    std::string prompt;
    std::string hint;
    std::string defaultApproach;
    bool solved = false;
    bool bookmarked = false;
    bool needsRevision = false;
    std::size_t attempts = 0;
    std::string notes;
    std::string userApproach;
};

struct LeetCodeProblem {
    std::string id;
    std::string title;
    std::string url;
    std::string difficulty; // "Easy", "Medium", "Hard"
    std::string topic;
    std::string status; // "Not Started", "Attempted", "Solved", "Needs Revision"
    bool bookmarked = false;
    std::string notes;
    std::string updatedAt;
};

struct RevisionCard {
    std::string id;
    std::string category; // "Concept", "Complexity", "Algorithm"
    std::string front;
    std::string back;
    int intervalDays = 0;
    double easeFactor = 2.5;
    int repetitions = 0;
    std::string nextReviewDate;
    std::string lastRating = "New";
    std::size_t reviewCount = 0;
};

struct MistakeNote {
    std::string id;
    std::string title;
    std::string topic;
    std::string note;
    std::string createdAt;
};

struct ExternalResource {
    std::string id;
    std::string name;
    std::string category;
    std::string description;
    std::string url;
};

struct DayActivity {
    std::string date; // YYYY-MM-DD
    std::size_t count = 0;
};

struct TopicPracticeCount {
    std::string topic;
    std::size_t count = 0;
};

struct AnalyticsSnapshot {
    std::size_t problemsSolved = 0;
    std::size_t problemsAttempted = 0;
    std::size_t leetcodeSolved = 0;
    std::size_t leetcodeTotal = 0;
    std::size_t roadmapCompleted = 0;
    std::size_t roadmapTotal = 0;
    double topicCompletionPercent = 0.0;
    std::size_t quizAttempts = 0;
    double quizAvgAccuracy = 0.0;
    std::size_t revisionTotalReviews = 0;
    std::size_t revisionDueCount = 0;
    std::size_t needsRevisionCount = 0;
    std::size_t mistakesCount = 0;
    std::vector<DayActivity> weeklyActivity; // last 7 days
    std::vector<DayActivity> dailyHeatmap;   // last 28 days
    std::vector<TopicPracticeCount> mostPracticedTopics;
};

class LearningHub {
public:
    explicit LearningHub(std::string filePath = "dsa_ecosystem.dat");

    void setPreferences(const std::string& leetcodeUsername, const std::string& learningGoal);
    UserPreferences preferences() const;

    void toggleRoadmapTopic(const std::string& topicId, bool completed);
    std::vector<RoadmapTopic> roadmap() const;

    void updatePracticeProblem(const std::string& problemId, const std::string& action,
                               const std::string& notes, const std::string& approach);
    std::vector<PracticeProblem> practiceProblems() const;

    LeetCodeProblem addLeetCodeProblem(const std::string& title, const std::string& url,
                                       const std::string& difficulty, const std::string& topic,
                                       const std::string& status, const std::string& notes);
    LeetCodeProblem updateLeetCodeProblem(const std::string& id, const std::string& status,
                                          bool toggleBookmark, bool hasBookmark, bool bookmarkVal,
                                          bool hasNotes, const std::string& notes,
                                          const std::string& difficulty, const std::string& topic);
    void deleteLeetCodeProblem(const std::string& id);
    std::vector<LeetCodeProblem> leetcodeProblems() const;

    RevisionCard reviewFlashcard(const std::string& cardId, const std::string& rating);
    std::vector<RevisionCard> flashcards() const;

    MistakeNote addMistake(const std::string& title, const std::string& topic, const std::string& note);
    void removeMistake(const std::string& id);
    std::vector<MistakeNote> mistakes() const;

    std::vector<ExternalResource> resources() const;

    void recordActivityToday(std::size_t units = 1);
    AnalyticsSnapshot computeAnalytics(const ProgressManager& progress, const HistoryLogger& logger) const;
    const std::string& filePath() const noexcept { return filePath_; }

private:
    void initDefaults();
    void loadFromDisk();
    void saveToDisk() const;
    static std::string todayDate();
    static std::string addDaysToToday(int days);
    static std::string escapeField(const std::string& s);
    static std::string unescapeField(const std::string& s);
    static std::vector<std::string> splitEscaped(const std::string& line);

    std::string filePath_;
    UserPreferences prefs_;
    std::vector<RoadmapTopic> roadmap_;
    std::vector<PracticeProblem> practice_;
    std::vector<LeetCodeProblem> leetcode_;
    std::vector<RevisionCard> flashcards_;
    std::vector<MistakeNote> mistakes_;
    std::vector<ExternalResource> resources_;
    std::map<std::string, std::size_t> dailyActivity_;
    std::size_t nextLcSeq_ = 1;
    std::size_t nextMistakeSeq_ = 1;
    mutable std::mutex mutex_;
};

} // namespace dsa
