# grarts2d — dziennik projektu

## Cel gry
Strategia 3D, która wygląda jak 2D. Gracz zakłada osady, potem wioski, a na końcu zamki i warownie. Dochodzą do tego bitwy z dużymi armiami różnych rodzajów wojsk.

**Świat:** Europa w skali 1:8 (około 524 × 524 km) z prawdziwą rzeźbą terenu i charakterystycznymi miejscami.

**Zoom działa jak w Google Maps:**
- blisko widać 3D z drzewami,
- dalej pojawia się mapa terenu z tym, co się dzieje,
- najdalej widać cały świat.

**Podział na prowincje:** małe nieregularne obszary osadnicze łączą się w powiaty, a powiaty w krainy.

**Sprzęt docelowy:** i7-14700KF, 32 GB RAM, RTX 4060 8 GB.

## Wzorzec wyglądu (zrzuty od użytkownika, 17.11.2026)
- Widok prawie izometryczny pod kątem około 50°, wąska perspektywa.
- Lekki efekt pikseli: obraz renderowany w niższej rozdzielczości, bez rozmycia krawędzi.
- Trawa w stonowanych, oliwkowych zieleniach z plamami. Łąki są jaśniejsze i mają kwiatki.
- Pola uprawne mają dwa wyglądy: brązowe bruzdy albo złote zboże. Otaczają je żywopłoty i płotki.
- Drzewa: kępiaste, okrągłe korony (dąb, buk), brzozy z białym pniem, rzadziej iglaki. Cienie są miękkie.
- Woda jest płaska, szaroniebieska, z jasnym brzegiem i kamieniami w strumieniach.
- Drogi: brązowe ścieżki gruntowe oraz szare drogi brukowane i żwirowe.
- Budynki z muru pruskiego (białe ściany, ciemne belki) oraz drewniane chaty. Dachy kryte strzechą (jasnobrązowe) albo gontem (szare).
- Granice prowincji to cienkie jasne linie na terenie.

## Decyzje użytkownika (17.11.2026, po pierwszym oglądaniu)
Poziomy zoomu, od najbliższego do najdalszego (wzorce to zrzuty od użytkownika):
1. **Budowa** (do ok. 1,5 km): 3D z lekkim efektem pikseli, jak zrzuty z wioskami i polami.
2. **Teren z góry** (ok. 1,5–10 km): gładki, bez pikseli. Realistyczny teren z cieniami, gęste lasy z pojedynczych drzew, kręte rzeki i strumienie, łąki w różnych kolorach (żółte, wrzosowe), skałki i niskie chmury.
3. **Poziomice** (ok. 10–40 km): kolory wysokości, linie poziomic, lasy i rzeki.
4. **Malowana mapa** (od ok. 40 km do całej Europy): cieniowane góry, lasy jako kępki rysowanych drzewek, niebieskie strumienie i morze z ciemniejszą krawędzią przy brzegu. Później dojdą granice, drogi i nazwy na wstęgach.

Pozostałe decyzje:
- Na razie nie ma osad, granic ani nazw. Teraz robimy tylko przyrodę: teren, lasy, rzeki i morza.
- Gra nie trzyma się konkretnej epoki: klimat średniowieczny, bez historycznej wierności.
- Lasy, łąki i inne pokrycie terenu biorą się z prawdziwych danych, uzupełnionych losowym detalem. Źródło: Copernicus Land Cover 100 m z 2018 r. (CC-BY 4.0), z podziałem na lasy iglaste, liściaste i mieszane, krzewy, łąki, mokradła, tereny bez roślin i śnieg.
- **Dzika Europa:** pola uprawne i miasta z danych zamieniamy na puszczę z polanami i łąkami, a gracz sam karczuje teren pod pola. Prawdziwe lasy, góry i mokradła zostają tak, jak są.
  - Polany to ok. 30% puszczy, a skraj lasu (las rzadki) ok. 16%.
  - Wyspy Brytyjskie zalesiamy tak samo jak resztę. Tamtejsze dzisiejsze pastwiska położone wyżej niż 350 m zamieniamy na wrzosowiska.
- Kolejność prac: teren i woda, potem drzewa, na końcu mapy (poziomice i malowana mapa).

## Decyzja użytkownika: trzy mapy (17.11.2026, wieczorem)
Ciągły zoom z doczytywaniem był „ani grywalny, ani ładny”. Nowy podział:
1. **Mapa Europy** w pełnym oddaleniu: zostaje bez zmian, użytkownik uznał ją za dobrą.
2. **Mapa pośrednia** (do ok. 35% przybliżenia): ta sama mapa kafli, trochę więcej szczegółów, bez drzew.
3. **Mapa grywalna (region):**
   - Wejście: przybliżenie poniżej `ProgRegionu` (2,5 km) → ekran ładowania → region **4 × 4 km** w grze (8×8 kafli 512 m z siatką co 4 m i drzewami), liczony raz. W trakcie gry nic się nie doczytuje.
   - Wyjście: oddalenie na granicy zoomu regionu wraca do mapy Europy (znowu ekran ładowania).
   - Wygląd ma być **identyczny ze zrzutem wzorcowym** przysłanym przez użytkownika: wieś, pola, pastwiska, rozrzucone drzewa i kępy, równa ziarnista trawa, jezioro, przerywana granica prowincji, mgła.
   - Kamera: stały kąt 45°, pole widzenia 20° (prawie bez perspektywy). Zoom od ~90 m do ~420 m szerokości widoku. Obrót o 90° klawiszami Q i E.

## Jak uruchomić
1. Zamknij Unreal Editor i uruchom `Kompiluj.bat` (zrobi to też AI).
2. Otwórz `grarts2d.uproject` i naciśnij Play albo uruchom grę z opcją `-game`.
3. Sterowanie:
   - kółko myszy przybliża i oddala do punktu pod kursorem,
   - przeciąganie lewym albo środkowym przyciskiem przesuwa mapę,
   - WASD lub strzałki przesuwają widok, Shift przyspiesza,
   - Q i E obracają widok.

