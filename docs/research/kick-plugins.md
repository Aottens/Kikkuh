# Kick-plugins: vergelijkend warenonderzoek

Stand: 3 november 2026. Doel: bepalen wat Kikkuh moet zijn, en vooral wat het anders moet doen.

**Wat dit wel en niet is.** Dit is een bureau-onderzoek op basis van reviews, productpagina's en
gebruikers op fora. Ik heb niets beluisterd of zelf bediend, dus over klank zegt dit weinig.
Forumreacties komen deels uit 2024; plugins zijn sindsdien bijgewerkt. Gearspace blokkeerde het
lezen en Reddit leverde niets op, dus de gebruikersstem komt vooral van KVR en Polarity (een
Bitwig-producer). Prijzen zijn momentopnames.

## Conclusie

1. **Alle grote kick-synths zijn hetzelfde idee:** een paar lagen, per laag node-curves voor pitch
   en volume, en daarachter een effectenrek. Ze verschillen in de afwerking, niet in het model.
2. **Kick Ninja geldt bij meerdere gebruikers juist als de snelste workflow**, en jou overtuigde
   die niet. Mijn hypothese (nog niet bewezen): je probleem zit niet in de uitwerking van Kick
   Ninja, maar in het model zelf, waarin je een geluid moet vertalen naar punten op een curve.
3. **Rumble is een losse productcategorie geworden.** Kikzilla 2, Maddix Rumble, Soundspear Rumble en
   Seaquake bestaan omdat de kick-synths dit niet als eigen sectie hebben. Voor techno is kick plus
   rumble juist het geluid. Dit is het duidelijkste gat in de markt.
4. **CLAP ontbreekt overal:** bij geen van de vier grote kick-synths vond ik CLAP in de genoemde
   formaten. Alleen het gratis Phykick noemt het. Klein voordeel in Bitwig, geen hoofdzaak.

## De kandidaten

| Plugin | Prijs | Motor | Import van kick | Formaten | Beste aan | Zwakste aan |
| --- | --- | --- | --- | --- | --- | --- |
| **Sonic Academy Kick 3** | £64–80 | 5 parallelle lagen (sub of sample), tot 9 harmonischen per sub, 15 effecten | Ja, sterk maar wisselend | VST3, AU, AAX | Log-tijdas, presets (540+), Trigger-modus | Tabbladen en lagen doorklikken, macro stuurt maar één effect, weinig modulatie |
| **The Him DSP Kick Ninja** | ca. $60 | Morphende oscillator + 3 samplelagen, 28 envelopes, oversampled FX | Ja, sterk maar wisselend | VST3, AU, AAX | Alles op één scherm, curves op distortion en harmonischen, nootliniaal | Drukke GUI, 70 presets, zoomklachten in 2024 |
| **Audija KickDrum** | €44 | Modulaire keten: sinus, wavetable, sample, EQ, saturator, clipper | Ja, vraagt meer nawerk | VST3, AU | Precisie, phase lock, punch, prijs | GUI verwarrend, sample-import onhandig, weinig presets, geen goede handleiding (2024) |
| **d16 PunchBox 2** | $109 / €99 | 606/808/909/sample/wavetable + 3 samplelagen, patchbay | Niet gevonden (wel pitch-detector) | VST3, VST2, AU, AAX | Hardware-gevoel, directe resultaten, analoge variatie, 1100+ presets | Prijs, overweldigend bij start, sinusgenerator verwijderd, back panel verstopt |
| **Plugin Boutique BigKick** | niet nagekeken | Sub-synth + click/top-laag, EQ, overdrive, hard clipper | Niet gevonden | niet nagekeken | Presets, slepen van en naar de DAW | Minder diep; alleen oppervlakkig onderzocht |
| **F9 KiCK! (Argon Drums)** | £29,50 | Geen synth: 127 samples, gemapt op racks en Kontakt | n.v.t. | Rack/Kontakt/etc. | Licht en bruikbaar | Curves niet bewerkbaar |

**Rumble-specialisten (geen volledige kick-synth, behalve waar vermeld)**

