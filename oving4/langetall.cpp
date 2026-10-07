// Øving 4, deloppgave 1: Lange tall med dobbeltlenka lister.
//
// Tallene lagres som dobbeltlenka lister der hvert element inneholder
// ett siffer. Hodet peker på det mest betydende sifferet og halen på
// det minst betydende (enerne). Regneoperasjonene går derfor gjennom
// listene fra halen og mot hodet, mens utskrift går fra hodet.

#include <iostream>
#include <string>

using namespace std;

struct Node {
    char digit;      // ett siffer, '0'-'9'
    Node* prev;
    Node* next;
};

class LongNumber {
private:
    Node* head = nullptr;   // mest betydende siffer
    Node* tail = nullptr;  // minst betydende siffer
    int count = 0;

    // Legger et siffer først i lista (nytt hode).
    void pushFront(char digit) {
        Node* newNode = new Node{digit, nullptr, head};
        if (head != nullptr) head->prev = newNode;
        else tail = newNode;
        head = newNode;
        count++;
    }

    // Legger et siffer sist i lista (ny hale).
    void pushBack(char digit) {
        Node* newNode = new Node{digit, tail, nullptr};
        if (tail != nullptr) tail->next = newNode;
        else head = newNode;
        tail = newNode;
        count++;
    }

    // Fjerner det fremste elementet i lista.
    void popFront() {
        Node* toDelete = head;
        head = head->next;
        delete toDelete;
        if (head != nullptr) head->prev = nullptr;
        else tail = nullptr;
        count--;
    }

public:
    LongNumber() = default;

    // Bygger tallet fra en sifferstreng, f.eks. "12345".
    // Ledende nuller hoppes over ("0007" blir tallet 7).
    explicit LongNumber(const string& digits) {
        size_t i = 0;
        while (i + 1 < digits.size() && digits[i] == '0') i++;
        for (; i < digits.size(); i++) pushBack(digits[i]);
    }

    ~LongNumber() {
        while (head != nullptr) popFront();
    }

    // Flyttekonstruktør: lar resultatet returneres fra add()/subtract()
    // uten å kopiere alle nodene.
    LongNumber(LongNumber&& other) noexcept
        : head(other.head), tail(other.tail), count(other.count) {
        other.head = nullptr;
        other.tail = nullptr;
        other.count = 0;
    }

    LongNumber(const LongNumber&) = delete;
    LongNumber& operator=(const LongNumber&) = delete;

    // Sann hvis *this er et mindre tall enn other.
    bool lessThan(const LongNumber& other) const {
        if (count != other.count) return count < other.count;
        // Like mange sifre: finn første siffer som skiller.
        const Node* p = head;
        const Node* q = other.head;
        while (p != nullptr) {
            if (p->digit != q->digit) return p->digit < q->digit;
            p = p->next;
            q = q->next;
        }
        return false;   // tallene er like
    }

    // Addisjon: går gjennom begge listene fra halen (enerne) og mot hodet.
    // Sifre som summerer til mer enn 9 gir overføring til neste siffer.
    LongNumber add(const LongNumber& other) const {
        LongNumber result;
        const Node* p = tail;
        const Node* q = other.tail;
        int carry = 0;

        while (p != nullptr || q != nullptr || carry != 0) {
            int sum = carry;
            if (p != nullptr) {
                sum += p->digit - '0';
                p = p->prev;
            }
            if (q != nullptr) {
                sum += q->digit - '0';
                q = q->prev;
            }
            carry = sum / 10;
            // Enernesifferet regnes ut først, så det settes inn fremst.
            result.pushFront('0' + sum % 10);
        }
        return result;
    }

    // Subtraksjon: forutsetter at *this >= other (ikke-negative svar).
    // Når et siffer blir negativt lånes det 10 fra neste siffer.
    LongNumber subtract(const LongNumber& other) const {
        LongNumber result;
        const Node* p = tail;
        const Node* q = other.tail;
        int borrow = 0;

        while (p != nullptr) {
            int difference = (p->digit - '0') - borrow;
            if (q != nullptr) {
                difference -= q->digit - '0';
                q = q->prev;
            }
            if (difference < 0) {
                difference += 10;    // må låne fra neste siffer
                borrow = 1;
            } else {
                borrow = 0;
            }
            p = p->prev;
            result.pushFront('0' + difference);
        }

        // Fjern ledende nuller (100 - 99 skal bli 1, ikke 001).
        while (result.count > 1 && result.head->digit == '0') {
            result.popFront();
        }
        return result;
    }

    string toString() const {
        string text;
        for (const Node* p = head; p != nullptr; p = p->next) {
            text += p->digit;
        }
        return text;
    }
};

// Sjekker at teksten bare inneholder sifre.
static bool isValid(const string& text) {
    if (text.empty()) return false;
    for (char c : text) {
        if (c < '0' || c > '9') return false;
    }
    return true;
}

static void compute(const string& aText, const string& opSymbol,
                    const string& bText) {
    LongNumber a(aText);
    LongNumber b(bText);

    cout << a.toString() << ' ' << opSymbol << ' ' << b.toString()
         << " = ";

    if (opSymbol == "+") {
        cout << a.add(b).toString() << endl;
    } else if (a.lessThan(b)) {
        // a - b blir negativt: regn ut b - a og sett minustegn foran.
        cout << '-' << b.subtract(a).toString() << endl;
    } else {
        cout << a.subtract(b).toString() << endl;
    }
}

int main(int argc, char* argv[]) {
    // Kommandolinje: ./langetall <tall> < + | - > <tall>
    if (argc == 4) {
        string a = argv[1];
        string op = argv[2];
        string b = argv[3];
        if (!isValid(a) || !isValid(b) || (op != "+" && op != "-")) {
            cerr << "Bruk: " << argv[0] << " <tall> < + | - > <tall>"
                 << endl;
            return 1;
        }
        compute(a, op, b);
        return 0;
    }

    cout << "Lange tall: skriv inn et regnestykke på formen" << endl;
    cout << "   <tall> < + | - > <tall>" << endl;
    cout << "Avslutt med ctrl-d." << endl;

    string a, op, b;
    while (true) {
        cout << "> ";
        if (!(cin >> a >> op >> b)) break;
        if (!isValid(a) || !isValid(b) || (op != "+" && op != "-")) {
            cout << "Ugyldig innputt, prøv igjen." << endl;
            continue;
        }
        compute(a, op, b);
    }
    return 0;
}
