#include "dsa/IntelligenceHub.hpp"
#include "dsa/Json.hpp"

#include <algorithm>
#include <chrono>
#include <cctype>
#include <curl/curl.h>
#include <ctime>
#include <fstream>
#include <iomanip>
#include <set>
#include <sstream>

namespace dsa {

namespace {

std::size_t curlWriteCallback(void* contents, std::size_t size, std::size_t nmemb, void* userp) {
    const std::size_t total = size * nmemb;
    auto* out = static_cast<std::string*>(userp);
    out->append(static_cast<const char*>(contents), total);
    return total;
}

std::string httpPostJson(const std::string& url, const std::string& jsonBody) {
    CURL* curl = curl_easy_init();
    if (!curl) {
        throw DSAException("Failed to initialize libcurl HTTP client.", "NETWORK_ERROR");
    }

    std::string response;
    struct curl_slist* headers = nullptr;
    headers = curl_slist_append(headers, "Content-Type: application/json");
    headers = curl_slist_append(headers, "Accept: application/json");
    headers = curl_slist_append(headers, "Referer: https://leetcode.com/");

    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_POST, 1L);
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, jsonBody.c_str());
    curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, static_cast<long>(jsonBody.size()));
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    curl_easy_setopt(curl, CURLOPT_USERAGENT, "DSA-Manager-Lite/1.0");
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, curlWriteCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 15L);
    curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 8L);

    CURLcode res = curl_easy_perform(curl);
    long httpCode = 0;
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &httpCode);

    curl_slist_free_all(headers);
    curl_easy_cleanup(curl);

    if (res != CURLE_OK) {
        throw DSAException(std::string("Network sync failed: ") + curl_easy_strerror(res), "NETWORK_ERROR");
    }
    if (httpCode < 200 || httpCode >= 300) {
        throw DSAException("Remote server returned HTTP " + std::to_string(httpCode) + ".", "NETWORK_ERROR");
    }
    return response;
}

std::string slugifyTitle(const std::string& title) {
    std::string out;
    bool prevDash = false;
    for (char c : title) {
        if (std::isalnum(static_cast<unsigned char>(c))) {
            out.push_back(static_cast<char>(std::tolower(static_cast<unsigned char>(c))));
            prevDash = false;
        } else if (!prevDash && !out.empty()) {
            out.push_back('-');
            prevDash = true;
        }
    }
    while (!out.empty() && out.back() == '-') out.pop_back();
    return out;
}

int difficultyRank(const std::string& diff) {
    if (diff == "Easy") return 1;
    if (diff == "Medium") return 2;
    if (diff == "Hard") return 3;
    return 4;
}

std::string normalizeDifficulty(const std::string& d) {
    if (d == "Easy" || d == "easy" || d == "EASY" || d == "1") return "Easy";
    if (d == "Medium" || d == "medium" || d == "MEDIUM" || d == "2") return "Medium";
    if (d == "Hard" || d == "hard" || d == "HARD" || d == "3") return "Hard";
    return "Medium";
}

bool isValidStatus(const std::string& s) {
    return s == "Not Started" || s == "Attempted" || s == "Solved" || s == "Needs Revision";
}

} // namespace

IntelligenceHub::IntelligenceHub(std::string catalogPath, std::string statePath)
    : catalogPath_(std::move(catalogPath)), statePath_(std::move(statePath)) {
    loadCatalogue();
    loadState();
}

std::string IntelligenceHub::todayDate() {
    const auto now = std::chrono::system_clock::now();
    const std::time_t t = std::chrono::system_clock::to_time_t(now);
    std::tm tmBuf{};
    localtime_r(&t, &tmBuf);
    std::ostringstream oss;
    oss << std::put_time(&tmBuf, "%Y-%m-%d");
    return oss.str();
}

std::string IntelligenceHub::nowTimestamp() {
    const auto now = std::chrono::system_clock::now();
    const std::time_t t = std::chrono::system_clock::to_time_t(now);
    std::tm tmBuf{};
    localtime_r(&t, &tmBuf);
    std::ostringstream oss;
    oss << std::put_time(&tmBuf, "%Y-%m-%d %H:%M:%S");
    return oss.str();
}

std::string IntelligenceHub::addDaysToToday(int days) {
    const auto target = std::chrono::system_clock::now() + std::chrono::hours(24 * days);
    const std::time_t t = std::chrono::system_clock::to_time_t(target);
    std::tm tmBuf{};
    localtime_r(&t, &tmBuf);
    std::ostringstream oss;
    oss << std::put_time(&tmBuf, "%Y-%m-%d");
    return oss.str();
}

std::string IntelligenceHub::epochToDate(long long epochSec) {
    const std::time_t t = static_cast<std::time_t>(epochSec);
    std::tm tmBuf{};
    localtime_r(&t, &tmBuf);
    std::ostringstream oss;
    oss << std::put_time(&tmBuf, "%Y-%m-%d");
    return oss.str();
}

std::string IntelligenceHub::epochToTimestamp(long long epochSec) {
    const std::time_t t = static_cast<std::time_t>(epochSec);
    std::tm tmBuf{};
    localtime_r(&t, &tmBuf);
    std::ostringstream oss;
    oss << std::put_time(&tmBuf, "%Y-%m-%d %H:%M:%S");
    return oss.str();
}

std::string IntelligenceHub::toLower(std::string s) {
    for (char& c : s) {
        c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    }
    return s;
}

std::string IntelligenceHub::escapeField(const std::string& s) {
    std::string out;
    out.reserve(s.size());
    for (char c : s) {
        if (c == '\\') out += "\\\\";
        else if (c == '|') out += "\\p";
        else if (c == '\n') out += "\\n";
        else if (c == '\r') out += "";
        else out.push_back(c);
    }
    return out;
}

std::string IntelligenceHub::unescapeField(const std::string& s) {
    std::string out;
    out.reserve(s.size());
    for (std::size_t i = 0; i < s.size(); ++i) {
        if (s[i] == '\\' && i + 1 < s.size()) {
            char n = s[i + 1];
            if (n == '\\') { out.push_back('\\'); ++i; }
            else if (n == 'p') { out.push_back('|'); ++i; }
            else if (n == 'n') { out.push_back('\n'); ++i; }
            else out.push_back(s[i]);
        } else {
            out.push_back(s[i]);
        }
    }
    return out;
}

std::vector<std::string> IntelligenceHub::splitEscaped(const std::string& line) {
    std::vector<std::string> parts;
    std::string curr;
    for (char c : line) {
        if (c == '|') {
            parts.push_back(unescapeField(curr));
            curr.clear();
        } else {
            curr.push_back(c);
        }
    }
    parts.push_back(unescapeField(curr));
    return parts;
}

