#include <cmath>
#include <iostream>
#include <chrono>
#include <iomanip>

double metode1(double x, int n);
double metode2(double x, int n);
double metode3(double x, int n);

int main() {

    int n = 11;
    double x = 5.0;

    // legges inn for å vise hele testsvaret for 5.0^11
    std::cout << std::fixed << std::setprecision(0);

    std::cout << "Metode 1 for 5.0^11: " << metode1(x,n) << std::endl;
    std::cout << "Metode 2 for 5.0^11: " << metode2(x,n) << std::endl;
    std::cout << "Metode 3 for 5.0^11: " << metode3(x,n) << std::endl;

    int tall[] = {100, 1000, 10000, 100000};

    for (int i = 0; i < 4; i ++) {
        int n = tall[i];
        int runder = 1000;
        volatile int maal_n = n;
        volatile double resultat = 0.0;

        // Metode 1
        auto start1 = std::chrono::steady_clock::now();
        for (int j = 0; j < runder; j++) {
            resultat = metode1(5.0, maal_n);
        }
        auto slutt1 = std::chrono::steady_clock::now();
        auto ns1 = std::chrono::duration_cast<std::chrono::nanoseconds>(slutt1 - start1).count();

        // Metode 2
        auto start2 = std::chrono::steady_clock::now();
        for (int j = 0; j < runder; j++) {
            resultat = metode2(5.0, maal_n);
        }
        auto slutt2 = std::chrono::steady_clock::now();
        auto ns2 = std::chrono::duration_cast<std::chrono::nanoseconds>(slutt2 - start2).count();

        // Metode 3
        auto start3 = std::chrono::steady_clock::now();
        for (int j = 0; j < runder; j++) {
            resultat = metode3(5.0, maal_n);
        }
        auto slutt3 = std::chrono::steady_clock::now();
        auto ns3 = std::chrono::duration_cast<std::chrono::nanoseconds>(slutt3 - start3).count();


        // Skriv ut gjennomsnitt per kall i nanosekunder
        std::cout << "\nn = " << n << std::endl;
        std::cout << "Metode 1: " << (ns1 / runder) << " ns/kall" << std::endl;
        std::cout << "Metode 2: " << (ns2 / runder) << " ns/kall" << std::endl;
        std::cout << "Metode 3: " << (ns3 / runder) << " ns/kall" << std::endl;

    }
    return 0;

}

double metode1(double x, int n) {
    if (n == 1) {
        return x;
    }
    return x*metode1(x, n-1);

}

double metode2(double x, int n) {
    if (n == 1) {
        return x;
    }
     if ((n & 1)  == 0) {
        return metode2(x * x, n / 2);
    }
     return x * metode2(x * x,(n - 1) / 2);
}

double metode3(double x, int n) {
    return pow(x,n);
}
