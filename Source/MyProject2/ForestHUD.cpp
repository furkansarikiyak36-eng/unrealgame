// Copyright Epic Games, Inc. All Rights Reserved.

#include "ForestHUD.h"
#include "ForestCharacter.h"
#include "HarvestableActor.h"
#include "LogWallBlueprint.h"
#include "LogConstructionActor.h"
#include "StoneMasonryActor.h"
#include "PickupItem.h"
#include "CarvableLog.h"
#include "SurvivalComponent.h"
#include "MechanicsDemo.h"
#include "Campfire.h"
#include "StructureLink.h"
#include "EngineUtils.h"
#include "Engine/Canvas.h"
#include "Engine/Engine.h"
#include "Engine/Font.h"

namespace
{
	/**
	 * Değer arttıkça renk Kırmızı (0.0) -> Sarı (0.5) -> Canlı Yeşil (1.0) yönünde dinamik değişir.
	 */
	FLinearColor GetValueGradientColor(float NormalizedValue)
	{
		NormalizedValue = FMath::Clamp(NormalizedValue, 0.0f, 1.0f);
		if (NormalizedValue < 0.5f)
		{
			return FMath::Lerp(FLinearColor(0.95f, 0.2f, 0.15f, 0.95f), FLinearColor(0.95f, 0.8f, 0.2f, 0.95f), NormalizedValue * 2.0f);
		}
		else
		{
			return FMath::Lerp(FLinearColor(0.95f, 0.8f, 0.2f, 0.95f), FLinearColor(0.2f, 1.0f, 0.35f, 0.95f), (NormalizedValue - 0.5f) * 2.0f);
		}
	}
}

void AForestHUD::DrawPanel(float X, float Y, float Width, float Height)
{
	DrawRect(FLinearColor(0.f, 0.f, 0.f, 0.45f), X, Y, Width, Height);
}

