//
// Created by Peter Li on 6/26/22.
//

#include "flip_distance_source.h"
#include <stack>
#include <algorithm>
#include <queue>
#include <unordered_set>

int branchCounter = 0;

inline void addNeighbors(std::vector<std::pair<Edge, Edge>> &next,
                         const TriangulatedGraph &g, const Edge &e) {
    auto neighbors = g.getNeighbors(e);
    next.emplace_back(neighbors[0], neighbors[1]);
    next.emplace_back(neighbors[2], neighbors[3]);
}

void addNeighborsToForbid(const Edge &e, const TriangulatedGraph &g,
                          std::unordered_multiset<Edge> &forbid) {
    forbid.insert(e);
    for (const Edge &neighbor: g.getNeighbors(e)) {
        forbid.insert(neighbor);
    }
}

template<class T>
inline void eraseOne(std::unordered_multiset<T> &set, const T &e) {
    auto find = set.find(e);
    if (find != set.end()) {
        set.erase(find);
    }
}

void removeNeighborsFromForbid(const Edge &e, const TriangulatedGraph &g,
                               std::unordered_multiset<Edge> &forbid) {
    eraseOne(forbid, e);
    for (const Edge &neighbor: g.getNeighbors(e)) {
        eraseOne(forbid, neighbor);
    }
}

std::vector<std::pair<Edge, Edge>>
filterAndMapEdgePairs(const std::vector<std::pair<Edge, Edge>> &sources,
                      const std::function<bool(int)> &filter,
                      const std::function<int(int)> &mapper) {
    std::vector<std::pair<Edge, Edge>> result;
    for (auto pair: sources) {
        if (filter(pair.first.first) && filter(pair.first.second)
            && filter(pair.second.first) && filter(pair.second.second)) {
            result.emplace_back(
                    std::pair(Edge(mapper(pair.first.first), mapper(pair.first.second)),
                              Edge(mapper(pair.second.first), mapper(pair.second.second))
                    ));
        }
    }
    return result;
}

bool assertNoCommonEdge(const TriangulatedGraph &start, const TriangulatedGraph &end) {
    for (const Edge &e: start.getEdges()) {
        assert(!end.hasEdge(e));
    }
    return true;
}

bool assertNoFreeEdge(const TriangulatedGraph &start, const TriangulatedGraph &end) {
    TriangulatedGraph g = start;
    for (const Edge &e: g.getEdges()) {
        Edge result = g.flip(e);
        g.flip(result);
        assert(!end.hasEdge(result));
    }
    return true;
}

bool assertNonTrivial(const TriangulatedGraph &start, const TriangulatedGraph &end) {
    return assertNoCommonEdge(start, end) &&
           assertNoFreeEdge(start, end);
}

bool FlipDistanceSource::flipDistanceDecision(unsigned int k) {
    if (start == end) {
        return true;
    }
    TriangulatedGraph g = start;
    for (Edge e: g.getEdges()) {
        if (end.hasEdge(e)) {
            return splitAndSearch(g, e, (int) k, {});
        }
        Edge result = g.flip(e);
        if (end.hasEdge(result)) {
            bool ret = splitAndSearch(g, result, (int) k - 1, {});
            g.flip(result);
            return ret;
        }
        g.flip(result);
    }
    std::vector<std::vector<Edge>> sources = start.getSources();
    for (const auto &source: sources) {
        if (flipDistanceDecision(k, source)) {
            return true;
        }
    }
    return false;
}

bool isIndependentSet(const std::vector<Edge> &sources, const TriangulatedGraph &g) {
    for (const Edge &e: sources) {
        for (const Edge &e2: sources) {
            if (e != e2 && g.shareTriangle(e, e2)) {
                return false;
            }
        }
    }
    return true;
}

typedef std::pair<TriangulatedGraph, TriangulatedGraph> TriangulationPair;
typedef std::vector<std::pair<Edge, Edge>> EdgePairs;
typedef std::pair<TriangulationPair, EdgePairs> FDProblem;