bool IntelligenceHub::upsertCatalogProblem(CatalogProblem prob) {
    if (prob.slug.empty() && !prob.title.empty()) {
        prob.slug = slugifyTitle(prob.title);
    }
    if (prob.slug.empty() || prob.title.empty() || prob.id <= 0) {
        return false;
    }
    if (prob.url.empty()) {
        prob.url = "https://leetcode.com/problems/" + prob.slug + "/";
    }
    prob.difficulty = normalizeDifficulty(prob.difficulty);

    auto slugIt = slugToIdx_.find(prob.slug);
    if (slugIt != slugToIdx_.end()) {
        // Update metadata if newer import provides tags or info
        CatalogProblem& existing = catalogue_[slugIt->second];
        if (!prob.topicTags.empty()) existing.topicTags = std::move(prob.topicTags);
        existing.isPremium = prob.isPremium;
        existing.difficulty = prob.difficulty;
        return false; // not a new duplicate
    }
    auto idIt = idToIdx_.find(prob.id);
    if (idIt != idToIdx_.end()) {
        return false; // duplicate frontend ID prevented
    }

    const std::size_t idx = catalogue_.size();
    slugToIdx_[prob.slug] = idx;
    idToIdx_[prob.id] = idx;
    catalogue_.push_back(std::move(prob));
    return true;
}

void IntelligenceHub::loadCatalogue() {
    std::ifstream in(catalogPath_);
    if (!in.is_open()) return;
    std::string line;
    while (std::getline(in, line)) {
        if (line.empty()) continue;
        auto parts = splitEscaped(line);
        if (parts.size() < 7 || parts[0] != "PROB") continue;
        try {
            CatalogProblem p;
            p.id = std::stoi(parts[1]);
            p.title = parts[2];
            p.slug = parts[3];
            p.difficulty = parts[4];
            p.isPremium = (parts[5] == "1");
            p.url = parts[6];
            if (parts.size() >= 8 && !parts[7].empty()) {
                std::stringstream ss(parts[7]);
                std::string tag;
                while (std::getline(ss, tag, ',')) {
                    if (!tag.empty()) p.topicTags.push_back(tag);
                }
            }
            upsertCatalogProblem(std::move(p));
        } catch (...) {
        }
    }
}

void IntelligenceHub::saveCatalogue() const {
    std::ofstream out(catalogPath_, std::ios::trunc);
    if (!out.is_open()) {
        throw FileIOException("Failed to write catalogue file: " + catalogPath_);
    }
    for (const auto& p : catalogue_) {
        std::ostringstream tags;
        for (std::size_t i = 0; i < p.topicTags.size(); ++i) {
            if (i > 0) tags << ",";
            tags << p.topicTags[i];
        }
        out << "PROB|" << p.id << "|"
            << escapeField(p.title) << "|"
            << escapeField(p.slug) << "|"
            << escapeField(p.difficulty) << "|"
            << (p.isPremium ? "1" : "0") << "|"
            << escapeField(p.url) << "|"
            << escapeField(tags.str()) << "\n";
    }
    out.flush();
}

void IntelligenceHub::loadState() {
    std::ifstream in(statePath_);
    if (!in.is_open()) return;
    std::string line;
    while (std::getline(in, line)) {
        if (line.empty()) continue;
        auto parts = splitEscaped(line);
        if (parts.empty()) continue;
        const std::string& tag = parts[0];

        if (tag == "PROFILE" && parts.size() >= 11) {
            try {
                profile_.username = parts[1];
                profile_.totalSolved = std::stoi(parts[2]);
                profile_.easySolved = std::stoi(parts[3]);
                profile_.mediumSolved = std::stoi(parts[4]);
                profile_.hardSolved = std::stoi(parts[5]);
                profile_.ranking = std::stoi(parts[6]);
                profile_.lastProfileSync = parts[7];
                profile_.lastCatalogSync = parts[8];
                profile_.lastSyncError = parts[9];
                profile_.extensionLastSeen = parts[10];
                if (parts.size() >= 12) {
                    profile_.extensionEventsCount = static_cast<std::size_t>(std::stoul(parts[11]));
                }
            } catch (...) {}
        } else if (tag == "TRACK" && parts.size() >= 16) {
            try {
                ProblemProgressRecord r;
                r.slug = parts[1];
                r.status = parts[2];
                r.firstAttemptDate = parts[3];
                r.latestAttemptDate = parts[4];
                r.attemptCount = static_cast<std::size_t>(std::stoul(parts[5]));
                r.accepted = (parts[6] == "1");
                r.source = parts[7];
                r.bookmarked = (parts[8] == "1");
                r.needsRevision = (parts[9] == "1");
                r.revisionIntervalDays = std::stoi(parts[10]);
                r.revisionEase = std::stod(parts[11]);
                r.revisionReps = std::stoi(parts[12]);
                r.nextReviewDate = parts[13];
                r.lastRating = parts[14];
                r.reviewCount = static_cast<std::size_t>(std::stoul(parts[15]));
                tracker_[r.slug] = std::move(r);
            } catch (...) {}
        } else if (tag == "NOTE" && parts.size() >= 12) {
            ProblemNotes n;
            n.slug = parts[1];
            n.intuition = parts[2];
            n.bruteForce = parts[3];
            n.optimized = parts[4];
            n.timeComplexity = parts[5];
            n.spaceComplexity = parts[6];
            n.mistakesMade = parts[7];
            n.observations = parts[8];
            n.cppSnippet = parts[9];
            n.revisionComments = parts[10];
            n.updatedAt = parts[11];
            notes_[n.slug] = std::move(n);
        } else if (tag == "SOL" && parts.size() >= 12) {
            try {
                SavedSolution s;
                s.id = parts[1];
                s.slug = parts[2];
                s.problemId = std::stoi(parts[3]);
                s.problemTitle = parts[4];
                s.approachName = parts[5];
                s.timeComplexity = parts[6];
                s.spaceComplexity = parts[7];
                s.explanation = parts[8];
                s.cppCode = parts[9];
                if (!parts[10].empty()) {
                    std::stringstream ss(parts[10]);
                    std::string t;
                    while (std::getline(ss, t, ',')) {
                        if (!t.empty()) s.topics.push_back(t);
                    }
                }
                s.createdAt = parts[11];
                solutions_.push_back(std::move(s));
                if (parts[1].rfind("sol-", 0) == 0) {
                    std::size_t seq = static_cast<std::size_t>(std::stoul(parts[1].substr(4)));
                    if (seq >= nextSolSeq_) nextSolSeq_ = seq + 1;
                }
            } catch (...) {}
        } else if (tag == "SUB" && parts.size() >= 8) {
            try {
                SubmissionRecord sub;
                sub.id = parts[1];
                sub.slug = parts[2];
                sub.problemId = std::stoi(parts[3]);
                sub.title = parts[4];
                sub.difficulty = parts[5];
                sub.timestamp = parts[6];
                sub.date = parts[7];
                if (parts.size() >= 9) sub.source = parts[8];
                submissions_.push_back(std::move(sub));
                if (parts[1].rfind("sub-", 0) == 0) {
                    std::size_t seq = static_cast<std::size_t>(std::stoul(parts[1].substr(4)));
                    if (seq >= nextSubSeq_) nextSubSeq_ = seq + 1;
                }
            } catch (...) {}
        } else if (tag == "GOAL" && parts.size() >= 3) {
            try {
                topicGoals_[parts[1]] = static_cast<std::size_t>(std::stoul(parts[2]));
            } catch (...) {}
        } else if (tag == "DAY" && parts.size() >= 3) {
            try {
                dailyActivity_[parts[1]] = static_cast<std::size_t>(std::stoul(parts[2]));
            } catch (...) {}
        }
    }
}

