# grarts2d: audyt stanu gry i plan rozwoju (20.11.2026)

## 1. Najważniejsze w skrócie
- **Mamy bardzo dobry świat, ale jeszcze nie mamy gry.** Jest piękny teren Europy z prawdziwych danych, mapa grywalna z drzewami i rzekami oraz papierowa mapa strategiczna. Nie ma ani jednej mechaniki rozgrywki: jednostek, budynków, surowców, wroga, interfejsu, zapisu gry, dźwięku ani menu.
- **Kod jest w dobrym stanie:** kompiluje się (20.11, 17:30), 2 testy automatyczne przechodzą, kod jest uporządkowany i opisany w `Docs/PROJEKT.md`.
- **Największe ryzyko to brak kopii i historii zmian.** Folder `D:\GRY\grarts2d` nie ma gita, a repozytorium na GitHubie (`Judaszver0.1`) zawiera tylko jeden plik `CLAUDE.md`. Awaria dysku albo zła zmiana AI = utrata tygodni pracy.
- **Drugie ryzyko to zakres.** „Cała Europa + budowanie od osady do zamku + duże bitwy” to zakres dużego studia. Trzeba go zawęzić do jednej rdzeniowej pętli i dowieźć ją jako grywalny wycinek (vertical slice), zanim dołożymy resztę.
- **Najmocniejsze, co już mamy na marketing:** prawdziwa Europa („zacznij w swojej okolicy”), „dzika Europa” do karczowania i mapa w stylu pergaminu z płynnym zoomem. To są gotowe wyróżniki, wokół których warto zbudować mechaniki.

---

## 2. Audyt: co jest w projekcie

### Technologia
| Element | Stan |
|---|---|
| Silnik | Unreal Engine 5.8, build ze źródeł (`D:/UE_5.8`), C++ |
| Kod gry | ~1 400 linii C++ w 6 klasach + ~1 700 linii skryptów Python (dane, tekstury, modele) |
| Kompilacja | OK (`Kompilacja.log`, 20.11.2026 17:30, „Result: Succeeded”) |
| Testy | 2 testy automatyczne (`grarts2d.Swiat.Dane`, `grarts2d.Swiat.MapaGrywalna`), oba przechodzą (`Saved/Testy.log`, 20.11 17:31) |
| Dokumentacja | Bardzo dobra: `Docs/PROJEKT.md` (dziennik decyzji, opis plików), `Docs/Assety_zrodla.md` (licencje) |
| Kontrola wersji | **Brak** (żadnego `.git` w folderze) |
| Rozmiar | Content 72 MB, Data 260 MB, Art 67 MB; surowe dane 2,1 GB i Intermediate 2,7 GB (nie trafiają do gry) |

### Co działa (systemy, które istnieją)
1. **Świat Europy 1:8** (`FSwiat`, `SwiatWysokosci.cpp`): prawdziwa rzeźba (ETOPO), pokrycie terenu (Copernicus) przerobione na „dziką Europę”, morza, jeziora, 326 rzek z linii. Sprawdzone testami (Kraków, Śnieżka, Bałtyk, Śniardwy).
2. **Teren z kafli quadtree** (`TerenKafle.cpp`): liczony w tle na wielu rdzeniach, bez dziur przy zoomie, ekran ładowania.
3. **Mapa grywalna (region)**: pierścień 11×11 kafli po 512 m, siatka co 4 m, drzewa (6 gatunków × 2 warianty, Nanite), kępki trawy, kamienie, rzeki z brzegiem i błotem, przesadzona i wygładzona rzeźba. Płynna jazda kamerą bez ekranu ładowania.
4. **Mapa strategiczna w stylu pergaminu**: papier, lasy jako wzorek drzewek, rzeki jako wstęgi (`AMapaRzeki`). Użytkownik ją zaakceptował.
5. **Kamera** (`KameraMapy.cpp`): zoom od 25 m do 600 km, obrót Q/E, WASD, przeciąganie, efekt pikseli na bliskim zoomie.
6. **Narzędzia**: skrypty do przeliczenia danych, generowania tekstur i drzew, budowania materiałów, zrzut kontrolny (`-zrzut`).

