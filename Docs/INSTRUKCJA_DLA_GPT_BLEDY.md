# Instrukcja dla modelu AI: znajdź i napraw błędy w grze grarts2d

Ten plik jest dla Ciebie, modelu AI (GPT, Codex albo innego), który dostał kod gry do przeglądu. Twoje zadanie: **przeszukać kod, znaleźć prawdziwe błędy i naprawić je małymi, bezpiecznymi poprawkami**, nie psując tego, co już działa. Pisz do użytkownika po polsku, prosto. Użytkownik nie jest programistą.

Stan opisany tutaj: 25.11.2026.

---

## 1. Gra w kilku zdaniach

**grarts2d** to strategia średniowieczna w Unreal Engine 5.8 (C++, build ze źródeł w `D:/UE_5.8`). Ma wygląd 3D z lekkim efektem pikseli, prawie izometryczny. Świat to prawdziwa Europa w skali 1:8. Gracz zakłada osadę i ją rozwija (podobnie jak w Manor Lords). Do tego dochodzą rody AI, handel, bitwy w stylu Total War, wyprawy na osady rodów, podpalanie i podbój.

- Folder gry na komputerze użytkownika: `D:\GRY\grarts2d`.
- Kopia kodu na GitHubie: repo `JestemJudasz/Judaszver0.1`, gałąź `claude/project-thread-k5nsde` (kod, konfiguracja, skrypty i dokumentacja z 25.11.2026). Assety (`Content/`, `Art/`) w tej gałęzi są starsze (z 20.11.2026), więc gra z samego repo nie zbuduje wszystkich modeli. Do przeglądu kodu to wystarcza.
- Całość to ok. 30 tys. linii C++ w jednym module `Source/grarts2d/`.
- UI jest pisane w **Slate** (C++), a nie w UMG/Blueprintach. Blueprintów praktycznie nie ma.
- Modele budynków, ludzi i efektów powstają ze skryptów Python (`Tools/*.py`) i są importowane przez pliki `Tools/Zbuduj_*.bat`.

## 2. Przeczytaj najpierw

1. `CLAUDE.md` w katalogu głównym: zasady pracy w projekcie (obowiązują też Ciebie).
2. `Docs/PROJEKT.md`: dziennik projektu (ok. 950 linii). Opisuje każdą mechanikę, każdy plik i każdą decyzję użytkownika. **Zanim uznasz coś za błąd, sprawdź tam, czy to nie jest zamierzone.** Wpisy oznaczone „shortcut:” to świadome uproszczenia, a nie błędy (patrz sekcja 6).
3. `Docs/Audyt_i_plan_rozwoju.md`: audyt z 20.11.2026 (częściowo nieaktualny, bo od tamtej pory doszła cała rozgrywka).

## 3. Mapa kodu (`Source/grarts2d/`)