void IntelligenceHub::saveState() const {
    std::ofstream out(statePath_, std::ios::trunc);
    if (!out.is_open()) {
        throw FileIOException("Failed to write intelligence state file: " + statePath_);
    }

    out << "PROFILE|" << escapeField(profile_.username) << "|"
        << profile_.totalSolved << "|" << profile_.easySolved << "|"
        << profile_.mediumSolved << "|" << profile_.hardSolved << "|"
        << profile_.ranking << "|" << escapeField(profile_.lastProfileSync) << "|"
        << escapeField(profile_.lastCatalogSync) << "|" << escapeField(profile_.lastSyncError) << "|"
        << escapeField(profile_.extensionLastSeen) << "|" << profile_.extensionEventsCount << "\n";

    for (const auto& [slug, r] : tracker_) {
        out << "TRACK|" << escapeField(slug) << "|" << escapeField(r.status) << "|"
            << escapeField(r.firstAttemptDate) << "|" << escapeField(r.latestAttemptDate) << "|"
            << r.attemptCount << "|" << (r.accepted ? "1" : "0") << "|"
            << escapeField(r.source) << "|" << (r.bookmarked ? "1" : "0") << "|"
            << (r.needsRevision ? "1" : "0") << "|" << r.revisionIntervalDays << "|"
            << r.revisionEase << "|" << r.revisionReps << "|"
            << escapeField(r.nextReviewDate) << "|" << escapeField(r.lastRating) << "|"
            << r.reviewCount << "\n";
    }

    for (const auto& [slug, n] : notes_) {
        out << "NOTE|" << escapeField(slug) << "|"
            << escapeField(n.intuition) << "|" << escapeField(n.bruteForce) << "|"
            << escapeField(n.optimized) << "|" << escapeField(n.timeComplexity) << "|"
            << escapeField(n.spaceComplexity) << "|" << escapeField(n.mistakesMade) << "|"
            << escapeField(n.observations) << "|" << escapeField(n.cppSnippet) << "|"
            << escapeField(n.revisionComments) << "|" << escapeField(n.updatedAt) << "\n";
    }

    for (const auto& s : solutions_) {
        std::ostringstream topics;
        for (std::size_t i = 0; i < s.topics.size(); ++i) {
            if (i > 0) topics << ",";
            topics << s.topics[i];
        }
        out << "SOL|" << escapeField(s.id) << "|" << escapeField(s.slug) << "|"
            << s.problemId << "|" << escapeField(s.problemTitle) << "|"
            << escapeField(s.approachName) << "|" << escapeField(s.timeComplexity) << "|"
            << escapeField(s.spaceComplexity) << "|" << escapeField(s.explanation) << "|"
            << escapeField(s.cppCode) << "|" << escapeField(topics.str()) << "|"
            << escapeField(s.createdAt) << "\n";
    }

    for (const auto& sub : submissions_) {
        out << "SUB|" << escapeField(sub.id) << "|" << escapeField(sub.slug) << "|"
            << sub.problemId << "|" << escapeField(sub.title) << "|"
            << escapeField(sub.difficulty) << "|" << escapeField(sub.timestamp) << "|"
            << escapeField(sub.date) << "|" << escapeField(sub.source) << "\n";
    }

    for (const auto& [topic, goal] : topicGoals_) {
        out << "GOAL|" << escapeField(topic) << "|" << goal << "\n";
    }
    for (const auto& [day, cnt] : dailyActivity_) {
        out << "DAY|" << escapeField(day) << "|" << cnt << "\n";
    }
    out.flush();
}

std::size_t IntelligenceHub::importCatalogueJson(const std::string& jsonText) {
    json::JsonValue root = json::parse(jsonText);
    std::lock_guard<std::mutex> lock(mutex_);

    std::vector<CatalogProblem> incoming;
    auto parseQuestionObj = [&](const json::JsonValue& q) {
        CatalogProblem p;
        if (q.has("frontendQuestionId")) {
            const auto* fp = q.getPtr("frontendQuestionId");
            if (fp && fp->type == json::JsonValue::Type::String) {
                try { p.id = std::stoi(fp->strVal); } catch (...) {}
            } else if (fp && fp->type == json::JsonValue::Type::Int) {
                p.id = static_cast<int>(fp->intVal);
            }
        } else if (q.has("id")) {
            p.id = static_cast<int>(q.getInt("id", 0));
        }
        p.title = q.getString("title");
        p.slug = q.getString("titleSlug", q.getString("slug"));
        p.difficulty = q.getString("difficulty", "Medium");
        p.isPremium = q.getBool("paidOnly", q.getBool("isPremium", false));
        p.url = q.getString("url");

        if (const auto* tagsPtr = q.getPtr("topicTags")) {
            if (tagsPtr->isArray()) {
                for (const auto& t : tagsPtr->arrVal) {
                    if (t.isObject() && t.has("name")) p.topicTags.push_back(t.getString("name"));
                    else if (t.type == json::JsonValue::Type::String) p.topicTags.push_back(t.strVal);
                }
            }
        }
        if (p.id > 0 && !p.title.empty()) {
            incoming.push_back(std::move(p));
        }
    };

    if (root.isArray()) {
        for (const auto& item : root.arrVal) {
            if (item.isObject()) parseQuestionObj(item);
        }
    } else if (root.isObject()) {
        if (const auto* data = root.getPtr("data")) {
            const auto* pql = data->getPtr("problemsetQuestionList");
            if (!pql) pql = data->getPtr("questionList");
            if (pql) {
                if (const auto* qs = pql->getPtr("questions")) {
                    for (const auto& q : qs->arrVal) {
                        if (q.isObject()) parseQuestionObj(q);
                    }
                } else if (const auto* dataArr = pql->getPtr("data")) {
                    for (const auto& q : dataArr->arrVal) {
                        if (q.isObject()) parseQuestionObj(q);
                    }
                }
            }
        } else if (const auto* pairs = root.getPtr("stat_status_pairs")) {
            for (const auto& pair : pairs->arrVal) {
                const auto* stat = pair.getPtr("stat");
                const auto* diff = pair.getPtr("difficulty");
                if (!stat) continue;
                CatalogProblem p;
                p.id = static_cast<int>(stat->getInt("frontend_question_id", 0));
                p.title = stat->getString("question__title");
                p.slug = stat->getString("question__title_slug");
                p.isPremium = pair.getBool("paid_only", false);
                int lvl = diff ? static_cast<int>(diff->getInt("level", 2)) : 2;
                p.difficulty = (lvl == 1) ? "Easy" : (lvl == 3 ? "Hard" : "Medium");
                if (p.id > 0 && !p.title.empty()) incoming.push_back(std::move(p));
            }
        } else if (const auto* qs = root.getPtr("questions")) {
            for (const auto& q : qs->arrVal) {
                if (q.isObject()) parseQuestionObj(q);
            }
        }
    }

    if (incoming.empty()) {
        throw ValidationException("No valid LeetCode problems found in JSON payload.");
    }

    std::size_t added = 0;
    for (auto& p : incoming) {
        if (upsertCatalogProblem(std::move(p))) ++added;
    }
    profile_.lastCatalogSync = nowTimestamp();
    profile_.lastSyncError.clear();
    saveCatalogue();
    saveState();
    return added;
}

