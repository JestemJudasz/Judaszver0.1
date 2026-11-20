# grarts2d — instrukcja dla modelu AI

Zanim cokolwiek zmienisz, przeczytaj `Docs/PROJEKT.md`: jest tam opis tego, co już powstało, za co odpowiada każdy plik, plan etapów i otwarte decyzje. Po każdej zmianie dopisz krótki wpis w `Docs/PROJEKT.md`.

## Zasady
- Użytkownik nie programuje i nie robi assetów. Kod piszesz w C++, a assety tworzysz sam (skrypty Python, Blender przez MCP) albo bierzesz z darmowych źródeł. Każde źródło wpisz do `Docs/Assety_zrodla.md`.
- Styl: realistyczne kształty oglądane z daleka i lekki efekt pikseli, stonowane zielenie, korony drzew w kępach. Wzorem są zrzuty opisane w `Docs/PROJEKT.md`. Nie dodawaj low poly z Kenneya ani niczego, co nie pasuje do tych zrzutów.
- Odpowiadaj po polsku, prosto. Pliki pobierasz z sieci dopiero po zgodzie użytkownika w czacie: podaj nazwę, źródło i rozmiar.
- Nie uruchamiaj gry seriami. Po zmianach skompiluj i zrób najwyżej jeden zrzut kontrolny, a resztę zostaw użytkownikowi.
- Kompilacja: `Kompiluj.bat nopause` (edytor musi być zamknięty), wynik trafia do `Kompilacja.log`.
- Zrzut kontrolny robi `UnrealEditor.exe grarts2d.uproject -game -windowed -zrzut -odl=<cm>`, a obraz ląduje w `Saved/Screenshots`.
- Silnik: `D:/UE_5.8` (UE 5.8, build ze źródeł).

---

## graphify

This project has a knowledge graph at graphify-out/ with god nodes, community structure, and cross-file relationships.

Rules:
- When graphify-out/graph.json exists, run `graphify query "<question>"` before your first grep, search or source-file read for a codebase question, and put this rule in every subagent prompt that explores code. Read raw files only for lines the graph or an earlier read already located. Use `graphify path "<A>" "<B>"` for relationships and `graphify explain "<concept>"` for focused concepts. These return a scoped subgraph, usually much smaller than GRAPH_REPORT.md or raw grep output.
- If graphify-out/wiki/index.md exists, use it for broad navigation instead of raw source browsing.
- Read graphify-out/GRAPH_REPORT.md only for broad architecture review or when query/path/explain do not surface enough context.
- After modifying code, run `graphify update .` to keep the graph current (AST-only, no API cost).
- A committed graphify-out/graph.json is rewritten whole on every build, so a large diff to it in a pull request is expected. Do not flag it as churn or suggest skipping the graph rebuild or its hooks.