| Folder / plik | Linie | Za co odpowiada | Ważne klasy i funkcje |
|---|---|---|---|
| `Swiat/SwiatWysokosci.*` | 630 | Wysokość terenu, pokrycie, woda (`FSwiat::Wysokosc`, `Pokrycie`, `Woda`). Czyta `Data/europa_*.r16/.r8`. `GRegion` jest `std::atomic`, bo czytają go wątki robocze | `FSwiat` |
| `Swiat/TerenKafle.*` | 1260 | Teren z kafli quadtree liczonych w tle, drzewa (ISM), polany, karczowanie | `ATerenKafle::Tick`, `Sadz`, `Wykarczuj`, `DodajPolane` |
| `Swiat/SwiatGameMode.*` | 370 | Słońce, niebo, ekran ładowania, tryby zrzutu z linii poleceń | `ASwiatGameMode`, `PoWczytaniu` |
| `Swiat/MapaRzeki.*` | 140 | Rzeki z linii | `AMapaRzeki` |
| `Kamera/KameraMapy.*` | 460 | Kamera jak Google Maps, region, obrót 360° | `AKameraMapy`, `StrefaMapy`, `UstawPiksele` |
| `Osada/Osada.*` | 2600 | **Serce gry gracza**: rodziny, zapasy, posiłki, zadowolenie, przydział do budów, rozwóz, podatki, poziom osady, rozbiórka, ruiny | `AOsada::Tick`, `Przydzielaj`, `Rozwoz`, `RozwozWolami`, `Posilek`, `CelZadowolenia`, `PobierzPodatki`, `Rozbierz`, `Zwroc`, `Zablokowana`, `Grabiez` |
| `Osada/Surowce.h` | 130 | Lista towarów `ESurowiec` i zapasy | `ESurowiec` |
| `Osada/Kalendarz.h` | 37 | Pory roku | |
| `Osada/Drogi.*` | 1040 | Drogi (krzywe Béziera), graf dróg, trasa Dijkstrą, przyciąganie budynków do drogi | `ADrogi::Trasa`, `Przyciagnij`, `PrzyciagnijKoniec`, `Krzywa` |
| `Osada/Rody.*` | 1590 | **AI rodów** liczone liczbami: decyzje budowy, wojsko, władca (napady, danina, pokój), bitwy liczbami, dziennik AI, strażnik zastoju, poziomy trudności | `ARody::Decyduj`, `Produkuj`, `Wladca`, `Bitwa`, `Przewin`, `Pilnuj`, `PoWyprawie`, `FRod` |
| `Osada/OsadaRodu.*` | 1530 | Widoczna osada rodu: układ ulic i działek, mury, bramy, ludziki przy kamerze, palenie, oblężenie | `AOsadaRodu::Odswiez`, `Tick`, `UderzBrame`, `Spal`, `DoSpalenia`, `WMurach`, `PrzyRozbitejBramie` |
| `Budowy/Budowa.*` | 3110 | Budynek: rodzaje (`FRodzajBudynku`), fazy budowy, ulepszenia, magazyn, wytrzymałość, ogień, zawalenie, ruina | `ABudowa::Tick`, `Ulepsz`, `AnulujUlepszenie`, `Uszkodz`, `Podpal`, `Gas`, `Palnosc` |
| `Budowy/Budowniczy.*` | 2280 | Sterowanie graczem: stawianie budynków, pola/pastwiska prostokątem, obrót, klawisze, start gry, „Postaw wszystko” | `ABudowniczy::Tick`, `BeginPlay`, `PostawWszystko`, `ObrocKlawiszem` |
| `Ludzie/Ludzik.*` | 3310 | **Każdy człowiek**: chodzenie, praca w każdym budynku, noszenie, pomoc przy budowie, woj, tryb walki, animacje | `ALudzik::Tick`, `IdzDo`, `Zwolnij`, `Pomoz`, `ZadanieWoja`, `ZadanieRybaka`, `WrocZBitwy`, `WalkaIdz`, `WalkaCios`, `OminBudynki` |
| `Ludzie/Wol.*` | 360 | Woły i konie z wozami | `AWol::Ciagnij`, `UstawKonia` |
| `Wojna/Bitwa.*` | 4200 | **Najbardziej złożony plik**: oddziały, szyki, zwarcie, morale, ucieczka, strzelcy i pociski, ruszenie, napad bandytów, wyprawa na ród (marsz, brama, palenie), dziennik bitwy | `ABitwa::Tick`, `Wyprawa`, `DodajRuszenie`, `Strzelanie`, `PociskiTick`, `Miary`, `FOddzial`, `FWojownik`, `FMilicja` |
| `Wojna/RodzajeWojsk.h` | 60 | Statystyki rodzajów wojsk | |
| `Wojna/OknoWojska.*`, `Wojna/PanelBitwy.*` | 920 | Okno wojska (J), panel oddziałów | |
| `Efekty/Efekty.*` | 600 | Ogień, dym, iskry, kurz, gruz (cząstki na CPU w 5 instancjach) | `AEfekty::W`, `Pozar`, `Zawal`, `PunktPowierzchni` |
| `Dzwiek/Dzwieki.*` | 250 | System dźwięku (brakujący plik = cisza, to nie błąd) | `UDzwieki` |
| `UI/*` | 2800 | Slate: pasek budowy, okno budynku, okno rodów (H), panel F3, menu główne, ustawienia klawiszy | `SPasekBudowy`, `SOknoBudynku`, `SOknoRodow`, `SPanelDeweloperski`, `FTrybDeweloperski`, `FUstawienia::Klawisz` |
| `Dev/Galeria.*` | 650 | Galerie testowe F5 (budynki) i F6 (wojsko) | `AGaleria` |
| `Testy/*.cpp` | 1000 | Testy automatyczne UE (patrz sekcja 5) | |