std::size_t IntelligenceHub::syncLeetCodeCatalogue(const std::string& graphqlEndpoint) {
    const std::string gqlQuery =
        R"(query problemsetQuestionList($categorySlug: String, $limit: Int, $skip: Int, $filters: QuestionListFilterInput) { )"
        R"(problemsetQuestionList: questionList(categorySlug: $categorySlug limit: $limit skip: $skip filters: $filters) { )"
        R"(total: totalNum questions: data { frontendQuestionId: questionFrontendId title titleSlug difficulty paidOnly: isPaidOnly topicTags { name } } } })";

    std::vector<CatalogProblem> fetched;
    int skip = 0;
    const int limit = 500;
    int totalNum = -1;

    try {
        while (totalNum < 0 || skip < totalNum) {
            std::ostringstream payload;
            payload << "{\"query\":" << json::escape(gqlQuery)
                    << ",\"variables\":{\"categorySlug\":\"\",\"skip\":" << skip
                    << ",\"limit\":" << limit << ",\"filters\":{}}}";

            const std::string body = httpPostJson(graphqlEndpoint, payload.str());
            json::JsonValue root = json::parse(body);
            const auto* data = root.getPtr("data");
            const auto* pql = data ? data->getPtr("problemsetQuestionList") : nullptr;
            const auto* qs = pql ? pql->getPtr("questions") : nullptr;
            if (!pql || !qs || !qs->isArray()) {
                throw DSAException("Unexpected GraphQL response format from LeetCode catalogue.", "SYNC_ERROR");
            }
            totalNum = static_cast<int>(pql->getInt("total", static_cast<long long>(qs->arrVal.size())));
            if (qs->arrVal.empty()) break;

            for (const auto& q : qs->arrVal) {
                CatalogProblem p;
                const auto* fp = q.getPtr("frontendQuestionId");
                if (fp && fp->type == json::JsonValue::Type::String) {
                    try { p.id = std::stoi(fp->strVal); } catch (...) {}
                } else if (fp && fp->type == json::JsonValue::Type::Int) {
                    p.id = static_cast<int>(fp->intVal);
                }
                p.title = q.getString("title");
                p.slug = q.getString("titleSlug");
                p.difficulty = q.getString("difficulty", "Medium");
                p.isPremium = q.getBool("paidOnly", false);
                if (const auto* tagsPtr = q.getPtr("topicTags")) {
                    for (const auto& t : tagsPtr->arrVal) {
                        if (t.isObject() && t.has("name")) p.topicTags.push_back(t.getString("name"));
                    }
                }
                if (p.id > 0 && !p.title.empty() && !p.slug.empty()) {
                    fetched.push_back(std::move(p));
                }
            }
            skip += limit;
            if (skip > 10000) break; // safety guard
        }
    } catch (const std::exception& ex) {
        std::lock_guard<std::mutex> lock(mutex_);
        profile_.lastSyncError = ex.what();
        saveState();
        throw;
    }

    std::lock_guard<std::mutex> lock(mutex_);
    for (auto& p : fetched) {
        upsertCatalogProblem(std::move(p));
    }
    profile_.lastCatalogSync = nowTimestamp();
    profile_.lastSyncError.clear();
    saveCatalogue();
    saveState();
    return catalogue_.size();
}

CataloguePageResult IntelligenceHub::queryCatalogue(const CatalogueQuery& query) const {
    std::lock_guard<std::mutex> lock(mutex_);
    CataloguePageResult res;
    res.totalCatalogue = catalogue_.size();

    std::set<std::string> topicSet;
    for (const auto& p : catalogue_) {
        for (const auto& t : p.topicTags) topicSet.insert(t);
    }
    res.availableTopics.assign(topicSet.begin(), topicSet.end());

    const std::string needle = toLower(query.search);
    std::vector<const CatalogProblem*> matched;
    matched.reserve(catalogue_.size());

    for (const auto& p : catalogue_) {
        if (!needle.empty()) {
            const std::string idStr = std::to_string(p.id);
            if (idStr != needle &&
                toLower(p.title).find(needle) == std::string::npos &&
                toLower(p.slug).find(needle) == std::string::npos) {
                continue;
            }
        }
        if (query.difficulty != "all" && !query.difficulty.empty() && p.difficulty != query.difficulty) {
            continue;
        }
        if (query.availability == "free" && p.isPremium) continue;
        if (query.availability == "premium" && !p.isPremium) continue;

        if (query.topic != "all" && !query.topic.empty()) {
            if (std::find(p.topicTags.begin(), p.topicTags.end(), query.topic) == p.topicTags.end()) {
                continue;
            }
        }

        auto trIt = tracker_.find(p.slug);
        const std::string st = (trIt != tracker_.end()) ? trIt->second.status : "Not Started";
        const bool bm = (trIt != tracker_.end()) ? trIt->second.bookmarked : false;
        if (query.status == "Bookmarked") {
            if (!bm) continue;
        } else if (query.status != "all" && !query.status.empty() && st != query.status) {
            continue;
        }
        matched.push_back(&p);
    }

    const bool desc = (query.sortDir == "desc");
    std::sort(matched.begin(), matched.end(), [&](const CatalogProblem* a, const CatalogProblem* b) {
        int cmp = 0;
        if (query.sortBy == "title") {
            cmp = a->title.compare(b->title);
        } else if (query.sortBy == "difficulty") {
            cmp = difficultyRank(a->difficulty) - difficultyRank(b->difficulty);
        } else {
            cmp = (a->id < b->id) ? -1 : (a->id > b->id ? 1 : 0);
        }
        return desc ? (cmp > 0) : (cmp < 0);
    });

    res.totalFiltered = matched.size();
    res.pageSize = std::max<std::size_t>(10, std::min<std::size_t>(query.pageSize, 100));
    res.totalPages = std::max<std::size_t>(1, (res.totalFiltered + res.pageSize - 1) / res.pageSize);
    res.page = std::max<std::size_t>(1, std::min(query.page, res.totalPages));

    const std::size_t startIdx = (res.page - 1) * res.pageSize;
    const std::size_t endIdx = std::min(startIdx + res.pageSize, matched.size());

    for (std::size_t i = startIdx; i < endIdx; ++i) {
        const CatalogProblem& prob = *matched[i];
        CatalogueItemView item;
        item.problem = prob;
        auto trIt = tracker_.find(prob.slug);
        if (trIt != tracker_.end()) item.progress = trIt->second;
        else item.progress.slug = prob.slug;
        item.hasNotes = notes_.find(prob.slug) != notes_.end();
        for (const auto& s : solutions_) {
            if (s.slug == prob.slug) ++item.solutionCount;
        }
        res.items.push_back(std::move(item));
    }
    return res;
}

