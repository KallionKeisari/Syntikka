# Syntikka

### Kansiorakenne
- Builds-kansio sisältää konfiguraatiot ohjelmointialustoille
- JuceLibraryCode-kansio sisältää juce-moduulit ja kirjastot
- Source-kansiossa on ohjelman varsinainen koodi
  - Oskillaattorit ovat main-tiedostossa (pohjautuu tutoriaalikoodiin)
  - Filtterille on omat tiedostot
  - #TODO LFO ei low-frequency oscillator taajuus- tai filtterimodulointia varten
 
### TODO:t
- LFO
- ADSR (Attack, decay, sustain ja release -säätimet)
- MIDI-yhteensopivuus
- Siniaallon vaihtaminen saha-aalloksi
- Main-tiedoston hajauttaminen moduuleihin / Eri moduulien yhteensovitus