**Współrzędne:** +X to północ, +Y to wschód, jednostka to cm. Ludzik ma w grze ok. 2,4 m (`ALudzik::Skala` 1,4).

**Wskaźniki:** kod trzyma aktory przez `TWeakObjectPtr` (np. `AOsada::Budynki`, `Ludzie`, `FRodzina::Maz/Dom/Praca`, `ALudzik::Budowa/Zrodlo/Stala`), a komponenty przez `UPROPERTY() TObjectPtr`. Słabe wskaźniki mogą w każdej chwili zrobić się puste (budynek spłonął, ludzik zginął). To najczęstsze źródło awarii, patrz sekcja 7.

## 4. Zasady pracy (obowiązkowe)

1. **Nie zmieniaj zachowania gry, które działa i jest opisane w `Docs/PROJEKT.md`.** Naprawiasz błędy, nie przeprojektowujesz. Balans (liczby, czasy, ceny) zmieniaj tylko wtedy, gdy liczba jest ewidentnie błędna (np. dzielenie przez zero, ujemny zapas), a nie dlatego, że inna wydaje Ci się lepsza.
2. **Małe poprawki.** Jedna usterka = jedna poprawka w jak najmniejszej liczbie linii. Bez refaktoryzacji, bez zmiany nazw, bez przenoszenia kodu między plikami.
3. **Zachowaj styl kodu:** polskie nazwy bez ogonków w identyfikatorach (`Osada`, `Budowa`, `Ludzik`), komentarze po polsku z datą i powodem, np. `// Poprawka 25.11.2026: ...`. Teksty dla gracza są po polsku z ogonkami.
4. **Te same reguły dla gracza i AI.** Jeśli poprawiasz zasadę ekonomii albo wojny, sprawdź, czy ta sama zasada jest w `Rody.cpp` / `OsadaRodu.cpp`.
5. **Czas gry:** mechaniki zależne od czasu muszą liczyć z `DeltaSeconds` (z dylatacją), a nie z `FApp::GetDeltaTime()`, żeby działało przyspieszenie x2–x10 i pauza z panelu F3. Wyjątki są zamierzone i opisane w kodzie: obrót budynku (`ABudowniczy::ObrocKlawiszem`) i zwolnienie czasu w bitwie (`ABitwa::Tick`, `Zwolnienie`) liczą w czasie rzeczywistym. Kamera i `ABudowniczy` mają `CustomTimeDilation` jako odwrotność globalnej.
6. **Nie dodawaj nowych funkcji**, assetów ani zależności. Nie pobieraj niczego z sieci.
7. **Nowy plik `.cpp`** wymaga skasowania `Intermediate/Build/Win64/x64/grarts2dEditor/Development/Makefile.bin` przed kompilacją, inaczej UBT go nie widzi. Najlepiej nie twórz nowych plików, poza testami.
8. **Po każdej serii poprawek dopisz wpis w `Docs/PROJEKT.md`** (nagłówek `## Poprawki błędów (data)`, lista: plik, co było źle, co zmienione).
9. Jeśli nie masz pewności, czy coś jest błędem, **nie zmieniaj tego**: wpisz to do raportu jako „podejrzane” (sekcja 9).

## 5. Jak budować i testować

Wszystko na Windows, w folderze `D:\GRY\grarts2d`, **przy zamkniętym Unreal Editorze**.