### Czego brakuje (cała rozgrywka)
- Jednostki (osadnicy, wojsko), zaznaczanie, rozkazy, poruszanie się, **szukanie drogi (pathfinding)**.
- Budynki, stawianie budynków, drogi, pola, karczowanie lasu.
- Surowce i ekonomia (drewno, jedzenie, kamień, ludność).
- Walka, morale, formacje.
- Przeciwnik (AI wroga albo zagrożenia jak bandyci, zwierzęta, zima).
- Interfejs gry (HUD, paski surowców, panele budynków), menu główne, ustawienia.
- Zapis i wczytanie gry.
- Dźwięk i muzyka.
- Prowincje i granice (zaplanowane jako etap 5, nie zaczęte).
- Kolizje terenu (kafle ich nie mają; dla jednostek wystarczy funkcja `FSwiat::Wysokosc`, więc to nie blokuje).
- Wersja „shipping” gry (do Steama) nie była jeszcze budowana ani testowana.

### Co jest zepsute albo niepewne
- **Brak historii zmian i kopii zapasowej** (krytyczne, opis wyżej).
- **Dużo zmian „w grze nieoglądane”**: w dzienniku wiele ostatnich poprawek ma dopisek, że nikt ich nie widział w grze. Kod się kompiluje, ale wygląd jest niepotwierdzony.
- **Wydajność nieznana:** cel to 60 FPS na i7-14700KF + RTX 4060, ale nikt tego nie zmierzył (setki tysięcy instancji drzew, cienie wirtualne, 121 kafli z siatką co 4 m).
- **Stare awarie (17.11)** w `Saved/Crashes`: dwie w `TerenKafle.cpp:80` (zapis poza tablicą przy budowie kafla, już poprawione według dziennika) i dwie w edytorze materiałów (błąd silnika przy przebudowie materiału skryptem). Od tamtej pory nowych awarii nie ma.
- **Znane skróty z dziennika:** zoom do kursora rozjeżdża się w górach, poziom wody skacze przy ujściu dopływu, małe jeziora są kanciaste, jeziora w regionie nie są spłaszczone, przy szybkiej jeździe kamery mogą pojawić się dziury na krawędzi kadru.
- **Nazwa i opis vs rzeczywistość:** folder i projekt nazywają się „2D pixel”, a gra jest pełnym 3D w UE5 z efektem pikseli. To dobry wybór techniczny, ale na stronie Steam trzeba to opisać spójnie („3D o wyglądzie pikselowej mapy”).
- **Licencje:** Copernicus jest na CC-BY 4.0, więc w napisach końcowych i na stronie gry musi być atrybucja (jest już zapisana w `Assety_zrodla.md`).

### Ocena jakości kodu
Dobra jak na prototyp. Klasy są małe i mają jedną odpowiedzialność, nazwy i komentarze po polsku są spójne, liczenie w tle jest bezpieczne wątkowo (`std::atomic`, `TFuture`), są testy. Do poprawy przy rozwoju: `TerenKafle.cpp` (648 linii) i `SwiatWysokosci.cpp` (546 linii) zaczynają być duże. Rozgrywkę (jednostki, budynki) trzeba będzie od początku pisać jako osobne systemy, a nie dokładać do terenu.

---

## 3. Gdzie jest rynek: co ludzie dziś chcą w strategiach

### Co się sprzedaje
- **Manor Lords** (średniowieczne budowanie + bitwy, jeden twórca) sprzedał ponad 3 mln kopii w 2024 r. i dalej ciągnie sprzedaż wydawcy. To najbliższy wzorzec: budowa osady organicznie na terenie + bitwy w formacjach. Gracze krytykowali głównie płytkość późnej gry i ślimacze aktualizacje, a chwalili wygląd, organiczne działki i klimat.
- 2025 był najmocniejszym rokiem dla RTS od lat (Tempest Rising, Dune: Awakening w trybie RTS-owym, kolejne nowe tytuły), ale bez jednego wielkiego hitu.
- **Stormgate** (RTS nastawiony na e-sport i szybkie klikanie) okazał się porażką. Wniosek: rynek indie nie chce kolejnej gry na APM i rankingi, chce strategii dla jednego gracza z głębią i klimatem.
- Popularne są hybrydy: budowanie + przetrwanie (Against the Storm, Diplomacy is Not an Option, Kingdoms and Castles, Going Medieval, Foundation, Bellwright).

