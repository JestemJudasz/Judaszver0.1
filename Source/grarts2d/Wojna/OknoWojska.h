// Plik: Source/grarts2d/Wojna/OknoWojska.h
// Okno Wojsko (21.11.2026, klawisz J albo przycisk pod panelem osady), wzor: zakladka wojska w Manor Lords.
//   - Ilu mezczyzn w osadzie, ilu wolnych (poza oddzialami), ile wloczni w zapasach; ostrzezenie, gdy nadchodza bandyci.
//   - Oddzialy pospolitego ruszenia (do 30 chlopa): w domu / w polu (stan, duch). Przyciski: Powolaj (rzucaja prace,
//     biora wlocznie z zapasow, reszta narzedzia), Rozwiaz (wracaja do pracy), Uzupelnij (wolni mezczyzni), Usun.
//   - Nowy oddzial z wolnych mezczyzn, Powolaj wszystkie, Rozwiaz wszystkie.
// Budowanie i praca reszty osady trwaja dalej; powolanymi steruje sie jak w bitwie (Wojna/PanelBitwy).
#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"
#include "Styling/SlateTypes.h"

class ABitwa;
class SVerticalBox;

class SOknoWojska : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SOknoWojska) {}
		SLATE_ARGUMENT(TWeakObjectPtr<ABitwa>, Bitwa)
		SLATE_EVENT(FSimpleDelegate, NaZamknij)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);
	virtual void Tick(const FGeometry& G, const double Czas, const float Dt) override;

private:
	void Wiersze();
	TWeakObjectPtr<ABitwa> Bitwa;
	FSimpleDelegate NaZamknij;
	TSharedPtr<SVerticalBox> Lista;
	int32 Wersja = -1;
	FButtonStyle Styl;
};
