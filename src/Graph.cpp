#include "dsa/Graph.hpp"

#include <algorithm>
#include <cctype>
#include <queue>
#include <set>
#include <stack>

namespace dsa {

Graph::Graph(bool directed) : directed_(directed) {}

std::string Graph::normalizeLabel(const std::string& rawLabel) {
    std::string trimmed;
    for (char c : rawLabel) {
        if (!std::isspace(static_cast<unsigned char>(c))) {
            trimmed.push_back(static_cast<char>(std::toupper(static_cast<unsigned char>(c))));
        }
    }
    if (trimmed.empty() || trimmed.size() > 4) {
        throw ValidationException("Vertex label must be 1 to 4 alphanumeric characters.");
    }
    for (char c : trimmed) {
        if (!std::isalnum(static_cast<unsigned char>(c))) {
            throw ValidationException("Vertex label must contain only letters or digits.");
        }
    }
    return trimmed;
}

bool Graph::hasEdgeInternal(const std::string& u, const std::string& v) const {
    auto it = adj_.find(u);
    if (it == adj_.end()) return false;
    return std::find(it->second.begin(), it->second.end(), v) != it->second.end();
}

void Graph::setDirected(bool directed) {
    if (directed_ == directed) return;
    clear();
    directed_ = directed;
}

void Graph::addVertex(const std::string& rawLabel) {
    const std::string label = normalizeLabel(rawLabel);
    if (adj_.size() >= 16) {
        throw OverflowException("Graph vertex limit (16) reached.");
    }
    if (adj_.find(label) != adj_.end()) {
        throw ValidationException("Vertex '" + label + "' already exists.");
    }
    adj_[label] = {};
}

void Graph::removeVertex(const std::string& rawLabel) {
    const std::string label = normalizeLabel(rawLabel);
    auto it = adj_.find(label);
    if (it == adj_.end()) {
        throw ValidationException("Vertex '" + label + "' does not exist.");
    }
    adj_.erase(it);
    for (auto& [v, neighbors] : adj_) {
        neighbors.erase(std::remove(neighbors.begin(), neighbors.end(), label), neighbors.end());
    }
}

void Graph::addEdge(const std::string& rawFrom, const std::string& rawTo) {
    const std::string u = normalizeLabel(rawFrom);
    const std::string v = normalizeLabel(rawTo);
    if (adj_.find(u) == adj_.end() || adj_.find(v) == adj_.end()) {
        throw ValidationException("Both endpoints must exist before adding an edge.");
    }
    if (u == v) {
        throw ValidationException("Self-loops are not allowed in a simple graph.");
    }
    if (hasEdgeInternal(u, v)) {
        throw ValidationException("Edge (" + u + ", " + v + ") already exists.");
    }

    adj_[u].push_back(v);
    std::sort(adj_[u].begin(), adj_[u].end());
    if (!directed_) {
        adj_[v].push_back(u);
        std::sort(adj_[v].begin(), adj_[v].end());
    }
}

void Graph::removeEdge(const std::string& rawFrom, const std::string& rawTo) {
    const std::string u = normalizeLabel(rawFrom);
    const std::string v = normalizeLabel(rawTo);
    if (!hasEdgeInternal(u, v)) {
        throw ValidationException("Edge (" + u + ", " + v + ") does not exist.");
    }
    auto& nu = adj_[u];
    nu.erase(std::remove(nu.begin(), nu.end(), v), nu.end());
    if (!directed_) {
        auto& nv = adj_[v];
        nv.erase(std::remove(nv.begin(), nv.end(), u), nv.end());
    }
}

namespace {

std::vector<std::string> queueToVec(std::queue<std::string> q) {
    std::vector<std::string> out;
    while (!q.empty()) {
        out.push_back(q.front());
        q.pop();
    }
    return out;
}

std::vector<std::string> stackToVec(std::stack<std::string> s) {
    std::vector<std::string> out;
    while (!s.empty()) {
        out.push_back(s.top());
        s.pop();
    }
    std::reverse(out.begin(), out.end());
    return out;
}

} // namespace

GraphTraversalResult Graph::bfs(const std::string& rawStart) const {
    if (adj_.empty()) {
        throw UnderflowException("Graph is empty. Add vertices before running BFS.");
    }
    const std::string start = normalizeLabel(rawStart);
    if (adj_.find(start) == adj_.end()) {
        throw ValidationException("Start vertex '" + start + "' does not exist.");
    }

    GraphTraversalResult res;
    res.algorithm = "bfs";
    res.startVertex = start;

    std::set<std::string> seen;
    std::queue<std::string> q;
    seen.insert(start);
    q.push(start);

    res.steps.push_back(GraphStep{
        start, "", "", res.order, queueToVec(q), "start",
        "Initialized BFS queue with start vertex " + start + "."});

    while (!q.empty()) {
        const std::string curr = q.front();
        q.pop();
        res.order.push_back(curr);

        res.steps.push_back(GraphStep{
            curr, "", "", res.order, queueToVec(q), "visit",
            "Dequeued and visited vertex " + curr + "."});

        for (const auto& nbr : adj_.at(curr)) {
            if (seen.find(nbr) == seen.end()) {
                seen.insert(nbr);
                q.push(nbr);
                res.steps.push_back(GraphStep{
                    curr, curr, nbr, res.order, queueToVec(q), "discover",
                    "Discovered neighbor " + nbr + " from " + curr + "; enqueued " + nbr + "."});
            }
        }
    }

    res.steps.push_back(GraphStep{
        "", "", "", res.order, {}, "done",
        "BFS traversal complete."});
    return res;
}

GraphTraversalResult Graph::dfs(const std::string& rawStart) const {
    if (adj_.empty()) {
        throw UnderflowException("Graph is empty. Add vertices before running DFS.");
    }
    const std::string start = normalizeLabel(rawStart);
    if (adj_.find(start) == adj_.end()) {
        throw ValidationException("Start vertex '" + start + "' does not exist.");
    }

    GraphTraversalResult res;
    res.algorithm = "dfs";
    res.startVertex = start;

    std::set<std::string> visitedSet;
    std::stack<std::string> st;
    st.push(start);

    res.steps.push_back(GraphStep{
        start, "", "", res.order, stackToVec(st), "start",
        "Pushed start vertex " + start + " onto DFS stack."});

    while (!st.empty()) {
        const std::string curr = st.top();
        st.pop();
        if (visitedSet.find(curr) != visitedSet.end()) {
            continue;
        }
        visitedSet.insert(curr);
        res.order.push_back(curr);

        res.steps.push_back(GraphStep{
            curr, "", "", res.order, stackToVec(st), "visit",
            "Popped and visited vertex " + curr + "."});

        const auto& neighbors = adj_.at(curr);
        for (auto it = neighbors.rbegin(); it != neighbors.rend(); ++it) {
            if (visitedSet.find(*it) == visitedSet.end()) {
                st.push(*it);
                res.steps.push_back(GraphStep{
                    curr, curr, *it, res.order, stackToVec(st), "discover",
                    "Pushed unvisited neighbor " + *it + " from " + curr + "."});
            }
        }
    }

    res.steps.push_back(GraphStep{
        "", "", "", res.order, {}, "done",
        "DFS traversal complete."});
    return res;
}

void Graph::clear() {
    adj_.clear();
}

void Graph::loadSample() {
    clear();
    for (const char* v : {"A", "B", "C", "D", "E", "F", "G"}) {
        addVertex(v);
    }
    addEdge("A", "B");
    addEdge("A", "C");
    addEdge("B", "D");
    addEdge("B", "E");
    addEdge("C", "F");
    addEdge("E", "G");
    addEdge("F", "G");
}

GraphSnapshot Graph::snapshot() const {
    GraphSnapshot snap;
    snap.directed = directed_;
    std::set<std::pair<std::string, std::string>> emitted;

    for (const auto& [u, neighbors] : adj_) {
        snap.vertices.push_back(u);
        for (const auto& v : neighbors) {
            if (directed_) {
                snap.edges.push_back(GraphEdge{u, v});
            } else {
                auto key = (u < v) ? std::make_pair(u, v) : std::make_pair(v, u);
                if (emitted.insert(key).second) {
                    snap.edges.push_back(GraphEdge{key.first, key.second});
                }
            }
        }
    }
    return snap;
}

} // namespace dsa