- **Kompilacja:** `Kompiluj.bat nopause`. Wynik w `Kompilacja.log` (szukaj `error`; na końcu `Result: Succeeded`).
- **Testy automatyczne:** `Tools\Testy.bat nopause` (uruchamia `UnrealEditor-Cmd.exe ... -ExecCmds="Automation RunTests grarts2d" -nullrhi`). Pełny log: `Saved\Testy.log`. Skrypt wypisuje linie `Test Completed`, `Expected`, `[AI-TEST]` i `ZAST` (zastój AI).
  - `grarts2d.Swiat.*` (`Testy/SwiatTesty.cpp`): dane Europy, mapa grywalna.
  - `grarts2d.Drogi.Luk` (`Testy/DrogiTesty.cpp`).
  - `grarts2d.AI.Bramka`, `AI.Scenariusze`, `AI.Wojsko`, `AI.Wladca`, `AI.Mury`, `AI.Balans` (`Testy/AITesty.cpp`): rody AI przez 5–10 lat gry w ułamku sekundy. Wszystkie są obowiązkowe i przechodziły 23.11.2026.
  - `grarts2d.AI.UkladOsady` (`Testy/OsadaRoduTesty.cpp`): układ osady rodu, mury, bramy. Zapisuje `Saved/Plany/<ród>.txt`, a `Tools/plan_osady.py` rysuje z tego PNG.
  - `grarts2d.Wojna.Zniszczenia` (`Testy/ZniszczeniaTesty.cpp`): palność, ruina, odbudowa, przeskok ognia.
  - `grarts2d.Wojna.Podboj` (`Testy/PodbojTesty.cpp`): bramy, spalenie osady, skutki wyprawy.
  - `grarts2d.Wojna.Bitwa` (`Testy/BitwaTesty.cpp`): bitwa bez okna, linie `[BT-TEST]` (odsetek walczących, bezczynnych, czas bitwy). Wynik 24.11.2026: 52% walczących, 1,0 bezczynnych przy wrogu, 53 s.
  - **Zasada:** wszystkie testy, które przechodziły, muszą przechodzić po Twoich poprawkach. Gdy poprawiasz błąd, który da się sprawdzić bez okna gry, dopisz test do istniejącego pliku w `Testy/` (wzoruj się na sąsiednich testach).
- **Zrzut kontrolny** (najwyżej jeden po serii poprawek, nie uruchamiaj gry seriami): `D:\UE_5.8\Engine\Binaries\Win64\UnrealEditor.exe grarts2d.uproject -game -windowed -zrzut -region -osada -ui -odl=7000`. Obraz trafia do `Saved/Screenshots` albo `Saved/zrzut_*.png`.
- Jeśli nie możesz sam uruchamiać programów na komputerze użytkownika, **podaj mu dokładne polecenia** i poproś o wklejenie końca `Kompilacja.log` oraz linii z `Saved\Testy.log`.

### Narzędzia w grze do szukania błędów

| Co | Jak włączyć | Do czego |
|---|---|---|
| Panel deweloperski | **F3** (przycisk w prawym dolnym rogu) | budowa natychmiast, czas Pauza/x1/x2/x5/x10, +100/+1000 surowców, „Postaw wszystko” (22 budynki i rodziny w pracy), „Dziennik AI”, „Dziennik bitwy”, „Trudność AI”, galerie |
| Galeria budynków | **F5** albo `-galeria` | każdy model: fazy, warianty, ulepszenia, uszkodzony `_Z1`, ruina `_Z2`, mury, bramy; płonący dom i co 12 s zawalenie |
| Galeria wojska | **F6** albo `-galeriawojska` | każdy rodzaj wojska w każdej animacji walki |
| Dziennik AI | F3 albo `-aidziennik` | linie `[AI] rok pora / ród / rodziny, woje, budynki / zapasy / decyzja / powód` w logu i `Saved/AI_dziennik.csv`; `ZASTÓJ` = ród 2 lata bez postępu |
| Dziennik bitwy | F3 albo `-bitwadziennik` | co sekundę każdy oddział (`[BITWA]`) w logu i `Saved/Logs/Bitwa_dziennik.txt`, podsumowanie na końcu |
| Log ogólny | `Saved/Logs/grarts2d.log` | `UE_LOG(LogTemp, ...)`: m.in. posiłki osady, ostrzeżenia, błędy silnika (`Error`, `Ensure`, `Accessed None`) |

