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

## Czytelna mapa papierowa: znaki jak rysowane przez dziecko, bez doczytywania (20.11.2026)
- **Uwaga użytkownika:** na mapie papierowej nie widać, gdzie są góry; po przybliżeniu coś się doczytuje. Góry i lasy mają mieć proste kształty, „jakby dziecko namalowało”.
- **Przyczyna doczytywania:** mapa papierowa miała rzeźbę 3D z kafli quadtree; przy przybliżaniu kafle dzieliły się na dokładniejsze i rzeźba zmieniała się na oczach gracza. Do tego napis „Doczytywanie terenu… N%”.
- **Zmiany:**
  - Mapa papierowa jest płaską kartką na wysokości 0 (`ATerenKafle::Zbuduj(…, bPlaski)`), kafle tylko do poziomu 7 (`MaxPoziomMapy`, 4 km). Dzielenie kafli niczego nie zmienia na ekranie. Kamera (`AKameraMapy::Ziemia`) i wstęgi rzek (`AMapaRzeki`) też na 0. Mapa grywalna bez zmian (rzeźba, ten sam kąt i pole widzenia na progu).
  - Napis „Doczytywanie terenu” wyłączony (`SwiatGameMode`); ekran ładowania na starcie i przy przejściach został.
  - `Tools/mapa_znaki.py` (nowy, numpy + pillow, bez Unreala): atlas `Art/Teren/T_MapaZnaki.png` (16 znaków: góry, ośnieżone szczyty, pagórki, drzewa liściaste i iglaste, po 1–3 sztuki; gruby kontur tuszem, płaskie kolory) i dane `T_MapaDane.png` (4096², R = rzeźba w promieniu ~3 km, G = las, B = iglaste, A = ląd). Rysuje też podglądy `Docs/mapa_znaki_podglad_*.png`.
  - `M_Teren` (`ue_teren.py`, węzeł Custom „ZnakiMapy”): znaki w siatce trójkątnej w UV mapy, stała wielkość na ekranie (`ATerenKafle::ZnakiGestosc` = 16 oczek na szerokość widoku). Punkty siatki zostają na miejscu przy każdym przybliżeniu, nowe znaki wyrastają płynnie między starymi (bez podmieniania). Typ znaku: góra przy rzeźbie > 0,3 (ośnieżona > 0,7), pagórek > 0,15, inaczej drzewo w lesie. Znaki stoją prosto względem ekranu (obrót Q/E) i są rozciągnięte o 1/sin(kąt), więc przy ukośnym widoku nie są spłaszczone. Bliższe znaki zasłaniają dalsze.
  - Tło: wzorek kropkowanych drzewek usunięty; pod lasem lekko zielonkawy papier, w górach ciepły beż (z `T_MapaDane`).
  - Parametry co klatkę (`ATerenKafle::Tick`): `ZnakiPoziom`, `ZnakiOs` (prawo ekranu w UV), `ZnakiRozciag`.
- Kompilacja, shadery i oba testy OK. Zrzut kontrolny (40 km nad Krakowem): znaki widoczne, góry na południu, drzewa w lasach.
- **Do strojenia:** `ZnakiGestosc` (mniej = większe znaki), progi w `KOD_ZNAKOW` (`ue_teren.py`) i w `mapa_znaki.py` (podgląd liczy to samo). Po zmianie tekstur: usunąć `Content/Swiat/Teren/T_MapaZnaki.uasset` / `T_MapaDane.uasset` i `Zbuduj_Teren.bat`.

## Budynki, etap 1: kościół budowany w fazach (20.11.2026)
- **Prośba użytkownika:** praca nad budynkami, osadnikami i wojskiem, zaczynamy od budynków. Wzór (zrzuty w czacie): kamienny kościół z wieżą, budowany w kilku fazach z rusztowaniami, „ludziki powoli pracują i z czasem powstaje pełny budynek”. Wygląd ludzików jeszcze nieustalony.
- `Tools/budynki.py` (numpy + pillow, jak `natura_drzewa.py`): `Art/Budynki/SM_Kosciol_F1..F5.glb` i tekstury kafelkowe `T_Bud_Kamien` (ciosy, kafel 2 m), `T_Bud_Dach` (gont, 2 m), `T_Bud_Drewno` (1 m). Kościół: wieża 4,5 × 4,5 m, 15 m + iglica 6 m, nawa 13,5 × 8 m (mury 6,5 m), prezbiterium 4,5 × 6 m; cokół schodzi 3 m w ziemię. Fazy: F1 cokół i niskie mury w rusztowaniu, F2 wyższe mury i wieża w rusztowaniu, F3 gola więźba, F4 prezbiterium kryte, nawa do połowy, iglica, F5 gotowy z krzyżem. Przy budowie leżą kamienie, skrzynie, beczki, belki (ubywa ich z fazami). Materiały w modelu: „Kamien”, „Dach”, „Drewno”.
- `Tools/ue_budynki.py` + `Tools/Zbuduj_Budynki.bat` (edytor zamknięty, kasuje `Content/Swiat/Budynki`): import z Nanite, `M_Bud_Kamien/Dach/Drewno` = tekstura × kolor wierzchołków × jasność (2,3 / 2,0 / 1,8; przy ×1 mury w grze wyszły ciemnoszare), `M_Bud_Podglad` (półprzezroczysty, parametr `Kolor`). Log `Saved/Zbuduj_Budynki.log`.
- `Source/grarts2d/Budowy/Budowa.*` (`ABudowa`): plac budowy, `Postep` 0..1, fazy dzielą postęp po równo, ostatnia = gotowy. Postęp dają tylko osadnicy (`DodajPrace` × `ABudowa::Tempo`, `CzasBudowy` 240 s pracy jednej osoby). `FRodzajBudynku::Kosciol()` = model, fazy, czas, obrys.
- `Source/grarts2d/Budowy/Budowniczy.*` (`ABudowniczy`, stawia go `SwiatGameMode`): tylko na mapie grywalnej. **B** = kościół pod kursorem (zielony = można, czerwony = nie), **R** = obrót o 45°, **lewy klik bez przeciągania** = postaw (z Shiftem stawia dalej), **prawy klik / Esc** = anuluj, **T** = tempo budowy ×1/×10. Warunki: brak wody i rzeki (≥ 4 m), różnica terenu pod obrysem ≤ 2,5 m (`MaxSpadek`), odstęp od innych budynków. Budynek stoi na najwyższym punkcie obrysu. Podpowiedź w lewym dolnym rogu. Poza mapą grywalną budynki są ukryte. Opcja `-kosciol`: po wczytaniu regionu stawia 5 kościołów w kolejnych fazach i przenosi tam kamerę (zrzut kontrolny).
- `ATerenKafle::Wykarczuj(Srodek, Promien)`: usuwa drzewa i drobiazgi w kole pod budynkiem; kafle liczone później też je pomijają (`Polany`). `AKameraMapy::UstawOgnisko`.
- Kompilacja, import i oba testy OK. Zrzuty kontrolne: `Docs/budynki_zrzut_fazy.png` (pierwszy, ciemne mury, wieża z prawej) i `Docs/budynki_zrzut_fazy2.png` (jaśniej, wieża z lewej jak na wzorze, materiał przed kościołem). Kopia plików sprzed zmian: `Saved/Kopia_przed_budynkami/`.
- **Dalej:** osadnicy (ludziki noszą materiał i pracują przy rusztowaniach, napędzają `DodajPrace`), potem wojsko. Do strojenia: jasność materiałów, kolor dachu, gęstość rusztowań.

- 2026-11-20: Tools/ue_ludzie.py - M_Ludzie dostaje flage used_with_nanite (gra zglaszala 'missing usage flag Nanite' i brala material domyslny).
- 2026-11-20: Tools/ue_budynki.py - M_Bud_* z flaga used_with_nanite (wczesniejsze zrzuty budynkow pokazywaly material domyslny zamiast tekstur).

## Osadnicy, etap 1: budowniczy i tragarze przy kościele (20.11.2026)
- **Decyzja użytkownika:** ludziki realistyczne, powiększone ok. ×1,4 (przy 1,7 m mają na ekranie kilka pikseli). Rozpoznawane po sylwetce, kolorze tuniki i tym, co niosą; wojsko na tym samym ciele z kolorem gracza na tunice/tarczy.
- `Tools/ludzie.py` → `Art/Ludzie/SM_Ludz_{Tulow,Glowa,Reka,Noga,Mlotek,Kamien}.glb`: sztywne części z początkiem układu w stawie (biodro 90 cm, szyja +50, barki ±23/+45). Kolor wierzchołków, alfa = maska: 1 tunika, 0,5 włosy. Korzysta z `Siatka` i brył z `budynki.py` (Siatka przyjmuje teraz nazwy materiałów i kolory RGBA).
- `Tools/ue_ludzie.py` + `Tools/Zbuduj_Ludzi.bat`: import do `/Game/Swiat/Ludzie`, `M_Ludzie`: kolor wierzchołków, tunika z Custom Primitive Data 0..2, włosy z CPD 3..5, ×1,8. Flaga `used_with_nanite` potrzebna (import włącza Nanite na siatkach; bez flagi gra brała materiał domyślny). To samo poprawione w `M_Bud_*`: **wcześniejsze zrzuty budynków pokazywały materiał domyślny**. `M_Natura` i `M_Natura_Kora` też zgłaszają brak tej flagi; nieruszane, bo drzewa użytkownik zaakceptował.
- `Source/grarts2d/Ludzie/Ludzik.*` (`ALudzik`): komponenty-części ruszane w kodzie (chód, zamach młotkiem, schylanie, noszenie przed sobą), skala ×1,4, `UstawKolory`. Role: **Budowniczy** chodzi po prostokącie wokół budynku (`FRodzajBudynku::ObwodMin/Max`, poza rusztowaniem), staje twarzą do muru i stuka 4–8 s (`DodajPrace(dt)`). **Tragarz**: sterta kamieni (`Sterta`) → pod mur (`ZrzutX/ZrzutY`) → odkłada (`DodajPrace(6)`). Po skończeniu budowy odchodzą kilka metrów i stoją. Ruch prosto po terenie (`FSwiat::Wysokosc`), bez szukania drogi.
- `ABudowniczy::WyslijLudzi`: po postawieniu budynku przychodzi 2 budowniczych + 2 tragarzy z ~25 m, losowe stonowane kolory tunik i włosów. `-kosciol`: osadnicy od razu przy pracy w fazach 1–4. Poza mapą grywalną ukryci.
- Kościół: budowa ~1 min z 4 osadnikami (T = ×10). Skrzynie i beczki odsunięte dalej od murów (ścieżka budowniczych), karczowanie 16 m.
- Kompilacja, import, testy OK. Zrzuty: `Docs/ludzie_zrzut*.png` (ostatni `ludzie_zrzut4.png`).
- **Do strojenia:** faktura kamienia i gontu z bliska prawie niewidoczna przy efekcie pikseli (szwy węższe niż piksel) – większe ciosy / mocniejszy kontrast; mur lekko niebieskawy w świetle regionu; ludziki małe przy największym oddaleniu.
- **Dalej:** wojsko (te same części + hełm, tarcza, włócznia, kolor gracza; przy dużych armiach części jako instancje).
- 2026-11-20: **żółte kwiatki usunięte z łąk mapy grywalnej** (prośba użytkownika: przy efekcie pikseli wyglądały jak plamy farby). `ue_teren.py`: `c = duze` zamiast mieszania z `kwiat` (węzły `w_kwiat` zostały, nieużywane); przebudowane `Zbuduj_Teren.bat`, shader bez błędów. Tekstura kamienia: 4 rzędy ciosów na 2 m (było 6), większe różnice jasności (0,72–1,0), grubsze spoiny; gont 0,7–1,1. Zrzut `Docs/bez_kwiatkow_zrzut.png`: kwiatków brak, ciosy widać, ale mur wyszedł ciemniejszy niż jasny wapień ze wzoru (do rozjaśnienia: jasność `M_Bud_Kamien` w `ue_budynki.py`, teraz 1,4).

## Osada, etap 1: domy, obóz startowy, pasek budowy (20.11.2026)
- **Prośba użytkownika:** zaczynamy GUI: dolny pasek budowy z kategoriami jak w Manor Lords. Docelowo gracz zaczyna w wybranym miejscu: obóz z namiotów i 10 osadników (5 rodzin), namioty to tylko 5 miejsc, więc trzeba stawiać domy, kilka rodzajów, budowane tak samo jak kościół przez te rodziny. Surowce (drewno, kamień) później; na razie budowa kosztuje tylko pracę.
- `Tools/budynki.py`: nowe materiały „Tynk” i „Strzecha” (tekstury `T_Bud_Tynk`, `T_Bud_Strzecha`), domy po 4 fazy: `SM_Chata` (bale na zrąb, strzecha, 7 × 5 m), `SM_DomPruski` (mur pruski: biały tynk, ciemny szkielet belek, strzecha, 9 × 6 m), `SM_DomDuzy` (kamienny parter, piętro z muru pruskiego, gont, komin, rusztowanie, 11 × 7 m); fazy: podmurówka i pierwsze bale / podwalina, ściany albo goły szkielet, szczyty + więźba + pół dachu, gotowy. Drzwi i materiał po stronie −Y generatora (+Y w Unrealu), ludziki ×1,4, więc drzwi 2,4 m. `SM_Namiot`, `SM_Ognisko` bez faz. `szczyt`, `polac`, `mury`, `wiezba` przyjmują teraz materiał / kolor.
- `Tools/podglad.py` (podgląd bez Unreala, był tylko w sesji) i `Tools/ikony.py` → `Art/Budynki/T_Ikona_*.png` (gotowy budynek, przezroczyste tło, piksele). `Zbuduj_Budynki.bat` uruchamia oba; `ue_budynki.py` importuje ikony jako tekstury UI (bez mipmap, filtr Nearest) i materiały `M_Bud_Tynk` (×1,3), `M_Bud_Strzecha` (×1,5).
- `Tools/ludzie.py`: kobiety `SM_Ludz_TulowK` (suknia do kostek, fartuch), `SM_Ludz_GlowaK` (chusta = maska włosów, kolor z CPD 3..5), `SM_Ludz_NogaK`; `SM_Ludz_Bal` (bal noszony przy domach). Trzeba przebudować `Zbuduj_Ludzi.bat`.
- `FRodzajBudynku`: `Kategoria`, `Opis`, `Ikona`, `Niesione`, `Rodzin`, `Wejscie`; rodzaje `Chata()`, `DomPruski()`, `DomDuzy()`, `Namiot()`, `Ognisko()`, lista paska `DoBudowania()` (nowy budynek = dopisać tam). Fazy = 1: model bez `_F`, od razu gotowy.
- `Source/grarts2d/Osada/Osada.*` (`AOsada`, stawia ją `ABudowniczy`): budynki, ludzie, rodziny. `ZalozOboz`: 5 namiotów po okręgu 6,5 m wejściem do ogniska, przy każdym rodzina (mężczyzna + kobieta). Co 0,5 s: wolni osadnicy idą do najbliższej niedokończonej budowy (najstarsze budowy najpierw, komplet budowniczych i tragarzy z `FRodzajBudynku`), a do gotowego domu wprowadza się rodzina z najbliższego namiotu i jej namiot znika (duży dom: 2 rodziny). Bez pracy osadnik kręci się 2–5 m od wejścia swojego domu / namiotu.
- `ABudowniczy`: po wejściu na mapę grywalną najpierw **wybór miejsca obozu** (podgląd 5 namiotów i ogniska, zielony / czerwony, lewy klik). Potem **pasek budowy** na dole (`Source/grarts2d/UI/PasekBudowy.*`, Slate): przycisk „BUDOWA”, zakładki Mieszkalne / Religia, nad nimi budynki z ikoną, nazwą i podpowiedzią (opis, ilu buduje, ile rodzin). Klik = podgląd pod kursorem jak dawniej (R obrót, Shift kilka, prawy klik / Esc anuluj); Esc bez wybranego budynku zamyka zakładkę. Klawisz B usunięty. Panel osady w lewym górnym rogu: osadnicy (wolni), rodziny (w namiotach), budowy. Domyślny obrót 90° (front do kamery).
- Opcja `-osada` (zrzut kontrolny): obóz, za nim chata gotowa (rodzina już w niej), dom pruski w 3. fazie, duży dom w 1. fazie, rodziny od razu przy pracy, pasek otwarty. `-zrzut` bez `-osada` nie pokazuje interfejsu.
- Kopia plików sprzed zmian: `Saved/Kopia_przed_osada/`.
- 2026-11-20: podpowiedz klawiszy w lewym dolnym rogu bez zawijania (zawijala sie po 1-2 slowa, zrzut uzytkownika); wejdzie przy nastepnej kompilacji.

- 2026-11-21: poprawki kompilacji ekonomii drewna: w `Ludzik.cpp` (`Przewiez` i noszenie do magazynu) zmienna `Cel` -> `CelB` (zaslaniala skladowa `FVector2D Cel`, blad C4458); `AOsada::UstawTeren` przeniesione z `Osada.h` do `Osada.cpp` (niekompletny typ `ATerenKafle`). Kompilacja i import ludzi/budynkow OK, zrzut `Saved/zrzut_drewno.png`.

## Ekonomia, etap 2: kamień, jedzenie, zadowolenie (21.11.2026)
- **Decyzja użytkownika:** plan "Ekonomia gry" zatwierdzony ("wszystko pasuje dodawaj"), etapy po kolei bez pytania ("kiedy skończysz leć dalej z etapami").
- `Osada/Surowce.h`: czwarty surowiec `ESurowiec::Jedzenie` (jeden rodzaj: jagody, grzyby, mięso). Kamień wrócił do kosztów: kościół 10 kłód / 20 desek / 60 kamieni, dom pruski 6/6/2, duży dom 6/10/12.
- Nowe budynki (`Budowy/Budowa.cpp`, modele w `Tools/budynki.py`, po 4 fazy): **kamieniołom** (2 rodziny, kilof, stos kamieni), **chata zbieracza** (1 rodzina, krzaki z mapy w 30 m, krzak odrasta po 180 s), **chata myśliwego** (1 rodzina, las w 50 m, szansa trafienia 25-60% zależnie od gęstości lasu, sarna = 3 porcje), **spichlerz** (magazyn jedzenia, 40 sztuk). Stosy `SM_Stos_Jedzenie_1..4`.
- Ludziki (`Ludzie/Ludzik.*`): role Kamieniarz, Zbieracz, Myśliwy z animacjami (kilof, łuk, zbieranie pochylonym); części `SM_Ludz_Kilof/Luk/Kosz/Worek/Sarna` w `Tools/ludzie.py`. Noszący pisze, dokąd niesie (na plac / do spichlerza / do obozu).
- `Osada/Osada.*`: co 90 s posiłek, każda rodzina zjada porcję z najbliższego zapasu. Zadowolenie 0-100% = jedzenie 50 + domy 30 + kościół 20, zmienia się powoli (1 pkt/s). Od 60% i przy wolnym domu co 75 s przychodzi nowa rodzina; poniżej 30% co 120 s jedna odchodzi (zostają najmniej 2). Obóz startowy: 20 kłód i 25 porcji jedzenia.
- Interfejs: licznik jedzenia (czerwony, gdy nie starczy na posiłek), serce zadowolenia z podpowiedzią (składniki, czas do posiłku), wiadomości osady pod licznikiem (przybyła / odeszła rodzina, brak jedzenia). Ikony `T_Ikona_Jedzenie`, `T_Ikona_Zadowolenie` (`Tools/ikony.py`).
- `Swiat/TerenKafle.*`: `ZnajdzKrzak`, `DrzewWKole` (gęstość lasu dla myśliwego).
- `-osada` pokazuje też kamieniołom, spichlerz z jedzeniem i chatę zbieracza; okno kamieniołomu otwarte.
- Kopia plików sprzed zmian: `Saved/Kopia_przed_etapem2/`.


## Ekonomia, etap 3: pory roku, pola, młyn, piekarnia, tartak wodny (21.11.2026)
- **Według planu "Ekonomia gry"** (zatwierdzony przez użytkownika), etap 3 bez pytania ("leć dalej z etapami").
- `Osada/Kalendarz.h` (nowy): rok = 4 pory po 240 s gry (16 min przy tempie ×1), liczony od założenia obozu. Przy zmianie pory wiadomość osady; kolor świata zmienia się płynnie w ostatnich 20% pory (`AOsada::KolorPory`, mnoży ustawienia post processu obozu: nasycenie, temperatura bieli, wzmocnienie, ekspozycja; zima szara i chłodna, jesień złota).
- `Osada/Surowce.h`: 6 surowców, nowe **zboże** (snopy) i **mąka** (worki). Spichlerz trzyma jedzenie, zboże i mąkę.
- Nowe budynki (`Budowy/Budowa.cpp`, modele w `Tools/budynki.py`):
  - **Pole** (20 × 14 m, ogrodzone, 2 fazy, 2 rodziny). Wiosną orka motyką (60 s pracy) i siew (50 s), latem zboże rośnie (rolnicy pielą), jesienią żniwa sierpem: 20 snopów na stos przy miedzy. Niezebrane do zimy zboże gnije. Wygląd uprawy podmieniany: `SM_Pole_<Zaorane|Zasiane|Rosnie|Dojrzale|Scierniska>`.
  - **Młyn** (wiatrak kozłak, 1 rodzina): 1 snop → 1 worek mąki w ok. 8 s; skrzydła `SM_Mlyn_Skrzydla` kręcą się przy pracy.
  - **Piekarnia** (piec chlebowy z boku, 1 rodzina): 1 worek mąki → 4 porcje jedzenia w ok. 10 s.
  - **Tartak wodny** (koło wodne `SM_TartakWodny_Kolo` kręci się stale): kłoda → 2 deski w 4 s; da się postawić tylko nad rzeką (koniec koryta do 6 m od środka rzeki).
  - Bilans: jedno pole karmi ok. 7 rodzin przez rok (20 snopów → 20 worków → 80 porcji).