## Pliki i za co odpowiadają
| Plik | Co robi |
|---|---|
| `grarts2d.uproject` | Projekt UE 5.8 z modułem C++ `grarts2d` i wtyczką ProceduralMeshComponent |
| `Kompiluj.bat` | Kompilacja edytora, log w `Kompilacja.log`; argument `nopause` uruchamia bez pauzy |
| `Config/DefaultEngine.ini` | Pusta mapa startowa `/Engine/Maps/Entry` i tryb gry `SwiatGameMode`. Wyłączone: Lumen, ray tracing i AA. Efekt pikseli: `r.ScreenPercentage=50` i `r.Upscale.Quality=0` |
| `Source/grarts2d/Swiat/SwiatWysokosci.*` | `FSwiat`: rozmiar świata, skala pionowa i funkcja `Wysokosc(x,y)` używana przez cały kod. Czyta `Data/europa_wys.r16` i `.txt`. Bez tych plików buduje teren testowy z szumu. Dodaje własne drobne pagórki |
| `Source/grarts2d/Swiat/TerenKafle.*` | `ATerenKafle`: teren z kafli quadtree (od kafla 524 km do kafli 512 m) liczonych w tle. Fartuchy na krawędziach zakrywają szpary. Na kaflach poziomu ≥8 (`MinPoziomDrzew`) sadzi drzewa i krzaki (`Sadz`): oczko 9 m (`KomorkaDrzew`), los z numeru oczka (drzewo stoi w tym samym miejscu na każdym poziomie), na grubszych poziomach zostaje 40% / 15% drzew, za to większych, a krzaki rosną tylko na najbliższym. Gatunek wynika z klasy pokrycia, a powyżej 1100 m rosną głównie świerki. Bez drzew: woda, plaża, zbocza ze spadkiem > 0,7. Drzewa to `UInstancedStaticMeshComponent`, po jednym na model i kafel |
| `Source/grarts2d/Swiat/SwiatGameMode.*` | Stawia słońce, atmosferę, sky light, korektę ekspozycji i teren.<br>**Ekran ładowania** (Slate, procent gotowych kafli) zasłania świat, dopóki wszystkie kafle dla widoku nie są gotowe, i potem jeszcze 0,5 s. Później w prawym dolnym rogu pokazuje się „Doczytywanie terenu… N%”, gdy coś się jeszcze dociąga (użytkownik prosił, żeby oceniać tylko gotowy obraz).<br>Opcja `-zrzut` robi zrzut 2 s po wczytaniu i wychodzi; jeśli wczytanie nie skończy się w 3 min, wychodzi bez zrzutu |
| `Tools/dem_europa.py` + `Tools/Przelicz_Europe.bat` | Przelicza `Dane_zrodlowe/` (ETOPO + Natural Earth) na `Data/`. Odwzorowanie: rownopolowe Lamberta o srodku 52°N 10°E, kwadrat 4194 km, 8192² pikseli (512 m prawdziwych = 64 m w grze). Lad wedlug Natural Earth (poldery nie sa zalewane), rzeki wciete o 4 m. Sprawdza Mont Blanc, Sniezke, Baltyk i Krakow. Rysuje `Docs/europa_podglad.png` |
| `Tools/_py/` | Biblioteki Pythona (numpy, tifffile, imagecodecs, pillow, pyshp) dla Pythona z UE 5.8 |
| `Dane_zrodlowe/` | Surowe pobrane dane (443 MB): 18 kafli ETOPO i 3 paczki Natural Earth. Do gry nie sa potrzebne, tylko do ponownego przeliczenia |
| `Data/europa_wys.r16` / `.txt` | Wysokosci Europy w uint16, wiersz 0 to polnoc. Plik txt: `szer wys min_m max_m startX startY`, gdzie start to Krakow w cm |
| `Tools/pokrycie_europa.py` | Czyta z pliku Copernicus LC100 tylko kafle obejmujące Europę i przelicza je do `Data/europa_pokrycie.r8`. Klasy terenu: 0 woda, 1 łąka, 2 wrzosowisko, 3 las liściasty, 4 las iglasty, 5 las mieszany, 6 las rzadki, 7 mokradło, 8 skały, 9 śnieg, 10 tundra. Realizuje „dziką Europę”: pola uprawne i miasta zamienia na puszczę z polanami. Rodzaj lasu zależy od szerokości geograficznej i wysokości. Rysuje `Docs/europa_pokrycie_podglad.png` |
| `Data/europa_pokrycie.r8` | Pokrycie terenu, układ jak `europa_wys.r16`. Gra czyta je przez `FSwiat::Pokrycie(x,y)` (granice falowane szumem ±64 m) i według niego sadzi drzewa |
| `Tools/natura_drzewa.py` | Tworzy modele drzew i krzaków jako `Art/Natura/*.glb` (w metrach) oraz teksturę `T_Liscie.png`: lewa połowa to kępa liści, prawa to gałązki z igłami.<br>Gatunki: dąb, buk, brzoza z białym pniem, sosna, świerk i krzak, każdy w 2 wariantach.<br>Korona składa się z kilkudziesięciu kęp, każda z kilku kart z teksturą liści. Normalne kart wychodzą od środka korony, więc korona świeci jak bryła. Wnętrze i spód korony są ciemne, góra jasna. Pień i gałęzie widać w prześwitach.<br>Materiały w modelu: „Kora” i „Liscie” |
| `Tools/ue_natura.py` + `Tools/Zbuduj_Nature.bat` | Bat tworzy od nowa pliki .glb, kasuje cały `Content/Swiat/Natura` i importuje modele jako `SM_<Gatunek>_<1/2>` z Nanite.<br>`M_Natura` (liście): maskowany i dwustronny; tekstura × kolor wierzchołków × losowa jasność na drzewo. Mipmapy trzymają pokrycie maski.<br>`M_Natura_Kora`: kolor wierzchołków.<br>Log: `Saved/Zbuduj_Nature.log` |
| `Tools/ue_sprawdz_material.py` | Kompiluje shadery `M_Teren` i `M_Natura` w commandlecie, bez okna gry; błędy w logu jako „Failed to compile” |
| `Art/Teren/T_Europa_Woda.png` | Pole odległości od brzegu (R, 0,5 = brzeg) i głębokość (G). Materiał rysuje z niego ostrą wodę, jasną linię brzegu i błotnisty pas |
| `Data/europa_woda.r8` | Maska wody: 0 lad, 1 morze, 2 jezioro, 3 rzeka. `FSwiat::Woda(x,y)` |
| `Data/teren_detal.rgb8` | Surowa kopia `T_Detal` (512×512 RGB) dla C++: z niej `FSwiat::Pokrycie` falowało granice klas tak samo jak materiał. Tworzy ją `tekstury_teren.py` |
| `Tools/tekstury_teren.py` | Tworzy `Art/Teren/T_Europa_Kolor.png` (8192², kolor całej Europy: pokrycie terenu z plamami, morze jaśniejsze przy brzegu, szarość skał wysoko w górach) i `T_Detal.png` (512², kafelkujący szum w kanałach: R kępki trawy, G łaty, B duże plamy). Podgląd w `Docs/europa_kolor_podglad.png` |
| `Tools/ue_teren.py` + `Tools/Zbuduj_Teren.bat` | Działają wewnątrz UE (commandlet, edytor zamknięty). Importują brakujące tekstury i budują od nowa `/Game/Swiat/Teren/M_Teren`. Żeby podmienić teksturę, usuń jej `.uasset`. Log trafia do `Saved/Zbuduj_Teren.log` |
| `Content/Swiat/Teren/M_Teren` | Materiał terenu:<br>• UV1 (cała mapa) daje kolor z T_Europa_Kolor; granice klas są falowane szumem 256 m.<br>• UV0 (1 = 4 m) daje detal: ziarno trawy 20 cm–1,3 m, łaty 1–8 m z odcieniem suchej i świeżej trawy, duże plamy.<br>• Skała pojawia się tylko na stromych zboczach (`normal.z < 0,55`).<br>• Woda pochodzi z T_Europa_Woda |
| `Source/grarts2d/Kamera/KameraMapy.*` | Kamera z zoomem od 25 m do 600 km. Z daleka prostuje się do pionu i poszerza pole widzenia. `StrefaMapy()` i `StrefaSwiata()` zwracają wartość 0..1 dla materiałów i UI. Opcja `-odl=cm` ustawia oddalenie na starcie |