ProblemDetailView IntelligenceHub::getProblemDetail(const std::string& slug) const {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = slugToIdx_.find(slug);
    if (it == slugToIdx_.end()) {
        throw ValidationException("Problem '" + slug + "' not found in catalogue.");
    }
    ProblemDetailView view;
    view.problem = catalogue_[it->second];

    auto trIt = tracker_.find(slug);
    if (trIt != tracker_.end()) view.progress = trIt->second;
    else view.progress.slug = slug;

    auto nIt = notes_.find(slug);
    if (nIt != notes_.end()) view.notes = nIt->second;
    else view.notes.slug = slug;

    for (const auto& s : solutions_) {
        if (s.slug == slug) view.solutions.push_back(s);
    }
    for (auto rIt = submissions_.rbegin(); rIt != submissions_.rend(); ++rIt) {
        if (rIt->slug == slug) view.submissions.push_back(*rIt);
    }
    return view;
}

bool IntelligenceHub::addSubmissionUnique(const SubmissionRecord& sub) {
    for (const auto& existing : submissions_) {
        if (existing.slug == sub.slug && existing.timestamp == sub.timestamp) {
            return false; // exact duplicate prevented
        }
    }
    submissions_.push_back(sub);
    return true;
}

std::string IntelligenceHub::resolveSlug(const std::string& slugOrId, int problemId, const std::string& title) {
    if (!slugOrId.empty() && slugToIdx_.count(slugOrId)) return slugOrId;
    if (problemId > 0 && idToIdx_.count(problemId)) return catalogue_[idToIdx_[problemId]].slug;
    if (!slugOrId.empty()) return slugOrId;
    return slugifyTitle(title);
}

LeetCodeProfileStats IntelligenceHub::syncLeetCodeProfile(const std::string& username,
                                                          const std::string& graphqlEndpoint) {
    if (username.empty() || username.size() > 64) {
        throw ValidationException("Please enter a valid LeetCode username.");
    }
    for (char c : username) {
        if (!std::isalnum(static_cast<unsigned char>(c)) && c != '_' && c != '-') {
            throw ValidationException("Username may only contain letters, digits, '-' or '_'.");
        }
    }

    const std::string gql =
        R"(query publicProfileAndRecent($username: String!, $limit: Int!) { )"
        R"(matchedUser(username: $username) { username profile { ranking } submitStatsGlobal { acSubmissionNum { difficulty count } } } )"
        R"(recentAcSubmissionList(username: $username, limit: $limit) { id title titleSlug timestamp } })";

    std::ostringstream payload;
    payload << "{\"query\":" << json::escape(gql)
            << ",\"variables\":{\"username\":" << json::escape(username) << ",\"limit\":20}}";

    std::string body;
    try {
        body = httpPostJson(graphqlEndpoint, payload.str());
    } catch (const std::exception& ex) {
        std::lock_guard<std::mutex> lock(mutex_);
        profile_.username = username;
        profile_.lastSyncError = ex.what();
        saveState();
        throw;
    }

    json::JsonValue root = json::parse(body);
    const auto* data = root.getPtr("data");
    const auto* user = data ? data->getPtr("matchedUser") : nullptr;
    if (!user || user->isNull()) {
        std::lock_guard<std::mutex> lock(mutex_);
        profile_.username = username;
        profile_.lastSyncError = "LeetCode user '" + username + "' not found or profile is private.";
        saveState();
        throw ValidationException(profile_.lastSyncError);
    }

    std::lock_guard<std::mutex> lock(mutex_);
    profile_.username = user->getString("username", username);
    if (const auto* prof = user->getPtr("profile")) {
        profile_.ranking = static_cast<int>(prof->getInt("ranking", 0));
    }
    if (const auto* statsGlobal = user->getPtr("submitStatsGlobal")) {
        if (const auto* acList = statsGlobal->getPtr("acSubmissionNum")) {
            for (const auto& entry : acList->arrVal) {
                std::string d = entry.getString("difficulty");
                int cnt = static_cast<int>(entry.getInt("count", 0));
                if (d == "All") profile_.totalSolved = cnt;
                else if (d == "Easy") profile_.easySolved = cnt;
                else if (d == "Medium") profile_.mediumSolved = cnt;
                else if (d == "Hard") profile_.hardSolved = cnt;
            }
        }
    }

    if (const auto* recList = data->getPtr("recentAcSubmissionList")) {
        for (const auto& item : recList->arrVal) {
            const std::string slug = item.getString("titleSlug");
            const std::string title = item.getString("title");
            const std::string tsRaw = item.getString("timestamp", "0");
            long long epoch = 0;
            try { epoch = std::stoll(tsRaw); } catch (...) {}
            const std::string tsStr = epoch > 0 ? epochToTimestamp(epoch) : nowTimestamp();
            const std::string dateStr = epoch > 0 ? epochToDate(epoch) : todayDate();

            int pid = 0;
            std::string diff = "Medium";
            if (slugToIdx_.count(slug)) {
                const auto& cp = catalogue_[slugToIdx_[slug]];
                pid = cp.id;
                diff = cp.difficulty;
            }

            SubmissionRecord sub{
                "sub-" + std::to_string(nextSubSeq_++), slug, pid, title, diff, tsStr, dateStr, "synced_recent"};
            if (addSubmissionUnique(sub)) {
                auto& tr = tracker_[slug];
                tr.slug = slug;
                if (tr.firstAttemptDate.empty() || dateStr < tr.firstAttemptDate) tr.firstAttemptDate = dateStr;
                if (tr.latestAttemptDate.empty() || dateStr > tr.latestAttemptDate) tr.latestAttemptDate = dateStr;
                ++tr.attemptCount;
                tr.accepted = true;
                if (tr.status != "Needs Revision") tr.status = "Solved";
                if (tr.source.empty() || tr.source == "manual_marked") tr.source = "synced_recent";
                dailyActivity_[dateStr] += 1;
            }
        }
    }

    profile_.lastProfileSync = nowTimestamp();
    profile_.lastSyncError.clear();
    saveState();
    return profile_;
}

