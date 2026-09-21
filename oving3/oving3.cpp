#include <iostream>
#include <vector>
#include <chrono>
#include <cstdlib>
#include <ctime>
#include <iomanip>
#include <functional>
#include <algorithm>

using namespace std;

//  HJELPEFUNKSJONER

void bytt(vector<int>& t, int i, int j) {
    int tmp = t[i];
    t[i] = t[j];
    t[j] = tmp;
}

long long sjekksum(const vector<int>& arr) {
    long long sum = 0;
    for (int x : arr) sum += x;
    return sum;
}

bool er_sortert(const vector<int>& arr) {
    for (size_t i = 0; i + 1 < arr.size(); i++) {
        if (arr[i+1] < arr[i]) return false;
    }
    return true;
}

//  SINGLE-PIVOT QUICKSORT (fra lærebok s. 59-63)

int median3sort(vector<int>& t, int v, int h) {
    int m = (v + h) / 2;
    if (t[v] > t[m]) bytt(t, v, m);
    if (t[m] > t[h]) {
        bytt(t, m, h);
        if (t[v] > t[m]) bytt(t, v, m);
    }
    return m;
}

int splitt(vector<int>& t, int v, int h) {
    int m = median3sort(t, v, h);
    int dv = t[m];
    bytt(t, m, h - 1);

    int iv = v, ih = h - 1;
    for (;;) {
        while (t[++iv] < dv);
        while (t[--ih] > dv);
        if (iv >= ih) break;
        bytt(t, iv, ih);
    }
    bytt(t, iv, h - 1);
    return iv;
}

void quicksort_single(vector<int>& t, int v, int h) {
    if (h - v > 2) {
        int delepos = splitt(t, v, h);
        quicksort_single(t, v, delepos - 1);
        quicksort_single(t, delepos + 1, h);
    } else {
        median3sort(t, v, h);
    }
}

void quicksort_single(vector<int>& t) {
    if (t.size() > 1) quicksort_single(t, 0, t.size() - 1);
}

//  DUAL-PIVOT QUICKSORT

pair<int,int> partitionDual(vector<int>& arr, int low, int high) {
    // Fikspunkt 1: Unngå skjevdeling på sorterte data
    int posLow = low + (high - low) / 3;
    int posHigh = high - (high - low) / 3;
    bytt(arr, low, posLow);
    bytt(arr, high, posHigh);

    if (arr[low] > arr[high]) bytt(arr, low, high);

    int p1 = arr[low];
    int p2 = arr[high];

    int lt = low + 1;
    int gt = high - 1;
    int i = low + 1;

    while (i <= gt) {
        if (arr[i] < p1) {
            bytt(arr, i, lt);
            lt++;
            i++;
        } else if (arr[i] > p2) {
            bytt(arr, i, gt);
            gt--;
        } else {
            i++;
        }
    }

    bytt(arr, low, lt - 1);
    bytt(arr, high, gt + 1);

    return {lt - 1, gt + 1};
}

void quicksort_dual(vector<int>& arr, int low, int high) {
    if (low < high) {
        auto [p1pos, p2pos] = partitionDual(arr, low, high);

        quicksort_dual(arr, low, p1pos - 1);
        if (arr[p1pos] != arr[p2pos]) {
            quicksort_dual(arr, p1pos + 1, p2pos - 1);
        }
        quicksort_dual(arr, p2pos + 1, high);
    }
}

void quicksort_dual(vector<int>& arr) {
    if (arr.size() > 1) quicksort_dual(arr, 0, arr.size() - 1);
}

//  DATAGENERATORER
vector<int> generer_tilfeldig(int n) {
    vector<int> arr(n);
    for (int i = 0; i < n; i++) arr[i] = rand() % 10000000;
    return arr;
}

vector<int> generer_duplikater(int n) {
    vector<int> arr(n);
    for (int i = 0; i < n; i++) arr[i] = (i % 2 == 0) ? 42 : rand() % 10000000;
    return arr;
}

vector<int> generer_sortert(int n) {
    vector<int> arr(n);
    for (int i = 0; i < n; i++) arr[i] = i;
    return arr;
}

vector<int> generer_baklengs(int n) {
    vector<int> arr(n);
    for (int i = 0; i < n; i++) arr[i] = n - i;
    return arr;
}

//  TIDSMÅLING + TESTING

void mal_tid(string navn, void (*sorter)(vector<int>&), vector<int> data) {
    long long sum_for = sjekksum(data);

    auto start = chrono::steady_clock::now();
    sorter(data);
    auto slutt = chrono::steady_clock::now();

    bool ok = true;
    if (!er_sortert(data)) {
        cerr << "FEIL: " << navn << " sorterte IKKE korrekt!" << endl;
        ok = false;
    }
    if (sjekksum(data) != sum_for) {
        cerr << "FEIL: " << navn << " mistet data!" << endl;
        ok = false;
    }

    double ms = chrono::duration<double, milli>(slutt - start).count();
    cout << "    " << navn << ": ";
    if (ok) {
        cout << fixed << setprecision(2) << ms << " ms" << endl;
    } else {
        cout << "FEIL" << endl;
    }
}


int main() {
    srand(time(0));

    // n verdier vi tester med
    vector<int> n_values = {1000000, 10000000, 50000000};

    for (int n : n_values) {
        cout << "\n========================================" << endl;
        cout << "n = " << n << endl;

        // Test 1: Tilfeldig
        {
            auto d1 = generer_tilfeldig(n);
            auto d2 = d1;
            cout << "\n  Tilfeldig:" << endl;
            mal_tid("Single", quicksort_single, d1);
            mal_tid("Dual  ", quicksort_dual, d2);
        }

        // Test 2: Duplikater
        {
            auto d1 = generer_duplikater(n);
            auto d2 = d1;
            cout << "\n  Duplikater:" << endl;
            mal_tid("Single", quicksort_single, d1);
            mal_tid("Dual  ", quicksort_dual, d2);
        }

        // Test 3: Sortert
        {
            auto d1 = generer_sortert(n);
            auto d2 = d1;
            cout << "\n  Sortert:" << endl;
            mal_tid("Single", quicksort_single, d1);
            mal_tid("Dual  ", quicksort_dual, d2);
        }

        // Test 4: Baklengs
        {
            auto d1 = generer_baklengs(n);
            auto d2 = d1;
            cout << "\n  Baklengs:" << endl;
            mal_tid("Single", quicksort_single, d1);
            mal_tid("Dual  ", quicksort_dual, d2);
        }
    }

    return 0;
}
