# Øving 4 Petter Birkelund-Jensen

Lenka lister og trær

## Deloppgave 1: Lange tall (langetall.cpp)

Implementasjon:
- Tallene lagres som dobbeltlenka lister (egen Node-struct med prev/next) der hvert element har ett siffer
- Hodet peker på mest betydende siffer, halen på enerne
- Addisjon og subtraksjon går gjennom begge listene fra halen og mot hodet, ett siffer om gangen, med overføring (sum > 9) og lån (negativt siffer)
- a - b med a < b regnes ut som b - a med minustegn foran

## Deloppgave 2: Binært søketre (soeketre.cpp)

Implementasjon:
- Egen Node-struct med ord + venstre/høyre-peker; innsetting i den rekkefølgen ordene oppgis, duplikater ignoreres
- Utskrift av de fire øverste nivåene: rotnoden får 64 tegns plass, deretter 32, 16 og 8, ordet står midt i sin rute. Tomme barneplasser reserverer plass i utskriften