std::size_t IntelligenceHub::importSubmissionsJson(const std::string& jsonText) {
    json::JsonValue root = json::parse(jsonText);
    const json::JsonValue* arr = nullptr;
    if (root.isArray()) arr = &root;
    else if (root.isObject() && root.has("submissions")) arr = root.getPtr("submissions");
    if (!arr || !arr->isArray()) {
        throw ValidationException("Expected JSON array of submissions or { \"submissions\": [...] }.");
    }

    std::lock_guard<std::mutex> lock(mutex_);
    std::size_t imported = 0;
    for (const auto& item : arr->arrVal) {
        if (!item.isObject()) continue;
        std::string title = item.getString("title");
        int pid = static_cast<int>(item.getInt("problemId", item.getInt("id", 0)));
        std::string slug = resolveSlug(item.getString("slug", item.getString("titleSlug")), pid, title);
        if (slug.empty()) continue;
        std::string ts = item.getString("timestamp", nowTimestamp());
        std::string date = (ts.size() >= 10) ? ts.substr(0, 10) : todayDate();
        std::string diff = normalizeDifficulty(item.getString("difficulty", "Medium"));
        if (slugToIdx_.count(slug)) {
            const auto& cp = catalogue_[slugToIdx_[slug]];
            if (pid == 0) pid = cp.id;
            if (title.empty()) title = cp.title;
            diff = cp.difficulty;
        }
        if (title.empty()) title = slug;

        SubmissionRecord sub{"sub-" + std::to_string(nextSubSeq_++), slug, pid, title, diff, ts, date, "manual_imported"};
        if (addSubmissionUnique(sub)) {
            ++imported;
            auto& tr = tracker_[slug];
            tr.slug = slug;
            if (tr.firstAttemptDate.empty() || date < tr.firstAttemptDate) tr.firstAttemptDate = date;
            if (tr.latestAttemptDate.empty() || date > tr.latestAttemptDate) tr.latestAttemptDate = date;
            ++tr.attemptCount;
            tr.accepted = true;
            if (tr.status != "Needs Revision") tr.status = "Solved";
            tr.source = "manual_imported";
            dailyActivity_[date] += 1;
        }
    }
    saveState();
    return imported;
}

SubmissionRecord IntelligenceHub::recordExtensionSubmission(int problemId, const std::string& title,
                                                            const std::string& rawSlug,
                                                            const std::string& difficulty,
                                                            const std::string& timestampStr) {
    std::lock_guard<std::mutex> lock(mutex_);
    std::string slug = resolveSlug(rawSlug, problemId, title);
    if (slug.empty()) {
        throw ValidationException("Extension submission requires problem slug or title.");
    }

    int pid = problemId;
    std::string finalTitle = title;
    std::string diff = difficulty.empty() ? "Medium" : normalizeDifficulty(difficulty);

    if (slugToIdx_.count(slug)) {
        const auto& cp = catalogue_[slugToIdx_[slug]];
        if (pid <= 0) pid = cp.id;
        if (finalTitle.empty()) finalTitle = cp.title;
        diff = cp.difficulty;
    }
    if (finalTitle.empty()) finalTitle = slug;

    const std::string ts = timestampStr.empty() ? nowTimestamp() : timestampStr;
    const std::string date = (ts.size() >= 10) ? ts.substr(0, 10) : todayDate();

    SubmissionRecord sub{"sub-" + std::to_string(nextSubSeq_++), slug, pid, finalTitle, diff, ts, date, "extension_detected"};
    const bool inserted = addSubmissionUnique(sub);
    if (inserted) {
        auto& tr = tracker_[slug];
        tr.slug = slug;
        if (tr.firstAttemptDate.empty()) tr.firstAttemptDate = date;
        tr.latestAttemptDate = date;
        ++tr.attemptCount;
        tr.accepted = true;
        if (tr.status != "Needs Revision") tr.status = "Solved";
        tr.source = "extension_detected";
        dailyActivity_[date] += 1;
    }
    profile_.extensionLastSeen = nowTimestamp();
    if (inserted) ++profile_.extensionEventsCount;
    saveState();
    return sub;
}

LeetCodeProfileStats IntelligenceHub::profileStats() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return profile_;
}

std::vector<SubmissionRecord> IntelligenceHub::recentSubmissions(std::size_t limit) const {
    std::lock_guard<std::mutex> lock(mutex_);
    std::vector<SubmissionRecord> out = submissions_;
    std::reverse(out.begin(), out.end());
    if (out.size() > limit) out.resize(limit);
    return out;
}

ProblemProgressRecord IntelligenceHub::updateProblemStatus(const std::string& slug, const std::string& status,
                                                             bool hasBookmark, bool bookmarkVal,
                                                             bool toggleBookmark, bool hasNeedsRevision,
                                                             bool needsRevisionVal) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (!slugToIdx_.count(slug)) {
        throw ValidationException("Problem '" + slug + "' not found in catalogue.");
    }
    auto& tr = tracker_[slug];
    tr.slug = slug;
    const std::string today = todayDate();

    if (!status.empty()) {
        if (!isValidStatus(status)) {
            throw ValidationException("Invalid status: '" + status + "'.");
        }
        if (status != "Not Started") {
            if (tr.firstAttemptDate.empty()) tr.firstAttemptDate = today;
            tr.latestAttemptDate = today;
            if (tr.status != status) {
                ++tr.attemptCount;
                dailyActivity_[today] += 1;
            }
        }
        tr.status = status;
        if (status == "Solved") {
            tr.accepted = true;
            tr.needsRevision = false;
        } else if (status == "Needs Revision") {
            tr.needsRevision = true;
            if (tr.nextReviewDate.empty()) tr.nextReviewDate = today;
        }
        if (tr.source.empty()) tr.source = "manual_marked";
    }

    if (toggleBookmark) tr.bookmarked = !tr.bookmarked;
    else if (hasBookmark) tr.bookmarked = bookmarkVal;

    if (hasNeedsRevision) {
        tr.needsRevision = needsRevisionVal;
        if (needsRevisionVal && tr.status != "Needs Revision") {
            tr.status = "Needs Revision";
            if (tr.nextReviewDate.empty()) tr.nextReviewDate = today;
        }
    }

    saveState();
    return tr;
}

