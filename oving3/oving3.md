# Øving 3 Petter Birkelund-Jensen

Alternativ 1: QuickSort sammenligning


Implementasjon:
- Single-pivot: Lærebok kap 3.9 (s.59-63), median3sort for å unngå skjevdeling på sorterte data
- Dual-pivot: GeeksforGeeks med 2 fikser: (1) bytte pivot-posisjoner for sorterte data, (2) skip midtdel ved like pivoter for duplikater

Måleoppsett og kontroll:
Begge algoritmene får identiske kopier av dataene. I duplikatdatasettet er annenhvert element 42. Bare selve sorteringen måles; kopiering, sjekksum og rekkefølgetest ligger utenfor tidsmålingen. Programmet skriver bare ut en tid dersom begge kontrollene består.

Tidsmålinger (ms):

| n         | Type      | Single | Dual  |
|-----------|-----------|--------|-------|
| 1M        | Tilfeldig | 145    | 144   |
| 1M        | Duplikater| 85     | 72    |
| 1M        | Sortert   | 27     | 68    |
| 1M        | Baklengs  | 50     | 67    |
| 10M       | Tilfeldig | 1324   | 1623  |
| 10M       | Duplikater| 1030   | 833   |
| 10M       | Sortert   | 305    | 784   |
| 10M       | Baklengs  | 587    | 814   |
| 50M       | Tilfeldig | 7228   | 8681  |
| 50M       | Duplikater| 5623   | 4680  |
| 50M       | Sortert   | 1849   | 4299  |
| 50M       | Baklengs  | 3098   | 4336  |

Analyse:
- Single-pivot er raskest på tilfeldige, sorterte og baklengs sorterte data ved 10M og 50M. Ved 1M tilfeldige tall er tidene omtrent like.
- Dual-pivot er raskest på duplikater. Kontrollen for like pivoter unngår unødvendig sortering av midtdelen.
- Pivotvalg og forskjeller i implementasjonen kan påvirke tidene; målingene isolerer ikke effekten av hvert enkelt grep.
- Begge har teoretisk Θ(n log n) kjøretid i gjennomsnitt.

Konklusjon:
På denne maskinenen er single-pivot raskest i tre av fire datatyper ved 50 millioner tall, mens dual-pivot er raskest på duplikater.