**Układ współrzędnych:** +X to północ, +Y to wschód, środek świata leży w (0,0), jednostką jest cm.

**Skala wysokości:** prawdziwe metry × 12,5 × 2,5 (`FSwiat::MetryNaCm` = 31,25), czyli przesada pionowa ×2,5, żeby wzniesienia były widoczne.

## Etapy
1. ✅ **17.11.2026**
   - Szkielet projektu C++, kamera jak Google Maps i kafle quadtree na terenie testowym.
   - Projekt się kompiluje.
   - Pierwsze uruchomienie wysypało się na błędzie w fartuchach kafli. Błąd jest poprawiony, ale wygląd po poprawce nie był jeszcze oglądany.
2. ✅ **17.11.2026** Dane Europy:
   - pobrane: ETOPO 2022 15″ (18 kafli od 30°W do 60°E i od 30°N do 75°N) oraz Natural Earth 10m (ląd, jeziora, rzeki),
   - przeliczone do `Data/`; kontrola: Mont Blanc 4606 m, Śnieżka 1513 m, Kraków 213 m,
   - kamera startuje nad Krakowem, a jeziora i rzeki mają kolor wody. Wyglądu w grze nikt jeszcze nie oglądał.
   - Kadr obejmuje Islandię, Nordkapp, Moskwę, Afrykę Północną i Anatolię (`Docs/europa_podglad.png`).
3. ⏳ **17.11.2026** Materiał terenu jest zrobiony:
   - pokrycie Europy (Copernicus, dzika Europa), detal trawy, skała na zboczach, oświetlenie od słońca,
   - piksele tylko do 1,5 km, mgiełka powietrza 0,05.

   Gry z nowym materiałem nikt jeszcze nie oglądał. Pierwsza wersja nie skompilowała się (zły typ próbkowania detalu), a po poprawce nie było zrzutu kontrolnego.

   Pozostało: woda jako osobna powierzchnia i małe strumienie.

   Pierwotny zakres etapu: własny materiał terenu z teksturami (trawa, łąka, ziemia, pole, skała, piasek, śnieg), woda i dopracowanie efektu pikseli.
   Uzupełnienie: woda z ostrym brzegiem (T_Europa_Woda), jasna linia i błotnisty pas przy brzegu, ziarnista trawa w skali 0,5 m. Shadery się kompilują. W grze jeszcze nieoglądane.
4. ⏳ **17.11.2026** Drzewa i krzaki:
   - modele z Pythona (nie z Blendera), 12 sztuk z Nanite,
   - sadzone na kaflach według Copernicusa; kod się kompiluje.

   W grze jeszcze nieoglądane. Brakuje skał jako modeli. Do sprawdzenia: wydajność (setki tysięcy instancji, cienie) oraz to, czy z bliska widać przeskok między poziomami kafli.

   **Uwagi użytkownika do pierwszej wersji:** ani drzewa, ani teren nie przypominały zrzutów.
   - Korony wyglądały jak gładkie balony, kolory były jasne i wyblakłe, cienie czarne, trawa płaska, a na łagodnych zboczach pojawiały się beżowe pasy skały.
   - Użytkownik wybrał, żebym zrobił drzewa sam, bez Fab. Fab zostaje zapasem, jeśli te się nie spodobają.

   **Poprawki:**
   - korony z kart z teksturą liści;
   - stała korekta ekspozycji −1 EV (`APostProcessVolume` w `SwiatGameMode`);
   - sky light włączany przez `SetRealTimeCapture` (wcześniej cienie były czarne);
   - ziarno trawy przeniesione do skali 20 cm–1,3 m, wcześniej ginęło w mipmapach;
   - łaty odcienia trawy;
   - skała dopiero od `normal.z < 0,55`, a drzewa rosną do spadku 1,3.

   Shadery się kompilują. W grze jeszcze nieoglądane.

   **Druga ocena użytkownika:** drzewa mu się podobają. Problemem było doczytywanie: oglądał świat w połowie wczytany.
   - Dodany ekran ładowania i napis o doczytywaniu.
   - Do pierwszego wczytania kafle liczą się na wszystkich rdzeniach.
   - `ATerenKafle::Postep()` i `Gotowe()` mówią, ile kafli jest gotowych.

   Na zrzucie z tej oceny widać też, że z bliska kolor terenu tworzy kwadraty 64 m (fioletowe wrzosowiska, zielone łąki), bo tekstura Europy ma 64 m na piksel. Do zrobienia: proceduralne granice klas z bliska.

   **Poprawka granic:**
   - Falowanie granic klas w `M_Teren` działa teraz osobno w U i V. Wcześniej działało tylko po przekątnej, przez co kwadraty zostawały.
   - Przesunięcie jest silniejsze: ±2 piksele mapy na dużych plamach i ±0,5 piksela na drobnych.
   - `FSwiat::Pokrycie` liczy dokładnie ten sam wzór z `Data/teren_detal.rgb8` (kopia T_Detal zapisywana przez `tekstury_teren.py`), więc drzewa rosną tam, gdzie teren ma kolor lasu.
   - Klasa 2 („wrzos”) to w Copernicusie wszystkie zarośla, także śródziemnomorska makia. Ma teraz stonowany oliwkowo-brązowy kolor z lekkimi łatami wrzosu zamiast fioletu.
5. Prowincje na 3 poziomach z granicami w materiale i nazwami na mapie. Pełne przejścia między strefami zoomu (Material Parameter Collection).
6. Wydajność, cel to 60 FPS na bliskim zoomie.

Później, w osobnych planach: domy i wioski, drogi, pola, zamki i bitwy.