- Ludziki: role **Rolnik** (orka, siew, pielenie, żniwa, snop na stos, zimą w domu) i **Rzemieślnik** (młyn, piekarnia, tartak wodny: przynosi wsad, przerabia). Części `SM_Ludz_Sierp/Snop/Maka/Siewka` w `Tools/ludzie.py`. Zbieracz zimą nie zbiera.
- Zima: każda rodzina przy posiłku pali 1 kłodę. Zadowolenie = jedzenie 40 + ciepło 10 + domy 30 + kościół 20.
- Interfejs: pora roku w liczniku u góry (kolor pory, podpowiedź: co się dzieje, ile do następnej pory), licznik zboża i mąki, okno pola (orka / siew w %, snopy do zebrania, plon), okno młyna / piekarni / tartaku (co z czego robi). Nowa zakładka paska **Rolnictwo** (pole, młyn, piekarnia); tartak wodny w zakładce Surowce.
- `-osada`: wczesna jesień, pole z dojrzałym zbożem i rolnikami przy żniwach, młyn z rodziną, piekarnia; okno pola otwarte, zakładka Rolnictwo.


## Ekonomia, etap 4: owce i sukno, ruda i broń, zbrojownia z wojami (21.11.2026)
- **Według planu "Ekonomia gry"** (zatwierdzony przez użytkownika), etap 4 bez pytania ("leć dalej z etapami").
- `Osada/Surowce.h`: 10 surowców, nowe **wełna** (runo), **sukno** (bele), **ruda żelaza** (kosze), **broń** (włócznie).
- Nowe budynki (`Budowy/Budowa.cpp`, modele w `Tools/budynki.py`, po 4 fazy):
  - **Owczarnia** (Rolnictwo, 1 rodzina): szopa pod strzechą i ogrodzone pastwisko; 6 owiec (`SM_Owca`, `SM_Owca_Strzyzona`) chodzi po zagrodzie, pasie się. Pasterz strzyże owcę z runem nożycami i niesie wełnę na stos przy bramie; runo odrasta po ok. 150 s.
  - **Chata tkacza** (Rzemiosło, 1 rodzina): krosno pod wiatą; 1 runo → 1 bela sukna w ok. 12 s.
  - **Kopalnia rudy** (Surowce, 2 rodziny): rdzawe wzgórze ze sztolnią; górnicy kują rudę kilofem jak kamieniarze.
  - **Kuźnia** (Rzemiosło, 1 rodzina): palenisko, kowadło, miech; kowal przynosi rudę i kłody (węgiel) i kuje: 1 kosz rudy + 1 kłoda → 1 włócznia w ok. 14 s (pierwszy budynek z dwoma wsadami, `Wsad2`).
  - **Zbrojownia** (nowa zakładka **Wojsko**, 3 rodziny): magazyn broni i sukna, przed nią plac ćwiczeń z manekinami. W przydzielonej rodzinie **wojem zostaje mąż** (żona zostaje wolna): bierze włócznię (ze zbrojowni albo prosto z kuźni) i przeszywanicę z sukna (jasnobura tunika), ćwiczy pchnięcia przy manekinach. Gdy rodzina dostanie inną pracę, oddaje broń i sukno do zbrojowni.
- Ludziki: role **Pasterz** i **Woj** (animacja pchnięcia włócznią z wypadem), kamieniarz uogólniony na wyrób budynku (kamień / ruda), rzemieślnik z drugim wsadem; kowal pracuje młotem. Części `SM_Ludz_Nozyce/Welna/Sukno/Ruda/Bron/Wlocznia` w `Tools/ludzie.py`.
- Interfejs: licznik 10 surowców i liczba wojów u góry; okno owczarni (owce, ile z runem), kuźni (co z czego), zbrojowni (woje tej zbrojowni i wszyscy w osadzie). Ikony nowych budynków i surowców (`Tools/ikony.py`).
- `-osada`: przed obozem zbrojownia z trzema wojami, kuźnia, owczarnia ze stadem, chata tkacza i kopalnia rudy (7 rodzin więcej, bez domów); kamera na nich, okno zbrojowni, zakładka Wojsko.
- Panel osady w lewym górnym rogu w trzech wierszach (licznik 10 surowców u góry zachodził na jeden długi wiersz). Zrzut kontrolny: `Saved/zrzut_bron.png` (woje ćwiczą przy manekinach, pasterz strzyże owce, górnicy w kopalni rudy; chata tkacza pominięta w pokazie, bo teren na mapie testowej jej nie pozwolił).

## Ekonomia, etap 5: inne rody, handel i dyplomacja (21.11.2026)
Według planu „Ekonomia gry”: te same reguły dla gracza i rodów AI. Moje odczytanie etapu: dalekie rody liczone są liczbami (bez ludzików) i widać je na mapie papierowej; handel idzie przez targ gracza za grosze; dyplomacja to podarunki, danina i stosunki. Rody blisko kamery z prawdziwymi ludzikami i sama wojna zostają na później (po wojsku).
- `Osada/Rody.h/.cpp` (`ARody`): 8 rodów (Toporczycy, Gryfici, Odrowążowie, Awdańcy, Nałęcze, Pałucy, Leliwici, Starykoniowie), każdy z barwą i cechą (pobożny / kupiecki / wojowniczy), 12–60 km od obozu gracza, na lądzie. Zakładane razem z obozem; zaczynają wcześniej (3–20 minut gry przewagi).
  - Te same budynki (`FRodzajBudynku::DoBudowania`), koszty i wsady co u gracza. Co 2 s gry: produkcja według wydajności budynku (jedna rodzina na budynek), posiłek co 90 s, zimą kłoda na rodzinę, zadowolenie tym samym wzorem, nowe i odchodzące rodziny przy tych samych progach. Woje z broni w zbrojowni.
  - Decyzje: lista priorytetów (jedzenie, drewno, domy, deski, kamień, plac, spichlerz, targ, owce/ruda/kuźnia/zbrojownia, kościół), cecha zmienia kolejność (pobożny wcześniej kościół, kupiecki wcześniej targ, wojowniczy wcześniej wojsko).
  - Poziom osady (te same progi dla gracza): obóz → wieś (8 rodzin, 4 domy) → osada targowa (15 rodzin, targ) → miasto (30 rodzin, targ, kościół).
  - Co rok: lennicy płacą daninę; silny wojowniczy ród (dwa razy większa siła: woje ×3 + rodziny) żąda daniny od gracza. Odmowa (albo brak odpowiedzi przez porę roku): ród wrogi, zrywa handel, grozi wojną.
- **Targ** (nowa zakładka **Handel**): plac z trzema kramami i wagą, magazyn każdego towaru (po 20 sztuk); tragarze znoszą tu towar jak na plac składowy. Ceny według zapasów rodu, odległości i stosunków (kupiecki ród trochę taniej).
- **Grosze**: skarbiec osady (50 na start), licznik u góry ekranu.
- **Okno rodów** (klawisz **H** albo przycisk pod panelem osady): lista rodów (barwa, cecha, poziom, rodziny, km, stosunki, lennik / wrogi / żąda daniny), wybrany ród (siła, grosze, co ostatnio robi), dyplomacja (podarunek 20 groszy, żądanie daniny, zapłać / odmów) i handel na targu: 10 towarów, kup / sprzedaj po 5 sztuk.
- Na mapie papierowej chorągiewki rodów w ich barwach i „Wasza osada” (nazwa, poziom, stosunki).
- `-osada -rody`: targ z towarem, rody po 15 minutach gry, zakup kamienia u najbliższego rodu, okno rodów i targu otwarte. Zrzut kontrolny: `Saved/zrzut_rody.png`.

## Wojna, etap W1: bitwa próbna (21.11.2026)
Projekt całej wojny: Claude Doc „Wojna, bitwy i oblężenia: projekt” (etapy W1–W6). Użytkownik przyjął wszystkie propozycje („Dawaj”): małe armie 20–150 ludzi, pauza ze spacją i zwolnienie przy zderzeniu, pospolite ruszenie + drużyna, władca jako ludzik z następcą, pięć rodzajów wojsk, cztery wybory dla podbitej wsi (głównie przyłączenie z namiestnikiem), palenie wsi tak, egzekucje bez pokazywania; kolejność: W1, potem od razu W3 (obrona osady).
- `Source/grarts2d/Wojna/Bitwa.*` (`ABitwa`, stawia ją `ABudowniczy`): oddziały prawdziwych ludzików. Szyk w 3 szeregach wokół środka oddziału, frontem do celu (zwarty 1,3 m / luźny 2,3 m). Rozkazy na oddział: stój, idź (prawy klik w teren; kilka oddziałów staje obok siebie), atakuj (prawy klik we wroga albo A), szyk F, odwrót R (zrywa walkę, cofa o 25 m, kosztuje ducha). Lewy klik / 1–4 = wybór (Shift = kilka). Spacja = pauza (ludziki zamrożone, rozkazy działają).
- Zwarcie: każdy woj szuka najbliższego wroga (5 m, przy ataku 8 m), podchodzi na 1,75 m i zadaje ciosy co 1,1–1,7 s. Szansa trafienia = oddział (włócznicy 42%, bandyci 36%) + doświadczenie − zmęczenie; z boku +12%, z tyłu +25%, w plecy uciekającego +30%. Włócznik pada po 3 trafieniach, bandyta po 2. Krok symulacji stały 0,1 s.
- Morale (duch) zamiast punktów życia oddziału: polegli, ciosy z boku i z tyłu, zmęczenie powyżej 70% i złamanie sąsiada obniżają; z dala od walki wraca (sufit zależny od strat). Przy 0 oddział ucieka biegiem; 26 m od wrogów raz się zbiera (35%), za drugim razem opuszcza pole. Koniec, gdy jedna strona nie ma oddziału, który stoi: tablica wyniku (polegli, uciekli, czas), Enter zamyka i sprząta pole.
- Pierwsze zderzenie oddziału: 0,9 s czasu rzeczywistego w zwolnieniu ×0,3.
- Prosta SI wroga: po 6 s rusza na najbliższy oddział gracza.
- Interfejs: chorągwie nad oddziałami (barwa, nazwa, liczba, pasek ducha, „ucieka!”), złote kropki pod wojami wybranych oddziałów, panel w lewym dolnym rogu (oddziały: ludzie, duch, zmęczenie, co robią, „bity z boku”) z przyciskami Atak / Stój / Szyk / Odwrót / Pauza.
- `ALudzik`: tryb walki (`WejdzDoBitwy`, `WalkaIdz/Stoj/Cios/Padnij`, stan `Lezy`): bez szukania pracy, szybsze pchnięcie, poległy pada na twarz i leży.
- Bitwa próbna: klawisz **N** w regionie (pod kursorem): 2 × 12 włóczników (czerwone chorągwie) kontra bandyci i zbóje 2 × 14 z toporami (czarne), na karczowanej łące. Wojownicy są tworzeni na czas bitwy, nie z rodzin (to etap W3). Osada w tym czasie żyje dalej (pauza zatrzymuje tylko bitwę).
- `-bitwa` (zrzut: `-game -zrzut -region -bitwa -ui -odl=26000 -czekaj=9`): najrówniejsza łąka w okolicy startu, od razu atak. Zrzut kontrolny: `Saved/zrzut_bitwa.png`.
- **Dalej:** strojenie czucia walki przez użytkownika, potem W3: napady na osadę, ostrokół, wał, brama, wieża, pospolite ruszenie, woje z rodzin.

## Szybsi osadnicy (21.11.2026)
Prośba użytkownika: „za wolno to wszystko się dzieje”. Osadnicy poza bitwą żyją 1,4 raza szybciej (`ALudzik::TempoOsadnikow`): chód, praca, przerwy, animacja i ich wkład w budowę. Bitwa ma swoje prędkości (ABitwa) i tego nie używa.

## Wojna W1: poprawki po uwagach użytkownika (21.11.2026)
Użytkownik: żołnierze nie mogą wchodzić jeden w drugiego, zaznaczanie przeciągnięciem, lepsze animacje, szybsze tempo.
- `ABitwa::Rozepchnij` (co klatkę): pary wojów bliżej niż 0,95 m (swoi) / 1,2 m (wróg) rozsuwają się. W zwarciu woj woli przeciwnika z mniejszą liczbą napastników (+1,6 m kary za każdego), więc walka rozkłada się wzdłuż linii.
- Zaznaczanie ramką: przeciągnięcie lewym (złota ramka), wybiera twoje oddziały z wojami w ramce; w bitwie mapa przesuwa się tylko środkowym (`AKameraMapy::bBezLewegoPrzeciagania`).
- `ALudzik` w walce: postawa bojowa (przysiad, broń przed sobą, oddech), marsz z bronią i bieg z pochyleniem, cięcie toporem znad głowy (bandyci), zachwianie po trafieniu (`WalkaTrafiony`), poległy pada na twarz albo na plecy; cios trwa 0,5 s.
- Tempo: marsz 2,6 m/s (było 1,7), szarża 3,8, ucieczka 4,2; ciosy co 0,75–1,15 s (było 1,1–1,7).
- Zrzut kontrolny: `Saved/zrzut_bitwa.png`.

## Balans osady (21.11.2026)
Uwagi użytkownika: rodziny za szybko jedzą i tracą zadowolenie, budowa (donoszenie materiału) i zbieranie trwają za długo.
- Posiłek co 150 s (było 90 s); zadowolenie spada 0,3 punktu na sekundę (rośnie dalej 1 punkt); rodzina odchodzi po 180 s poniżej 30% (było 120 s).
- Tragarz budowy niesie do 3 sztuk naraz; praca przy budowie liczy się 1,5 raza, bal / kamień pod mur daje 9 s pracy (było 6). Rody AI budują w czasie `CzasBudowy` (było 1,5×).
- Drzewo daje 2 kłody, kosz zbieracza 2 porcje; krótsza praca: drwal 5–7 s, kamieniarz / górnik 5,5–7,5 s, zbieracz 4–5,5 s, myśliwy 5–8 s, pilarz 6–8 s, pasterz 4,5–6 s.

## Tryb deweloperski (21.11.2026)
Prośba użytkownika: tryb kreatywny w GUI — budowa natychmiast, przyspieszanie czasu, wszystkie surowce.
- `Source/grarts2d/UI/PanelDeweloperski.*`: przycisk „Tryb deweloperski (F3)” w prawym dolnym rogu mapy grywalnej (F3 też otwiera/zamyka). Na przycisku widać, gdy czas jest zmieniony albo budowa natychmiast jest włączona.
- Budowa natychmiast (`ABudowa::bNatychmiast`): każdy plac budowy, też już zaczęty, od razu gotowy (materiał liczony jako przyniesiony, nic nie schodzi z zapasów).
- Czas gry: Pauza / x1 / x2 / x5 / x10 — globalne `TimeDilation` świata (`FTrybDeweloperski::Zastosuj` co klatkę z `ABudowniczy::Tick`); kamera i `ABudowniczy` dostają `CustomTimeDilation` = odwrotność, więc sterowanie zostaje w czasie rzeczywistym. W czasie bitwy panel oddaje czas `ABitwa` (x1, jej pauza i zwolnienie). Klawisz T (tempo budowy x10) działa jak dawniej, niezależnie.
- Surowce: +100 / +1000 każdego z 10 towarów i tyle samo groszy (`AOsada::DodajSurowce`) — do ogniska obozu, a bez niego do pierwszego gotowego magazynu.
- Na początku każdej gry wszystko wyłączone (`FTrybDeweloperski::Wyzeruj` w `ABudowniczy::BeginPlay`).

## Pole i pastwisko zaznaczane na terenie (21.11.2026)
Prośba użytkownika: owczarnia jak w Manor Lords (duży teren ogrodzony płotem, owce się tam pasą), pole tak samo — po zaznaczeniu prostokąta widać hektary, gracz potwierdza „Zbuduj”. Wielkość ma zmieniać wydajność.
- Stawianie (`ABudowniczy::StawianieObszaru`): pierwszy klik = róg, drugi klik = przeciwny róg (boki w całych metrach), R obraca o 45°. Nad paskiem budowy panel: wymiary, hektary i m², plon / owce, rodziny do pracy, koszt i długość płotu, „Tu nie można…” gdy teren zły; **Zbuduj (Enter)** / **Anuluj (Esc)**, prawy klik = krok wstecz.
- `FRodzajBudynku::NaObszar(Baza, W, H)`: kopia rodzaju na prostokąt. Pole 10×10 … 60×60 m (bazowe 20×14 m = plon 20, 2 rodziny): plon, orka i siew rosną z powierzchnią (`AOsada::OrkaPola/SiewPola`), rodzin do pracy 2×powierzchnia/bazowa (najwyżej 8), kłody na płot co 30 m. Pastwisko 14×10 … 50×50 m: szopa przy tylnym płocie, owca na każde ~11 m² pastwiska (4–30), pasterzy 1–3, kłody 4 + 1 na 12 m płotu.
- Niedokończony siew (za duże pole, za mało rolników) nie przepada: wschodzi obsiana część (`ABudowa::Zasiano`), plon proporcjonalny.
- Wygląd: płot z segmentów 2 m (`SM_Plot`, `SM_Plot_Palik`) wzdłuż obwodu, każdy na wysokości terenu — rośnie z postępem budowy (najpierw same słupki). Pole to kafle uprawy 5×5 m (`SM_Pole_Kafel_<Ugor|Zaorane|Zasiane|Rosnie|Dojrzale|Scierniska>`) na terenie. `UInstancedStaticMeshComponent`; materiały budynków mają flagę `used_with_instanced_static_meshes`. Owczarnia z `budynki.py` już bez płotu i żerdzi (stawia go gra).
- Kolizje pola i pastwiska z innymi budynkami liczone obróconymi prostokątami (`AOsada::Koliduje(R, Gdzie, Obrot)`), karczowanie kołami wzdłuż dłuższego boku. Spadek terenu dopuszczalny rośnie z wielkością (płot i kafle idą za terenem); szopa owczarni sprawdzana jak zwykły budynek.
- Pokaz: `-osada -obszar` (pole 32×20 m przy żniwach, pastwisko 24×18 m ze stadem, zaznaczone nowe pole 24×16 m z otwartym panelem). Zrzut kontrolny: `Saved/zrzut_obszar.png`.

- 21.11.2026: zwolnienie czasu przy zderzeniu oddziałów usunięte (prośba użytkownika: „bez sensu”).

## Wojna W1: panel oddziałów na dole (21.11.2026)
Prośba użytkownika: GUI dla zaznaczonych oddziałów jak w Total War (zrzut wzoru w czacie). Układ ten sam, wygląd nasz (ciemne drewno, złoto, pikselowe postacie rysowane w kodzie, bez tekstur).
- `Source/grarts2d/Wojna/PanelBitwy.*` (`ZrobPanelBitwy`): na dole na środku trzy części.
  - **Dowództwo** (lewa): duży pikselowy woj wybranego oddziału (hełm, broń, tunika oddziału), nazwa (albo „N oddziały”), ludzie, duch, zmęczenie, stan, stopień (rekruci / wojacy / weterani), szyk.
  - **Drużyna** (środek): karta każdego twojego oddziału: chorągiewka w barwie, postać z włócznią / toporem, numer klawisza, liczba ludzi / początkowo, złote kwadraty doświadczenia, pasek ducha, stan; czerwona ramka = bity z boku, ciemna = poza polem. Klik = wybór (Shift dodaje), podwójny klik = kamera na oddział. Pod kartami przyciski z ikonami: Atak, Stój, Zwarty/Luźny, Odwrót, Pauza (podpowiedzi z klawiszami).
  - **Mapka** (prawa): pole bitwy dopasowane do wojów, siatka co 10 m, kropki (twoi czerwoni, wróg czarny, wybrani złoci, polegli przygaszeni), góra = północ; klik = kamera tam.
- `ABitwa`: publiczne `Wybierz`, `Rozkaz`, `PrzelaczSzyk`, `PrzelaczPauze`, `PokazKamere`, `OpisStanu`; `FOddzial::Tunika`, `bTopor`. Stary panel tekstowy w lewym rogu usunięty.
- Zrzut kontrolny: `Saved/zrzut_bitwa.png`.

## Wojna W1: jeden panel naraz, koniec bez tablicy wyniku (21.11.2026)
- W bitwie pasek budowy jest ukryty (nakładał się na panel bitwy); po bitwie wraca.
- Usunięta tablica wyniku (zatrzymywała grę). Koniec bitwy = krótka wiadomość osady u góry („Bitwa wygrana, bandyci rozbici (polegli: …)”), panel bitwy znika, sterowanie wraca do budowania.
- Uciekający znikają, gdy są poza kadrem i 15 m od wrogów („wracają do swojej osady”, `WasRecentlyRendered`). Po bitwie (`ABitwa::PoBitwie`) po 8 s każdy woj, żywy i poległy, znika, gdy kamera na niego nie patrzy; po 90 s pole sprzątane do końca.

