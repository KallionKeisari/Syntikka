# Syntikka

## Väliraportti 22.11.

### Kansiorakenne / toteutetut asiat
- Builds-kansio sisältää konfiguraatiot ohjelmointialustoille. (EI MUOKATA)
- JuceLibraryCode-kansio sisältää juce-moduulit ja kirjastot. (EI MUOKATA)
- Source-kansiossa on ohjelman varsinainen koodi.
  - Oskillaattorit ovat main-tiedostossa (pohjautuu tutoriaalikoodiin)
  - Filtterille on omat tiedostot
 
Tällaisenaan syntikan voi avata ja sillä voi soittaa polyfonisesti siniaaltoäänellä käyttäen tietokoneen näppäimistöä. Vaippakäyrä, filtteri ja LFO vielä implementoimatta.
  
 
### TODO:t
- Modulaarisuuden edistäminen
  - Main-tiedoston hajauttaminen moduuleihin
  - Moduulien yhteensovitus
  - Filtterin yhteensovitus
- Siniaallon vaihtaminen saha-aalloksi
  - Tätä varten tarvitaan myös lowpass-filtteri aliasoitumisen estämiseksi.
- LFO
  - Koodi
  - yhteensovitus
- Vaippakäyrä eli ADSR (Attack, decay, sustain ja release -säätimet)
  - Koodi
  - yhteensovitus
- MIDI-yhteensopivuus

### Pohdittavaa / ongelmia:
- Oskillaattorin pitänee toimia taajuustasossa, jotta filtterin ja LFO:n implementointi on mahdollisimman aikatehokasta?
- ADSR:n eri säädöt taitavat löytyä pitkin koodia. Ainakin release-säädölle on koodissa jo parametri. Attack ja decay saataneen samoille alueille. Tarvitseeko sustain-säätö jonkun ajastimen jonnekin?
- Modulaarisuuden edistämiseen käytettiin jo kokonainen työpäivä ilman tuloksia. Siellä on siis jotain vaikeaa.
- LFO
  - Mitä aaltomuotoja LFO:hon? Siniaalto, kanttiaalto? Halutaanko sen olla vaihdettavissa?
  - Vaikuttaako LFO Taajuuteen vai esim filtteriin?
  - LFO:lle omat taajuus- ja intensiteettisäätimet?

  
