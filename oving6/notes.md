# Oppgave  notater komprimering

## Lempel ziv (lz)
Lagrer gjentagelser som referanser, 
Visst en fil inneholder ABCABCABC!
Så ser vi at ABC gjetnar seg selv tre ganger.
Så for hvert sted fra venstre til høyre spør den seg selv om den har sett det før.
Visst den finner noe som er gjentagende tar den istedenfor å lagre ABC lagrer den refernasen.
Algoritmen må også vurdere størrelsen på referansen i forhold til det man kopierer.



Komprimering psudokode hovedide:
````
while (pos < data.size()) {
    // Let tidligere i data etter det som begynner ved pos.
    auto match = finnBesteMatch(data, pos);

    if (referansenSparerPlass(match)) {
        skrivReferanse(match.avstand, match.lengde);
        pos += match.lengde;
    } else {
        skrivDirekteByte(data[pos]);
        pos += 1;
    }
}
````