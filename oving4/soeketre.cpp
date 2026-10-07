// Øving 4, deloppgave 2: Binært søketre med ord i nodene.
//
// Ordene leses inn fra brukeren og settes inn i treet i den rekkefølgen
// de oppgis. Ordet i hver node er sorteringsnøkkelen.
//
// Utskrift av de fire øverste nivåene: rotnoden får 64 tegns plass og
// ordet står midt i dette (ubrukte tegnplasser fylles med mellomrom).
// Ordene på neste nivå får 32 tegn hver, deretter 16 og 8. Det settes
// også av plass for noder som ikke finnes (tomme barneplasser).

#include <iostream>
#include <string>
#include <vector>

using namespace std;

const int NUM_LEVELS = 4;
const int ROOT_WIDTH = 64;   // tegnplasser til rotnoden

struct Node {
    string word;
    Node* left;
    Node* right;
    explicit Node(const string& w) : word(w), left(nullptr), right(nullptr) {}
};

class SearchTree {
private:
    Node* root = nullptr;

    // Setter ordet inn i subtreet med "node" som rot.
    static Node* insert(Node* node, const string& word) {
        if (node == nullptr) return new Node(word);
        if (word < node->word) {
            node->left = insert(node->left, word);
        } else if (word > node->word) {
            node->right = insert(node->right, word);
        }
        // Like ord blir ignorert.
        return node;
    }

    // Sletter alle nodene i subtreet (ettertraversering).
    static void destroy(Node* node) {
        if (node == nullptr) return;
        destroy(node->left);
        destroy(node->right);
        delete node;
    }

    // Skriver ordet til "node" inn i ruten for nivå "level".
    // "index" er nodens plass på nivået (0 = lengst til venstre).
    // Tomme plasser får stående som mellomrom.
    static void placeNode(const Node* node, int level, int index,
                          vector<string>& rows, bool& hasDeeper) {
        if (node == nullptr) return;   // tom plass: plassen reserveres
        if (level >= NUM_LEVELS) {
            hasDeeper = true;
            return;
        }

        int cellWidth = ROOT_WIDTH / (1 << level);   // 64, 32, 16, 8
        int center = index * cellWidth + cellWidth / 2;
        int length = static_cast<int>(node->word.size());
        if (length > ROOT_WIDTH) length = ROOT_WIDTH;
        int start = center - length / 2;
        if (start < 0) start = 0;
        if (start + length > ROOT_WIDTH) start = ROOT_WIDTH - length;

        rows[level].replace(start, length, node->word, 0, length);

        // Barnenodene havner på plassene 2*index og 2*index + 1.
        placeNode(node->left, level + 1, index * 2, rows, hasDeeper);
        placeNode(node->right, level + 1, index * 2 + 1, rows, hasDeeper);
    }

public:
    SearchTree() = default;
    ~SearchTree() { destroy(root); }

    SearchTree(const SearchTree&) = delete;
    SearchTree& operator=(const SearchTree&) = delete;

    void insert(const string& word) { root = insert(root, word); }

    void display() const {
        if (root == nullptr) {
            cout << "(treet er tomt)" << endl;
            return;
        }

        // Én tekstlinje per nivå, klargjort med mellomrom.
        vector<string> rows(NUM_LEVELS, string(ROOT_WIDTH, ' '));
        bool hasDeeper = false;

        placeNode(root, 0, 0, rows, hasDeeper);

        for (const string& row : rows) {
            // Ta bort mellomrom på slutten av linja.
            size_t last = row.find_last_not_of(' ');
            if (last == string::npos) cout << endl;
            else cout << row.substr(0, last + 1) << endl;
        }
        if (hasDeeper) {
            cout << "(treet har flere nivåer, viser bare de "
                 << NUM_LEVELS << " øverste)" << endl;
        }
    }
};

int main(int argc, char* argv[]) {
    SearchTree tree;

    if (argc > 1) {
        // Ord direkte på kommandolinja: ./soeketre hode bein hals ...
        for (int i = 1; i < argc; i++) {
            tree.insert(argv[i]);
        }
    } else {
        cout << "Binært søketre: skriv inn ord, adskilt med mellomrom."
             << endl;
        cout << "Ordene settes inn i treet i den rekkefølgen du skriver dem."
             << endl;
        cout << "Avslutt med ctrl-d." << endl;
        string word;
        while (cin >> word) {
            tree.insert(word);
        }
    }

    tree.display();
    return 0;
}