Klawisze w grze (zmienialne w ustawieniach, `UI/Ustawienia.cpp`): WASD kamera, Q/E obrót, B budowa, G drogi, J wojsko, H rody i handel, R/F obrót budynku, T tempo budowy x10. W bitwie: Spacja pauza, R odwrót, F szyk, K krew, 1–9 wybór oddziałów, Ctrl+A wszystkie. N bitwa próbna, P napad bandytów.

Parametry linii poleceń (przydatne do odtworzenia błędu): `-game -region -osada -rody -wszystko -napad -bitwa -drogi -tempo=10 -trudnosc=0..2 -latarodow=N -widokrodu -czekaj=S -odl=CM -zrzut -ui`.

## 6. Co NIE jest błędem (świadome uproszczenia z `Docs/PROJEKT.md`)

Nie „naprawiaj” tych rzeczy, chyba że powodują awarię:
- Napad rodu AI na gracza liczony z siły, bez prawdziwej bitwy przy kamerze.
- Ludziki w osadach rodów tylko udają pracę (towar liczy `ARody` liczbami).
- Pojedynczy woje w szyku mogą przenikać przez róg muru (mur blokuje środek oddziału).
- Spalone budynki dalej liczą się rodowi w budynkach.
- Dźwięki bez plików (`Petla_Ogien`, `Zawalenie`, `Krzyk_Panika`, `Brama_Uderzenie`) grają ciszę.
- Koła wozu się nie kręcą. Jęczmienia nie ma (słodownia bierze zboże). Wędzarnia odłożona.
- Stawianie budynku bez materiału jest dozwolone (jak w Manor Lords).
- Ocaleni z przegranej wyprawy wracają od razu pod obóz.

## 7. Gdzie szukać błędów (kolejność według ryzyka)

Te obszary są największe, najnowsze (21–24.11.2026) i najmniej sprawdzone w grze. Dla każdego obszaru: najpierw przeczytaj odpowiedni wpis w `Docs/PROJEKT.md`, potem kod.

### 7.1 Awarie: puste wskaźniki i zniszczone aktory (najwyższy priorytet)
- Każde użycie `TWeakObjectPtr` (`.Get()`, `->`) bez `IsValid()` / sprawdzenia `nullptr`, zwłaszcza po zdarzeniach, które usuwają aktory: spalenie, zawalenie (`ABudowa`), rozbiórka (`AOsada::Rozbierz`), anulowanie przebudowy, śmierć w bitwie, rozwiązanie oddziału, sprzątanie pola bitwy (Enter), usunięcie ludzików osady rodu po oddaleniu kamery (`AOsadaRodu::Tick`), zamknięcie galerii F5/F6.
- Ludzik, który ma zadanie u budynku, który właśnie zniknął: `ALudzik::Budowa`, `Zrodlo`, `Stala`, `PracaPrzed`; rodzina z `Dom`/`Praca`/`Namiot` wskazującym na ruinę.
- Wół/koń (`AWol`) z kursem do budowy albo magazynu, który zniknął w trakcie (rezerwacje `Zarezerwowane` / `WDrodze`).
- Slate: okna (`SOknoBudynku`, `SOknoRodow`, `SOknoWojska`, `SPanelDeweloperski`) trzymają wskaźniki do aktorów. Sprawdź, co się dzieje, gdy budynek, którego okno jest otwarte, spłonie albo zostanie rozebrany, i gdy świat się zmienia (wejście/wyjście z regionu, menu główne).
- Lambdy (`FTimerDelegate`, `CreateLambda`, `OnClicked`) łapiące `this` albo surowe wskaźniki do aktorów, które mogą zniknąć przed wywołaniem.
- Indeksy tablic bez `IsValidIndex` (w `.cpp` jest ok. 54 miejsc z `[0]`), `RemoveAt` w pętli idącej do przodu (ok. 22 wywołań `RemoveAt`/`RemoveAtSwap`), indeksy oddziałów i wojowników w `ABitwa` (`FCios { OA, WA, OE, WE }`) po usunięciu oddziału albo poległego.
- Dzielenie przez zero: liczba rodzin, oddziałów, wojowników, długość wektora przy normalizacji (`GetSafeNormal` zamiast `/ Size()`).