## Wojna: długie zwarcie, linia frontu (21.11.2026)
- Oddziały w zwarciu stoją czołem w czoło (odstęp `Styk` 1,7 m), zamiast zbijać się w kulę. Atak podchodzi do najbliższego brzegu szyku wroga (czoło, bok albo tył) i staje do niego przodem.
- Walczy głównie pierwszy szereg, para na parę; tylni biją tylko tego, kto wszedł między nich. Woj odchodzi od swojego miejsca w szyku najwyżej na krok.
- Stałe miejsca w szyku (`FWojownik::Slot`, `FOddzial::Kol/Szeregi`). Gdy ktoś padnie, `ABitwa::Uloz` przekłada szyk: na miejsce poległego wchodzi ten, kto stał za nim, a szyk zwęża się po stratach.
- Z przodu większość ciosów zatrzymuje tarcza albo zbicie (`Blok` 0,3; zmęczony gorzej się zasłania). Z boku i z tyłu obrona słaba. Ciosy co 1–1,5 s, mniejsza utrata ducha za poległego: równe oddziały biją się około minuty i dłużej.
- Spychanie (`ABitwa::Nacisk`): silniejszy oddział (duch, siły, liczba) powoli przesuwa linię, słabszy cofa się i traci trochę ducha.
- Wróg atakujący w bok zabiera ducha 3/s, w plecy 6/s. Zmęczenie rośnie wolniej (1,1/s), odpoczynek poza walką jak dotąd.
- Chorągiew nad środkiem głębokości oddziału, napis wroga po lewej stronie drzewca, żeby znaki w zwarciu się nie nakładały.

## Wojna: wygląd wojów, krew, życie w szeregach (21.11.2026)
- Nowe modele (`Tools/ludzie.py`, import `Tools/Zbuduj_Ludzi.bat`): `SM_Ludz_Tarcza` (lico w barwie rodu), `SM_Ludz_Helm` (szpiczasty z nosalem), `SM_Ludz_Czapka` (futrzana), `SM_Ludz_Kaptur` (w kolorze tuniki), `SM_Ludz_Broda`, `SM_Ludz_Choragiew`, `SM_Ludz_Kropla`, `SM_Ludz_Plama`.
- `ALudzik::UbierzDoBitwy`: części tworzone dopiero w bitwie (osadnicy ich nie mają). Włócznicy: hełm 55%, czapka 30%, tarcza zawsze. Bandyci: kaptur, czapka albo nic, tarcza 40%. Broda u co drugiego. Tarcza i drzewce nie obracają się z ręką. W doświadczonym oddziale jeden woj ze środka drugiego szeregu niesie chorągiew.
- Zatrzymany cios z przodu: obrońca podnosi tarczę (`WalkaBlok`).
- Krew (`ABitwa::Krew/Plama/KrewTick`, instancje `KropleC`, `PlamyC`): rozprysk przy trafieniu, kałuża pod poległym, krople zostawiają plamy, do 500 plam na bitwę. Śmierć od mocnego ciosu (topór 30%, inna broń 12%) odcina głowę albo rękę (`ALudzik::Odetnij`): część odlatuje i leży na ziemi. Broń, tarcza i chorągiew poległego wypadają i zostają przy ciele.
- Klawisz K w bitwie włącza i wyłącza krew. Ustawienie jest zapisane w `GameUserSettings.ini`, sekcja `[Bitwa]`, klucz `Krew`. Podpowiedź jest w panelu bitwy.
- Życie w szeregach:
  - Każdy woj ma własny rytm oddechu, rozgląda się i przenosi ciężar z nogi na nogę.
  - Co kilkanaście sekund przestępuje kawałek (`FWojownik::Luz`).
  - Przed szarżą oddział wznosi okrzyk z bronią w górze, a tylne szeregi dopingują walczących.
  - Gdy wróg się łamie, pobliskie oddziały wznoszą okrzyk.
  - Zmiany w pierwszym szeregu (`ABitwa::Zmiany`): ranny cofa się, a zdrowszy zza jego pleców zajmuje jego miejsce.
- Znak wroga nad oddziałem ma wyższe drzewce, więc w zwarciu napisy się nie nakładają.

## Wojna: kamera bitwy, pospolite ruszenie, przesuwane panele (21.11.2026)
- Kamera: w czasie bitwy kamerę regionu można zbliżyć do 8000 zamiast 25000 (`AKameraMapy::bBitwaBlisko`, `MinRegionu()`). Po bitwie wraca zwykła granica. Parametr zrzutu `-odl=` działa też poniżej 25000.
- Pospolite ruszenie (`ABitwa::ZacznijNapad`, klawisz P albo parametr `-napad` 6 s po demie osady):
  - Bandyci napadają na osadę. Wszyscy dorośli mężczyźni (`AOsada::Mezczyzni`) rzucają pracę, zbierają się i stają do walki w grupach do 30 (`Ruszenie I`, `II`...).
  - Każdy bierze narzędzie z pracy: drwal siekierę, kamieniarz kilof, rolnik sierp, borowy motykę, reszta młotek (`ALudzik::DoRuszenia`). Walczą słabiej niż woje, tarcza tylko u co siódmego.
  - Bandytów jest około 0,8 liczby chłopów (od 8 do 60).
  - Po bitwie ocaleni wracają do swojej pracy (`ALudzik::WrocZBitwy`), polegli znikają.
- Panele bitwy (dowodzenie, drużyna, mapka) można przeciągać za puste miejsce i zmieniać ich wielkość narożnikiem (0,6–1,8). Podwójne kliknięcie przywraca położenie. Ustawienie jest zapisane w `GameUserSettings.ini`, sekcja `[PanelBitwy]`.

## Wojna: pospolite ruszenie jak w Manor Lords, chód z kolanami (21.11.2026)
- Uwaga gracza: ruszenie pod P było złe (samo się powoływało, w bitwie nie dało się budować), chód sztywny.
- Okno Wojsko (klawisz J albo przycisk pod panelem osady, `Wojna/OknoWojska.*`), wzór: zakładka wojska w Manor Lords.
  - Oddziały ruszenia (`FMilicja` w `ABitwa`): „+ Nowy oddział” bierze do 30 wolnych mężczyzn. Każdy oddział ma przyciski Powołaj, Rozwiąż, Uzupełnij i Usuń, są też Powołaj wszystkie i Rozwiąż wszystkie.
  - Powołaj (`ABitwa::Powolaj`): chłopi rzucają pracę i stają przed obozem, od strony bandytów. Biorą włócznie z zapasów (`AOsada::Wez`, broń z kuźni), reszta idzie z narzędziami. Oddział dostaje kartę w panelu bitwy i steruje się nim jak w bitwie.
  - Rozwiąż (`ABitwa::Rozwiaz`): żywi wracają do swojej pracy, włócznie wracają do zapasów (bez tych zgubionych przez poległych). Oddział, który cały uciekł z pola, rozwiązuje się sam.
- Napad (P, `ABitwa::ZacznijNapad`): przychodzą tylko bandyci (0,8 liczby mężczyzn, od 8 do 60), z ok. 60 m. Przez 16 s zbierają się, potem ruszają. Ruszenia nie powołuje gra, robi to gracz.
  - Gdy nikt nie broni (żaden oddział bliżej niż 70 m), bandyci idą do obozu i plądrują: 35% każdego towaru z budynków i 35% groszy (`AOsada::Grabiez`). Potem odchodzą tam, skąd przyszli, i łup przepada.
  - Rozbici bandyci oddają łup (`AOsada::Zwroc`), na górze ekranu pojawia się wiadomość.
- Budowanie działa dalej przy wojsku w polu. Pasek budowy jest widoczny, a panel bitwy wisi nad nim.
  - Gdy gracz stawia budynek, mysz i klawisze należą do budowania (`ABitwa::bMyszZajeta`).
  - Klawisze A i S nie wydają już rozkazów, bo ruszają kamerą. Atak i stój są w panelu i pod prawym klikiem. Spacja (pauza) działa tylko w czasie walki, R (odwrót) tylko przy wybranym oddziale.
- Pole bitwy żyje dłużej:
  - Po walce powołane ruszenie zostaje, aż gracz je rozwiąże. Woje bitwy próbnej i resztki wroga schodzą.
  - Ciała leżą 2 minuty, potem znikają poza kadrem (`FWojownik::Smierc`, `ABitwa::PoBitwie`).
- Chód (`ALudzik::Animuj`, `ALudzik::UstawNoge`):
  - Noga ma kolano: `SM_Ludz_Udo` i `SM_Ludz_Lydka`, u kobiet `SM_Ludz_UdoK` i `SM_Ludz_LydkaK`. Modele są w `Tools/ludzie.py`.
  - Proste IK stawia stopę na ziemi. Stopa stoi bez ślizgania, pod koniec kroku pięta idzie w górę, potem stopa przenosi się łukiem do przodu.
  - Biodro obniża się w rozkroku, w stójce kolana lekko ugięte. Tempo kroku rośnie z prędkością, a każdy ludzik chodzi trochę innym krokiem (`Rytm`).
  - Ręce wahają się według stóp, barki skręcają się przeciw biodrom, ciało przenosi ciężar na nogę, a głowa patrzy prosto. Ruszanie i zatrzymanie przechodzą płynnie (`WagaChodu`).
  - Przysiad, wypad i rozkrok w walce zginają kolana zamiast unosić stopę.
- Zrzut kontrolny: `-osada -napad -odl=11000` (napad 6 s po pokazie, jeden oddział powołany, okno Wojsko otwarte).

## Wojna W2: rodzaje wojsk pieszych i strzelcy (21.11.2026)
- Tabela rodzajów w `Wojna/RodzajeWojsk.h` (`ERodzajWojska`, `RodzajWojska()`). Każdy rodzaj ma broń, trafienie, pancerz, zdrowie, tarczę (`Blok`, czyli ile ciosów z przodu przechodzi), tempo ciosów, przebicie pancerza, szansę odcięcia i duch. Do tego wygląd: szanse na hełm, czapkę, kaptur i tarczę oraz kolor tuniki (u miecznika kolczuga jest szara).
  - Włócznicy: równi we wszystkim.
  - Pałkarze: maczuga bije przez pancerz, ale słabo się zasłaniają i nie mają tarcz.
  - Tarczownicy z toporem: mur tarcz (przechodzi tylko 18% ciosów z przodu), często tną głowy.
  - Miecznicy z tarczą: kolczuga, szybsze ciosy (×0,7), 3,5 zdrowia.
  - Łucznicy: zasięg 46 m, strzał co 2,6 s, 24 strzały. Wręcz są słabi.
  - Kusznicy: zasięg 50 m, strzał co 5,5 s, 16 bełtów. Bełt przebija pancerz, rani za 1,5 i częściej przechodzi przez tarczę.
  - Ruszenie i bandyci działają jak dotąd.
- `ABitwa::DodajOddzial(ERodzajWojska, ...)` bierze wartości z tabeli. Zwarcie liczy blok tarczy, tempo, przebicie i cięcie według rodzaju. Śmierć i rany są wspólne dla ciosów i strzał (`ABitwa::Rana`, `ABitwa::Zabij`).
- Strzelanie (`ABitwa::Strzelanie`, `ABitwa::PociskiTick`):
  - Strzelcy z rozkazem ataku podchodzą na zasięg i stają. Bez rozkazu sami strzelają do najbliższego wroga w zasięgu. W marszu nie strzelają, a gdy wróg wejdzie w zwarcie, walczą wręcz.
  - Strzała leci łukiem, bełt prawie płasko i szybciej. Rozrzut rośnie z odległością.
  - Na końcu lotu najbliższy wróg w promieniu 75 cm może oberwać. Tarcza od przodu zatrzymuje 70% strzał i 40% bełtów, pancerz też chroni. Świst strzał zabiera trochę ducha.
  - Chybione strzały zostają wbite w ziemię, do 600 na bitwę. Gdy skończą się strzały, oddział walczy wręcz.
- Wygląd:
  - Nowe modele `SM_Ludz_Miecz`, `SM_Ludz_Maczuga`, `SM_Ludz_Kusza`, `SM_Ludz_Strzala` i `SM_Ludz_Belt` (`Tools/ludzie.py`).
  - Łucznik naciąga cięciwę do twarzy i puszcza, a łuk w dłoni zawsze stoi pionowo. Kusznik celuje z ramienia, po strzale kusza odrzuca.
  - Na kartach i w portrecie panelu widać broń danego rodzaju. Portret strzelców pokazuje, ile zostało strzał lub bełtów.
- Bitwa próbna (N, `-bitwa`): twoi włócznicy, miecznicy i łucznicy przeciw tarczownikom, pałkarzom i kusznikom.
- Dalej w W2: konni (lekko- i ciężkozbrojni), ciężkozbrojni i bracia zakonni. Do tego towary i budynki do rodzajów wojsk (skóra, tarcza, łuk, strzały, kolczuga, koń) oraz premie z terenu.

## Dolny pasek jak w Manor Lords (21.11.2026)
Prośba użytkownika (wzór: zrzut paska z Manor Lords): zakładki jak tam, a klik gdzieś na trawę zamyka wszystkie okna.
- `UI/PasekBudowy` od nowa: na dole rząd głównych przycisków ze znakami — **Budowa (B)**, **Wojsko (J)**, **Rody (H)** (czerwone „!” przy napadzie bandytów albo żądaniu daniny; złota ramka, gdy okno jest otwarte). Budowa otwiera nad nim pasek kategorii (znaki: dom, siekiera, chleb, snop, kowadło, tarcza, waga, krzyż; podpowiedź z nazwą, z lewej nazwa otwartej kategorii), a nad paskiem karty budynków: pergamin z obrazkiem budynku, nazwą, kosztem i znakiem kategorii w kółku na dolnej krawędzi; wybrana karta w złotej ramce.
- Przyciski „Rody i handel (H)” i „Wojsko (J)” z lewego górnego rogu przeniesione na dolny pasek (okna otwierają się tam, gdzie dotąd).
- Klik w trawę (nie w budynek, gdy nic nie stawiasz) zamyka okno budynku, okno rodów, okno wojska i karty budowy. Klik idzie dalej do `ABitwa` (zaznaczanie oddziałów w polu).
- Znaki: `Tools/ikony.py` → `T_Ikona_Znak_<Budowa|Wojsko|Rody|Mieszkalne|Surowce|Jedzenie|Rolnictwo|Rzemioslo|Wojsko_Kat|Handel|Religia>` (32×32 pikseli, jasny pergamin z ciemnym obrysem).

## Grafika: korekta obrazu mapy grywalnej (21.11.2026)
Prośba użytkownika „poprawić animacje, tekstury, shadery, grafikę, wodę i całość”. Pierwszy tani krok, tylko `ASwiatGameMode::StartPlay` (post-process `Korekta`, działa tylko w regionie):
- SSAO mocniej (intensywność 0,8, promień 150 cm): cień kontaktowy pod domami, płotami, ludźmi.
- Ciepłe światła i chłodniejsze cienie (`ColorGainHighlights` 1,05/1,01/0,93, `ColorGainShadows` 0,95/0,99/1,06), kontrast 1,06.
- Winieta 0,3.
Cofnięcie: usunąć ten blok w `StartPlay`. Dalszy plan grafiki (woda, wiatr w drzewach, cienie chmur, AA pikseli) w wątku „Poprawić grafikę, animacje i wodę”.

## Grafika: woda na mapie grywalnej (21.11.2026)
Plan grafiki, krok 1 (użytkownik: „działaj”). Tylko region, mapa Europy bez zmian.
- Nowa tekstura `Art/Teren/T_WodaFale.png` (512², kafelkująca mapa normalnych): `Tools/tekstury_teren.py fale`. Import jako `TC_NORMALMAP` w `Tools/ue_teren.py`.
- `M_Teren`: normalna w przestrzeni świata (`tangent_space_normal = False`). Ląd dostaje normalną wierzchołka, jak dotąd. Woda dostaje dwie warstwy fal: drobne (kafel 16 m) i długie (44 m), płynące w różne strony, siła 0,3.
- Szorstkość wody w regionie 0,07 zamiast 0,3, więc widać blask słońca i odbicie nieba (SSR).
- Ruchoma piana przy brzegu: pas ~2–3 m, plamy szumu `T_Trawa.G` płyną z prądem, krycie 0,55.
- Wymaga `Tools\Zbuduj_Teren.bat` (edytor zamknięty). Strojenie: siła fal `n_sila` (0,3), szorstkość `szorstkosc_wody` (0,07), piana `piana_al2` (0,55).

## Grafika: wiatr, cienie chmur, spokojniejsza trawa (21.11.2026)
Plan grafiki, kroki 2–4 (użytkownik: „działaj” na cały plan).
- **Wiatr** (`Tools/ue_natura.py`, `KOD_WIATRU`, `wiatr()`): WPO w jednym węźle Custom. Całe drzewo kołysze się z wysokością (h^1,5, 25 cm na 10 m), faza losowa na drzewo, podmuchy idą falą po lesie (kierunek 0,8/0,6). Liście dodatkowo drgają (5 cm). Pień ma to samo kołysanie, bez drgań, więc się nie łamie. Kępki trawy mają nowy materiał `M_Natura_Trawa` (dwustronny, drgania źdźbeł 9 cm). Wymaga `Tools\Zbuduj_Nature.bat`.
  - `TerenKafle.cpp`: `WorldPositionOffsetDisableDistance = 60000` na ISM drzew, czyli wiatr działa tylko do 600 m od kamery (wydajność Nanite + VSM).
- **Cienie chmur**: funkcja światła `M_ChmuryCien` (`Tools/ue_teren.py`, `material_chmur`). Dwie próbki `T_Detal` (plamy 0,5–2 km) płyną z wiatrem, w cieniu słońce ma 55%. Podpina ją `ASwiatGameMode::UstawSwiatloRegionu`, tylko w regionie. Brak assetu = brak chmur. Strojenie: próg `0.74–0.88` (pokrycie), `0.55` (siła cienia).
- **Trawa**: kontrast źdźbeł `T_Trawa` w `M_Teren` mniejszy (0,8–1,2 i 0,9–1,1 zamiast 0,7–1,3 i 0,85–1,15), mniej plam „w kratkę”.
- Wymaga `Kompiluj.bat`, `Zbuduj_Teren.bat` i `Zbuduj_Nature.bat`.
- **Jaśniejsze korony i trawa** (21.11.2026, `ue_natura.py`, `jasniej()`): korony i kępki trawy wychodziły prawie czarne. Liście: jasność 2,6–3,4 (było 2,0–2,8), model cieniowania Two Sided Foliage (prześwit 0,7 koloru), lekka emisja 0,10 koloru. Trawa: jasność 1,7–2,1 (było 1,2–1,6), Two Sided Foliage 0,6, emisja 0,12. Kora bez zmian. Materiały natury mają też `used_with_nanite` (bez tego gra podstawiała materiał domyślny).
- **Mocniej** (21.11.2026 wieczorem, użytkownik: „nic się nie zmieniło”): fale 0,3 → 0,55, piana 0,55 → 0,75, cienie chmur gęstsze (próg 0,66–0,80) i ciemniejsze (słońce 40%), kołysanie drzew 25 → 45 cm na 10 m, drgania liści 5 → 9 cm, trawy 9 → 14 cm.
- **Noc 21.11.2026** (użytkownik: woda super, ale białe plamki i słaba ziemia): usunięte białe kropki na wodzie (`f_al2` = 0) i białe „kamyki” na brzegu (`k_alfa` = 0). Sucha trawa bardziej oliwkowa (106,112,60 zamiast brązowej 118,108,62), przejście świeża/sucha miękkie (0,66–0,92).
- „Postaw wszystko” (druga prośba użytkownika, 21.11.2026: jeden przycisk do sprawdzania wszystkiego naraz): `ABudowniczy::PostawWszystko` — obóz przy kamerze (jeśli go nie ma), każdy budynek z paska gotowy w pierścieniach co 7 m wokół obozu (do ok. 230 m, tartak wodny szuka rzeki), nowe rodziny przy ognisku tyle, ile miejsc pracy, wszystkie od razu w pracy, +100 surowców. Drugi klik nie stawia drugi raz tego samego. Zrzut: `-game -zrzut -region -wszystko -ui -odl=14000 -czekaj=8` (22 budynki, 15 rodzin, `Saved/zrzut_wszystko.png`).

## Drogi, obrót 360°, lepsza kolizja (22.11.2026)

Prośba użytkownika (21.11.2026 w nocy): drogi jak w Manor Lords (darmowe, z zaokrągleniami), budynki przyciągane do drogi, ludzie wybierają drogę i mają na niej +5–10% prędkości, lepsza kolizja przy stawianiu, budynki w każdym kierunku 360°, kamera tak samo.

