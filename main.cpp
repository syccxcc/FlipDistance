#include <iostream>
#include "triangulation/TriangulatedGraph.h"
#include "algo/flip_distance_middle.h"
#include "algo/flip_distance_bfs.h"
#include "algo/flip_distance_fast.h"
#include "algo/flip_distance_dfs.h"
#include "algo/flip_distance_simple.h"
#include "algo/flip_distance_source.h"
#include "triangulation/Helper.h"
#include <unordered_map>
#include <ctime>
#include <cstring>
#include <cassert>
#include <cstdio>

using namespace std;

void printTriangulation(const TriangulatedGraph &g) {
    printf("%lu\n", g.getSize());
    for (Edge e : g.getEdges()) {
        printf("%d %d\n", e.first, e.second);
    }
}

void measureFpt() {
    
}

FlipDistance* getAlgoByName(const std::string &name, TriangulatedGraph &g, TriangulatedGraph &g2) {
    if (name == "bfs") return new FlipDistanceBfs(g, g2);
    if (name == "dfs") return new FlipDistanceDfs(g, g2);
    if (name == "middle") return new FlipDistanceMiddle(g, g2);
    if (name == "simple") return new FlipDistanceSimple(g, g2);
    if (name == "fast") return new FlipDistanceFast(g, g2);
    if (name == "source") return new FlipDistanceSource(g, g2);
    printf("No algorithm named %s found.", name.c_str());
    exit(1);
}

int main(int argc, char **argv) {
    if (argc < 3) {
        fprintf(stderr, "Need at least 2 arguments.");
        return 1;
    }
    if (strcmp(argv[1], "-c") == 0) {
        BinaryString bs(treeStringToParentheses(argv[2]));
        printTriangulation(TriangulatedGraph(bs.getBits()));
        printf("%s\n", bs.toString().c_str());
        return 0;
    }
    std::string 
        s1 = std::string(argv[1]),
        s2 = std::string(argv[2]);
    
    TriangulatedGraph g(BinaryString(treeStringToParentheses(s1)).getBits());
    TriangulatedGraph g2(BinaryString(treeStringToParentheses(s2)).getBits());
    std::string name = argc > 3 ? argv[3] : "bfs";
    FlipDistance *m = getAlgoByName(name, g, g2);
    bool decision = false;
    if (argc > 4) {
        int input;
        sscanf(argv[4], "%d", &input);
        decision = input;
    }
    if (decision) {
        size_t start = 1, end = g.getSize() * 2 - 6;
        for (size_t i = start; i <= end; ++i) {
            clock_t startTime = clock();
            printf("%d ", m->flipDistanceDecision(i));
            clock_t endTime = clock();
            printf("%.2f\n", (double)(endTime - startTime) / CLOCKS_PER_SEC);
        }
    } else {
        clock_t startTime = clock();
        printf("%d\n", m->flipDistance());
        clock_t endTime = clock();
        printf("%.2f\n", (double)(endTime - startTime) / CLOCKS_PER_SEC);
        printf("0\n");
    }
    return 0;
}