### 7.2 Walka i wyprawy (`Wojna/Bitwa.cpp`, `Osada/OsadaRodu.cpp`)
- Jednostki, które się blokują albo stoją bezczynnie przy wrogu (`bObchodzi`, wybór najbliższego wroga do 11 m, samoczynny atak poniżej 10 m). Miarą jest test `grarts2d.Wojna.Bitwa` i dziennik bitwy.
- Koniec bitwy: czy zawsze się kończy (ucieczka, wszyscy polegli, odwrót), czy ludzie wracają do pracy (`ALudzik::WrocZBitwy`, `ZadanieWoja`), czy czas gry wraca do normy (`SetGlobalTimeDilation` po `Zwolnienie`, pauza bitwy vs panel F3).
- Wyprawa: marsz w obie strony (`FMarsz`), stan po przegranej, po zamknięciu okna, po zapisie czasu x10; brama (`UderzBrame`, `Bramy`), wejście do murów tylko przy rozbitej bramie, panel „Osada zdobyta!” (Złupić / Hołd / Spalić) i `ARody::PoWyprawie`: czy skutki nie liczą się dwa razy.
- Palenie: `DoSpalenia`, `Podpal`, rozbiegnięcie się wojów po „Spalić”; czy wyprawa wraca po ~45 s albo 6–10 budynkach także wtedy, gdy nie ma już czego palić.
- Ruszenie (`FMilicja`, `DodajRuszenie`, `WolniMezczyzni`): czy ten sam mężczyzna nie trafia do dwóch oddziałów i czy po rozwiązaniu wraca do swojej pracy i rodziny.
- Pociski (`Strzelanie`, `PociskiTick`): cel, który zginął w locie.

### 7.3 Ogień i zniszczenia (`Budowy/Budowa.cpp`, `Efekty/Efekty.cpp`)
- Przeskok ognia przegląda wszystkie budynki świata (`TActorIterator<ABudowa>`) co ok. 2 s dla każdego płonącego budynku: poprawność (czy nie podpala ruin, zgliszcz, podglądu budowy) i wydajność przy wielu pożarach.
- Zawalenie: podmiana modelu na `_Z2`, ruina w `AOsada::Ruiny`, „Odbuduj” i „Uprzątnij” (zwrot 1/3 kamienia), czy rodzina i pracownicy zostają zwolnieni z budynku, który się zawalił, czy zapasy w magazynie znikają poprawnie z licznika osady.
- `AEfekty`: limit świateł (6), wyłączanie `Tick`, gdy nic nie płonie, sprzątanie po zakończeniu pożaru.

### 7.4 AI rodów (`Osada/Rody.cpp`)
- `Decyduj`: zastoje (ród bez postępu), pętla „wytwórca braku” (łańcuch do 3 kroków), czy ród może wydać więcej, niż ma (ujemne zapasy).
- `Wladca`: napady, rozejmy, danina, napad na gracza od 2. roku z ostrzeżeniem porę wcześniej; stosunki `FRod::Wobec` w zakresie i powrót do bazy.
- `Bitwa` liczbami: przegrany zostaje z min. 2 rodzinami, wojów nie ubywa poniżej zera.
- Losowość: w `Rody.cpp` ma być `ARody::Los` (`FRandomStream`), nie `FMath::FRand`, bo testy wymagają powtarzalności przy tym samym ziarnie.
- `PoraRodow()` bez `AOsada` (własny kalendarz).