ProblemNotes IntelligenceHub::saveProblemNotes(const ProblemNotes& input) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (!slugToIdx_.count(input.slug)) {
        throw ValidationException("Problem '" + input.slug + "' not found in catalogue.");
    }
    ProblemNotes n = input;
    n.updatedAt = nowTimestamp();
    notes_[n.slug] = n;
    saveState();
    return n;
}

std::vector<ProblemNotes> IntelligenceHub::searchNotes(const std::string& query) const {
    std::lock_guard<std::mutex> lock(mutex_);
    const std::string needle = toLower(query);
    std::vector<ProblemNotes> out;
    for (const auto& [slug, n] : notes_) {
        const bool allEmpty = n.intuition.empty() && n.bruteForce.empty() && n.optimized.empty() &&
                              n.mistakesMade.empty() && n.observations.empty() &&
                              n.cppSnippet.empty() && n.revisionComments.empty();
        if (allEmpty) continue;
        if (needle.empty() ||
            toLower(slug).find(needle) != std::string::npos ||
            toLower(n.intuition).find(needle) != std::string::npos ||
            toLower(n.bruteForce).find(needle) != std::string::npos ||
            toLower(n.optimized).find(needle) != std::string::npos ||
            toLower(n.mistakesMade).find(needle) != std::string::npos ||
            toLower(n.observations).find(needle) != std::string::npos ||
            toLower(n.cppSnippet).find(needle) != std::string::npos ||
            toLower(n.revisionComments).find(needle) != std::string::npos) {
            out.push_back(n);
        }
    }
    return out;
}

SavedSolution IntelligenceHub::addSolution(const SavedSolution& input) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (input.approachName.empty() || input.cppCode.empty()) {
        throw ValidationException("Approach name and C++ solution code are required.");
    }
    SavedSolution sol = input;
    sol.id = "sol-" + std::to_string(nextSolSeq_++);
    sol.createdAt = nowTimestamp();

    auto it = slugToIdx_.find(sol.slug);
    if (it != slugToIdx_.end()) {
        const auto& cp = catalogue_[it->second];
        sol.problemId = cp.id;
        sol.problemTitle = cp.title;
        if (sol.topics.empty()) sol.topics = cp.topicTags;
    }
    solutions_.push_back(sol);
    saveState();
    return sol;
}

void IntelligenceHub::deleteSolution(const std::string& solutionId) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = std::remove_if(solutions_.begin(), solutions_.end(), [&](const SavedSolution& s) {
        return s.id == solutionId;
    });
    if (it == solutions_.end()) {
        throw ValidationException("Solution ID not found.");
    }
    solutions_.erase(it, solutions_.end());
    saveState();
}

std::vector<SavedSolution> IntelligenceHub::searchSolutions(const std::string& query,
                                                            const std::string& topic,
                                                            const std::string& complexity) const {
    std::lock_guard<std::mutex> lock(mutex_);
    const std::string qLow = toLower(query);
    const std::string cLow = toLower(complexity);
    std::vector<SavedSolution> out;

    for (auto it = solutions_.rbegin(); it != solutions_.rend(); ++it) {
        const auto& s = *it;
        if (!qLow.empty()) {
            const std::string pidStr = std::to_string(s.problemId);
            if (pidStr != qLow &&
                toLower(s.problemTitle).find(qLow) == std::string::npos &&
                toLower(s.approachName).find(qLow) == std::string::npos &&
                toLower(s.slug).find(qLow) == std::string::npos) {
                continue;
            }
        }
        if (topic != "all" && !topic.empty()) {
            if (std::find(s.topics.begin(), s.topics.end(), topic) == s.topics.end()) continue;
        }
        if (!cLow.empty() && cLow != "all") {
            if (toLower(s.timeComplexity).find(cLow) == std::string::npos &&
                toLower(s.spaceComplexity).find(cLow) == std::string::npos) {
                continue;
            }
        }
        out.push_back(s);
    }
    return out;
}

void IntelligenceHub::setTopicGoal(const std::string& topic, std::size_t goalTarget) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (topic.empty()) throw ValidationException("Topic name cannot be empty.");
    if (goalTarget < 1 || goalTarget > 5000) {
        throw ValidationException("Topic goal must be between 1 and 5000.");
    }
    topicGoals_[topic] = goalTarget;
    saveState();
}

std::vector<TopicProgressItem> IntelligenceHub::computeTopicProgress() const {
    std::lock_guard<std::mutex> lock(mutex_);
    std::map<std::string, TopicProgressItem> byTopic;

    for (const auto& p : catalogue_) {
        auto trIt = tracker_.find(p.slug);
        const bool isSolved = (trIt != tracker_.end() && (trIt->second.status == "Solved" || trIt->second.accepted));
        const bool isAttempted = (trIt != tracker_.end() && trIt->second.status != "Not Started");

        for (const auto& tag : p.topicTags) {
            auto& tp = byTopic[tag];
            tp.topic = tag;
            ++tp.totalProblems;
            if (p.difficulty == "Easy") ++tp.easyTotal;
            else if (p.difficulty == "Medium") ++tp.mediumTotal;
            else ++tp.hardTotal;

            if (isSolved) {
                ++tp.solvedProblems;
                if (p.difficulty == "Easy") ++tp.easySolved;
                else if (p.difficulty == "Medium") ++tp.mediumSolved;
                else ++tp.hardSolved;
            } else if (isAttempted) {
                ++tp.attemptedProblems;
            }
        }
    }

    std::vector<TopicProgressItem> out;
    out.reserve(byTopic.size());
    for (auto& [topic, tp] : byTopic) {
        tp.remainingProblems = (tp.totalProblems > tp.solvedProblems)
            ? (tp.totalProblems - tp.solvedProblems) : 0;
        tp.completionPercent = tp.totalProblems > 0
            ? (100.0 * static_cast<double>(tp.solvedProblems) / static_cast<double>(tp.totalProblems))
            : 0.0;
        auto gIt = topicGoals_.find(topic);
        tp.goalTarget = (gIt != topicGoals_.end()) ? gIt->second : std::min<std::size_t>(10, tp.totalProblems);
        out.push_back(std::move(tp));
    }

    std::sort(out.begin(), out.end(), [](const TopicProgressItem& a, const TopicProgressItem& b) {
        if (a.solvedProblems != b.solvedProblems) return a.solvedProblems > b.solvedProblems;
        return a.totalProblems > b.totalProblems;
    });
    return out;
}

ProblemProgressRecord IntelligenceHub::scheduleProblemRevision(const std::string& slug, bool needsRevision) {
    return updateProblemStatus(slug, "", false, false, false, true, needsRevision);
}