- **Drogi** (`Osada/Drogi.h/.cpp`, `ADrogi`, spawnuje `ABudowniczy`): klawisz **G** albo przycisk „Drogi (G)” na dolnym pasku. Klik = początek, kolejne kliki = następne punkty; odcinek wygina się łukiem (Bezier 2. stopnia z kierunku poprzedniego), Shift = prosto, prawy klik = koniec ciągu, drugi prawy klik / Esc / G = wyjście, Ctrl + klik = usuń odcinek. Początek/koniec przyciąga się do końców i punktów dróg obok (skrzyżowania). Nie wolno: woda, rzeka (3 m), stok > 45%, budynek. Droga karczuje drzewa i trawę. Szerokość 3,2 m (`Polszerokosc` = 160 cm).
- **Wygląd**: `UProceduralMeshComponent` 14 cm nad terenem, 5 wierzchołków w poprzek (koleiny ciemniejsze), okrągłe końce (bez szpar na zakrętach i skrzyżowaniach). Materiał `M_Droga` (`Tools/drogi.py` + `Tools/ue_drogi.py`, uruchamia `Tools\Zbuduj_Drogi.bat`): `T_Droga` × kolor wierzchołka × 1,7; maska = alfa wierzchołka + szum tekstury → poszarpany pikselowy brzeg. Bez materiału: jednolity brąz (BasicShapeMaterial).
- **Przyciąganie budynków**: przy drodze (do 6 m od obrysu) budynek staje frontem (+Y) do drogi, tuż przy niej (`ADrogi::Przyciagnij`). Alt = bez przyciągania. Budynek nie może stanąć na drodze (`PrzecinaBudynek`, też pole i pastwisko).
- **Ludziki**: graf dróg (węzły co 2,5 m, skrzyżowania = węzły różnych odcinków bliżej niż 2 m). `ALudzik::IdzDo` liczy trasę (Dijkstra, `ADrogi::Trasa`) dla celów dalej niż 25 m: idzie drogą, jeśli dojście + droga/1,1 + zejście jest krótsze niż prosto. Na drodze prędkość × 1,1 (`ADrogi::Premia`, `ALudzik::NaDrodze` w `PredkoscChodu`).
- **Obrót budynków 360°** (`ABudowniczy::ObrocKlawiszem`): R krótko = następne 45°, R przytrzymane = płynnie 120°/s; F to samo w drugą stronę. Też dla pola i pastwiska.
- **Kamera 360°**: Q/E obracają płynnie także na mapie grywalnej (wcześniej skokami 90°), środkowy przycisk przeciągany w bok obraca kamerę (w bitwie środkowy dalej przesuwa mapę).
- **Kolizja budynków** (`AOsada::Koliduje`): obrócone prostokąty obrysu (`Polowa`) z 0,5 m zapasu zamiast kół `Promien` (koła nie pasowały do prostokątnych domów).
- Zrzut kontrolny: `-game -zrzut -region -osada -drogi -ui` (kręta droga ~65 m od obozu z domami przy niej).
- **Dopasowanie do dróg** (23.11.2026, prośba: „drogi powinny się dopasowywać do drogi, drogę lepiej stawiać”): łuk to krzywa Béziera 3. stopnia z warunkiem na obu końcach (`ADrogi::Krzywa`). Przedłużenie końca drogi wychodzi i wchodzi stycznie, a początek lub koniec w środku innej drogi wychodzi z niej lub wchodzi w nią pod kątem prostym (`PrzyciagnijKoniec` zwraca tryb: wolny punkt, koniec drogi, środek drogi). Klik na innej drodze łączy i kończy ciąg. Wjazd w środek drogi poszerza wstęgę lejkiem do 1,9 szerokości na ok. 5 m (`WchodziWDroge`), zamiast kończyć ją kołem. Wstęga jest gęściej próbkowana (środki z Catmulla-Roma), więc zakręty są gładsze. Każdy odcinek leży na innej wysokości (14 cm + 1,5 cm × (nr mod 6)), a koła końców 0,7 cm niżej, więc nachodzące drogi i łączenia nie migają. Test `grarts2d.Drogi.Luk`.
- **Wygląd dróg** (23.11.2026, „może coś z grafik?”): `T_Droga` 512 px (4 m) z miękkimi plamami ubitej ziemi, ciepłymi i szarawymi odcieniami, zielonkawym nalotem, pęknięciami i kamykami ze światłem i cieniem. W alfie jest szum brzegu (fale ok. 1 m + drobne piksele), a `M_Droga` ma maskę alfa wierzchołka + (alfa tekstury − 0,5) × 0,9, więc brzeg jest falisty, nie tylko poszarpany. Wstęga ma 7 kolumn: brzeg wpadający w zieleń, pobocze, dwie ciemne koleiny (rozstaw 1,6 m) i zielonkawy pas trawy pośrodku. Wzdłuż brzegów rosną kępki trawy (`SM_Trawa_1/2`, skala 0,35–0,65), rzadko leży kamień (`SM_Kamien_1`), a między koleinami są pojedyncze niskie kępki (`ADrogi::PosadzKepki`, instancje bez cienia, położenia z hasza punktu, więc te same po każdej przebudowie; bez kępek na innej drodze i pod budynkiem). Wymaga `Tools\Zbuduj_Drogi.bat` (zrobione) i `Kompiluj.bat`. Ten sam materiał ma ubita ziemia osad rodów.

## Rozbiórka budynków (23.11.2026, pytanie: „da się niszczyć budynki?”)
- Okno budynku: przycisk „Rozbierz” na dole (drugi klik potwierdza: „Na pewno? Kliknij jeszcze raz”). Nie ma go przy ognisku i namiotach.
- `AOsada::Rozbierz`: gotowy budynek oddaje zapasy i połowę kosztu, plac budowy cały przyniesiony materiał, a przebudowa najpierw cofa się (`AnulujUlepszenie`, materiał przebudowy w całości). Zwrot idzie do ogniska albo magazynu (`Zwroc`, po zniknięciu budynku, więc rozbierany magazyn sam siebie nie wybierze).
- Ludzie: rodziny tracą tam pracę (wolne do przydziału), mieszkańcy domu czekają przy obozie i wprowadzają się do najbliższego wolnego domu (`Wprowadzaj` bierze teraz też rodziny bez domu i bez namiotu). Budowniczy i pomocnicy wracają do swoich zajęć (`ALudzik::Odwolaj`), a kurs z tego budynku jest przerywany (towar w rękach wraca do niego przed zwrotem). Woły same porzucają kurs (`AWol::Porzuc`).
- Nie sprawdzone w grze (tylko kompilacja i testy bez okna).


## Poziomy domu i kościoła, warianty domów (22.11.2026)

Prośba użytkownika: trzy domy zostają, ale jako poziomy ulepszania jednego domu; każdy poziom ma kilka lekko losowych wariantów z kominami; kościół też w trzech poziomach (drewniany, kamienny, okazały z placem).

- **Dom**: poziom 1 = chata z bali (8 kłód, jedyny dom na pasku budowy), 2 = dom z muru pruskiego, 3 = duży dom (2 rodziny). Koszt ulepszenia = koszt następnego poziomu.
- **Kościół**: poziom 1 = `SM_KosciolDrewniany` (zrąb, wieża-dzwonnica z izbicą, soboty dookoła nawy; 16 kłód + 12 desek), 2 = dotychczasowy kamienny `SM_Kosciol`, 3 = `SM_KosciolWielki` (kaplica od frontu, brukowany plac, mur z bramą, studnia, lipy, figura). Zadowolenie z kościoła 12 / 16 / 20 punktów wg poziomu.
- **Warianty domów** (`Tools/budynki.py`: `CHATA_W`, `PRUSKI_W`, `DUZY_W`, `LUSTRO`): 4 na poziom, różnią się wymiarami, dachem (strzecha / gont, odcienie), kolorem tynku, kominem (gliniany, kamienny, ceglany, zewnętrzny przy szczycie) i dodatkami (szopka z tyłu, ganek, ławka, beczka, płotek). Warianty 1 i 3 są odbite w X. Modele `SM_<Dom>_F<n>` (wariant 0) i `SM_<Dom>_W<w>_F<n>`. Gra losuje wariant przy stawianiu (`ABudowa::WariantB`), wejście i sterta odbijane w `ABudowa::OdbijWariant`.
- **Ulepszanie**: okno budynku, sekcja POZIOM, przycisk „Ulepsz” (`SOknoBudynku::CzescUlepszenia`). `AOsada::Ulepsz` sprawdza miejsce dla większego obrysu (`Koliduje(..., Pomin)`), karczuje, a `ABudowa::Ulepsz` przełącza ten sam budynek na nowy rodzaj: modele `SM_<Budynek>[_W<w>]_U` (stary budynek w rusztowaniu + materiał), potem F(n-1) i gotowy. Rodzina mieszka dalej, budowniczy i tragarze przychodzą jak na zwykłą budowę.
- `FRodzajBudynku`: nowe pola `Poziom`, `Wariantow`, `Ulepszenie` (funkcja następnego poziomu), `Religia()`.
- Wymaga `Kompiluj.bat` i `Tools\Zbuduj_Budynki.bat` (nowe modele i ikona `T_Ikona_KosciolDrewniany`).

## Chata drwala i chata borowego z poziomami, pierwsza chata drwala za darmo (22.11.2026)
- Pierwsza chata drwala w osadzie (też u rodów) bez materiału: `FRodzajBudynku::bPierwszyDarmowy`, `AOsada::Darmowy` (ta sama Funkcja i Wyrob) w `Postaw` zdejmuje koszt (`ABudowa::BezKosztu`). Chata (dom poziom 1) kosztuje 4 kłody zamiast 8.
- Chata drwala: 2 rodziny od startu. Poziom 2 "Chata drwala z kozłami" (4 kłody, 4 deski): zasięg 50 m, pojemność 14. Poziom 3 "Zagroda drwali" (6/6/4): 3 rodziny, zasięg 55 m, pojemność 18. Pilarnia i tartak wodny bez zmian.
- Chata borowego: poziom 2 "Chata leśnika" (6/4/0) zasięg 45 m, poziom 3 "Leśniczówka" (6/6/4) zasięg 50 m i 2 rodziny.
- Modele: `Tools/budynki.py` `chata_drwala(faza, poziom)`, `chata_borowego(faza, poziom)`, `przebudowa_prosta` (SM_ChataDrwala2/3, SM_ChataBorowego2/3 i ich _U). Pokaz: `-poziomy -pilarnie` (kamera na rzędy drwali i borowych).- Poprawka 22.11.2026 (użytkownik: „nadal nie buduje od razu”): `ABudowa` ma Tick wyłączony (włącza go tylko część ruchoma), więc „Budowa natychmiast” kończy teraz place budowy w `FTrybDeweloperski::Zastosuj` (co klatkę, `TActorIterator<ABudowa>`, `ABudowa::Podglad()` pomija podgląd). Opcja `-natychmiast` włącza ją od startu (zrzut: `-osada -natychmiast`, w logu „budowa natychmiast - Dom z muru pruskiego gotowy”).


## Podłoża budynków na terenie (22.11.2026)
- Zrzut użytkownika: place z ziemią (kamieniołom, targ i inne) były płaskimi, kanciastymi płytami w modelu, nie przylegały do terenu i ostro odcinały się od trawy.
- `Tools/budynki.py`: w 8 budynkach wyłączone `klepisko(...)` (pilarnia, plac składowy, kamieniołom, chata tkacza, kopalnia rudy, kuźnia, zbrojownia, targ). Pola, owczarnia i szkółka bez zmian.
- `FRodzajBudynku::Klepisko` (kolor; A > 0 = jest ziemia) w tych 8 rodzajach; kamieniołom szarawy, kopalnia rdzawa.
- `ABudowa::Podloze`: siatka `UProceduralMeshComponent` co ok. 1 m pod obrysem (`Polowa` − 0,5 m, zaokrąglone rogi) leży na terenie +10 cm. Gdzie teren opada poniżej budynku, robi nasyp do poziomu budynku i łagodną skarpę (4 m) w dół do terenu. Brzeg rwie się w trawę jak na drodze: materiał `M_Droga` (alfa wierzchołków + szum). Budowana w `Ustaw` i `Ulepsz`, nie dla podglądu ani pola/pastwiska.
- Wymaga `Kompiluj.bat` i `Tools\Zbuduj_Budynki.bat` (modele bez płyt).

## Pole leży na terenie, okno pola z postępem (22.11.2026)
- Zrzut użytkownika: pola z kafli 5 × 5 m były płaskimi blokami nad terenem i kanciastymi prostokątami.
- `ABudowa::Lan` (`UProceduralMeshComponent`, materiał `M_Bud_Tynk` z kolorem wierzchołków) zamiast kafli `SM_Pole_Kafel_*` (modele zostają w `budynki.py`, gra ich już nie używa). `ZbudujLan`: siatka co 1 m wzdłuż rzędów i 25 cm w poprzek, każdy wierzchołek na wysokości terenu +6 cm; rzędy co 50 cm (bruzda / grzbiet), łan podniesiony według uprawy (dojrzałe ~55 cm + kłosy), kępy z szumu. Brzeg: prostokąt z zaokrąglonymi rogami poszarpany szumem Perlina, łan przy brzegu opada i przechodzi w kolor miedzy. Kolory jak w `budynki.py`, zamienione na liniowe (sRGB → liniowe, inaczej blade).
- Latem łan rośnie: `AOsada` co 0,5 s ustawia `ABudowa::Wzrost` = ułamek lata (`UstawWzrost`, przebudowa co 10%): wyżej i z zielonego w żółtawy.
- Okno pola: pasek „Wzrost zboża”, „Żniwa za: X min Y s” (do początku jesieni, `AOsada::DoZniw`) albo „Żniwa trwają: zostało … do zimy”, „Spodziewany zbiór” (`AOsada::SpodziewanyPlon`: dojrzałe = snopy na pniu, obsiane = plon × obsiana część, wiosną i poza sezonem pełny plon, ugór latem 0). Dawny „Plon” to teraz „Pełny plon”.
- Zrzut kontrolny: `-game -zrzut -region -ui -osada -obszar`.
- Poprawka (użytkownik: „powinny być 3D roślinki”): zboże w 3D. `Tools/budynki.py` `zboze(rodzaj)`: kępa źdźbeł na 1 m rzędu (`SM_Zboze_Zielone` z liśćmi, `SM_Zboze_Dojrzale` z kłosami, `SM_Zboze_Scierniska`). `ABudowa::Zboze` (InstancedStaticMesh, bez cienia, bo gęsty łan sam się zacieniał na czarno) stawia kępę na każdym metrze grzbietu rzędu, na wysokości terenu; wysokość: obsiane 0,12, latem 0,2→0,95 według `Wzrost`, dojrzałe 1, przy brzegu niższe. Łan pod roślinami to sama ziemia w rzędach. Ugór i zaorane bez roślin.

## Spokojny obraz bez migotania, ludzie omijają budynki (22.11.2026)
Użytkownika bolała głowa od migotania pikseli przy ruchu kamery. Druga prośba: ludzie chodzili przez budynki.
- **Antyaliasing z pikselami** (`KameraMapy::UstawPiksele`, `Config/DefaultEngine.ini`): TSR włączony zawsze. Efekt pikseli robi teraz drugie skalowanie: `r.ScreenPercentage=100`, `r.SecondaryScreenPercentage.GameViewport=50` (poza pikselami 0), `r.Upscale.Quality=0`. TSR wygładza obraz w 50% rozdzielczości, a najbliższy sąsiad powiększa go ×2, więc piksele zostają, a drzewa, trawa i krawędzie nie migają. Do tego `r.MaxAnisotropy=8`, `r.TSR.ShadingRejection.Flickering=1`, `r.Shadow.Virtual.ResolutionLodBiasDirectionalMoving=1.0`. Cofnięcie: w `UstawPiksele` wrócić do ScreenPercentage 50 i AA 0.
- **Kamera**: WASD rusza i hamuje łagodnie (`RuchGladki`, ok. 0,15 s).
- **Okna UI** (budynek, rody, wojsko) pojawiają się płynnie w 0,15 s (`PokazPlynnie` w `Budowniczy.cpp`).
- **Omijanie budynków** (`OminBudynki` w `Ludzik.cpp`, wołane z `ALudzik::IdzDo` poza bitwą): przeszkodą jest obrys każdego budynku (`Obrys()` + 25 cm), bez pól i pastwisk i bez podglądu. Cel w gotowym budynku = najpierw drzwi (`Wejscie`, wypchnięte za obrys), start w budynku = najpierw wyjście drzwiami. Odcinek trasy, który przecina budynek, obchodzi go najkrótszym narożnikiem (zachłannie, do 24 narożników). Działa też na trasach po drogach (premia +10% bez zmian). Test `grarts2d.Ludzie.Przecina`.
- Skrót: to nie jest pełne szukanie drogi; w bardzo gęstej zabudowie objazd może być dłuższy, a ciasne przejścia (0,5 m między domami) są traktowane jak przejście.
- **Poprawka (22.11.2026, użytkownik: „za bardzo rozmazana”)**: bez pikseli 50%. Pełna rozdzielczość z TSR (`r.SecondaryScreenPercentage.GameViewport=0`) i wyostrzenie `r.Tonemapper.Sharpen=0.8`. Powrót do pikseli: w `UstawPiksele` ustawić `bPiksele ? 50.f : 0.f`.

## Wojna: szyki przeciąganiem, marsz w szyku, łokcie, filmowa walka (22.11.2026)
Prośba użytkownika: rozstawianie szyków jak w Total War / Manor Lords, poruszanie wojsk, animacje walki, sztywne ręce; potem: „rigowanie lepsze”, blok tarczą „cinematic, nie cały czas, żeby nie obciążać”, realistyczniejsza krew i reakcja na strzały.
- **Szyk przeciąganiem** (`ABitwa::Sterowanie`, `RozstawNaLinii`, `WydajMarsz`): prawy przycisk przytrzymany i przeciągnięty = linia nowego szyku. Długość = szerokość, front prostopadle (ciągnięcie w prawo ekranu = front od kamery). Podgląd: jasne kropki miejsc wojów i złota strzałka frontu (`PodgladSzyku`, `SZnakiBitwy`). Kilka oddziałów dzieli linię według liczby ludzi, odstęp 2,5 m, kolejno jak stoją (bez krzyżowania). Najwyżej 12 szeregów. Zwykły klik jak dawniej (obecne szerokości, front w stronę marszu). Podwójny prawy klik = biegiem (`bBiegiem`, szybciej, zmęczenie rośnie).
- **Szerokość gracza zostaje** (`FOddzial::KolGracza`, `Uloz`): straty zmniejszają głębokość, nie szerokość.
- **Marsz w szyku** (`RuchOddzialow`, `Obroc`): długi marsz (> 12 m) obraca szyk płynnie w stronę marszu (skrzydła nie szybciej niż marsz) i idzie wolniej, dopóki nie stanie frontem; krótki od razu do frontu celu; po dojściu obrót do frontu wskazanego przez gracza (`FrontCelu`, `bDoFrontu`). Zwrot o ponad 120° = „w tył zwrot”: tylny szereg staje się przednim (też odwrót R). Kilka oddziałów idzie tempem najwolniejszego (`TempoGrupy`). Środek oddziału omija budynki z zapasem pół szerokości (`ALudzik::OminBudynkiTrasa`, `FOddzial::Trasa`).
- **Szkielet ludzika** (`Tools/ludzie.py`, `ALudzik`): `SM_Ludz_Biodra` + `SM_Ludz_Klatka` (klatka na pasie skręca się i pochyla osobno), `SM_Ludz_Ramie` + `SM_Ludz_Przedramie` (łokieć). Narzędzie, tarcza i chorągiew w dłoni przedramienia (`PrzedL/PrzedP`). Kobiety: suknia w całości na biodrach, klatka bez modelu. `SM_Ludz_Tulow` i `SM_Ludz_Reka` nieużywane (Content je jeszcze ma do następnego `Zbuduj_Ludzi`).
- **Animacje** (`ALudzik::Animuj`): RL/RP to dalej kąt przedramienia (narzędzia), łokieć sam z kąta (wyżej = mocniej zgięty), więc wszystkie stare animacje pracy dostały łokcie bez przepisywania. Walka: miecz tnie na zmianę z prawej i z lewej (przedramię w bok, skręt klatki), topór/maczuga z łokciem jak bicz, włócznia prostuje rękę przy pchnięciu, łucznik zgina łokieć przy naciągu. Tarcza przy ciele, przy bloku w górę i przez ciało; bez tarczy zbicie bronią. Tarcza i chorągiew trzymane pionowo w świecie (`SetWorldRotation`).
- **Cios w drodze** (`ABitwa::FCios`, `CiosyTick`): skutek liczony przy zamachu, widoczny w chwili uderzenia (`ALudzik::DoUderzenia` = 0,26 s); obrońca podnosi tarczę 0,16 s wcześniej, przy bloku obaj drgają (`WalkaWstrzas`) i lecą iskry (`SM_Ludz_Iskra`, `Iskry/IskryTick`).
- **Filmowe pary** (`ABitwa::Filmowe`, co 3 s): do 4 par w zwarciu najbliżej środka ekranu: szybsza wymiana ciosów, kontra zaraz po bloku, więcej iskier, animacja co klatkę. **Oszczędność**: ludzik poza kadrem animuje się co 4. klatkę, dalej niż 180 m od kamery co 2. (`ALudzik::Tick`, `DtAnim`).
- **Trafienia i rany**: zachwianie w bok od ciosu; strzała zostaje w ciele albo w tarczy (`WbijStrzale`, do 5), trafiony strzałą zgina się i łapie za ranę, co trzeci przyklęka. Ranny (`UstawRane`): kuleje, garbi się, wolna ręka przy boku, krew na tunice (gdy K włączone), 15% wolniej, trafia o 6% gorzej.
- **Padanie** (`WalkaPadnij(Kier, bStrzala, bKona)`): w stronę ciosu; prosto, na kolana i na twarz (częściej po strzale), z obrotem, odrzut do tyłu. Co czwarty konający rusza jeszcze ręką i głową kilka sekund. Z kikuta krew tryska 2–3 s (`Tryski`, `ALudzik::Kikut`).
- Nie zrobione: krew na broni (materiał broni nie ma maski koloru).
- Wymaga `Kompiluj.bat` i `Tools\Zbuduj_Ludzi.bat`. Zrzut kontrolny: `-zrzut -region -bitwa -ui -odl=9000 -czekaj=14` → `Saved/zrzut_bitwa.png`.
- Poprawka (22.11.2026, użytkownik: „miecze leżą w poziomie”): nadgarstek miecza (`Nadg` w `Animuj`, obrót `Mlotek` na przedramieniu): w postawie i marszu klinga prawie pionowo (+24°), w zamachu cofnięta za bark, w cięciu prostuje się z ręką. Postawa miecznika 62° zamiast 55°.
- Poprawka 2: miecz obrócony o 90° wokół klingi (`Mlotek` roll 90), ostrzem do wroga, a nie płazem.
- Napór tylnych szeregów (22.11.2026, użytkownik: „jednostki z tyłu mogą dopychać linię, domykać”): w walce (`O.bWalczy`) `ABitwa::DoSzyku` stawia woja z dalszego szeregu tuż za plecami żywego woja przed nim w tej samej kolumnie (`Przestrzen + 8` cm za nim, o ile ten jest bliżej niż 260 cm od swojego miejsca), więc szereg idzie za pierwszym, gdy ten się przesuwa. `ALudzik::WalkaNapor(bool)` włącza płynnie (`NaporW`) postawę naporu w postawie bojowej: pochylony, tarcza wysunięta na plecy przedniego, noga zaparta, pchnięcie co ok. 1,5 s; włócznia uniesiona nad ramieniem pierwszego. Łucznicy bez naporu. Luki po poległych dalej zamyka `Uloz` (szerokość gracza zostaje, ubywa głębi).