## Otwarte decyzje i skróty
- Kafle mają na razie 64 kwadraty na bok, co daje oczko 8 m na najbliższym poziomie. Plan zakłada 4 m: jeśli z bliska będzie za mało szczegółów, ustawić `Kwady=128`.
- `KameraMapy::PunktPodKursorem` szuka punktu na płaszczyźnie wysokości ogniska, a nie na terenie. W górach zoom do kursora może się trochę rozjeżdżać.
- Kafle nie mają kolizji. Kamera bierze wysokość z `FSwiat::Wysokosc`, a kolizje dojdą razem z budynkami i jednostkami.
- Efekt pikseli przełącza się w `KameraMapy::UstawPiksele`. Do 1,5 km obraz jest renderowany w 50% bez wygładzania, dalej w 100% z TSR. W edytorze (PIE) ustawienie zostaje po wyjściu z gry.
- `Kafle` nie mają już kolorów wierzchołków: cały wygląd pochodzi z `M_Teren`.
- Kolejność wierzchołków trójkątów w `TerenKafle.cpp` sprawdzono w grze: `{A, C, B, A, E, C}`. Odwrotna kolejność sprawiała, że z góry widać było niebo przez teren.
- Rzeki z Natural Earth to tylko duże rzeki o szerokości 1–2 pikseli (64–128 m w grze). Małe strumienie dojdą później, liczone ze spływu wody po DEM.
- Woda nie jest jeszcze osobną powierzchnią: na razie to tylko kolor terenu, a morze to teren spłaszczony do 0.

## Mapa grywalna (region) - jak działa (17.11.2026)
- `ATerenKafle::WejdzDoRegionu(punkt)`: kasuje wszystkie kafle (`Wyczysc`), wyznacza 8×8 kafli poziomu 10 wokół punktu (`RegionKafli`) i liczy je z `KwadyRegionu=128` (co 4 m) oraz drzewami w skali `SkalaDrzewRegionu=0,55` (korony ~6 m jak na zrzucie). `WyjdzZRegionu()` wraca do kafli według kamery.
- Na mapie Europy nie ma już drzew (`MinPoziomDrzew=99`), żeby mapa doczytywała się szybko.
- Kafle regionu mają `M_Teren` z parametrem `Region=1`: w 80% równa trawa (0,072 / 0,15 / 0,04 liniowo) z ziarnem i łatami; skała na stromiznach i woda zostają.
- `AKameraMapy`: tryb `bRegion` – kąt `NachylenieRegionu=45`, `FovRegionu=20`, zoom `MinOdlRegionu..MaxOdlRegionu` (25 000–120 000 cm), ognisko trzyma się `MarginesRegionu` od krawędzi, Q/E obracają o 90°, piksele zawsze włączone. Przejścia woła `ASwiatGameMode::WejdzDoRegionu / WyjdzZRegionu` (ekran ładowania z napisem „Wczytywanie regionu… / mapy…”).
- Opcja `-region` startuje od razu w regionie wokół Krakowa (do zrzutów kontrolnych: `-zrzut -region`).
- **Jeszcze nie zrobione w stosunku do zrzutu wzorcowego:** wieś i budynki, pola, płoty, drogi, zwierzęta, granica prowincji (przerywana linia), mgła i chmury, jezioro z liliami, drobne krzaki przy płotach. Kolory i jasność ustawione bez porównania w grze.

## Ocena regionu przez użytkownika (17.11.2026) i poprawki
Użytkownik: region nie przypomina wzoru, brak rzeźby terenu. Mapy Europy i pośredniej mają zostać sztywne (**nie ruszać ich**), a cała praca ma iść w mapę grywalną, która ma wyglądać jak zrzut wzorcowy.
Poprawki tylko w regionie:
- `FSwiat::UstawRegion`: wewnątrz regionu prawdziwe różnice wysokości ×3 (`PrzesadaRegionu`) liczone od najniższego lądu regionu, do tego łagodne pagórki (~800 m i ~200 m). Poza regionem `Wysokosc` jest bez zmian.
- Kamera regionu: kąt 38° (było 45°, wyglądało prawie jak z góry).
- `ASwiatGameMode::UstawSwiatloRegionu`: w regionie słońce nisko (30°) z lewej góry ekranu, długie cienie w prawo w dół; korekta obrazu (ekspozycja −0,7, nasycenie 0,82). Po wyjściu z regionu wszystko wraca do ustawień mapy Europy (korekta wyłączona, poprzednia −1 EV na mapie Europy cofnięta).
- Trawa regionu stonowana i szarozielona (0,06 / 0,088 / 0,058). Liście ×2–2,8 jaśniejsze (w grze były prawie czarne), kora ×1,2–1,6.

## Małe poprawki: korony drzew (17.11.2026)
Użytkownik woli małe, precyzyjne kroki. Na zrzucie regionu: korony liściaste były poszarpane, z dziurami i cytrynowymi plamkami, iglaki wyglądały jak suche patyki, a cienie kończyły się w połowie ekranu.
- `T_Liscie`: pod liśćmi i igłami jest nieprzezroczysty krążek (pełne kępy bez dziur), liści jest 900 i są większe, różnice jasności mniejsze, igły grube (4× szersze) i rozłożone równo na całej kępie.
- Większe karty liści (dąb 2,8, buk 2,6, brzoza 1,9 i 32 kępy), świerk ma 3 karty na kępę, łagodniejsze zaciemnienie wnętrza (AO 0,45–1,0).
- Cienie: `r.Shadow.Virtual.MaxPhysicalPages=8192`, `ResolutionLodBiasDirectional=1.0`, komunikat o braku stron cieni włączony; zasięg cieni słońca bez cieni wirtualnych 3 km.

## Teren, woda i drobiazgi regionu według 3 zrzutów wzorcowych (17.11.2026)
Wzór: gęsty las, łąka z łatami trawy, suchej trawy, wrzosu i żółtych kwiatków, granatowa rzeka z błyskami i błotnistym, kamienistym brzegiem.
Zmiany dotyczą tylko mapy grywalnej (`Region=1`). Mapa Europy jest bez zmian.
- `Tools/tekstury_teren.py trawa` → `Art/Teren/T_Trawa.png` (512², kafel 64 m). R i G to dwie warstwy krótkich źdźbeł (2–4 m, skierowane na północ, czyli w górę ekranu). B to kropki ~1 m (kwiatki, kamyczki, błyski wody).
- `M_Teren` (region):
  - ziemia to świeża trawa, sucha trawa (łaty z T_Detal), ciemniejsze runo tam, gdzie mapa ma las, i wrzos tam, gdzie mapa ma zarośla;
  - progi przejść są przesuwane szumem źdźbeł, więc granice są miękkie i poszarpane;
  - na wierzchu jasność źdźbeł, żółte kwiatki na części łąk i skała na stromiznach;
  - woda: granat (płytko 0,045/0,141/0,223, głęboko 0,021/0,076/0,162) i jasne błyski przesuwane Pannerem;
  - brzeg: brązowe błoto ze źdźbłami i jasnymi kamyczkami.