### 7.5 Ekonomia osady (`Osada/Osada.cpp`, `Ludzie/Ludzik.cpp`)
- Zapasy nigdy ujemne; towar w drodze (`Zarezerwowane`, `WDrodze`) zwolniony, gdy tragarz/wół zrezygnuje, zginie albo budowa zniknie. Zwrot przy rozbiórce i anulowaniu przebudowy bez podwajania (`AOsada::Zwroc`).
- `Przydzielaj`: priorytety budów, zablokowane budowy, pomocnicy (`BezRoboty` po 20 s, `DoPomocy`, powrót do `Stala`).
- `Posilek`: jedzenie + ryby, opał zimą (najpierw opał, potem kłoda), ubranie, buty, piwo; zadowolenie przycięte do 0–100; odchodzenie i przychodzenie rodzin (min. 2 rodziny).
- `PobierzPodatki` przy każdej nowej porze roku tylko raz; dziesięcina tylko z kościołem.
- Poziom osady (`Poziom`, tylko rośnie) i blokady budynków (`Wymaga`, `CzemuZablokowany`).
- Ludzik bez celu: zawieszony w miejscu na zawsze, chodzi w kółko, utknął przy budynku (`OminBudynki`, przeszkody z obrysów).

### 7.6 Drogi i chodzenie (`Osada/Drogi.cpp`, `Ludzie/Ludzik.cpp`)
- `ADrogi::Trasa` (Dijkstra) dla celów dalej niż 25 m: graf bez połączenia, pusty graf, start/koniec poza drogą.
- `OminBudynki` przegląda wszystkie budynki przy każdym wyznaczaniu trasy (`TActorIterator<ABudowa>`): sprawdź koszt przy 100+ ludzikach.
- Usuwanie odcinka (Ctrl + klik) w czasie, gdy ludzik idzie tą drogą.

### 7.7 Interfejs (`UI/*`)
- Przyciski, które działają na nieistniejącym obiekcie (patrz 7.1), podwójny klik „Rozbierz”, przyciski −/+ (priorytet 1..5, rodziny w pracy) poza zakresem.
- Teksty: brakujące ogonki, ucięte napisy, liczby bez odmiany („1 rodzin”).
- Ustawienia klawiszy (`FUstawienia`): dwa działania na jednym klawiszu (R i F są używane i do obrotu budynku, i w bitwie: to zamierzone, ale sprawdź, czy nie działają oba naraz).

### 7.8 Wydajność (cel: 60 FPS na RTX 4060)
- `TActorIterator` wywoływany w `Tick` albo w pętli po ludzikach (lista miejsc: `Budowa.cpp`, `Ludzik.cpp`, `Bitwa.cpp`, `Osada.cpp`, `OknoRodow.cpp`, `PanelDeweloperski.cpp`, `OknoWojska.cpp`).
- Przebudowa siatek co klatkę (`ProceduralMeshComponent`, ISM) zamiast przy zmianie.
- `AOsadaRodu::Odswiez` (5–30 ms według testu) wywoływane częściej niż przy zmianie.
- Logi (`UE_LOG`) co klatkę poza dziennikami włączanymi z F3.

### 7.9 Teren i świat (niski priorytet, ten kod jest najstarszy i sprawdzony)
- Wątki robocze kafli (`ATerenKafle`): dostęp do danych współdzielonych bez synchronizacji.
- Wejście/wyjście z regionu (ekran ładowania) i powrót do mapy Europy.

## 8. Checklista przeglądu każdego pliku

Dla każdego pliku z sekcji 3 (zacznij od największych: `Bitwa.cpp`, `Ludzik.cpp`, `Budowa.cpp`, `Osada.cpp`, `Budowniczy.cpp`, `OsadaRodu.cpp`, `Rody.cpp`):