## Dźwięk, etap 1: system (22.11.2026)
Prośba użytkownika: realistyczne odgłosy walki (szczęk, tarcze, krzyki, okrzyki, jęki), pracy (siekiery, kilofy, piły, kowal), gwar osady i unikatowe kwestie oddziałów po wybraniu i rozkazie. Plan 5 etapów: system, walka, praca, gwar osady, kwestie oddziałów. Wcześniej w projekcie nie było żadnego dźwięku.
- `Dzwiek/Dzwieki.h/.cpp`, `UDzwieki` (podsystem świata, sam się tworzy): `UDzwieki::Graj(this, "Nazwa", Gdzie, EDzwiek::Walka)`, `Graj2D` (kwestie, interfejs), `Petla` (szum tła, gra woła co klatkę z głośnością 0..1, bez wołania cichnie). Pliki to `/Game/Dzwieki/**/<Grupa>_<n>`, gra losuje wariant (nie ten sam dwa razy pod rząd), wysokość ±7%, głośność 85–100%. Brak plików = cisza, więc kod można podpinać przed assetami.
- Słuchacz stoi w ognisku kamery na ziemi (`SetAudioListenerOverride`), obrócony jak kamera. Tłumienie od ogniska: pełna głośność do 15 m, cisza od ok. 265 m.
- Kategorie (głośność, limit głosów naraz): Walka 1,0 / 20, Praca 0,8 / 10, Głosy 0,9 / 8, Osada 0,7 / 6, Rozkazy 1,0 / 1 (nowa kwestia przerywa poprzednią). Ten sam dźwięk najwyżej co 0,03 s.
- Mix od przybliżenia (`Blisko()`): do 400 m oddalenia kamery wszystko słychać, do 1400 m pojedyncze dźwięki gasną (log, smoothstep). Poza mapą grywalną świat milknie, a rozkazy grają dalej. Szum bitwy i osady z daleka podają pętle (etapy 2–4).
- `AKameraMapy`: gettery `Ognisko()`, `Oddalenie()`, `WRegionie()`. `grarts2d.Build.cs`: moduł `AssetRegistry`.
- Import: `Tools\Zbuduj_Dzwieki.bat` (`Tools/ue_dzwieki.py`): `Art/Dzwieki/<folder>/*.wav` → `/Game/Dzwieki/<folder>/`, pliki `Petla_*` z `looping`. Każdy plik wpisać do `Docs/Assety_zrodla.md`.
- Pominięte na razie: MetaSounds, Sound Class / Sound Mix w edytorze, suwak głośności w ustawieniach. Do pakowania na Steam trzeba będzie dodać `/Game/Dzwieki` do zawsze gotowanych katalogów, bo pliki są wczytywane po nazwie.
- Sterowanie i cel ataku (22.11.2026, użytkownik: „nie wykonują poleceń, nie ma indicatora, jaką jednostkę atakują”):
  - Marsz gracza nie staje już sam, gdy wróg jest bliżej niż `Kontakt` (wcześniej każdy rozkaz „idź” i odwrót przy wrogu zamieniał się w „stój”). Staje tak tylko SI. W marszu (`Zwarcie`, `bIdzie`) woje biją tylko tych, którzy są tuż przy nich (260 cm od miejsca w szyku), i szybciej odpuszczają, więc oddział idzie dalej.
  - Klik w oddział liczy się według obrazu: `OddzialPod(..., PC, Ekran)` najpierw szuka woja, w którego postać celuje kursor (30 px), a dopiero potem punktu na ziemi (kamera z ukosa trafiała w ziemię za wojem i atak zmieniał się w marsz).
  - Znaczniki w `SZnakiBitwy`: od każdego atakującego oddziału gracza czerwone kropki ze strzałką do celu (wybrane wyraźnie, reszta bladziej), pod wojami celu czerwone znaczki i czerwona ramka chorągwi. Wróg pod kursorem przy wybranych oddziałach też się podświetla (`CelPodKursorem`): prawy klik zaatakuje właśnie jego.

## Menu główne i ustawienia (22.11.2026)
- `UI/MenuGlowne.*` (`SMenuGlowne`, Slate, styl `StylUI`): menu startowe (Nowa gra, Wczytaj — wyłączone, bo zapisu jeszcze nie ma, Ustawienia, Wyjście). W grze F10 albo Esc (na mapie Europy; w osadzie Esc, gdy nic nie ma do zamknięcia): Kontynuuj + pauza. Otwarte menu zabiera grze mysz i klawiaturę.
- Zakładki: Grafika (rozdzielczość, tryb okna, VSync, limit FPS, jakość ogólna i 6 grup, metoda wygładzania, skala renderu 50–100%), Dźwięk (ogólna + kategorie `EDzwiek`), Sterowanie (przypisanie klawiszy, czerwony = konflikt w grupie), Kamera (prędkości, czułość, odwrócenia), Interfejs (skala UI, język — tłumaczeń jeszcze nie ma).
- `UI/Ustawienia.*` (`FUstawienia`): zapis w `GameUserSettings.ini` (`[Ustawienia]`, `[Klawisze]`, `[Dzwiek]`) + `UGameUserSettings`. `FUstawienia::Klawisz("Budowa")` zamiast `EKeys::B` w kamerze, `ABudowniczy`, `ABitwa`. `UDzwieki::Gain` i pętle mnożą przez `FUstawienia::Glosnosc(K)`.
- Wygładzanie: TSR 100% domyślnie (jak dotąd). DLSS / FSR / XeSS pojawiają się same, gdy ich wtyczka jest w grze (sprawdzane po zmiennych `r.NGX.DLSS.Enable`, `r.FidelityFX.FSR*.Enabled`, `r.XeSS.Enabled`); DLSS przy 100% = DLAA. Dla UE 5.8 NVIDIA wydaje wtyczkę DLSS 4.5 (8.7.2); „DLSS 5” jako oficjalnej wtyczki brak. `KameraMapy::UstawPiksele` nie nadpisuje już skali i AA.
- Zrzut kontrolny: `-zrzut -menu -zakladka=0 -ui`.

## Dźwięk, etap 2: walka (22.11.2026)
- Pobrane (zgoda użytkownika „pobieraj”): dwie paczki CC0 z OpenGameArt (Ben Jaszczak i Brian Nelson), 332 MB w `Dane_zrodlowe/dzwieki/oga_medieval`, rozpakowane 485 MB (92 długie nagrania 192 kHz). Wpis w `Docs/Assety_zrodla.md`.
- `Tools/dzwieki_walka.py` (python z UE, `PYTHONPATH=Tools/_py`): wykrywa uderzenia z obwiedni, bierze najgłośniejsze, tnie (0,5–0,9 s, wyciszenie 80 ms), normalizuje, zapisuje 48 kHz mono 16 bit do `Art/Dzwieki/Walka`. Grupy: Szczek 14, Drzewce 10, Tarcza 8 (drewno o drewno), Maczuga 10, Zamach 10, Luk 6, Kusza 5, Swist 8. Potem `Tools\Zbuduj_Dzwieki.bat` (71 plików w `/Game/Dzwieki/Walka`).
- `ABitwa`: zamach przy `WalkaCios` (0,45). W chwili uderzenia (`CiosyTick`): blok tarczą = Tarcza, inaczej broń atakującego (pałka = Maczuga, włócznia = Drzewce, reszta = Szczek); trafienie ciszej (0,6) + „Trafienie”. Strzał = Luk / Kusza, świst przy 75% lotu, strzała w tarczę = Tarcza. Śmierć = „Smierc”, okrzyk oddziału = „Okrzyk” (jeden na oddział).
- Brak jeszcze nagrań: Trafienie (cios w ciało), Smierc (krzyk, jęk), Okrzyk, szum bitwy z daleka. Kod je woła, ale bez plików jest cisza. Źródło: Freesound CC0 (wymaga konta użytkownika).
- Sprawdzone: gra wczytuje 71 plików w 8 grupach, bez błędów audio. Brzmienia nikt jeszcze nie słuchał.
- **DLSS (22.11.2026, wieczór)**: wtyczka NVIDIA DLSS 4.5 8.9.0 w `Plugins/DLSS` + `Plugins/StreamlineNGXCommon`, włączona w `grarts2d.uproject`, kompilowana ze źródeł (prekompilowane Binaries z zipa usunięte, zostały tylko `nvngx_dlss*.dll`). Log: „NVIDIA NGX DLSS supported DLSS-SR=1”. W menu: Metoda wygładzania → DLSS; skala 100% = DLAA (ta wersja nie ma osobnego `r.NGX.DLAA.Enable`, DLAA to tryb jakości z ułamkiem 1,0). XeSS i FSR nie pobrane (decyzja użytkownika).

## Dźwięk: poprawki walki po odsłuchu (23.11.2026)
Użytkownik: dźwięki trzeszczą na słuchawkach, są za głośne i brzmią jak bicie patykami; brak krzyków i odgłosów pracy.
- Trzaski: import kompresował krótkie uderzenia BINKA do ~4% rozmiaru. `Tools/ue_dzwieki.py` ustawia teraz `sound_asset_compression_type = PCM` dla wszystkiego poza `Petla_*`.
- Głośność: pliki normalizowane do -6 dBFS (było -3). `UDzwieki`: Walka 0,45 i 8 głosów (było 1,0 i 20), Praca 0,6 / 8, Głosy 0,7 / 6.
- „Patyki”: `Tools/dzwieki_walka.py` obniża Tarcza ×0,55 i ucina górę powyżej 2,5 kHz, Drzewce ×0,8 (6 kHz), Maczuga ×0,85, Szczek ×0,92. Włócznicy biją pół na pół Drzewce i Szczek.
- Krzyki i praca: Freesound pozwala pobrać podgląd HQ (MP3 128 kb/s) bez logowania. Lista 35 nagrań CC0 (~9 MB) czeka na zgodę użytkownika.
- Rozkład stereo (23.11.2026, użytkownik: dźwięk „na jednej słuchawce”): `UDzwieki` ma `NonSpatializedRadiusStart/End` = 5 / 80 m. Blisko ogniska dźwięk jest w obu uszach, strona narasta z odległością, na krawędzi kadru z bliska to mniej więcej połowa. Strojenie w `Dzwieki.cpp`.

## Dźwięk, etapy 3–4 i krzyki walki: nagrania z Freesound (23.11.2026)
- Pobrane za zgodą użytkownika: 35 nagrań CC0 z Freesound jako podglądy HQ MP3 (bez logowania), 12,7 MB w `Dane_zrodlowe/dzwieki/freesound/<id>.mp3`. Lista z autorami: `Tools/dzwieki_freesound.txt`, wpis w `Docs/Assety_zrodla.md`.
- `Tools/dzwieki_glosy.py`: MP3 dekoduje `libsndfile-1.dll` z silnika (ctypes), tnie na zdarzenia (funkcje z `dzwieki_walka.py`), pętle z 2 s przenikania. Wynik `Art/Dzwieki/Freesound/<Grupa>_51..` (88 plików); grupy o tej samej nazwie co w `Walka/` łączą się w grze: Okrzyk 8, Smierc 11, Rana 8, Trafienie 10, Tarcza +4, Szczek +4, Siekiera 9, Kilof 7, Pila 6, Kowal 10, Mlotek 8, `Petla_Bitwa` (60 s wrzawy), `Petla_Wioska` (40 s), `Petla_Gwar` (36 s rozmów). Razem 159 plików w 20 grupach.
- `ALudzik::Animuj`: dźwięk pracy, gdy cykl narzędzia się domyka (`Faza` przechodzi przez pełną liczbę): drwal Siekiera, kamieniarz Kilof, pilarz Pila, rzemieślnik z młotkiem Kowal, reszta z młotkiem (budowa) Mlotek.
- `ABitwa`: „Rana” przy trafieniu, które nie zabija; `Petla_Bitwa` co klatkę: min(1, walczące oddziały / 4) × (0,35 + 0,65 × oddalenie).
- `AOsada::Tick`: `Petla_Wioska` 0,5 i `Petla_Gwar` 0,35 × przybliżenie × min(1, ludzi / 25); pełne do 150 m od obozu, cisza od 400 m.
- Sprawdzone: kompilacja, import (156 plików PCM i 3 pętle), gra wczytuje 159 plików bez błędów audio. Brzmienia nikt jeszcze nie słuchał. Zostaje etap 5: kwestie oddziałów.