### Mechaniki, o które gracze RTS i city builderów najczęściej proszą
1. **Organiczne budowanie** zamiast siatki (działki, drogi gdzie chcesz) i ładne miasto, które „rośnie samo”.
2. **Bitwy bez szybkiego klikania:** formacje, morale, teren ma znaczenie, aktywna pauza lub spowolnienie.
3. **Logistyka i zaopatrzenie** armii, pory roku, zima jako zagrożenie.
4. **Postacie z imieniem** (dowódcy, osadnicy), małe historie jak w RimWorld.
5. **Sensowna późna gra:** cel po zbudowaniu miasta (rozrost na region, dyplomacja, zagrożenia rosnące w czasie).
6. **Dobra AI przeciwnika** z osobowością, nie oszukująca.
7. **Kooperacja** z kumplem przeciw AI (częsta prośba w Manor Lords). Kosztowna w zrobieniu, więc na później.
8. **Mody i własne mapy** (wydłużają życie gry).

### Wyjątkowe mechaniki, które pasują właśnie do naszej gry
Te pomysły wynikają z tego, co już mamy (prawdziwa Europa, dzika puszcza, mapa-pergamin, zoom). Warto wybrać 2–3, a nie wszystkie.

| # | Pomysł | Dlaczego wyjątkowy | Koszt |
|---|---|---|---|
| A | **„Zacznij w swojej okolicy”**: gracz wybiera dowolne miejsce w Europie (np. swoje miasto) i tam zakłada osadę na prawdziwym terenie | Ogromny haczyk marketingowy (każdy chce zobaczyć swoją okolicę, filmiki „zbudowałem zamek tam, gdzie mieszkam”). Teren już to umie | Mały, bo świat już istnieje |
| B | **Dzika Europa do karczowania**: pola trzeba wydrzeć puszczy; las odrasta, daje drewno, zwierzynę i schronienie bandytom; wycięcie zboczy powoduje osuwiska i powodzie | Ekonomia oparta na terenie, którego nikt inny nie ma. Wynika z decyzji o „dzikiej Europie” | Średni |
| C | **Mapa, którą rysujesz**: mapa strategiczna jest pusta, a zwiadowcy i kartografowie dorysowują ją na pergaminie (mgła wojny jako niezbadany papier) | Zamienia nasz styl mapy w mechanikę. Bardzo dobrze wygląda w zwiastunie | Średni |
| D | **Teren decyduje w bitwie**: wzgórza, brody, mosty, mokradła i lasy na zasadzki z prawdziwej rzeźby; rzeki jako granice i szlaki | Prawdziwa geografia = prawdziwe pola bitew (np. obrona przeprawy przez Wisłę) | Średni do dużego |
| E | **Zaopatrzenie i pory roku**: armia je zapasy, zimą rzeki zamarzają (można je przejść), wiosną wylewają | Głębia bez szybkiego klikania | Średni |
| F | **Rozrost od osady do warowni** w organicznym stylu, z budynkami z muru pruskiego i strzechy jak na zrzutach wzorcowych | To główny cel gry z `PROJEKT.md` | Duży (assety) |
| G | **Ludzie z imieniem**: rodziny osadników, dowódcy z cechami i historią | Emocje i historie do udostępniania | Średni |

**Rekomendacja:** rdzeń gry to F + B (budowanie i karczowanie), haczyk marketingowy to A, a C jako wyróżnik wyglądu. Bitwy (D) w prostej wersji w wycinku, rozbudowane później.

---

## 4. Plan rozwoju

Założenie do czasu: pracujesz z AI ok. 15–20 godzin tygodniowo. Szacunki są zgrubne i raczej optymistyczne (w grach realny czas zwykle wychodzi ×1,5–2). AI pisze kod i robi assety, Ty oceniasz zrzuty i grasz.

### Etap 0: zabezpieczenie (1 tydzień), zrobić od razu
- Git + Git LFS w folderze gry i wypchnięcie na GitHub (`Judaszver0.1` albo nowe prywatne repo). Ignorowane: `Intermediate`, `Saved`, `DerivedDataCache`, `Binaries`, `Dane_zrodlowe`.
- Pomiar FPS na mapie grywalnej (`stat fps`, `stat unit`) jako punkt odniesienia.
- Obejrzenie w grze wszystkich zmian „nieoglądanych” i krótka lista poprawek.
- **Decyzja o rdzeniu gry** (patrz pytania na końcu).

