#pragma once

#include "dsa/Types.hpp"

#include <map>
#include <string>
#include <vector>

namespace dsa {

class Graph {
public:
    explicit Graph(bool directed = false);
    ~Graph() = default;

    void setDirected(bool directed);
    bool isDirected() const noexcept { return directed_; }

    void addVertex(const std::string& rawLabel);
    void removeVertex(const std::string& rawLabel);
    void addEdge(const std::string& rawFrom, const std::string& rawTo);
    void removeEdge(const std::string& rawFrom, const std::string& rawTo);

    GraphTraversalResult bfs(const std::string& rawStart) const;
    GraphTraversalResult dfs(const std::string& rawStart) const;

    void clear();
    void loadSample();
    GraphSnapshot snapshot() const;

private:
    static std::string normalizeLabel(const std::string& rawLabel);
    bool hasEdgeInternal(const std::string& u, const std::string& v) const;

    bool directed_ = false;
    std::map<std::string, std::vector<std::string>> adj_;
};

} // namespace dsa