void AForestHUD::DrawHUD()
{
	Super::DrawHUD();

	const AForestCharacter* Player = Cast<AForestCharacter>(GetOwningPawn());
	if (!Player || !Canvas)
	{
		return;
	}

	UFont* Font = GEngine->GetMediumFont();
	UFont* SmallFont = GEngine->GetSmallFont();
	const float CenterX = Canvas->ClipX * 0.5f;
	const float CenterY = Canvas->ClipY * 0.5f;
	const FLinearColor White(1.f, 1.f, 1.f, 0.9f);

	// Crosshair
	DrawRect(White, CenterX - 2.f, CenterY - 2.f, 4.f, 4.f);

	// Target under the crosshair
	if (Player->GetFocusedCarvable() && Player->GetCarriedItem() == EForestItem::None)
	{
		const ECarveTool Tool = Player->GetCarveTool();
		FString Detail;
		if (Tool == ECarveTool::Notch)
		{
			Detail = Player->GetCarveAngle() < 1.f ? FString(TEXT("boyuna kanal")) : FString::Printf(TEXT("açı %.0f°"), Player->GetCarveAngle());
		}
		else if (Tool == ECarveTool::Split)
		{
			Detail = FString::Printf(TEXT("açı %.0f°"), Player->GetCarveAngle());
		}
		const bool bProfile = AForestCharacter::IsProfileTool(Tool);
		if (bProfile)
		{
			static const TCHAR* Depths[] = { TEXT("çapın 1/6'sı"), TEXT("çapın 1/3'ü"), TEXT("çapın yarısı") };
			Detail = FString::Printf(TEXT("%.0f cm geniş, %s derin (%.1f cm)"), Player->GetCarveWidth(),
				Depths[Player->GetCarveDepthIndex()], Player->GetCarveDepth());
		}
		if (!Player->IsCarveSpotValid())
		{
			Detail = TEXT("bu kütükte yer kalmadı");
		}
		if (Player->GetFocusedCarvable()->GetDamage() > 0.01f)
		{
			Detail += FString::Printf(TEXT("  |  hasar %%%.0f"), Player->GetFocusedCarvable()->GetDamage() * 100.f);
		}
		if (Player->GetFocusedCarvable()->IsTemporary())
		{
			Detail += TEXT("  |  geçici (iskele)");
		}
		if (Player->GetFocusedCarvable()->IsBurning())
		{
			Detail += FString::Printf(TEXT("  |  YANIYOR %.0f sn"), Player->GetFocusedCarvable()->GetBurnSecondsLeft());
		}
		else if (Player->GetFocusedCarvable()->IsCharred())
		{
			Detail += TEXT("  |  yanık");
		}
		const TCHAR* Wheel = bProfile ? TEXT("[Tekerlek] derinlik")
			: Tool == ECarveTool::CrossCut ? TEXT("65 cm'lik modül çizgileri")
			: AForestCharacter::IsEndTool(Tool) ? TEXT("kütüğün iki ucundan biri")
			: Tool == ECarveTool::Split ? TEXT("[Tekerlek] dikey/yatay") : TEXT("[Tekerlek] köşe/oluk");
		const FString Label = FString::Printf(TEXT("Oyma: %s  %s   vuruş %d/2   |   [R] araç   %s"),
			AForestCharacter::GetCarveToolName(Tool), *Detail, Player->GetCarveHits(), Wheel);
		// A piece of a structure says what the structure is.
		const FString Made = StructureLink::DescribeFeaturesOf(GetWorld(), Player->GetFocusedCarvable());
		if (!Made.IsEmpty())
		{
			float MadeW, MadeH;
			GetTextSize(Made, MadeW, MadeH, Font);
			DrawText(Made, FLinearColor(1.f, 0.85f, 0.4f), CenterX - MadeW * 0.5f, CenterY + 40.f, Font);
		}
		float TextW, TextH;
		GetTextSize(Label, TextW, TextH, Font);
		DrawText(Label, FLinearColor(0.55f, 0.8f, 1.f), CenterX - TextW * 0.5f, CenterY + 18.f, Font);
	}
	else if (const AHarvestableActor* Target = Player->GetFocusedHarvestable())
	{
		const FString Label = Target->IsTrunk()
			? FString::Printf(TEXT("%s   %s  |  Sol t\u0131k: kes"), *Target->GetDisplayName(),
				Target->GetLogItem() == EForestItem::BigLog ? TEXT("[E] Omzuna al")
				: TEXT("[E] \u0130ple \u00E7ek"))
			: Target->GetDisplayName();
		float TextW, TextH;
		GetTextSize(Label, TextW, TextH, Font);
		DrawText(Label, White, CenterX - TextW * 0.5f, CenterY + 18.f, Font);

		const float BarW = 160.f;
		const float BarY = CenterY + 22.f + TextH;
		DrawRect(FLinearColor(0.f, 0.f, 0.f, 0.6f), CenterX - BarW * 0.5f, BarY, BarW, 8.f);
		DrawRect(FLinearColor(0.85f, 0.3f, 0.15f, 0.9f), CenterX - BarW * 0.5f, BarY, BarW * Target->GetHealthPercent(), 8.f);
	}
	else if (const ALogWallBlueprint* Wall = Player->GetFocusedBlueprint())
	{
		const float FilledRatio = (float)Wall->GetFilled() / FMath::Max(1, Wall->GetTotal());
		const FLinearColor DynamicColor = GetValueGradientColor(FilledRatio);

		if (!Wall->IsComplete())
		{
			const FString Label = FString::Printf(TEXT("Duvar planı  %d / %d   [E] Kütük ekle (elinde %d)"),
				Wall->GetFilled(), Wall->GetTotal(), Player->GetItemCount(EForestItem::Log));
			float TextW, TextH;
			GetTextSize(Label, TextW, TextH, Font);
			DrawText(Label, DynamicColor, CenterX - TextW * 0.5f, CenterY + 18.f, Font);

			const float BarW = 200.f;
			const float BarY = CenterY + 22.f + TextH;
			DrawRect(FLinearColor(0.f, 0.f, 0.f, 0.6f), CenterX - BarW * 0.5f, BarY, BarW, 8.f);
			DrawRect(DynamicColor, CenterX - BarW * 0.5f, BarY, BarW * FilledRatio, 8.f);
		}
		else
		{
			// Duvar tamamlandı: R-değeri ve chinking durumu
			// Chinking yoksa R-7.0 (Sarı), Chinking varsa R-12.8 (Parlak Yeşil!)
			const float RRatio = Wall->IsChinked() ? 1.0f : 0.45f;
			const FLinearColor RColor = GetValueGradientColor(RRatio);

			const FString Label = Wall->IsChinked()
				? TEXT("Kütük Duvar  |  Yalıtım: R-12.8 (Mükemmel Sızdırmaz)")
				: TEXT("Kütük Duvar  |  Yalıtım: R-7.0 (Zayıf)  —  [T] Mala ile Chinking çek");

			float TextW, TextH;
			GetTextSize(Label, TextW, TextH, Font);
			DrawText(Label, RColor, CenterX - TextW * 0.5f, CenterY + 18.f, Font);

			const float BarW = 200.f;
			const float BarY = CenterY + 22.f + TextH;
			DrawRect(FLinearColor(0.f, 0.f, 0.f, 0.6f), CenterX - BarW * 0.5f, BarY, BarW, 8.f);
			DrawRect(RColor, CenterX - BarW * 0.5f, BarY, BarW * RRatio, 8.f);
		}
	}
	else if (const ALogConstructionActor* LogPiece = Player->GetFocusedLogActor())
	{
		// Değer arttıkça (Ham R-5 -> Chinking R-11 -> Plaster R-14): Kırmızı -> Sarı -> Yeşil
		const float RVal = LogPiece->GetEffectiveRValue();
		const float RRatio = FMath::Clamp((RVal - 5.0f) / 9.0f, 0.0f, 1.0f);
		const FLinearColor DynamicColor = GetValueGradientColor(RRatio);

		const FString StateStr = LogPiece->IsPlastered() ? TEXT("Sıvalı (Plastered)") : (LogPiece->IsChinked() ? TEXT("Chinked (Yalıtımlı)") : TEXT("Ham Kütük"));
		const FString Label = FString::Printf(TEXT("Kütük  |  %s  |  Yalıtım: R-%.1f  |  Kesme: %.1f kN"),
			*StateStr, RVal, LogPiece->CalculateShearCapacity());

		float TextW, TextH;
		GetTextSize(Label, TextW, TextH, Font);
		DrawText(Label, DynamicColor, CenterX - TextW * 0.5f, CenterY + 18.f, Font);

		const float BarW = 200.f;
		const float BarY = CenterY + 22.f + TextH;
		DrawRect(FLinearColor(0.f, 0.f, 0.f, 0.6f), CenterX - BarW * 0.5f, BarY, BarW, 8.f);
		DrawRect(DynamicColor, CenterX - BarW * 0.5f, BarY, BarW * RRatio, 8.f);
	}
	else if (const AStoneMasonryActor* StonePiece = Player->GetFocusedStoneActor())
	{
		// Değer arttıkça (Kuru Taş 2500 kN -> Harçlı Taş 3500 kN): Sarı -> Yeşil
		const float CapKN = StonePiece->GetCompressiveCapacity();
		const float CapRatio = FMath::Clamp((CapKN - 1500.0f) / 2000.0f, 0.0f, 1.0f);
		const FLinearColor DynamicColor = GetValueGradientColor(CapRatio);

		const FString TypeStr = (StonePiece->GetMasonryType() == EMasonryType::MortaredStone)
			? TEXT("Harçlı Taş Duvar (Mortared)")
			: TEXT("Kuru Taş Duvar (Dry Stone)");

		const FString Label = FString::Printf(TEXT("Taş Temel  |  %s  |  Taşıma: %.0f kN"),
			*TypeStr, CapKN);

		float TextW, TextH;
		GetTextSize(Label, TextW, TextH, Font);
		DrawText(Label, DynamicColor, CenterX - TextW * 0.5f, CenterY + 18.f, Font);

		const float BarW = 200.f;
		const float BarY = CenterY + 22.f + TextH;
		DrawRect(FLinearColor(0.f, 0.f, 0.f, 0.6f), CenterX - BarW * 0.5f, BarY, BarW, 8.f);
		DrawRect(DynamicColor, CenterX - BarW * 0.5f, BarY, BarW * CapRatio, 8.f);
	}
	else if (const APickupItem* Pickup = Player->GetFocusedPickup())
	{
		FString Label = ForestItems::IsCarriedItem(Pickup->GetItem())
			? FString::Printf(TEXT("[E] Omzuna al: %s"), *ForestItems::GetDisplayName(Pickup->GetItem()))
			: FString::Printf(TEXT("[E] Topla: %s"), *ForestItems::GetDisplayName(Pickup->GetItem()));
		if (Pickup->GetItem() == EForestItem::Log && Player->GetCurrentTool() == EForestTool::Axe)
		{
			Label += TEXT("   |   Sol tık: oymaya hazırla");
		}
		float TextW, TextH;
		GetTextSize(Label, TextW, TextH, Font);
		DrawText(Label, White, CenterX - TextW * 0.5f, CenterY + 18.f, Font);
	}

	// Quick slots and backpack summary, bottom left (GDD 6.3: tool, food, water)
	{
		const FLinearColor Gold(1.f, 0.85f, 0.5f);
		const float SlotW = 92.f, SlotH = 50.f, Gap = 6.f;
		const float X0 = 20.f, Y0 = Canvas->ClipY - SlotH - 20.f;
		const FString Tool = Player->GetCurrentTool() == EForestTool::Pickaxe
			? TEXT("Kazma")
			: (Player->GetCurrentTool() == EForestTool::Trowel ? TEXT("Mala [T]") : TEXT("Balta"));
		const FString Labels[3] = { Tool, TEXT("\u2014"), TEXT("\u2014") };
		const FString Titles[3] = { TEXT("Alet"), TEXT("Yiyecek"), TEXT("Su") };
		for (int32 Index = 0; Index < 3; ++Index)
		{
			const float X = X0 + Index * (SlotW + Gap);
			DrawPanel(X, Y0, SlotW, SlotH);
			DrawText(Titles[Index], FLinearColor(1.f, 1.f, 1.f, 0.55f), X + 8.f, Y0 + 6.f, SmallFont);
			DrawText(Labels[Index], Index == 0 ? White : FLinearColor(1.f, 1.f, 1.f, 0.35f), X + 8.f, Y0 + 24.f, Font);
		}
		const FString Bag = FString::Printf(TEXT("\u00C7anta  %d / %d   [Tab]"), Player->GetUsedSlots(), AForestCharacter::InventorySlots);
		DrawPanel(X0, Y0 - 34.f, 3 * SlotW + 2 * Gap, 28.f);
		DrawText(Bag, Player->GetUsedSlots() >= AForestCharacter::InventorySlots ? FLinearColor(1.f, 0.4f, 0.35f) : Gold, X0 + 8.f, Y0 - 30.f, Font);
	}

	// Backpack grid (Tab): 6 x 4 slots
	if (Player->IsInventoryOpen())
	{
		TArray<TPair<EForestItem, int32>> Slots;
		Player->GetInventorySlots(Slots);
		const int32 Cols = 6, Rows = AForestCharacter::InventorySlots / 6;
		const float CellW = 112.f, CellH = 58.f, Gap = 6.f;
		const float GridW = Cols * CellW + (Cols - 1) * Gap, GridH = Rows * CellH + (Rows - 1) * Gap;
		const float GX = CenterX - GridW * 0.5f, GY = CenterY - GridH * 0.5f;
		DrawPanel(GX - 16.f, GY - 44.f, GridW + 32.f, GridH + 60.f);
		DrawText(FString::Printf(TEXT("\u00C7anta  %d / %d"), Slots.Num(), AForestCharacter::InventorySlots), FLinearColor(1.f, 0.85f, 0.5f), GX, GY - 36.f, Font);
		for (int32 Index = 0; Index < AForestCharacter::InventorySlots; ++Index)
		{
			const float X = GX + (Index % Cols) * (CellW + Gap);
			const float Y = GY + (Index / Cols) * (CellH + Gap);
			DrawRect(FLinearColor(1.f, 1.f, 1.f, 0.08f), X, Y, CellW, CellH);
			if (Slots.IsValidIndex(Index))
			{
				DrawText(ForestItems::GetDisplayName(Slots[Index].Key), White, X + 6.f, Y + 6.f, SmallFont);
				const FString Count = FString::Printf(TEXT("%d"), Slots[Index].Value);
				float TextW, TextH;
				GetTextSize(Count, TextW, TextH, Font);
				DrawText(Count, FLinearColor(1.f, 0.85f, 0.5f), X + CellW - TextW - 6.f, Y + CellH - TextH - 4.f, Font);
			}
		}
	}

	// What the hands hold
	if (Player->GetCarriedItem() != EForestItem::None)
	{
		FString What;
		if (Player->GetCarriedItem() == EForestItem::TrunkLog)
		{
			What = TEXT("G\u00F6vde (iple)");
		}
		else if (Player->GetCarriedItem() == EForestItem::Log)
		{
			What = FString::Printf(TEXT("%d parça, yük %%%.0f"), Player->GetCarriedCount(), Player->GetCarryLoad() * 100.f);
		}
		else if (Player->GetCarriedCount() > 1)
		{
			What = FString::Printf(TEXT("%s \u00D7%d"), *ForestItems::GetDisplayName(Player->GetCarriedItem()), Player->GetCarriedCount());
		}
		else
		{
			What = ForestItems::GetDisplayName(Player->GetCarriedItem());
		}
		const FString Label = FString::Printf(TEXT("Ta\u015F\u0131nan: %s   [E] b\u0131rak"), *What);
		float TextW, TextH;
		GetTextSize(Label, TextW, TextH, Font);
		DrawPanel(CenterX - TextW * 0.5f - 12.f, Canvas->ClipY - 110.f, TextW + 24.f, TextH + 12.f);
		DrawText(Label, FLinearColor(1.f, 0.85f, 0.5f), CenterX - TextW * 0.5f, Canvas->ClipY - 104.f, Font);
	}

	// Survival values, bottom right (GDD 13.1)
	if (const USurvivalComponent* Survival = Player->GetSurvival())
	{
		struct FBar { const TCHAR* Label; float Value; FLinearColor Color; };
		const FBar Bars[] = {
			{ TEXT("Sa\u011Fl\u0131k"),      Survival->GetHealth(),  FLinearColor(0.85f, 0.2f, 0.2f) },
			{ TEXT("Dayan\u0131kl\u0131l\u0131k"), Survival->GetStamina(), FLinearColor(0.95f, 0.8f, 0.25f) },
			{ TEXT("A\u00E7l\u0131k"),        Survival->GetHunger(),  FLinearColor(0.85f, 0.5f, 0.2f) },
			{ TEXT("Susuzluk"),               Survival->GetThirst(),  FLinearColor(0.3f, 0.6f, 0.95f) },
		};
		const float BarW = 180.f, BarH = 10.f, RowH = 24.f;
		const float X = Canvas->ClipX - BarW - 40.f;
		float Y = Canvas->ClipY - 20.f - RowH * UE_ARRAY_COUNT(Bars) - 12.f;
		DrawPanel(X - 110.f, Y - 8.f, BarW + 130.f, RowH * UE_ARRAY_COUNT(Bars) + 12.f);
		for (const FBar& Bar : Bars)
		{
			const float Fraction = FMath::Clamp(Bar.Value / Survival->GetMax(), 0.f, 1.f);
			const bool bLow = Fraction < 0.2f;
			DrawText(Bar.Label, bLow ? FLinearColor(1.f, 0.4f, 0.35f) : White, X - 100.f, Y - 2.f, SmallFont);
			DrawRect(FLinearColor(0.f, 0.f, 0.f, 0.6f), X, Y + 2.f, BarW, BarH);
			DrawRect(Bar.Color, X, Y + 2.f, BarW * Fraction, BarH);
			Y += RowH;
		}
	}

	// Controls, top left
	static const TArray<FString> Help = {
		TEXT("WASD hareket  |  Shift koş  |  Space zıpla"),
		TEXT("Sol tık: vur / blok koy  |  E: topla  |  [T]: Mala (Chinking/Harç)"),
		TEXT("[3] 3 Taş -> 1 Taş Blok"),
		TEXT("Odun tarifle değil, kesilerek yapılır (kütük -> kiriş, yarım, kısa, kalas...)  |  [Tab] Çanta"),
		TEXT("[P] Admin modu: [G] malzeme, [H] oda, [J] kes, [N] aç, [M] hasar, [K] oyma kütüğü, [L] demo, [Y] hasar"),
		TEXT("[B] İnşa modu  |  Tekerlek / [Q]: seç  |  [R] döndür  |  Sağ tık: iptal"),
		TEXT("Kütük Duvar: köşelere tıkla, plana bakıp [E] ile kütük ekle"),
		TEXT("Parça omuzdayken: [Sol tık] koy (kütüğe: çaprazlama, direğe: kiriş/payanda, ateşe: odun)  |  [R] yatır / dik / eğik  |  [F] çakmak (basılı tut)  |  [C] taş koy (7 taş = ateş yeri)  |  [X] it  |  [O] açıklık aç  |  [V] iskele işareti"),
		TEXT("Oyma: [R] alet (çentik, enine kesim, yarma, oval, düz, sivri uç, pah, uç düzeltme, kırlangıç)  |  [E] parçayı omza al"),
	};
	DrawPanel(20.f, 20.f, 620.f, 16.f + 20.f * Help.Num());
	for (int32 Index = 0; Index < Help.Num(); ++Index)
	{
		DrawText(Help[Index], FLinearColor(1.f, 1.f, 1.f, 0.8f), 30.f, 28.f + Index * 20.f, SmallFont);
	}

	// Fires nearby: how long they burn, over the fire
	for (TActorIterator<ACampfire> It(GetWorld()); It; ++It)
	{
		FVector2D Screen;
		const FVector Above = It->GetActorLocation() + FVector(0.f, 0.f, 110.f);
		if (FVector::DistSquared(Above, Player->GetActorLocation()) > FMath::Square(900.f) || !GetOwningPlayerController()
			|| !GetOwningPlayerController()->ProjectWorldLocationToScreen(Above, Screen))
		{
			continue;
		}
		const int32 Seconds = FMath::CeilToInt(It->GetFuelSeconds());
		const FString Text = It->IsBurning() ? FString::Printf(TEXT("Ateş  %d:%02d"), Seconds / 60, Seconds % 60) : FString(TEXT("Ateş söndü"));
		float TextW, TextH;
		GetTextSize(Text, TextW, TextH, SmallFont);
		DrawText(Text, It->IsBurning() ? FLinearColor(1.f, 0.7f, 0.3f) : FLinearColor(0.6f, 0.6f, 0.6f), Screen.X - TextW * 0.5f, Screen.Y, SmallFont);
	}

	// Carrying pieces: what a left click does where the player is looking
	if (Player->GetCarriedItem() == EForestItem::Log)
	{
		FString Label;
		switch (Player->GetCarryPlace())
		{
		case AForestCharacter::ECarryPlace::Across: Label = TEXT("[Sol tık] Çaprazlama yerleştir  (eyer çentiği kendiliğinden açılır)"); break;
		case AForestCharacter::ECarryPlace::Stand:  Label = TEXT("[Sol tık] Dik koy (direk)   [R] yatır"); break;
		case AForestCharacter::ECarryPlace::Lay:    Label = TEXT("[Sol tık] Yatır   [R] dik koy"); break;
		case AForestCharacter::ECarryPlace::Beam:   Label = TEXT("[Sol tık] Kiriş koy (direğin üstüne)"); break;
		case AForestCharacter::ECarryPlace::Brace:  Label = TEXT("[Sol tık] Payanda koy (45°, kirişin altına)"); break;
		case AForestCharacter::ECarryPlace::Floor:  Label = TEXT("[Sol tık] Döşeme tahtası koy (düz yüz üstte)"); break;
		case AForestCharacter::ECarryPlace::Fuel:   Label = TEXT("[Sol tık] Ateşe odun at"); break;
		case AForestCharacter::ECarryPlace::Repair: Label = TEXT("[Sol tık] Hasarlı parçayı bununla değiştir"); break;
		case AForestCharacter::ECarryPlace::Slope:  Label = FString::Printf(TEXT("[Sol tık] Eğik koy (%.0f°)   [Tekerlek] açı   [R] yatır"), Player->GetSlopeAngle()); break;
		case AForestCharacter::ECarryPlace::Load:   Label = TEXT("[Sol tık] Yükle"); break;
		default:                                    Label = TEXT("Yere ya da yerdeki bir kütüğe bak"); break;
		}
		if (Player->GetCarryPlace() != AForestCharacter::ECarryPlace::None && !Player->IsPlaceSupported())
		{
			Label += TEXT("   —  desteksiz, düşer");
		}
		float TextW, TextH;
		GetTextSize(Label, TextW, TextH, Font);
		DrawText(Label, FLinearColor(0.8f, 0.92f, 1.f), CenterX - TextW * 0.5f, CenterY + 44.f, Font);
	}

	// Guided demo checklist, under the controls
	for (TActorIterator<AMechanicsDemo> It(GetWorld()); It; ++It)
	{
		if (!It->IsGuiding())
		{
			continue;
		}
		TArray<FDemoGoalLine> Lines;
		FString Hint;
		It->GetGuide(Lines, Hint);
		const float X = 20.f;
		float Y = 36.f + 20.f * Help.Num() + 12.f;
		const float RowH = 22.f;
		DrawPanel(X, Y, 620.f, 40.f + RowH * Lines.Num() + 26.f);
		DrawText(TEXT("MEKANİK DEMOSU"), FLinearColor(1.f, 0.85f, 0.4f), X + 10.f, Y + 8.f, Font);
		Y += 38.f;
		for (const FDemoGoalLine& Line : Lines)
		{
			const FLinearColor Color = Line.bDone ? FLinearColor(0.35f, 1.f, 0.45f)
				: Line.bCurrent ? FLinearColor(1.f, 0.9f, 0.35f) : FLinearColor(1.f, 1.f, 1.f, 0.45f);
			const FString Mark = Line.bDone ? TEXT("[x]") : Line.bCurrent ? TEXT("[>]") : TEXT("[ ]");
			DrawText(FString::Printf(TEXT("%s  %s"), *Mark, *Line.Text), Color, X + 12.f, Y, SmallFont);
			Y += RowH;
		}
		DrawText(Hint, FLinearColor(0.75f, 0.9f, 1.f), X + 12.f, Y + 4.f, SmallFont);
		break;
	}

	// Admin mode badge, top right
	if (Player->IsAdminMode())
	{
		const FString Label = TEXT("ADMIN");
		float TextW, TextH;
		GetTextSize(Label, TextW, TextH, Font);
		DrawPanel(Canvas->ClipX - TextW - 44.f, 20.f, TextW + 24.f, TextH + 12.f);
		DrawText(Label, FLinearColor(1.f, 0.4f, 0.35f), Canvas->ClipX - TextW - 32.f, 26.f, Font);
	}

	// Build mode banner
	if (Player->IsInBuildMode())
	{
		const EForestItem Selected = Player->GetSelectedBuildItem();
		const bool bWall = Player->IsWallTool();
		const int32 Count = bWall ? 1 : Player->GetItemCount(Selected);
		FString Label;
		if (bWall)
		{
			Label = Player->IsWallStartSet()
				? FString(TEXT("İNŞA MODU  -  Kütük Duvar: bitiş noktasına tıkla"))
				: FString(TEXT("İNŞA MODU  -  Kütük Duvar: başlangıç noktasına tıkla"));
		}
		else
		{
			Label = Count > 0
				? FString::Printf(TEXT("İNŞA MODU  -  %s: %d"), *ForestItems::GetDisplayName(Selected), Count)
				: FString(TEXT("İNŞA MODU  -  yerleştirecek eşya yok"));
		}
		float TextW, TextH;
		GetTextSize(Label, TextW, TextH, Font);
		DrawPanel(CenterX - TextW * 0.5f - 12.f, Canvas->ClipY - 70.f, TextW + 24.f, TextH + 12.f);
		DrawText(Label, FLinearColor(0.6f, 1.f, 0.6f), CenterX - TextW * 0.5f, Canvas->ClipY - 64.f, Font);

		if (Count > 0 && !Player->IsPlacementValid())
		{
			const FString Blocked = TEXT("Yerleştirilemez");
			GetTextSize(Blocked, TextW, TextH, Font);
			DrawText(Blocked, FLinearColor(1.f, 0.35f, 0.3f), CenterX - TextW * 0.5f, CenterY + 18.f, Font);
		}
	}

	// Last message, fades out
	const float Age = Player->GetMessageAge();
	if (!Player->GetMessage().IsEmpty() && Age < 3.f)
	{
		const float Alpha = FMath::Clamp(3.f - Age, 0.f, 1.f);
		float TextW, TextH;
		GetTextSize(Player->GetMessage(), TextW, TextH, Font);
		DrawText(Player->GetMessage(), FLinearColor(1.f, 1.f, 1.f, Alpha), CenterX - TextW * 0.5f, Canvas->ClipY * 0.22f, Font);
	}
}