| Plugin | Prijs | Wat het doet | Interessant voor ons |
| --- | --- | --- | --- |
| **SINEE Kikzilla 2** | €99 (intro €79) | Rumble onder de kick, 2 tempo-gesyncte envelopes voor level en high cut, aparte FX-ketens voor kick, rumble en som | Het sterkste voorbeeld: stelt "wanneer komt de bas na de kick" als tempovraag |
| **Maddix Rumble** | niet nagekeken | Kick-sample naar rumble met delay en reverb, 62 presets | Reset-knoppen die de staart afkappen op de volgende kick |
| **Soundspear Rumble** | niet nagekeken | Rumble uit je kick | Kan alleen de staart uitsturen voor eigen bewerking |
| **Codwaves Seaquake** | gratis | Eenvoudige rumble | Gratis instap; voor extreme kicks heb je er extra plugins bij nodig |
| **QiOO SUBTERRAIN Rumble Kick** | €59 (intro €29) | Gesynthetiseerde kick + ritmische lange reverb, stappensequencer voor velocity (macOS, sept. 2026) | Nieuwste poging kick + rumble in één; nog geen reviews gevonden |
| **G-Sonique KFXT-41** | ca. €35 | Effect: saturatie, resonatoren, mono, reverb op een bestaande kick (Windows) | Knoppen die klinken als wat ze doen |

## Workflow per onderwerp

| Onderwerp | Kick 3 | Kick Ninja | KickDrum | PunchBox 2 |
| --- | --- | --- | --- | --- |
| Alles op één scherm | Nee, tabs en lagen | Ja, maar druk | Keten, UI verwarrend | Vrijwel, plus Advanced-venster en verstopt back panel |
| Attack nauwkeurig bewerken | Log-tijdas | Zoom; klachten in 2024, nu "extreme zoom" geadverteerd | Zoom geprezen, log optioneel | Advanced-envelopes, oordeel onbekend |
| Staart blijft staan als je de attack bewerkt | Phase-lock per node (rechtsklik, later toegevoegd) | Phase-lock per node (rechtsklik) | Phase lock, "beter dan de rest" volgens een gebruiker | Niet gevonden |
| Lagen | Parallel over de hele kick | Attack gaat over in oscillator; import legt nu ook ruis over de oscillator | Keten met devices | Generator + samplelagen |
| Curves op drive/harmonischen | Bètastatus bij de review | Ja, geprezen | Ja, maar slecht gedocumenteerd | Pitch en amp |
| Snelle knoppen | 4 macro's, elk op één effect | Niet gevonden | Niet gevonden | Ja, hardware-frontpaneel |
| Rumble als eigen sectie | Niet gevonden | Niet gevonden | Niet gevonden | Niet gevonden; envelopes wel bruikbaar voor rumble |

## Terugkerende frustraties

- **Tijdas.** De eerste 10 ms bepalen de klik en de rest is staart. Op een lineaire as moet je
  voortdurend in- en uitzoomen; die klacht komt bij meerdere plugins terug.
- **Bewerk de attack en de staart verschuift.** Een andere pitchcurve verandert de fase van alles
  daarna, dus ook de uitlijning met je bas. Iedereen lost dit op met een extra handeling:
  rechtsklikken op een node. Het is standaardgedrag dat je moet uitzetten, en dat is omgekeerd.
- **Klikken door lagen en tabs.** Polarity vond Kick 3 om die reden minder intuïtief dan Kick Ninja.
- **Drukte.** Kick Ninja en KickDrum krijgen allebei commentaar op de GUI: veel envelopes,
  overlappende elementen.
- **Importeren is geen magie.** Hetzelfde sample lukt in de ene plugin wel en in de andere niet,
  en er zijn meldingen van afgekapte staarten en verdwenen hoog. De meningen spreken elkaar tegen.
- **Weinig beweging.** Kick 3 krijgt kritiek op ontbrekende velocity- en envelope-modulatie.
  Een gebruiker miste in 2024 host-automatisering van nodes in Kick Ninja en KickDrum; niet
  nagekeken of dat nog geldt.

