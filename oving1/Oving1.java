import java.util.Random;

public class Oving1 {

    // Tar inn kursendringene og gjør det om til aksjepris over tid
    public static int[] kursOverTid(int[] kursEndringer) {
        int[] kurser = new int[kursEndringer.length];
        int kurs = 0;

        for (int i = 0; i < kursEndringer.length; i++) {
            kurs += kursEndringer[i];
            kurser[i] = kurs;
        }
        return kurser;
    }

    // kursbergening() inneholdeer algorytmen som tar inn
    // kursendringene å beregner når man skal selge å kjøpe
    public static void kursberegning(int[] kursForandring) {
        int[] kurser = kursOverTid(kursForandring);

        int besteProfitt = Integer.MIN_VALUE;
        int kjøpsDag = -1;
        int salgsDag = -1;

        for (int kjøp = 0; kjøp < kurser.length; kjøp++) {
            for (int salg = kjøp + 1; salg < kurser.length; salg++) {
                int profitt = kurser[salg] - kurser[kjøp];

                if (profitt > besteProfitt) {
                    besteProfitt = profitt;
                    kjøpsDag = kjøp;
                    salgsDag = salg;
                }
            }
        }

        System.out.println("Kjøp ble gjort på dag " + (kjøpsDag + 1));
        System.out.println("Salg ble gjort på dag " + (salgsDag + 1));
        System.out.println("Total fortjeneste ved kjøp og salg: " + besteProfitt);
    }

    // målTid() tar inn n som er probelsmtørrelsen 
    // å beregner kjøretiden i ms
    static void målTid(int n) {
        int[] data = genererData(n);
        int[] kurser = kursOverTid(data);

        long start = System.nanoTime();
        int runder = 0;
        int besteProfitt;

        do {
            besteProfitt = Integer.MIN_VALUE;

            for (int kjøp = 0; kjøp < n; kjøp++) {
                for (int salg = kjøp + 1; salg < n; salg++) {
                    int profitt = kurser[salg] - kurser[kjøp];

                    if (profitt > besteProfitt) {
                        besteProfitt = profitt;
                    }
                }
            }

            runder++;
        } while (System.nanoTime() - start < 1_000_000_000L);

        long totalTid = System.nanoTime() - start;
        double msPerRunde = totalTid / 1_000_000.0 / runder;

        System.out.println(
            "n=" + n
            + " bruker " + msPerRunde + " ms"
            + " (" + runder + " runder)");
    }

    // genererData() brukes for å ha forskjellige
    //  problemstørrelser til å finne kjøretiden
    static int[] genererData(int n) {
        Random random = new Random();
        int[] data = new int[n];

        for (int i = 0; i < n; i++) {
            data[i] = random.nextInt(21) - 10;
        }

        return data;
    }

    public static void main(String[] args) {
        int[] testData = {-1, 3, -9, 2, 2, -1, 2, -1, -5};

        System.out.println("Med oppgavens data");
        kursberegning(testData);

        System.out.println();
        System.out.println("Tidsmålinger:");

        målTid(1000);
        målTid(10000);
        målTid(100000);
    }
}