- `natura_drzewa.py`:
  - `SM_Kamien_1/2` (spłaszczona zaszumiona kula 0,4–0,9 m);
  - `SM_Trawa_1/2` (kępka 18 źdźbeł ~0,8 m, obie strony);
  - pnie (`walec`) mają teraz ściany przeciwnie do zegara jak w glTF (wcześniej były odwrócone).
- `TerenKafle.cpp`:
  - `SadzDrobne` działa tylko w regionie, z oczkiem co 4 m;
  - kępki trawy: szansa 22% na łące, ~5% w lesie, bez cieni (oszczędność pamięci VSM);
  - kamyki: 25% w pasie 12 m od wody, 8% na zboczach, 0,8% gdzie indziej;
  - małe krzaczki (skala 0,3–0,6): 1–3%;
  - modele mają indeksy 12–15 (`EGatunek::Kamien`, `Trawa`);
  - wspólne funkcje `WysNaKaflu` i `FLos`.
- Nie sprawdzono w grze: kolory, gęstość kępek i skala tekstury źdźbeł są ustawione na oko. Do strojenia po zrzucie użytkownika.

## Mapa grywalna bez granic: płynna jazda kamerą (17.11.2026)
Użytkownik: teren i drzewa są takie, jak trzeba, ale region był zamknięty w kwadracie 4 × 4 km. Ma się dać jechać kamerą dalej, bez ekranu ładowania. Mapy oddalone służą tylko do podglądu i zostają bez zmian.
- `ATerenKafle` w trybie `bRegion`:
  - zamiast stałych 8×8 kafli jest pierścień (2·`PromienRegionu`+1)² = 7×7 kafli 512 m wokół ogniska kamery (pionka);
  - przy największym oddaleniu widać ~0,5 km, a zapas to ~1,5 km, więc kafle przed kamerą liczą się w tle, zanim wjadą w kadr;
  - na mapie grywalnej liczą wszystkie rdzenie;
  - kafel znika dopiero, gdy jest o 1 kafel dalej niż pierścień (histereza);
  - na ekran trafia najwyżej `KafliNaKlatke=2` gotowe kafle na klatkę (bez przycięć);
  - `WejdzDoRegionu()` nie zwraca już obszaru; `WRegionie()` mówi, czy jesteśmy na mapie grywalnej.
- `FSwiat::UstawRegion(bool)`:
  - przesada ×3 liczy się od gładkiej „podłogi” zamiast od minimum jednego kwadratu;
  - podłoga to minimum lądu ±7 km, wygładzone (3 × średnia ±2 km) na siatce 512² (1 km);
  - jest ciągła i zawsze ≤ terenu, więc nie ma uskoków między kaflami i nic nie schodzi pod morze;
  - liczy się raz przy pierwszym wejściu.
- `AKameraMapy::UstawRegion(bool, Srodek)`: ognisko nie jest już ograniczone do kwadratu, tylko 2 km od krawędzi świata (`MarginesRegionu`).
- `ASwiatGameMode`: na mapie grywalnej nie pokazuje napisu „Doczytywanie terenu”.
- Ryzyko: przy bardzo szybkim przesuwie (Shift, duże oddalenie) liczenie może nie nadążyć i na skraju kadru pojawi się dziura. Wtedy trzeba zwiększyć `PromienRegionu` albo zmniejszyć prędkość kamery w regionie.

## Rzeki z linii na mapie grywalnej (17.11.2026)
Problem (zrzuty użytkownika): rzeka z pikseli 64 m szła schodkami, a pas błota wypełniał całe piksele, więc wychodziły brązowe kwadraty i prostokąty. Mapy oddalone służą tylko do podglądu i zostają bez zmian.
- `Tools/rzeki.py` (uruchamiać z `PYTHONPATH=Tools/_py`) → `Data/rzeki.bin`:
  - linie Natural Earth `ne_10m_rivers_lake_centerlines`, wygładzone (Chaikin ×3), z punktem co 20 m w grze;
  - zakola ±20 m (fale 170 i 420 m) wygaszone na końcach;
  - linie przedłużone o 40 m, żeby dopływ wchodził w rzekę;
  - połowa szerokości wg `scalerank`: 20 / 14 / 10 / 7 m;
  - 326 linii, ~400 tys. punktów; kontrola: Wisła 77 m od startu w Krakowie.
- `FSwiat` (SwiatWysokosci.cpp):
  - wczytuje linie i siatkę 512 m z odcinkami w zasięgu 300 m;
  - `NajblizszaRzeka` zwraca odległość od brzegu i poziom wody;
  - poziom wody liczy się raz przy wejściu (`PoliczPoziomyRzek`): teren doliny pod punktem, wzdłuż rzeki minimum ±300 m i średnia ±200 m, 60 cm niżej;
  - `Wysokosc` w regionie:
    - dolina bez pagórków do ~200 m od brzegu;
    - lustro wody jest płaskie;
    - przy wodzie próg 40 cm, czyli ostra krawędź;
    - dalej skarpa 1:2 (25–120 m) do terenu;
  - na mapie grywalnej wcięcie rzek z pikseli (−4 m w DEM) jest oddawane (`DemPiksel(..., bBezWciec)`);
  - `Woda` na mapie grywalnej rzeki bierze z linii, a jeziora i morze z pikseli;
  - `OdRzeki` → UV2.x kafla (metry).
- `T_Europa_Woda.B` = rzeki z pikseli poszerzone o 1 piksel (`tekstury_teren.py woda`; przed `Zbuduj_Teren.bat` usunąć `Content/Swiat/Teren/T_Europa_Woda.uasset`).
- `M_Teren` w regionie:
  - wyłącza wodę i błoto rzek z pikseli;
  - rzekę rysuje z UV2: woda przy odległości < 0 z ostrą krawędzią, jasna linia 0,6 m od brzegu, środek ciemniejszy (6 m), pas błota z kamyczkami 2–5 m.
  - Mapa Europy (`Region=0`) wygląda jak wcześniej.
- Wiadomo: przy ujściu dopływu poziom wody może skoczyć o kilkadziesiąt cm (wygrywa bliższy brzeg). Zakola i szerokości ustawione na oko, nie sprawdzone w grze.