### Etap 1: rdzeń rozgrywki, prototyp (6–8 tygodni)
Cel: w jednym regionie da się zagrać 20 minut i mieć z tego frajdę, nawet na brzydkich klockach.
- Osadnicy jako jednostki: zaznaczanie, rozkazy, ruch po terenie, pathfinding na siatce (omijanie wody, stromizn i gęstego lasu).
- 3 surowce: drewno, jedzenie, kamień. Ścinanie drzew (usuwanie instancji z kafla), karczowanie pod pole.
- 5–6 budynków (chata, drwal, pole, magazyn, kamieniołom, palisada), stawianie z podglądem na terenie.
- Prosty HUD: surowce, ludność, panel budynku.
- Jedno zagrożenie: bandyci z lasu, żeby był powód do obrony.
- Zapis i wczytanie gry.
- **Test:** 5–10 osób gra 30 minut, mierzymy, czy chcą grać dalej.

### Etap 2: grywalny wycinek, vertical slice (8–10 tygodni)
Cel: kawałek gry, który wygląda jak gotowy produkt, do zwiastuna i strony Steam.
- Modele budynków w stylu zrzutów wzorcowych (mur pruski, strzecha, gont), drogi, płoty, pola z dwoma wyglądami.
- Rozrost osada → wioska (poziomy budynków, potrzeby mieszkańców).
- Proste bitwy: 2–3 rodzaje wojsk, oddziały w formacji, morale, przewaga wzgórza.
- Wybór miejsca startu na mapie-pergaminie (pomysł A).
- Dźwięk i muzyka (darmowe lub zamówione źródła, wpisane do `Assety_zrodla.md`), menu główne.
- Wydajność: 60 FPS na docelowym sprzęcie.
- **Strona Steam** (patrz rozdział 5): pod koniec etapu, ok. 4–5 miesięcy od dziś.

### Etap 3: głębia i demo (3–4 miesiące)
- Pory roku i zaopatrzenie (E), las odrasta i reaguje na wycinkę (B).
- Prowincje: granice na mapie, przejmowanie sąsiednich obszarów, AI sąsiada (lord z osobowością).
- Zamek/warownia jako cel wycinka, oblężenie w prostej formie.
- Zwiadowcy dorysowują mapę (C).
- **Demo na Steam Next Fest** (najbliższy realny termin: czerwiec albo październik 2027).

### Etap 4: wczesny dostęp, Early Access (3–4 miesiące)
- 1 pełna kampania lub tryb piaskownicy z celem, samouczek, ustawienia, lokalizacja (PL + EN, potem DE i inne).
- Budowa i test wersji shipping, Steamworks (osiągnięcia, chmura zapisu).
- **Premiera EA: ok. 14–18 miesięcy od dziś**, jeśli liczba wishlist na to pozwala.

### Później (po EA)
Większe armie, oblężenia, dyplomacja, kooperacja, mody, kolejne regiony Europy z charakterem (góry, mokradła, wybrzeża).

---

## 5. Jak sprawdzamy, co jest potrzebne (walidacja)

### Testy z graczami (playtesty)
- **Od etapu 1, co 2–3 tygodnie.** Na początku znajomi i Discord, potem Steam Playtest (darmowy, osobna aplikacja na stronie gry).
- Gdzie szukać testerów: r/RealTimeStrategy, r/CityBuilders, r/BaseBuildingGames, polskie grupy i serwery o strategiach, własny Discord.
- Co mierzymy: czy gracz sam z siebie gra dalej po 30 minutach, gdzie się gubi (nagranie ekranu + krótka ankieta 5 pytań), co chce mieć „następne”.
- Zasada: patrzymy, co ludzie robią, a nie tylko co mówią.