ProblemProgressRecord IntelligenceHub::reviewProblemRevision(const std::string& slug, const std::string& rating) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (!slugToIdx_.count(slug)) {
        throw ValidationException("Problem '" + slug + "' not found in catalogue.");
    }
    auto& tr = tracker_[slug];
    tr.slug = slug;

    if (rating == "again") {
        tr.revisionReps = 0;
        tr.revisionIntervalDays = 0;
        tr.revisionEase = std::max(1.3, tr.revisionEase - 0.2);
        tr.lastRating = "Again";
        tr.needsRevision = true;
        tr.status = "Needs Revision";
    } else if (rating == "difficult") {
        tr.revisionReps = std::max(1, tr.revisionReps);
        tr.revisionIntervalDays = std::max(1, tr.revisionIntervalDays == 0 ? 1 : static_cast<int>(tr.revisionIntervalDays * 1.2));
        tr.revisionEase = std::max(1.3, tr.revisionEase - 0.1);
        tr.lastRating = "Difficult";
        tr.needsRevision = true;
    } else if (rating == "good") {
        ++tr.revisionReps;
        if (tr.revisionReps == 1) tr.revisionIntervalDays = 2;
        else if (tr.revisionReps == 2) tr.revisionIntervalDays = 5;
        else tr.revisionIntervalDays = static_cast<int>(std::max(5.0, tr.revisionIntervalDays * tr.revisionEase));
        tr.lastRating = "Good";
        tr.needsRevision = false;
        if (tr.status == "Needs Revision" && tr.accepted) tr.status = "Solved";
    } else if (rating == "easy") {
        ++tr.revisionReps;
        if (tr.revisionReps == 1) tr.revisionIntervalDays = 5;
        else tr.revisionIntervalDays = static_cast<int>(std::max(7.0, tr.revisionIntervalDays * (tr.revisionEase + 0.3)));
        tr.revisionEase = std::min(3.0, tr.revisionEase + 0.15);
        tr.lastRating = "Easy";
        tr.needsRevision = false;
        if (tr.status == "Needs Revision" && tr.accepted) tr.status = "Solved";
    } else {
        throw ValidationException("Invalid rating. Use again, difficult, good, or easy.");
    }

    tr.nextReviewDate = addDaysToToday(tr.revisionIntervalDays);
    ++tr.reviewCount;
    dailyActivity_[todayDate()] += 1;
    saveState();
    return tr;
}

std::vector<CatalogueItemView> IntelligenceHub::revisionQueue() const {
    std::lock_guard<std::mutex> lock(mutex_);
    std::vector<CatalogueItemView> queue;
    for (const auto& [slug, tr] : tracker_) {
        if (tr.needsRevision || tr.status == "Needs Revision" || !tr.nextReviewDate.empty()) {
            auto idxIt = slugToIdx_.find(slug);
            if (idxIt == slugToIdx_.end()) continue;
            CatalogueItemView item;
            item.problem = catalogue_[idxIt->second];
            item.progress = tr;
            item.hasNotes = notes_.find(slug) != notes_.end();
            queue.push_back(std::move(item));
        }
    }
    std::sort(queue.begin(), queue.end(), [](const CatalogueItemView& a, const CatalogueItemView& b) {
        if (a.progress.needsRevision != b.progress.needsRevision) {
            return a.progress.needsRevision > b.progress.needsRevision;
        }
        return a.progress.nextReviewDate < b.progress.nextReviewDate;
    });
    return queue;
}

IntelligenceAnalytics IntelligenceHub::computeAnalytics() const {
    auto topics = computeTopicProgress();

    std::lock_guard<std::mutex> lock(mutex_);
    IntelligenceAnalytics a;
    a.totalCatalogue = catalogue_.size();
    a.totalSolutions = solutions_.size();
    a.totalNotes = notes_.size();

    const std::string today = todayDate();
    for (const auto& [slug, tr] : tracker_) {
        if (tr.status == "Solved" || tr.accepted) {
            ++a.totalSolved;
            auto idxIt = slugToIdx_.find(slug);
            if (idxIt != slugToIdx_.end()) {
                const std::string& d = catalogue_[idxIt->second].difficulty;
                if (d == "Easy") ++a.solvedEasy;
                else if (d == "Medium") ++a.solvedMedium;
                else if (d == "Hard") ++a.solvedHard;
            }
        }
        if (tr.status != "Not Started" || tr.attemptCount > 0) {
            ++a.totalAttempted;
        }
        if (tr.needsRevision || tr.status == "Needs Revision") {
            ++a.needsRevisionCount;
            if (tr.nextReviewDate.empty() || tr.nextReviewDate <= today) {
                ++a.revisionDue;
            }
        }
        a.revisionCompleted += tr.reviewCount;
    }

    if (!topics.empty()) {
        double sumPct = 0.0;
        for (const auto& tp : topics) sumPct += tp.completionPercent;
        a.overallTopicCompletionPercent = sumPct / static_cast<double>(topics.size());
    }

    for (int offset = -27; offset <= 0; ++offset) {
        const std::string d = addDaysToToday(offset);
        const std::size_t cnt = dailyActivity_.count(d) ? dailyActivity_.at(d) : 0;
        a.dailyActivityHeatmap.push_back(DayActivity{d, cnt});
        if (offset >= -6) {
            a.weeklyActivity.push_back(DayActivity{d, cnt});
        }
    }

    // Current streak: consecutive days ending today (or yesterday if today has 0 yet)
    int streakOffset = 0;
    if (!dailyActivity_.count(todayDate()) || dailyActivity_.at(todayDate()) == 0) {
        streakOffset = -1;
    }
    while (true) {
        const std::string d = addDaysToToday(streakOffset);
        if (dailyActivity_.count(d) && dailyActivity_.at(d) > 0) {
            ++a.currentStreak;
            --streakOffset;
        } else {
            break;
        }
    }

    a.mostPracticedTopics = topics;
    if (a.mostPracticedTopics.size() > 6) a.mostPracticedTopics.resize(6);

    // Weakest topics: among topics with totalProblems > 0, prioritize attempted-but-low-solved% or lowest solved%
    std::vector<TopicProgressItem> started;
    for (const auto& tp : topics) {
        if (tp.attemptedProblems > 0 || tp.solvedProblems > 0) started.push_back(tp);
    }
    auto& weakSrc = started.empty() ? topics : started;
    std::sort(weakSrc.begin(), weakSrc.end(), [](const TopicProgressItem& x, const TopicProgressItem& y) {
        if (x.completionPercent != y.completionPercent) return x.completionPercent < y.completionPercent;
        return x.attemptedProblems > y.attemptedProblems;
    });
    a.weakestTopics = weakSrc;
    if (a.weakestTopics.size() > 6) a.weakestTopics.resize(6);

    return a;
}

} // namespace dsa