## Kwadraty na mapach: prawdziwa przyczyna i rzeki z linii na mapach oddalonych (17.11.2026)
Użytkownik: rzeka i teren regionu są „mega”, o ten efekt chodzi. Na mapie oddalonej dalej były kwadraty błota i wody oraz kanciaste plamy.
- **Przyczyna:** `ProceduralMeshComponent` zapisuje UV w połowie precyzji (16 bit). UV1 mapy Europy (0..1) skakało co 2–4 piksele mapy, czyli co 128–256 m, więc woda, błoto i plamy koloru wychodziły kwadratami. UV0 detalu (do 1024) też było zaokrąglane.
- **Poprawka:** `M_Teren` liczy UV z pozycji w świecie (`WorldPosition`, pełna precyzja LWC):
  - `uv1 = (Y/Bok + 0,5; 0,5 − X/Bok)`;
  - `uv0 = Frac(XY / (400·65536))·65536`: okres 262 km, wszystkie skale detalu to 1/2^n, więc tekstury nie mają szwów (druga warstwa trawy 1/32 zamiast 0,037);
  - UV0/UV1 z C++ nie są już używane przez materiał (zostały w siatce); UV2 (odległość od rzeki) zostaje.
- **Mapy oddalone, rzeki z linii:** `tekstury_teren.py woda` liczy `T_Europa_Woda` od nowa:
  - R = pole odległości morza i jezior z pikseli połączone z dokładną odległością od linii `Data/rzeki.bin`; rzeka ma tu 100 m szerokości, bo węższa znikałaby między pikselami 64 m;
  - B = te rzeki z pasem błota (mapa grywalna je wyłącza i rysuje własne z UV2);
  - liczenie trwa ~35 s;
  - symulacja okolic Krakowa: rzeka gładka i ciągła.
- Małe jeziora z pikseli nadal są kanciaste (64 m). Do zrobienia później z wielokątów Natural Earth, jeśli będzie przeszkadzać.
- **Poprawka tego samego dnia:** UV z `WorldPosition` (LWC) dawały wodę w złych miejscach. Na Mazurach woda była narysowana na lesie z drzewami, a na morzu jej brakowało. Tekstura była poprawna, więc źle liczyła się pozycja w materiale. Teraz:
  - C++ (`ATerenKafle::Tick`) podaje róg kafla w double jako Custom Primitive Data: 0, 1 = UV mapy Europy, 2, 3 = detal z okresem 1024;
  - `M_Teren` dolicza `LocalPosition` (małe liczby) przez parametry `Kafel0..3` z `use_custom_primitive_data`;
  - wynik jest ten sam co dawne UV z C++, ale w pełnej precyzji.
- Jeziora na mapie grywalnej nie są jeszcze spłaszczone: dostają tę samą przesadę i pagórki co ląd. Do zrobienia, jeśli będzie widać.

## Przegląd kodu i testy automatyczne (18.11.2026)
Prośba: sprawdzić błędy, tekstury psujące się na oddaleniu, przetestować kod.
- `GRegion` w `SwiatWysokosci.cpp` jest `std::atomic<bool>`, bo kafle czytają go na wątkach roboczych.
- Wysokość w regionie pyta o najbliższą rzekę raz na punkt (odległość i poziom razem), a nie dwa razy.
- `ATerenKafle::Tick` odrzuca gotowe kafle, które w międzyczasie wypadły poza pierścień (nie tworzy ich na darmo).
- Tekstury terenu mają `never_stream`. `ProceduralMeshComponent` nie podaje streamerowi gęstości tekseli, więc z daleka silnik mógł wczytywać za małe mipmapy (rozmyte, „psujące się” tekstury). Hipoteza, czeka na zrzut użytkownika.
- **Testy:** `Source/grarts2d/Testy/SwiatTesty.cpp`, uruchamiane przez `Tools\Testy.bat` (edytor zamknięty, pełny log `Saved\Testy.log`):
  - `grarts2d.Swiat.Dane`: Kraków 150–300 m, Śnieżka 900–1700 m, Bałtyk = morze poniżej 0, Śniardwy = jezioro;
  - `grarts2d.Swiat.MapaGrywalna`: brak uskoków co 4 m na 6 km, Wisła w promieniu 2 km, płaskie lustro wody, brzeg wyżej niż woda, wyjście z regionu przywraca wysokości Europy.
  - Wynik 18.11.2026: oba testy przeszły.
- Uwaga: nowy plik `.cpp` wymaga skasowania `Intermediate/Build/Win64/x64/grarts2dEditor/Development/Makefile.bin` przed `Kompiluj.bat`, inaczej UBT go nie widzi.

## Większe oddalenie na mapie grywalnej (18.11.2026)
Użytkownik: największe oddalenie w regionie (~420 m widoku) było za małe. Wybrał widok ~2 km.
- `AKameraMapy::MaxOdlRegionu` = 5,7 km (było 1,2 km). Przy kącie 38° i polu widzenia 20° widać ~2 km wszerz, a róg kadru jest ~1,6 km od ogniska.
- `ATerenKafle::PromienRegionu` = 5, czyli 11×11 kafli (było 7×7). Zapas to co najmniej 2,5 km od ogniska.
- `MaxPrzesuwRegionu` = 1,5 km/s (z Shiftem): wyżej jazda jest za szybka, żeby kafle zdążyły się policzyć.
- `ZasiegDrobnych` = 2,5 km: dalej od kamery kępki trawy i kamyki nie są rysowane (odległość znikania instancji ISM).
- Wejście do regionu zostawia kamerę na wysokości progu 2,5 km, bez skoku. Wyjście ustawia mapę Europy na 1,2 × max regionu.
- Kafle ładują się po ~2,5 raza dłużej niż przy 7×7. Drzewa to Nanite, więc z daleka upraszczają się same.
- Niesprawdzone w grze: płynność (FPS) i cienie VSM przy dużym oddaleniu. Wcześniej cienie urywały się przy samych drzewach.

## Jaśniejsze korony, wyższe słońce, pochylone drzewa (19.11.2026)
Użytkownik (zrzut z regionu): środki koron za ciemne, chce różne ustawienia drzew (pochylone, jak na sawannie). Próba z Tripo odrzucona: własny styl generatora jest lepszy, nie wracamy do Tripo w tym projekcie.
- `natura_drzewa.py`: wnętrze koron jaśniejsze (`ao` od 0,65 zamiast 0,45; środek karty liścia 0,78 zamiast 0,62). Przebudowane `Zbuduj_Nature.bat`.
- `SwiatGameMode::UstawSwiatloRegionu`: w regionie słońce 45° (było 30°), światło nieba 2,0 (było 1,2), czyli krótsze i jaśniejsze cienie. Mapa Europy bez zmian.
- `TerenKafle.cpp` (`Sadz`): każde drzewo (nie krzak) ma pochylenie 0–7° w losową stronę i losowe proporcje (szerokość ×0,85–1,2, wysokość ×0,85–1,15).
- Niesprawdzone w grze. Plan na później: korony parasolowe, topole, wierzby w generatorze, ciekawszy teren i woda.