## Paczka gry do wysłania (23.11.2026)
- `Spakuj.bat` (edytor zamknięty): RunUAT BuildCookRun Win64 Shipping (`set KONF=Development` przed uruchomieniem zmienia konfigurację) → `Paczka\Windows\`, kopiuje `Data\` obok gry (czytane z dysku przez `FPaths::ProjectDir()/Data`), robi `Paczka\grarts2d.zip` (tar z Windows). Log: `Pakowanie.log`.
- `DefaultGame.ini`: `DirectoriesToAlwaysCook=/Game`, bo modele, tekstury i dźwięki są wczytywane po nazwie.
- `grarts2d.uproject`: `ModelContextProtocol` tylko w edytorze (nie trafia do gry kolegi).

## Budowy: anulowanie przebudowy, priorytety, zablokowane budowy (23.11.2026)
Prośba użytkownika: rozpoczętego ulepszenia nie dało się cofnąć; budowniczy stali na budowach bez materiału; priorytety budów jak w Manor Lords.
- **Anuluj przebudowę** (okno budynku, sekcja BUDOWA, tylko w trakcie ulepszania): `AOsada::AnulujUlepszenie` → `ABudowa::AnulujUlepszenie` przywraca `RodzajPrzed` i `ModelePrzed` (zapamiętane w `ABudowa::Ulepsz`), budynek znów gotowy; przyniesiony materiał (`Dostarczone`) wraca przez `AOsada::Zwroc` do obozu / magazynu. Towar niesiony w tej chwili tragarz odnosi do źródła sam (`ALudzik::Zwolnij`).
- **Priorytet** `ABudowa::Priorytet` 1..5 (3 = zwykły), przyciski − / + w sekcji BUDOWA. `AOsada::Przydzielaj`: budowy po priorytecie (przy równym najstarsze), najpierw wolni osadnicy, a gdy ich brak, budowniczy / tragarze z budowy o niższym priorytecie.
- **Zablokowana budowa** (`AOsada::Zablokowana`): stoi bez materiału, nikt go nie niesie i nie ma skąd wziąć. Oddaje swoich ludzi, nikt nowy tam nie idzie; wraca do kolejki, gdy materiał się pojawi. Okno: „Czeka na materiał: budowniczowie poszli na inne budowy”.
- Stawianie budynków bez materiału dalej dozwolone (jak w Manor Lords) — czeka na potwierdzenie użytkownika.
- Sprawdzone: kompilacja. Bez zrzutu (zmiana widoczna dopiero w otwartym oknie budowy) i bez gry.
- Dopisek: rodzina wysłana do budynku pracy schodzi z budowy (tak było zawsze: budują tylko wolni osadnicy). Gdy przy budowie nikogo nie ma i nie ma wolnych osadników, okno budowy pisze „Nikt nie buduje: wszyscy osadnicy mają pracę…” (`AOsada::Budujacych`). Czeka na decyzję użytkownika, czy rodziny z pracą mają pomagać przy budowach.
- Dopisek (po rozwiązaniu oddziału stali ludzie): woj zbrojowni bez włóczni (polegli gubią swoje) czekał tam, gdzie stał oddział — teraz wraca pod zbrojownię i czeka tam (`ALudzik::ZadanieWoja`). Człowiek bez pracy i bez domu wraca z pola bitwy do obozu (`ALudzik::WrocZBitwy`). Wolni bez budów nadal tylko kręcą się przy domu.

## Paczka A, krok 1: ubranie z targu i poziom osady (23.11.2026)
Plan: dokument „Plan rozwoju gry do wydania na Steam”, sekcja „Co jeszcze dodać”, paczka A (wybór użytkownika).
- **Ubranie** (nowa potrzeba): `FRodzina::bUbrana / DoUbrania`. Co `AOsada::PosilkowNaUbranie` (4) posiłki rodzina bierze 1 belę sukna, **tylko z targu** (`AOsada::Targ`). Brak = nieubrana do następnego razu. Nowa rodzina przychodzi ubrana.
- **Targ zaopatruje się sam**: `AOsada::Rozwoz` — gdy na targu jest mniej niż 6 sukna, wolny osadnik donosi je z tkacza albo magazynu (`ALudzik::Przewiez`).
- **Zadowolenie** (`CelZadowolenia`): jedzenie 35, ciepło 10, domy 25, kościół do 20, ubranie 10 (było 40 / 10 / 30 / 20). Podpowiedź licznika ma wiersz „Ubranie”.
- **Poziom osady** (`AOsada::Poziom`, `EPoziomOsady` z `Osada/Rody.h`, tylko rośnie, sprawdzany co 0,5 s w `Tick`, ogłoszenie „Awans!”): wieś = 8 rodzin, 4 domy, kościół, zadowolenie 50%; osada targowa = 15 rodzin, targ, 3/4 rodzin ubranych, 60%; miasto = 30 rodzin, kościół kamienny, 70% (`WarunkiPoziomu`). `ARody::PoziomGracza` bierze go z osady.
- **Blokady**: `FRodzajBudynku::Wymaga` — targ, kuźnia, zbrojownia, kościół kamienny (ulepszenie) od wsi; kościół wielki i duży dom od osady targowej. Pasek budowy wyszarza kartę z podpowiedzią „Wymaga poziomu osady: …” (`SPasekBudowy::Blokada` → `AOsada::CzemuZablokowany`), `CzemuNieUlepsz` też. „Postaw wszystko” (tryb deweloperski) omija blokady.
- Panel osady (lewy górny róg): pierwszy wiersz = poziom osady, podpowiedź = warunki awansu i co odblokuje (`OpisPoziomu`).
- Nie ma jeszcze: rodziny chodzące fizycznie na targ, różnych rodzajów jedzenia (potrzebne nowe surowce: ryby, chleb osobno), drewna opałowego, rybaka, wędzarni, krawca/garbarza/szewca, browaru, podatków. Sprawdzone: kompilacja.

## Paczka A, krok 2: drewno opałowe, rybak, dwa rodzaje jedzenia (23.11.2026)
- Nowe surowce `ESurowiec::Opal` (drewno opałowe) i `Ryby` (ikony, stosy `SM_Stos_Opal/Ryby`, niesione `SM_Ludz_Opal/Ryby`). Stosy: targ (opał, ryby), spichlerz (ryby), plac składowy (opał).
- **Chata opałowa** (`FRodzajBudynku::ChataOpalowa`, Przetworca): z jednej kłody 3 naręcza polan. Zimą rodzina przy posiłku pali naręcze opału, a gdy go nie ma, kłodę (`AOsada::Posilek`).
- **Chata rybaka** (`ChataRybaka`, `bNadRzeka`): rybak idzie od chaty w stronę rzeki do ostatniego suchego miejsca, łowi wędką (`SM_Ludz_Wedka`), ryba w ok. 45% prób (zimą 25%), niesie na stos (`ALudzik::ZadanieRybaka`).
- **Różne jedzenie**: posiłek bierze po porcji jedzenia i ryb; `FRodzina::Rodzajow`. W zadowoleniu jedzenie 25 za sytość + 10 za dwa rodzaje (`CelZadowolenia`), podpowiedź licznika pokazuje „dwa rodzaje jedzenia: X z N rodzin”.
- Wędzarnia odłożona (bez psucia się jedzenia nie ma po co). Modele: `Tools/budynki.py` (`chata_opalowa`, `chata_rybaka`, `sagi`, `lodka`, `stos_opalu`, `stos_ryb`), `Tools/ludzie.py` (`wedka`, `ryby_niesione`, `opal_niesiony`).

## Pomoc przy budowie i woły (23.11.2026)
Prośba użytkownika: rodzina, która akurat nie ma nic do roboty, pomaga przy budowie; woły jak w Manor Lords do zwózki kłód, bez kupowania, na start 2, kolejne z zagrody.
- **Pomoc**: `ALudzik::BezRoboty` zlicza sekundy czekania bez zajęcia w budynku pracy (pełny stos, brak wsadu, zima, pole czeka — `Bezrobotny(...)` zamiast `Czekaj`), `Pracuj` zeruje. Po 20 s `DoPomocy()`; `AOsada::Przydzielaj` bierze takich po wolnych, a przed ludźmi z budów o niższym priorytecie (`ALudzik::Pomoz`). Pomocnik pamięta swoją pracę (`Stala`) i wraca do niej, gdy budowa się skończy albo zablokuje (`KoniecPracy` / `Odwolaj`). W oknie jego budynku: „pomaga przy budowie (…)”.
- **Woły** (`Ludzie/Wol.h`, `AWol`): 2 na start przy obozie (`AOsada::WolowNaStart`), **Zagroda wołów** (`FRodzajBudynku::ZagrodaWolow`, kategoria Surowce, koszt 5 kłód + 2 deski) daje 2, ulepszona do **Obory** (`ZagrodaWolow2`, `Wolow = 4`) razem 4. Startowe przechodzą do pierwszej zagrody. `AOsada::RozwozWolami`: wolny wół dostaje kurs — najpierw budowa o najwyższym priorytecie, której brakuje min. 2 kłód (ze źródła `ZrodloSurowca`), potem wywózka od drwala (min. 3 kłody) do najbliższego magazynu; do 6 kłód naraz (`AWol::MaxLadunek`). Rezerwacje jak u tragarzy (`Zarezerwowane` / `WDrodze`). Chód: nogi na krzyż, kołysanie tułowia, kłody wleczone na łańcuchach (`SM_Ludz_WolKlody`). Bez kursu pasie się przy zagrodzie.
- Modele: `Tools/ludzie.py` (`wol_tulow`, `wol_noga`, `wol_klody`; własny walec `rura`, bo `walec` z budynki.py ma ściany do środka, a `M_Ludzie` jest jednostronny), `Tools/budynki.py` (`zagroda_wolow(faza, poziom)`, `SM_ZagrodaWolow2_U`), ikona w `Tools/ikony.py`.
- Konie: jeszcze nie (następny krok: stajnia i wóz do desek / kamienia).
- Sprawdzone: kompilacja, import, jeden zrzut (`-zrzut -region -wszystko -odl=5000 -czekaj=25`): woły stoją przy zagrodzie; na zrzucie tułów był pusty w środku (walec od środka) — poprawione i zaimportowane, bez drugiego zrzutu.

## Konie i stajnia (23.11.2026)
Prośba użytkownika („dawaj dalej” po wołach): konie jak w Manor Lords.
- **Stajnia** (`FRodzajBudynku::Stajnia`, kategoria Surowce, od poziomu wsi, koszt 6 kłód + 4 deski + 2 kamienie) daje 2 konie (`Koni = 2`).
- Koń to `AWol` z `UstawKonia()`: inny model (`SM_Ludz_KonTulow/KonNoga`, wyższe stawy nóg), za nim wóz `SM_Ludz_Woz`, na wozie ładunek `SM_Ludz_WozDeski` / `SM_Ludz_WozKamien`. Szybszy od wołu (170 / 135 cm/s z wozem), do 8 sztuk (`Udzwig`).
- `AOsada::RozwozWolami`: woły biorą kursy z kłodami, konie z deskami, a potem z kamieniem. Zasady takie same: najpierw budowa o najwyższym priorytecie (brak min. 2 sztuk), potem wywózka od wytwórcy (pilarnia, kamieniołom; min. 3) do najbliższego magazynu. `AWol::Ciagnij(Z, Cel, Ile, Surowiec)`.
- Modele: `Tools/ludzie.py` (`kon_tulow`, `kon_noga`, `woz`, `woz_deski`, `woz_kamien`), `Tools/budynki.py` (`stajnia`), ikona w `Tools/ikony.py`.
- Sprawdzone: kompilacja, import, jeden zrzut (`-zrzut -region -wszystko -odl=7000 -czekaj=30`): konie z wozami jadą przez obóz, woły przy zagrodzie mają już pełny tułów (poprawka z poprzedniego kroku działa). Koła wozu się nie kręcą.

## Paczka A, krok 3: rzemiosło i piwo (23.11.2026)
- Nowe surowce (`ESurowiec`): skóry, skóra, buty, ubrania, słód, piwo (ceny w `Rody.cpp`, ikony w `Tools/ikony.py`, niesione modele w `Tools/ludzie.py`).
- Łańcuchy (wszystkie Przetworca, od poziomu wsi, wspólny układ `Warsztat(...)` w `Budowa.cpp`): **Garbarnia** skóry → skóra, **Warsztat szewca** skóra → 2 buty, **Warsztat krawca** sukno → 2 ubrania (kategoria Rzemiosło); **Słodownia** zboże → słód, **Browar** słód → 2 piwa (kategoria Jedzenie). Myśliwy przy każdej sarnie odkłada też skórę (`Wyrob2 = Skory`, do 4 na stosie).
- **Karczma** (`FRodzajBudynku::Karczma`, Magazyn tylko na piwo, 16 baryłek). Targ trzyma też ubrania i buty.
- Potrzeby (`AOsada::Posilek`): przy zmianie ubrania rodzina bierze najpierw ubranie z targu (2 pkt), a jak brak, belę sukna (1 pkt); do tego parę butów. Co drugi posiłek baryłka piwa z karczmy. Zadowolenie: jedzenie 30, ciepło 10, domy 20, ubranie 5, wygody (buty + piwo) 10, kościół 12/16/20. Poziom miasta wymaga karczmy.
- Jęczmienia jako osobnej uprawy nie ma (słodownia bierze zwykłe zboże).
- Sprawdzone: import, kompilacja, jeden zrzut (`-wszystko -odl=7000`): gra startuje, budynki się ładują.

## Podatki i dziesięcina (23.11.2026, koniec paczki A)
- `AOsada::Podatek` (0 brak, 1 niski, 2 średni, 3 wysoki; start: niski) i `AOsada::bDziesiecina`. Na pasku u góry obok groszy dwa przyciski: „Podatek: …” (klik zmienia stawkę po kolei) i „Dziesięcina: tak/nie”; podpowiedź `OpisPodatkow()`.
- `AOsada::PobierzPodatki()` przy każdej nowej porze roku (`NowaPora`): każda rodzina w domu płaci tyle groszy, ile wynosi stawka (rodziny w namiotach nie płacą). Dziesięcina (tylko z kościołem): 1/10 jedzenia z magazynów, zaokrąglone w górę.
- Zadowolenie (`ZPodatkow()`): brak podatku +5, niski 0, średni -6, wysoki -14, oddana dziesięcina +5. Cel zadowolenia jest teraz przycięty do 0–100.
- Wędzarnia dalej odłożona (bez psucia jedzenia).
- Sprawdzone: kompilacja. Bez zrzutu (zmiana w UI i rachunkach).

## AI przeciwników, faza AI0: dziennik i testy (23.11.2026)
Plan: dokument „AI przeciwników: plan” (claude.ai/artifact/X4DXj61Bh3sq2aitZCauRy). AI0 tylko mierzy, zachowanie rodów bez zmian.
- **Stałe ziarno**: `ARody::Los` (`FRandomStream`) zamiast `FMath::FRand` w `Rody.cpp`; w grze losowe, w testach stałe.
- **Kalendarz bez osady gracza**: `ARody::Kalendarz` liczony w `Przewin`; `PoraRodow()` bierze porę z `AOsada`, a bez niej z własnego kalendarza.
- **Dziennik AI** (`ARody::bDziennik`; panel F3 „Dziennik AI”, parametr `-aidziennik`, w testach zawsze): linie `[AI] rok N pora | ród (cecha) | rodzin, woje, budynków | kłody deski kamień jedzenie | co | bo: …` przy każdej decyzji, przy zmianie powodu czekania i raz na porę roku; plik `Saved/AI_dziennik.csv` (wiersz na ród co porę). `Decyduj` zapisuje powód każdej pozycji listy (`FRod::Dlaczego`), a gdy nic nie stawia — na jaki budynek czeka i czego brakuje.
- **Strażnik zastoju** (`ARody::Pilnuj`, `CzasZastoju` = 2 lata gry): postęp = nowy rekord rodzin, budynków albo wojów; bez postępu → `ZASTÓJ` w logu (zawsze) i `FRod::bZastoj`. W oknie rodów (H) nowa linia „Dlaczego: …” z napisem ZASTÓJ.
- **Testy** (`Testy/AITesty.cpp`, `Tools\Testy.bat` pokazuje też linie `[AI-TEST]` i `ZASTÓJ`):
  - `grarts2d.AI.Bramka` (musi przechodzić): 8 rodów, 10 lat gry w 0,19 s, każdy ród ma wpisy, to samo ziarno = ten sam wynik — ZALICZONY.
  - `grarts2d.AI.Scenariusze` (testy z AI1, na razie tylko raport; `bScenariuszeObowiazkowe = true` w AI1): Od zera, Strata, Głód, Zima — ZALICZONE; Wzrost i Bez zastoju — BŁĄD.
- **Wynik pierwszego raportu** (dla AI1): oba rody pobożne (Odrowążowie, Pałucy) stają w 4. roku na 14 rodzinach: kościół jest pierwszy na liście, a ród nie ma pilarni ani kamieniołomu (desek 0/20, kamienia 0/60), choć ma 180 kłód — to zastój nr 1 z tabeli planu. Pozostałe 6 rodów po 10 latach: 54 rodziny, 77 budynków, ale **0 wojów** (także wojownicze) i wszystkie identyczne — cecha prawie nie zmienia rozwoju.

## AI przeciwników, faza AI1 i początek AI2 (23.11.2026)
- **Błąd kościoła**: rody chciały `FRodzajBudynku::Kosciol()` (kamienny, ulepszenie, nie ma go na liście `DoBudowania`), więc pobożne stawały na zawsze. Teraz `KosciolDrewniany()` (decyzje, zadowolenie, poziom osady); dla gracza liczy się drewniany albo kamienny.
- **Wytwórca braku** (`ARody::Decyduj`, koniec): gdy żadnej z 3 pozycji nie stać, a rodowi brakuje surowca, którego nikt u niego nie wytwarza, stawia wytwórcę (łańcuchem po wsadzie do 3 kroków, np. broń → kuźnia → ruda → kopalnia). Dziennik: „buduje: Pilarnia | bo: brak: Deski na …”.
- **Wojowniczy ród** (AI2, część): od 8 rodzin kuźnia, kopalnia rudy i zbrojownia mają pierwszeństwo przed domami; kolejne zbrojownie, gdy wszystkie miejsca zajęte, do 1/3 rodzin wojami.
- Testy: `grarts2d.AI.Scenariusze` są teraz **obowiązkowe** i wszystkie ZALICZONE (Wzrost: najmniej 30 rodzin po 6 latach; Od zera, Strata, Głód, Zima, Bez zastoju: 0 zastojów). Bramka: rody wojownicze po 10 latach mają 15 wojów (było 0), pozostałe 57–59 rodzin.
- Nowy test `grarts2d.AI.Wojsko` (AI2, na razie raport): BŁĄD — wojownicze mają 1 woja po 4 latach (wymagane 8). Dalej w AI2: wcześniejsze zbrojenie, zagrożenie od sąsiadów, rozwiązywanie wojów po wojnie.

## AI przeciwników, faza AI2: wojsko (23.11.2026)
- Wojowniczy ród dobiera kolejne zbrojownie do 1/2 rodzin wojami (`Decyduj`) i — gdy broni jest mniej niż 4 — najpierw obsadza kopalnię rudy i kuźnię (`Produkuj`, kolejność obsadzania).
- Test `grarts2d.AI.Wojsko` obowiązkowy i ZALICZONY: wojownicze po 4 latach 8 wojów (16 rodzin, dalej rosną do 45 po 10 latach), pokojowe bez wojów (≤ 1/4 rodzin); „Utrata armii”: po zabiciu wszystkich wojów w 1,5 roku znów 9.
- Nie zrobione z AI2: liczba wojów według zagrożenia od sąsiadów i rozpuszczanie wojów po wojnie (rody jeszcze nie walczą — to AI3).

## AI przeciwników, faza AI3: decyzje władcy (23.11.2026)
- `ARody::Wladca(i)` raz na porę roku (z `NowaPoraRodow`) dla każdego rodu, kolejność: **odbudowa** po napadzie (szuka zgody z napastnikiem) → **napad** na wrogiego (stosunek < -20) sąsiada co najmniej 1,5 raza słabszego (min. 3 woje, jedzenie na posiłek) → **napad na gracza** (tylko wojowniczy, od 2. roku, siła ≥ 1,5 × gracza, zapowiedziany porę wcześniej ogłoszeniem; łup przez `AOsada::Grabiez(0.25)`) → **danina** od dwa razy słabszego → uzbrojony wobec silniejszego wroga: **pokój** (rozejm) → **obrona** (`FRod::bObrona`: zbroi się do 1/4 rodzin) → **pokój** z równym wrogiem → **handel** z przyjacielem latem i jesienią → **rozwój**.
- Stosunki między rodami `FRod::Wobec` (`UstawStosunki`, `BazaStosunku`: te same cechy +20, wojowniczy z innymi -30, dwa wojownicze -40), co porę wracają o 5 do bazy; napad -30, rozejm do 0.
- `ARody::Bitwa`: siła (woje × 3 + rodziny) × los 0,85–1,15, obrońca × 1,2. Wygrany zabiera 30% zapasów i groszy, przegrany traci połowę wojów i 1/7 rodzin (zostają min. 2).
- shortcut: napad na gracza liczony z siły, bez prawdziwej `ABitwa` przy kamerze — do zrobienia z wojną W3 / AI4.
- Test `grarts2d.AI.Wladca` (obowiązkowy, ZALICZONY): silny na słabego napada w ciągu roku; 0 napadów słabszego na silniejszego (5 napadów w 10 lat); napadnięci odbudowują się (15 → 50 rodzin); przyjaciele nie walczą; każdy ród ≥ 4 różne decyzje (próg z planu 5 obniżony: rody pokojowe z zasady nie napadają ani nie żądają daniny, wojownicze mają 5).
- Skutek uboczny (zamierzony): rody pokojowe zbroją się przed wojowniczymi (10–12 wojów, ≤ 1/4 rodzin).
- AI4 (ludzie rodu przy kamerze) według planu dopiero po wojsku z planu Steam (F2).

## AI przeciwników, faza AI5: poziomy trudności (23.11.2026)
- `ARody::Trudnosc` (0 łatwy, 1 średni, 2 trudny): linia poleceń `-trudnosc=`, przycisk „Trudność AI” w panelu F3. Łatwy: ród decyduje o budowie tylko w 40% kroków, napada przy przewadze 2×, na gracza od 3. roku; trudny: przewaga 1,3×, zbroi się od 6 rodzin, trzyma większy zapas kłód.
- Test `grarts2d.AI.Balans` (50 ziaren × 3 poziomy × 5 lat): 0 zastojów. Średnio ok. 23 rodziny i 4 woje na ród na każdym poziomie, napadów na grę 8 / 5 / 12. Poziom prawie nie zmienia wzrostu, tylko wojny — do strojenia po ocenie.

## AI przeciwników, faza AI4 (część widoczna) i sprawdzenie na x10 (23.11.2026)
- `AOsadaRodu` (Osada/OsadaRodu.*): osada rodu w terenie w miejscu `FRod::Gdzie`, gotowe budynki rodu (modele gracza) na siatce co 30 m i chorągiew w barwie rodu; odświeżana co porę roku (`ARody::OdswiezWidoki`). shortcut: bez ludzików rodu, rod dalej liczony liczbami.
- Okno rodów (H): przycisk „Pokaż osadę” przenosi kamerę nad osadę rodu.
- `-tempo=10` (start gry od razu z przyspieszeniem, jak klawisz T) — parsowane w `ABudowniczy::BeginPlay` (wcześniej siedziało w pokazie `-osada` i nie działało z `-wszystko`).
- Sprawdzenie `-wszystko -tempo=10 -czekaj=150`: 2 lata gry bez błędów; pory roku, podatek co porę, woły z wozami, rody AI rosną do 12–16 rodzin w 2 lata. Uwagi: (1) w pokazie 28 rodzin i jedno pole — od zimy roku 1 wszyscy głodni (to pokaz, nie zwykła gra, ale warto dać ostrzeżenie o braku jedzenia); (2) podatek płaci tylko rodzina w domu (w pokazie 1 dom = +1 grosz); (3) przez 2 lata żaden ród nie ma wojów — wojownicze czekają na kamień do kuźni (brak kamieniołomu w ich kolejce przed kuźnią); (4) `-widokrodu` działa tylko z `-osada -rody` (tryb zrzutu ustawia kamerę po swojemu).

## Poprawki po sprawdzeniu x10 i mocniejsze AI z murami (23.11.2026, prośba: „Poprawiaj, zrób dobre AI ... osady które będą miały mury”)
- Ostrzeżenie „Kończy się jedzenie” (`AOsada::Posilek`), gdy zapas < 2 posiłki dla wszystkich rodzin — zanim przyjdzie głód.
- Podatek: rodzina w namiocie płaci połowę stawki (w górę), w domu całą (`PobierzPodatki`, opis w podpowiedzi).
- Rody: `FRod::Mury` (0 brak, 1 palisada, 2 mur kamienny), `Buduje = FRod::BudujeMury`. Palisada (40 kłód, 150 s) od 10 rodzin u zagrożonych (silniejszy wróg albo byli napadnięci; trudny od 8), od 16 u spokojnych; mur kamienny (60 kamienia + 20 kłód, 300 s) 10 rodzin później z kamieniołomem. Obrońca za palisadą ×1,3, za murem ×1,6 (`Obrona()`, `SilaObrony()` w `Bitwa`, przy wyborze celu napadu i przy żądaniu daniny przez gracza).
- Wojowniczy: przy braku kamienia na kuźnię najpierw drugi kamieniołom; zbroi się od 6 rodzin. Głodny ród (`bGlodny` z ostatniego posiłku) rozpuszcza wojów do 1/3 rodzin (wojowniczy) albo 1/4 — wcześniej wojowniczy stawał na 16 rodzinach bez jedzenia. Teraz rosną do 41–49 rodzin w 10 lat (najmniej po 6 latach 23 zamiast 16).
- Test `grarts2d.AI.Mury` (obowiązkowy, ZALICZONY): woje po 4 latach ≥ 6 u wojowniczych, po 6 latach 8 palisad i 1–2 mury kamienne, mury bronią. `grarts2d.AI.Wojsko`: próg z 8 na „≥ 7 i połowa rodzin” (palisada kosztuje ród trochę wzrostu). Wszystkie testy zielone.
- `AOsadaRodu`: układ pierścieniami wokół placu z chorągwią (kościół, targ, magazyny najbliżej, potem domy, warsztaty), budynki wzdłuż okręgu, przerwa na drogę do bramy (+X); mury po okręgu za zabudową (odcinki idą za terenem, baszty co ~1/8 obwodu i dwie przy bramie); drwal, kamieniołom, zbieracz, myśliwy, owczarnia, młyn, zagrody, kopalnia za murami. Cały środek osady wykarczowany.
- Modele (Tools/budynki.py): `SM_Palisada` (odcinek 4 m z zaostrzonych pali), `SM_Palisada_Wieza` (drewniana strażnica), `SM_Mur` (odcinek muru z blankami), `SM_Mur_Wieza` (kamienna baszta). Zewnętrzna strona -Y w Pythonie = +Y w Unrealu.
- `ARody::Przewin` odświeża widoki na końcu (w grze pory rodów idą za kalendarzem gracza). Zrzut: `-osada -rody -latarodow=7 -widokrodu -odl=...` (kamera nad 1. rodem w połowie `-czekaj`, ustawiane w `ASwiatGameMode::PoWczytaniu`).
- Nie sprawdzone po ostatnich zmianach: ponowny przebieg x10 i zrzut murów z daleka (zostawione użytkownikowi).
- Wymiary murów pod ludzika (23.11.2026, prośba użytkownika): ludzik w grze ok. 2,4 m (1,7 m × `ALudzik::Skala` 1,4). Palisada 4,4 m z pomostem obrońców na 2,1 m (bez drabin w odcinkach — były w każdym 4-metrowym kawałku; wchodzi się przy strażnicach); strażnica z pomostem na 5,6 m, balustradą 1,4 m i drabiną; mur 5,6 m z chodnikiem 2,4 m szerokości i blankami 1,6 m; baszta 6 × 6 m, platforma na 9 m, drzwi 1,6 × 3 m od środka osady. Podgląd z ludzikami: `Saved/podglad_mury_ludzik.png`. `Tools/podglad.py`: poprawka indeksu tekstury (mod 1 dawał czasem 1,0).
- Osady rodów (23.11.2026, prośba: „domki powinny mieć różny styl”): każdy dom losuje jeden z 4 wariantów modelu (`SM_<Dom>_W1..3`, jak u gracza), budynki są przemieszane w pierścieniu i lekko przesunięte i skręcone (do 1,5 m i 8°); los z nazwy rodu, więc osada wygląda tak samo po odświeżeniu. Zrzut kontrolny: HighresScreenshot00031 (palisada z pomostem i drabinami widoczna).

## Życie w osadach rodów AI (23.11.2026, prośba: „w tych wioskach AI nic się nie dzieje?”)
- `AOsadaRodu` tyka: gdy kamera jest bliżej niż 450 m od środka osady, stawia do 26 ludzików (`ALudzik` w trybie walki, sterowane jak w `ABitwa`: `WalkaIdz`, `WalkaCios`, `WalkaStoj`), po oddaleniu kamery je usuwa.
  - Pracownicy (szare i brązowe lny, przy koszach część to kobiety) chodzą z domu do swojego budynku i pracują narzędziem: drwal siekiera, kamieniarz i górnik kilof, rolnik motyka (jesienią sierp), myśliwy włócznia, rybak wędka, pasterz nożyce, pilarz piła, warsztaty młotek; potem wracają do domu, odpoczywają i idą znowu.
  - Plac budowy: budowany budynek (`FRod::Buduje`) stoi w połowie budowy (faza ze środka) i trzech ludzi stuka przy nim młotkami.
  - Woje (do 8) w barwie rodu, z hełmem i tarczą, ćwiczą z włóczniami wokół chorągwi.
  - Pola rodu za murami (`SM_Pole_F2`) z uprawą według pory roku (zasiane, rośnie, dojrzałe, ścierniska).
- `ARody::OdswiezWidoki` dostaje porę roku; widok odświeża się przy zmianie budynków, murów, budowy albo pory (co krok rodu, 2 s).
- shortcut: ludziki tylko udają pracę (towar dalej liczony liczbami w ARody), bez noszenia towaru i bez wozów.
- Zrzut kontrolny: HighresScreenshot00032 (`-osada -rody -latarodow=4 -widokrodu -odl=9000 -czekaj=40`).

## Osady rodów jak średniowieczne wsie, nowe mury i baszty (23.11.2026, prośba: „mury, palisady, wieże słabo wyglądają … robią kółko … teren dziwnie wygląda … przy kościele się bardziej starałeś”)
- Układ (`AOsadaRodu::Odswiez`, wszystko z nazwy rodu, więc ten sam przy każdym odświeżeniu):
  - Plac 34 × 25 m z chorągwią i studnią (`SM_Studnia`); kościół, magazyny, targ i karczma najpierw szukają miejsca frontem do placu.
  - Ulica główna w dwie strony od placu, łagodnie wygięta (do 110 m zabudowy), dalej droga do pól (do 270 m). Boczne uliczki odchodzą skosem w osadzie (27, 60 i 93 m od placu) i jako drogi polne za bramą (148 i 210 m).
  - Budynki frontem do ulicy, 1,7 m podwórza; domy mają za sobą ogródek 7 m (grządki z `SM_Pole_Kafel_*` według pory roku) z płotem z żerdzi (`SM_Plot`). Każdy budynek szuka pierwszego wolnego miejsca na każdej ulicy i bierze najbliższe placu.
  - Działki są zapamiętane (`FDzialka`: rodzaj i numer budynku), więc nowy budynek nie przestawia starych; plac budowy stoi na przyszłej działce.
  - Mur po obrysie zabudowy (otoczka wypukła działek z ogrodami + 5 m), baszty w rogach i co ~34 m, bramy tam, gdzie ulica główna przecina obrys, jedne schody w dłuższym ciągu. Gospodarstwo, las i pola przy drogach za bramami; gdy osada urośnie, obejścia za blisko muru szukają nowego miejsca.
  - Przedmieście: dom lub warsztat, który nie zmieści się w murach, staje przy drodze za bramą (też z ogródkiem); przy następnym odświeżeniu najpierw próbuje wejść do środka.
- Teren: `ARody::Zaloz` wybiera z 40 prób najrówniejsze miejsce (najmniejsza różnica wysokości w kole 90 m, z dala od rzeki). Budynki stoją na najwyższym punkcie obrysu (jak u gracza, podmurówka schodzi w dół). Ubita ziemia placu, ulic, podwórzy i ścieżek do ulicy: jedna siatka na terenie (`ZbudujZiemie`, M_Droga, poszarpany brzeg), budowana, gdy kamera jest bliżej niż 1,5 km. Karczowanie tylko pod działkami, ulicami, placem i murem (już nie całe koło, przez co środek osady był jednolicie zielony).
- `ATerenKafle`: polany w komórkach 50 m (`PolanyKomorki`, `DodajPolane` pomija powtórki), `Wykarczuj` przegląda tylko kafle pod polaną — wcześniej każde wywołanie przeglądało wszystkie drzewa wszystkich kafli, a osady rodów karczowały przy każdym odświeżeniu od nowa.
- Modele w instancjach (`Instancje`: jeden komponent na model) zamiast osobnego komponentu na każdy budynek i odcinek muru.
- Nowe modele (Tools/budynki.py, podgląd z ludzikiem sprawdzony):
  - `SM_Palisada` / `SM_Palisada_Schody`: wał ziemny 0,9 m z darnią, pale różnej grubości i wysokości (3,5–4 m nad wałem) z jasnymi zaostrzonymi czubkami, dwa wiązania, ganek obrońców 2 m nad wałem z poręczą, ostrokół na stoku; odcinek ze schodami zamiast drabin.
  - `SM_Palisada_Wieza`: baszta 4,6 m na zrąb z bali, strzelnice, drzwi od środka ze stopniami, izba z desek wysunięta na wspornikach, dach z gontu z iglicą (12 m).
  - `SM_Palisada_Brama`: wieża bramna z przejazdem 3,8 × 4,3 m, otwarte wrota, izba z desek i dach czterospadowy.
  - `SM_Mur` / `SM_Mur_Schody`: cokół ze skarpą, warstwy kamienia (co druga cofnięta o 1 cm, różne odcienie), gzyms, przedpiersie, blanki z daszkami i strzelnicami, otwory po rusztowaniu; kamienne schody od środka.
  - `SM_Mur_Wieza`: okrągła baszta (skarpa, gzymsy, wsporniki pod wysuniętym zwieńczeniem z okienkami, strzelnice, drzwi), dach stożkowy z dachówki (ok. 19 m).
  - `SM_Mur_Brama`: kamienna wieża bramna z przejazdem łukowym, broną, otwartymi wrotami, oknami, wspornikami, blankami i dachem czterospadowym z dachówki.
- Test `grarts2d.AI.UkladOsady` (Testy/OsadaRoduTesty.cpp, bez okna gry): 8 rodów po 10 latach, każda osada w tymczasowym świecie. Sprawdza, że stoją wszystkie budynki, działki nie nachodzą, zabudowa w środku jest w murze, są 2 bramy, baszty i odcinki muru, a po dwóch nowych domach stare budynki w środku zostają na miejscu. Wynik: 63–73 budynki na ród, 14–20 za murem (pola, gospodarstwa, las, kilka domów na przedmieściu), 21–24 baszty, ok. 170 odcinków muru, 5–30 ms na odświeżenie. Wcześniej w środku brakowało miejsca dla 4–14 budynków (głównie domy), stąd trzecia para uliczek, dłuższa ulica główna i przedmieście.
- Plan osady z góry bez uruchamiania gry: test zapisuje `Saved/Plany/<ród>.txt` (`AOsadaRodu::ZapiszPlan`), a `Tools/plan_osady.py` rysuje z nich PNG (ulice, mur, domy z ogródkami, warsztaty, pola).

## Wojna: zniszczenia budynków, ogień, wskaźnik ataku i podbój osad rodów (23.11.2026, prośba: „etapy niszczenia budynków podczas wojny i mechanika żeby podbić te miasta … krzyki, krew, dym, ogień realistyczny … lepszy indykator co jednostki atakują”)
- `Efekty/Efekty.h/.cpp`, `AEfekty` (jeden na świat, `AEfekty::W(World)`): cząstki liczone na procesorze, rysowane w 5 instancjach (płomienie, dym, iskry, kurz, gruz) z kolorem w danych instancji. `Pozar` / `UstawPozar` / `ZakonczPozar` (języki ognia nad dachem, kłąb dymu sortowany od najdalszego, iskry z wiatrem, do 6 ciepłych świateł z łagodnym pulsowaniem), `Zawal` (kurz przy ziemi, kamienie albo zwęglone belki odbijające się od terenu, snop iskier), `Kurz`. Tick wyłącza się, gdy nic nie płonie.
- `ABudowa`: wytrzymałość (`MaxPunktow` z kosztu i powierzchni), `Uszkodz`, `Podpal`, `Gas`, `Palnosc` (drewno pali się mocno, kamień słabo). Poniżej 70% model `_Z1`; ogień trawi punkty; przy zerze zawalenie przez 2,6 s (osiada, drży, przechyla się, kurz zasłania podmianę) i ruina `_Z2`, która tli się jeszcze 45–70 s gry. Plac budowy spala się do cna i znika. Ogień przeskakuje na budynki do ~9 m od brzegu (szansa od siły ognia i palności sąsiada). Pętla trzasku ognia według odległości i krzyki ludzi przy płonącym domu.
- `AOsada`: ruiny osobno (`Ruiny`), ogłoszenie przy zawaleniu; okno budynku ruiny ma „Odbuduj” (plac budowy od zera) i „Uprzątnij” (zwraca 1/3 kamienia). Czerwony pasek wytrzymałości i „Płonie!” w oknie budynku.
- Napad: bandyci po grabieży podpalają najbliższe budynki (1 na 8 zbójów, najwyżej 5).
- Wskaźnik ataku (`SZnakiBitwy`): twój atak to gruba linia z płynącymi szewronami i dużym grotem, wróg idący na ciebie to przerywana ciemnoczerwona linia, zwarcie to skrzyżowane miecze między oddziałami, cel pod kursorem ma pulsujący pierścień na ziemi i miecze nad sobą.
- Podbój osad rodów:
  - `AOsadaRodu`: bramy z wytrzymałością (`Bramy`, palisada 600, mur kamienny 1400 punktów), `UderzBrame` (poniżej 60% model `_Z1`, przy zerze `_Z2`, kurz, gruz i łoskot), `WMurach`, `PrzyRozbitejBramie`, `Spal` (budynki w murach płoną; po ~12 s gry uszkodzone, po ~35 s się zawalają, po ~80 s gasną; zgliszcza zostają, ród odbudowuje dwa na porę roku i wtedy naprawia bramy). Podczas oblężenia mieszkańcy się chowają.
  - `ABitwa::Wyprawa` (okno Rody, przycisk „Wyprawa wojenna”): powołane ruszenie staje od razu 60 m przed bramą najbliższą waszej osady. Za murem obrońcy: woje rodu w jego barwie (wojowniczy z tarczami, inni z włóczniami), przy 10+ wojach łucznicy przy murze, reszta to chłopi (razem woje + co trzecia rodzina, 6–60). Póki brama stoi, obrońcy czekają (łucznicy strzelają), a mur zatrzymuje oba wojska; atak na obrońców albo marsz w mury prowadzi pod bramę, gdzie pierwszy szereg rąbie wrota (topór 4, włócznia 2,5 punktu na cios). Gdy brama padnie, obrońcy ruszają, twoi wznoszą okrzyk.
  - Po zwycięstwie panel „Osada zdobyta!”: Złupić (połowa zapasów i 60% groszy do waszej osady, ród wrogi), Hołd i danina (lennik: 30% groszy teraz i danina co rok), Spalić (osada płonie, ród traci 1/3 rodzin i 60% zapasów). Liczby rodu zmienia `ARody::PoWyprawie`; przegrana wyprawa: ród wrogi, stosunki −20. Ocaleni wracają od razu pod obóz (bez marszu przez mapę); kamera wraca do domu, poza spaleniem (widać pożar).
  - shortcut: pojedynczy woje w szyku mogą przenikać przez róg muru (mur blokuje środek oddziału), spalone budynki dalej liczą się rodowi w budynkach (traci rodziny i zapasy).
- Dźwięki nowych zdarzeń są podpięte (`Petla_Ogien`, `Zawalenie`, `Krzyk_Panika`, `Brama_Uderzenie`), ale plików jeszcze nie ma: grają cisza, póki nie pobierzemy ich za zgodą (CC0).
- Modele: 110 modeli `_Z1` / `_Z2` (Tools/budynki.py), efekty (Tools/efekty.py, Tools/ue_efekty.py, Tools/Zbuduj_Efekty.bat).
- Testy (bez okna gry): `grarts2d.Wojna.Zniszczenia` (Testy/ZniszczeniaTesty.cpp: palność, model `_Z1` poniżej 70%, chata spala się do ruiny w ~85 s gry, ruina w osadzie i odbudowa, plac budowy znika, ogień przeskakuje po ~12 s) i `grarts2d.Wojna.Podboj` (Testy/PodbojTesty.cpp: dwie bramy, palisada pęka po 61 ciosach toporem i pada po 150, przejście tylko przy rozbitej bramie, 6 budynków spalonych do zgliszcz, naprawa i odbudowa w nowej porze, skutki wyprawy w liczbach rodu). Wszystkie testy przechodzą. W grze jeszcze niesprawdzone.

## Testy, walka jak w Total War, marsz wyprawy i palenie (24.11.2026, prośba: „walka jest trochę dziwna … jednostki się blokują, jedni czekali obok zamiast walczyć … żeby jednostki rzeczywiście szły na wyprawę, a nie się teleportowały … zaznaczyć spal ten budynek … ogień bardziej na budynku … lepsze testy”)
- Galerie testowe (`Dev/Galeria.h/.cpp`, `AGaleria`): **F5** = galeria budynków: każdy model z `/Game/Swiat/Budynki` (zbierane z rejestru zasobów, nowe modele pojawiają się same) w rzędach: etapy F, warianty W, przebudowa U, uszkodzony Z1, ruina Z2, mury, bramy, pola, do tego Natura i Drogi; przed galerią płonący dom i co 12 s zawalenie. **F6** = galeria wojska: każdy rodzaj wojska w rzędzie, w kolumnach każda animacja walki (stoi, idzie, biegnie, ranny, cios / strzał, blok, trafiony, trafiony strzałą, napór, okrzyk, pada od ciosu, pada od strzały, ścięty, bez ręki), zapętlone; rząd narzędzi ruszenia i kobieta. Napisy nad modelami. Drugie naciśnięcie zamyka. Te same przyciski w panelu F3. Zrzut: `-galeria` albo `-galeriawojska` (z `-odl=`).
- Dziennik bitwy (`ABitwa::bDziennik`, przycisk w F3 albo `-bitwadziennik`): co sekundę każdy oddział (rozkaz, cel, żywi, ilu walczy, ilu stoi bezczynnie przy wrogu < 2,5 m, duch, zmęczenie, flanka, stan) w logu `[BITWA]` i w `Saved/Logs/Bitwa_dziennik.txt`; na końcu podsumowanie. Miary (`ABitwa::Miary`) liczone zawsze.
- Test `grarts2d.Wojna.Bitwa` (Testy/BitwaTesty.cpp): bitwa próbna bez okna, liczy odsetek walczących, bezczynnych i czas bitwy (`[BT-TEST]`). Przed poprawką: walczyło 30% żywych, 3,1 bezczynnych przy wrogu, bitwa 156 s; po: 52%, 1,0 i 53 s.
- Walka: gdy oddział już walczy, woj bez przeciwnika idzie do najbliższego wroga, którego nikt (albo jeden) nie bije, do 11 m (`bObchodzi`): nadmiar ludzi owija skrzydła i bije w bok i plecy. Stojący oddział wręcz sam rusza na wroga, który jest bliżej niż 10 m. Sterowanie: 1–9 wybiera oddziały, Ctrl+A wszystkie.
- Wyprawa idzie naprawdę: woje wychodzą z obozu w stronę osady rodu i znikają za kadrem, kolumna idzie po mapie 10–30 s (chorągiew „Wyprawa” z postępem na mapie rodów), wychodzą 90 m przed bramą i marszem stają 60 m przed nią. Dopiero wtedy obrońcy stają w murach. Powrót tak samo (wchodzą do obozu na miejsce zbiórki). Sprzątanie pola przenosi od razu.
- Palenie: z wybranymi oddziałami prawy klik w budynek osady rodu (przy rozbitej bramie albo poza murem) = woje biegną i go podpalają (`AOsadaRodu::DoSpalenia`, `Podpal`). Po wyborze „Spalić” woje rozbiegają się po osadzie, każdy podpala najbliższy cały budynek, a mieszkańcy uciekają (kogo dopadną, tego biją); po ok. 45 s albo 6–10 budynkach wyprawa wraca. `ARody::PoWyprawie` już sam nie pali.
- Ogień na budynku (`AEfekty::PunktPowierzchni`): 65% płomieni liże połacie dachu i ściany (mniejsze, przy powierzchni), połowa iskier to żar, który zostaje na dachu i ścianach; reszta jak dotąd nad dachem. Dotyczy też budynków gracza.
- shortcut: obchodzący woje mogą zbić się w gromadę przy małym wrogu (2 na jednego najwyżej); marsz wyprawy nie omija przeszkód na mapie (kolumna to znak, nie ludzie); test w grze jeszcze nie zrobiony.

## Poprawki błędów (25.11.2026, prośba: „szukaj błędów”)
Przegląd według `INSTRUKCJA_DLA_GPT_BLEDY.md` (awarie, walka i wyprawy, ogień, AI, ekonomia, drogi, UI). Przed zmianami kompilacja i wszystkie testy przechodziły; po zmianach też.
- `Ludzie/Wol.cpp` (`AWol::Tick`): wół z kursem do budynku, który się zawalił (ruina zostaje w świecie, więc wskaźnik był ważny), zrzucał kłody na ruinę i towar przepadał. Teraz ruina i zgliszcza liczą się jak brak celu: wół porzuca kurs, kłody wracają na stos, rezerwacje są zwalniane.
- `Osada/Osada.cpp` (`AOsada::Mezczyzni`) i `Ludzie/Ludzik.cpp` (`Tick`, `Odejdz`): mężczyzna z rodziny, która odchodzi z osady, mógł trafić do pospolitego ruszenia, a w bitwie znikał z pola, gdy doszedł na miejsce w szyku. Odchodzący nie są już brani do ruszenia; gdy rodzina odchodzi w trakcie bitwy, mąż walczy do końca, a po powrocie do obozu odchodzi. Nowa funkcja `ALudzik::Odchodzi()`.
- `Wojna/Bitwa.cpp` (`ABitwa::Odeslij`): „Rozwiąż” w czasie powrotu wyprawy (kolumna ukryta na mapie) wypuszczał ludzi pod osadą rodu, skąd szli kilkadziesiąt km przez mapę. Teraz wracają od razu pod obóz, jak przy rozwiązaniu w czasie wymarszu.
- `Osada/Osada.cpp` (`AOsada::Posilek`): odmiana w ogłoszeniu o braku ubrań („3 rodziny”, nie „3 rodzin”).
- Do decyzji / podejrzane (bez zmian): `ABitwa::bWynik` nigdy nie jest ustawiane na prawdę, więc Enter/Esc „sprząta pole” już nie działa (tablicę wyniku zastąpiła krótka wiadomość; pole sprząta się samo po 90 s); `OminBudynki` przegląda wszystkie budynki świata przy każdym `IdzDo` (możliwy koszt przy 100+ ludzikach); dostawy do budynku w trakcie przebudowy, który się zawalił, przepadają (zamierzone w `ABudowa::Zawal`).

## graphify (25.11.2026, prośba: „pip install graphifyy”)
- Zainstalowane Pythonem z UE do `Tools/_py` (`pip install --target`). Służy do budowania grafu z kodu i dokumentów dla asystenta AI, a w grze nie jest używane. Uruchamia się tak: `PYTHONPATH=Tools/_py`, potem `python -m graphify`. Skrót `graphify.exe` nie powstał, bo `Tools/_py/bin` już istniał. numpy 2.4.6 zostało bez zmian.
- Graf kodu: `python -m graphify extract . --code-only` (lokalnie, bez wysyłania kodu do AI), potem `cluster-only . --no-label`. Wynik trafia do `graphify-out/` (`graph.html`, `GRAPH_REPORT.md`, `graph.json`; w `.gitignore`). `.graphifyignore` pomija Plugins, Paczka, Build, Content, Art i Data. Pierwszy graf: 88 plików, 3943 węzły, 9624 powiązania. Najwięcej powiązań mają ALudzik, ABitwa, AOsada i ABudowa. Aktualizacja po zmianach w kodzie: `python -m graphify update .`.

## Umocnienia gracza, mur AI rośnie, dźwięki osady, życie ludzików (26.11.2026, prośba: „budowanie murów, wież … nie widzę zamków, które budują AI … dźwięki kamieniołomu i piły za głośne … namiot, a słychać 20/50 ludzi … więcej życia w ludzikach”)
Rekrutacja wojsk (zbyt „manorlordsowa”) odłożona na później, użytkownik chce to jeszcze przemyśleć.
- **Umocnienia gracza** (`FRodzajBudynku::Palisada/Mur/WiezaPalisadowa/WiezaMurowana/BramaPalisadowa/BramaMurowana`, zakładka Wojsko): modele z osad rodów, bez faz; model wyrasta z ziemi razem z postępem budowy (`Narasta`, `ABudowa::ZOpuszczenie` w `OdswiezFaze`). Koszty i poziomy: palisada i strażnica/brama drewniana od wsi, mur, baszta i brama miejska od osady targowej. Budują je zwykli budowniczowie i tragarze.
  - Palisada i mur: przytrzymaj lewy przycisk i przeciągnij linię, powstaje szereg odcinków po 4 m (do 40), zielone/czerwone podglądy, puszczenie stawia zielone, wybór zostaje (Esc / prawy klik kończy) — `ABudowniczy::StawianieLinii`. Wieże i bramy stawia się jak zwykłe budynki (R obraca).
  - Umocnienia nie kolidują ze sobą (`AOsada::Koliduje`) i ludzie ich nie omijają (`OminBudynki`): shortcut, ludziki przechodzą przez mur, bramy na razie tylko ozdobne; brak modeli Z1/Z2 sprawdzonych w grze; ikony to znak tarczy (`T_Ikona_Znak_Wojsko_Kat`), prawdziwe wymagają przebudowy `Zbuduj_Budynki.bat`. Odcinek ma początek modelu na końcu, więc obrys kolizji jest przesunięty o 2 m.
- **Mur rodu AI buduje się na oczach** (`AOsadaRodu::Odswiez`): w czasie `FRod::BudujeMury` nowy pierścień rośnie od bramy co 4% postępu (stary stoi na reszcie obwodu), na czole budowy stoi pięciu budowniczych z młotkami (`FMiejsce::bMur`). Wcześniej mur pojawiał się dopiero po skończeniu budowy.
- **Dźwięki**: piła ×0,3 i kilof ×0,4 (reszta ×0,8) w `ALudzik::Animuj`; tło osady zależy od liczby ludzi (`Petla_Wioska` 0,08–0,5, `Petla_Gwar` od ~10 do 70 osób) w `AOsada::Tick`. Osady rodów AI dostały to samo tło i odgłosy pracy (siekiera, młotek, kilof, piła) w `AOsadaRodu::Tick`.
- **Życie wolnych ludzików** (`ALudzik::Tick`, stan Stoi bez pracy): spacer 2–6 m od domu, w 40% wizyta przed innym gotowym budynkiem osady, w 20% podejście do wolnego sąsiada (stanie we dwoje), przerwy 3–12 s.
- Sprawdzone: kompilacja i wszystkie testy automatyczne (AI, Wojna, Drogi). W grze jeszcze nie oglądane.
- Poprawki po przeglądzie (26.11.2026): obrys odcinka muru gracza liczony w tym samym punkcie, w którym jest zapisany (`Polowa.X` 400, sprawdzanie w `P0`); umocnienia gracza są zniszczalne i palne (`Zniszczalny`, modele `_Z1/_Z2`), ale zbóje ich nie podpalają przy grabieży; przy wzroście muru AI nowy cel dostają tylko budowniczowie muru; linia puszczona poza mapą przepada; czas budowy muru w jednym miejscu (`FRod::CzasMuru`); głośność narzędzi w jednym miejscu (`UDzwieki::GrajPrace`); dwaj wolni ludzie nie ganiają się wzajemnie. Test `UkladOsady` sprawdza, że mur w połowie budowy ma 30–70% odcinków. Kompilacja i wszystkie testy zielone. Shortcut: efekty zawalenia odcinka muru liczą środek w jego początku.

## Poprawki po zagraniu, paczki P1 i P2 (26.11.2026, prośba: uwagi z gry w wątku audytu, „rozpocznij prace”)
Kopia Source sprzed zmian: `Kopie/2026-11-26_przed_P1`. Lista 20 uwag i paczek P1–P7 jest w dokumencie „Audyt stanu gry — 26.11.2026” (claude.ai/artifact/8KjCoQ1DKctC21a3hFferR).
- **Kompilacja**: kod z rana 26.11 (umocnienia, dźwięki, życie ludzików) się nie kompilował (`Ludzik.cpp`: zmienna `Cel` zasłaniała pole klasy, C4458), więc w grze działała wersja z 25.11. Zmienna nazywa się teraz `Odwiedza`.
- **Drwal i drzewa borowego**: posadzone drzewo da się ściąć od 70% wzrostu (`ATerenKafle::DoScinki`), wcześniej dopiero od 100%, choć już wyglądało na dorosłe. Wzrost trwa dłużej (`CzasWzrostu` 240 → 360 s gry), borowy pilnuje do 10 młodych drzew (było 16).
- **Borowy nie sadzi na budynkach i drogach** (`AOsada::MiejsceNaDrzewo`): obrócony prostokąt obrysu po pełnym ulepszeniu + 3 m zamiast koła `Promien` (pola i pastwiska są dużo większe niż Promien) i 3 m od drogi (`ADrogi::NaDrodze`).
- **Kolizja ducha jak po pełnym ulepszeniu** (`FRodzajBudynku::NajwyzszyPoziom`, `AOsada::Koliduje`, sprawdzanie dróg w `ABudowniczy::MoznaPostawic`): i stawiany budynek, i stojące liczą się obrysem ostatniego poziomu, więc późniejsze „Ulepsz” nie jest blokowane brakiem miejsca.
- **Woły w zagrodzie** (`AWol::Tick`, krok Pasie): pasą się na całym wolnym wybiegu przed wiatą (`FRodzajBudynku::WybiegMin/Max`, 9,6 × 4,2 m), w miejscu najdalej od innych wołów; wcześniej w kole 3 m przy bramie. Woły startowe też idą na wybieg pierwszej zagrody. Większy model zagrody nie był robiony.
- **Pola pod terenem** (`ABudowa::ZbudujLan`): lan i kępy zboża biorą wysokość jak kafel mapy grywalnej (siatka 4 m, trójkąty jak w `TerenKafle.cpp`), większą z niej i z `FSwiat::Wysokosc`; na wypukłych zboczach lan chował się pod płaskimi trójkątami terenu.
- **Osady rodów na mapie papierowej** (`AOsadaRodu::Tick`): poza mapą grywalną osada rodu jest ukryta, a jej ludzie usunięci (jak osada gracza).
- **Dźwięk**: piła 0,18, kilof 0,22, reszta narzędzi 0,7 (`UDzwieki::GrajPrace`); tło wsi 0,05–0,3 i gwar 0,2 u gracza i w osadach rodów.
- Nie znaleziono pewnej przyczyny „woły przestały pracować”: woły wożą tylko kłody (od drwala do magazynu albo na budowę), więc bez kłód stoją. Do sprawdzenia w grze po tych poprawkach.
- Sprawdzone: kompilacja i wszystkie testy automatyczne (14). W grze jeszcze nie oglądane.

## Mury, wieże i bramy łączą się same (26.11.2026, prośba: „brama i mury powinno dać się łączyć, przeciągać pod różnymi kątami, automatycznie wykrywać przy budowaniu, czy się połączyć”)
- `Budowy/Budowniczy.cpp` (przestrzeń nazw przed `ABudowniczy::Stawianie`): `ZbierzUmocnienia` zbiera węzły umocnień osady gracza (końce odcinków muru, boki bram, środki wież) i linie odcinków; `Zaczep` szuka węzła w promieniu, `NaMurze` punktu na odcinku.
- Przeciąganie muru (`StawianieLinii`): początek i koniec same przyciągają się do węzła w 3,5 m (Alt wyłącza), pod dowolnym kątem; odcinki są równo dzielone (ostatni nie wystaje), więc linia kończy się dokładnie w węźle.
- Wieża przyciąga się do węzła w 4 m albo na mur. Brama „wpina się” w mur: bierze jego kierunek (ten z dwóch, bliższy obrotowi gracza), a odcinki muru pod nią są rozbierane po postawieniu (`ABudowniczy::MurPodBrama`).

## Zapis gry ze slotami, P4 (26.11.2026, uwaga z gry: „nie ma zapisu gry ani slotów”)
- Nowe `Gra/ZapisGry.h/.cpp` (`FZapisGry`): 3 sloty gracza i autozapis (slot 0, co 5 min gry, nie w trakcie bitwy), pliki JSON `Saved/Zapisy/Slot<N>.json` (najpierw `.tmp`, potem podmiana). Moduł `Json` w `grarts2d.Build.cs`.
- Zapisuje: obóz, budynki (rodzaj po modelu i nazwie, pole/owczarnię z wymiarami obszaru, miejsce, obrót, wariant, postęp, przebudowę w toku, zapasy, dostarczony materiał, priorytet, wytrzymałość, uprawę), rodziny (dom, namiot, praca, jedzenie, ubranie, piwo), skarbiec, podatki, dziesięcinę, kalendarz, zadowolenie, poziom osady, zerwane krzaki, polany i sadzonki terenu, drogi, rody AI (wszystkie pola `FRod`) i kamerę. Nie zapisuje trwającej bitwy, ruszenia, ognia, ruin ani bieżących zajęć ludzi (po wczytaniu biorą pracę od nowa).
- Wczytanie przeładowuje poziom: `FZapisGry::DoWczytania` (statyczne), `ASwiatGameMode::StartPlay` pomija menu i wchodzi do regionu w miejscu kamery z zapisu, `ABudowniczy::Tick` po gotowym terenie woła `FZapisGry::Zastosuj`. Linia poleceń: `-wczytaj=N`.
- Menu (`UI/MenuGlowne`, `PokazSloty`): w grze „Zapisz grę” (sloty 1–3, nadpisywanie), „Wczytaj grę” w grze i w menu startowym (też autozapis), opis slotu „pora, rok · rodziny · poziom — data”.
- `friend struct FZapisGry` w `AOsada`, `ABudowa`, `ARody`, `ATerenKafle`, `ADrogi`, `AKameraMapy`; nowe `AOsada::UstawPrace` (rodzina do konkretnego budynku).
- Test `grarts2d.Gra.Zapis` (`Testy/ZapisTesty.cpp`): osada z drwalem z rodziną, placem budowy 40%, polem 30 × 20 m z uprawą i drogą przechodzi zapis i wczytanie bez zmian. Kompilacja i wszystkie testy (15) zielone. W grze jeszcze nie oglądane.

## Budowanie, P3 (26.11.2026, uwagi z gry: „większe składy po ulepszeniu, lepsze dopasowanie do terenu, ładniejsze drogi, pola i owczarnie rysowane punktami w dowolny kształt jak Manor Lords”)
- Składy z poziomami: plac składowy → „Duży plac składowy” (13 × 10 m, 50 sztuk) → „Skład z szopą” (16 × 12 m, 80 sztuk, wymaga osady targowej); spichlerz → „Duży spichlerz” (70 sztuk, dwoje drzwi). Modele `plac_skladowy(faza, poziom)`, `spichlerz(faza, poziom)` w `Tools/budynki.py`, ulepszenie przyciskiem „Ulepsz” jak domy.
- Dopasowanie do terenu: każdy budynek na zboczu dostaje nasyp fundamentu (`ABudowa::Podloze`, skarpa 4–9 m wg spadku), wysokość liczona tak jak trójkąty siatki terenu (`FSwiat::WysokoscKafla`, przeniesione z Budowa.cpp), więc rogi nie wiszą ani nie toną.
- Drogi: „ładniejsze drogi” odczytane jako drogi chowające się pod terenem; wstęga drogi (`ADrogi`) idzie teraz po tej samej wysokości co siatka terenu. Jeśli chodziło o coś innego (kształt, szerokość, bruk), do dopytania.
- Pola i owczarnie dowolnym kształtem: klik stawia róg, klik w pierwszy punkt (2,5 m) albo Enter zamyka kształt, prawy przycisk / Backspace cofa punkt, Esc anuluje, Shift czyści. Obrys rysowany linią na terenie (`ABudowniczy::RysujLinie`). `FRodzajBudynku::NaWielokat` sprawdza kształt (przecinające się boki, za mały / za duży), najdłuższy bok to przód z bramą, plot idzie po obrysie, zboże przycięte do kształtu, owce chodzą tylko po pastwisku, szopa owczarni szuka miejsca w środku. Zapis gry trzyma rogi („Wk”).
- Test `grarts2d.Budowy.Wielokat`: pole w kształcie L (420 m², płot 100 m), klepsydra i mały trójkąt odrzucone, owczarnia z pięciokąta, zapis i wczytanie pola L. Wszystkie testy (16) zielone.

## Bez ekranu wczytywania, oddalenie, minimapa, P5 (26.11.2026)
- Ekran wczytywania tylko przy pierwszym starcie (pod menu) i do zrzutów kontrolnych (`-zrzut`, `-zaslona`). Przejście mapa papierowa ↔ mapa grywalna bez czarnej zasłony, postęp mały w prawym dolnym rogu (`ASwiatGameMode::Zaslon`).
- Większe oddalenie na mapie grywalnej: `MaxOdlRegionu` 1,5e5 → 2,2e5 cm, teren liczony dalej (`PromienRegionu` 5 → 6, 13 × 13 kafli 512 m).
- Minimapa (`UI/Minimapa.h/.cpp`, `SMinimapa`): prawy dolny róg, 210 px. Na mapie papierowej cała Europa ze złotym znakiem osady gracza i rodami w ich barwach (czerwona obwódka = wrogi), na mapie grywalnej ~6 km wokół kamery z budynkami. Obraz z pokrycia terenu, wody i cieniowania stoków (tekstura 112 × 112, przeliczana co 1/4 zakresu). Prostokąt = widok kamery (z obrotem). Klik albo przeciągnięcie przenosi kamerę. Klawisz M (do zmiany w ustawieniach) chowa i pokazuje.

## Okno rozwoju osady, P7 (26.11.2026, uwaga z gry: „okno rozwoju osady, co zrobić, czego brakuje”)
- Klik w panel osady (lewy górny róg) albo O otwiera pod nim okno „Rozwój osady” (`AOsada::RozwojOsady`, odświeżane co 0,5 s w `ABudowniczy::Tick`).
- Sekcje: PILNE (pożar, głód, mało jedzenia, brak drwala i kłód, budowy bez materiału, rodziny w namiotach, zapas na zimę, niskie zadowolenie), CO DALEJ (rodziny bez pracy i puste budynki, brak domów, borowy, pilarnia, kamieniołom, pole i młyn, zbrojownia, pełny skład do ulepszenia) i POZIOM (warunki awansu ✓/✗ i co odblokuje, jak wcześniej w podpowiedzi).

## Wojna po zagraniu, P6 (26.11.2026, uwagi z gry: „nie ma indykatora, co oddziały atakują i ile budynki mają hp … system szkolenia wojska od nowa w zupełnie innym stylu … jednostki rzeczywiście muszą maszerować i walczyć, ma być widać na mapie herb wojska … mamy widzieć, czy ktoś wypowiedział nam wojnę”)
- Szkolenie oddziałów (okno Wojsko, J; `FMilicja::Rodzaj/Wyszkolenie/bCwiczy`, `ABitwa::ZmienRodzaj/PrzelaczCwiczenia/SzkolenieTick`): każdy oddział ma rodzaj (ruszenie, włócznicy, pałkarze, tarczownicy, miecznicy, łucznicy, kusznicy) i przycisk „Ćwiczenia”. Ćwiczący oddział zyskuje wyszkolenie (do 50% w porę roku, wyżej tylko ze zbrojownią, do 100%), je 1 jedzenie na 5 chłopa co pół pory; bez ćwiczeń wyszkolenie spada 10 na porę, zmiana rodzaju zabiera połowę. Przy powołaniu oddział bierze broń z zapasów wg rodzaju (`BroniNaWoja`: włócznia 1, łuk 1, pałka 0, topór z tarczą 2, kusza 2, miecz 3), walczy wartościami rodzaju z `RodzajeWojsk.h`, wyszkolenie daje celność, ducha i doświadczenie (rekruci / wojacy / weterani). Bez broni dla połowy idzie jako ruszenie.
- Wojna z graczem (`FRod::bWojna`, `ARody::Wypowiedz`): ród wypowiada wojnę przy zapowiedzi napadu (i wojowniczy przy odmowie daniny), gracz - wyprawą wojenną. Duży baner na górze ekranu („X wypowiadają wam wojnę!” / „Pokój z rodem X”), w oknie rodów i na mapie „WOJNA z wami”. Koniec: „Zawrzyj pokój” w oknie rodów (30 + 2 grosze na rodzinę, `ZawrzyjPokoj`), danina, hołd. Zapisywane w zapisie gry.
- Armie w marszu (`ARody::FArmia`, `ArmieTick`): napad rodu na ród i na gracza nie liczy się już od razu - armia idzie po mapie (6 s na km, 25–120 s), skutek po dojściu. Pod osadą gracza prawdziwa bitwa (`ABitwa::NapadRodu`): woje w barwie rodu (wojowniczy tarczownicy i miecznicy, kupcy włócznicy i kusznicy, pobożni włócznicy i łucznicy) plus chłopi, od strony osady rodu, plądrują bez obrony; duże armie pomniejszone (do 60 na polu, `SkalaNapadu`). Łup lub straty wracają do rodu (`PoNapadzieNaGracza`). Gdy gracz jest na mapie papierowej, armia czeka pod osadą do 90 s, potem liczby jak dawniej. W testach bez osady gracza po staremu (od razu).
- Herby (`UI/Herb.h/.cpp`, `FHerb`): tarcza z podziałem (w słup, w pas, krzyż, skos, pas, słup, krokiew, w czwór) w barwie rodu i metalu, z nazwy rodu; wasz herb czerwony ze złotą krokwią. Na mapie papierowej przy osadach rodów i na obu mapach przy armiach (`UI/ZnakiArmii`): chorągiew z herbem, ilu ludzi, dokąd i ile sekund, przerywana linia do celu (czerwona, gdy idą na was); wasza wyprawa tak samo.
- Wrogie budynki: klik w budynek lub bramę osady rodu otwiera okienko (nazwa, ród, poziom, życie z paskiem; płonący traci życie na oczach). `AOsadaRodu::DzialkaPod/BramaPod/ZycieDzialki/PlonieDzialka/RodzajDzialki`. W bitwie pod osadą rodu pasek życia nad każdą bramą, linie od atakujących oddziałów do bramy i od palących do budynku z jego paskiem.
- Kompilacja i 16 testów zielone. Zrzut kontrolny (-zrzut -region -ui): minimapa była wyblakła (tekstura bez sRGB), poprawione. W grze nie oglądane: szkolenie, armie, herby, okienko wrogiego budynku.
- shortcut: armie w marszu nie są zapisywane w zapisie gry (po wczytaniu napad zaczyna się od nowej zapowiedzi); wyszkolenie oddziałów też nie (oddziały ruszenia nie są w zapisie).

## Drogi: łączenia, ścieżki do drzwi; wyjście naprzeciw armii (26.11.2026, wieczór)
Prośba: „łączenie i tekstury”, ścieżki do domków jak w Manor Lords, drogi źle się wyginają; wojsko wysyłane na mapie papierowej, żeby wyprzedzić oblężenie.
- `Osada/Drogi.*`: przy innej drodze koleiny i trawa przechodzą w jednolitą ubitą ziemię (`Wstega(..., Ktory)` + `OdInnejDrogi`); na skrzyżowaniach, wjazdach i rozwidleniach łata ubitej ziemi nad wstęgami (`Skrzyzowania`, `Laty`, sekcja `Odcinki.Num()`). Łuki łagodniejsze: styczna najwyżej 0,28 długości i 12 m, a przy skręcie ostrzejszym niż ok. 55° zakręt w miejscu kliknięcia (bez esów).
- Ścieżki: nowy komponent `Sciezki`. Od drzwi każdego budynku (bez murów, pól i pastwisk) do najbliższej drogi w promieniu 30 m biegnie wąska wydeptana ścieżka (`Pasek`, `BudujSciezki`). Odświeżanie: przy zmianie dróg i co 2 s, gdy zmieni się hasz budynków (`SprawdzSciezki`). Ścieżka nie idzie przez cudzy budynek. Budynek przy drodze dalej obraca się drzwiami do niej (`Przyciagnij`). `AOsada::WszystkieBudynki()`.
- Przechwycenie: `ARody::Przechwyc(Id)`, tylko w czasie wojny, dla armii idącej na gracza. Miejsce spotkania to punkt, w którym armia będzie, gdy dojdzie tam wojsko gracza; armia tam staje (`FArmia::bPrzechwycona`, `StopCzas`) i czeka do 120 s. `ABitwa::Przechwyc` prowadzi powołane ruszenie jak wyprawę (`FMarsz::bPole`). `DotarliPole` ustawia bitwę w polu: wasi od strony osady, armia rodu naprzeciw (skład jak przy napadzie, skala dla dużych armii). Po bitwie `ARody::PoPrzechwyceniu`: wygrana rozbija armię i oblężenia nie ma; przegrana oznacza, że armia idzie dalej, mniejsza. Ocaleni wracają do domu.
- Wyzwalanie: klik w chorągiew wrogiej armii na mapie (obie mapy, podpis „kliknij: wyjdź naprzeciw”, `SZnakiArmii::ArmiaPod` w `ABudowniczy::Tick`) albo przycisk „Wyjdź naprzeciw” w oknie rodów.
- Skróty: przechwycenie nie jest w zapisie gry, tak samo jak armie w marszu. Kompilacja i 16 testów zielone. Zrzut kontrolny (`-osada -drogi -odl=6000`) wyszedł szary (mgła albo zły kadr), więc w grze to jeszcze nieoglądane.