### Strona Steam i wishlisty
- Stronę „Coming Soon” zakładamy, gdy mamy 1 dobry zwiastun i 5 dobrych zrzutów (koniec etapu 2). Opłata Steam Direct to 100 USD za grę.
- Wishlisty to główny wskaźnik zainteresowania. Często podawane orientacyjne progi z branży (howtomarketagame.com): poniżej ~7 tys. w dniu premiery jest ryzykownie, ~10 tys.+ to punkt, od którego Steam zaczyna sam promować grę, a 30–50 tys.+ daje realne szanse na sukces.
- Co obserwujemy co tydzień: wishlisty na dzień, odsetek odwiedzających stronę, którzy dodają do wishlisty, oraz CTR miniaturki (capsule).
- Testy haczyków: zanim zbudujemy dużą mechanikę, wrzucamy krótki filmik/GIF z pomysłem (np. „zbudowałem osadę w swoim mieście”) na Reddit/TikTok/X i patrzymy, który wywołuje reakcje. To tanie i szybkie.
- Steam Next Fest: demo z dobrym pierwszym kwadransem, ale dopiero gdy strona ma już kilka tysięcy wishlist.

### Badanie konkurencji
- Lista 10–15 gier porównawczych: Manor Lords, Knights of Honor II, Bellwright, Foundation, Kingdoms and Castles, Going Medieval, Diplomacy is Not an Option, Against the Storm, Farthest Frontier, Stronghold, The Settlers.
- Dla każdej: tagi Steam, szacowana sprzedaż (SteamDB, Gamalytic/VG Insights), cena, data premiery, liczba recenzji.
- **Czytanie negatywnych recenzji konkurencji** (najwięcej mówią, czego brakuje na rynku). AI może zebrać i pogrupować tematy.
- Wynik: jedna strona „czym się różnimy” do opisu na Steam.

---

## 6. Decyzje do podjęcia przez Ciebie
1. **Rdzeń gry:** czy gra to przede wszystkim budowanie osady z bitwami w tle (jak Manor Lords), czy przede wszystkim bitwy i podbój z budowaniem w tle? Rekomenduję pierwsze, bo do tego pasuje już zrobiony teren.
2. **Haczyk:** czy idziemy w „zacznij w swojej okolicy” jako główne hasło gry?
3. **Git:** czy mogę założyć repozytorium w folderze gry i wypychać na GitHub (`Judaszver0.1`)?
4. **Tempo:** ile godzin tygodniowo realnie masz? Od tego zależą terminy z rozdziału 4.

---

## Źródła
- [Manor Lords: 3 mln sprzedanych kopii (PC Gamer)](https://pcgamer.com/games/sim/manor-lords-the-best-city-builder-of-2024-hits-3-million-sales-as-players-continue-to-fill-its-maps-with-muddy-medieval-towns)
- [Hooded Horse i renesans strategii (GamesRadar)](https://gamesradar.com/games/strategy/manor-lords-publisher-leads-a-strategy-game-renaissance-with-the-medieval-city-builder-racking-up-more-than-half-of-its-4-5-million-yearly-sales)
- [2025 najmocniejszym rokiem RTS od lat (Gamepressure)](https://www.gamepressure.com/newsroom/2025-is-the-strongest-year-for-rts-in-ages-but-the-big-breakout-h/zc8400)
- [Stormgate: porażka i recenzje (Windows Central)](https://www.windowscentral.com/gaming/pc-gaming/stormgate-dev-blames-flop-industry-issues-reviews-suggest-otherwise)
- [Pathfinding w Manor Lords i problemy strategii (Game Developer)](https://www.gamedeveloper.com/press-release/after-manor-lords-reworked-pathfinding-what-else-do-strategy-games-need-to-solve-)
- [Ile wishlist daje Steam Next Fest (howtomarketagame)](https://howtomarketagame.com/2025/03/26/benchmarks-how-many-wishlists-can-i-get-from-steam-next-fest/)
- [Next Fest październik 2025: gry, które się przebiły (howtomarketagame)](https://howtomarketagame.com/2025/10/20/steam-next-fest-october-2025-checking-in-on-the-games-that-broke-through/)
- [Konwersja wishlist na zakupy po Next Fest (Alinea Analytics)](https://alineaanalytics.substack.com/p/wishlist-to-buyer-conversions-for)
- [Wishlisty w liczbach (80.lv)](https://80.lv/articles/steam-wishlists-by-the-numbers-what-actually-gets-wishlisted)
- Gry porównawcze: [Foundation](https://store.steampowered.com/app/690830/Foundation/), [Going Medieval](https://en.wikipedia.org/wiki/Going_Medieval), [Kingdoms and Castles](https://en.wikipedia.org/wiki/Kingdoms_and_Castles), [Tiny Glade](https://en.wikipedia.org/wiki/Tiny_Glade)