## Mapa strategiczna, etap 1: styl pergaminu (19.11.2026)
Użytkownik pokazał wzór: szara papierowa mapa, wyraźne błękitne rzeki z obrysem, lasy jako wzorek drzewek, później nazwy prowincji, granice, herby, wioski i budynki w uproszczeniu. Plan w 5 etapach: (1) styl mapy, (2) dane prowincji 3 poziomy, (3) granice jako wstęgi, (4) nazwy wygięte wzdłuż prowincji, (5) herby, wioski, budynki, drogi. Nazwy i grafiki własne, ze wzoru bierzemy tylko typ wyglądu. Etap 1 zrobiony:
- **Przejście:** poza regionem `AKameraMapy::StrefaMapy()` rośnie od 0 przy 7 km oddalenia do 1 przy 14 km (logarytmicznie). Mapa pośrednia i pełna to teraz jedna mapa: blisko teren jak dawniej, dalej pergamin. `SzerokoscWidoku()` podaje szerokość kadru.
- **`M_Teren`, parametry `Mapa` i `SkalaMapy`** (ustawia `ATerenKafle::Tick` na wspólnym `MaterialMapy` dla kafli poza regionem):
  - papier jasnoszary (0,30/0,29/0,27 liniowo) z fakturą `T_Mapa.G`;
  - las (z koloru Europy, ten sam próg co runo w regionie) ciemniejszy z jasnymi drzewkami `T_Mapa.R`;
  - góry i skały jaśniejsze;
  - morze i jeziora szaroniebieskie, przy brzegu ciemniejszy pas, na brzegu linia tuszu ~1,2 px;
  - rzeki z pikseli wyłączone (rysuje je `AMapaRzeki`);
  - wzorki mają stałą wielkość na ekranie: `SkalaMapy = log2(20 · Bok / szerokość widoku)`, dwie oktawy przenikają się wg części ułamkowej.
- **`T_Mapa.png`** (`tekstury_teren.py mapa`): R = 8×8 drzewek na kafel (korona, cień w prawo w dół, blik), G = faktura papieru.
- **`AMapaRzeki`** (`Swiat/MapaRzeki.*`, nowy): wstęgi z `Data/rzeki.bin` (co 5. punkt = co 100 m), przerwane na jeziorach i morzu. `M_MapaRzeki` (półprzezroczysty, bez testu głębi, oświetlony jak płaski teren) rozsuwa wierzchołki osi o `Szerokosc` = 0,25% szerokości widoku × ranga rzeki; środek błękitny, brzegi ciemny tusz; krycie = `Mapa`. W regionie ukryte.
- `FSwiat::Rzeki(...)` udostępnia linie rzek. `Zbuduj_Teren.bat` buduje też `M_MapaRzeki`, `ue_sprawdz_material.py` go kompiluje.
- Kompilacja i testy OK. **W grze nieoglądane** (zrzut kontrolny zablokowany). Kolory dobrane na oko pod oświetlenie mapy Europy (auto ekspozycja).
- **Ocena użytkownika:** mapa się podoba („o to chodziło”). Błąd: przy oddalaniu i przybliżaniu tekstury skaczą i się rozjeżdżają (szwy na granicach kafli, trójkąty przy brzegach, poziome smugi).
- **Przyczyna:** `M_Teren` liczył UV mapy z `LocalPosition`. W pikselu jest to liczone z głębi i przy kaflach setek km każdy kafel myli się inaczej, więc przy zmianie poziomu kafli obraz skakał.
- **Poprawka:** UV1 siatki to pozycja w kaflu (i/K, j/K), dokładna nawet w 16-bitowych UV ProceduralMesh. Nowe Custom Primitive Data 4 = bok kafla / Bok. Materiał: uv1 = róg + pozycja × bok, uv0 tak samo (× Bok/400). `M_Teren` ma pełną precyzję liczb (`MFPM_FULL`). Kompilacja, shadery i testy OK, w grze nieoglądane.

### Cała mapa papierowa (bez etapu realistycznego terenu)
- **Ocena użytkownika:** poprawka skaczących tekstur „idealnie”. Prośba: usunąć etap przybliżenia z realistycznym terenem (zielone plamy, bez drzew) i zostawić papier na całym oddaleniu.
- `AKameraMapy::StrefaMapy()` zwraca 1 na każdym oddaleniu poza regionem (dawniej przejście 7–14 km). Realistyczny teren jest teraz tylko w regionie (mapa grywalna z drzewami). Wejście do regionu przełącza wygląd od razu, bez przenikania.
- Kompilacja i testy OK, w grze nieoglądane.
- **Uwaga użytkownika:** zielony etap dalej był widoczny. To była już mapa grywalna z wysoka (2,5–5,7 km), gdzie drzewa są za małe i widać plamy tekstury. Wybrano „papier niżej”.
- `ProgRegionu` 2,5e5 → 1,0e5 (wejście do mapy grywalnej przy 1 km), `MaxOdlRegionu` 5,7e5 → 1,5e5 (widok w mapie grywalnej najwyżej ~0,5 km szerokości). Wyjście nadal przy `max(Prog·1,6; Max·1,2)` = 1,8 km.
- `SkalaMapy` ograniczona do 0..16 (dawniej 14), żeby papier przy 1 km nie był rozciągnięty.
- Kompilacja i testy OK, w grze nieoglądane.

### Większe i gładsze wzniesienia w mapie grywalnej
- **Uwaga użytkownika (zrzut):** wzniesienia i góry przechodzą w teren za ostro, powinny być większe i gładsze.
- **Przyczyna:** DEM czytany dwuliniowo (piksel 64 m w grze) × `PrzesadaRegionu` 3 dawał załamania na każdym pikselu (ostre grzbiety, płaskie trójkąty), a drobne pagórki (200 m) robiły „grudki”.
- **Zmiana (`SwiatWysokosci.cpp`, tylko mapa grywalna):** nowe `DemGladki()` = B-spline 3. stopnia na siatce co 2 piksele (węzeł = średnia 2×2). Do tego dwie największe oktawy pagórków mapy Europy, bez drobnicy. Przy brzegu morza (0–30 m) zostaje dawna wysokość, więc linia brzegu się nie przesuwa. Pagórki regionu: ~2,5 km (100) + ~700 m (15) zamiast ~800 m (80) + ~200 m (20).
- Wygładzenie podnosiło dno wąskich dolin ponad teren obok rzeki. Dlatego do ~200 m od brzegu teren ma „podłogę” (lustro wody + 1 m + 2% spadku), która płynnie wygasa do 300 m.
- **Test:** `MapaGrywalna` mierzy załamania co 4 m na linii 6 km przez Kraków, z dala od rzek. Było 156 cm, jest 6,7 cm; próg testu to 30 cm. Oba testy OK, w grze nieoglądane.
- **Uwaga techniczna:** `cmd //c "Kompiluj.bat nopause"` z Git Basha potrafi po cichu nic nie zrobić i zostawić stary `Kompilacja.log`. Uruchamiaj przez PowerShell pełną ścieżką: `& "D:\GRY\grarts2d\Kompiluj.bat" nopause`, i sprawdzaj datę logu.