- [ ] Każdy `TWeakObjectPtr` sprawdzony przed użyciem; każdy surowy wskaźnik do aktora sprawdzony `IsValid()`.
- [ ] Brak dostępu do tablicy bez sprawdzenia indeksu; brak usuwania z tablicy w pętli idącej do przodu.
- [ ] Brak dzielenia przez zero i normalizacji wektora zerowego.
- [ ] Czas liczony z `DeltaSeconds` (poza opisanymi wyjątkami); pauza (dylatacja 0) nie psuje liczników.
- [ ] Zasoby: każda rezerwacja ma zwolnienie; każdy zwrot liczony raz; zapasy nie schodzą poniżej zera.
- [ ] Stan ludzika/oddziału zawsze ma wyjście (brak stanu, z którego nie da się wrócić).
- [ ] Usunięcie aktora czyści wszystkie odwołania do niego (listy w `AOsada`, `ABitwa`, `AOsadaRodu`, otwarte okna UI).
- [ ] Brak ciężkich pętli po wszystkich aktorach w `Tick`.
- [ ] `Super::` wywołane w `BeginPlay`, `Tick`, `EndPlay`; timery i delegaty czyszczone w `EndPlay`.
- [ ] Zasada „te same reguły dla gracza i AI” zachowana.
- [ ] Kod się kompiluje i wszystkie testy przechodzą.

## 9. Format raportu

Zanim zaczniesz zmieniać kod, oddaj użytkownikowi **listę znalezionych usterek**, a potem naprawiaj je po kolei według priorytetu. Każda usterka w tym formacie:

```
### [P1] Krótki tytuł usterki
- Plik i linia: Source/grarts2d/Wojna/Bitwa.cpp:1234 (funkcja ABitwa::NazwaFunkcji)
- Co jest źle: jedno, dwa zdania technicznie.
- Kiedy to widać w grze: prostymi słowami, np. „gra się wyłącza, gdy dom spłonie, a jego okno jest otwarte”.
- Jak odtworzyć: kroki albo parametry, np. „-game -region -wszystko, F3 → x10, P (napad)”.
- Poprawka: co zmieniasz (diff albo opis zmiany w 1–5 liniach).
- Pewność: pewne / prawdopodobne / podejrzane (do sprawdzenia w grze).
- Sprawdzone: kompilacja tak/nie, testy tak/nie, test dopisany: nazwa albo „brak”.
```

Priorytety:
- **P1** – awaria gry, zawieszenie, utrata postępu, pętla bez końca.
- **P2** – mechanika działa źle: towar znika albo się podwaja, ludzie albo oddziały stoją na zawsze, bitwa się nie kończy, AI staje.
- **P3** – wydajność (spadek FPS, przycięcia).
- **P4** – drobne: UI, teksty, wygląd, zła odmiana słów.

Na końcu krótkie podsumowanie dla użytkownika po polsku, bez żargonu: ile usterek znalazłeś, ile naprawiłeś, co wymaga sprawdzenia w grze i jakie polecenie ma uruchomić (kompilacja, testy).

## 10. Kolejność pracy

1. Przeczytaj `CLAUDE.md` i `Docs/PROJEKT.md` (przynajmniej tabelę plików i wpisy od 21.11.2026).
2. Upewnij się, że kod się kompiluje (`Kompiluj.bat nopause`) i zapisz wynik testów **przed** zmianami (`Tools\Testy.bat nopause`). Jeśli coś nie przechodzi już na starcie, zapisz to w raporcie jako stan wyjściowy.
3. Przejrzyj obszary z sekcji 7 w podanej kolejności, z checklistą z sekcji 8.
4. Oddaj raport (sekcja 9).
5. Naprawiaj od P1 do P4, po jednej usterce. Po każdej grupie: kompilacja i testy.
6. Dopisz wpis do `Docs/PROJEKT.md`.
7. Na koniec jeden zrzut kontrolny albo poproś użytkownika, żeby sprawdził w grze konkretne miejsca (podaj klawisze i kroki).

Pamiętaj: lepiej naprawić 10 prawdziwych błędów małymi zmianami niż przepisać pół gry. Jeśli poprawka wymaga zmiany projektu gry, opisz ją i zapytaj użytkownika.
