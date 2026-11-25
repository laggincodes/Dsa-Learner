#pragma once

#include "dsa/Types.hpp"

#include <cstddef>
#include <memory>
#include <sstream>
#include <string>
#include <vector>

namespace dsa {

// ============================================================================
// Function Templates (Generic Utilities)
// ============================================================================

namespace utils {

template <typename T>
void swapValues(T& a, T& b) {
    T temp = a;
    a = b;
    b = temp;
}

template <typename T>
bool isSorted(const std::vector<T>& arr) {
    for (std::size_t i = 1; i < arr.size(); ++i) {
        if (arr[i] < arr[i - 1]) {
            return false;
        }
    }
    return true;
}

template <typename T>
std::string joinValues(const std::vector<T>& arr, const std::string& sep = ", ") {
    std::ostringstream oss;
    for (std::size_t i = 0; i < arr.size(); ++i) {
        if (i > 0) {
            oss << sep;
        }
        oss << arr[i];
    }
    return oss.str();
}

// Function overloading for random array generation
std::vector<int> generateRandomArray(std::size_t size);
std::vector<int> generateRandomArray(std::size_t size, int minVal, int maxVal);
std::vector<int> generatePatternArray(std::size_t size, const std::string& pattern);

void validateArrayInput(const std::vector<int>& arr, std::size_t minSize = 1, std::size_t maxSize = 60);

} // namespace utils

// ============================================================================
// Abstract Base Hierarchy (Inheritance & Virtual Functions)
// ============================================================================

class Algorithm {
public:
    explicit Algorithm(AlgorithmInfo info);
    virtual ~Algorithm() = default;

    const AlgorithmInfo& getInfo() const noexcept;

protected:
    AlgorithmInfo info_;
};

class Sorter : public Algorithm {
public:
    using Algorithm::Algorithm;
    ~Sorter() override = default;

    virtual SortResult sort(const std::vector<int>& input) const = 0;
};

class BubbleSorter final : public Sorter {
public:
    BubbleSorter();
    SortResult sort(const std::vector<int>& input) const override;
};

class SelectionSorter final : public Sorter {
public:
    SelectionSorter();
    SortResult sort(const std::vector<int>& input) const override;
};

class InsertionSorter final : public Sorter {
public:
    InsertionSorter();
    SortResult sort(const std::vector<int>& input) const override;
};

class Searcher : public Algorithm {
public:
    using Algorithm::Algorithm;
    ~Searcher() override = default;

    virtual SearchResult search(const std::vector<int>& input, int target) const = 0;
};

class LinearSearcher final : public Searcher {
public:
    LinearSearcher();
    SearchResult search(const std::vector<int>& input, int target) const override;
};

class BinarySearcher final : public Searcher {
public:
    BinarySearcher();
    SearchResult search(const std::vector<int>& input, int target) const override;
};

// Factory / Registry helpers
std::unique_ptr<Sorter> createSorter(const std::string& id);
std::unique_ptr<Searcher> createSearcher(const std::string& id);
std::vector<AlgorithmInfo> getAllAlgorithmInfo();
std::vector<BenchmarkResult> compareSortingAlgorithms(const std::vector<int>& input);

} // namespace dsa