std::vector<FDProblem>
performFreeFlips(const TriangulationPair &initialPair,
                 const EdgePairs &initialSource,
                 int &k) {
    std::stack<FDProblem> cur;
    std::vector<FDProblem> noFree;
    cur.push({initialPair, initialSource});
    while (!cur.empty()) {
        auto fd = cur.top();
        cur.pop();
        auto g1 = fd.first.first, g2 = fd.first.second;
        bool noFreeEdge = true;
        for (const Edge &e: g1.getEdges()) {
            Edge result = g1.flip(e);
            if (g2.hasEdge(result)) {
                noFreeEdge = false;
                k--;
                EdgePairs next = fd.second;
                next.erase(std::remove_if(next.begin(), next.end(), [=](auto pair) {
                    return pair.first == e || pair.second == e;
                }), next.end());
                addNeighbors(next, g1, result);
                int v1 = result.first, v2 = result.second;
                cur.push({{g1.subGraph(v1, v2), g2.subGraph(v1, v2)},
                          filterAndMapEdgePairs(next,
                                                TriangulatedGraph::getVertexFilter(v1, v2),
                                                g1.getVertexMapper(v1, v2))});
                cur.push({{g1.subGraph(v2, v1), g2.subGraph(v2, v1)},
                          filterAndMapEdgePairs(next,
                                                TriangulatedGraph::getVertexFilter(v2, v1),
                                                g1.getVertexMapper(v2, v1))});
                break;

            }
            g1.flip(result);
        }
        if (noFreeEdge) {
            noFree.push_back(fd);
        }
    }
    return noFree;
}

bool FlipDistanceSource::search(const std::vector<Edge> &sources, TriangulatedGraph g,
                                int k) { // keep as int; possible overflow for unsigned int
    branchCounter++;
    // sanity check
    assert(assertNonTrivial(g, end));
    assert(isIndependentSet(sources, g));
    if (g == end && k >= 0) {
        return true;
    }
    if (g.getSize() - 3 > k - sources.size()) {
        return false;
    }
    if (sources.empty()) {
        return false;
    }
    for (const Edge &e: g.getEdges()) {
        Edge result = g.flip(e);
        if (end.hasEdge(result)) {
            bool ret = std::count(sources.begin(), sources.end(), e) > 0 &&
                       splitAndSearch(g, result, k - 1, sources);
            g.flip(result);
            return ret;
        }
        g.flip(result);
    }
    std::vector<std::pair<Edge, Edge>> next;
    for (const Edge &e: sources) {
        assert(g.flippable(e));
        Edge result = g.flip(e);
        addNeighbors(next, g, result);
    }
    k -= (int) sources.size();
    auto result = performFreeFlips({g, end}, next, k);
    if (k < 0) {
        return false;
    }
    for (const auto &pair: result) {
        FlipDistanceSource algo(pair.first.first, pair.first.second);
        auto source = pair.second;
        int i = 0;
        for (; i <= k; ++i) {
            if (algo.search(source, algo.start, i)) {
                k -= i;
                break;
            }
        }
        if (i == k + 1) {
            return false;
        }
    }
    return k >= 0;
}

bool FlipDistanceSource::search(const std::vector<std::pair<Edge, Edge>> &sources, TriangulatedGraph g,
                                int k) { // keep as int; possible overflow for unsigned int
    // sanity check
    assert(assertNonTrivial(g, end));
    std::vector<Edge> cur;
    std::unordered_multiset<Edge> forbid;
    std::function<bool(int)> generateNext = [&](int index) -> bool {
        if (index == sources.size()) {
            return search(cur, g, k);
        }
        if (generateNext(index + 1)) {
            return true;
        }
        for (const Edge &e: {sources[index].first, sources[index].second}) {
            if (!g.flippable(e) || forbid.count(e) > 0) {
                continue;
            }
            addNeighborsToForbid(e, g, forbid);
            cur.push_back(e);
            auto ret = generateNext(index + 1);
            removeNeighborsFromForbid(e, g, forbid);
            cur.pop_back();
            if (ret) {
                return true;
            }
        }
        return false;
    };
    return generateNext(0);
}

bool FlipDistanceSource::splitAndSearch(const TriangulatedGraph &g,
                                        Edge &divider, int k,
                                        const std::vector<Edge> &sources) {
    if (k <= 0) {
        return g == end && k == 0;
    }
    int v1 = divider.first, v2 = divider.second;
    TriangulatedGraph s1 = g.subGraph(v1, v2), e1 = end.subGraph(v1, v2);
    auto sources1 = g.filterAndMapEdges(v1, v2, sources);
    TriangulatedGraph s2 = g.subGraph(v2, v1), e2 = end.subGraph(v2, v1);
    auto sources2 = g.filterAndMapEdges(v2, v1, sources);
    FlipDistanceSource algo(s1, e1);
    for (auto i = s1.getSize() - 3; i <= k; ++i) {
        // FIXME: use sources1 and sources2
        if (algo.flipDistanceDecision(i)) {
            FlipDistanceSource algo2(s2, e2);
            return algo2.flipDistanceDecision(k - i);
        }
    }
    return false;
}

std::vector<int> FlipDistanceSource::getStatistics() {
    return {branchCounter};
}

void resetBranchCounter() {
    branchCounter = 0;
}