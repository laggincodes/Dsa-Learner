#pragma once

#include "dsa/LearningHub.hpp"

#include <cstddef>
#include <map>
#include <mutex>
#include <string>
#include <unordered_map>
#include <vector>

namespace dsa {

struct CatalogProblem {
    int id = 0;
    std::string title;
    std::string slug;
    std::string difficulty; // "Easy", "Medium", "Hard"
    std::vector<std::string> topicTags;
    std::string url;
    bool isPremium = false;
};

struct ProblemProgressRecord {
    std::string slug;
    std::string status = "Not Started"; // "Not Started", "Attempted", "Solved", "Needs Revision"
    std::string firstAttemptDate;
    std::string latestAttemptDate;
    std::size_t attemptCount = 0;
    bool accepted = false;
    std::string source = "manual_marked"; // "manual_marked", "synced_recent", "manual_imported", "extension_detected"
    bool bookmarked = false;
    bool needsRevision = false;
    int revisionIntervalDays = 0;
    double revisionEase = 2.5;
    int revisionReps = 0;
    std::string nextReviewDate;
    std::string lastRating = "New";
    std::size_t reviewCount = 0;
};

struct ProblemNotes {
    std::string slug;
    std::string intuition;
    std::string bruteForce;
    std::string optimized;
    std::string timeComplexity;
    std::string spaceComplexity;
    std::string mistakesMade;
    std::string observations;
    std::string cppSnippet;
    std::string revisionComments;
    std::string updatedAt;
};

struct SavedSolution {
    std::string id;
    std::string slug;
    int problemId = 0;
    std::string problemTitle;
    std::string approachName;
    std::string timeComplexity;
    std::string spaceComplexity;
    std::string explanation;
    std::string cppCode;
    std::vector<std::string> topics;
    std::string createdAt;
};

struct SubmissionRecord {
    std::string id;
    std::string slug;
    int problemId = 0;
    std::string title;
    std::string difficulty;
    std::string timestamp;
    std::string date; // YYYY-MM-DD
    std::string source; // "synced_recent", "manual_imported", "extension_detected", "manual_marked"
};

struct LeetCodeProfileStats {
    std::string username;
    int totalSolved = 0;
    int easySolved = 0;
    int mediumSolved = 0;
    int hardSolved = 0;
    int ranking = 0;
    std::string lastProfileSync;
    std::string lastCatalogSync;
    std::string lastSyncError;
    std::string extensionLastSeen;
    std::size_t extensionEventsCount = 0;
};

struct TopicProgressItem {
    std::string topic;
    std::size_t totalProblems = 0;
    std::size_t solvedProblems = 0;
    std::size_t attemptedProblems = 0;
    std::size_t remainingProblems = 0;
    std::size_t easySolved = 0;
    std::size_t mediumSolved = 0;
    std::size_t hardSolved = 0;
    std::size_t easyTotal = 0;
    std::size_t mediumTotal = 0;
    std::size_t hardTotal = 0;
    std::size_t goalTarget = 5;
    double completionPercent = 0.0;
};

struct CatalogueQuery {
    std::string search;
    std::string difficulty = "all";
    std::string topic = "all";
    std::string availability = "all"; // "all", "free", "premium"
    std::string status = "all";       // "all", "Not Started", "Attempted", "Solved", "Needs Revision", "Bookmarked"
    std::string sortBy = "id";        // "id", "title", "difficulty"
    std::string sortDir = "asc";      // "asc", "desc"
    std::size_t page = 1;
    std::size_t pageSize = 50;
};

struct CatalogueItemView {
    CatalogProblem problem;
    ProblemProgressRecord progress;
    bool hasNotes = false;
    std::size_t solutionCount = 0;
};

struct CataloguePageResult {
    std::vector<CatalogueItemView> items;
    std::size_t totalFiltered = 0;
    std::size_t totalCatalogue = 0;
    std::size_t page = 1;
    std::size_t pageSize = 50;
    std::size_t totalPages = 1;
    std::vector<std::string> availableTopics;
};

struct ProblemDetailView {
    CatalogProblem problem;
    ProblemProgressRecord progress;
    ProblemNotes notes;
    std::vector<SavedSolution> solutions;
    std::vector<SubmissionRecord> submissions;
};

struct IntelligenceAnalytics {
    std::size_t totalCatalogue = 0;
    std::size_t totalSolved = 0;
    std::size_t totalAttempted = 0;
    std::size_t solvedEasy = 0;
    std::size_t solvedMedium = 0;
    std::size_t solvedHard = 0;
    double overallTopicCompletionPercent = 0.0;
    std::size_t currentStreak = 0;
    std::size_t revisionCompleted = 0;
    std::size_t revisionDue = 0;
    std::size_t needsRevisionCount = 0;
    std::size_t totalSolutions = 0;
    std::size_t totalNotes = 0;
    std::vector<DayActivity> weeklyActivity;
    std::vector<DayActivity> dailyActivityHeatmap;
    std::vector<TopicProgressItem> weakestTopics;
    std::vector<TopicProgressItem> mostPracticedTopics;
};

class IntelligenceHub {
public:
    IntelligenceHub(std::string catalogPath = "dsa_catalogue.dat",
                    std::string statePath = "dsa_intelligence.dat");