**Tegenstrijdig:** MusicTech noemt Kick 3 het meest intuïtief van de node-editors; Polarity en KVR
noemen juist Kick Ninja. "Intuïtief" betekent hier dus vooral: welke curve-editor stoort het minst.

## Waar Kikkuh kan winnen

Mijn eigen voorstel, geen onderzoeksresultaat. Toetsen in het spec-gesprek.

1. **Praten in wat je hoort, niet in nodes.** Vaste secties zoals Click, Punch, Body, Tail, Drive,
   Rumble. De knoppen schrijven zichtbaar in de curve, en wie wil pakt de curve zelf vast.
2. **De staart ligt standaard vast.** Attack bewerken verandert nooit de fase of toon van de staart.
3. **Log-tijdas als standaard, geen zoom nodig.** Eén scherm, geen tabs.
4. **Noot als instelling.** Je kiest de toonsoort-noot en de staart zit erop; de as leest in noten.
5. **Rumble als eerste klas sectie.** Op de host-tempo, gereset door de volgende kick, met eigen
   uitgang of mute om apart te kunnen bewerken.
6. **CLAP en VST3,** Bitwig als eerste host.

**Kandidaten om juist niet te bouwen in v1:** AI-import (duur, wisselend resultaat, en het is precies
waar de concurrenten al sterk zijn), grote presetbibliotheek en samplebrowser, rek met 15 effecten.

## Bronnen

- [MusicTech: best kick drum plugins 2026](https://musictech.com/guides/buyers-guide/best-kick-drum-plugins-2026-tested-reviewed/)
- [MusicTech: Kick 3 review](https://musictech.com/reviews/plug-ins/sonic-academy-kick-3-review/)
- [CDM: PunchBox 2 review](https://cdm.link/d16-punchbox-2-kick-review/)
- [Rekkerd: Kick Ninja review](https://rekkerd.org/review-kick-ninja-synth-plugin-by-the-him-dsp/)
- [Plugin Boutique: Kick Ninja](https://www.pluginboutique.com/product/1-Instruments/139-Drum-Synth/13436-Kick-Ninja)
- [Polarity: Kick 3](https://polarity.me/posts/polarity-music/2024-09-24-kick-3-sonic-academy/)
- [Polarity: Kick Ninja en Kick 3-updates](https://polarity.me/posts/polarity-music/2024-10-03-kick-ninja-kick3-updates-notegrabber-and-spectrum-analyzer/)
- [KVR: Audija KickDrum vs Kick 3 vs Kick Ninja](https://www.kvraudio.com/forum/viewtopic.php?p=8999764)
- [KVR: Kick Ninja als alternatief](https://www.kvraudio.com/forum/viewtopic.php?t=614301&start=15)
- [KVR: Audija KickDrum-draad](https://www.kvraudio.com/forum/viewtopic.php?p=8987693)
- [Rekkerd: KickDrum v2.8](https://rekkerd.org/audija-updates-kickdrum-modular-kick-drum-plugin-to-v2-8/)
- [Rekkerd: Kikzilla 2](https://rekkerd.org/sinee-kikzilla-2-brings-rumble-design-bassline-shaping-and-kick-bus-processing-into-one-plugin/)
- [KVR: Maddix Rumble](https://www.kvraudio.com/product/maddix-rumble-by-revealed-recordings)
- [KVR: Soundspear Rumble](https://www.kvraudio.com/product/rumble---techno-kick-generator-by-soundspear/details)
- [BedroomProducersBlog: Seaquake](https://bedroomproducersblog.com/2024/12/09/codwaves-seaquake-techno-rumble/)
- [SonicState: SUBTERRAIN Rumble Kick](https://sonicstate.com/news/2026/09/07/kick-generator-for-macos/)
- [SonicState: KFXT-41](https://sonicstate.com/news/2023/03/29/-fast-and-easy-way-to-create-techno-kick-drums/)
- [MusicRadar: 5 of the best kick plugins](https://musicradar.com/news/tech/5-of-the-best-vst-au-kick-drum-plugins-639062)
- [Phykick](https://imaiai.gumroad.com/l/phykick)
