#include <fstream>
#include <iostream>
#include <vector>
#include <queue>

//Innlesnings metode
std::vector<std::vector<int>> readGraph(const std::string& filename, int& n) {
    std::ifstream file(filename);
    if (!file) {
        std::cerr << "Could not open the file" << '\n';
        exit(1);
    }

    int totalEdges;
    file >> n >> totalEdges;

    std::vector<std::vector<int>> cap(n,std::vector<int>(n,0));

    struct Edge { int from, to, k;};
    std::vector<Edge> edges;

    for (int i = 0; i < totalEdges; i++) {
        Edge edge;
        file >> edge.from >> edge.to >> edge.k;
        edges.push_back(edge);
    }

    for (const Edge& edge : edges) {
        cap[edge.from][edge.to] = edge.k;
    }

    //Setter lik 0 kun der det ikke er en ekte kant.
    for (const Edge& edge : edges) {
        if (cap[edge.to][edge.from] == 0) {
            cap[edge.to][edge.from] = 0;
        }
    }
    return cap;
}

// Finner flytøkenede vei, returnerer forgjenger-tabellen
std::vector<int> bfs(const std::vector<std::vector<int>>& cap,
    const std::vector<std::vector<int>>& flow,
    int source) {

    int n = cap.size();

    std::vector<bool> visited(n, false);
    std::vector<int> prev(n,-1);
    std::queue<int> q;

    visited[source] = true;
    q.push(source);

    while (!q.empty()) {
        int curr = q.front();
        q.pop();

        for (int y = 0; y < n; y++) {
            if (!visited[y] && cap[curr][y] - flow[curr][y] > 0) {
                visited[y] = true;
                prev[y] = curr;
                q.push(y);
            }
        }
    }
    return prev;
}


int main(int argc, char* argv[]) {
    // Sjekk at brukeren ga med en graffil på kommandolinja.
    if (argc < 4) {
        std::cerr << "Use: " << argv[0] << " <graphfile> <source> <sink>" << std::endl;
        return 1;
    }

    int n;
    std::vector<std::vector<int>> cap = readGraph(argv[1], n);

    std::vector<std::vector<int>> flow(n, std::vector<int>(n, 0));

    int source = std::stoi(argv[2]);
    int sink   = std::stoi(argv[3]);
    int totalFlow = 0;

    while (true) {

        // Kjør bfs
        std::vector<int> prev = bfs(cap, flow, source);

        // Sjekk om den flyten nådde frem
        if (prev[sink] == -1) break;

        // Lager veien som blir fulgt i søket
        std::vector<int> path;
        for (int node = sink; node != -1; node = prev[node]) {
            path.push_back(node);
        }
        std::reverse(path.begin(), path.end());

        // fimn kanten på veien som har mist ledig restkapasitet.
        int bottleneck = INT_MAX;
        for (int i = 0; i + 1 < (int)path.size(); i++) {
            int x = path[i];
            int y = path[i + 1];
            int rest = cap[x][y] - flow[x][y];
            if (rest < bottleneck) {
                bottleneck = rest;
            }
        }

        // Send flyt langs veien:
        for (int i = 0; i + 1 < (int)path.size(); i++) {
            int x = path[i];
            int y = path[i + 1];
            flow[x][y] += bottleneck;
            flow[y][x] -= bottleneck;
        }

        totalFlow += bottleneck;

        // Skriv ut veien
        std::cout << bottleneck << "  ";
        for (int node : path) {
            std::cout << node << " ";
        }
        std::cout << std::endl;
    }

    std::cout << "Maksimal flyt ble " << totalFlow << std::endl;
    return 0;
}