    // Catalogue import & querying
    std::size_t syncLeetCodeCatalogue(const std::string& graphqlEndpoint = "https://leetcode.com/graphql");
    std::size_t importCatalogueJson(const std::string& jsonText);
    CataloguePageResult queryCatalogue(const CatalogueQuery& query) const;
    ProblemDetailView getProblemDetail(const std::string& slug) const;

    // LeetCode profile & submissions
    LeetCodeProfileStats syncLeetCodeProfile(const std::string& username,
                                             const std::string& graphqlEndpoint = "https://leetcode.com/graphql");
    std::size_t importSubmissionsJson(const std::string& jsonText);
    LeetCodeProfileStats profileStats() const;
    std::vector<SubmissionRecord> recentSubmissions(std::size_t limit = 30) const;

    // Extension detection
    SubmissionRecord recordExtensionSubmission(int problemId, const std::string& title,
                                               const std::string& slug, const std::string& difficulty,
                                               const std::string& timestampStr);

    // Status tracking, notes & solutions
    ProblemProgressRecord updateProblemStatus(const std::string& slug, const std::string& status,
                                                bool hasBookmark, bool bookmarkVal,
                                                bool toggleBookmark, bool hasNeedsRevision,
                                                bool needsRevisionVal);
    ProblemNotes saveProblemNotes(const ProblemNotes& notes);
    std::vector<ProblemNotes> searchNotes(const std::string& query) const;
    SavedSolution addSolution(const SavedSolution& sol);
    void deleteSolution(const std::string& solutionId);
    std::vector<SavedSolution> searchSolutions(const std::string& query, const std::string& topic,
                                               const std::string& complexity) const;

    // Topic progress & goals
    void setTopicGoal(const std::string& topic, std::size_t goalTarget);
    std::vector<TopicProgressItem> computeTopicProgress() const;

    // Revision system
    ProblemProgressRecord scheduleProblemRevision(const std::string& slug, bool needsRevision);
    ProblemProgressRecord reviewProblemRevision(const std::string& slug, const std::string& rating);
    std::vector<CatalogueItemView> revisionQueue() const;

    // Real analytics
    IntelligenceAnalytics computeAnalytics() const;

private:
    void loadCatalogue();
    void saveCatalogue() const;
    void loadState();
    void saveState() const;
    bool upsertCatalogProblem(CatalogProblem prob);
    bool addSubmissionUnique(const SubmissionRecord& sub);
    std::string resolveSlug(const std::string& slugOrId, int problemId, const std::string& title);

    static std::string todayDate();
    static std::string nowTimestamp();
    static std::string addDaysToToday(int days);
    static std::string epochToDate(long long epochSec);
    static std::string epochToTimestamp(long long epochSec);
    static std::string escapeField(const std::string& s);
    static std::string unescapeField(const std::string& s);
    static std::vector<std::string> splitEscaped(const std::string& line);
    static std::string toLower(std::string s);

    std::string catalogPath_;
    std::string statePath_;

    std::vector<CatalogProblem> catalogue_;
    std::unordered_map<std::string, std::size_t> slugToIdx_;
    std::unordered_map<int, std::size_t> idToIdx_;

    std::map<std::string, ProblemProgressRecord> tracker_;
    std::map<std::string, ProblemNotes> notes_;
    std::vector<SavedSolution> solutions_;
    std::vector<SubmissionRecord> submissions_;
    std::map<std::string, std::size_t> topicGoals_;
    std::map<std::string, std::size_t> dailyActivity_;
    LeetCodeProfileStats profile_;
    std::size_t nextSolSeq_ = 1;
    std::size_t nextSubSeq_ = 1;

    mutable std::mutex mutex_;
};

} // namespace dsa