## Audyt i plan rozwoju (20.11.2026)
- Nowy plik `Docs/Audyt_i_plan_rozwoju.md`: stan gry (świat gotowy, brak rozgrywki), ryzyka (brak gita, zakres, niezmierzona wydajność), mechaniki, które pasują do gry, plan etapów z czasem i sposób walidacji (playtesty, wishlisty, konkurencja).
- Kod bez zmian.

## Kopia na GitHubie (20.11.2026)
- Projekt jest w repozytorium `JestemJudasz/Judaszver0.1`, gałąź `claude/project-thread-sjbjq6`. Folder `D:\GRY\grarts2d` nie ma jeszcze lokalnego gita: AI nie dostało zgody na usuwanie plików w folderze, a bez niej git zostawia pliki blokad `.lock`. Kopię odświeża się, wysyłając zmienione pliki do tej gałęzi z sesji AI.
- W repo: `Source`, `Config`, `Content`, `Art`, `Data`, `Docs`, `Tools` (bez `Tools/_py`), pliki z katalogu głównego. Poza repo (`.gitignore`): `Binaries`, `Intermediate`, `Saved`, `DerivedDataCache`, `Dane_zrodlowe` (2 GB surowych danych), `Tools/_py`.
- `Data/europa_wys.r16` (134 MB) przekracza limit GitHuba 100 MB, więc w repo jest jego kopia `Data/europa_wys.r16.gz`. Po zmianie wysokości (`Przelicz_Europe.bat`) trzeba ją odświeżyć: `gzip -9 -k -f Data/europa_wys.r16`. Odtworzenie: `gzip -d -k Data/europa_wys.r16.gz`.
- Git LFS nie działa z tej sesji (serwer LFS zablokowany), więc assety są w zwykłym gicie. Duże pliki zmieniać rzadko, bo każda wersja zostaje w historii.
- `CLAUDE.md` w repo ma na końcu dopisane zasady graphify z wcześniejszej wersji repo.

## Zgranie przybliżenia mapy papierowej z grywalną (20.11.2026)
- **Uwaga użytkownika:** przybliżenie na mapie papierowej i na mapie grywalnej jest „rozjechane”.
- **Przyczyny (`KameraMapy.cpp`):**
  1. Na progu wejścia (1 km) mapa papierowa miała kąt 50° i pole widzenia 25° (kadr ~443 m), a mapa grywalna 38° i 20° (kadr ~353 m). Wejście przybliżało więc skokiem o ~25% i zmieniało kąt.
  2. Wejście ustawiało stałą odległość `ProgRegionu` zamiast obecnej, a środek kadru przeskakiwał na punkt pod kursorem.
  3. Wysokość ogniska zmieniała się po przełączeniu przesady rzeźby i dopiero dojeżdżała, więc kadr dryfował.
- **Poprawka:**
  - Mapa papierowa na progu `ProgRegionu` ma kąt `NachylenieRegionu` i pole `FovRegionu`. Kąt płynnie rośnie do `NachylenieDaleko` przy 40 km, pole widzenia do `FovBlisko` przy 20 km i dalej do `FovDaleko` jak dawniej. `NachylenieBlisko` usunięte.
  - Wejście zostawia środek kadru i obecną odległość (przycięte do zakresu regionu), a zoom do kursora dokańcza się już w regionie.
  - Wyjście: jeden klik kółka dalej niż najdalszy widok regionu (`MaxOdlRegionu · KrokKolka` = 1,8 km), przy tym samym kącie wychodzi to jak zwykłe oddalenie.
  - Przy przełączeniu ognisko od razu dostaje wysokość nowego terenu.
- Skutek uboczny: mapa papierowa przy bliskim oddaleniu (1–3 km) jest teraz bardziej ukośna niż dawniej (38–48° zamiast 50–52°). Od ~5 km jak dawniej.

## Łagodniejsze góry i płynne przejścia na mapie grywalnej (20.11.2026)
- **Uwaga użytkownika:** góry i wzniesienia mają być gładsze, lepiej przechodzić w teren, wszystko ma się ładnie łączyć.
- **Przyczyna:** przesada rzeźby ×3 (razem z ×2,5 pionowo) zamieniała prawdziwe zbocza w ściany: w Tatrach połowa zboczy > 20°, 10% > 42°, w Alpach mediana 54°. Do tego skała w `M_Teren` wchodziła twardą linią (57–67°).
- **Zmiany (tylko mapa grywalna, mapa papierowa/Europy bez zmian):**
  - `SwiatWysokosci.cpp`: `DemGladki` liczy B-spline na siatce co 3 piksele (`OczkoGladkie`, węzeł = średnia 3×3), było co 2.
  - Nowe `Przesada(D)` = 3 · Z · ln(1 + D/Z), `FSwiat::ZlagodzenieGor` = 1200 m: niskie pagórki dalej ×3, wysokie góry coraz łagodniej (100 m → 288, 500 m → 1254, 1500 m → 2919 zamiast 300/1500/4500). Funkcja ma ciągłą pochodną, więc nie robi załamań.
  - Symulacja na danych (spadki w grze, percentyl 90): okolice Krakowa 25° → 14°, Tatry 42° → 24°, Alpy 69° → 45°. Góry są niższe niż dotąd (Tatry ~60% dawnej wysokości nad doliną).
  - `ue_teren.py`: skała w regionie = saturate((0,82 − nz) · 2,4 + (T_Detal.G − 0,5) · 0,35): prześwituje od ~35°, pełna od ~66°, granica poszarpana łatami 1–8 m zamiast linii. Mapa Europy ma dawny próg (osobne węzły `stromo_eu`). Przebudowane `Zbuduj_Teren.bat`.
- Kompilacja, shadery i oba testy OK. W grze nieoglądane. Do strojenia: `ZlagodzenieGor` (większe = wyższe, ostrzejsze góry) i `OczkoGladkie`.
