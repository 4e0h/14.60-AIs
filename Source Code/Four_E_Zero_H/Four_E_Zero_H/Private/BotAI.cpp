#include "pch.h"
#include "../Public/BotAI.h"
#include "../Public/Configuration.h"

#include "../Public/NPCs.h"

#include "../Public/PlaceDump.h"
#include "../Public/ServerConsole.h"
#include "../../FortniteGame/Public/FortGameMode.h"
#include "../../FortniteGame/Public/BattleRoyaleGamePhaseLogic.h"

#include "../../FortniteGame/Public/FortWeapon.h"
#include "../../FortniteGame/Public/BuildingSMActor.h"
#include "../../FortniteGame/Public/FortPlayerControllerAthena.h"

#include "../Plugins/CrashReporter/Public/CrashReporter.h"
#include <chrono>
#include <thread>
#include <map>
#include <set>
#include <vector>
#include <string>
#include <cmath>
#include <algorithm>
#include <cctype>
#include <cstdarg>
#include <cstdio>
#include <cstdlib>
#include <cstring>

extern bool GStormMapDataWasRepaired;

uint8 ToDeathCause(AFortPlayerPawnAthena* Pawn, FGameplayTagContainer& DeathTags, bool bDBNO);

static void LogLine(const char* Fmt, ...)
{
    if (!FConfiguration::bBotLog || !Fmt)
        return;

    char Buffer[1024];

    va_list Args;
    va_start(Args, Fmt);
    vsnprintf(Buffer, sizeof(Buffer), Fmt, Args);
    va_end(Args);

    const size_t Length = strlen(Buffer);
    printf("%s%s", Buffer, (Length > 0 && Buffer[Length - 1] == '\n') ? "" : "\n");
    fflush(stdout);
}

namespace BotAI
{
    static std::set<void*> ThankedBots;

    static std::set<void*> MovedToIsland;

    static std::set<void*> BTRetried;

    static int MoveResult_Failed = 0;
    static int MoveResult_AlreadyAtGoal = 0;
    static int MoveResult_Success = 0;
    static int MoveResult_Other = 0;

    static float BusCountdownStart = -1.f;
    static int LastBusSecondAnnounced = -1;

    static std::map<void*, int> BotCombatTick;

    static AFortInventory* EnsureBotInventory(AActor* BotControllerActor);

    static const UFortItemDefinition* FindItemDefByName(const wchar_t* name);

    static std::set<void*> LoadoutGiven;

    static std::map<void*, const UFortItemDefinition*> BotWeapons;

    static std::map<void*, AFortInventory*> BotInventories;

    static std::map<void*, int> BotWeaponClipSize;

    static std::set<void*> BotTeamsAssigned;

    static std::set<void*> PickaxeGiven;

    static std::map<void*, int> BotZoneMoveTick;

    static std::map<void*, float> BotLastZoneDistance;

    static std::map<void*, float> BotFacingYaw;

    struct FBotFireState
    {
        int ReactionTicksLeft = -1;
        int BurstTicksLeft = 0;
        int CooldownTicksLeft = 0;
        bool bFiring = false;

        int ReloadTicksLeft = 0;

        int RoundsSinceReload = 0;

        int LastSeenAmmo = -1;
    };

    static std::map<void*, FBotFireState> BotFire;

    static int GCombatEngagements = 0;
    static int GCombatBurstsFired = 0;
    static int GCombatBotOnBotHits = 0;

    static int GCombatNoWeapon = 0;

    static int GMagazinesRefilled = 0;

    static int GReloadsStarted = 0;

    static int GBotsMadeKillableAgain = 0;

    static int GWeaponsReissued = 0;

    
    static int GLocksMade = 0;
    static int GLocksEnded = 0;
    static int GBurstsCutOutOfSight = 0;
    static int GTicksHeldFireOutOfSight = 0;
    static int GAimDeliberateMisses = 0;
    static int GChaseHops = 0;
    static std::map<void*, std::string> BotLockedOnName;   

    
    static std::map<void*, AFortPlayerPawnAthena*> BotCombatTarget;

    
    
    static std::map<void*, FVector> BotLastSeenSpot;

    
    static std::set<void*> BotStoppedForFight;

    
    static std::set<void*> BotHasFocus;

    
    
    static std::map<void*, std::pair<void*, int>> BotAmmoSeen;   
    static std::map<void*, int> BotTriggerUpTick;                
    static int GShotTicksOurs = 0;
    static int GShotTicksStray = 0;

    
    struct FBotAimRecord
    {
        double Yaw = 0.0;
        double Pitch = 0.0;
    };
    static std::map<void*, FBotAimRecord> BotIntendedAim;
    static int GAimChecks = 0;
    static int GAimHeld = 0;
    static double GAimOffSum = 0.0;
    static double GAimOffMax = 0.0;

    
    static int GHitsOnLockedTarget = 0;
    static int GHitsOnSomebodyElse = 0;

    
    static int GTerrainBlocks = 0;

    static const uint8 GamePhase_Aircraft = 2;
    static const uint8 GamePhase_SafeZones = 3;

    static bool PlayersAreProtected(AFortGameStateAthena* GameState)
    {
        if (!GameState || !GameState->HasGamePhase())
            return false;

        return GameState->GamePhase < GamePhase_Aircraft;
    }

    static std::set<void*> LandedBots;

    static bool AircraftPhaseReached()
    {
        auto World = UWorld::GetWorld();
        if (!World || !World->GameState)
            return false;

        auto GS = (AFortGameStateAthena*)World->GameState;

        if (!GS->HasGamePhase())
            return true;

        return GS->GamePhase >= GamePhase_Aircraft;
    }

    static bool HasLanded(void* ControllerKey, AFortPlayerPawnAthena* Pawn)
    {
        if (!ControllerKey)
            return false;

        if (!AircraftPhaseReached())
            return false;

        if (LandedBots.find(ControllerKey) != LandedBots.end())
            return true;

        if (!Pawn)
            return false;

        bool bDropped = false;

        if (MovedToIsland.find((void*)Pawn) != MovedToIsland.end())
            bDropped = true;

        if (!bDropped && Pawn->HasbIsSkydiving() && Pawn->bIsSkydiving)
            bDropped = true;

        if (!bDropped && Pawn->HasbIsSkydivingFromBus() && Pawn->bIsSkydivingFromBus)
            bDropped = true;

        if (!bDropped && Pawn->HasbIsParachuteOpen() && Pawn->bIsParachuteOpen)
            bDropped = true;

        if (!bDropped)
        {
            FVector Loc = Pawn->K2_GetActorLocation();
            if (Loc.Z > 8000.0)
                bDropped = true;
        }

        if (bDropped)
        {
            LandedBots.insert(ControllerKey);
            return true;
        }

        return false;
    }

    static void KeepTheClockHonest()
    {
        if (!FConfiguration::bLockServerSpeed)
            return;

        auto World = UWorld::GetWorld();
        if (!World || !World->HasPersistentLevel() || !World->PersistentLevel)
            return;

        auto Level = (UObject*)World->PersistentLevel;

        static int32 SettingsOffset = -2;
        if (SettingsOffset == -2)
            SettingsOffset = (int32)Level->GetOffset("WorldSettings");

        if (SettingsOffset < 0)
            return;

        auto Settings = GetFromOffset<UObject*>(Level, (uint32)SettingsOffset);
        if (!Settings)
            return;

        static const char* Fields[] = { "TimeDilation", "MatineeTimeDilation", "DemoPlayTimeDilation" };
        static int32 Offsets[3] = { -2, -2, -2 };

        for (int i = 0; i < 3; i++)
        {
            if (Offsets[i] == -2)
                Offsets[i] = (int32)Settings->GetOffset(Fields[i]);

            if (Offsets[i] < 0)
                continue;

            float& Value = GetFromOffset<float>(Settings, (uint32)Offsets[i]);

            if (Value != 1.f)
            {
                Value = 1.f;

                static int Reported = 0;
                if (++Reported <= 3)
                    LogLine("[SPEED] Something changed the server's clock (%s). Put back to normal speed.", Fields[i]);
            }
        }
    }

    
    
    static bool LooksLikeAName(const std::string& Name)
    {
        if (Name.size() < 3 || Name.size() > 32)
            return false;

        int Letters = 0;
        int HexDigits = 0;

        for (size_t i = 0; i < Name.size(); i++)
        {
            const unsigned char Ch = (unsigned char)Name[i];

            if (Ch < 0x20 || Ch > 0x7E)
                return false;

            if ((Ch >= 'a' && Ch <= 'z') || (Ch >= 'A' && Ch <= 'Z'))
                Letters++;

            if ((Ch >= '0' && Ch <= '9') || (Ch >= 'a' && Ch <= 'f') || (Ch >= 'A' && Ch <= 'F'))
                HexDigits++;
        }

        if (Letters == 0)
            return false;

        
        if (Name.size() >= 16 && HexDigits == (int)Name.size())
            return false;

        return true;
    }

    
    static std::string LongestAnsiRun(const uint8* Bytes, int Count)
    {
        std::string Best;
        std::string Current;

        for (int i = 0; i < Count; i++)
        {
            const unsigned char Ch = (unsigned char)Bytes[i];

            if (Ch >= 0x20 && Ch <= 0x7E)
            {
                Current += (char)Ch;
                continue;
            }

            if (Current.size() > Best.size())
                Best = Current;

            Current.clear();
        }

        if (Current.size() > Best.size())
            Best = Current;

        return Best;
    }

    
    
    
    static std::string LongestWideRun(const uint8* Bytes, int Count)
    {
        std::string Best;

        for (int Start = 0; Start < 2; Start++)
        {
            std::string Current;

            for (int i = Start; i + 1 < Count; i += 2)
            {
                const unsigned char Low = (unsigned char)Bytes[i];
                const unsigned char High = (unsigned char)Bytes[i + 1];

                if (High == 0x00 && Low >= 0x20 && Low <= 0x7E)
                {
                    Current += (char)Low;
                    continue;
                }

                if (Current.size() > Best.size())
                    Best = Current;

                Current.clear();
            }

            if (Current.size() > Best.size())
                Best = Current;
        }

        return Best;
    }

    
    
    static std::string NameFromUniqueId(AFortPlayerStateAthena* PlayerState)
    {
        if (!PlayerState)
            return std::string();

        const uint8* Bytes = nullptr;
        int Count = 0;

        if (!FUniqueNetIdRepl::HasReplicationBytes())
            return std::string();

        if (PlayerState->HasUniqueId())
        {
            auto& Replicated = PlayerState->UniqueId.ReplicationBytes;

            if (Replicated.IsValid() && Replicated.Num() > 0)
            {
                Bytes = (const uint8*)Replicated.GetData();
                Count = Replicated.Num();
            }
        }

        if ((!Bytes || Count <= 0) && PlayerState->HasUniqueID())
        {
            auto& Replicated = PlayerState->UniqueID.ReplicationBytes;

            if (Replicated.IsValid() && Replicated.Num() > 0)
            {
                Bytes = (const uint8*)Replicated.GetData();
                Count = Replicated.Num();
            }
        }

        if (!Bytes || Count <= 0 || Count > 4096)
            return std::string();

        std::string Narrow = LongestAnsiRun(Bytes, Count);
        std::string Wide = LongestWideRun(Bytes, Count);

        
        std::string Picked = Wide.size() > Narrow.size() ? Wide : Narrow;

        
        const size_t Colon = Picked.find_last_of(':');

        if (Colon != std::string::npos && (Picked.size() - Colon - 1) >= 3)
            Picked = Picked.substr(Colon + 1);

        while (!Picked.empty() && Picked.front() == ' ')
            Picked.erase(Picked.begin());

        while (!Picked.empty() && Picked.back() == ' ')
            Picked.pop_back();

        return LooksLikeAName(Picked) ? Picked : std::string();
    }

    static std::string NameAsStored(AFortPlayerStateAthena* PlayerState);

    static std::string SafeNameOf(AFortPlayerStateAthena* PlayerState)
    {
        if (!PlayerState)
            return std::string();

        
        
        const bool bIsBot = PlayerState->HasbIsABot() && PlayerState->bIsABot;

        if (!bIsBot)
        {
            const std::string FromAccount = NameFromUniqueId(PlayerState);

            if (!FromAccount.empty())
                return FromAccount;
        }

        return NameAsStored(PlayerState);
    }

    static std::string NameAsStored(AFortPlayerStateAthena* PlayerState)
    {
        if (!PlayerState || !PlayerState->HasPlayerNamePrivate())
            return std::string();

        const auto& Stored = PlayerState->PlayerNamePrivate;

        const wchar_t* Characters = Stored.CStr();
        int32 Length = Stored.Num();

        if (!Characters || Length <= 0 || Length > 512)
            return std::string();

        std::wstring Wide;
        Wide.reserve((size_t)Length);

        for (int32 i = 0; i < Length; i++)
        {
            const wchar_t Ch = Characters[i];

            if (Ch == L'\0')
                break;

            Wide += Ch;
        }

        if (Wide.empty())
            return std::string();

        std::string Narrow;

        const int Needed = WideCharToMultiByte(CP_UTF8, 0, Wide.c_str(), (int)Wide.size(), nullptr, 0, nullptr, nullptr);

        if (Needed > 0)
        {
            Narrow.resize((size_t)Needed);
            WideCharToMultiByte(CP_UTF8, 0, Wide.c_str(), (int)Wide.size(), &Narrow[0], Needed, nullptr, nullptr);
        }
        else
        {
            for (size_t i = 0; i < Wide.size(); i++)
            {
                const wchar_t Ch = Wide[i];

                if (Ch > 0 && Ch < 128)
                    Narrow += (char)Ch;
            }
        }

        std::string Clean;
        Clean.reserve(Narrow.size());

        for (size_t i = 0; i < Narrow.size(); i++)
        {
            const unsigned char Ch = (unsigned char)Narrow[i];

            if (Ch >= 0x20 && Ch != 0x7F)
                Clean += (char)Ch;
        }

        while (!Clean.empty() && Clean.front() == ' ')
            Clean.erase(Clean.begin());

        while (!Clean.empty() && Clean.back() == ' ')
            Clean.pop_back();

        return Clean;
    }

    static void HoldTheStorm(AFortGameMode* GameMode, float Now)
    {
        if (!UFortGameStateComponent_BattleRoyaleGamePhaseLogic::bPausedZone)
            return;

        if (!GameMode || !GameMode->HasSafeZoneIndicator() || !GameMode->SafeZoneIndicator)
            return;

        auto Indicator = GameMode->SafeZoneIndicator;

        float HeldOpen = Now + 600.f;

        if (Indicator->HasSafeZoneStartShrinkTime() && (float)Indicator->SafeZoneStartShrinkTime < HeldOpen)
            Indicator->SafeZoneStartShrinkTime = HeldOpen;

        if (Indicator->HasSafeZoneFinishShrinkTime() && (float)Indicator->SafeZoneFinishShrinkTime < HeldOpen)
            Indicator->SafeZoneFinishShrinkTime = HeldOpen;

        auto World = UWorld::GetWorld();

        if (World && World->GameState)
        {
            auto GameState = (AFortGameStateAthena*)World->GameState;

            if (GameState->HasSafeZonesStartTime() && (float)GameState->SafeZonesStartTime < HeldOpen)
                GameState->SafeZonesStartTime = HeldOpen;
        }

        if (GameMode->HasbSafeZonePaused())
            GameMode->bSafeZonePaused = true;

        static bool bSaid = false;
        if (!bSaid)
        {
            bSaid = true;
            LogLine("[ZONE] Storm held where it is. Nothing shrinks and the clock stops until \"cheat resume zone\".");
        }
    }

    static void ReleaseTheStorm(AFortGameMode* GameMode, float Now)
    {
        static bool bWasPaused = false;

        if (UFortGameStateComponent_BattleRoyaleGamePhaseLogic::bPausedZone)
        {
            bWasPaused = true;
            return;
        }

        if (!bWasPaused)
            return;

        bWasPaused = false;

        if (!GameMode || !GameMode->HasSafeZoneIndicator() || !GameMode->SafeZoneIndicator)
            return;

        auto Indicator = GameMode->SafeZoneIndicator;

        float StartsIn = Now + 30.f;
        float FinishesIn = Now + 60.f;

        if (Indicator->HasSafeZoneStartShrinkTime())
            Indicator->SafeZoneStartShrinkTime = StartsIn;

        if (Indicator->HasSafeZoneFinishShrinkTime())
            Indicator->SafeZoneFinishShrinkTime = FinishesIn;

        auto World = UWorld::GetWorld();

        if (World && World->GameState)
        {
            auto GameState = (AFortGameStateAthena*)World->GameState;

            if (GameState->HasSafeZonesStartTime() && (float)GameState->SafeZonesStartTime > Now)
                GameState->SafeZonesStartTime = Now;
        }

        if (GameMode->HasbSafeZonePaused())
            GameMode->bSafeZonePaused = false;

        LogLine("[ZONE] Storm running again -- it starts closing in 30 seconds.");
    }

    
    
    static std::set<const UFortItemDefinition*> GrapplerDefinitions;

    static void ApplyWeaponHotfixes()
    {
        static bool bDone = false;

        if (bDone)
            return;

        static const wchar_t* Grapplers[] = {
            L"WID_Hook_Gun_VR_Ore_T03",
            L"WID_Hook_Gun_SR_Ore_T03",
            L"WID_Hook_Gun_Athena_VR_Ore_T03",
            L"WID_Hook_Gun_Athena_SR_Ore_T03",
            L"WID_Boss_Adventure_GH",
        };

        const int Wanted = FConfiguration::GrapplerClipSize;

        if (Wanted <= 0)
        {
            bDone = true;
            return;
        }

        int Patched = 0;
        int Found = 0;

        for (auto Name : Grapplers)
        {
            auto Definition = FindObject<UFortWeaponItemDefinition>(Name);

            if (!Definition)
            {
                std::wstring Wide = Name;
                std::string Narrow(Wide.begin(), Wide.end());
                Definition = TUObjectArray::FindObject<UFortWeaponItemDefinition>(Narrow.c_str());
            }

            if (!Definition)
                continue;

            Found++;
            GrapplerDefinitions.insert((const UFortItemDefinition*)Definition);

            auto Stats = AFortInventory::GetStats(Definition);

            if (!Stats || !FFortRangedWeaponStats::HasClipSize())
                continue;

            if (Stats->ClipSize == Wanted)
            {
                Patched++;
                continue;
            }

            int32 Clip = (int32)Wanted;
            Stats->ClipSize = Clip;

            if (FFortRangedWeaponStats::HasInitialClips())
            {
                int32 Clips = 1;
                Stats->InitialClips = Clips;
            }

            Patched++;
        }

        if (Found == 0)
            return;

        bDone = true;

        LogLine("[HOTFIX] Grappler clip size set to %d on %d of %d grappler definition(s) found -- the same thing the "
                "backend RowUpdate does, applied here instead.", Wanted, Patched, Found);
    }

    
    
    
    
    
    
    static int32* WeaponAmmoCount(AActor* Weapon);

    static void TopUpGrapplers(AFortGameStateAthena* GameState, int TickNow)
    {
        const int Wanted = FConfiguration::GrapplerClipSize;

        if (Wanted <= 0 || GrapplerDefinitions.empty() || !GameState || !GameState->HasPlayerArray())
            return;

        
        static int NextCheck = 0;

        if (TickNow < NextCheck)
            return;

        NextCheck = TickNow + 15;

        if (!FFortItemEntry::HasItemDefinition() || !FFortItemEntry::HasLoadedAmmo())
            return;

        int ToppedUp = 0;

        for (int i = 0; i < GameState->PlayerArray.Num(); i++)
        {
            auto PS = (AFortPlayerStateAthena*)GameState->PlayerArray[i];
            if (!PS)
                continue;

            
            
            
            if (PS->HasbIsABot() && PS->bIsABot)
                continue;

            auto Controller = (AFortPlayerControllerAthena*)PS->Owner;
            if (!Controller || !Controller->IsA<AFortPlayerControllerAthena>())
                continue;

            auto Inventory = Controller->WorldInventory;
            if (!Inventory || !Inventory->HasInventory())
                continue;

            for (int e = 0; e < Inventory->Inventory.ItemInstances.Num(); e++)
            {
                auto Item = Inventory->Inventory.ItemInstances[e];
                if (!Item || !Item->HasItemEntry())
                    continue;

                auto& Entry = Item->ItemEntry;

                if (!Entry.ItemDefinition)
                    continue;

                if (GrapplerDefinitions.find(Entry.ItemDefinition) == GrapplerDefinitions.end())
                    continue;

                
                
                if (Entry.LoadedAmmo >= Wanted)
                    continue;

                int32 Full = (int32)Wanted;
                Entry.LoadedAmmo = Full;
                Inventory->Update(&Entry);
                ToppedUp++;
            }

            
            
            auto Pawn = (AFortPlayerPawnAthena*)Controller->Pawn;

            if (Pawn && Pawn->HasCurrentWeapon() && Pawn->CurrentWeapon)
            {
                auto Held = (AFortWeapon*)Pawn->CurrentWeapon;

                if (Held->HasWeaponData() && Held->WeaponData &&
                    GrapplerDefinitions.find((const UFortItemDefinition*)Held->WeaponData) != GrapplerDefinitions.end())
                {
                    if (auto Ammo = WeaponAmmoCount((AActor*)Held))
                        if (*Ammo < Wanted)
                            *Ammo = Wanted;
                }
            }
        }

        if (ToppedUp > 0)
        {
            static int Reported = 0;

            if (++Reported <= 5)
                LogLine("[HOTFIX] Grappler put back to %d round(s) on %d item(s).", Wanted, ToppedUp);
        }
    }

    
    
    
    static std::map<void*, void*> HumanPawnToppedUp;
    static std::map<void*, int> HumanTopUpUntilTick;

    static void* GOwnerController = nullptr;

    bool IsServerOwner(AFortPlayerControllerAthena* Controller)
    {
        if (!Controller)
            return false;

        if (!GOwnerController)
            return true;

        return GOwnerController == (void*)Controller;
    }

    static void ClaimServerOwner(AFortGameStateAthena* GameState)
    {
        if (!GameState || !GameState->HasPlayerArray())
            return;

        void* First = nullptr;
        bool bOwnerStillHere = false;

        for (int i = 0; i < GameState->PlayerArray.Num(); i++)
        {
            auto PS = (AFortPlayerStateAthena*)GameState->PlayerArray[i];
            if (!PS)
                continue;

            if ((PS->HasbIsABot() && PS->bIsABot) || NPCs::IsNPC(PS))
                continue;

            auto Owner = (AActor*)PS->Owner;
            if (!Owner)
                continue;

            if (!First)
                First = (void*)Owner;

            if (GOwnerController == (void*)Owner)
                bOwnerStillHere = true;
        }

        if (GOwnerController && bOwnerStillHere)
            return;

        if (GOwnerController != First)
        {
            GOwnerController = First;

            if (GOwnerController)
                LogLine("[OWNER] The server owner is the first player who joined.");
        }
    }

    static bool bBotSpawningPaused = false;

    
    
    
    
    
    static bool bBusClearedForLaunch = false;

    void ClearBusForLaunch(const char* Why)
    {
        if (bBusClearedForLaunch)
            return;

        bBusClearedForLaunch = true;

        LogLine("[BUS] Cleared for launch (%s).", Why ? Why : "no reason given");
    }

    bool BusIsClearedForLaunch()
    {
        return bBusClearedForLaunch;
    }

    void EnsureWeaponHotfixes()
    {
        ApplyWeaponHotfixes();
    }

    void SetBotSpawningPaused(bool bPaused)
    {
        if (bBotSpawningPaused == bPaused)
            return;

        bBotSpawningPaused = bPaused;

        if (bPaused)
        {
            
            
            bBusClearedForLaunch = false;
            BusCountdownStart = -1.f;
            LastBusSecondAnnounced = -1;
        }

        LogLine("[BotAI] Bot spawning %s.", bPaused ? "paused -- the bus is held on the ground too" : "resumed");
    }

    bool AreBotSpawnsPaused()
    {
        return bBotSpawningPaused;
    }

    void HoldTheWarmupClock(AFortGameStateAthena* GameState, AFortGameMode* GameMode, float Now)
    {
        float HeldOpen = Now + 600.f;

        if (GameState)
        {
            if (GameState->HasWarmupCountdownStartTime())
                GameState->WarmupCountdownStartTime = Now;
            if (GameState->HasWarmupCountdownEndTime())
                GameState->WarmupCountdownEndTime = HeldOpen;
        }

        if (GameMode && GameMode->HasWarmupEarlyCountdownDuration())
            GameMode->WarmupEarlyCountdownDuration = HeldOpen;

        static int Reported = 0;
        if (++Reported <= 3)
            LogLine("[BUS] Warmup clock held open -- the bus waits for %d players.",
                    FConfiguration::AutoFillTargetPlayerCount);
    }

    bool BusMayLaunch()
    {
        if (bBotSpawningPaused)
            return false;

        const int Target = FConfiguration::AutoFillTargetPlayerCount;

        if (Target <= 0)
            return true;

        auto World = UWorld::GetWorld();

        if (!World || !World->AuthorityGameMode)
            return false;

        auto GameMode = (AFortGameMode*)World->AuthorityGameMode;

        if (!GameMode->HasAlivePlayers())
            return true;

        return GameMode->AlivePlayers.Num() >= Target;
    }

    bool MatchHasStarted()
    {
        auto World = UWorld::GetWorld();
        if (!World || !World->GameState)
            return true;

        return !PlayersAreProtected((AFortGameStateAthena*)World->GameState);
    }

    static void WipeTheSlateAtBusTime(AFortGameStateAthena* GameState);

    static std::map<void*, std::pair<void*, int>> ShotAtBy;

    static const int ShotAtByValidTicks = 150;

    
    
    static std::map<void*, std::pair<void*, int>> AggroedByHuman;

    
    static std::map<void*, int> BotUnderFireSince;
    static std::set<void*> BotAlreadyBuiltWall;

    static int GElimsFromLedger = 0;
    static int GElimsFromShotAt = 0;
    static int GElimsFromFallback = 0;

    static std::map<void*, FVector> MovementProbe;

    static int LandingLogCount = 0;

    static std::map<void*, std::string> AssignedBotNames;

    static std::set<void*> EliminatedBots;

    static std::set<void*> SeenAliveBots;

    static void HandleBotDeath(AFortPlayerControllerAthena* DeadBotController);

    static bool bVictoryDeclared = false;

    static int PeakAlivePlayers = 0;
    static int PeakAliveBots = 0;

    static int VictoryConfirmTicks = 0;
    static constexpr int VictoryConfirmTicksRequired = 3;

    static int VictoryResendTicksLeft = 0;
    static uint8 VictoryDeathCause = 0;
    static constexpr int VictoryResendDurationTicks = 450;
    static constexpr int VictoryResendEveryTicks = 30;

    struct FAliveSnapshot
    {
        int Total = 0;
        int Bots = 0;
        int Humans = 0;
    };

    static FAliveSnapshot SnapshotAlive(AFortGameStateAthena* GameState,
                                        std::vector<AFortPlayerStateAthena*>* OutAlive,
                                        AFortPlayerStateAthena* Ignore)
    {
        FAliveSnapshot Snap{};

        if (OutAlive)
            OutAlive->clear();

        if (!GameState || !GameState->HasPlayerArray())
            return Snap;

        for (int i = 0; i < GameState->PlayerArray.Num(); i++)
        {
            auto PS = (AFortPlayerStateAthena*)GameState->PlayerArray[i];
            if (!PS || PS == Ignore || NPCs::IsNPC(PS))
                continue;

            auto OwnerActor = (AActor*)PS->Owner;
            if (!OwnerActor)
                continue;

            auto Pw = (AFortPlayerPawnAthena*)((AFortPlayerControllerAthena*)OwnerActor)->Pawn;
            if (!Pw || Pw->GetHealth() <= 0.f)
                continue;

            Snap.Total++;
            if (PS->HasbIsABot() && PS->bIsABot)
                Snap.Bots++;
            else
                Snap.Humans++;

            if (OutAlive)
                OutAlive->push_back(PS);
        }

        return Snap;
    }

    static void InvokeNoArgs(const UObject* Object, const char* FunctionName)
    {
        if (!Object || !FunctionName)
            return;

        auto Fn = Object->GetFunction(FunctionName);
        if (!Fn)
            return;

        const int32 Size = Fn->GetPropertiesSize();

        if (Size <= 0 || Size > 0x1000)
        {
            Object->ProcessEvent(Fn, nullptr);
            return;
        }

        std::vector<uint8> Buffer((size_t)Size, 0);
        Object->ProcessEvent(Fn, Buffer.data());
    }

    static std::set<void*> WinAnnouncedTo;

    static void SendWinTo(AFortPlayerStateAthena* WinnerState, uint8 DeathCause, bool bAnnounceToClient)
    {
        if (!WinnerState)
            return;

        if (WinnerState->HasPlace())
        {
            WinnerState->Place = 1;
            InvokeNoArgs(WinnerState, "OnRep_Place");
        }

        auto OwnerActor = (AActor*)WinnerState->Owner;
        auto Controller = OwnerActor ? OwnerActor->Cast<AFortPlayerControllerAthena>() : nullptr;
        if (!Controller)
            return;

        if (bAnnounceToClient && WinAnnouncedTo.find((void*)WinnerState) == WinAnnouncedTo.end())
        {
            WinAnnouncedTo.insert((void*)WinnerState);

            auto WinnerPawn = Controller->Pawn;
            UObject* NoWeapon = nullptr;

            if (Controller->GetFunction("PlayWinEffects"))
                Controller->PlayWinEffects(WinnerPawn, NoWeapon, DeathCause, false);

            if (Controller->GetFunction("ClientNotifyWon"))
                Controller->ClientNotifyWon(WinnerPawn, NoWeapon, DeathCause);

            if (Controller->GetFunction("ClientNotifyTeamWon"))
                Controller->ClientNotifyTeamWon(WinnerPawn, NoWeapon, DeathCause);
        }

        InvokeNoArgs(Controller, "ForceNetUpdate");
    }

    static void DeclareVictory(const std::vector<AFortPlayerStateAthena*>& Winners, uint8 DeathCause,
                               const char* Reason)
    {
        auto World = UWorld::GetWorld();
        if (!World || !World->GameState || Winners.empty())
            return;

        auto GameState = (AFortGameStateAthena*)World->GameState;

        bVictoryDeclared = true;
        VictoryDeathCause = DeathCause;

        AFortPlayerStateAthena* Headline = Winners[0];
        for (auto PS : Winners)
        {
            if (PS && !(PS->HasbIsABot() && PS->bIsABot))
            {
                Headline = PS;
                break;
            }
        }

        if (GameState->HasWinningTeam() && Headline->HasTeamIndex())
        {
            GameState->WinningTeam = (int32)Headline->TeamIndex;
            InvokeNoArgs(GameState, "OnRep_WinningTeam");
        }

        if (GameState->HasWinningPlayerState())
        {
            GameState->WinningPlayerState = Headline;
            InvokeNoArgs(GameState, "OnRep_WinningPlayerState");
        }

        for (auto PS : Winners)
            SendWinTo(PS, DeathCause,  true);

        if (GameState->HasPlayersLeft())
        {
            GameState->PlayersLeft = (int32)Winners.size();
            InvokeNoArgs(GameState, "OnRep_PlayersLeft");
        }
        if (GameState->HasTeamsLeft())
            GameState->TeamsLeft = 1;

        if (GameState->HasGamePhase() && (uint8)GameState->GamePhase < 5)
        {
            GameState->GamePhase = (uint8)5;
            InvokeNoArgs(GameState, "OnRep_GamePhase");
        }

        VictoryResendTicksLeft = VictoryResendDurationTicks;

        LogLine("[WIN] ================ VICTORY ROYALE ================");
        LogLine("[WIN] %d winner(s) -- reason: %s", (int)Winners.size(), Reason);
        for (auto PS : Winners)
        {
            if (!PS || !PS->HasPlayerNamePrivate())
                continue;
            auto w = PS->PlayerNamePrivate.ToString();
            std::string n(w.begin(), w.end());
            LogLine("[WIN]   '%s' (team %d)%s", n.c_str(),
                    PS->HasTeamIndex() ? (int)PS->TeamIndex : -1,
                    (PS->HasbIsABot() && PS->bIsABot) ? "  [bot -- no client to notify]" : "");
        }
        LogLine("[WIN] Notification sent once. The won state is re-asserted for the next 15s in case a client is still catching up.");
    }

    static void TickVictoryResend()
    {
        if (VictoryResendTicksLeft <= 0)
            return;

        VictoryResendTicksLeft--;
        if ((VictoryResendTicksLeft % VictoryResendEveryTicks) != 0)
            return;

        auto World = UWorld::GetWorld();
        auto GameState = World ? (AFortGameStateAthena*)World->GameState : nullptr;
        if (!GameState || !GameState->HasPlayerArray())
            return;

        for (int i = 0; i < GameState->PlayerArray.Num(); i++)
        {
            auto PS = (AFortPlayerStateAthena*)GameState->PlayerArray[i];
            if (!PS || !PS->HasPlace() || PS->Place != 1)
                continue;

            SendWinTo(PS, VictoryDeathCause,  false);
        }
    }

    bool ShouldGrantVictoryTo(AFortPlayerStateAthena* CandidateState, AFortPlayerStateAthena* DyingVictimState)
    {
        if (bVictoryDeclared || !CandidateState || CandidateState == DyingVictimState)
            return false;

        auto World = UWorld::GetWorld();
        if (!World || !World->GameState)
            return false;

        auto GameState = (AFortGameStateAthena*)World->GameState;

        if (PlayersAreProtected(GameState))
            return false;

        if (PeakAlivePlayers < 2)
            return false;

        std::vector<AFortPlayerStateAthena*> Alive;
        auto Snap = SnapshotAlive(GameState, &Alive, DyingVictimState);

        const bool bLastStanding = (Snap.Total <= 1);
        const bool bAllBotsDead = (PeakAliveBots > 0 && Snap.Bots == 0);

        if (!bLastStanding && !bAllBotsDead)
            return false;

        if (std::find(Alive.begin(), Alive.end(), CandidateState) == Alive.end())
            Alive.push_back(CandidateState);

        DeclareVictory(Alive, 0, bLastStanding ? "final elimination -- one player left"
                                               : "final elimination -- all AI players dead");
        return true;
    }

    void CheckForVictory()
    {
        if (bVictoryDeclared)
        {
            TickVictoryResend();
            return;
        }

        auto World = UWorld::GetWorld();
        if (!World || !World->GameState || !World->AuthorityGameMode)
            return;

        auto GameState = (AFortGameStateAthena*)World->GameState;
        auto GameMode = (AFortGameMode*)World->AuthorityGameMode;

        if (PlayersAreProtected(GameState))
        {
            VictoryConfirmTicks = 0;
            return;
        }

        static auto InProgressName = FName(L"InProgress");
        if (GameMode->HasMatchState() && !(GameMode->MatchState == InProgressName))
        {
            VictoryConfirmTicks = 0;
            return;
        }

        std::vector<AFortPlayerStateAthena*> Alive;
        auto Snap = SnapshotAlive(GameState, &Alive, nullptr);

        if (Snap.Total > PeakAlivePlayers)
            PeakAlivePlayers = Snap.Total;
        if (Snap.Bots > PeakAliveBots)
            PeakAliveBots = Snap.Bots;

        if (PeakAlivePlayers < 2 || Snap.Total == 0)
        {
            VictoryConfirmTicks = 0;
            return;
        }

        const char* Reason = nullptr;

        if (Snap.Total == 1)
        {
            Reason = "one player left standing";
        }
        else
        {
            bool bSameTeam = Alive[0] && Alive[0]->HasTeamIndex();
            if (bSameTeam)
            {
                const uint8 Team = (uint8)Alive[0]->TeamIndex;
                for (auto PS : Alive)
                {
                    if (!PS || !PS->HasTeamIndex() || (uint8)PS->TeamIndex != Team)
                    {
                        bSameTeam = false;
                        break;
                    }
                }
            }

            if (bSameTeam)
                Reason = "only one team left alive";
        }

        if (!Reason)
        {
            VictoryConfirmTicks = 0;
            return;
        }

        if (VictoryConfirmTicks == 0)
            LogLine("[WIN] Win condition met (%s) -- confirming over %d ticks before awarding...",
                    Reason, VictoryConfirmTicksRequired);

        if (++VictoryConfirmTicks < VictoryConfirmTicksRequired)
            return;

        DeclareVictory(Alive,  0, Reason);
    }

    static float LastThankTime = -1000.f;

    static std::map<void*, float> PendingParachuteOpen;

    static std::map<void*, AFortPickupAthena*> BotLootTargets;

    
    static std::map<void*, float> GlidingBots;
    static std::map<void*, int> GlidingSettledFor;

    
    
    
    
    
    

    class BFortPlayerPawn : public UObject
    {
    public:
        UCLASS_COMMON_MEMBERS(BFortPlayerPawn);

        DEFINE_FUNC(ForceOpenParachuteAndOverrideGlider, void);
        DEFINE_FUNC(OnRep_IsParachuteForcedOpen, void);
        DEFINE_BITFIELD_PROP(bIsParachuteForcedOpen);
    };

    static const wchar_t* BotGliderIds[] = {
        L"Umbrella_Season_11",
        L"Umbrella_BuildABrella",
        L"Umbrella_Silver",
        L"Glider_ID_015_Brite",
        L"Glider_ID_031_Metal",
        L"Glider_ID_080_PrairiePusher",
    };

    static std::vector<const UObject*> BotGliderChoices;   
    static std::map<void*, const UObject*> BotGliderOf;    

    static const UObject* PickBotGlider()
    {
        static bool bResolved = false;

        if (!bResolved)
        {
            bResolved = true;

            static auto GliderItemClass = FindClass("AthenaGliderItemDefinition");

            const int Listed = (int)(sizeof(BotGliderIds) / sizeof(BotGliderIds[0]));
            std::string Missing;

            for (int i = 0; i < Listed; i++)
            {
                std::wstring Wide(BotGliderIds[i]);
                std::wstring Path = L"/Game/Athena/Items/Cosmetics/Gliders/" + Wide + L"." + Wide;
                std::string Narrow(Wide.begin(), Wide.end());

                const UObject* Glider = FindObject(Path.c_str(), UObject::StaticClass());

                if (!Glider)
                    Glider = TUObjectArray::FindObject<UObject>(Narrow.c_str());

                if (Glider && GliderItemClass && !Glider->IsA(GliderItemClass))
                    Glider = nullptr;

                if (Glider)
                {
                    BotGliderChoices.push_back(Glider);
                }
                else
                {
                    if (!Missing.empty())
                        Missing += ", ";

                    Missing += Narrow;
                }
            }

            LogLine("[GLIDE] %d of the %d bot gliders are on this build%s%s.", (int)BotGliderChoices.size(), Listed,
                    Missing.empty() ? "" : " -- not found: ", Missing.c_str());
        }

        if (BotGliderChoices.empty())
            return nullptr;

        return BotGliderChoices[(size_t)rand() % BotGliderChoices.size()];
    }

    
    struct FBotGlide
    {
        float JumpTime = 0.f;
        float JumpZ = 0.f;
        float OpenTime = -1.f;
        float OpenZ = 0.f;
        float LastDescentZ = 0.f;
        float LastDescentTime = 0.f;
        float NextReopenTime = 0.f;
        int Reopens = 0;
        bool bOpenLastTick = false;
    };

    static std::map<void*, FBotGlide> BotGlides;   

    static int GGlideOpened = 0;
    static int GGlideOpenedByGame = 0;
    static int GGlideOpenedByFlag = 0;
    static int GGlideOpenFailed = 0;
    static int GGlideReopened = 0;
    static int GGlideLanded = 0;
    static int GGlideLandedGliding = 0;
    static bool bGliderForceOpenInFlight = false;
    static bool bGliderForceOpenBroken = false;

    static std::string BotPawnName(AFortPlayerPawnAthena* Pawn)
    {
        if (!Pawn || !Pawn->HasPlayerState() || !Pawn->PlayerState)
            return "a bot";

        const std::string Name = SafeNameOf((AFortPlayerStateAthena*)Pawn->PlayerState);
        return Name.empty() ? std::string("a bot") : Name;
    }

    
    static const char* OpenBotGlider(AFortPlayerPawnAthena* Pawn)
    {
        if (!Pawn || !Pawn->HasbIsParachuteOpen())
            return "";

        
        if (bGliderForceOpenInFlight)
        {
            bGliderForceOpenInFlight = false;
            bGliderForceOpenBroken = true;
            LogLine("[GLIDE] The game's force-open crashed on a bot -- gliders are opened with bIsParachuteOpen from now on.");
        }

        auto GliderIt = BotGliderOf.find((void*)Pawn);
        const UObject* Glider = (GliderIt != BotGliderOf.end()) ? GliderIt->second : nullptr;

        if (!bGliderForceOpenBroken && Glider && ((UObject*)Pawn)->GetFunction("ForceOpenParachuteAndOverrideGlider"))
        {
            const UObject* GliderArgument = Glider;
            bool bResetOverrideOnLanding = true;
            bool bIntoGliderRedeploy = false;

            bGliderForceOpenInFlight = true;
            ((BFortPlayerPawn*)Pawn)->ForceOpenParachuteAndOverrideGlider(GliderArgument, bResetOverrideOnLanding, bIntoGliderRedeploy);
            bGliderForceOpenInFlight = false;

            if (Pawn->bIsParachuteOpen)
                return "the game's force-open";
        }

        
        Pawn->bIsParachuteOpen = true;
        Pawn->OnRep_IsParachuteOpen(false);

        return Pawn->bIsParachuteOpen ? "bIsParachuteOpen" : "";
    }

    
    
    
    static void ForceClearSkydive(AFortPlayerPawnAthena* Pawn)
    {
        if (!Pawn)
            return;

        if (Pawn->HasbIsParachuteOpen() && Pawn->bIsParachuteOpen)
        {
            Pawn->bIsParachuteOpen = false;
            Pawn->OnRep_IsParachuteOpen(true);
        }

        
        auto Chute = (BFortPlayerPawn*)Pawn;
        if (Chute->HasbIsParachuteForcedOpen() && Chute->bIsParachuteForcedOpen)
        {
            Chute->bIsParachuteForcedOpen = false;
            Chute->OnRep_IsParachuteForcedOpen();
        }

        if (Pawn->HasbIsSkydiving() && Pawn->bIsSkydiving)
        {
            Pawn->bIsSkydiving = false;

            
            
            bool bWasSkydiving = true;
            Pawn->OnRep_IsSkydiving(bWasSkydiving);
        }
        if (Pawn->HasbIsSkydivingFromBus() && Pawn->bIsSkydivingFromBus)
            Pawn->bIsSkydivingFromBus = false;

        if (AFortPlayerPawnAthena::EndSkydivingOG)
            AFortPlayerPawnAthena::EndSkydiving(Pawn);
    }

    void TickLandings(UWorld* World)
    {
        
        
        
        
        
        if (World && World->GameState)
        {
            auto GS = (AFortGameStateAthena*)World->GameState;
            if (GS->HasPlayerArray())
            {
                for (int i = 0; i < GS->PlayerArray.Num(); i++)
                {
                    auto PS = (AFortPlayerStateAthena*)GS->PlayerArray[i];
                    if (!PS || !PS->HasbIsABot() || !PS->bIsABot || NPCs::IsNPC(PS))
                        continue;
                    auto Ctrl = (AFortPlayerControllerAthena*)PS->Owner;
                    if (!Ctrl)
                        continue;
                    auto Pawn = (AFortPlayerPawnAthena*)Ctrl->Pawn;
                    if (!Pawn || Pawn->GetHealth() <= 0.f)
                        continue;

                    if (GlidingBots.find((void*)Pawn) != GlidingBots.end())
                        continue;

                    bool bWalking = false;
                    if (Pawn->HasCharacterMovement() && Pawn->CharacterMovement &&
                        Pawn->CharacterMovement->HasMovementMode())
                    {
                        const uint8 Mode = Pawn->CharacterMovement->MovementMode;
                        
                        if (Mode == 1 || Mode == 2)
                            bWalking = true;
                    }

                    const bool bSkydiveStuck =
                        (Pawn->HasbIsSkydiving() && Pawn->bIsSkydiving) ||
                        (Pawn->HasbIsSkydivingFromBus() && Pawn->bIsSkydivingFromBus) ||
                        (Pawn->HasbIsParachuteOpen() && Pawn->bIsParachuteOpen);

                    if (bSkydiveStuck && bWalking)
                    {
                        ForceClearSkydive(Pawn);
                        GlidingBots.erase((void*)Pawn);
                        GlidingSettledFor.erase((void*)Pawn);
                        MovedToIsland.insert((void*)Pawn);
                    }
                }
            }
        }

        if (GlidingBots.empty())
            return;

        const float Now = World ? (float)UGameplayStatics::GetTimeSeconds(World) : 0.f;

        
        
        for (auto it = GlidingBots.begin(); it != GlidingBots.end();)
        {
            auto Pawn = (AFortPlayerPawnAthena*)it->first;

            if (!Pawn)
            {
                GlidingSettledFor.erase(it->first);
                BotGlides.erase(it->first);
                it = GlidingBots.erase(it);
                continue;
            }

            const float Height = (float)Pawn->K2_GetActorLocation().Z;
            FBotGlide& Glide = BotGlides[it->first];

            if (Glide.LastDescentTime <= 0.f)
            {
                Glide.LastDescentZ = Height;
                Glide.LastDescentTime = Now;

                if (Glide.JumpTime <= 0.f)
                {
                    Glide.JumpTime = Now;
                    Glide.JumpZ = Height;
                }
            }

            const bool bGliderOpen = Pawn->HasbIsParachuteOpen() && Pawn->bIsParachuteOpen;

            bool bDown = false;
            const char* Why = "";

            
            if (Pawn->HasCharacterMovement() && Pawn->CharacterMovement &&
                Pawn->CharacterMovement->HasMovementMode())
            {
                const uint8 Mode = Pawn->CharacterMovement->MovementMode;

                
                if (Mode == 1 || Mode == 2)
                {
                    bDown = true;
                    Why = "on the ground";
                }
                else if (Mode == 4)
                {
                    bDown = true;
                    Why = "in the water";
                }
            }

            
            
            
            if (Height < Glide.LastDescentZ - 20.f)
            {
                Glide.LastDescentZ = Height;
                Glide.LastDescentTime = Now;
            }
            else if (!bDown && (Now - Glide.LastDescentTime) > 1.5f)
            {
                bDown = true;
                Why = "stopped going down";
            }

            
            if (!bDown && (Now - Glide.JumpTime) > 240.f)
            {
                bDown = true;
                Why = "four minutes without landing";
            }

            it->second = Height;

            if (!bDown)
            {
                
                if (Glide.OpenTime >= 0.f && !bGliderOpen && Now >= Glide.NextReopenTime)
                {
                    Glide.NextReopenTime = Now + 0.5f;

                    Pawn->bIsParachuteOpen = true;
                    Pawn->OnRep_IsParachuteOpen(false);

                    Glide.Reopens++;
                    GGlideReopened++;

                    if (GGlideReopened <= 6)
                        LogLine("[GLIDE] %s's glider was closed in the air at Z=%.0f -- opened it again (now %s).",
                                BotPawnName(Pawn).c_str(), Height, Pawn->bIsParachuteOpen ? "open" : "STILL CLOSED");
                }

                Glide.bOpenLastTick = Pawn->HasbIsParachuteOpen() && Pawn->bIsParachuteOpen;

                ++it;
                continue;
            }

            
            const bool bWasGliding = bGliderOpen || Glide.bOpenLastTick;

            GGlideLanded++;

            if (bWasGliding)
                GGlideLandedGliding++;

            if (GGlideLanded <= 8)
            {
                char Glided[96] = "";

                if (Glide.OpenTime >= 0.f)
                    snprintf(Glided, sizeof(Glided), ", glider opened at Z=%.0f and flown for %.1fs", Glide.OpenZ, Now - Glide.OpenTime);

                LogLine("[GLIDE] '%s' touched down (%s) at Z=%.0f, %.1fs after leaving the bus -- %s%s%s.", BotPawnName(Pawn).c_str(), Why,
                        Height, Now - Glide.JumpTime, bWasGliding ? "still under its glider" : "NOT under a glider", Glided,
                        Glide.Reopens > 0 ? " (it had to be re-opened in the air)" : "");
            }
            else if ((GGlideLanded % 20) == 0)
            {
                LogLine("[GLIDE] %d bot(s) down from the bus: %d still under the glider when they touched down, %d not.", GGlideLanded,
                        GGlideLandedGliding, GGlideLanded - GGlideLandedGliding);
            }

            ForceClearSkydive(Pawn);
            MovedToIsland.insert((void*)Pawn);

            GlidingSettledFor.erase(it->first);
            BotGlides.erase(it->first);
            BotGliderOf.erase(it->first);
            it = GlidingBots.erase(it);
        }

        static bool bSaidEveryoneDown = false;

        if (GlidingBots.empty() && GGlideLanded > 0 && !bSaidEveryoneDown)
        {
            bSaidEveryoneDown = true;
            LogLine("[GLIDE] Every bot is down from the bus: %d of %d were still under the glider when they touched down | gliders opened: "
                    "%d (%d with the game's force-open and the bot's own glider, %d with bIsParachuteOpen), %d could not be opened, "
                    "%d re-opened after closing in the air.",
                    GGlideLandedGliding, GGlideLanded, GGlideOpened, GGlideOpenedByGame, GGlideOpenedByFlag, GGlideOpenFailed, GGlideReopened);
        }
    }

    void TickParachutes(UWorld* World)
    {
        if (PendingParachuteOpen.empty())
            return;

        float Now = (float)UGameplayStatics::GetTimeSeconds(World);
        for (auto it = PendingParachuteOpen.begin(); it != PendingParachuteOpen.end();)
        {
            if (Now < it->second)
            {
                ++it;
                continue;
            }

            auto Pawn = (AFortPlayerPawnAthena*)it->first;
            it = PendingParachuteOpen.erase(it);

            
            if (!Pawn || GlidingBots.find((void*)Pawn) == GlidingBots.end())
                continue;

            const char* How = OpenBotGlider(Pawn);
            const float Z = (float)Pawn->K2_GetActorLocation().Z;

            FBotGlide& Glide = BotGlides[(void*)Pawn];

            if (*How)
            {
                Glide.OpenTime = Now;
                Glide.OpenZ = Z;

                GGlideOpened++;

                if (How[0] == 't')
                    GGlideOpenedByGame++;
                else
                    GGlideOpenedByFlag++;
            }
            else
            {
                GGlideOpenFailed++;
            }

            if ((GGlideOpened + GGlideOpenFailed) <= 10)
            {
                auto GliderIt = BotGliderOf.find((void*)Pawn);
                const UObject* Glider = (GliderIt != BotGliderOf.end()) ? GliderIt->second : nullptr;

                LogLine("[GLIDE] '%s' %s its glider (%s) at Z=%.0f, %.1fs after leaving the bus%s%s.", BotPawnName(Pawn).c_str(),
                        *How ? "opened" : "could NOT open", Glider ? Glider->Name.ToString().c_str() : "the default one", Z,
                        Glide.JumpTime > 0.f ? Now - Glide.JumpTime : 0.f, *How ? " -- " : "", How);
            }
        }
    }

    static bool bBusDeparted = false;

    
    
    
    
    
    
    
    
    
    

    
    static uint32 BotPersonalSeed(void* Pawn)
    {
        uint64 Value = (uint64)Pawn;

        Value ^= Value >> 33;
        Value *= 0xFF51AFD7ED558CCDull;
        Value ^= Value >> 33;

        return (uint32)Value;
    }

    
    
    static bool BotIsLookingAt(AFortPlayerPawnAthena* Me, const FVector& MyLoc, const FVector& ThemLoc)
    {
        const double dx = ThemLoc.X - MyLoc.X;
        const double dy = ThemLoc.Y - MyLoc.Y;
        const double Flat = sqrt(dx * dx + dy * dy);

        if (Flat <= (double)FConfiguration::BotHearingRange)
            return true;

        if (Flat < 1.0)
            return true;

        FVector Facing = Me->GetActorForwardVector();

        const double FacingLength = sqrt(Facing.X * Facing.X + Facing.Y * Facing.Y);

        if (FacingLength < 0.001)
            return true;

        
        const double Dot = ((Facing.X / FacingLength) * (dx / Flat)) + ((Facing.Y / FacingLength) * (dy / Flat));

        double HalfAngle = (double)FConfiguration::BotFieldOfView * 0.5;

        if (!(HalfAngle > 5.0) || HalfAngle > 180.0)
            HalfAngle = 60.0;

        return Dot >= cos(HalfAngle * 3.14159265358979323846 / 180.0);
    }

    
    
    
    static bool BotHasLineOfSight(AAIController* AIController, AActor* Them)
    {
        static int Available = -1;

        if (Available == -1)
        {
            Available = (AIController && AIController->GetFunction("LineOfSightTo")) ? 1 : 0;

            LogLine("[EYES] The game's own line-of-sight check %s -- bots %s see through walls.",
                    Available ? "is available" : "is NOT on this build",
                    Available ? "cannot" : "can still");
        }

        if (Available != 1 || !AIController || !Them)
            return true;

        FVector NoViewPoint{};

        return AIController->LineOfSightTo(Them, NoViewPoint, false);
    }

    
    
    static std::map<void*, std::pair<void*, int>> BotLastSaw;

    static bool BotCanSee(AAIController* AIController, AFortPlayerPawnAthena* Me, const FVector& MyLoc,
                          AFortPlayerPawnAthena* Them, AActor* ThemActor, int TickNow, bool bStrict = false)
    {
        if (!Me || !Them)
            return false;

        const FVector ThemLoc = Them->K2_GetActorLocation();

        const bool bInFront = BotIsLookingAt(Me, MyLoc, ThemLoc);
        const bool bClearLine = bInFront && BotHasLineOfSight(AIController, ThemActor);

        if (bInFront && bClearLine)
        {
            BotLastSaw[(void*)Me] = { (void*)Them, TickNow };
            return true;
        }

        
        
        if (bStrict)
            return false;

        
        auto Remembered = BotLastSaw.find((void*)Me);

        if (Remembered != BotLastSaw.end() && Remembered->second.first == (void*)Them)
        {
            int Memory = FConfiguration::BotMemoryTicks;

            if (Memory < 0)
                Memory = 0;

            if ((TickNow - Remembered->second.second) <= Memory)
                return true;

            BotLastSaw.erase(Remembered);
        }

        return false;
    }

    
    
    static bool BotHasLanded(void* Pawn)
    {
        if (!Pawn)
            return false;
        if (!bBusDeparted)
            return false;
        if (MovedToIsland.find(Pawn) == MovedToIsland.end())
            return false;
        if (GlidingBots.find(Pawn) != GlidingBots.end())
            return false;

        
        
        return true;
    }

    
    
    static bool BotCombatIsOpen()
    {
        if (!bBusDeparted)
            return false;

        auto World = UWorld::GetWorld();
        if (!World || !World->GameState)
            return false;

        auto GS = (AFortGameStateAthena*)World->GameState;
        if (PlayersAreProtected(GS))
            return false;

        
        if (GS->HasGamePhase() && GS->GamePhase < GamePhase_SafeZones)
            return false;

        return true;
    }

    int SpawnBots(const FVector& Origin, int Count, float ScatterRadius, const std::wstring& SpawnerDataAssetPath)
    {
        if (Count > 100)
        {
            LogLine("[BotAI] Requested %d bots, capping at 100.\n", Count);
            Count = 100;
        }

        auto World = UWorld::GetWorld();
        if (!World)
        {
            LogLine("[BotAI] No world yet, can't spawn bots.\n");
            return 0;
        }

        auto AISystem = (UAthenaAISystem*)World->AISystem;
        if (!AISystem)
        {
            LogLine("[BotAI] World->AISystem is null -- can't spawn bots yet (too early in match startup?).\n");
            return 0;
        }

        static bool bBotManagerRequested = false;
        if (!bBotManagerRequested)
        {
            bBotManagerRequested = true;
            auto GameModeForManager = (AFortGameMode*)World->AuthorityGameMode;
            if (GameModeForManager)
            {
                auto Manager = GameModeForManager->GetServerBotManager();
                LogLine("[BotAI] GetServerBotManager() returned %s.\n", Manager ? "a manager" : "NULL");
            }
        }

        auto AISpawner = AISystem->AISpawner;
        if (!AISpawner)
        {
            LogLine("[BotAI] AISystem->AISpawner is null -- can't spawn bots.\n");
            return 0;
        }

        std::wstring path = SpawnerDataAssetPath.empty()
            ? L"/Game/Athena/AI/Phoebe/BP_AISpawnerData_Phoebe.BP_AISpawnerData_Phoebe_C"
            : SpawnerDataAssetPath;

        auto SpawnerDataClass = FindObject<UClass>(path.c_str());
        if (!SpawnerDataClass)
        {
            LogLine(
                "[BotAI] Could not find spawner data class at path (wide-string, check console encoding): "
                "this asset path may not exist on this build. Pass a different SpawnerDataAssetPath to "
                "BotAI::SpawnBots to try an alternative.\n");
            return 0;
        }

        TArray<AActor*> PlayerStartsLocal;
        static auto WarmupStartClass = FindClass("FortPlayerStartWarmup");
        if (WarmupStartClass)
            UGameplayStatics::GetAllActorsOfClass(World, WarmupStartClass, &PlayerStartsLocal);

        if (PlayerStartsLocal.Num() == 0)
        {
            LogLine(
                "[BotAI] No FortPlayerStartWarmup actors found -- refusing to spawn bots, since you asked "
                "for spawn-island only and I have no confirmed spawn-island location to use. This usually "
                "means it's too early (world/warmup not fully loaded yet) or this build names the class "
                "differently -- try again in a few seconds.\n");
            return 0;
        }

        int spawned = 0;
        for (int i = 0; i < Count; i++)
        {
            auto Start = PlayerStartsLocal[rand() % PlayerStartsLocal.Num()];
            if (!Start)
                continue;
            FVector SpawnLoc = Start->K2_GetActorLocation();

            FTransform Transform{};
            Transform.Translation = SpawnLoc;
            FVector Scale = FVector(1, 1, 1);
            Transform.Scale3D = Scale;

            auto ComponentList = UFortAthenaAISpawnerData::CreateComponentListFromClass(SpawnerDataClass, World);
            if (!ComponentList)
            {
                LogLine("[BotAI] CreateComponentListFromClass returned null for bot %d, skipping.\n", i);
                continue;
            }

            int32 RequestID = AISpawner->RequestSpawn(ComponentList, Transform);
            if (RequestID >= 0)
            {
                spawned++;
            }
            else
            {
                LogLine("[BotAI] RequestSpawn failed (returned %d) for bot %d.\n", RequestID, i);
            }
        }

        LogLine("[BotAI] Queued %d/%d bot spawn request(s) via the native AI spawner.\n", spawned, Count);
        if (spawned == 0)
        {
            LogLine(
                "[BotAI] Zero bots queued. Check the messages above -- most likely AISystem wasn't ready "
                "yet (try calling this a moment after match start rather than immediately), or the spawner "
                "data asset path doesn't exist on this build.\n");
        }

        return spawned;
    }

    bool MoveSomeBotsToIsland(int MaxThisTick)
    {
        auto World = UWorld::GetWorld();
        if (!World || !World->GameState)
            return true;

        auto GameState = (AFortGameStateAthena*)World->GameState;
        if (!GameState->HasPlayerArray())
            return true;

        static std::vector<FVector> LandingPlan;
        static size_t LandingPlanCursor = 0;
        static bool bPlanBuilt = false;

        if (!bPlanBuilt)
        {
            bPlanBuilt = true;

            auto Normalize = [](std::string s) -> std::string
            {
                std::string out;
                for (char ch : s)
                {
                    if (ch == ' ' || ch == '\'')
                        continue;
                    out += (char)tolower((unsigned char)ch);
                }
                return out;
            };

            TArray<AActor*> PoiVolumesRaw;
            static auto PoiVolumeClass = FindClass("FortPoiVolume");
            if (PoiVolumeClass)
                UGameplayStatics::GetAllActorsOfClass(World, PoiVolumeClass, &PoiVolumesRaw);

            LogLine("[BotAI] Found %d POI volume(s) in the level.\n", PoiVolumesRaw.Num());

            struct FoundPoi { FVector Loc; std::vector<std::string> Tags; bool bUsed; };
            std::vector<FoundPoi> FoundPois;
            for (int i = 0; i < PoiVolumesRaw.Num(); i++)
            {
                auto Poi = (AFortPoiVolume*)PoiVolumesRaw[i];
                if (!Poi)
                    continue;

                FoundPoi fp;
                fp.Loc = Poi->K2_GetActorLocation();
                fp.bUsed = false;

                if (Poi->HasLocationTags())
                {
                    for (int t = 0; t < Poi->LocationTags.GameplayTags.Num(); t++)
                    {
                        auto& Tag = Poi->LocationTags.GameplayTags.Get(t, FGameplayTag::Size());
                        auto WideStr = Tag.TagName.ToString();
                        std::string TagStr(WideStr.begin(), WideStr.end());
                        fp.Tags.push_back(Normalize(TagStr));
                    }
                }
                FoundPois.push_back(fp);
            }

            std::vector<FoundPoi*> RealPois;
            for (auto& fp : FoundPois)
            {
                for (auto& tag : fp.Tags)
                {
                    if (tag.find("location.poi.") != std::string::npos)
                    {
                        RealPois.push_back(&fp);

                        LogLine("[BotAI] Real POI: %-40s at X=%.0f Y=%.0f Z=%.0f\n",
                               tag.c_str(), (float)fp.Loc.X, (float)fp.Loc.Y, (float)fp.Loc.Z);
                        break;
                    }
                }
            }

            LogLine("[BotAI] %zu real POI volume(s) identified out of %zu total volumes.\n",
                   RealPois.size(), FoundPois.size());

            std::vector<FVector> Plan;

            FVector BoundsOrigin{};
            FVector BoundsExtent{};
            bool bHaveBounds = false;

            if (GameState->HasMapInfo() && GameState->MapInfo && GameState->MapInfo->HasCachedPlayableBoundsForClients())
            {
                auto& B = GameState->MapInfo->CachedPlayableBoundsForClients;
                BoundsOrigin = B.Origin;
                BoundsExtent = B.BoxExtent;

                if (BoundsExtent.X > 1000.0 && BoundsExtent.Y > 1000.0)
                    bHaveBounds = true;
                LogLine("[BotAI] Playable bounds: origin X=%.0f Y=%.0f Z=%.0f extent X=%.0f Y=%.0f (usable=%s)\n",
                       (float)BoundsOrigin.X, (float)BoundsOrigin.Y, (float)BoundsOrigin.Z,
                       (float)BoundsExtent.X, (float)BoundsExtent.Y, bHaveBounds ? "yes" : "no");
            }
            else
            {
                LogLine("[BotAI] MapInfo/CachedPlayableBoundsForClients unavailable.\n");
            }

            if (!bHaveBounds)
            {
                BoundsOrigin = FVector(0, 0, 0);
                BoundsExtent = FVector(112000, 112000, 0);
                LogLine("[BotAI] Using fallback playable bounds +/-112000 (derived from level overlay extents).\n");
            }

            {
                int total = FConfiguration::AutoFillTargetPlayerCount;
                if (total < 1) total = 100;

                
                
                int cols = (int)ceil(sqrt((double)total));
                if (cols < 1) cols = 1;
                int rows = (total + cols - 1) / cols;

                double usableX = BoundsExtent.X * 1.55;
                double usableY = BoundsExtent.Y * 1.55;
                double startX = BoundsOrigin.X - usableX * 0.5;
                double startY = BoundsOrigin.Y - usableY * 0.5;
                double cellW = usableX / (double)cols;
                double cellH = usableY / (double)rows;

                for (int i = 0; i < total; i++)
                {
                    int cx = i % cols;
                    int cy = i / cols;

                    double jx = ((double)(rand() % 1000) / 1000.0) * cellW * 0.85 + cellW * 0.075;
                    double jy = ((double)(rand() % 1000) / 1000.0) * cellH * 0.85 + cellH * 0.075;

                    FVector Spot{};
                    Spot.X = startX + cx * cellW + jx;
                    Spot.Y = startY + cy * cellH + jy;
                    Spot.Z = 0.0;
                    Plan.push_back(Spot);
                }

                LogLine("[BotAI] Built %zu widely-spread landing spots on a %dx%d grid (usable %.0fx%.0f).\n",
                       Plan.size(), cols, rows, (float)usableX, (float)usableY);
            }

            
            
            {
                const char* Wanted = FConfiguration::HotDropPoi;
                int HowMany = FConfiguration::HotDropBots;

                if (HowMany > (int)Plan.size())
                    HowMany = (int)Plan.size();

                if (Wanted && *Wanted && HowMany > 0)
                {
                    FVector Middle{};
                    bool bFound = false;

                    
                    
                    const std::string WantedTag = Normalize(Wanted);

                    for (auto* Poi : RealPois)
                    {
                        for (auto& tag : Poi->Tags)
                        {
                            if (tag.find(WantedTag) == std::string::npos)
                                continue;

                            
                            
                            
                            
                            const double Away = sqrt((double)Poi->Loc.X * (double)Poi->Loc.X +
                                                     (double)Poi->Loc.Y * (double)Poi->Loc.Y);

                            if (Away < 1000.0)
                            {
                                LogLine("[BotAI] The level's \"%s\" volume says it is at the map origin, "
                                        "which cannot be right -- using the recording instead.", Wanted);
                                continue;
                            }

                            Middle = Poi->Loc;
                            bFound = true;
                            break;
                        }

                        if (bFound)
                            break;
                    }

                    if (bFound)
                        LogLine("[BotAI] Hot drop \"%s\" found as a POI volume at X=%.0f Y=%.0f.",
                                Wanted, (float)Middle.X, (float)Middle.Y);
                    else if (PlaceDump::GetLandmark(Wanted, Middle))
                    {
                        bFound = true;
                        LogLine("[BotAI] Hot drop \"%s\" taken from the recorded places at X=%.0f Y=%.0f.",
                                Wanted, (float)Middle.X, (float)Middle.Y);
                    }

                    if (!bFound)
                        LogLine("[BotAI] Hot drop \"%s\" is not a POI volume on this map and there is no "
                                "recording by that name -- everyone spreads out as usual.", Wanted);
                    else
                    {
                        std::vector<FVector> Spots((size_t)HowMany);

                        int Got = PlaceDump::GetPlacesNear(Middle, FConfiguration::HotDropRadius,
                                                           HowMany, Spots.data());

                        
                        if (Got <= 0)
                        {
                            for (int i = 0; i < HowMany; i++)
                            {
                                const double Angle = ((double)(rand() % 3600) / 10.0) * 3.14159265358979323846 / 180.0;
                                const double Away = (double)FConfiguration::HotDropRadius * ((double)(rand() % 1000) / 1000.0);

                                Spots[(size_t)i] = Middle;
                                Spots[(size_t)i].X += cos(Angle) * Away;
                                Spots[(size_t)i].Y += sin(Angle) * Away;
                            }

                            Got = HowMany;
                        }

                        for (int i = 0; i < Got && i < (int)Plan.size(); i++)
                            Plan[(size_t)i] = Spots[(size_t)i];

                        LogLine("[BotAI] %d bot(s) are dropping on \"%s\"; the other %d spread out over the map.",
                                Got, Wanted, (int)Plan.size() - Got);
                    }
                }
            }

            for (size_t i = Plan.size(); i > 1; i--)
            {
                size_t j = (size_t)(rand() % (int)i);
                std::swap(Plan[i - 1], Plan[j]);
            }

            LandingPlan = Plan;
            LogLine("[BotAI] Landing plan built: %zu destinations.\n", LandingPlan.size());
        }

        if (LandingPlan.empty())
        {
            LogLine("[BotAI] Landing plan is empty -- can't place bots on the island.\n");
            return true;
        }

        int movedThisTick = 0;
        bool anyRemaining = false;

        for (int i = 0; i < GameState->PlayerArray.Num(); i++)
        {
            if (movedThisTick >= MaxThisTick)
            {
                anyRemaining = true;
                break;
            }

            auto PlayerState = (AFortPlayerStateAthena*)GameState->PlayerArray[i];
            if (!PlayerState || !PlayerState->HasbIsABot() || !PlayerState->bIsABot || NPCs::IsNPC(PlayerState))
                continue;

            auto Controller = (AFortPlayerControllerAthena*)PlayerState->Owner;
            if (!Controller)
                continue;

            auto Pawn = (AFortPlayerPawnAthena*)Controller->Pawn;
            if (!Pawn)
                continue;

            if (MovedToIsland.find((void*)Pawn) != MovedToIsland.end())
                continue;
            MovedToIsland.insert((void*)Pawn);

            FVector PoiLoc = LandingPlan[LandingPlanCursor % LandingPlan.size()];
            LandingPlanCursor++;

            FVector Dest = PoiLoc;

            Dest.Z = 15500.0;

            FVector Before = Pawn->K2_GetActorLocation();

            
            
            
            
            
            bool bTeleported = Pawn->K2_TeleportTo(Dest, Pawn->K2_GetActorRotation());
            if (!bTeleported)
                Pawn->K2_SetActorLocation(Dest, false, (void*)nullptr, true);

            
            
            
            
            Pawn->BeginSkydiving(true);

            
            if (Pawn->HasbIsSkydivingFromBus() && !Pawn->bIsSkydivingFromBus)
                Pawn->bIsSkydivingFromBus = true;
            if (Pawn->HasbIsSkydiving() && !Pawn->bIsSkydiving)
            {
                Pawn->bIsSkydiving = true;
                bool bWasSkydiving = false;
                Pawn->OnRep_IsSkydiving(bWasSkydiving);
            }

            
            const UObject* Glider = PickBotGlider();
            if (Glider)
                BotGliderOf[(void*)Pawn] = Glider;

            FVector After = Pawn->K2_GetActorLocation();
            double movedDistSq = (After.X - Before.X) * (After.X - Before.X)
                               + (After.Y - Before.Y) * (After.Y - Before.Y);

            if (LandingLogCount < 12)
            {
                LandingLogCount++;
                LogLine("[BotAI] Land#%d: target X=%.0f Y=%.0f Z=%.0f | teleport=%s | ended X=%.0f Y=%.0f Z=%.0f | moved %.0f units | skydiving=%s | glider=%s\n",
                       LandingLogCount,
                       (float)Dest.X, (float)Dest.Y, (float)Dest.Z,
                       bTeleported ? "OK" : "REFUSED->forced",
                       (float)After.X, (float)After.Y, (float)After.Z,
                       (float)sqrt(movedDistSq),
                       (Pawn->HasbIsSkydiving() && Pawn->bIsSkydiving) ? "yes" : "NO",
                       Glider ? Glider->Name.ToString().c_str() : "default");
            }

            const float DropTime = (float)UGameplayStatics::GetTimeSeconds(World);

            PendingParachuteOpen[(void*)Pawn] = DropTime + 3.f;

            
            GlidingBots[(void*)Pawn] = (float)After.Z;
            GlidingSettledFor[(void*)Pawn] = 0;

            FBotGlide& Glide = BotGlides[(void*)Pawn];
            Glide = FBotGlide{};
            Glide.JumpTime = DropTime;
            Glide.JumpZ = (float)After.Z;
            Glide.LastDescentZ = (float)After.Z;
            Glide.LastDescentTime = DropTime;

            movedThisTick++;
        }

        return !anyRemaining;
    }

    void ThankAllBots()
    {
        if (!bBusDeparted)
            return;

        auto World = UWorld::GetWorld();
        if (!World || !World->GameState)
            return;

        auto GameState = (AFortGameStateAthena*)World->GameState;
        if (!GameState->HasPlayerArray())
            return;

        float Now = (float)UGameplayStatics::GetTimeSeconds(World);
        if ((Now - LastThankTime) < 1.0f)
            return;
        LastThankTime = Now;

        int thanksThisPass = 3;

        for (int i = 0; i < GameState->PlayerArray.Num(); i++)
        {
            auto PlayerState = (AFortPlayerStateAthena*)GameState->PlayerArray[i];
            if (!PlayerState || !PlayerState->HasbIsABot() || !PlayerState->bIsABot || NPCs::IsNPC(PlayerState))
                continue;

            auto Controller = (AActor*)PlayerState->Owner;
            if (!Controller)
                continue;

            if (ThankedBots.find((void*)Controller) != ThankedBots.end())
                continue;
            ThankedBots.insert((void*)Controller);

            if (PlayerState->HasbThankedBusDriver())
            {
                PlayerState->bThankedBusDriver = true;
                PlayerState->OnRep_ThankedBusDriver();

                if (PlayerState->HasPlayerNamePrivate())
                {
                    auto Wide = PlayerState->PlayerNamePrivate.ToString();
                    std::string Narrow(Wide.begin(), Wide.end());

                    if (!Narrow.empty())
                        LogLine("[BUS] %s thanked the bus driver.", Narrow.c_str());
                }
            }

            if (--thanksThisPass <= 0)
                break;
        }
    }

    void LogHudState()
    {
        auto World = UWorld::GetWorld();
        if (!World || !World->GameState || !World->AuthorityGameMode)
            return;

        auto GameState = (AFortGameStateAthena*)World->GameState;
        auto GameMode = (AFortGameMode*)World->AuthorityGameMode;
        if (!GameState->HasPlayerArray())
            return;

        static float LastHudLog = -1000.f;
        float Now = (float)UGameplayStatics::GetTimeSeconds(World);
        if ((Now - LastHudLog) < 5.0f)
            return;
        LastHudLog = Now;

        int trueAliveBots = 0;
        int trueAliveHumans = 0;
        int botsWithPawn = 0;
        int botsDeadPawn = 0;

        for (int i = 0; i < GameState->PlayerArray.Num(); i++)
        {
            auto PS = (AFortPlayerStateAthena*)GameState->PlayerArray[i];
            if (!PS) continue;
            bool bIsBot = PS->HasbIsABot() && PS->bIsABot;

            auto OwnerActor = (AActor*)PS->Owner;
            AFortPlayerPawnAthena* Pw = nullptr;
            if (OwnerActor)
                Pw = (AFortPlayerPawnAthena*)((AFortPlayerControllerAthena*)OwnerActor)->Pawn;

            bool bAlive = Pw && Pw->GetHealth() > 0.f;

            if (bIsBot)
            {
                if (Pw) botsWithPawn++; else botsDeadPawn++;
                if (bAlive) trueAliveBots++;
            }
            else if (bAlive)
            {
                trueAliveHumans++;
            }
        }

        int hudPlayersLeft = GameState->HasPlayersLeft() ? (int)GameState->PlayersLeft : -1;
        int hudTeamsLeft   = GameState->HasTeamsLeft()   ? (int)GameState->TeamsLeft   : -1;
        int aliveArrayNum  = GameMode->HasAlivePlayers() ? GameMode->AlivePlayers.Num() : -1;
        int trueAlive      = trueAliveBots + trueAliveHumans;

        LogLine("[HUD] PlayersLeft(shown)=%d TeamsLeft(shown)=%d | TRUE alive=%d (bots=%d humans=%d) | AlivePlayers[]=%d PlayerArray=%d | bots w/pawn=%d w/o=%d%s\n",
               hudPlayersLeft, hudTeamsLeft, trueAlive, trueAliveBots, trueAliveHumans,
               aliveArrayNum, GameState->PlayerArray.Num(), botsWithPawn, botsDeadPawn,
               (hudPlayersLeft != trueAlive) ? "  <<< MISMATCH: HUD disagrees with reality" : "");

        for (int i = 0; i < GameState->PlayerArray.Num(); i++)
        {
            auto PS = (AFortPlayerStateAthena*)GameState->PlayerArray[i];
            if (!PS) continue;
            bool bIsBot = PS->HasbIsABot() && PS->bIsABot;

            int kills = PS->HasKillScore() ? (int)PS->KillScore : -1;
            int killScore = PS->HasKillScore() ? (int)PS->KillScore : -1;

            if (!bIsBot)
            {
                std::string nm;
                if (PS->HasPlayerNamePrivate())
                {
                    auto w = PS->PlayerNamePrivate.ToString();
                    nm.assign(w.begin(), w.end());
                }
                LogLine("[HUD] Human '%s': Kills(shown)=%d KillScore=%d\n", nm.c_str(), kills, killScore);
            }
            else if (kills > 0)
            {
                LogLine("[HUD] Bot has %d kill(s) (KillScore=%d)\n", kills, killScore);
            }
        }
    }

    void UpdateHudCounts()
    {
        auto World = UWorld::GetWorld();
        if (!World || !World->GameState)
            return;

        auto GameState = (AFortGameStateAthena*)World->GameState;
        if (!GameState->HasPlayerArray() || !GameState->HasPlayersLeft())
            return;

        int trueAlive = 0;
        for (int i = 0; i < GameState->PlayerArray.Num(); i++)
        {
            auto PS = (AFortPlayerStateAthena*)GameState->PlayerArray[i];
            if (!PS)
                continue;

            auto OwnerActor = (AActor*)PS->Owner;
            if (!OwnerActor)
                continue;

            auto Pw = (AFortPlayerPawnAthena*)((AFortPlayerControllerAthena*)OwnerActor)->Pawn;
            if (Pw && Pw->GetHealth() > 0.f)
                trueAlive++;
        }

        if (trueAlive <= 0)
            return;

        if ((int)GameState->PlayersLeft != trueAlive)
        {
            int before = (int)GameState->PlayersLeft;
            GameState->PlayersLeft = trueAlive;
            GameState->OnRep_PlayersLeft();
            if (GameState->HasTeamsLeft())
                GameState->TeamsLeft = trueAlive;

            LogLine("[HUD] PlayersLeft corrected %d -> %d (live recount).\n", before, trueAlive);
        }
    }

    static void NoOpExecFunction(void*, void*, void*)
    {
        static int Calls = 0;

        if (++Calls <= 5)
        {
            printf("[Storm] A silenced distant-storm function was called (%d) -- did nothing, as intended.\n", Calls);
            fflush(stdout);
        }
    }

    static void SilenceDistantStorm()
    {
        static bool bDone = false;
        if (bDone)
            return;

        static int Countdown = 0;
        static int Attempts = 0;

        if (Countdown-- > 0)
            return;

        Countdown = 15;

        if (++Attempts > 240)
        {
            bDone = true;
            printf("[SafeZone] Never found the safe zone indicator class -- distant storm left as it is.\n");
            fflush(stdout);
            return;
        }

        const UStruct* IndicatorClass = (const UStruct*)FindClass("SafeZoneIndicator_C");

        if (!IndicatorClass)
        {
            const int Total = TUObjectArray::Num();

            for (int i = 0; i < Total; i++)
            {
                auto Obj = TUObjectArray::GetObjectByIndex(i);

                if (!Obj || !Obj->Class || !(Obj->Class->GetCastFlags() & 0x20))
                    continue;

                auto RawName = Obj->Name.ToString();
                std::string Lowered;
                for (const char* p = RawName.c_str(); *p; p++)
                    Lowered += (char)tolower((unsigned char)*p);

                if (Lowered.find("safezoneindicator") != std::string::npos)
                {
                    IndicatorClass = (const UStruct*)Obj;
                    break;
                }
            }
        }

        if (!IndicatorClass)
            return;

        bDone = true;

        int Silenced = 0;

        for (const UStruct* Cls = IndicatorClass; Cls; Cls = Cls->GetSuper())
        {
            for (const UField* Child = Cls->GetChildren(); Child; Child = Child->GetNext())
            {
                if (!Child->Class || !(Child->Class->GetCastFlags() & 0x80000))
                    continue;

                auto RawName = Child->GetName().ToString();
                std::string Lowered;
                for (const char* p = RawName.c_str(); *p; p++)
                    Lowered += (char)tolower((unsigned char)*p);

                if (Lowered.find("distant") == std::string::npos && Lowered.find("oneshot") == std::string::npos)
                    continue;

                Hooking::ExecHook((UFunction*)Child, (void*)&NoOpExecFunction);
                Silenced++;

                printf("[SafeZone] Silenced cosmetic storm function \"%s\".\n", RawName.c_str());
            }
        }

        if (Silenced == 0)
            printf("[SafeZone] Found the indicator class but no distant-storm functions on it.\n");
        else
            printf("[SafeZone] %d distant-storm function(s) silenced -- that is the one that crashes.\n", Silenced);

        fflush(stdout);
    }

    
    
    static const float kZoneWait[] = {
        170.f, 
        120.f, 
         90.f, 
         80.f, 
         50.f, 
         30.f, 
          0.f, 
          0.f, 
          0.f, 
         10.f, 
    };
    static const float kZoneShrink[] = {
        210.f, 
        120.f, 
         90.f, 
         70.f, 
         60.f, 
         60.f, 
         55.f, 
         50.f, 
         75.f, 
         10.f, 
    };
    static const int kZoneCount = (int)(sizeof(kZoneWait) / sizeof(kZoneWait[0]));

    static void ApplyCustomZoneTimings(AFortGameMode* GameMode)
    {
        if (!GameMode || !GameMode->HasSafeZoneIndicator() || !GameMode->SafeZoneIndicator)
            return;

        auto Indicator = GameMode->SafeZoneIndicator;
        if (!Indicator->HasSafeZonePhases())
            return;

        auto& Phases = Indicator->SafeZonePhases;
        const int N = Phases.Num();
        if (N <= 0)
            return;

        static bool bLogged = false;
        for (int i = 0; i < N; i++)
        {
            
            
            int idx = i;
            if (idx >= kZoneCount)
                idx = kZoneCount - 1;

            auto& Phase = Phases.Get(i, FFortSafeZonePhaseInfo::Size());
            if (FFortSafeZonePhaseInfo::HasWaitTime())
            {
                float W = kZoneWait[idx];
                Phase.WaitTime = W;
            }
            if (FFortSafeZonePhaseInfo::HasShrinkTime())
            {
                float S = kZoneShrink[idx];
                Phase.ShrinkTime = S;
            }
        }

        if (!bLogged)
        {
            bLogged = true;
            LogLine("[SafeZone] Applied custom phase timings (%d phases).", N);
        }
    }

    static void EnsureStormTimingData(AFortGameStateAthena* GameState)
    {
        static bool bDone = false;

        if (bDone || !GameState || !GameState->HasMapInfo() || !GameState->MapInfo)
            return;

        auto MapInfo = GameState->MapInfo;

        if (!MapInfo->HasSafeZoneDefinition())
            return;

        bDone = true;

        auto& Definition = MapInfo->SafeZoneDefinition;

        
        if (FFortSafeZoneDefinition::HasWaitTime())
        {
            auto& WaitTime = Definition.WaitTime;
            WaitTime.Value = kZoneWait[0];
            WaitTime.Curve.CurveTable = nullptr;
            GStormMapDataWasRepaired = true;
            printf("[SafeZone] Definition WaitTime forced to %.0fs (phase 1 form).\n", kZoneWait[0]);
        }

        if (FFortSafeZoneDefinition::HasShrinkTime())
        {
            auto& ShrinkTime = Definition.ShrinkTime;
            ShrinkTime.Value = kZoneShrink[0];
            ShrinkTime.Curve.CurveTable = nullptr;
            GStormMapDataWasRepaired = true;
            printf("[SafeZone] Definition ShrinkTime forced to %.0fs (phase 1 close).\n", kZoneShrink[0]);
        }

        fflush(stdout);
    }

    static bool GetSafeZoneCircle(AFortGameMode* GameMode, FVector& OutCentre, float& OutRadius)
    {
        if (!GameMode || !GameMode->HasSafeZoneIndicator() || !GameMode->SafeZoneIndicator)
            return false;

        auto Indicator = GameMode->SafeZoneIndicator;

        bool bHaveCentre = false;

        if (Indicator->HasNextCenter())
        {
            OutCentre = Indicator->NextCenter;
            bHaveCentre = true;
        }
        else if (Indicator->HasLastCenter())
        {
            OutCentre = Indicator->LastCenter;
            bHaveCentre = true;
        }
        else if (Indicator->HasPreviousCenter())
        {
            OutCentre = Indicator->PreviousCenter;
            bHaveCentre = true;
        }

        if (!bHaveCentre)
            return false;

        OutRadius = 0.f;
        if (Indicator->HasNextRadius())
            OutRadius = (float)Indicator->NextRadius;
        if (!(OutRadius > 0.f) && Indicator->HasLastRadius())
            OutRadius = (float)Indicator->LastRadius;
        if (!(OutRadius > 0.f) && Indicator->HasPreviousRadius())
            OutRadius = (float)Indicator->PreviousRadius;

        if (!(OutRadius > 0.f))
            return false;

        if (OutCentre.X == 0.0 && OutCentre.Y == 0.0 && OutCentre.Z == 0.0)
            return false;

        return true;
    }

    static int GetStormPhase(AFortGameMode* GameMode)
    {
        if (!GameMode)
            return 0;

        if (GameMode->HasSafeZoneIndicator() && GameMode->SafeZoneIndicator && GameMode->SafeZoneIndicator->HasCurrentPhase())
            return (int)GameMode->SafeZoneIndicator->CurrentPhase;

        if (GameMode->HasSafeZonePhase())
            return (int)GameMode->SafeZonePhase;

        return 0;
    }

    static float StormDamagePerSecond(int Phase)
    {
        static const float Table[] = {
             1.f,
             1.f,
             1.f,
             2.f,
             5.f,
             8.f,
             10.f,
             10.f,
             10.f,
             10.f,
        };

        const int Count = (int)(sizeof(Table) / sizeof(Table[0]));

        if (Phase < 0)
            Phase = 0;
        if (Phase >= Count)
            Phase = Count - 1;

        float Damage = Table[Phase] * FConfiguration::StormDamageScale;

        if (!(Damage > 0.f))
            Damage = 0.f;

        return Damage;
    }

    static int GStormTick = 0;

    static std::map<void*, float> LastKnownHealth;

    static std::map<void*, int> LastPlayerDamageTick;

    static std::map<void*, int> LastOutsideZoneTick;

    static std::map<void*, FVector> LastKnownLocation;

    static void ApplyStormDamage(AFortGameMode* GameMode, AFortGameStateAthena* GameState, int TickNow)
    {
        if (!FConfiguration::bManageStormDamage || FConfiguration::bDisableStorm)
        {
            if (GameState && GameState->HasPlayerArray())
            {
                for (int i = 0; i < GameState->PlayerArray.Num(); i++)
                {
                    auto PS = (AFortPlayerStateAthena*)GameState->PlayerArray[i];
                    if (!PS)
                        continue;

                    auto Owner = (AActor*)PS->Owner;
                    if (!Owner)
                        continue;

                    auto Pawn = (AFortPlayerPawnAthena*)((AFortPlayerControllerAthena*)Owner)->Pawn;
                    if (!Pawn)
                        continue;

                    if (Pawn->HasbIsInsideSafeZone() && !Pawn->bIsInsideSafeZone)
                        LastOutsideZoneTick[(void*)Owner] = TickNow;
                }
            }

            return;
        }

        if (!GameMode || !GameState || !GameState->HasPlayerArray())
            return;

        const float Now = (float)UGameplayStatics::GetTimeSeconds(GameState);

        static float LastRunTime = 0.f;
        float Delta = Now - LastRunTime;
        LastRunTime = Now;

        if (!(Delta > 0.f) || Delta > 1.0f)
            return;

        const int Phase = GetStormPhase(GameMode);
        const float PerSecond = StormDamagePerSecond(Phase);
        const float ThisTick = PerSecond * Delta;

        static int Reported = 0;

        for (int i = 0; i < GameState->PlayerArray.Num(); i++)
        {
            auto PS = (AFortPlayerStateAthena*)GameState->PlayerArray[i];
            if (!PS)
                continue;

            auto Owner = (AActor*)PS->Owner;
            if (!Owner)
                continue;

            auto Pawn = (AFortPlayerPawnAthena*)((AFortPlayerControllerAthena*)Owner)->Pawn;
            if (!Pawn)
                continue;

            const float Health = Pawn->GetHealth();
            auto Previous = LastKnownHealth.find((void*)Owner);
            const float Before = (Previous != LastKnownHealth.end()) ? Previous->second : Health;

            LastKnownHealth[(void*)Owner] = Health;

            if (Health <= 0.f)
                continue;

            const bool bOutside = Pawn->HasbIsInsideSafeZone() && !Pawn->bIsInsideSafeZone;
            if (!bOutside)
                continue;

            LastOutsideZoneTick[(void*)Owner] = TickNow;

            if (Pawn->HasbIsInvulnerable() && Pawn->bIsInvulnerable)
                continue;

            auto Hit = LastPlayerDamageTick.find((void*)Owner);
            const bool bTradingShots = (Hit != LastPlayerDamageTick.end()) && ((TickNow - Hit->second) < 20);

            float Take = ThisTick;

            if (!bTradingShots)
            {
                const float Lost = Before - Health;

                if (Lost > 0.f)
                    Take = ThisTick - Lost;
            }

            if (!(Take > 0.f))
                continue;

            float Target = Health - Take;

            if (Target < 0.f)
                Target = 0.f;

            Pawn->SetHealth(Target);
            LastKnownHealth[(void*)Owner] = Target;

            if (++Reported <= 3)
                LogLine("[ZONE] Storm holding at %.1f health a second (phase %d): took %.2f off this tick%s.",
                        PerSecond, Phase, Take, bTradingShots ? " on top of the shot they just took" : "");
        }
    }

    static void LogStormState(AFortGameMode* GameMode, AFortGameStateAthena* GameState)
    {
        if (!FConfiguration::bStormLog || !(FConfiguration::StormLogIntervalSeconds > 0.f))
            return;

        if (!GameMode || !GameMode->HasSafeZoneIndicator() || !GameMode->SafeZoneIndicator)
            return;

        auto Indicator = GameMode->SafeZoneIndicator;

        const float Now = (float)UGameplayStatics::GetTimeSeconds(GameState);

        static float NextReport = 0.f;
        if (Now < NextReport)
            return;
        NextReport = Now + FConfiguration::StormLogIntervalSeconds;

        const int Phase = Indicator->HasCurrentPhase()
                              ? (int)Indicator->CurrentPhase
                              : (GameMode->HasSafeZonePhase() ? (int)GameMode->SafeZonePhase : -1);

        const float Start = Indicator->HasSafeZoneStartShrinkTime() ? (float)Indicator->SafeZoneStartShrinkTime : -1.f;
        const float Finish = Indicator->HasSafeZoneFinishShrinkTime() ? (float)Indicator->SafeZoneFinishShrinkTime : -1.f;
        const float NextRadius = Indicator->HasNextRadius() ? (float)Indicator->NextRadius : -1.f;

        float CurrentRadius = -1.f;
        if (Indicator->HasPreviousRadius() && Indicator->PreviousRadius > 0.f)
            CurrentRadius = (float)Indicator->PreviousRadius;
        else if (Indicator->HasLastRadius())
            CurrentRadius = (float)Indicator->LastRadius;

        const char* What = (Now < Start) ? "holding" : (Now < Finish ? "CLOSING" : "due to move on");

        printf("[Storm] phase %d, %s -- starts closing in %.1fs, done in %.1fs | radius %.0f -> %.0f\n",
               Phase, What, Start - Now, Finish - Now, CurrentRadius, NextRadius);
        fflush(stdout);
    }

    static std::map<void*, std::pair<void*, int>> BotTopDamager;
    
    static std::map<void*, std::pair<void*, int>> BotLastDamager;

    static AFortPlayerStateAthena* ResolveDamagerState(AActor* Causer)
    {
        if (!Causer)
            return nullptr;

        if (Causer->IsA<AFortPlayerStateAthena>())
            return (AFortPlayerStateAthena*)Causer;

        if (Causer->IsA<AFortPlayerPawnAthena>())
        {
            auto PawnController = (AActor*)((AFortPlayerPawnAthena*)Causer)->Controller;

            return PawnController ? (AFortPlayerStateAthena*)((AFortPlayerControllerAthena*)PawnController)->PlayerState : nullptr;
        }

        if (Causer->GetOffset("PlayerState") != (uint32)-1)
            return (AFortPlayerStateAthena*)((AFortPlayerControllerAthena*)Causer)->PlayerState;

        return nullptr;
    }

    static int32 DamagerInfoSize()
    {
        static int32 Size = -1;

        if (Size == -1)
        {
            auto Struct = FindStruct("DamagerInfo");
            Size = Struct ? Struct->GetPropertiesSize() : (int32)sizeof(FDamagerInfo);

            if (Size <= 0 || Size > 0x200)
                Size = (int32)sizeof(FDamagerInfo);
        }

        return Size;
    }

    static void WipeTheSlateAtBusTime(AFortGameStateAthena* GameState)
    {
        static bool bDone = false;

        if (!GameState || !GameState->HasPlayerArray())
            return;

        if (PlayersAreProtected(GameState))
        {
            bDone = false;
            LandedBots.clear();
            return;
        }

        if (bDone)
            return;

        bDone = true;

        int Healed = 0;

        for (int i = 0; i < GameState->PlayerArray.Num(); i++)
        {
            auto PS = (AFortPlayerStateAthena*)GameState->PlayerArray[i];
            if (!PS || !PS->HasOwner() || !PS->Owner)
                continue;

            auto Owner = (AActor*)PS->Owner;
            auto Pawn = (AFortPlayerPawnAthena*)((AFortPlayerControllerAthena*)Owner)->Pawn;
            if (!Pawn)
                continue;

            const float Health = Pawn->GetHealth();

            if (Health > 0.f && Health < 100.f)
            {
                float Full = 100.f;
                Pawn->SetHealth(Full);
                Healed++;
            }

            LastKnownHealth.erase((void*)Owner);
            LastPlayerDamageTick.erase((void*)Owner);
            BotTopDamager.erase((void*)Owner);
            BotLastDamager.erase((void*)Owner);
            ShotAtBy.erase((void*)Owner);
            AggroedByHuman.erase((void*)Owner);
        }

        LogLine("[LANDED] Everyone is on the ground. %d player(s) healed back to full and every scratch from the island "
                "and the bus forgotten. Damage and eliminations start counting from here.", Healed);
    }

    
    
    
    
    
    
    
    
    
    
    
    
    
    static constexpr double kTile   = 512.0;  
    static constexpr double kFloor  = 384.0;  
    static constexpr double kSub    = 128.0;  

    static double SnapTile(double v)
    {
        return round(v / kTile) * kTile;
    }

    static double SnapFloorZ(double v)
    {
        
        return round(v / kFloor) * kFloor;
    }

    
    
    
    
    
    
    static bool BuildFacesEastWest(double Yaw)
    {
        const long Quarter = lround(Yaw / 90.0);
        return (Quarter % 2) != 0;
    }

    static FVector SnapBuildLocation(const FVector& V, double Yaw)
    {
        if (BuildFacesEastWest(Yaw))
            return FVector{ SnapTile(V.X - 256.0) + 256.0, SnapTile(V.Y), SnapFloorZ(V.Z) };

        
        return FVector{ SnapTile(V.X), SnapTile(V.Y - 256.0) + 256.0, SnapFloorZ(V.Z) };  
    }

    static FRotator SnapBuildRotation(const FRotator& R)
    {
        FRotator Out{};
        Out.Pitch = 0.0;
        Out.Roll  = 0.0;
        Out.Yaw   = round(R.Yaw / 90.0) * 90.0;
        return Out;
    }

    static UClass* ResolveBuildClass(const wchar_t* Path, const char* ShortName)
    {
        UClass* Cls = const_cast<UClass*>(FindObject<UClass>(Path));
        if (!Cls)
            Cls = const_cast<UClass*>(FindClass(ShortName));
        return Cls;
    }

    static ABuildingSMActor* SpawnBuildPiece(UClass* Class, const FVector& Loc, const FRotator& Rot,
                                             AFortPlayerControllerAthena* Controller, AFortPlayerStateAthena* PS)
    {
        if (!Class || !Controller)
            return nullptr;

        FRotator GridRot = SnapBuildRotation(Rot);
        FVector Snapped = SnapBuildLocation(Loc, GridRot.Yaw);

        ABuildingSMActor* Building = UWorld::SpawnActorUnfinished<ABuildingSMActor>(Class, Snapped, GridRot, Controller);
        if (!Building)
            return nullptr;

        Building->InitializeKismetSpawnedBuildingActor(Building, Controller, true, nullptr, false);
        UWorld::FinishSpawnActor(Building, Snapped, GridRot);

        Building->CurrentBuildingLevel = 0;
        Building->OnRep_CurrentBuildingLevel();
        Building->SetMirrored(false);
        Building->bPlayerPlaced = true;

        if (PS && PS->HasTeamIndex())
            Building->Team = PS->TeamIndex;
        if (Building->HasTeamIndex())
            Building->TeamIndex = Building->Team;

        Building->ForceNetUpdate();
        return Building;
    }

    
    
    
    static void TryBotReactiveBuild(AFortGameStateAthena* GameState)
    {
        if (!GameState || !GameState->HasPlayerArray())
            return;

        const int BuildAfterTicks = 0;

        static UClass* WallWood = nullptr;
        static UClass* WallBrick = nullptr;
        static UClass* WallMetal = nullptr;
        static bool bResolved = false;

        if (!bResolved)
        {
            bResolved = true;
            WallWood  = ResolveBuildClass(L"/Game/Building/ActorBlueprints/Player/Wood/L1/PBWA_W1_Solid.PBWA_W1_Solid_C", "PBWA_W1_Solid_C");
            WallBrick = ResolveBuildClass(L"/Game/Building/ActorBlueprints/Player/Stone/L1/PBWA_S1_Solid.PBWA_S1_Solid_C", "PBWA_S1_Solid_C");
            WallMetal = ResolveBuildClass(L"/Game/Building/ActorBlueprints/Player/Metal/L1/PBWA_M1_Solid.PBWA_M1_Solid_C", "PBWA_M1_Solid_C");

            if (!WallWood)
                WallWood = ResolveBuildClass(L"/Game/Building/ActorBlueprints/Player/Wood/L1/PBWA_W1_Brace.PBWA_W1_Brace_C", "PBWA_W1_Brace_C");

            if (!WallWood && !WallBrick && !WallMetal)
            {
                LogLine("[BUILD] Could not resolve any wall class -- reactive building disabled.");
                return;
            }
            LogLine("[BUILD] Tile=512 (5m) Floor=384 | classes ready.");
        }

        if (!WallWood && !WallBrick && !WallMetal)
            return;

        for (int i = 0; i < GameState->PlayerArray.Num(); i++)
        {
            auto PS = (AFortPlayerStateAthena*)GameState->PlayerArray[i];
            if (!PS || !PS->HasbIsABot() || !PS->bIsABot || NPCs::IsNPC(PS))
                continue;

            auto Controller = (AFortPlayerControllerAthena*)PS->Owner;
            if (!Controller)
                continue;

            if (BotAlreadyBuiltWall.find((void*)Controller) != BotAlreadyBuiltWall.end())
                continue;

            auto Since = BotUnderFireSince.find((void*)Controller);
            if (Since == BotUnderFireSince.end())
                continue;
            if ((GStormTick - Since->second) > 60)
                continue;
            if ((GStormTick - Since->second) < BuildAfterTicks)
                continue;

            auto Pawn = (AFortPlayerPawnAthena*)Controller->Pawn;
            if (!Pawn || Pawn->GetHealth() <= 0.f)
                continue;

            if (!BotHasLanded((void*)Pawn))
                continue;
            if (GlidingBots.find((void*)Pawn) != GlidingBots.end())
                continue;
            if (Pawn->HasbIsSkydiving() && Pawn->bIsSkydiving)
                continue;
            if (Pawn->HasbIsSkydivingFromBus() && Pawn->bIsSkydivingFromBus)
                continue;
            if (Pawn->HasbIsParachuteOpen() && Pawn->bIsParachuteOpen)
                continue;

            FVector Loc = Pawn->K2_GetActorLocation();
            if (Loc.Z > 8000.0)
                continue;

            
            
            AFortPlayerPawnAthena* Threat = nullptr;
            const char* Toward = "the way it was facing";

            {
                auto Locked = BotCombatTarget.find((void*)Pawn);

                if (Locked != BotCombatTarget.end() && Locked->second && Locked->second->GetHealth() > 0.f)
                {
                    Threat = Locked->second;
                    Toward = "the one it is fighting";
                }
                else
                {
                    auto Shooter = BotLastDamager.find((void*)Controller);

                    if (Shooter != BotLastDamager.end() && Shooter->second.first)
                    {
                        auto ShooterState = (AFortPlayerStateAthena*)Shooter->second.first;

                        if (ShooterState->HasOwner() && ShooterState->Owner)
                        {
                            auto ShooterPawn = (AFortPlayerPawnAthena*)((AFortPlayerControllerAthena*)ShooterState->Owner)->Pawn;

                            if (ShooterPawn && ShooterPawn != Pawn && ShooterPawn->GetHealth() > 0.f)
                            {
                                Threat = ShooterPawn;
                                Toward = "whoever just shot it";
                            }
                        }
                    }
                }
            }

            FRotator Facing = Pawn->K2_GetActorRotation();

            if (Threat)
            {
                const FVector ThreatLoc = Threat->K2_GetActorLocation();
                const double ToX = ThreatLoc.X - Loc.X;
                const double ToY = ThreatLoc.Y - Loc.Y;

                if ((ToX * ToX + ToY * ToY) > 1.0)
                {
                    double ThreatYaw = atan2(ToY, ToX) * (180.0 / 3.14159265358979323846);
                    Facing.Yaw = ThreatYaw;
                }
            }

            
            FRotator GridFacing = SnapBuildRotation(Facing);

            const double YawRad = GridFacing.Yaw * (3.14159265358979323846 / 180.0);
            const double Fx = cos(YawRad);
            const double Fy = sin(YawRad);
            const bool bEastWest = BuildFacesEastWest(GridFacing.Yaw);

            double Ahead = (double)FConfiguration::BotWallDistance;
            if (!(Ahead > 0.0))
                Ahead = kTile + kTile;

            
            
            
            FVector Wall1{};

            if (!bEastWest)
            {
                Wall1.X = SnapTile(Loc.X + (Fx > 0.0 ? Ahead : -Ahead));  
                Wall1.Y = SnapTile(Loc.Y - 256.0) + 256.0;                
            }
            else
            {
                Wall1.X = SnapTile(Loc.X - 256.0) + 256.0;
                Wall1.Y = SnapTile(Loc.Y + (Fy > 0.0 ? Ahead : -Ahead));
            }

            
            
            
            const double FloorZ = floor(Loc.Z / kFloor) * kFloor;
            Wall1.Z = FloorZ;

            UClass* WallCls = WallWood;
            int MatRoll = rand() % 100;
            if (MatRoll >= 70 && WallBrick) WallCls = WallBrick;
            else if (MatRoll >= 90 && WallMetal) WallCls = WallMetal;
            if (!WallCls) WallCls = WallWood ? WallWood : (WallBrick ? WallBrick : WallMetal);

            const bool bPlaced = SpawnBuildPiece(WallCls, Wall1, GridFacing, Controller, PS) != nullptr;

            BotAlreadyBuiltWall.insert((void*)Controller);

            if (bPlaced)
            {
                static int WallReports = 0;

                const double Away = bEastWest ? fabs(Wall1.Y - Loc.Y) : fabs(Wall1.X - Loc.X);
                const char* Direction = !bEastWest ? (Fx > 0.0 ? "north" : "south") : (Fy > 0.0 ? "east" : "west");

                const int ModX = ((int)llround(Wall1.X) % 512 + 512) % 512;
                const int ModY = ((int)llround(Wall1.Y) % 512 + 512) % 512;

                if (++WallReports <= 25)
                    LogLine("[BUILD] '%s' put up ONE wall %.1f m to its %s, towards %s | X=%.0f Y=%.0f Z=%.0f "
                            "(X%%512=%d Y%%512=%d -- where your own %s walls snap).",
                            SafeNameOf(PS).c_str(), (float)(Away / 100.0), Direction, Toward,
                            (float)Wall1.X, (float)Wall1.Y, (float)Wall1.Z, ModX, ModY,
                            bEastWest ? "east/west-facing" : "north/south-facing");
            }
        }
    }

    void TrackBotDamage()
    {
        auto World = UWorld::GetWorld();
        if (!World || !World->GameState)
            return;

        auto GameState = (AFortGameStateAthena*)World->GameState;
        if (!GameState->HasPlayerArray())
            return;

        const int32 Stride = DamagerInfoSize();

        for (int i = 0; i < GameState->PlayerArray.Num(); i++)
        {
            auto PS = (AFortPlayerStateAthena*)GameState->PlayerArray[i];
            if (!PS || !PS->HasbIsABot() || !PS->bIsABot || NPCs::IsNPC(PS))
                continue;

            auto Controller = (AActor*)PS->Owner;
            if (!Controller)
                continue;

            auto Pawn = (AFortPlayerPawnAthena*)((AFortPlayerControllerAthena*)Controller)->Pawn;
            if (!Pawn || !Pawn->HasDamagers())
                continue;

            auto& Damagers = Pawn->Damagers;

            if (Damagers.Num() <= 0 || !Damagers.GetData())
                continue;

            void* Causer = nullptr;
            int CauserDamage = 0;

            for (int d = Damagers.Num() - 1; d >= 0; d--)
            {
                auto& Entry = Damagers.Get(d, Stride);

                auto DamagerState = ResolveDamagerState(Entry.DamageCauser);
                if (!DamagerState || DamagerState == PS)
                    continue;

                Causer = (void*)DamagerState;
                CauserDamage = Entry.DamageAmount;
                break;
            }

            {
                auto Previous = BotTopDamager.find((void*)Controller);
                const bool bChanged = (Previous == BotTopDamager.end()) ||
                                      (Previous->second.first != Causer) ||
                                      (Previous->second.second != CauserDamage);

                if (bChanged && Causer)
                {
                    auto CauserState = (AFortPlayerStateAthena*)Causer;
                    if (CauserState->HasbIsABot() && CauserState->bIsABot)
                    {
                        GCombatBotOnBotHits++;
                        

                        
                        
                        if (CauserState->HasOwner() && CauserState->Owner)
                        {
                            auto ShooterPawn = (AFortPlayerPawnAthena*)((AFortPlayerControllerAthena*)CauserState->Owner)->Pawn;
                            auto ShooterLock = BotCombatTarget.find((void*)ShooterPawn);

                            if (ShooterLock != BotCombatTarget.end() && ShooterLock->second == Pawn)
                            {
                                GHitsOnLockedTarget++;
                            }
                            else
                            {
                                GHitsOnSomebodyElse++;

                                if (GHitsOnSomebodyElse <= 10)
                                {
                                    auto LockName = BotLockedOnName.find((void*)ShooterPawn);

                                    LogLine("[HITS] '%s' hit '%s', but the one it is locked onto is '%s'.",
                                            SafeNameOf(CauserState).c_str(), SafeNameOf(PS).c_str(),
                                            LockName != BotLockedOnName.end() ? LockName->second.c_str() : "nobody");
                                }
                            }
                        }
                    }
                    else
                    {
                        
                        AggroedByHuman[(void*)Controller] = { Causer, GStormTick };
                    }
                }
            }

            BotTopDamager[(void*)Controller] = { Causer, CauserDamage };

            
            
            if (Causer)
            {
                auto PrevLast = BotLastDamager.find((void*)Controller);
                const bool bNewDamager = (PrevLast == BotLastDamager.end()) || (PrevLast->second.first != Causer);

                BotLastDamager[(void*)Controller] = { Causer, GStormTick };
                
                BotTopDamager[(void*)Controller] = { Causer, CauserDamage };

                BotUnderFireSince[(void*)Controller] = GStormTick;

                
                if (bNewDamager)
                    BotAlreadyBuiltWall.erase((void*)Controller);
            }
        }
    }

    static void TrackEveryBotAlive(AFortGameStateAthena* GameState, int TickNow)
    {
        if (!GameState || !GameState->HasPlayerArray())
            return;

        for (int i = 0; i < GameState->PlayerArray.Num(); i++)
        {
            auto PS = (AFortPlayerStateAthena*)GameState->PlayerArray[i];
            if (!PS || !PS->HasbIsABot() || !PS->bIsABot || NPCs::IsNPC(PS))
                continue;

            auto Controller = (AActor*)PS->Owner;
            if (!Controller)
                continue;

            auto Pawn = (AFortPlayerPawnAthena*)((AFortPlayerControllerAthena*)Controller)->Pawn;
            if (!Pawn || Pawn->GetHealth() <= 0.f)
                continue;

            if (BotTopDamager.find((void*)Controller) == BotTopDamager.end())
                BotTopDamager[(void*)Controller] = { nullptr, 0 };

            auto Tracked = BotTopDamager.find((void*)Controller);
            if (Tracked != BotTopDamager.end() && Tracked->second.first)
            {
                static std::map<void*, std::pair<void*, int>> LastSeenLedger;

                auto Seen = LastSeenLedger.find((void*)Controller);
                if (Seen == LastSeenLedger.end() || Seen->second != Tracked->second)
                {
                    LastSeenLedger[(void*)Controller] = Tracked->second;
                    LastPlayerDamageTick[(void*)Controller] = TickNow;
                }
            }
        }
    }

    static FGameplayTagContainer* ResolveDamageTags(AFortPlayerPawnAthena* Pawn, FGameplayTagContainer& Fallback)
    {
        if (!Pawn)
            return &Fallback;

        const int32 Anchor = Pawn->GetOffset("MoveSoundStimulusBroadcastInterval");
        if (Anchor == -1)
            return &Fallback;

        const int32 Delta = (VersionInfo.FortniteVersion >= 11 && VersionInfo.FortniteVersion < 18) ? 0x18 : 0x10;
        auto Candidate = (FGameplayTagContainer*)(__int64(Pawn) + Anchor + Delta);

        if (IsBadReadPtr((void*)Candidate))
            return &Fallback;

        return Candidate;
    }

    static bool DiedToTheStorm(void* Controller, int TickNow)
    {
        if (!Controller)
            return false;

        auto Outside = LastOutsideZoneTick.find(Controller);
        if (Outside == LastOutsideZoneTick.end() || (TickNow - Outside->second) > 90)
            return false;

        auto Hit = LastPlayerDamageTick.find(Controller);
        if (Hit != LastPlayerDamageTick.end() && (TickNow - Hit->second) < 150)
            return false;

        return true;
    }

    static AFortPlayerStateAthena* FindBotKiller(AActor* BotController, AFortPlayerStateAthena* VictimState,
                                                 AFortGameStateAthena* GameState, const char** OutHow)
    {
        const char* How = "nobody";
        AFortPlayerStateAthena* Killer = nullptr;

        if (PlayersAreProtected(GameState))
        {
            if (OutHow)
                *OutHow = "the match had not started";
            return nullptr;
        }

        
        auto Last = BotLastDamager.find((void*)BotController);
        if (Last != BotLastDamager.end() && Last->second.first)
        {
            Killer = (AFortPlayerStateAthena*)Last->second.first;
            How = "last damager";
            GElimsFromLedger++;
        }

        auto Tracked = BotTopDamager.find((void*)BotController);
        const bool bHaveLedger = (Tracked != BotTopDamager.end());

        if (!Killer && bHaveLedger && Tracked->second.first)
        {
            Killer = (AFortPlayerStateAthena*)Tracked->second.first;
            How = "damage ledger";
            GElimsFromLedger++;
        }

        if (!Killer)
        {
            auto Shooter = ShotAtBy.find((void*)BotController);

            if (Shooter != ShotAtBy.end() && Shooter->second.first &&
                (GStormTick - Shooter->second.second) <= ShotAtByValidTicks)
            {
                Killer = (AFortPlayerStateAthena*)Shooter->second.first;
                How = "was being shot at by them";
                GElimsFromShotAt++;
            }
        }

        if (!Killer && GameState)
        {
            AFortPlayerStateAthena* OnlyHuman = nullptr;
            AFortPlayerStateAthena* Nearest = nullptr;
            double NearestDistance = 0.0;
            int HumanCount = 0;

            FVector VictimAt{};
            bool bKnowWhereItDied = false;

            auto Where = LastKnownLocation.find((void*)BotController);
            if (Where != LastKnownLocation.end())
            {
                VictimAt = Where->second;
                bKnowWhereItDied = true;
            }

            for (int i = 0; i < GameState->PlayerArray.Num(); i++)
            {
                auto PS = (AFortPlayerStateAthena*)GameState->PlayerArray[i];
                if (!PS || (PS->HasbIsABot() && PS->bIsABot) || NPCs::IsNPC(PS))
                    continue;

                HumanCount++;
                if (!OnlyHuman)
                    OnlyHuman = PS;

                if (!bKnowWhereItDied || PS == VictimState)
                    continue;

                auto HumanController = (AFortPlayerControllerAthena*)PS->Owner;
                if (!HumanController)
                    continue;

                auto HumanPawn = (AFortPlayerPawnAthena*)HumanController->Pawn;
                if (!HumanPawn || HumanPawn->GetHealth() <= 0.f)
                    continue;

                const FVector HumanAt = HumanPawn->K2_GetActorLocation();

                const double DX = HumanAt.X - VictimAt.X;
                const double DY = HumanAt.Y - VictimAt.Y;
                const double DZ = HumanAt.Z - VictimAt.Z;
                const double Distance = sqrt(DX * DX + DY * DY + DZ * DZ);

                if (!Nearest || Distance < NearestDistance)
                {
                    Nearest = PS;
                    NearestDistance = Distance;
                }
            }

            if (HumanCount == 1)
            {
                Killer = OnlyHuman;
                How = "only player connected";
                GElimsFromFallback++;
            }
            else if (Nearest)
            {
                double CreditRange = (double)FConfiguration::ElimCreditRange;
                if (!(CreditRange > 0.0))
                    CreditRange = 25000.0;

                if (NearestDistance <= CreditRange)
                {
                    Killer = Nearest;
                    How = "nearest player to the body";
                    GElimsFromFallback++;
                }
                else
                {
                    How = "nobody close enough to credit";
                }
            }
        }

        if (Killer == VictimState)
        {
            Killer = nullptr;
            How = "nobody (self)";
        }

        if (OutHow)
            *OutHow = How;

        return Killer;
    }

    
    
    static std::set<void*> LootAlreadyDropped;

    static void DropBotLoot(AFortPlayerControllerAthena* Controller, AFortPlayerPawnAthena* Pawn)
    {
        if (!Controller)
            return;

        if (LootAlreadyDropped.find((void*)Controller) != LootAlreadyDropped.end())
            return;

        LootAlreadyDropped.insert((void*)Controller);

        auto VictimPS = (AFortPlayerStateAthena*)Controller->PlayerState;

        
        
        FVector dropLoc{};
        bool bHaveDropSpot = false;

        if (Pawn)
        {
            dropLoc = Pawn->K2_GetActorLocation();
            bHaveDropSpot = true;
        }
        else
        {
            auto Seen = LastKnownLocation.find((void*)Controller);

            if (Seen != LastKnownLocation.end())
            {
                dropLoc = Seen->second;
                bHaveDropSpot = true;
            }
        }

        if (!bHaveDropSpot)
        {
            if (VictimPS)
                BotInventories.erase((void*)VictimPS);

            static int noSpotReported = 0;
            if (++noSpotReported <= 3)
                LogLine("[DROP] A bot died with no pawn and no last known position -- nothing to drop onto.");

            return;
        }

        dropLoc.Z += 100.0;

        int droppedCount = 0;
        int fromInventory = 0;
        bool bDroppedMaterials = false;

        
        auto DropOne = [&](const UFortItemDefinition* Definition, int Count, int LoadedAmmo)
        {
            if (!Definition || Count <= 0)
                return;

            FVector spot = dropLoc;
            float ang = (float)(rand() % 360) * (3.14159265f / 180.f);
            spot.X += cosf(ang) * (float)(40 + (rand() % 90));
            spot.Y += sinf(ang) * (float)(40 + (rand() % 90));

            AFortInventory::SpawnPickup(spot, Definition, Count, LoadedAmmo,
                                        EFortPickupSourceTypeFlag::GetPlayer(),
                                        EFortPickupSpawnSource::GetPlayerElimination());
            droppedCount++;
        };

        if (VictimPS)
        {
            auto invIt = BotInventories.find((void*)VictimPS);

            if (invIt != BotInventories.end() && invIt->second)
            {
                auto Inv = invIt->second;
                auto& Entries = Inv->Inventory.ReplicatedEntries;

                for (int e = 0; e < Entries.Num(); e++)
                {
                    auto& Entry = Entries.Get(e, FFortItemEntry::Size());
                    if (!Entry.HasItemDefinition() || !Entry.ItemDefinition)
                        continue;

                    
                    if (Entry.ItemDefinition->Cast<UFortWeaponMeleeItemDefinition>())
                        continue;

                    int cnt  = Entry.HasCount() ? Entry.Count : 1;
                    int ammo = Entry.HasLoadedAmmo() ? Entry.LoadedAmmo : 0;
                    if (cnt <= 0)
                        cnt = 1;

                    DropOne(Entry.ItemDefinition, cnt, ammo);
                    fromInventory++;
                }

                BotInventories.erase(invIt);
            }
        }

        
        
        if (droppedCount == 0 && Pawn && Pawn->HasCurrentWeapon() && Pawn->CurrentWeapon)
        {
            auto Held = (AFortWeapon*)Pawn->CurrentWeapon;

            if (Held->HasWeaponData() && Held->WeaponData)
                DropOne((const UFortItemDefinition*)Held->WeaponData, 1, 30);
        }

        
        
        if (droppedCount == 0 && VictimPS)
        {
            auto wep = BotWeapons.find((void*)VictimPS);

            if (wep != BotWeapons.end() && wep->second)
            {
                int Clip = 30;
                auto ClipIt = BotWeaponClipSize.find((void*)VictimPS);

                if (ClipIt != BotWeaponClipSize.end() && ClipIt->second > 0)
                    Clip = ClipIt->second;

                DropOne(wep->second, 1, Clip);

                if (wep->second->Cast<UFortWeaponRangedItemDefinition>())
                {
                    
                    
                    static const wchar_t* SpareAmmo[] = {
                        L"AthenaAmmoDataBulletsMedium",
                        L"AthenaAmmoDataBulletsLight",
                        L"AthenaAmmoDataShells",
                        L"AthenaAmmoDataBulletsHeavy",
                    };

                    auto AmmoDef = FindItemDefByName(SpareAmmo[rand() % (int)(sizeof(SpareAmmo) / sizeof(SpareAmmo[0]))]);
                    DropOne(AmmoDef, 60, 0);
                }

                DropOne(FindItemDefByName(L"Athena_Bandage"), 3, 0);
            }
        }

        
        if (droppedCount > 0)
        {
            static const wchar_t* Materials[] = { L"WoodItemData", L"StoneItemData", L"MetalItemData" };

            auto MatDef = FindItemDefByName(Materials[rand() % 3]);

            if (MatDef)
            {
                DropOne(MatDef, 30 + (rand() % 60), 0);
                bDroppedMaterials = true;
            }
        }

        if (VictimPS)
        {
            BotWeapons.erase((void*)VictimPS);
            BotWeaponClipSize.erase((void*)VictimPS);
        }

        static int dropsReported = 0;
        if (++dropsReported <= 6)
            LogLine("[DROP] Bot died -- spawned %d pickup(s) at the death location (%d from its inventory; materials %s).",
                    droppedCount, fromInventory, bDroppedMaterials ? "yes" : "NO");
    }

    static void HandleBotDeath(AFortPlayerControllerAthena* DeadBotController)
    {
        auto World = UWorld::GetWorld();
        if (!World || !World->GameState || !DeadBotController)
            return;

        auto GameState = (AFortGameStateAthena*)World->GameState;
        if (!GameState->HasPlayerArray())
            return;

        auto VictimState = (AFortPlayerStateAthena*)DeadBotController->PlayerState;

        const char* How = "nobody";
        AFortPlayerStateAthena* KillerState = FindBotKiller((AActor*)DeadBotController, VictimState, GameState, &How);

        const bool bStormDeath = DiedToTheStorm((void*)DeadBotController, GStormTick);

        if (bStormDeath && KillerState)
        {
            LogLine("[ELIM] Bot died in the storm -- not credited to '%s'.", How);
            KillerState = nullptr;
            How = "the storm";
        }

        auto DeadPawn = (AFortPlayerPawnAthena*)DeadBotController->Pawn;

        
        
        
        DropBotLoot(DeadBotController, DeadPawn);

        BotTopDamager.erase((void*)DeadBotController);
        BotLastDamager.erase((void*)DeadBotController);
        BotUnderFireSince.erase((void*)DeadBotController);
        BotAlreadyBuiltWall.erase((void*)DeadBotController);
        LastPlayerDamageTick.erase((void*)DeadBotController);
        LastKnownHealth.erase((void*)DeadBotController);
        LastOutsideZoneTick.erase((void*)DeadBotController);

        ShotAtBy.erase((void*)DeadBotController);
        AggroedByHuman.erase((void*)DeadBotController);

        LandedBots.erase((void*)DeadBotController);
        LastKnownLocation.erase((void*)DeadBotController);

        if (VictimState && VictimState->HasDeathInfo())
        {
            FGameplayTagContainer EmptyTags{};
            FGameplayTagContainer* SourceTags = ResolveDamageTags(DeadPawn, EmptyTags);

            uint8 Cause =  0;

            if (!bStormDeath)
            {
                Cause = ToDeathCause(DeadPawn, *SourceTags, false);
                if (Cause == 0 && KillerState)
                    Cause = 5;
            }

            if (FDeathInfo::HasKiller())
                VictimState->DeathInfo.Killer = KillerState;
            if (FDeathInfo::HasFinisherOrDowner())
                VictimState->DeathInfo.FinisherOrDowner = KillerState ? (AActor*)KillerState : (AActor*)VictimState;
            if (FDeathInfo::HasbDBNO())
                VictimState->DeathInfo.bDBNO = false;
            if (FDeathInfo::HasDeathTags())
                VictimState->DeathInfo.DeathTags = *SourceTags;
            if (FDeathInfo::HasVictimTags() && DeadPawn)
                VictimState->DeathInfo.VictimTags = DeadPawn->GameplayTags;
            if (FDeathInfo::HasDeathClassSlot())
                VictimState->DeathInfo.DeathClassSlot = -1;
            if (FDeathInfo::HasDeathCause())
                VictimState->DeathInfo.DeathCause = Cause;
            if (FDeathInfo::HasbInitialized())
                VictimState->DeathInfo.bInitialized = true;

            VictimState->OnRep_DeathInfo();

            LogLine("[ELIM] Killfeed cause = %d (%s).", (int)Cause, bStormDeath ? "the storm" : How);
        }

        if (!KillerState || KillerState == VictimState)
        {
            LogLine("[ELIM] Bot died with no identifiable killer (%s) -- no credit given.", How);
            return;
        }

        LogLine("[ELIM] Killer resolved via %s.", How);

        if (KillerState->HasKillScore())
        {
            int before = (int)KillerState->KillScore;
            KillerState->KillScore++;
            KillerState->OnRep_Kills();

            if (KillerState->HasTeamKillScore())
            {
                KillerState->TeamKillScore++;
                KillerState->OnRep_TeamKillScore();
            }

            struct FReportKillParams
            {
                AFortPlayerStateAthena* Victim;
                uint8_t Padding[0x8];
            };

            FReportKillParams ReportParams{ VictimState };
            KillerState->ClientReportKill(ReportParams);

            std::string vn, kn;
            if (VictimState && VictimState->HasPlayerNamePrivate())
            {
                auto w = VictimState->PlayerNamePrivate.ToString();
                vn.assign(w.begin(), w.end());
            }
            if (KillerState->HasPlayerNamePrivate())
            {
                auto w = KillerState->PlayerNamePrivate.ToString();
                kn.assign(w.begin(), w.end());
            }

            LogLine("[ELIM] '%s' killed '%s' -- KillScore %d -> %d.",
                    kn.c_str(), vn.c_str(), before, (int)KillerState->KillScore);

            
            if (FConfiguration::SiphonAmount > 0 && KillerState->HasOwner() && KillerState->Owner)
            {
                auto KillerCtrl = (AFortPlayerControllerAthena*)KillerState->Owner;
                auto KillerPawn = KillerCtrl ? (AFortPlayerPawnAthena*)KillerCtrl->Pawn : nullptr;
                if (KillerPawn && KillerPawn->GetHealth() > 0.f)
                {
                    float Health = KillerPawn->GetHealth();
                    float Shield = KillerPawn->GetShield();
                    float Remaining = (float)FConfiguration::SiphonAmount;
                    if (Health < 100.f)
                    {
                        float Need = 100.f - Health;
                        float Give = Remaining < Need ? Remaining : Need;
                        Health += Give;
                        Remaining -= Give;
                    }
                    if (Remaining > 0.f && Shield < 100.f)
                    {
                        float Need = 100.f - Shield;
                        float Give = Remaining < Need ? Remaining : Need;
                        Shield += Give;
                    }
                    KillerPawn->SetHealth(Health);
                    KillerPawn->SetShield(Shield);
                }
            }
        }
        else
        {
            LogLine("[ELIM] Real player found but KillScore property did not resolve.");
        }
    }

    void ApplyDamageProtection()
    {
        auto World = UWorld::GetWorld();
        if (!World || !World->GameState)
            return;

        auto GameState = (AFortGameStateAthena*)World->GameState;
        if (!GameState->HasPlayerArray())
            return;

        for (int i = 0; i < GameState->PlayerArray.Num(); i++)
        {
            auto PS = (AFortPlayerStateAthena*)GameState->PlayerArray[i];
            if (!PS)
                continue;

            auto Owner = (AActor*)PS->Owner;
            if (!Owner)
                continue;

            auto Pawn = (AFortPlayerPawnAthena*)((AFortPlayerControllerAthena*)Owner)->Pawn;
            if (!Pawn)
                continue;

            HasLanded((void*)Owner, Pawn);

            const bool bKnownBot = PS->HasbIsABot() && PS->bIsABot;
            const bool bKnownHuman = PS->HasbIsABot() && !PS->bIsABot && !NPCs::IsNPC(PS);

            if (Pawn->HasbIgnoreNextFallingDamage())
                Pawn->bIgnoreNextFallingDamage = true;

            Pawn->ResetFallingHeight();

            if (Pawn->HasLastFallDistance())
                Pawn->LastFallDistance = 0.f;

            const bool bProtected = !AircraftPhaseReached();

            if (bProtected)
            {
                if (Pawn->HasbCanBeDamaged())
                    Pawn->bCanBeDamaged = false;

                if (Pawn->HasbIsInvulnerable())
                    Pawn->bIsInvulnerable = true;

                const float Health = Pawn->GetHealth();

                if (Health > 0.f && Health < 100.f)
                {
                    float Full = 100.f;
                    Pawn->SetHealth(Full);
                }

                continue;
            }

            if (bKnownHuman)
            {
                auto Remembered = HumanPawnToppedUp.find((void*)Owner);
                const bool bFreshPawn = Remembered == HumanPawnToppedUp.end() || Remembered->second != (void*)Pawn;

                if (bFreshPawn)
                {
                    HumanPawnToppedUp[(void*)Owner] = (void*)Pawn;

                    
                    
                    
                    HumanTopUpUntilTick[(void*)Owner] = GStormTick + 60;

                    LogLine("[PROTECT] Real player on a new pawn -- 100 health and 100 shield.");
                }

                auto Until = HumanTopUpUntilTick.find((void*)Owner);

                if (Until != HumanTopUpUntilTick.end() && GStormTick <= Until->second)
                {
                    float Full = 100.f;

                    if (Pawn->GetHealth() < 100.f)
                        Pawn->SetHealth(Full);

                    
                    
                    if (Pawn->GetShield() <= 0.f)
                        Pawn->SetShield(Full);
                }
                else if (Until != HumanTopUpUntilTick.end())
                {
                    HumanTopUpUntilTick.erase(Until);
                }
            }

            if (Pawn->HasbCanBeDamaged() && !Pawn->bCanBeDamaged)
                Pawn->bCanBeDamaged = true;

            if (Pawn->HasbIsInvulnerable())
            {
                if (bKnownHuman && FConfiguration::bGodModeForRealPlayer)
                {
                    Pawn->bIsInvulnerable = true;
                }
                else if (Pawn->bIsInvulnerable)
                {
                    Pawn->bIsInvulnerable = false;

                    if (bKnownBot)
                        GBotsMadeKillableAgain++;
                }
            }
        }

        static bool bAnnounced = false;
        if (!bAnnounced)
        {
            bAnnounced = true;
            LogLine("[PROTECT] Fall damage off for everyone, and nothing takes damage until the aircraft phase "
                    "starts. From that moment nobody is invulnerable and everyone can be shot out of the sky.");
        }

        static int LastReported = 0;
        if (GBotsMadeKillableAgain > LastReported)
        {
            LastReported = GBotsMadeKillableAgain;
            if (LastReported <= 10)
                LogLine("[PROTECT] Found a bot stuck on god mode and made it killable again (%d so far).", LastReported);
        }
    }

    void RemoveDeadBots()
    {
        auto World = UWorld::GetWorld();
        if (!World || !World->GameState || !World->AuthorityGameMode)
            return;

        auto GameState = (AFortGameStateAthena*)World->GameState;
        auto GameMode = (AFortGameMode*)World->AuthorityGameMode;
        if (!GameState->HasPlayerArray() || !GameMode->HasAlivePlayers())
            return;

        int removed = 0;
        float lowestHealthSeen = 100.0f;
        int botsChecked = 0;

        {
            std::set<void*> presentNow;
            for (int j = 0; j < GameMode->AlivePlayers.Num(); j++)
            {
                auto Ctrl = (AFortPlayerControllerAthena*)GameMode->AlivePlayers[j];
                if (!Ctrl || !Ctrl->PlayerState || !Ctrl->PlayerState->bIsABot || NPCs::IsNPC(Ctrl))
                    continue;

                auto P = (AFortPlayerPawnAthena*)Ctrl->Pawn;
                if (P)
                {
                    HasLanded((void*)Ctrl, P);
                    LastKnownLocation[(void*)Ctrl] = P->K2_GetActorLocation();
                }
                bool bAlive = (P != nullptr) && (P->GetHealth() > 0.f);

                if (bAlive)
                    presentNow.insert((void*)Ctrl);
            }

            if (!PlayersAreProtected(GameState))
            {
                for (auto prev : SeenAliveBots)
                {
                    if (presentNow.find(prev) != presentNow.end())
                        continue;
                    if (EliminatedBots.find(prev) != EliminatedBots.end())
                        continue;

                    if (LandedBots.find(prev) == LandedBots.end())
                        continue;

                    EliminatedBots.insert(prev);
                    removed++;

                    HandleBotDeath((AFortPlayerControllerAthena*)prev);
                }
            }

            SeenAliveBots = presentNow;
        }

        for (int j = GameMode->AlivePlayers.Num() - 1; j >= 0; j--)
        {
            auto Controller = (AFortPlayerControllerAthena*)GameMode->AlivePlayers[j];
            if (!Controller || !Controller->PlayerState || !Controller->PlayerState->bIsABot || NPCs::IsNPC(Controller))
                continue;

            auto Pawn = (AFortPlayerPawnAthena*)Controller->Pawn;
            if (Pawn)
            {
                HasLanded((void*)Controller, Pawn);
                botsChecked++;
                float health = Pawn->GetHealth();
                if (health < lowestHealthSeen)
                    lowestHealthSeen = health;
            }

            bool bDead = (!Pawn || Pawn->GetHealth() <= 0.0f) && !PlayersAreProtected(GameState) &&
                         LandedBots.find((void*)Controller) != LandedBots.end();

            if (bDead && EliminatedBots.find((void*)Controller) == EliminatedBots.end())
            {
                EliminatedBots.insert((void*)Controller);
                removed++;

                
                
                
                
                
                DropBotLoot(Controller, Pawn);

                auto VictimState = (AFortPlayerStateAthena*)Controller->PlayerState;

                AFortPlayerControllerAthena* KillerController = nullptr;
                AFortPlayerStateAthena* KillerState = nullptr;

                const char* How = "nobody";
                KillerState = FindBotKiller((AActor*)Controller, VictimState, GameState, &How);

                const bool bStormDeath = DiedToTheStorm((void*)Controller, GStormTick);

                if (bStormDeath && KillerState)
                {
                    LogLine("[ELIM] Bot died outside the circle with no recent damage -- the storm killed it, not '%s'. No credit given.",
                            How);
                    KillerState = nullptr;
                    How = "the storm";
                }

                BotTopDamager.erase((void*)Controller);
                LastPlayerDamageTick.erase((void*)Controller);
                LastKnownHealth.erase((void*)Controller);
                LastOutsideZoneTick.erase((void*)Controller);
                ShotAtBy.erase((void*)Controller);
                LandedBots.erase((void*)Controller);

                if (Pawn)
                {
                    BotLastZoneDistance.erase((void*)Pawn);
                    BotZoneMoveTick.erase((void*)Pawn);
                    BotFire.erase((void*)Pawn);
                    BotFacingYaw.erase((void*)Pawn);
                }

                if (KillerState && KillerState->HasOwner())
                {
                    auto KillerOwner = (AActor*)KillerState->Owner;

                    if (KillerOwner && KillerOwner->IsA<AFortPlayerControllerAthena>())
                        KillerController = (AFortPlayerControllerAthena*)KillerOwner;
                }

                std::string victimName;
                if (VictimState && VictimState->HasPlayerNamePrivate())
                {
                    auto vw = VictimState->PlayerNamePrivate.ToString();
                    victimName.assign(vw.begin(), vw.end());
                }
                std::string killerName;
                if (KillerState && KillerState->HasPlayerNamePrivate())
                {
                    auto kw = KillerState->PlayerNamePrivate.ToString();
                    killerName.assign(kw.begin(), kw.end());
                }

                LogLine("[ELIM] '%s' killed '%s' (resolved via %s).",
                        KillerState ? killerName.c_str() : "(nobody)",
                        victimName.c_str(),
                        How);

                if (VictimState && VictimState->HasDeathInfo())
                {
                    FGameplayTagContainer EmptyTags{};
                    FGameplayTagContainer* SourceTags = ResolveDamageTags(Pawn, EmptyTags);

                    uint8 Cause =  0;

                    if (bStormDeath)
                    {
                        Cause = 0;
                    }
                    else
                    {
                        Cause = ToDeathCause(Pawn, *SourceTags, false);

                        if (Cause == 0 && KillerState)
                            Cause = 5;
                    }

                    if (FDeathInfo::HasKiller())
                        VictimState->DeathInfo.Killer = KillerState;
                    if (FDeathInfo::HasFinisherOrDowner())
                        VictimState->DeathInfo.FinisherOrDowner = KillerState ? (AActor*)KillerState : (AActor*)VictimState;
                    if (FDeathInfo::HasbDBNO())
                        VictimState->DeathInfo.bDBNO = false;
                    if (FDeathInfo::HasDeathLocation() && Pawn)
                        VictimState->DeathInfo.DeathLocation = Pawn->K2_GetActorLocation();
                    if (FDeathInfo::HasDeathTags())
                        VictimState->DeathInfo.DeathTags = *SourceTags;
                    if (FDeathInfo::HasVictimTags() && Pawn)
                        VictimState->DeathInfo.VictimTags = Pawn->GameplayTags;
                    if (FDeathInfo::HasFinisherOrDownerTags())
                    {
                        if (KillerController && KillerController->Pawn)
                            VictimState->DeathInfo.FinisherOrDownerTags = ((AFortPlayerPawnAthena*)KillerController->Pawn)->GameplayTags;
                        else if (Pawn)
                            VictimState->DeathInfo.FinisherOrDownerTags = Pawn->GameplayTags;
                    }
                    if (FDeathInfo::HasDeathClassSlot())
                        VictimState->DeathInfo.DeathClassSlot = -1;
                    if (FDeathInfo::HasDeathCause())
                        VictimState->DeathInfo.DeathCause = Cause;
                    if (FDeathInfo::HasDistance() && Pawn && KillerController && KillerController->Pawn)
                    {
                        FVector a = Pawn->K2_GetActorLocation();
                        FVector b = KillerController->Pawn->K2_GetActorLocation();
                        double dx = a.X - b.X, dy = a.Y - b.Y, dz = a.Z - b.Z;
                        VictimState->DeathInfo.Distance = (float)sqrt(dx*dx + dy*dy + dz*dz);
                    }
                    if (FDeathInfo::HasbInitialized())
                        VictimState->DeathInfo.bInitialized = true;

                    VictimState->OnRep_DeathInfo();

                    LogLine("[ELIM] Killfeed cause = %d (%s).", (int)Cause,
                            bStormDeath ? "the storm" : (KillerState ? "read from the killing blow's tags" : "no killer"));
                }

                if (KillerState)
                {
                    if (KillerState->HasKillScore())
                    {
                        int before = (int)KillerState->KillScore;
                        KillerState->KillScore++;
                        KillerState->OnRep_Kills();

                        if (KillerState->HasTeamKillScore())
                        {
                            KillerState->TeamKillScore++;
                            KillerState->OnRep_TeamKillScore();
                        }

                        struct FReportKillParams
                        {
                            AFortPlayerStateAthena* Victim;
                            uint8_t Padding[0x8];
                        };

                        FReportKillParams ReportParams{ VictimState };
                        KillerState->ClientReportKill(ReportParams);

                        LogLine("[ELIM] CREDIT: KillScore %d -> %d, TeamKillScore=%d, ClientReportKill sent.",
                                before, (int)KillerState->KillScore,
                                KillerState->HasTeamKillScore() ? (int)KillerState->TeamKillScore : -1);
                    }
                    else
                    {
                        LogLine("[ELIM] Killer found but HasKillScore()==false -- property did not resolve.");
                    }
                }
                else
                {
                    LogLine("[ELIM] No killer identified for this death -- no credit awarded.");
                }

                LogLine("[BotAI] Bot eliminated (recorded, engine owns removal).\n");
            }
        }

        static int reportCounter = 0;
        if (((++reportCounter) % 300) == 0 && botsChecked > 0)
            LogLine("[BotAI] RemoveDeadBots: %d bot(s) checked, lowest health seen = %.1f, removed this pass = %d.\n",
                   botsChecked, lowestHealthSeen, removed);
    }

    void GiveBotPickaxes()
    {
        auto World = UWorld::GetWorld();
        if (!World || !World->GameState)
            return;

        auto GameState = (AFortGameStateAthena*)World->GameState;
        if (!GameState->HasPlayerArray())
            return;

        static auto DefaultPickaxe = FindObject<UFortItemDefinition>(
            L"/Game/Athena/Items/Weapons/WID_Harvest_Pickaxe_Athena_C_T01.WID_Harvest_Pickaxe_Athena_C_T01");

        static bool bReported = false;
        if (!bReported)
        {
            bReported = true;
            LogLine("[BotAI] Default pickaxe asset: %s\n", DefaultPickaxe ? "found" : "NOT FOUND");
        }
        if (!DefaultPickaxe)
            return;

        for (int i = 0; i < GameState->PlayerArray.Num(); i++)
        {
            auto PlayerState = (AFortPlayerStateAthena*)GameState->PlayerArray[i];
            if (!PlayerState || !PlayerState->HasbIsABot() || !PlayerState->bIsABot || NPCs::IsNPC(PlayerState))
                continue;

            auto OwnerActor = (AActor*)PlayerState->Owner;
            if (!OwnerActor)
                continue;

            if (PickaxeGiven.find((void*)OwnerActor) != PickaxeGiven.end())
                continue;

            AFortInventory* Inventory = nullptr;

            if (OwnerActor->IsA<AFortPlayerControllerAthena>())
            {
                Inventory = ((AFortPlayerControllerAthena*)OwnerActor)->WorldInventory;
            }
            else
            {
                Inventory = EnsureBotInventory(OwnerActor);
            }

            if (!Inventory)
            {
                static int noInvReported = 0;
                if (++noInvReported <= 3)
                    LogLine("[BotAI] Could not create an inventory for bot yet (no real player joined?).");
                continue;
            }

            PickaxeGiven.insert((void*)OwnerActor);
            Inventory->GiveItem(DefaultPickaxe);

            static int given = 0;
            if (++given <= 3)
                LogLine("[BotAI] Gave pickaxe to bot #%d.\n", given);
        }
    }

    static uint8 AllocateFreeTeam(AFortGameStateAthena* GameState)
    {
        for (int attempt = 0; attempt < 251; attempt++)
        {
            if (AFortGameMode::CurrentTeam < 3 || AFortGameMode::CurrentTeam > 253)
                AFortGameMode::CurrentTeam = 3;

            const uint8 candidate = AFortGameMode::CurrentTeam;

            AFortGameMode::CurrentTeam++;
            AFortGameMode::PlayersOnCurTeam = 0;

            bool bTaken = false;
            if (GameState->HasPlayerArray())
            {
                for (int i = 0; i < GameState->PlayerArray.Num(); i++)
                {
                    auto Other = (AFortPlayerStateAthena*)GameState->PlayerArray[i];
                    if (!Other || !Other->HasTeamIndex())
                        continue;

                    if ((uint8)Other->TeamIndex == candidate)
                    {
                        bTaken = true;
                        break;
                    }
                }
            }

            if (!bTaken)
                return candidate;
        }

        return 3;
    }

    void AssignBotTeams()
    {
        auto World = UWorld::GetWorld();
        if (!World || !World->GameState)
            return;

        auto GameState = (AFortGameStateAthena*)World->GameState;
        if (!GameState->HasPlayerArray())
            return;

        for (int i = 0; i < GameState->PlayerArray.Num(); i++)
        {
            auto PS = (AFortPlayerStateAthena*)GameState->PlayerArray[i];
            if (!PS || !PS->HasbIsABot() || !PS->bIsABot || NPCs::IsNPC(PS))
                continue;

            auto Ctrl = (AActor*)PS->Owner;
            if (!Ctrl)
                continue;

            if (BotTeamsAssigned.find((void*)Ctrl) != BotTeamsAssigned.end())
                continue;
            BotTeamsAssigned.insert((void*)Ctrl);

            uint8 team = AllocateFreeTeam(GameState);

            uint8 oldTeam = PS->HasTeamIndex() ? PS->TeamIndex : 0;

            if (PS->HasTeamIndex())
                PS->TeamIndex = team;
            if (PS->HasSquadId())
            {
                PS->SquadId = team;
                PS->OnRep_SquadId();
            }

            static int reported = 0;
            if (++reported <= 5)
                LogLine("[TEAM] Bot assigned team %d (was %d).", (int)team, (int)oldTeam);
        }
    }

    
    
    
    
    
    
    
    
    
    
    static const UFortItemDefinition* FindItemDefByName(const wchar_t* name)
    {
        if (!name || !*name)
            return nullptr;

        
        
        
        static std::map<std::wstring, const UFortItemDefinition*> cache;
        static std::map<std::wstring, std::chrono::steady_clock::time_point> missedAt;

        auto cached = cache.find(name);
        if (cached != cache.end())
            return cached->second;

        const auto NowTp = std::chrono::steady_clock::now();

        auto missed = missedAt.find(name);
        if (missed != missedAt.end() &&
            std::chrono::duration_cast<std::chrono::seconds>(NowTp - missed->second).count() < 30)
            return nullptr;

        std::wstring n(name);
        const UFortItemDefinition* found = nullptr;

        static auto AnyClass = UObject::StaticClass();

        auto TryPath = [&](const std::wstring& Path) -> bool
        {
            found = (const UFortItemDefinition*)FindObject(Path.c_str(), AnyClass);
            return found != nullptr;
        };

        auto StartsWith = [&](const wchar_t* Prefix) -> bool
        {
            return n.rfind(Prefix, 0) == 0;
        };

        const char* How = "";

        if (n[0] == L'/')
        {
            
            if (TryPath(n))
                How = "full path";
        }
        else
        {
            
            if (TryPath(n + L"." + n))
                How = "short path";

            
            std::wstring Likely;

            if (StartsWith(L"AthenaAmmoData") || StartsWith(L"AmmoData"))
                Likely = L"/Game/Athena/Items/Ammo/";
            else if (n == L"WoodItemData" || n == L"StoneItemData" || n == L"MetalItemData")
                Likely = L"/Game/Items/ResourcePickups/";
            else if (StartsWith(L"Athena_") && n.size() > 7)
                Likely = L"/Game/Athena/Items/Consumables/" + n.substr(7) + L"/";   
            else if (StartsWith(L"WID_"))
                Likely = L"/Game/Athena/Items/Weapons/";

            if (!found && !Likely.empty() && TryPath(Likely + n + L"." + n))
                How = "its own folder";

            
            if (!found)
            {
                static const wchar_t* folders[] = {
                    L"/Game/Athena/Items/Weapons/",
                    L"/Game/Items/Weapons/",
                    L"/Game/Weapons/",
                    L"/Game/Athena/Items/Consumables/",
                    L"/Game/Items/Consumables/",
                    L"/Game/Athena/Items/Ammo/",
                    L"/Game/Items/Ammo/",
                    L"/Game/Items/ResourcePickups/",
                    L"/Game/Athena/Items/",
                };

                for (auto f : folders)
                {
                    if (Likely == f)
                        continue;   

                    if (TryPath(std::wstring(f) + n + L"." + n))
                    {
                        How = "folder search";
                        break;
                    }
                }
            }

            
            if (!found && TryPath(n))
                How = "bare name";

            
            if (!found)
            {
                std::string narrowName(n.begin(), n.end());
                found = TUObjectArray::FindObject<UFortItemDefinition>(narrowName.c_str());
                if (found)
                    How = "already loaded";
            }
        }

        std::string narrow(n.begin(), n.end());

        if (found)
        {
            cache[n] = found;
            missedAt.erase(n);

            static int hitsReported = 0;
            if (++hitsReported <= 15)
                LogLine("[ASSET] Lookup '%s' -> FOUND (%s)", narrow.c_str(), How);
        }
        else
        {
            missedAt[n] = NowTp;

            static int missesReported = 0;
            if (++missesReported <= 30)
                LogLine("[ASSET] Lookup '%s' -> NOT FOUND in any known folder (will try again in 30s)", narrow.c_str());
        }

        return found;
    }

    static AFortInventory* EnsureBotInventory(AActor* BotControllerActor)
    {
        if (!BotControllerActor)
            return nullptr;

        auto BotCtrl = (AFortAthenaAIBotController*)BotControllerActor;

        if (BotCtrl->HasInventory() && BotCtrl->Inventory)
            return BotCtrl->Inventory;

        static TSubclassOf<AFortInventory> CachedInvClass{};
        static bool bClassResolved = false;

        if (!bClassResolved)
        {
            auto World = UWorld::GetWorld();
            if (World && World->GameState)
            {
                auto GS = (AFortGameStateAthena*)World->GameState;
                if (GS->HasPlayerArray())
                {
                    for (int i = 0; i < GS->PlayerArray.Num(); i++)
                    {
                        auto PS = (AFortPlayerStateAthena*)GS->PlayerArray[i];
                        if (!PS || (PS->HasbIsABot() && PS->bIsABot))
                            continue;

                        auto PC = (AActor*)PS->Owner;
                        if (!PC || !PC->IsA<AFortPlayerControllerAthena>())
                            continue;

                        auto RealPC = (AFortPlayerControllerAthena*)PC;
                        if (RealPC->HasWorldInventoryClass() && RealPC->WorldInventoryClass.ClassPtr)
                        {
                            CachedInvClass = RealPC->WorldInventoryClass;
                            bClassResolved = true;
                            LogLine("[INV] Resolved WorldInventoryClass from the real player -- can now build bot inventories.");
                        }
                        break;
                    }
                }
            }
        }

        if (!CachedInvClass.ClassPtr)
        {
            static bool bTriedDirectClass = false;
            if (!bTriedDirectClass)
            {
                bTriedDirectClass = true;

                auto DirectClass = FindClass("FortInventory");
                if (DirectClass)
                {
                    CachedInvClass = DirectClass;
                    LogLine("[INV] Resolved inventory class directly via FindClass(\"FortInventory\").");
                }
                else
                {
                    LogLine("[INV] FAILED: could not resolve an inventory class either from a player "
                            "controller or via FindClass. Bots cannot carry items.");
                }
            }
        }

        if (!CachedInvClass.ClassPtr)
            return nullptr;

        auto NewInv = UWorld::SpawnActor<AFortInventory>(CachedInvClass, FVector{}, FRotator{}, BotControllerActor);
        if (!NewInv)
        {
            static bool bSpawnFailReported = false;
            if (!bSpawnFailReported)
            {
                bSpawnFailReported = true;
                LogLine("[INV] SpawnActor<AFortInventory> returned null -- class resolved but the actor could not be created.");
            }
            return nullptr;
        }

        if (NewInv->HasInventoryType())
            NewInv->InventoryType = 0;

        if (BotCtrl->HasInventory())
            BotCtrl->Inventory = NewInv;

        static int created = 0;
        if (++created <= 3)
            LogLine("[INV] Created inventory for bot #%d.", created);

        return NewInv;
    }

    void GiveBotLoadouts()
    {
        auto World = UWorld::GetWorld();
        if (!World || !World->GameState)
            return;

        auto GameState = (AFortGameStateAthena*)World->GameState;
        if (!GameState->HasPlayerArray())
            return;

        static const wchar_t* CommonWeapons[] = {
            L"WID_Assault_Auto_Athena_C_Ore_T02",
            L"WID_Assault_SemiAuto_Athena_C_Ore_T02",
            L"WID_Shotgun_Standard_Athena_Common",
            L"WID_Shotgun_Combat_Athena_C_Ore_T03",
            L"WID_Pistol_AutoHeavySuppressed_Athena_C_Ore_T02",
            L"WID_Pistol_SixShooter_Athena_C_Ore_T02",
            L"WID_Sniper_BoltAction_Scope_Athena_C_Ore_T03",
            L"WID_Assault_AutoHigh_Athena_VR_Ore_T03",
        };
        static const wchar_t* UncommonWeapons[] = {
            L"WID_Assault_Auto_Athena_UC_Ore_T03",
            L"WID_Assault_SemiAuto_Athena_UC_Ore_T03",
            L"WID_Assault_Surgical_Athena_UC_Ore_T03",
            L"WID_Shotgun_Standard_Athena_C_Ore_T03",
            L"WID_Shotgun_Combat_Athena_UC_Ore_T03",
            L"WID_Pistol_Scavenger_Athena_UC_Ore_T03",
            L"WID_Pistol_AutoHeavySuppressed_Athena_UC_Ore_T03",
            L"WID_Pistol_SixShooter_Athena_UC_Ore_T03",
            L"WID_Sniper_BoltAction_Scope_Athena_UC_Ore_T03",
            L"WID_Assault_AutoHigh_Athena_VR_Ore_T03",
        };
        static const wchar_t* RareWeapons[] = {
            L"WID_Assault_Stark_Athena_R_Ore_T03",
            L"WID_Assault_Auto_Athena_R_Ore_T03",
            L"WID_Assault_SemiAuto_Athena_R_Ore_T03",
            L"WID_Assault_Surgical_Athena_R_Ore_T03",
            L"WID_Shotgun_Standard_Athena_UC_Ore_T03",
            L"WID_Shotgun_Combat_Athena_R_Ore_T03",
            L"WID_Pistol_Scavenger_Athena_R_Ore_T03",
            L"WID_Pistol_AutoHeavySuppressed_Athena_R_Ore_T03",
            L"WID_Pistol_SixShooter_Athena_R_Ore_T03",
            L"WID_Sniper_BoltAction_Scope_Athena_R_Ore_T03",
            L"WID_Assault_AutoHigh_Athena_VR_Ore_T03",
        };
        static const wchar_t* EpicWeapons[] = {
            L"WID_Assault_Stark_Athena_VR_Ore_T03",
            L"WID_Assault_Auto_Athena_VR_Ore_T03",
            L"WID_Assault_SemiAuto_Athena_VR_Ore_T03",
            L"WID_Assault_Surgical_Athena_VR_Ore_T03",
            L"WID_Shotgun_Standard_Athena_VR_Ore_T03",
            L"WID_Shotgun_Combat_Athena_VR_Ore_T03",
            L"WID_Pistol_Scavenger_Athena_VR_Ore_T03",
            L"WID_Pistol_SixShooter_Athena_VR_Ore_T03",
            L"WID_Sniper_BoltAction_Scope_Athena_VR_Ore_T03",
            L"WID_Assault_AutoHigh_Athena_VR_Ore_T03",
        };
        static const wchar_t* LegendaryWeapons[] = {
            L"WID_Assault_Stark_Athena_SR_Ore_T03",
            L"WID_Assault_Auto_Athena_SR_Ore_T03",
            L"WID_Assault_SemiAuto_Athena_SR_Ore_T03",
            L"WID_Assault_Surgical_Athena_SR_Ore_T03",
            L"WID_Shotgun_Standard_Athena_SR_Ore_T03",
            L"WID_Shotgun_Combat_Athena_SR_Ore_T03",
            L"WID_Pistol_Scavenger_Athena_SR_Ore_T03",
            L"WID_Pistol_SixShooter_Athena_SR_Ore_T03",
            L"WID_Sniper_BoltAction_Scope_Athena_SR_Ore_T03",
            L"WID_Assault_AutoHigh_Athena_VR_Ore_T03",
        };

        for (int i = 0; i < GameState->PlayerArray.Num(); i++)
        {
            auto PS = (AFortPlayerStateAthena*)GameState->PlayerArray[i];
            if (!PS || !PS->HasbIsABot() || !PS->bIsABot || NPCs::IsNPC(PS))
                continue;

            auto OwnerActor = (AActor*)PS->Owner;
            if (!OwnerActor)
                continue;

            if (LoadoutGiven.find((void*)OwnerActor) != LoadoutGiven.end())
                continue;

            auto Pawn = (AFortPlayerPawnAthena*)((AFortPlayerControllerAthena*)OwnerActor)->Pawn;
            if (!Pawn)
                continue;

            LoadoutGiven.insert((void*)OwnerActor);

            {
                int Low = (int)FConfiguration::BotShieldMinimum;
                int High = (int)FConfiguration::BotShieldMaximum;

                if (Low < 0)
                    Low = 0;
                if (Low > 100)
                    Low = 100;
                if (High > 100)
                    High = 100;
                if (High < Low)
                    High = Low;

                float Shield = (float)(Low + (rand() % (High - Low + 1)));

                Pawn->SetShield(Shield);

                static int ShieldsReported = 0;
                if (++ShieldsReported <= 6)
                    LogLine("[LOADOUT] Bot given %.0f shield.", Shield);
            }

            int roll = rand() % 100;
            const wchar_t** pool = nullptr;
            int poolSize = 0;
            const char* rarityName = "";

            if (roll < 35)      { pool = CommonWeapons;    poolSize = (int)(sizeof(CommonWeapons)/sizeof(CommonWeapons[0]));       rarityName = "Common"; }
            else if (roll < 62) { pool = UncommonWeapons;  poolSize = (int)(sizeof(UncommonWeapons)/sizeof(UncommonWeapons[0]));   rarityName = "Uncommon"; }
            else if (roll < 82) { pool = RareWeapons;      poolSize = (int)(sizeof(RareWeapons)/sizeof(RareWeapons[0]));           rarityName = "Rare"; }
            else if (roll < 95) { pool = EpicWeapons;      poolSize = (int)(sizeof(EpicWeapons)/sizeof(EpicWeapons[0]));           rarityName = "Epic"; }
            else                { pool = LegendaryWeapons; poolSize = (int)(sizeof(LegendaryWeapons)/sizeof(LegendaryWeapons[0])); rarityName = "Legendary"; }

            const wchar_t* chosen = pool[rand() % poolSize];
            auto WeaponDef = FindItemDefByName(chosen);

            
            
            
            
            if (!WeaponDef)
            {
                const wchar_t** Pools[] = { pool, CommonWeapons, UncommonWeapons, RareWeapons, EpicWeapons, LegendaryWeapons };
                const int Sizes[] = {
                    poolSize,
                    (int)(sizeof(CommonWeapons) / sizeof(CommonWeapons[0])),
                    (int)(sizeof(UncommonWeapons) / sizeof(UncommonWeapons[0])),
                    (int)(sizeof(RareWeapons) / sizeof(RareWeapons[0])),
                    (int)(sizeof(EpicWeapons) / sizeof(EpicWeapons[0])),
                    (int)(sizeof(LegendaryWeapons) / sizeof(LegendaryWeapons[0])),
                };
                const char* Names[] = { rarityName, "Common", "Uncommon", "Rare", "Epic", "Legendary" };

                const wchar_t* missing = chosen;

                for (int p = 0; p < 6 && !WeaponDef; p++)
                {
                    const int Start = rand() % Sizes[p];

                    for (int k = 0; k < Sizes[p] && !WeaponDef; k++)
                    {
                        const wchar_t* Alt = Pools[p][(Start + k) % Sizes[p]];

                        if (auto AltDef = FindItemDefByName(Alt))
                        {
                            WeaponDef = AltDef;
                            chosen = Alt;
                            rarityName = Names[p];
                        }
                    }
                }

                static int swapsReported = 0;
                if (++swapsReported <= 6)
                {
                    std::wstring wm(missing), wc(chosen);
                    std::string nm(wm.begin(), wm.end()), nc(wc.begin(), wc.end());

                    std::string Outcome = WeaponDef ? ("gave '" + nc + "' instead") : std::string("NO weapon in any pool resolved");
                    LogLine("[LOADOUT] '%s' is not on this build -- %s.", nm.c_str(), Outcome.c_str());
                }
            }

            auto Inv = EnsureBotInventory(OwnerActor);

            if (Inv)
                BotInventories[(void*)PS] = Inv;

            bool bEquipTookHold = false;

            if (WeaponDef)
            {
                int ClipSize = 0;
                if (auto RangedDef = WeaponDef->Cast<UFortWeaponItemDefinition>())
                {
                    if (auto Stats = AFortInventory::GetStats(RangedDef))
                        ClipSize = Stats->ClipSize;
                }

                if (ClipSize <= 0)
                    ClipSize = 30;

                FGuid guid{};

                if (Inv)
                {
                    auto Item = Inv->GiveItem(WeaponDef, 1, ClipSize);

                    if (Item && Item->HasItemEntry() && FFortItemEntry::HasItemGuid())
                        guid = Item->ItemEntry.ItemGuid;

                    BotWeaponClipSize[(void*)PS] = ClipSize;
                }

                auto EquippedWeapon = Pawn->EquipWeaponDefinition(WeaponDef, guid);

                bEquipTookHold = (EquippedWeapon != nullptr) ||
                                 (Pawn->HasCurrentWeapon() && Pawn->CurrentWeapon != nullptr);

                BotWeapons[(void*)PS] = WeaponDef;
            }

            if (Inv)
            {
                struct AmmoEntry { const wchar_t* Name; int Count; };
                static const AmmoEntry Ammunition[] = {
                    { L"AthenaAmmoDataBulletsMedium", 180 },
                    { L"AthenaAmmoDataBulletsLight",  180 },
                    { L"AthenaAmmoDataShells",         48 },
                    { L"AthenaAmmoDataBulletsHeavy",   30 },
                    { L"AmmoDataRockets",               6 },
                };

                int AmmoStacks = 0;

                for (auto& Ammo : Ammunition)
                {
                    auto AmmoDef = FindItemDefByName(Ammo.Name);
                    if (AmmoDef)
                    {
                        Inv->GiveItem(AmmoDef, Ammo.Count);
                        AmmoStacks++;
                    }
                }

                
                
                static const wchar_t* HealItems[] = {
                    L"Athena_Bandage",
                    L"Athena_Medkit",
                    L"Athena_Shields",
                };
                const wchar_t* HealName = HealItems[rand() % 3];
                auto HealDef = FindItemDefByName(HealName);
                if (HealDef)
                    Inv->GiveItem(HealDef, 3);

                bool bGaveGrenades = false;
                if ((rand() % 100) < 30)
                {
                    auto NadeDef = FindItemDefByName(L"Athena_Grenade");
                    if (NadeDef)
                    {
                        Inv->GiveItem(NadeDef, 3);
                        bGaveGrenades = true;
                    }
                }

                static int packedReported = 0;
                if (++packedReported <= 6)
                {
                    std::wstring wh(HealName);
                    std::string nh(wh.begin(), wh.end());
                    LogLine("[LOADOUT] Bot's inventory: gun %s | ammo stacks %d of %d | heal '%s' %s | grenades %s",
                            WeaponDef ? "yes" : "NO", AmmoStacks, (int)(sizeof(Ammunition) / sizeof(Ammunition[0])),
                            nh.c_str(), HealDef ? "x3" : "NOT FOUND", bGaveGrenades ? "x3" : "none");
                }
            }

            static int reported = 0;
            if (++reported <= 6)
            {
                auto ClipIt = BotWeaponClipSize.find((void*)PS);
                LogLine("[LOADOUT] Bot given a %s weapon: %s | in their hands: %s | magazine: %d rounds | inventory: %s",
                        rarityName,
                        WeaponDef ? "found" : "ASSET NOT FOUND",
                        bEquipTookHold ? "yes" : "NO",
                        ClipIt != BotWeaponClipSize.end() ? ClipIt->second : 0,
                        Inv ? "created" : "NONE");
            }
        }
    }

    void RegisterBotsAsAlive()
    {
        auto World = UWorld::GetWorld();
        if (!World || !World->GameState || !World->AuthorityGameMode)
            return;

        auto GameState = (AFortGameStateAthena*)World->GameState;
        auto GameMode = (AFortGameMode*)World->AuthorityGameMode;
        if (!GameState->HasPlayerArray() || !GameMode->HasAlivePlayers())
            return;

        int newlyAdded = 0;

        for (int i = 0; i < GameState->PlayerArray.Num(); i++)
        {
            auto PlayerState = (AFortPlayerStateAthena*)GameState->PlayerArray[i];
            if (!PlayerState || !PlayerState->HasbIsABot() || !PlayerState->bIsABot || NPCs::IsNPC(PlayerState))
                continue;

            auto Controller = (AActor*)PlayerState->Owner;
            if (!Controller)
                continue;

            if (EliminatedBots.find((void*)Controller) != EliminatedBots.end())
                continue;

            bool alreadyIn = false;
            for (int j = 0; j < GameMode->AlivePlayers.Num(); j++)
            {
                if (GameMode->AlivePlayers[j] == Controller)
                {
                    alreadyIn = true;
                    break;
                }
            }
            if (!alreadyIn)
            {
                if (!bBusDeparted)
                {
                    GameMode->AlivePlayers.Add(Controller);
                    newlyAdded++;
                }
            }
        }

        if (!bBusDeparted && GameState->HasPlayersLeft())
        {
            int32 realCount = GameMode->AlivePlayers.Num();
            if (GameState->PlayersLeft != realCount)
            {
                GameState->PlayersLeft = realCount;

                GameState->OnRep_PlayersLeft();

                if (GameState->HasTeamsLeft())
                    GameState->TeamsLeft = realCount;
            }
        }

        if (newlyAdded > 0)
            if (!bBusDeparted)
                LogLine("[BotAI] Registered %d new bot(s); player count now %d.\n", newlyAdded, GameMode->AlivePlayers.Num());
    }

    void RenameUnnamedBots()
    {
        auto World = UWorld::GetWorld();
        if (!World || !World->GameState)
            return;

        auto GameState = (AFortGameStateAthena*)World->GameState;
        if (!GameState->HasPlayerArray())
            return;

        auto GameMode = (AFortGameMode*)World->AuthorityGameMode;
        if (!GameMode)
            return;

        auto& Players = GameState->PlayerArray;

        static const std::vector<std::string> SpecialNames = {
            "&darkBeast&",
            "90cranker7",
            "A Sweaty Dog",
            "AboveMule633",
            "AimLikeIdaho",
            "AngryDuck51",
            "AthenaOrApollo",
            "AtTheBeach321",
            "BagelBoy82",
            "BatTacos",
            "Beebitme",
            "BellyFlop40",
            "Blackjack31",
            "BliceCake",
            "BlinkImGone44",
            "BoatingIsLife",
            "BobDobaleena",
            "BoldPrediction",
            "BraddleRoyale80",
            "BrainInvader",
            "Brauny Banana",
            "CactusDad80",
            "ColonelChugJug",
            "ConstantThorn",
            "CrazyPea96",
            "CrepeSalad",
            "DoctorLobby92",
            "DoubleDaring",
            "DoubleDuel75",
            "DoubleRainbow96",
            "DrPlanet",
            "DrumGunnar",
            "ElPollo85",
            "Discord.gg/4e0h",
            "FlavorCaptain",
            "FlimsyGoat",
            "FlossPatrol82",
            "Gatordile81",
            "GetItGotItGood",
            "GhostChicken12",
            "GnomeRider16",
            "Gooddoggo80",
            "Goosezilla13",
            "Grandma40",
            "HashtagToad57",
            "HeliumHog",
            "HeyThereFriend81",
            "Hoodwinked12",
            "HotelBlankets",
            "HowAreMy90s",
            "iHazHighGround",
            "JonseyForever35",
            "JustABitEpic",
            "KittyCat80",
            "Kregore73",
            "LetsBePals23",
            "LewtGoblin7",
            "LootTrooper51",
            "LousyCentaur",
            "LuckyNumber4279",
            "McCucumber71",
            "Meshuncle",
            "MintElephant26",
            "Mouthful95",
            "N0nDa1ry",
            "NotAPalidome",
            "Number141",
            "OldWaterBottle28",
            "ParanoidCactus",
            "PortableOx",
            "PositiveFeels",
            "PrancingPwnee",
            "PrinceWombat",
            "PurpleCrayon85",
            "Mehmehmeh",
            "QueenBeet74",
            "SergentSummer",
            "ShadowArrow58",
            "Shepard52",
            "ShieldHorse63",
            "ShootyMcGee40",
            "Neo_loves_overtonight",
            "SilverySilver",
            "SirTricksALot21",
            "SoggyCookie26",
            "SpiffyPowder6866",
            "SteelGoose18",
            "SweetPenguin16",
            "TAgYOuRIt9",
            "TheFreezer21",
            "ThermalDragon39",
            "TimeToGo80",
            "TooManyBeets",
            "WalkInThePark66",
            "WildCactusBob",
            "Wondertail",
            "Yeetman57",
            "AtomicPossum",
            "BananaBandit77",
            "CosmicBadger",
            "DiscoLizard42",
            "ElectricMoose",
            "FancyRaccoon",
            "GiantPanda88",
            "HappyTurnip",
            "IronOstrich",
            "JellyfishKing",
            "KillerToast",
            "LazyRocket",
            "MightyMuffin",
            "NinjaSquid",
            "AthenaFortPlayerController",
            "OrangeLlama",
            "PricklyPear55",
            "QuickQuokka",
            "RobotChicken",
            "SneakyPigeon",
            "TinyTornado",
            "UltraWombat",
            "VelvetShark",
            "WaffleKnight",
            "XenonTurtle",
            "YellowYeti",
            "ZanyZebra",
            "AmberAxolotl",
            "BlueberryBat",
            "CrankyCrab",
            "DizzyDolphin",
            "EmberFox",
            "FrostyFrog",
            "Overtonight",
            "GlowingGoat",
            "HiddenHedgehog",
            "IvoryIbis",
            "JollyJaguar",
            "KoalaKrusader",
            "LunarLemur",
            "MossyMole",
            "NeonNewt",
            "OceanOtter",
            "PinkPenguin",
            "QuietQuail",
            "RedRaccoon",
            "SilverSloth",
            "TurboToucan",
            "UrbanUmbreon",
            "VioletVulture",
            "WanderingWolf",
            "XRayYak",
            "YawningYak",
            "ZippyZorua",
            "AcornArmadillo",
            "BouncyBeaver",
            "CheerfulCheetah",
            "DapperDingo",
            "ElegantEmu",
            "FuzzyFerret",
            "GentleGiraffe",
            "HumbleHippo",
            "IcyIguana",
            "JeffDaKiller",
            "JazzyJackal",
            "KindKangaroo",
            "LuckyLynx",
            "MerryMeerkat",
            "NobleNarwhal",
            "OddOcelot",
            "PlayfulPanda",
            "QuirkyQuoll",
            "RowdyRhino",
            "SleepySeal",
            "TameTapir",
            "UniqueUrial",
            "VigilantVicuna",
            "WiseWalrus",
            "ZestyZebu",
            "BraveBison",
            "CleverCamel",
            "DaringDeer",
            "EagerEagle",
            "CoffeeGuzzler38",
            "FearlessFalcon",
            "GallantGazelle",
            "HonestHawk",
            "InvincibleIbis",
            "JoyfulJay",
            "KeenKiwi",
            "LoyalLion",
            "MysticMoth",
            "NimbleNumbat",
            "OptimisticOwl",
            "PatientParrot",
            "RadiantRaven",
            "SwiftSparrow",
            "TrustyTiger",
            "UpbeatUrchin",
            "ValiantVulture",
            "WildWoodpecker",
            "XenialXerus",
            "YoungYak",
            "ZealousZebra",
            "SoggyWaffle",
            "CrispyBacon",
            "JefferyEinstein.uasset",
            "SpicyNoodle",
            "FrozenPizza",
            "MeltyCheese",
            "CrunchyTaco",
            "ButteryBiscuit",
            "SaltyPretzel",
            "SweetMuffin",
            "SourLemon",
            "TangyOrange",
            "BitterMelon",
            "SavorySteak",
            "JuicyBurger",
            "CreamySoup",
            "ToastyBread",
            "StickyRice",
            "FluffyPancake",
            "GummyBear",
            "ChocolateMoose",
            "VanillaYeti",
            "StrawberrySlime",
            "BlueberryBison",
            "RaspberryRhino",
            "BananaBoat",
            "CherryBomb",
            "GrapeApe",
            "MangoMonkey",
            "PeachPanda",
            "PearParrot",
            "PlumPuppy",
            "KiwiKnight",
            "LemonLlama",
            "LimeLizard",
            "MelonMan",
            "OliveOtter",
            "PapayaPirate",
            "GuavaGoblin",
            "FigFalcon",
            "DateDragon",
            "LarryFizzlepot 2.0",
            "ApricotAngel",
            "CoconutCrab",
            "PecanPilot",
            "WalnutWizard",
            "AlmondArmadillo",
            "CashewCoyote",
            "PistachioPixie",
            "HazelnutHawk",
            "MacadamiaMole",
            "PeanutPanda",
            "CoffeeCat",
            "TeaTiger",
            "SodaShark",
            "JuiceJaguar",
            "MilkMoose",
            "WaterWombat",
            "FireFerret",
            "EarthEmu",
            "WindWolf",
            "StormStag",
            "ThunderTurtle",
            "LightningLynx",
            "SolarSquid",
            "LunarLobster",
            "StarfishSteve",
            "GalaxyGoat",
            "NebulaNewt",
            "CometCrab",
            "MeteorMoth",
            "AsteroidApe",
            "PlanetPigeon",
            "OrbitOtter",
            "RocketRaccoon",
            "CometCoyote",
            "GravityGoose",
            "CosmoCat",
            "AstroAnt",
            "NovaNewt",
            "QuasarQuail",
            "PulsarPug",
            "ShadowSheep",
            "GhostGecko",
            "DonaldMustardPlsComeBack",
            "PhantomPanda",
            "SpiritSparrow",
            "HauntedHamster",
            "SpookySquid",
            "CreepyCrab",
            "MysteriousMole",
            "SecretSquirrel",
            "HiddenHawk",
            "SilentSnake",
            "QuietQuokka",
            "WhisperWolf",
            "MurmurMoth",
            "EchoEmu",
            "BreezeBadger",
            "FogFox",
            "MistMoose",
            "CloudCoyote",
            "RainRaccoon",
            "SunnySalamander",
            "CloudyCat",
            "WindyWeasel",
            "RainyRabbit",
            "SnowySeal",
            "StormySwan",
            "FoggyFerret",
            "MistyMouse",
            "DustyDingo",
            "SandySquirrel",
            "MuddyMule",
            "RockyRaccoon",
            "PebblePenguin",
            "BoulderBear",
            "CrystalCrab",
            "DiamondDove",
            "RubyRabbit",
            "EmeraldEel",
            "SapphireSnake",
            "TopazTiger",
            "BronzeBadger",
            "SilverShark",
            "GoldenGoose",
            "PlatinumPanda",
            "CopperCoyote",
            "IronIguana",
            "SteelSeal",
            "TitaniumTurtle",
            "ChromeChicken",
            "NickelNewt",
            "BrassBeaver",
            "CobaltCat",
            "ZincZebra",
            "LeadLemur",
            "MercuryMoth",
            "NeonNarwhal",
            "ArgonApe",
            "HeliumHedgehog",
            "OxygenOtter",
            "CarbonCrab",
            "PixelPanda",
            "VectorVulture",
            "MatrixMole",
            "BinaryBear",
            "CyberCat",
            "DigitalDog",
            "AnalogAnt",
            "LaserLizard",
            "PlasmaPig",
            "QuantumQuail",
            "NanoNewt",
            "MicroMoose",
            "SophieRain",
            "MegaMoth",
            "GigaGoat",
            "TerraTiger",
            "ByteBadger",
            "DataDingo",
            "CodeCoyote",
            "GlitchGoose",
            "BugBunny",
            "AlphaAlpaca",
            "BetaBadger",
            "GammaGoose",
            "DeltaDuck",
            "EpsilonEmu",
            "ZetaZebra",
            "EtaEagle",
            "ThetaTiger",
            "IotaIbis",
            "KappaKoala",
            "LambdaLion",
            "MuMoose",
            "NuNewt",
            "XiXerus",
            "OmicronOwl",
            "PiPanda",
            "RhoRabbit",
            "SigmaSnake",
            "TauTurtle",
            "UpsilonUrchin",
            "PhiFox",
            "ChiCheetah",
            "PsiPenguin",
            "OmegaOtter",
            "PrimePanda",
            "ZeroZebra",
            "OneOtter",
            "Four_E_Zero_H",
            "TwoTiger",
            "ThreeThrush",
            "FourFox",
            "FiveFalcon",
            "SixSnake",
            "SevenSeal",
            "EightEagle",
            "NineNewt",
            "TenTurtle",
            "ElevenEmu",
            "TwelveToucan",
            "LuckyLlama",
            "HappyHippo",
            "GrumpyGoat",
            "SleepySloth",
            "DopeyDog",
            "BashfulBear",
            "SneezySnake",
            "DocDuck",
            "HappyHawk",
            "FunnyFox",
            "SillySeal",
            "CrazyCrab",
            "WackyWombat",
            "GoofyGoose",
            "ClumsyCat",
            "BouncyBear",
            "JumpyJaguar",
            "SkippySquid",
            "HoppingHare",
            "LeapingLizard",
            "DashingDeer",
            "PrancingPony",
            "GallopingGoat",
            "RunningRabbit",
            "FlyingFish",
            "SwimmingSwan",
            "DivingDolphin",
            "ClimbingCat",
            "CrawlingCrab",
            "SlitheringSnake",
            "WaddlingWalrus",
            "MarchingMoose",
            "DancingDingo",
            "SingingSparrow",
            "HummingHawk",
            "WhistlingWolf",
            "LaughingLlama",
            "SmilingSheep",
            "WinkingWolf",
            "BlinkingBat",
            "StaringStag",
            "GazingGoat",
            "NinjaNarwhal",
            "PiratePanda",
            "VikingVulture",
            "KnightKiwi",
            "WizardWeasel",
            "MageMoth",
            "RogueRabbit",
            "RangerRaccoon",
            "ClericCat",
            "DruidDuck",
            "BardBadger",
            "MonkMole",
            "PaladinPug",
            "WarlockWolf",
            "SorcererSnake",
            "AlchemistAnt",
            "NecromancerNewt",
            "SummonerSquid",
            "ArcherArmadillo",
            "HunterHawk",
            "SamuraiSwan",
            "CowboyCoyote",
            "SheriffShark",
            "DeputyDog",
            "MarshalMoose",
            "OutlawOtter",
            "BanditBadger",
            "RustlerRabbit",
            "GunfighterGoose",
            "SniperSparrow",
            "ScoutSquirrel",
            "MedicMoth",
            "EngineerEmu",
            "SoldierSnake",
            "CaptainCrab",
            "MajorMole",
            "GeneralGoat",
            "AdmiralApe",
            "CommanderCat",
            "SergeantSeal",
            "DoctorDove",
            "ProfessorPanda",
            "TeacherTiger",
            "StudentSquid",
            "ScientistSnail",
            "InventorIbis",
            "ExplorerEel",
            "AdventurerAnt",
            "TravelerToucan",
            "WandererWolf",
            "DreamerDuck",
            "ThinkerThrush",
            "BuilderBear",
            "MakerMoose",
            "CrafterCrab",
            "PainterPenguin",
            "SingerSwan",
            "DancerDeer",
            "WriterWren",
            "PlayerPanda",
            "PrintHelloWorld",
        };
        
        
        static std::vector<std::string> ActiveNames;
        static size_t ActiveNamesUsed = 0;
        static bool bActiveNamesReady = false;

        if (!bActiveNamesReady && !SpecialNames.empty())
        {
            ActiveNames = SpecialNames;
            
            for (size_t i = ActiveNames.size(); i > 1; --i)
            {
                size_t j = (size_t)(rand() % (int)i);
                std::swap(ActiveNames[i - 1], ActiveNames[j]);
            }
            const size_t Keep = ActiveNames.size() < 99 ? ActiveNames.size() : 99;
            ActiveNames.resize(Keep);
            bActiveNamesReady = true;
            LogLine("[NAMES] Picked %zu random names out of %zu for this match.",
                    ActiveNames.size(), SpecialNames.size());
        }

        for (int i = 0; i < Players.Num(); i++)
        {
            auto PlayerState = (AFortPlayerStateAthena*)Players[i];
            if (!PlayerState || !PlayerState->HasbIsABot() || !PlayerState->bIsABot || NPCs::IsNPC(PlayerState))
                continue;

            auto Controller = (AFortPlayerControllerAthena*)PlayerState->Owner;
            if (!Controller)
                continue;

            if (AssignedBotNames.find((void*)Controller) != AssignedBotNames.end())
                continue;

            if (ActiveNames.empty())
                continue;

            std::string NewName;
            {
                size_t idx = ActiveNamesUsed % ActiveNames.size();
                size_t pass = ActiveNamesUsed / ActiveNames.size();
                ActiveNamesUsed++;

                NewName = ActiveNames[idx];
                if (pass > 0)
                    NewName += " " + std::to_string(pass + 1);
            }

            AssignedBotNames[(void*)Controller] = NewName;

            std::wstring WideName(NewName.begin(), NewName.end());

            GameMode->ChangeName(Controller, FString(WideName.c_str()), true);
            PlayerState->OnRep_PlayerName();
        }
    }

    void DressUndressedBots()
    {
        static const std::vector<std::wstring> DefaultSkins = {
            L"/Game/Athena/Items/Cosmetics/Characters/CID_922_Athena_Commando_M_ParcelPrank.CID_922_Athena_Commando_M_ParcelPrank",
            L"/Game/Athena/Items/Cosmetics/Characters/CID_921_Athena_Commando_F_ParcelPetal.CID_921_Athena_Commando_F_ParcelPetal",
            L"/Game/Athena/Items/Cosmetics/Characters/CID_957_Athena_Commando_F_Ponytail.CID_957_Athena_Commando_F_Ponytail",
            L"/Game/Athena/Items/Cosmetics/Characters/CID_925_Athena_Commando_F_TapDance.CID_925_Athena_Commando_F_TapDance",
            L"/Game/Athena/Items/Cosmetics/Characters/CID_891_Athena_Commando_M_LunchBox.CID_891_Athena_Commando_M_LunchBox",
            L"/Game/Athena/Items/Cosmetics/Characters/CID_886_Athena_Commando_M_ChOneRenegade.CID_886_Athena_Commando_M_ChOneRenegade",
            L"/Game/Athena/Items/Cosmetics/Characters/CID_757_Athena_Commando_F_WildCat.CID_757_Athena_Commando_F_WildCat",
            L"/Game/Athena/Items/Cosmetics/Characters/CID_902_Athena_Commando_M_PumpkinPunk.CID_902_Athena_Commando_M_PumpkinPunk",
            L"/Game/Athena/Items/Cosmetics/Characters/CID_903_Athena_Commando_F_Frankie.CID_903_Athena_Commando_F_Frankie",
            L"/Game/Athena/Items/Cosmetics/Characters/CID_919_Athena_Commando_F_BabaYaga.CID_919_Athena_Commando_F_BabaYaga",
            L"/Game/Athena/Items/Cosmetics/Characters/CID_913_Athena_Commando_F_York_D.CID_913_Athena_Commando_F_York_D",
            L"/Game/Athena/Items/Cosmetics/Characters/CID_914_Athena_Commando_F_York_E.CID_914_Athena_Commando_F_York_E",
            L"/Game/Athena/Items/Cosmetics/Characters/CID_917_Athena_Commando_M_DurrburgerSkull.CID_917_Athena_Commando_M_DurrburgerSkull",
            L"/Game/Athena/Items/Cosmetics/Characters/CID_920_Athena_Commando_M_PartyTrooper.CID_920_Athena_Commando_M_PartyTrooper",
            L"/Game/Athena/Items/Cosmetics/Characters/CID_897_Athena_Commando_F_DarkBomberSummer.CID_897_Athena_Commando_F_DarkBomberSummer",
            L"/Game/Athena/Items/Cosmetics/Characters/CID_893_Athena_Commando_F_BlackWidowJacket.CID_893_Athena_Commando_F_BlackWidowJacket",
            L"/Game/Athena/Items/Cosmetics/Characters/CID_892_Athena_Commando_F_VampireCasual.CID_892_Athena_Commando_F_VampireCasual",
            L"/Game/Athena/Items/Cosmetics/Characters/CID_899_Athena_Commando_F_Poison.CID_899_Athena_Commando_F_Poison",
            L"/Game/Athena/Items/Cosmetics/Characters/CID_895_Athena_Commando_M_DeliSandwich.CID_895_Athena_Commando_M_DeliSandwich",
            L"/Game/Athena/Items/Cosmetics/Characters/CID_828_Athena_Commando_F_Valet.CID_828_Athena_Commando_F_Valet",
            L"/Game/Athena/Items/Cosmetics/Characters/CID_829_Athena_Commando_M_Valet.CID_829_Athena_Commando_M_Valet",
            L"/Game/Athena/Items/Cosmetics/Characters/CID_896_Athena_Commando_F_SpookyNeon.CID_896_Athena_Commando_F_SpookyNeon",
            L"/Game/Athena/Items/Cosmetics/Characters/CID_869_Athena_Commando_F_Cavalry.CID_869_Athena_Commando_F_Cavalry",
            L"/Game/Athena/Items/Cosmetics/Characters/CID_894_Athena_Commando_M_Palespooky.CID_894_Athena_Commando_M_Palespooky",
            L"/Game/Athena/Items/Cosmetics/Characters/CID_871_Athena_Commando_F_StreetFashionGarnet.CID_871_Athena_Commando_F_StreetFashionGarnet",
            L"/Game/Athena/Items/Cosmetics/Characters/CID_825_Athena_Commando_F_SportsFashion.CID_825_Athena_Commando_F_SportsFashion",
            L"/Game/Athena/Items/Cosmetics/Characters/CID_868_Athena_Commando_M_Backspin_3U6CA.CID_868_Athena_Commando_M_Backspin_3U6CA",
            L"/Game/Athena/Items/Cosmetics/Characters/CID_852_Athena_Commando_F_BlackWidowCorrupt.CID_852_Athena_Commando_F_BlackWidowCorrupt",
            L"/Game/Athena/Items/Cosmetics/Characters/CID_853_Athena_Commando_F_SniperHoodCorrupt.CID_853_Athena_Commando_F_SniperHoodCorrupt",
            L"/Game/Athena/Items/Cosmetics/Characters/CID_827_Athena_Commando_M_MultibotStealth.CID_827_Athena_Commando_M_MultibotStealth",
            L"/Game/Athena/Items/Cosmetics/Characters/CID_679_Athena_Commando_M_ModernMilitaryEclipse.CID_679_Athena_Commando_M_ModernMilitaryEclipse",
            L"/Game/Athena/Items/Cosmetics/Characters/CID_806_Athena_Commando_F_GreenJacket.CID_806_Athena_Commando_F_GreenJacket",
            L"/Game/Athena/Items/Cosmetics/Characters/CID_544_Athena_Commando_M_Kurohomura.CID_544_Athena_Commando_M_Kurohomura",
            L"/Game/Athena/Items/Cosmetics/Characters/CID_604_Athena_Commando_F_Razor.CID_604_Athena_Commando_F_Razor",
            L"/Game/Athena/Items/Cosmetics/Characters/CID_763_Athena_Commando_F_ShinyJacket.CID_763_Athena_Commando_F_ShinyJacket",
            L"/Game/Athena/Items/Cosmetics/Characters/CID_864_Athena_Commando_F_Elastic_E.CID_864_Athena_Commando_F_Elastic_E",
            L"/Game/Athena/Items/Cosmetics/Characters/CID_840_Athena_Commando_M_HightowerGrape.CID_840_Athena_Commando_M_HightowerGrape",
            L"/Game/Athena/Items/Cosmetics/Characters/CID_841_Athena_Commando_M_HightowerWasabi.CID_841_Athena_Commando_M_HightowerWasabi",
            L"/Game/Athena/Items/Cosmetics/Characters/CID_842_Athena_Commando_F_HightowerHoneydew.CID_842_Athena_Commando_F_HightowerHoneydew",
            L"/Game/Athena/Items/Cosmetics/Characters/CID_843_Athena_Commando_M_HightowerTomato_Casual.CID_843_Athena_Commando_M_HightowerTomato_Casual",
            L"/Game/Athena/Items/Cosmetics/Characters/CID_844_Athena_Commando_F_HightowerMango.CID_844_Athena_Commando_F_HightowerMango",
            L"/Game/Athena/Items/Cosmetics/Characters/CID_845_Athena_Commando_M_HightowerTapas.CID_845_Athena_Commando_M_HightowerTapas",
            L"/Game/Athena/Items/Cosmetics/Characters/CID_846_Athena_Commando_M_HightowerDate.CID_846_Athena_Commando_M_HightowerDate",
            L"/Game/Athena/Items/Cosmetics/Characters/CID_824_Athena_Commando_F_RaiderPink.CID_824_Athena_Commando_F_RaiderPink",
            L"/Game/Athena/Items/Cosmetics/Characters/CID_800_Athena_Commando_M_Robro.CID_800_Athena_Commando_M_Robro",
            L"/Game/Athena/Items/Cosmetics/Characters/CID_822_Athena_Commando_F_Angler.CID_822_Athena_Commando_F_Angler",
            L"/Game/Athena/Items/Cosmetics/Characters/CID_823_Athena_Commando_F_Islander.CID_823_Athena_Commando_F_Islander",
            L"/Game/Athena/Items/Cosmetics/Characters/CID_836_Athena_Commando_M_JonesyFlare.CID_836_Athena_Commando_M_JonesyFlare",
            L"/Game/Athena/Items/Cosmetics/Characters/CID_833_Athena_Commando_F_TripleScoop.CID_833_Athena_Commando_F_TripleScoop",
            L"/Game/Athena/Items/Cosmetics/Characters/CID_832_Athena_Commando_F_AntiLlama.CID_832_Athena_Commando_F_AntiLlama",
            L"/Game/Athena/Items/Cosmetics/Characters/CID_826_Athena_Commando_M_FloatillaCaptain.CID_826_Athena_Commando_M_FloatillaCaptain",
            L"/Game/Athena/Items/Cosmetics/Characters/CID_834_Athena_Commando_M_Axl.CID_834_Athena_Commando_M_Axl",
            L"/Game/Athena/Items/Cosmetics/Characters/CID_798_Athena_Commando_M_JonesyVagabond.CID_798_Athena_Commando_M_JonesyVagabond",
            L"/Game/Athena/Items/Cosmetics/Characters/CID_815_Athena_Commando_F_DurrburgerHero.CID_815_Athena_Commando_F_DurrburgerHero",
            L"/Game/Athena/Items/Cosmetics/Characters/CID_795_Athena_Commando_M_Dummeez.CID_795_Athena_Commando_M_Dummeez",
            L"/Game/Athena/Items/Cosmetics/Characters/CID_796_Athena_Commando_F_Tank.CID_796_Athena_Commando_F_Tank",
            L"/Game/Athena/Items/Cosmetics/Characters/CID_816_Athena_Commando_F_DirtyDocks.CID_816_Athena_Commando_F_DirtyDocks",
            L"/Game/Athena/Items/Cosmetics/Characters/CID_817_Athena_Commando_M_DirtyDocks.CID_817_Athena_Commando_M_DirtyDocks",
            L"/Game/Athena/Items/Cosmetics/Characters/CID_814_Athena_Commando_M_BananaSummer.CID_814_Athena_Commando_M_BananaSummer",
            L"/Game/Athena/Items/Cosmetics/Characters/CID_810_Athena_Commando_M_MilitaryFashionSummer.CID_810_Athena_Commando_M_MilitaryFashionSummer",
            L"/Game/Athena/Items/Cosmetics/Characters/CID_775_Athena_Commando_F_StreetRacerCobraGold.CID_775_Athena_Commando_F_StreetRacerCobraGold",
            L"/Game/Athena/Items/Cosmetics/Characters/CID_813_Athena_Commando_M_TeriyakiAtlantis.CID_813_Athena_Commando_M_TeriyakiAtlantis",
            L"/Game/Athena/Items/Cosmetics/Characters/CID_801_Athena_Commando_F_GolfSummer.CID_801_Athena_Commando_F_GolfSummer",
            L"/Game/Athena/Items/Cosmetics/Characters/CID_802_Athena_Commando_F_HeartBreaker.CID_802_Athena_Commando_F_HeartBreaker",
            L"/Game/Athena/Items/Cosmetics/Characters/CID_797_Athena_Commando_F_Taco.CID_797_Athena_Commando_F_Taco",
            L"/Game/Athena/Items/Cosmetics/Characters/CID_799_Athena_Commando_F_CupidDark.CID_799_Athena_Commando_F_CupidDark",
            L"/Game/Athena/Items/Cosmetics/Characters/CID_786_Athena_Commando_F_CavalryBandit_Ghost.CID_786_Athena_Commando_F_CavalryBandit_Ghost",
            L"/Game/Athena/Items/Cosmetics/Characters/CID_787_Athena_Commando_M_Heist_Ghost.CID_787_Athena_Commando_M_Heist_Ghost",
            L"/Game/Athena/Items/Cosmetics/Characters/CID_803_Athena_Commando_F_SharkSuit.CID_803_Athena_Commando_F_SharkSuit",
            L"/Game/Athena/Items/Cosmetics/Characters/CID_808_Athena_Commando_F_ConstellationSun.CID_808_Athena_Commando_F_ConstellationSun",
            L"/Game/Athena/Items/Cosmetics/Characters/CID_805_Athena_Commando_F_PunkDevilSummer.CID_805_Athena_Commando_F_PunkDevilSummer",
            L"/Game/Athena/Items/Cosmetics/Characters/CID_847_Athena_Commando_M_Soy_2AS3C.CID_847_Athena_Commando_M_Soy_2AS3C",
            L"/Game/Athena/Items/Cosmetics/Characters/CID_784_Athena_Commando_F_RenegadeRaiderFire.CID_784_Athena_Commando_F_RenegadeRaiderFire",
            L"/Game/Athena/Items/Cosmetics/Characters/CID_749_Athena_Commando_F_GraffitiAssassin.CID_749_Athena_Commando_F_GraffitiAssassin",
            L"/Game/Athena/Items/Cosmetics/Characters/CID_764_Athena_Commando_F_Loofah.CID_764_Athena_Commando_F_Loofah",
            L"/Game/Athena/Items/Cosmetics/Characters/CID_778_Athena_Commando_M_Gator.CID_778_Athena_Commando_M_Gator",
            L"/Game/Athena/Items/Cosmetics/Characters/CID_785_Athena_Commando_F_Python.CID_785_Athena_Commando_F_Python",
            L"/Game/Athena/Items/Cosmetics/Characters/CID_767_Athena_Commando_F_BlackKnight.CID_767_Athena_Commando_F_BlackKnight",
            L"/Game/Athena/Items/Cosmetics/Characters/CID_776_Athena_Commando_M_ProfessorPup.CID_776_Athena_Commando_M_ProfessorPup",
            L"/Game/Athena/Items/Cosmetics/Characters/CID_717_Athena_Commando_F_BlueFlames.CID_717_Athena_Commando_F_BlueFlames",
            L"/Game/Athena/Items/Cosmetics/Characters/CID_771_Athena_Commando_F_OceanRider.CID_771_Athena_Commando_F_OceanRider",
            L"/Game/Athena/Items/Cosmetics/Characters/CID_770_Athena_Commando_F_MechanicalEngineer.CID_770_Athena_Commando_F_MechanicalEngineer",
            L"/Game/Athena/Items/Cosmetics/Characters/CID_747_Athena_Commando_M_BadEgg.CID_747_Athena_Commando_M_BadEgg",
            L"/Game/Athena/Items/Cosmetics/Characters/CID_751_Athena_Commando_F_NeonCatSpy.CID_751_Athena_Commando_F_NeonCatSpy",
            L"/Game/Athena/Items/Cosmetics/Characters/CID_726_Athena_Commando_M_TargetPractice.CID_726_Athena_Commando_M_TargetPractice",
            L"/Game/Athena/Items/Cosmetics/Characters/CID_758_Athena_Commando_M_TechExplorer.CID_758_Athena_Commando_M_TechExplorer",
            L"/Game/Athena/Items/Cosmetics/Characters/CID_759_Athena_Commando_F_RapVillainess.CID_759_Athena_Commando_F_RapVillainess",
            L"/Game/Athena/Items/Cosmetics/Characters/CID_752_Athena_Commando_M_Comet.CID_752_Athena_Commando_M_Comet",
            L"/Game/Athena/Items/Cosmetics/Characters/CID_753_Athena_Commando_F_Hostile.CID_753_Athena_Commando_F_Hostile",
            L"/Game/Athena/Items/Cosmetics/Characters/CID_703_Athena_Commando_M_Cyclone.CID_703_Athena_Commando_M_Cyclone",
            L"/Game/Athena/Items/Cosmetics/Characters/CID_761_Athena_Commando_M_CycloneSpace.CID_761_Athena_Commando_M_CycloneSpace",
            L"/Game/Athena/Items/Cosmetics/Characters/CID_662_Athena_Commando_M_FlameSkull.CID_662_Athena_Commando_M_FlameSkull",
            L"/Game/Athena/Items/Cosmetics/Characters/CID_748_Athena_Commando_F_Hitman.CID_748_Athena_Commando_F_Hitman",
            L"/Game/Athena/Items/Cosmetics/Characters/CID_736_Athena_Commando_F_DonutDish.CID_736_Athena_Commando_F_DonutDish",
            L"/Game/Athena/Items/Cosmetics/Characters/CID_724_Athena_Commando_M_SpyTech.CID_724_Athena_Commando_M_SpyTech",
            L"/Game/Athena/Items/Cosmetics/Characters/CID_744_Athena_Commando_F_DuckHero.CID_744_Athena_Commando_F_DuckHero",
            L"/Game/Athena/Items/Cosmetics/Characters/CID_741_Athena_Commando_F_HalloweenBunnySpring.CID_741_Athena_Commando_F_HalloweenBunnySpring",
            L"/Game/Athena/Items/Cosmetics/Characters/CID_740_Athena_Commando_F_CardboardCrew.CID_740_Athena_Commando_F_CardboardCrew",
            L"/Game/Athena/Items/Cosmetics/Characters/CID_728_Athena_Commando_M_MinotaurLuck.CID_728_Athena_Commando_M_MinotaurLuck",
            L"/Game/Athena/Items/Cosmetics/Characters/CID_722_Athena_Commando_M_TeriyakiFishAssassin.CID_722_Athena_Commando_M_TeriyakiFishAssassin",
            L"/Game/Athena/Items/Cosmetics/Characters/CID_725_Athena_Commando_F_AgentX.CID_725_Athena_Commando_F_AgentX",
            L"/Game/Athena/Items/Cosmetics/Characters/CID_715_Athena_Commando_F_TwinDark.CID_715_Athena_Commando_F_TwinDark",
            L"/Game/Athena/Items/Cosmetics/Characters/CID_720_Athena_Commando_F_StreetFashionEmerald.CID_720_Athena_Commando_F_StreetFashionEmerald",
            L"/Game/Athena/Items/Cosmetics/Characters/CID_714_Athena_Commando_M_AnarchyAcresFarmer.CID_714_Athena_Commando_M_AnarchyAcresFarmer",
            L"/Game/Athena/Items/Cosmetics/Characters/CID_674_Athena_Commando_F_HoodieBandit.CID_674_Athena_Commando_F_HoodieBandit",
            L"/Game/Athena/Items/Cosmetics/Characters/CID_697_Athena_Commando_F_GraffitiFuture.CID_697_Athena_Commando_F_GraffitiFuture",
            L"/Game/Athena/Items/Cosmetics/Characters/CID_688_Athena_Commando_F_AgentRogue.CID_688_Athena_Commando_F_AgentRogue",
            L"/Game/Athena/Items/Cosmetics/Characters/CID_695_Athena_Commando_F_DesertOpsCamo.CID_695_Athena_Commando_F_DesertOpsCamo",
            L"/Game/Athena/Items/Cosmetics/Characters/CID_691_Athena_Commando_F_TNTina.CID_691_Athena_Commando_F_TNTina",
            L"/Game/Athena/Items/Cosmetics/Characters/CID_690_Athena_Commando_F_Photographer.CID_690_Athena_Commando_F_Photographer",
            L"/Game/Athena/Items/Cosmetics/Characters/CID_699_Athena_Commando_F_BrokenHeart.CID_699_Athena_Commando_F_BrokenHeart",
            L"/Game/Athena/Items/Cosmetics/Characters/CID_704_Athena_Commando_F_LollipopTrickster.CID_704_Athena_Commando_F_LollipopTrickster",
            L"/Game/Athena/Items/Cosmetics/Characters/CID_625_Athena_Commando_F_PinkTrooper.CID_625_Athena_Commando_F_PinkTrooper",
            L"/Game/Athena/Items/Cosmetics/Characters/CID_623_Athena_Commando_M_Frogman.CID_623_Athena_Commando_M_Frogman",
            L"/Game/Athena/Items/Cosmetics/Characters/CID_683_Athena_Commando_F_TigerFashion.CID_683_Athena_Commando_F_TigerFashion",
            L"/Game/Athena/Items/Cosmetics/Characters/CID_670_Athena_Commando_F_ArcticCamo.CID_670_Athena_Commando_F_ArcticCamo",
            L"/Game/Athena/Items/Cosmetics/Characters/CID_673_Athena_Commando_F_ArcticCamo_Slate.CID_673_Athena_Commando_F_ArcticCamo_Slate",
            L"/Game/Athena/Items/Cosmetics/Characters/CID_605_Athena_Commando_M_TourBus.CID_605_Athena_Commando_M_TourBus",
            L"/Game/Athena/Items/Cosmetics/Characters/CID_577_Athena_Commando_F_CODSquadPlaid.CID_577_Athena_Commando_F_CODSquadPlaid",
            L"/Game/Athena/Items/Cosmetics/Characters/CID_648_Athena_Commando_F_MsAlpine.CID_648_Athena_Commando_F_MsAlpine",
            L"/Game/Athena/Items/Cosmetics/Characters/CID_650_Athena_Commando_F_HolidayPJ_B.CID_650_Athena_Commando_F_HolidayPJ_B",
            L"/Game/Athena/Items/Cosmetics/Characters/CID_634_Athena_Commando_F_GalileoRocket_ARVEH.CID_634_Athena_Commando_F_GalileoRocket_ARVEH",
            L"/Game/Athena/Items/Cosmetics/Characters/CID_616_Athena_Commando_F_CavalryBandit.CID_616_Athena_Commando_F_CavalryBandit",
            L"/Game/Athena/Items/Cosmetics/Characters/CID_617_Athena_Commando_F_ForestQueen.CID_617_Athena_Commando_F_ForestQueen",
            L"/Game/Athena/Items/Cosmetics/Characters/CID_614_Athena_Commando_M_MissingLink.CID_614_Athena_Commando_M_MissingLink",
            L"/Game/Athena/Items/Cosmetics/Characters/CID_510_Athena_Commando_F_AngelEclipse.CID_510_Athena_Commando_F_AngelEclipse",
            L"/Game/Athena/Items/Cosmetics/Characters/CID_574_Athena_Commando_F_CubeRockerPunk.CID_574_Athena_Commando_F_CubeRockerPunk",
            L"/Game/Athena/Items/Cosmetics/Characters/CID_586_Athena_Commando_F_PunkDevil.CID_586_Athena_Commando_F_PunkDevil",
            L"/Game/Athena/Items/Cosmetics/Characters/CID_575_Athena_Commando_F_BulletBlue.CID_575_Athena_Commando_F_BulletBlue",
            L"/Game/Athena/Items/Cosmetics/Characters/CID_593_Athena_Commando_F_SoccerZombieA.CID_593_Athena_Commando_F_SoccerZombieA",
            L"/Game/Athena/Items/Cosmetics/Characters/CID_598_Athena_Commando_M_Mastermind.CID_598_Athena_Commando_M_Mastermind",
            L"/Game/Athena/Items/Cosmetics/Characters/CID_583_Athena_Commando_F_NoshHunter.CID_583_Athena_Commando_F_NoshHunter",
            L"/Game/Athena/Items/Cosmetics/Characters/CID_578_Athena_Commando_F_Fisherman.CID_578_Athena_Commando_F_Fisherman",
            L"/Game/Athena/Items/Cosmetics/Characters/CID_571_Athena_Commando_F_Sheath.CID_571_Athena_Commando_F_Sheath",
            L"/Game/Athena/Items/Cosmetics/Characters/CID_546_Athena_Commando_F_TacticalRed.CID_546_Athena_Commando_F_TacticalRed",
            L"/Game/Athena/Items/Cosmetics/Characters/CID_545_Athena_Commando_F_SushiNinja.CID_545_Athena_Commando_F_SushiNinja",
            L"/Game/Athena/Items/Cosmetics/Characters/CID_523_Athena_Commando_F_Cupid.CID_523_Athena_Commando_F_Cupid",
            L"/Game/Athena/Items/Cosmetics/Characters/CID_479_Athena_Commando_F_Davinci.CID_479_Athena_Commando_F_Davinci",
            L"/Game/Athena/Items/Cosmetics/Characters/CID_482_Athena_Commando_F_PizzaPit.CID_482_Athena_Commando_F_PizzaPit",
            L"/Game/Athena/Items/Cosmetics/Characters/CID_530_Athena_Commando_F_BlackMonday_1BV6J.CID_530_Athena_Commando_F_BlackMonday_1BV6J",
            L"/Game/Athena/Items/Cosmetics/Characters/CID_524_Athena_Commando_F_FutureBikerWhite.CID_524_Athena_Commando_F_FutureBikerWhite",
            L"/Game/Athena/Items/Cosmetics/Characters/CID_532_Athena_Commando_F_Punchy.CID_532_Athena_Commando_F_Punchy",
            L"/Game/Athena/Items/Cosmetics/Characters/CID_511_Athena_Commando_M_CubePaintWildCard.CID_511_Athena_Commando_M_CubePaintWildCard",
            L"/Game/Athena/Items/Cosmetics/Characters/CID_519_Athena_Commando_M_RaptorBlackOps.CID_519_Athena_Commando_M_RaptorBlackOps",
            L"/Game/Athena/Items/Cosmetics/Characters/CID_514_Athena_Commando_F_ToxicKitty.CID_514_Athena_Commando_F_ToxicKitty",
            L"/Game/Athena/Items/Cosmetics/Characters/CID_503_Athena_Commando_F_TacticalWoodlandFuture.CID_503_Athena_Commando_F_TacticalWoodlandFuture",
            L"/Game/Athena/Items/Cosmetics/Characters/CID_480_Athena_Commando_F_Bubblegum.CID_480_Athena_Commando_F_Bubblegum",
            L"/Game/Athena/Items/Cosmetics/Characters/CID_497_Athena_Commando_F_WildWest.CID_497_Athena_Commando_F_WildWest",
            L"/Game/Athena/Items/Cosmetics/Characters/CID_483_Athena_Commando_F_GraffitiRemix.CID_483_Athena_Commando_F_GraffitiRemix",
            L"/Game/Athena/Items/Cosmetics/Characters/CID_405_Athena_Commando_F_Masako.CID_405_Athena_Commando_F_Masako",
            L"/Game/Athena/Items/Cosmetics/Characters/CID_028_Athena_Commando_F.CID_028_Athena_Commando_F",
            L"/Game/Athena/Items/Cosmetics/Characters/CID_017_Athena_Commando_M.CID_017_Athena_Commando_M",
            L"/Game/Athena/Items/Cosmetics/Characters/CID_029_Athena_Commando_F_Halloween.CID_029_Athena_Commando_F_Halloween",
            L"/Game/Athena/Items/Cosmetics/Characters/CID_022_Athena_Commando_F.CID_022_Athena_Commando_F",
            L"/Game/Athena/Items/Cosmetics/Characters/CID_030_Athena_Commando_M_Halloween.CID_030_Athena_Commando_M_Halloween",
            L"/Game/Athena/Items/Cosmetics/Characters/CID_027_Athena_Commando_F.CID_027_Athena_Commando_F",
            L"/Game/Athena/Items/Cosmetics/Characters/CID_024_Athena_Commando_F.CID_024_Athena_Commando_F",
            L"/Game/Athena/Items/Cosmetics/Characters/CID_226_Athena_Commando_F_Octoberfest.CID_226_Athena_Commando_F_Octoberfest",
            L"/Game/Athena/Items/Cosmetics/Characters/CID_278_Athena_Commando_M_Yeti.CID_278_Athena_Commando_M_Yeti",
            L"/Game/Athena/Items/Cosmetics/Characters/CID_309_Athena_Commando_M_StreetGoth.CID_309_Athena_Commando_M_StreetGoth",
            L"/Game/Athena/Items/Cosmetics/Characters/CID_333_Athena_Commando_M_Squishy.CID_333_Athena_Commando_M_Squishy",
        };

        auto World = UWorld::GetWorld();
        if (!World || !World->GameState)
            return;

        auto GameState = (AFortGameStateAthena*)World->GameState;
        if (!GameState->HasPlayerArray())
            return;

        for (int i = 0; i < GameState->PlayerArray.Num(); i++)
        {
            auto PlayerState = (AFortPlayerStateAthena*)GameState->PlayerArray[i];
            if (!PlayerState || !PlayerState->HasbIsABot() || !PlayerState->bIsABot || NPCs::IsNPC(PlayerState))
                continue;

            if (!PlayerState->HasCharacterData())
                continue;

            auto* CharData = &PlayerState->CharacterData;
            if (!CharData)
                continue;

            auto** Parts = (UCustomCharacterPart**)((uint8_t*)CharData + 0x08);

            if (Parts[0] || Parts[1])
                continue;

            auto CID = FindObject<UAthenaCharacterItemDefinition>(DefaultSkins[rand() % DefaultSkins.size()].c_str());
            if (!CID)
            {
                static bool bReported = false;
                if (!bReported) { bReported = true; LogLine("[BotAI] Default CID asset NOT FOUND -- bots will be invisible\n"); }
                continue;
            }

            auto HeroType = CID->HeroDefinition;
            if (!HeroType || !HeroType->HasSpecializations() || HeroType->Specializations.Num() == 0)
                continue;

            auto SpecSoftPtr = HeroType->Specializations[0];
            auto Spec = (UFortHeroSpecialization*)SpecSoftPtr.Get();
            if (!Spec)
                continue;

            for (int j = 0; j < Spec->CharacterParts.Num() && j < 6; j++)
            {
                auto PartSoftPtr = Spec->CharacterParts[j];
                auto Part = (UCustomCharacterPart*)PartSoftPtr.Get();
                if (Part && (int)Part->CharacterPartType < 6)
                {
                    int slot = (int)Part->CharacterPartType;
                    Parts[slot] = Part;
                }
            }

            static UObject* KismetLibCDO = nullptr;
            static bool bKismetLooked = false;
            if (!bKismetLooked)
            {
                bKismetLooked = true;
                KismetLibCDO = (UObject*)FindObject<UObject>(L"/Script/FortniteGame.Default__FortKismetLibrary");
                LogLine("[BotAI] FortKismetLibrary CDO: %s\n", KismetLibCDO ? "found" : "not found");
            }
            if (KismetLibCDO)
            {
                static auto UpdateVisFn = KismetLibCDO->GetFunction("UpdatePlayerCustomCharacterPartsVisualization");
                if (UpdateVisFn)
                    KismetLibCDO->Call<void>(UpdateVisFn, PlayerState);
            }
            PlayerState->OnRep_CharacterData();

            static int dressed = 0;
            if (++dressed <= 3)
                LogLine("[BotAI] Dressed bot #%d via CharacterData path.\n", dressed);
        }
    }

    static std::map<void*, FVector> BotWanderGoals;

    
    static std::map<void*, FVector> BotAimError;
    static std::map<void*, int> BotAimErrorTick;

    static int GZoneOrdersIssued = 0;
    static int GZoneBotsInStorm = 0;
    static int GZoneBotsClosing = 0;
    static int GZoneBotsStuck = 0;

    static int GWanderUnstuck = 0;

    static void TopUpBotMagazine(void* PlayerStateKey)
    {
        if (!FConfiguration::bBotsNeverRunOutOfAmmo)
            return;

        auto InvIt = BotInventories.find(PlayerStateKey);
        if (InvIt == BotInventories.end() || !InvIt->second)
            return;

        auto WeaponIt = BotWeapons.find(PlayerStateKey);
        if (WeaponIt == BotWeapons.end() || !WeaponIt->second)
            return;

        auto ClipIt = BotWeaponClipSize.find(PlayerStateKey);

        int ClipSize = (ClipIt != BotWeaponClipSize.end() && ClipIt->second > 0) ? ClipIt->second : 30;

        auto Inventory = InvIt->second;
        if (!Inventory->HasInventory())
            return;

        for (int i = 0; i < Inventory->Inventory.ItemInstances.Num(); i++)
        {
            auto Item = Inventory->Inventory.ItemInstances[i];
            if (!Item || !Item->HasItemEntry())
                continue;

            auto& Entry = Item->ItemEntry;

            if (!FFortItemEntry::HasItemDefinition() || Entry.ItemDefinition != WeaponIt->second)
                continue;

            if (!FFortItemEntry::HasLoadedAmmo())
                return;

            int RefillBelow = ClipSize / 3;
            if (RefillBelow < 1)
                RefillBelow = 1;

            if (Entry.LoadedAmmo >= RefillBelow)
                return;

            Entry.LoadedAmmo = ClipSize;
            Inventory->Update(&Entry);
            GMagazinesRefilled++;
            return;
        }
    }

    static bool BotIsHoldingAWeapon(AFortPlayerPawnAthena* Pawn)
    {
        if (!Pawn || !Pawn->HasCurrentWeapon())
            return false;

        return Pawn->CurrentWeapon != nullptr;
    }

    static int32* WeaponAmmoCount(AActor* Weapon)
    {
        if (!Weapon)
            return nullptr;

        static int32 Offset = -2;

        if (Offset == -2)
        {
            Offset = (int32)Weapon->GetOffset("AmmoCount");
            LogLine("[RELOAD] The weapon's live round counter (AmmoCount) %s.",
                    Offset >= 0 ? "was found -- bots will reload" : "is NOT on this build's weapon class");
        }

        if (Offset < 0)
            return nullptr;

        return &GetFromOffset<int32>(Weapon, (uint32)Offset);
    }

    static void SetWeaponReloadingFlag(AActor* Weapon, bool bReloading)
    {
        if (!Weapon)
            return;

        static int32 Offset = -2;
        static uint8 Mask = 0;

        if (Offset == -2)
        {
            auto Prop = Weapon->GetProperty("bIsReloading", 0x20000);
            Offset = Prop ? (int32)GetFromOffset<uint32>(Prop, Offsets::Offset_Internal) : -1;
            Mask = Prop ? Prop->GetFieldMask() : 0;
        }

        if (Offset < 0 || Mask == 0)
            return;

        uint8& Byte = GetFromOffset<uint8>(Weapon, (uint32)Offset);
        Byte = bReloading ? (uint8)(Byte | Mask) : (uint8)(Byte & ~Mask);
    }

    static void FinishBotReload(AFortPlayerPawnAthena* Pawn, void* PlayerStateKey)
    {
        auto ClipIt = BotWeaponClipSize.find(PlayerStateKey);
        int ClipSize = (ClipIt != BotWeaponClipSize.end() && ClipIt->second > 0) ? ClipIt->second : 30;

        if (ClipSize < 4)
            ClipSize = 4;

        if (Pawn && Pawn->HasCurrentWeapon() && Pawn->CurrentWeapon)
        {
            auto Weapon = (AActor*)Pawn->CurrentWeapon;

            if (auto Ammo = WeaponAmmoCount(Weapon))
                *Ammo = ClipSize;

            SetWeaponReloadingFlag(Weapon, false);
        }

        TopUpBotMagazine(PlayerStateKey);
        GMagazinesRefilled++;
    }

    static bool BotWeaponIsEmpty(AFortPlayerPawnAthena* Pawn)
    {
        if (!Pawn || !Pawn->HasCurrentWeapon() || !Pawn->CurrentWeapon)
            return false;

        auto Ammo = WeaponAmmoCount((AActor*)Pawn->CurrentWeapon);

        return Ammo && *Ammo <= 0;
    }

    static int BotMagazineSize(void* PlayerStateKey)
    {
        auto ClipIt = BotWeaponClipSize.find(PlayerStateKey);
        int ClipSize = (ClipIt != BotWeaponClipSize.end() && ClipIt->second > 0) ? ClipIt->second : 30;

        if (ClipSize < 4)
            ClipSize = 4;

        return ClipSize;
    }

    static bool GAmmoCounterWorks = false;

    static bool BotNeedsToReload(AFortPlayerPawnAthena* Pawn, void* PlayerStateKey, FBotFireState& Fire)
    {
        int32* Ammo = nullptr;

        if (Pawn && Pawn->HasCurrentWeapon() && Pawn->CurrentWeapon)
            Ammo = WeaponAmmoCount((AActor*)Pawn->CurrentWeapon);

        if (Ammo)
        {
            if (!GAmmoCounterWorks && Fire.LastSeenAmmo >= 0 && *Ammo < Fire.LastSeenAmmo)
            {
                GAmmoCounterWorks = true;
                LogLine("[RELOAD] The weapon's round counter is live -- bots reload the moment the magazine runs out.");
            }

            Fire.LastSeenAmmo = *Ammo;

            if (GAmmoCounterWorks)
                return *Ammo <= 0;

            if (*Ammo <= 0)
                return true;
        }

        return Fire.RoundsSinceReload >= BotMagazineSize(PlayerStateKey);
    }

    
    
    
    
    
    
    

    class BAIPerceptionComponent : public UObject
    {
    public:
        UCLASS_COMMON_MEMBERS(BAIPerceptionComponent);

        DEFINE_FUNC(SetSenseEnabled, void);
        DEFINE_FUNC(ForgetAll, void);
    };

    static std::set<void*> BotSensesSilenced;
    static int GSensesSilencedBots = 0;
    static bool GSensesFaulted = false;

    static void SilenceSensesRaw(UObject* Perception, UClass** Classes, int Count)
    {
        auto Senses = (BAIPerceptionComponent*)Perception;

        for (int s = 0; s < Count; s++)
        {
            if (!Classes[s])
                continue;

            UClass* SenseClass = Classes[s];
            bool bEnable = false;
            Senses->SetSenseEnabled(SenseClass, bEnable);
        }

        Senses->ForgetAll();
    }

    
    
    static bool SilenceSensesGuarded(UObject* Perception, UClass** Classes, int Count)
    {
        __try
        {
            SilenceSensesRaw(Perception, Classes, Count);
            return true;
        }
        __except (EXCEPTION_EXECUTE_HANDLER)
        {
            GSensesFaulted = true;
            return false;
        }
    }

    static void SilenceNativeSenses(::AAIController* AIController)
    {
        if (!AIController || GSensesFaulted || BotSensesSilenced.find((void*)AIController) != BotSensesSilenced.end())
            return;

        BotSensesSilenced.insert((void*)AIController);

        static int32 PerceptionOffset = -2;
        static int32 BlueprintPerceptionOffset = -2;

        if (PerceptionOffset == -2)
        {
            PerceptionOffset = (int32)AIController->GetOffset("PerceptionComponent");
            BlueprintPerceptionOffset = (int32)AIController->GetOffset("AIPerception");
        }

        UObject* Perception = nullptr;

        if (PerceptionOffset >= 0)
            Perception = GetFromOffset<UObject*>(AIController, (uint32)PerceptionOffset);

        if (!Perception && BlueprintPerceptionOffset >= 0)
            Perception = GetFromOffset<UObject*>(AIController, (uint32)BlueprintPerceptionOffset);

        if (!Perception)
        {
            static bool bSaid = false;

            if (!bSaid)
            {
                bSaid = true;
                LogLine("[SENSES] This bot controller has no built-in senses to switch off -- nothing to do.");
            }

            return;
        }

        static const char* SenseNames[] = { "AISense_Sight", "AISense_Hearing", "AthenaAISense_Hearing", "AISense_Damage",
                                            "AISense_Team", "AISense_Touch", "AISense_Prediction", "AISense_Blueprint" };
        static const int SenseCount = (int)(sizeof(SenseNames) / sizeof(SenseNames[0]));
        static UClass* SenseClasses[8] = {};
        static int SensesFound = 0;
        static bool bLookedUp = false;

        if (!bLookedUp)
        {
            bLookedUp = true;

            for (int s = 0; s < SenseCount && s < 8; s++)
            {
                SenseClasses[s] = const_cast<UClass*>(FindClass(SenseNames[s]));

                if (SenseClasses[s])
                    SensesFound++;
            }
        }

        if (!SilenceSensesGuarded(Perception, SenseClasses, SenseCount < 8 ? SenseCount : 8))
        {
            LogLine("[SENSES] Switching off the game's own built-in bot senses faulted on this build -- they are left alone "
                    "(the bot code still picks its own targets; the aim focus still keeps its bursts on them).");
            return;
        }

        GSensesSilencedBots++;

        if (GSensesSilencedBots == 1)
            LogLine("[SENSES] Switched off the game's own built-in bot senses (%d kinds found: sight, hearing, being shot...). "
                    "They fed Epic's own target picker, which kept turning bots to aim at whoever it picked -- only the "
                    "bot code's lock-on picks targets now.", SensesFound);
    }

    
    
    
    
    
    
    static void WatchBotTrigger(AFortPlayerPawnAthena* Pawn, AFortPlayerStateAthena* PlayerState, int TickNow)
    {
        if (!Pawn || !Pawn->HasCurrentWeapon() || !Pawn->CurrentWeapon)
            return;

        auto Weapon = (AActor*)Pawn->CurrentWeapon;

        int32* Ammo = WeaponAmmoCount(Weapon);
        if (!Ammo)
            return;

        const int Rounds = *Ammo;

        auto Seen = BotAmmoSeen.find((void*)Pawn);

        if (Seen != BotAmmoSeen.end() && Seen->second.first == (void*)Weapon && Rounds < Seen->second.second)
        {
            auto FireIt = BotFire.find((void*)Pawn);
            const bool bBursting = FireIt != BotFire.end() && FireIt->second.bFiring;

            auto LetGo = BotTriggerUpTick.find((void*)Pawn);
            const bool bJustLetGo = LetGo != BotTriggerUpTick.end() && (TickNow - LetGo->second) <= 15;

            if (bBursting || bJustLetGo)
            {
                GShotTicksOurs++;
            }
            else
            {
                GShotTicksStray++;

                Pawn->PawnStopFire(0);
                BotTriggerUpTick[(void*)Pawn] = TickNow;

                if (GShotTicksStray <= 10)
                    LogLine("[TRIGGER] '%s' fired its gun without the bot code pulling the trigger -- let go of it.",
                            SafeNameOf(PlayerState).c_str());
            }
        }

        BotAmmoSeen[(void*)Pawn] = { (void*)Weapon, Rounds };
    }

    
    
    
    
    
    
    
    
    
    static bool GFocusFaulted = false;
    static bool GAimReadFaulted = false;

    
    static void BotFocusRaw(::AAIController* AIController, int Mode, AActor* Focus, const FVector* Point)
    {
        if (Mode == 1)
        {
            AActor* NewFocus = Focus;
            AIController->K2_SetFocus(NewFocus);
        }
        else if (Mode == 2)
        {
            FVector FocalPoint = *Point;
            AIController->K2_SetFocalPoint(FocalPoint);
        }
        else
        {
            AIController->K2_ClearFocus();
        }
    }

    
    
    static void BotFocusGuarded(::AAIController* AIController, int Mode, AActor* Focus, const FVector* Point)
    {
        __try
        {
            BotFocusRaw(AIController, Mode, Focus, Point);
        }
        __except (EXCEPTION_EXECUTE_HANDLER)
        {
            GFocusFaulted = true;
        }
    }

    static bool BotReadAimRaw(::AAIController* AIController, double* OutYaw)
    {
        const FRotator Actual = AIController->GetControlRotation();
        *OutYaw = (double)Actual.Yaw;
        return true;
    }

    static bool BotReadAimGuarded(::AAIController* AIController, double* OutYaw)
    {
        __try
        {
            return BotReadAimRaw(AIController, OutYaw);
        }
        __except (EXCEPTION_EXECUTE_HANDLER)
        {
            GAimReadFaulted = true;
            return false;
        }
    }

    
    static bool GStopFaulted = false;

    static void BotStopWalkingRaw(::AAIController* AIController)
    {
        AIController->StopMovement();
    }

    static void BotStopWalkingGuarded(::AAIController* AIController)
    {
        __try
        {
            BotStopWalkingRaw(AIController);
        }
        __except (EXCEPTION_EXECUTE_HANDLER)
        {
            GStopFaulted = true;
        }
    }

    static void BotAimAt(::AAIController* AIController, AFortPlayerPawnAthena* OnTarget, const FVector& From,
                         double AimYaw, double AimPitch, double Distance)
    {
        if (!AIController || GFocusFaulted)
            return;

        static int Available = -1;

        if (Available == -1)
        {
            Available = (AIController->GetFunction("K2_SetFocus") && AIController->GetFunction("K2_SetFocalPoint") &&
                         AIController->GetFunction("K2_ClearFocus")) ? 1 : 0;

            LogLine("[AIM] The engine's aim focus %s.",
                    Available ? "is available -- a bot's whole burst goes where the bot code aims it"
                              : "is NOT on this build -- only the first bullet of a burst follows the bot code's aim");
        }

        if (Available != 1)
            return;

        if (OnTarget)
        {
            BotFocusGuarded(AIController, 1, (AActor*)OnTarget, nullptr);
        }
        else
        {
            const double R = 3.14159265358979323846 / 180.0;

            FVector Point{};
            Point.X = From.X + cos(AimPitch * R) * cos(AimYaw * R) * Distance;
            Point.Y = From.Y + cos(AimPitch * R) * sin(AimYaw * R) * Distance;
            Point.Z = From.Z + sin(AimPitch * R) * Distance;

            BotFocusGuarded(AIController, 2, nullptr, &Point);
        }

        if (GFocusFaulted)
        {
            LogLine("[AIM] The engine's aim focus faulted on this build -- switched off; only the first bullet of a burst "
                    "follows the bot code's aim from here on.");
            return;
        }

        BotHasFocus.insert((void*)AIController);
    }

    static void BotStopAiming(::AAIController* AIController)
    {
        if (!AIController)
            return;

        auto It = BotHasFocus.find((void*)AIController);
        if (It == BotHasFocus.end())
            return;

        BotHasFocus.erase(It);

        if (!GFocusFaulted)
            BotFocusGuarded(AIController, 0, nullptr, nullptr);
    }

    
    
    
    
    
    

    class BKismetSystemLibrary : public UObject
    {
    public:
        UCLASS_COMMON_MEMBERS(BKismetSystemLibrary);

        DEFINE_STATIC_FUNC(LineTraceSingle, bool);
    };

    struct FBotHit
    {
        bool bHit = false;
        AActor* Actor = nullptr;
    };

    static bool GTraceFaulted = false;

    static bool BotTraceAvailable()
    {
        static int Available = -1;

        if (Available == -1)
        {
            auto Library = BKismetSystemLibrary::GetDefaultObj();
            Available = (Library && Library->GetFunction("LineTraceSingle")) ? 1 : 0;

            LogLine("[EYES] The engine's line trace %s.",
                    Available ? "is available -- bots also check the ground is not in the way before they shoot"
                              : "is NOT on this build -- only the game's line-of-sight check decides what a bot can see");
        }

        return Available == 1 && !GTraceFaulted;
    }

    static bool BotTraceRaw(AActor* Self, const FVector& Start, const FVector& End, uint8 Channel, uint8* HitBuffer)
    {
        struct FColour
        {
            float R, G, B, A;
        };

        UObject* Context = (UObject*)Self;
        TArray<AActor*> Ignore{};
        bool bComplex = false;
        uint8 DrawDebug = 0;        
        void* HitOut = (void*)HitBuffer;
        bool bIgnoreSelf = true;
        FColour TraceColour{ 1.f, 0.f, 0.f, 1.f };
        FColour HitColour{ 0.f, 1.f, 0.f, 1.f };
        float DrawTime = 0.f;

        return BKismetSystemLibrary::LineTraceSingle(Context, Start, End, Channel, bComplex, Ignore, DrawDebug,
                                                     HitOut, bIgnoreSelf, TraceColour, HitColour, DrawTime);
    }

    
    
    static bool BotTraceGuarded(AActor* Self, const FVector* Start, const FVector* End, uint8 Channel, uint8* HitBuffer)
    {
        __try
        {
            return BotTraceRaw(Self, *Start, *End, Channel, HitBuffer);
        }
        __except (EXCEPTION_EXECUTE_HANDLER)
        {
            GTraceFaulted = true;
            return false;
        }
    }

    static FBotHit BotTrace(AActor* Self, const FVector& Start, const FVector& End, uint8 Channel)
    {
        FBotHit Out{};

        if (!Self || !BotTraceAvailable())
            return Out;

        static int32 ActorOffset = -2;

        if (ActorOffset == -2)
        {
            auto HitStruct = FindStruct("HitResult");
            ActorOffset = HitStruct ? (int32)HitStruct->GetOffset("Actor") : -1;

            LogLine("[EYES] Line trace hit results: what was hit %s.", ActorOffset >= 0 ? "is readable" : "is NOT readable");
        }

        alignas(16) uint8 HitBuffer[0x400];
        memset(HitBuffer, 0, sizeof(HitBuffer));

        Out.bHit = BotTraceGuarded(Self, &Start, &End, Channel, HitBuffer);

        if (GTraceFaulted)
        {
            LogLine("[EYES] The engine's line trace faulted on this build -- switched off; the game's line-of-sight check "
                    "alone decides what a bot can see from here on.");
            return FBotHit{};
        }

        if (Out.bHit && ActorOffset >= 0 && ActorOffset < 0x3F0)
            Out.Actor = (AActor*)((FWeakObjectPtr*)(HitBuffer + ActorOffset))->Get();

        return Out;
    }

    static std::string BotObjectName(const UObject* Object)
    {
        if (!Object)
            return std::string("nothing");

        auto Name = Object->Name.ToString();
        return std::string(Name.c_str());
    }

    static std::string BotClassName(const UObject* Object)
    {
        if (!Object || !Object->Class)
            return std::string("?");

        auto Name = Object->Class->Name.ToString();
        return std::string(Name.c_str());
    }

    static bool BotGroundInTheWay(AFortPlayerPawnAthena* Me, const FVector& MyLoc, AFortPlayerPawnAthena* Them,
                                  const FVector& ThemLoc, std::string* WhatWasHit = nullptr)
    {
        if (!Me || !Them || !BotTraceAvailable())
            return false;

        FVector Eyes = MyLoc;
        Eyes.Z = MyLoc.Z + 60.0;

        FVector Chest = ThemLoc;
        Chest.Z = ThemLoc.Z + 30.0;

        
        const uint8 Channels[2] = { 1, 0 };

        for (int c = 0; c < 2; c++)
        {
            const FBotHit Hit = BotTrace((AActor*)Me, Eyes, Chest, Channels[c]);

            if (!Hit.bHit || !Hit.Actor || Hit.Actor == (AActor*)Them)
                continue;

            const std::string ClassName = BotClassName(Hit.Actor);

            if (ClassName.find("Landscape") != std::string::npos)
            {
                if (WhatWasHit)
                    *WhatWasHit = BotObjectName(Hit.Actor);

                return true;
            }
        }

        return false;
    }

    static std::map<void*, std::pair<int, bool>> BotGroundCheck;   

    static float TimeAccum = 0.f;
    static float LastFillTime = 0.f;
    static bool bMovedToIsland = false;
    static float BusStartedTime = 0.f;

    static void TickMovement(UWorld* World, AFortGameStateAthena* GameState, AFortGameMode* GameMode)
    {
        int nativeBrainCount = 0;
        int fallbackCount = 0;

        static int reportCounter_forDir = 0;
        static std::map<void*, int> BotWanderDirTick;

        
        
        std::set<void*> LivePawns;

        for (int i = 0; i < GameState->PlayerArray.Num(); i++)
        {
            auto LiveState = (AFortPlayerStateAthena*)GameState->PlayerArray[i];
            if (!LiveState || !LiveState->HasOwner() || !LiveState->Owner)
                continue;

            auto LivePawn = (AFortPlayerPawnAthena*)((AFortPlayerControllerAthena*)LiveState->Owner)->Pawn;

            if (LivePawn && LivePawn->GetHealth() > 0.f)
                LivePawns.insert((void*)LivePawn);
        }

        for (int i = 0; i < GameState->PlayerArray.Num(); i++)
        {
            auto PlayerState = (AFortPlayerStateAthena*)GameState->PlayerArray[i];
            if (!PlayerState || !PlayerState->HasbIsABot() || !PlayerState->bIsABot || NPCs::IsNPC(PlayerState))
                continue;

            auto Controller = (AFortPlayerControllerAthena*)PlayerState->Owner;
            if (!Controller)
                continue;

            auto Pawn = (AFortPlayerPawnAthena*)Controller->Pawn;
            if (!Pawn)
                continue;

            if (Pawn->HasCharacterMovement() && Pawn->CharacterMovement)
            {
                auto Move = Pawn->CharacterMovement;

                if (Move->HasMaxWalkSpeed())
                {
                    float Sprint = (FConfiguration::BotSprintSpeed > 0.f) ? FConfiguration::BotSprintSpeed : 510.f;
                    if (Move->MaxWalkSpeed < Sprint)
                        Move->MaxWalkSpeed = Sprint;
                }

                if (FConfiguration::bBotsFaceTheirDirection && Move->HasVelocity())
                {
                    const FVector Velocity = Move->Velocity;
                    const double SpeedX = Velocity.X;
                    const double SpeedY = Velocity.Y;
                    const double FlatSpeed = sqrt(SpeedX * SpeedX + SpeedY * SpeedY);

                    float Threshold = FConfiguration::BotFacingMinimumSpeed;
                    if (!(Threshold > 0.f))
                        Threshold = 40.f;

                    if (FlatSpeed > (double)Threshold)
                    {
                        const double TravelYaw = atan2(SpeedY, SpeedX) * (180.0 / 3.14159265358979323846);

                        auto Faced = BotFacingYaw.find(Pawn);
                        double Difference = (Faced != BotFacingYaw.end()) ? (TravelYaw - (double)Faced->second) : 999.0;

                        while (Difference > 180.0) Difference -= 360.0;
                        while (Difference < -180.0) Difference += 360.0;

                        if (fabs(Difference) > 4.0)
                        {
                            BotFacingYaw[Pawn] = (float)TravelYaw;

                            FRotator Heading{};
                            double HeadPitch = 0.0, HeadYaw = TravelYaw, HeadRoll = 0.0;
                            Heading.Pitch = HeadPitch;
                            Heading.Yaw = HeadYaw;
                            Heading.Roll = HeadRoll;

                            Pawn->K2_SetActorRotation(Heading, false);

                            ((::AAIController*)Controller)->SetControlRotation(Heading);
                        }
                    }
                }
            }

            auto AIController = (::AAIController*)Controller;

            
            SilenceNativeSenses(AIController);

            const bool bHasNativeBrain = AIController->HasBrainComponent() && AIController->BrainComponent;

            if (bHasNativeBrain)
            {
                nativeBrainCount++;

                if (!FConfiguration::bAllBotsFight)
                    continue;
            }
            else
            {
                fallbackCount++;
            }

            if (!bHasNativeBrain && BTRetried.find((void*)Controller) == BTRetried.end())
            {
                BTRetried.insert((void*)Controller);

                static UObject* PhoebeBT = nullptr;
                static bool bLookedUp = false;
                if (!bLookedUp)
                {
                    bLookedUp = true;
                    PhoebeBT = (UObject*)FindObject<UObject>(L"/Game/Athena/AI/Phoebe/BT_Phoebe.BT_Phoebe");
                    LogLine("[BotAI] BT_Phoebe asset lookup: %s\n", PhoebeBT ? "found" : "NOT FOUND");
                }

                if (PhoebeBT)
                {
                    bool started = AIController->RunBehaviorTree(PhoebeBT);
                    static int reported = 0;
                    if (reported < 3)
                    {
                        reported++;
                        LogLine("[BotAI] Manual RunBehaviorTree returned %s.\n", started ? "true" : "false");
                    }
                }
            }

            FVector MyLoc = Pawn->K2_GetActorLocation();

            FVector Direction;
            bool bOutsideSafeZone = Pawn->HasbIsInsideSafeZone() && !Pawn->bIsInsideSafeZone;

            auto combatIt = BotCombatTarget.find(Pawn);
            AFortPlayerPawnAthena* Target = (combatIt != BotCombatTarget.end()) ? combatIt->second : nullptr;

            
            
            
            
            
            
            const bool bCombatOpen = BotCombatIsOpen() && BotHasLanded((void*)Pawn);

            
            WatchBotTrigger(Pawn, PlayerState, reportCounter_forDir);

            if (!bCombatOpen)
            {
                Target = nullptr;
                BotCombatTarget[Pawn] = nullptr;
                BotLastSaw.erase((void*)Pawn);
                AggroedByHuman.erase((void*)Controller);
                BotUnderFireSince.erase((void*)Controller);

                BotStopAiming(AIController);
                BotIntendedAim.erase((void*)Pawn);

                auto FireIt = BotFire.find(Pawn);

                if (FireIt != BotFire.end())
                {
                    if (FireIt->second.bFiring || FireIt->second.BurstTicksLeft > 0)
                    {
                        Pawn->PawnStopFire(0);
                        BotTriggerUpTick[(void*)Pawn] = reportCounter_forDir;
                        FireIt->second.bFiring = false;
                    }

                    FireIt->second.BurstTicksLeft = 0;
                    FireIt->second.ReactionTicksLeft = -1;
                    FireIt->second.CooldownTicksLeft = 0;
                }

                
                continue;
            }

            
            
            
            if (Target && LivePawns.find((void*)Target) == LivePawns.end())
            {
                GLocksEnded++;

                auto Locked = BotLockedOnName.find(Pawn);

                if (GLocksEnded <= 10)
                    LogLine("[LOCK] %s's target '%s' is down -- free to lock onto somebody new.", SafeNameOf(PlayerState).c_str(),
                            Locked != BotLockedOnName.end() ? Locked->second.c_str() : "?");

                if (Locked != BotLockedOnName.end())
                    BotLockedOnName.erase(Locked);

                Target = nullptr;
                BotCombatTarget[Pawn] = nullptr;
                BotLastSaw.erase((void*)Pawn);
                BotLastSeenSpot.erase((void*)Pawn);
                BotIntendedAim.erase((void*)Pawn);
                BotGroundCheck.erase((void*)Pawn);
                BotStoppedForFight.erase((void*)Pawn);
            }

            if (!Target)
            {
                AFortPlayerPawnAthena* Best = nullptr;

                const double BotRange = (FConfiguration::BotEngageRange > 0.f) ? (double)FConfiguration::BotEngageRange : 12000.0;
                const double HuntRange = (FConfiguration::BotHuntPlayerRange > 0.f) ? (double)FConfiguration::BotHuntPlayerRange : 24000.0;
                const double Preference = (FConfiguration::BotPlayerPreference > 0.f) ? (double)FConfiguration::BotPlayerPreference : 0.35;

                double bestScore = -1.0;

                for (int e = 0; e < GameState->PlayerArray.Num(); e++)
                {
                    auto OtherState = (AFortPlayerStateAthena*)GameState->PlayerArray[e];
                    if (!OtherState || OtherState == PlayerState)
                        continue;
                    auto OtherController = (AFortPlayerControllerAthena*)OtherState->Owner;
                    if (!OtherController)
                        continue;
                    auto OtherPawn = (AFortPlayerPawnAthena*)OtherController->Pawn;
                    if (!OtherPawn || OtherPawn->GetHealth() <= 0.0f)
                        continue;

                    const bool bIsHuman = OtherState->HasbIsABot() && !OtherState->bIsABot && !NPCs::IsNPC(OtherState);

                    if (!bIsHuman && !FConfiguration::bBotsFightEachOther)
                        continue;

                    if (bIsHuman && !FConfiguration::bBotsFightRealPlayers)
                        continue;

                    FVector OLoc = OtherPawn->K2_GetActorLocation();
                    const double odx = OLoc.X - MyLoc.X;
                    const double ody = OLoc.Y - MyLoc.Y;
                    const double Distance = sqrt(odx * odx + ody * ody);

                    
                    
                    
                    const double kBotOnBotMaxRange = (FConfiguration::BotShootRealPlayerRange > 0.f)
                        ? (double)FConfiguration::BotShootRealPlayerRange : 3000.0;
                    double MaxRange = bIsHuman
                        ? (double)FConfiguration::BotShootRealPlayerRange
                        : kBotOnBotMaxRange;
                    if (bIsHuman && (MaxRange <= 0.0 || MaxRange > HuntRange))
                        MaxRange = HuntRange;

                    if (Distance > MaxRange)
                        continue;

                    
                    
                    
                    if (!BotCanSee(AIController, Pawn, MyLoc, OtherPawn, (AActor*)OtherPawn, reportCounter_forDir, bIsHuman))
                        continue;

                    
                    
                    if (BotGroundInTheWay(Pawn, MyLoc, OtherPawn, OLoc))
                        continue;

                    const double Score = bIsHuman ? (Distance * Preference) : Distance;

                    if (bestScore < 0.0 || Score < bestScore)
                    {
                        bestScore = Score;
                        Best = OtherPawn;
                    }
                }

                
                
                if (!Best)
                {
                    auto Agg = AggroedByHuman.find((void*)Controller);
                    if (Agg != AggroedByHuman.end() && Agg->second.first &&
                        (GStormTick - Agg->second.second) <= (int)FConfiguration::BotShotAggroTicks)
                    {
                        auto AggState = (AFortPlayerStateAthena*)Agg->second.first;
                        if (AggState && AggState->HasOwner() && AggState->Owner)
                        {
                            auto AggCtrl = (AFortPlayerControllerAthena*)AggState->Owner;
                            auto AggPawn = (AFortPlayerPawnAthena*)AggCtrl->Pawn;
                            if (AggPawn && AggPawn->GetHealth() > 0.f)
                                Best = AggPawn;
                        }
                        else
                            AggroedByHuman.erase((void*)Controller);
                    }
                }

                Target = Best;
                BotCombatTarget[Pawn] = Target;

                if (Target)
                {
                    GLocksMade++;

                    bool bLockedOnHuman = false;
                    if (auto LockController = (AFortPlayerControllerAthena*)Target->Controller)
                        if (auto LockState = (AFortPlayerStateAthena*)LockController->PlayerState)
                            bLockedOnHuman = LockState->HasbIsABot() && !LockState->bIsABot && !NPCs::IsNPC(LockState);

                    const std::string TargetName = BotPawnName(Target);
                    BotLockedOnName[Pawn] = TargetName;

                    const FVector LockLoc = Target->K2_GetActorLocation();
                    const double LockDx = LockLoc.X - MyLoc.X;
                    const double LockDy = LockLoc.Y - MyLoc.Y;

                    static int LockReports = 0;
                    if (bLockedOnHuman || ++LockReports <= 10)
                        LogLine("[LOCK] '%s' locked onto '%s' (%s) %.0f m away -- it fights only them now, until one of them dies.",
                                SafeNameOf(PlayerState).c_str(), TargetName.c_str(), bLockedOnHuman ? "a real player" : "a bot",
                                (float)(sqrt(LockDx * LockDx + LockDy * LockDy) / 100.0));
                }
            }

            bool bInCombat = false;
            bool bLongRangeAggro = false;

            
            
            double EngageRange = (FConfiguration::BotEngageRange > 0.f) ? (double)FConfiguration::BotEngageRange : 12000.0;
            if (Target)
            {
                if (auto TgtController = (AFortPlayerControllerAthena*)Target->Controller)
                    if (auto TgtState = (AFortPlayerStateAthena*)TgtController->PlayerState)
                        if (TgtState->HasbIsABot() && TgtState->bIsABot)
                            EngageRange = (FConfiguration::BotShootRealPlayerRange > 0.f)
                                ? (double)FConfiguration::BotShootRealPlayerRange : 3000.0; 
            }
            const double PreferredRange = 1400.0;

            const bool bMayFight = FConfiguration::bBotCombatEnabled && bCombatOpen;

            
            
            
            bool bTargetIsHuman = false;
            if (Target)
            {
                if (auto TgtController = (AFortPlayerControllerAthena*)Target->Controller)
                    if (auto TgtState = (AFortPlayerStateAthena*)TgtController->PlayerState)
                        bTargetIsHuman = TgtState->HasbIsABot() && !TgtState->bIsABot && !NPCs::IsNPC(TgtState);
            }

            bool bHumanTooFar = false;
            if (Target && bTargetIsHuman)
            {
                FVector TgtLoc = Target->K2_GetActorLocation();
                const double hx = TgtLoc.X - MyLoc.X;
                const double hy = TgtLoc.Y - MyLoc.Y;
                const double Dist = sqrt(hx * hx + hy * hy);
                bHumanTooFar = Dist > (double)FConfiguration::BotShootRealPlayerRange;

                
                if (bHumanTooFar)
                {
                    auto Agg = AggroedByHuman.find((void*)Controller);
                    if (Agg != AggroedByHuman.end() && Agg->second.first &&
                        (GStormTick - Agg->second.second) <= (int)FConfiguration::BotShotAggroTicks)
                    {
                        auto AggState = (AFortPlayerStateAthena*)Agg->second.first;
                        if (AggState)
                        {
                            if (auto TgtController = (AFortPlayerControllerAthena*)Target->Controller)
                                if (TgtController->PlayerState == AggState)
                                {
                                    bHumanTooFar = false;
                                    bLongRangeAggro = true;
                                }
                        }
                    }
                }
            }

            
            

            if (bMayFight && Target && Target->GetHealth() > 0.0f)
            {
                FVector TLoc = Target->K2_GetActorLocation();

                const double tdx = TLoc.X - MyLoc.X;
                const double tdy = TLoc.Y - MyLoc.Y;
                const double tdz = TLoc.Z - MyLoc.Z;
                const double Flat = sqrt(tdx * tdx + tdy * tdy);

                
                const double EffectiveEngage = bLongRangeAggro
                    ? ((FConfiguration::BotHuntPlayerRange > 0.f) ? (double)FConfiguration::BotHuntPlayerRange : 50000.0)
                    : EngageRange;

                {
                    bInCombat = true;

                    
                    
                    
                    
                    const bool bInShootRange = !bHumanTooFar && Flat < EffectiveEngage;
                    const bool bGameSaysVisible = BotHasLineOfSight(AIController, (AActor*)Target);

                    bool bGroundInTheWay = false;

                    if (bGameSaysVisible)
                    {
                        auto Checked = BotGroundCheck.find((void*)Pawn);

                        if (Checked == BotGroundCheck.end() || (reportCounter_forDir - Checked->second.first) >= 6)
                        {
                            std::string Blocker;
                            const bool bBlocked = BotGroundInTheWay(Pawn, MyLoc, Target, TLoc, &Blocker);

                            BotGroundCheck[(void*)Pawn] = { reportCounter_forDir, bBlocked };

                            if (bBlocked)
                            {
                                static int GroundReports = 0;

                                if (++GroundReports <= 8)
                                    LogLine("[EYES] The game's check said '%s' can see '%s' (%.0f m), but the ground (%s) is in "
                                            "the way -- no shot.",
                                            SafeNameOf(PlayerState).c_str(), BotPawnName(Target).c_str(),
                                            (float)(Flat / 100.0), Blocker.c_str());
                            }
                        }

                        bGroundInTheWay = BotGroundCheck[(void*)Pawn].second;

                        if (bGroundInTheWay)
                            GTerrainBlocks++;
                    }
                    else if (bInShootRange)
                    {
                        
                        static int BlockedSamples = 0;

                        if (BlockedSamples < 6 && BotTraceAvailable())
                        {
                            BlockedSamples++;

                            FVector Eyes = MyLoc;
                            Eyes.Z = MyLoc.Z + 60.0;

                            FVector Chest = TLoc;
                            Chest.Z = TLoc.Z + 30.0;

                            const FBotHit Hit = BotTrace((AActor*)Pawn, Eyes, Chest, 0);

                            LogLine("[EYES] Sample: the game's check says '%s' cannot see '%s' (%.0f m) -- a straight line to "
                                    "them hits %s (%s).",
                                    SafeNameOf(PlayerState).c_str(), BotPawnName(Target).c_str(), (float)(Flat / 100.0),
                                    Hit.bHit ? BotObjectName(Hit.Actor).c_str() : "nothing",
                                    Hit.bHit ? BotClassName(Hit.Actor).c_str() : "-");
                        }
                    }

                    const bool bTargetVisible = bGameSaysVisible && !bGroundInTheWay;
                    const bool bCanShoot = bInShootRange && bTargetVisible;

                    if (bTargetVisible)
                        BotLastSeenSpot[(void*)Pawn] = TLoc;

                    const double Yaw = atan2(tdy, tdx) * (180.0 / 3.14159265358979323846);
                    const double Pitch = atan2(tdz, Flat > 1.0 ? Flat : 1.0) * (180.0 / 3.14159265358979323846);

                    
                    
                    
                    {
                        auto Was = BotIntendedAim.find((void*)Pawn);
                        auto FireNow = BotFire.find((void*)Pawn);

                        if (Was != BotIntendedAim.end() && FireNow != BotFire.end() && FireNow->second.bFiring && !GAimReadFaulted)
                        {
                            double ActualYaw = 0.0;

                            if (BotReadAimGuarded(AIController, &ActualYaw))
                            {
                                double Off = ActualYaw - Was->second.Yaw;
                                while (Off > 180.0) Off -= 360.0;
                                while (Off < -180.0) Off += 360.0;
                                Off = fabs(Off);

                                GAimChecks++;

                                
                                
                                
                                if (Off <= 2.0)
                                    GAimHeld++;

                                GAimOffSum += Off;

                                if (Off > GAimOffMax)
                                    GAimOffMax = Off;
                            }
                        }
                    }

                    if (bTargetVisible)
                    {
                        
                        
                        
                        
                        
                        double ErrYaw = 0.0, ErrPitch = 0.0;

                        
                        const float AimDeg = bLongRangeAggro
                            ? FConfiguration::BotLongRangeAimErrorDegrees
                            : FConfiguration::BotAimErrorDegrees;

                        if (AimDeg > 0.f)
                        {
                            auto et = BotAimErrorTick.find(Pawn);
                            if (et == BotAimErrorTick.end() || (reportCounter_forDir - et->second) > 12)
                            {
                                BotAimErrorTick[Pawn] = reportCounter_forDir;

                                const double Max = (double)AimDeg;
                                const double PitchMax = Max * (double)FConfiguration::BotAimErrorPitchScale;

                                ErrYaw   = (((double)(rand() % 2001) / 1000.0) - 1.0) * Max;
                                ErrPitch = (((double)(rand() % 2001) / 1000.0) - 1.0) * PitchMax;

                                
                                
                                
                                if (FConfiguration::BotAimMissChance > 0.f &&
                                    (double)(rand() % 10000) < (double)FConfiguration::BotAimMissChance * 10000.0)
                                {
                                    const double AimDist = sqrt(Flat * Flat + tdz * tdz);
                                    const double HalfWidth = atan2(45.0, AimDist > 1.0 ? AimDist : 1.0) * (180.0 / 3.14159265358979323846);
                                    const double Miss = HalfWidth + 1.5 + ((double)(rand() % 1000) / 1000.0) * 2.5;

                                    ErrYaw = (rand() % 2) ? Miss : -Miss;
                                    GAimDeliberateMisses++;
                                }

                                BotAimError[Pawn] = FVector{ ErrYaw, ErrPitch, 0.0 };
                            }
                            else
                            {
                                auto ae = BotAimError.find(Pawn);
                                if (ae != BotAimError.end())
                                {
                                    ErrYaw = ae->second.X;
                                    ErrPitch = ae->second.Y;
                                }
                            }
                        }

                        
                        const double AimDist = sqrt(Flat * Flat + tdz * tdz);
                        const double HalfWide = atan2(45.0, AimDist > 1.0 ? AimDist : 1.0) * (180.0 / 3.14159265358979323846);
                        const double HalfTall = atan2(85.0, AimDist > 1.0 ? AimDist : 1.0) * (180.0 / 3.14159265358979323846);
                        const bool bOnTarget = fabs(ErrYaw) <= HalfWide && fabs(ErrPitch) <= HalfTall;

                        
                        
                        if (!bOnTarget && fabs(ErrYaw) <= HalfWide)
                            ErrYaw = (ErrYaw < 0.0 ? -1.0 : 1.0) * (HalfWide + 1.0);

                        FRotator Aim{};
                        double AimPitch = Pitch + ErrPitch, AimYaw = Yaw + ErrYaw, AimRoll = 0.0;
                        Aim.Pitch = AimPitch;
                        Aim.Yaw = AimYaw;
                        Aim.Roll = AimRoll;

                        AIController->SetControlRotation(Aim);

                        FRotator Face{};
                        double FacePitch = 0.0, FaceYaw = Yaw, FaceRoll = 0.0;
                        Face.Pitch = FacePitch;
                        Face.Yaw = FaceYaw;
                        Face.Roll = FaceRoll;
                        Pawn->K2_SetActorRotation(Face, false);

                        
                        BotAimAt(AIController, bOnTarget ? Target : nullptr, MyLoc, AimYaw, AimPitch, AimDist);

                        FBotAimRecord AimRecord{};
                        AimRecord.Yaw = bOnTarget ? Yaw : AimYaw;
                        AimRecord.Pitch = AimPitch;
                        BotIntendedAim[(void*)Pawn] = AimRecord;
                    }
                    else
                    {
                        
                        
                        BotStopAiming(AIController);
                        BotIntendedAim.erase((void*)Pawn);
                    }

                    
                    
                    
                    if (Flat > PreferredRange || !bTargetVisible)
                    {
                        BotStoppedForFight.erase((void*)Pawn);

                        FVector Goal = TLoc;

                        if (!bTargetVisible)
                        {
                            auto Spot = BotLastSeenSpot.find((void*)Pawn);

                            if (Spot != BotLastSeenSpot.end())
                            {
                                const double SpotX = Spot->second.X - MyLoc.X;
                                const double SpotY = Spot->second.Y - MyLoc.Y;

                                if ((SpotX * SpotX + SpotY * SpotY) > (500.0 * 500.0))
                                    Goal = Spot->second;
                                else
                                    BotLastSeenSpot.erase(Spot);   
                            }
                        }

                        const double GoalDx = Goal.X - MyLoc.X;
                        const double GoalDy = Goal.Y - MyLoc.Y;
                        const double GoalLen = sqrt(GoalDx * GoalDx + GoalDy * GoalDy);

                        auto ct = BotCombatTick.find(Pawn);
                        if (ct == BotCombatTick.end() || (reportCounter_forDir - ct->second) > 12)
                        {
                            BotCombatTick[Pawn] = reportCounter_forDir;

                            
                            AIController->MoveToLocation(Goal, 400.f, true,
                                                          false, false, false,
                                                         (UClass*)nullptr, true);
                        }

                        
                        
                        
                        static std::map<void*, FVector> ChaseOrigin;
                        static std::map<void*, int> ChaseCheckTick;

                        auto ChaseTick = ChaseCheckTick.find(Pawn);

                        if (ChaseTick == ChaseCheckTick.end() || (reportCounter_forDir - ChaseTick->second) >= 30)
                        {
                            auto ChaseFrom = ChaseOrigin.find(Pawn);

                            if (ChaseTick != ChaseCheckTick.end() && ChaseFrom != ChaseOrigin.end() && GoalLen > 1.0)
                            {
                                const double MovedX = MyLoc.X - ChaseFrom->second.X;
                                const double MovedY = MyLoc.Y - ChaseFrom->second.Y;

                                if ((MovedX * MovedX + MovedY * MovedY) < (150.0 * 150.0))
                                {
                                    FVector Push{};
                                    Push.X = GoalDx / GoalLen;
                                    Push.Y = GoalDy / GoalLen;
                                    Push.Z = 0.0;
                                    Pawn->AddMovementInput(Push, 1.f, false);

                                    FVector Hopup{};
                                    Hopup.X = Push.X * 120.0;
                                    Hopup.Y = Push.Y * 120.0;
                                    Hopup.Z = 520.0;
                                    Pawn->LaunchCharacterJump(Hopup, false, nullptr, true);

                                    GChaseHops++;
                                }
                            }

                            ChaseOrigin[Pawn] = MyLoc;
                            ChaseCheckTick[Pawn] = reportCounter_forDir;
                        }
                    }
                    else if (!GStopFaulted && BotStoppedForFight.insert((void*)Pawn).second)
                    {
                        
                        
                        BotStopWalkingGuarded(AIController);
                    }

                    FBotFireState& Fire = BotFire[Pawn];

                    if (!bCanShoot)
                    {
                        
                        
                        if (Fire.bFiring)
                        {
                            Pawn->PawnStopFire(0);
                            BotTriggerUpTick[(void*)Pawn] = reportCounter_forDir;

                            if (bInShootRange && !bTargetVisible)
                                GBurstsCutOutOfSight++;
                        }

                        Fire.bFiring = false;
                        Fire.BurstTicksLeft = 0;
                        Fire.ReactionTicksLeft = -1;

                        if (bInShootRange && !bTargetVisible)
                            GTicksHeldFireOutOfSight++;
                    }
                    else if (Fire.ReactionTicksLeft < 0)
                    {
                        Fire.ReactionTicksLeft = 8 + (rand() % 18);
                    }

                    if (Fire.ReloadTicksLeft > 0)
                    {
                        Fire.ReloadTicksLeft--;

                        if (Fire.ReloadTicksLeft == 0)
                        {
                            FinishBotReload(Pawn, (void*)PlayerState);
                            Fire.RoundsSinceReload = 0;

                            Fire.LastSeenAmmo = -1;

                            Fire.CooldownTicksLeft = 4 + (rand() % 8);
                        }
                    }
                    else if (BotNeedsToReload(Pawn, (void*)PlayerState, Fire))
                    {
                        if (Fire.bFiring)
                        {
                            Pawn->PawnStopFire(0);
                            BotTriggerUpTick[(void*)Pawn] = reportCounter_forDir;
                        }

                        Fire.bFiring = false;
                        Fire.BurstTicksLeft = 0;

                        Fire.ReloadTicksLeft = 45 + (rand() % 30);

                        if (Pawn->HasCurrentWeapon() && Pawn->CurrentWeapon)
                            SetWeaponReloadingFlag((AActor*)Pawn->CurrentWeapon, true);

                        GReloadsStarted++;
                    }
                    else if (!bCanShoot)
                    {
                        
                    }
                    else if (Fire.ReactionTicksLeft > 0)
                    {
                        Fire.ReactionTicksLeft--;
                    }
                    else if (Fire.BurstTicksLeft > 0)
                    {
                        Fire.BurstTicksLeft--;

                        if (Fire.BurstTicksLeft == 0)
                        {
                            Pawn->PawnStopFire(0);
                            BotTriggerUpTick[(void*)Pawn] = reportCounter_forDir;
                            Fire.bFiring = false;
                            Fire.CooldownTicksLeft = 12 + (rand() % 24);
                        }
                    }
                    else if (Fire.CooldownTicksLeft > 0)
                    {
                        Fire.CooldownTicksLeft--;
                    }
                    else
                    {
                        if (BotIsHoldingAWeapon(Pawn))
                        {
                            Pawn->PawnStartFire(0);
                            Fire.bFiring = true;
                            Fire.BurstTicksLeft = 8 + (rand() % 10);

                            Fire.RoundsSinceReload += 1 + (Fire.BurstTicksLeft / 5);

                            GCombatBurstsFired++;

                            if (auto TargetController = (AActor*)Target->Controller)
                                ShotAtBy[(void*)TargetController] = { (void*)PlayerState, GStormTick };
                        }
                        else
                        {
                            GCombatNoWeapon++;

                            Fire.CooldownTicksLeft = 60;

                            auto WeaponIt = BotWeapons.find((void*)PlayerState);
                            if (WeaponIt != BotWeapons.end() && WeaponIt->second)
                            {
                                FGuid ReEquipGuid{};
                                bool bFoundEntry = false;

                                auto InvIt = BotInventories.find((void*)PlayerState);
                                auto Inventory = (InvIt != BotInventories.end()) ? InvIt->second : nullptr;

                                if (Inventory && Inventory->HasInventory())
                                {
                                    for (int q = 0; q < Inventory->Inventory.ItemInstances.Num(); q++)
                                    {
                                        auto Item = Inventory->Inventory.ItemInstances[q];
                                        if (!Item || !Item->HasItemEntry())
                                            continue;
                                        if (!FFortItemEntry::HasItemDefinition() || Item->ItemEntry.ItemDefinition != WeaponIt->second)
                                            continue;
                                        if (FFortItemEntry::HasItemGuid())
                                            ReEquipGuid = Item->ItemEntry.ItemGuid;
                                        bFoundEntry = true;
                                        break;
                                    }

                                    if (!bFoundEntry)
                                    {
                                        int Clip = BotMagazineSize((void*)PlayerState);

                                        if (auto Fresh = Inventory->GiveItem(WeaponIt->second, 1, Clip))
                                        {
                                            if (Fresh->HasItemEntry() && FFortItemEntry::HasItemGuid())
                                                ReEquipGuid = Fresh->ItemEntry.ItemGuid;

                                            GWeaponsReissued++;
                                        }
                                    }
                                }

                                Pawn->EquipWeaponDefinition(WeaponIt->second, ReEquipGuid);
                            }
                        }
                    }

                    GCombatEngagements++;
                }
            }

            if (!bInCombat)
            {
                auto FireIt = BotFire.find(Pawn);
                if (FireIt != BotFire.end())
                {
                    if (FireIt->second.bFiring)
                    {
                        Pawn->PawnStopFire(0);
                        BotTriggerUpTick[(void*)Pawn] = reportCounter_forDir;
                    }

                    BotFire.erase(FireIt);
                }

                
                BotStopAiming(AIController);
                BotIntendedAim.erase((void*)Pawn);
                BotStoppedForFight.erase((void*)Pawn);
                BotLastSeenSpot.erase((void*)Pawn);
            }

            FVector ZoneCentre{};
            float ZoneRadius = 0.f;
            const bool bZoneKnown = FConfiguration::bBotsFleeTheStorm && GetSafeZoneCircle(GameMode, ZoneCentre, ZoneRadius);

            double ZoneDx = MyLoc.X - ZoneCentre.X;
            double ZoneDy = MyLoc.Y - ZoneCentre.Y;
            double ZoneDistance = bZoneKnown ? sqrt(ZoneDx * ZoneDx + ZoneDy * ZoneDy) : 0.0;

            const bool bWouldBeOutsideNextCircle = bZoneKnown && (ZoneDistance > (double)ZoneRadius * 0.85);

            if (bZoneKnown && (bOutsideSafeZone || bWouldBeOutsideNextCircle))
            {
                double dx = ZoneDx;
                double dy = ZoneDy;
                double Length = ZoneDistance;

                if (bOutsideSafeZone)
                    GZoneBotsInStorm++;
                {
                    auto WasAt = BotLastZoneDistance.find(Pawn);
                    if (WasAt != BotLastZoneDistance.end() && bOutsideSafeZone)
                    {
                        if (Length < WasAt->second - 1.0)
                            GZoneBotsClosing++;
                        else if (Length < WasAt->second + 1.0)
                            GZoneBotsStuck++;
                    }
                    BotLastZoneDistance[Pawn] = (float)Length;
                }

                
                
                
                
                
                
                
                
                FVector Goal = ZoneCentre;

                float Fraction = FConfiguration::BotZoneEntryFraction;
                if (!(Fraction >= 0.f) || Fraction > 0.9f)
                    Fraction = 0.5f;

                {
                    const uint32 Seed = BotPersonalSeed((void*)Pawn);

                    const double Angle = ((double)(Seed % 3600) / 10.0) * 3.14159265358979323846 / 180.0;

                    
                    
                    const double Spread = 0.30 + ((double)((Seed >> 12) % 60) / 100.0);

                    const double Out = (double)ZoneRadius * Spread * (double)(Fraction / 0.5f);

                    Goal.X = ZoneCentre.X + cos(Angle) * Out;
                    Goal.Y = ZoneCentre.Y + sin(Angle) * Out;
                    Goal.Z = MyLoc.Z;
                }

                if (Pawn->HasCharacterMovement() && Pawn->CharacterMovement)
                {
                    auto Move = Pawn->CharacterMovement;
                    if (Move->HasMaxWalkSpeed() && Move->MaxWalkSpeed < 500.f)
                        Move->MaxWalkSpeed = 510.f;
                }

                
                
                FVector Hop = Goal;

                {
                    double HopDx = Goal.X - MyLoc.X;
                    double HopDy = Goal.Y - MyLoc.Y;
                    double HopLen = sqrt(HopDx * HopDx + HopDy * HopDy);

                    const double HopDistance = 8000.0;

                    if (HopLen > HopDistance)
                    {
                        Hop = MyLoc;
                        Hop.X += (HopDx / HopLen) * HopDistance;
                        Hop.Y += (HopDy / HopLen) * HopDistance;
                    }
                }

                auto it = BotWanderGoals.find(Pawn);
                auto zt = BotZoneMoveTick.find(Pawn);

                const bool bGoalMoved = (it == BotWanderGoals.end()) ||
                                        (fabs(it->second.X - Hop.X) > 100.0) || (fabs(it->second.Y - Hop.Y) > 100.0);
                const bool bDueAgain = (zt == BotZoneMoveTick.end()) || ((reportCounter_forDir - zt->second) > 30);

                if (bGoalMoved || bDueAgain)
                {
                    BotWanderGoals[Pawn] = Hop;
                    BotZoneMoveTick[Pawn] = reportCounter_forDir;

                    AIController->MoveToLocation(Hop, 100.f, true,  false, false, false, (UClass*)nullptr, true);

                    double GoalDx = Goal.X - MyLoc.X;
                    double GoalDy = Goal.Y - MyLoc.Y;
                    double GoalLen = sqrt(GoalDx * GoalDx + GoalDy * GoalDy);

                    if (GoalLen > 1.0)
                    {
                        FRotator Look{};
                        Look.Pitch = 0.0;
                        Look.Yaw = (double)(atan2f((float)GoalDy, (float)GoalDx) * (180.f / 3.14159265f));
                        Look.Roll = 0.0;
                        Pawn->K2_SetActorRotation(Look, false);
                    }

                    GZoneOrdersIssued++;
                }

                {
                    static std::map<void*, FVector> ZoneMoveOrigin;
                    static std::set<void*> ZoneStuck;

                    auto Origin = ZoneMoveOrigin.find(Pawn);

                    if (bGoalMoved || bDueAgain || Origin == ZoneMoveOrigin.end())
                    {
                        if (Origin != ZoneMoveOrigin.end())
                        {
                            const double MovedX = MyLoc.X - Origin->second.X;
                            const double MovedY = MyLoc.Y - Origin->second.Y;

                            if ((MovedX * MovedX + MovedY * MovedY) < (200.0 * 200.0))
                                ZoneStuck.insert(Pawn);
                            else
                                ZoneStuck.erase(Pawn);
                        }

                        ZoneMoveOrigin[Pawn] = MyLoc;
                    }

                    if (ZoneStuck.find(Pawn) != ZoneStuck.end() && Pawn->HasCharacterMovement() && Pawn->CharacterMovement)
                    {
                        double GoalDx = Hop.X - MyLoc.X;
                        double GoalDy = Hop.Y - MyLoc.Y;
                        double GoalLen = sqrt(GoalDx * GoalDx + GoalDy * GoalDy);

                        if (GoalLen > 1.0)
                        {
                            FVector Push{};
                            Push.X = GoalDx / GoalLen;
                            Push.Y = GoalDy / GoalLen;
                            Push.Z = 0.0;
                            Pawn->AddMovementInput(Push, 1.f, false);

                            if ((reportCounter_forDir % 22) == 0)
                            {
                                FVector Hopup{};
                                Hopup.X = Push.X * 120.0;
                                Hopup.Y = Push.Y * 120.0;
                                Hopup.Z = 520.0;
                                Pawn->LaunchCharacterJump(Hopup, false, nullptr, true);
                            }
                        }
                    }
                }

                continue;
            }

            if (bInCombat)
                continue;

            {
                if (Pawn->HasCharacterMovement() && Pawn->CharacterMovement)
                {
                    auto Move = Pawn->CharacterMovement;
                    if (Move->HasMaxWalkSpeed() && Move->MaxWalkSpeed < 500.f)
                        Move->MaxWalkSpeed = 510.f;
                }

                auto it = BotWanderDirTick.find(Pawn);

                bool bNeedsNewDest = (it == BotWanderDirTick.end()) ||
                                     ((reportCounter_forDir - it->second) > 30);
                if (bNeedsNewDest)
                {
                    BotWanderDirTick[Pawn] = reportCounter_forDir;

                    
                    
                    FVector Dest;

                    {
                        float angle = (float)(rand() % 360) * (3.14159265f / 180.f);
                        float dist  = (float)(3000 + (rand() % 8000));
                        Dest = MyLoc;
                        Dest.X += cosf(angle) * dist;
                        Dest.Y += sinf(angle) * dist;
                    }

                    BotWanderGoals[Pawn] = Dest;

                    uint8_t moveResult = AIController->MoveToLocation(Dest, 50.f, true, false, false, false, (UClass*)nullptr, true);

                    if (moveResult == 0) MoveResult_Failed++;
                    else if (moveResult == 1) MoveResult_AlreadyAtGoal++;
                    else if (moveResult == 2) MoveResult_Success++;
                    else MoveResult_Other++;
                }

                {
                    static std::map<void*, FVector> WanderOrigin;
                    static std::map<void*, int> WanderStuckSince;

                    auto Origin = WanderOrigin.find(Pawn);

                    if (Origin == WanderOrigin.end())
                    {
                        WanderOrigin[Pawn] = MyLoc;
                    }
                    else if ((reportCounter_forDir % 20) == 0)
                    {
                        const double MovedX = MyLoc.X - Origin->second.X;
                        const double MovedY = MyLoc.Y - Origin->second.Y;
                        const bool bBarelyMoved = (MovedX * MovedX + MovedY * MovedY) < (150.0 * 150.0);

                        if (bBarelyMoved)
                            WanderStuckSince[Pawn] = WanderStuckSince.count(Pawn) ? WanderStuckSince[Pawn] + 1 : 1;
                        else
                            WanderStuckSince.erase(Pawn);

                        WanderOrigin[Pawn] = MyLoc;
                    }

                    auto StuckIt = WanderStuckSince.find(Pawn);

                    if (StuckIt != WanderStuckSince.end() && StuckIt->second >= 1 &&
                        Pawn->HasCharacterMovement() && Pawn->CharacterMovement)
                    {
                        auto GoalIt = BotWanderGoals.find(Pawn);

                        if (GoalIt != BotWanderGoals.end())
                        {
                            double GoalDx = GoalIt->second.X - MyLoc.X;
                            double GoalDy = GoalIt->second.Y - MyLoc.Y;
                            double GoalLen = sqrt(GoalDx * GoalDx + GoalDy * GoalDy);

                            if (GoalLen > 1.0)
                            {
                                FVector Push{};
                                Push.X = GoalDx / GoalLen;
                                Push.Y = GoalDy / GoalLen;
                                Push.Z = 0.0;
                                Pawn->AddMovementInput(Push, 1.f, false);

                                
                                
                                
                                
                                
                                
                                if ((reportCounter_forDir % 8) == 0)
                                {
                                    const double FaceYaw = atan2(GoalDy, GoalDx) * (180.0 / 3.14159265358979323846);
                                    FRotator Face = Pawn->K2_GetActorRotation();
                                    Face.Yaw = FaceYaw;
                                    Pawn->K2_SetActorRotation(Face, true);
                                }

                                if ((reportCounter_forDir % 24) == 0)
                                {
                                    FVector Hopup{};
                                    Hopup.X = Push.X * 120.0;
                                    Hopup.Y = Push.Y * 120.0;
                                    Hopup.Z = 520.0;
                                    Pawn->LaunchCharacterJump(Hopup, false, nullptr, true);
                                }
                            }
                        }

                        if (StuckIt->second >= 3)
                        {
                            BotWanderDirTick.erase(Pawn);
                            BotWanderGoals.erase(Pawn);
                            WanderStuckSince.erase(Pawn);
                            GWanderUnstuck++;
                        }
                    }
                }
            }
        }

        if (FConfiguration::bBotLog && (reportCounter_forDir % 150) == 0 && GZoneBotsInStorm > 0)
        {
            LogLine("[ZONE] In the storm: %d | closing on the circle: %d | not moving: %d | orders given: %d (straight-line, no navmesh)",
                    GZoneBotsInStorm, GZoneBotsClosing, GZoneBotsStuck, GZoneOrdersIssued);
        }

        if (FConfiguration::bBotLog && (reportCounter_forDir % 300) == 0 && GCombatEngagements > 0)
        {
            LogLine("[COMBAT] Engagements: %d | bursts fired: %d | shots that actually landed bot-on-bot: %d | "
                    "tried to fire holding nothing: %d | weapons handed back: %d | reloads: %d | magazines refilled: %d | "
                    "unstuck while wandering: %d",
                    GCombatEngagements, GCombatBurstsFired, GCombatBotOnBotHits,
                    GCombatNoWeapon, GWeaponsReissued, GReloadsStarted, GMagazinesRefilled, GWanderUnstuck);

            if ((GElimsFromLedger + GElimsFromShotAt + GElimsFromFallback) > 0)
            {
                LogLine("[ELIM] Credited from the game's own damage ledger: %d | from who was shooting at them: %d | from the last-resort guess: %d",
                        GElimsFromLedger, GElimsFromShotAt, GElimsFromFallback);
            }

            int LockedNow = 0;
            for (auto& Lock : BotLockedOnName)
                if (LivePawns.find(Lock.first) != LivePawns.end())
                    LockedNow++;

            LogLine("[LOCK] Locked on: %d | ended because the target died: %d | locked right now: %d | bursts cut off when the target "
                    "went out of sight: %d | ticks holding fire with the target in range but out of sight: %d | deliberate misses: %d | "
                    "hops while chasing: %d",
                    GLocksMade, GLocksEnded, LockedNow, GBurstsCutOutOfSight, GTicksHeldFireOutOfSight, GAimDeliberateMisses, GChaseHops);

            
            LogLine("[TRIGGER] Ticks a bot's gun actually went off: in its own bursts %d | with nothing in the bot code pulling the "
                    "trigger %d (let go of every time -- this should stay at 0).",
                    GShotTicksOurs, GShotTicksStray);

            LogLine("[AIM] Mid-burst, the gun was still pointed within 2 degrees of where the bot code aimed it on %d of %d checks "
                    "(off by %.1f degrees on average, worst %.0f) | built-in senses switched off on %d bot(s).",
                    GAimHeld, GAimChecks, GAimChecks > 0 ? GAimOffSum / (double)GAimChecks : 0.0, GAimOffMax, GSensesSilencedBots);

            LogLine("[HITS] Bot-on-bot hits on the one the shooter is locked onto: %d | on somebody else: %d | ticks a shot was "
                    "held because the ground was in the way: %d",
                    GHitsOnLockedTarget, GHitsOnSomebodyElse, GTerrainBlocks);
        }

        GZoneBotsInStorm = 0;
        GZoneBotsClosing = 0;
        GZoneBotsStuck = 0;

        reportCounter_forDir++;

        static int reportCounter = 0;
        if (((++reportCounter) % 900) == 0 && (nativeBrainCount + fallbackCount) > 0)
        {
            int movers = 0, stuck = 0;
            double totalDist = 0.0;
            double maxDist = 0.0;

            for (int i = 0; i < GameState->PlayerArray.Num(); i++)
            {
                auto PS = (AFortPlayerStateAthena*)GameState->PlayerArray[i];
                if (!PS || !PS->HasbIsABot() || !PS->bIsABot) continue;
                auto PC = (AFortPlayerControllerAthena*)PS->Owner;
                if (!PC) continue;
                auto Pw = (AFortPlayerPawnAthena*)PC->Pawn;
                if (!Pw) continue;

                FVector Now = Pw->K2_GetActorLocation();
                auto prev = MovementProbe.find((void*)Pw);
                if (prev != MovementProbe.end())
                {
                    double dx = Now.X - prev->second.X;
                    double dy = Now.Y - prev->second.Y;
                    double dz = Now.Z - prev->second.Z;
                    double d = sqrt(dx*dx + dy*dy + dz*dz);
                    totalDist += d;
                    if (d > maxDist) maxDist = d;
                    if (d > 200.0) movers++; else stuck++;
                }
                MovementProbe[(void*)Pw] = Now;
            }

            int probed = movers + stuck;
            LogLine("[BotAI] Movement: %d native-brain, %d fallback | moved-since-last-report: %d, stationary: %d, avg %.0f units, max %.0f units\n",
                   nativeBrainCount, fallbackCount, movers, stuck,
                   probed > 0 ? (float)(totalDist / probed) : 0.f, (float)maxDist);

            LogLine("[BotAI] Wander MoveOrders(total): success=%d failed=%d alreadyAtGoal=%d other=%d | all straight-line (navmesh removed)\n",
                   MoveResult_Success, MoveResult_Failed, MoveResult_AlreadyAtGoal, MoveResult_Other);
        }
    }

    static void GameThreadTick_Inner();

    static int SehFaultCount = 0;

    void GameThreadTick()
    {
        FCrashReporter::EnterGuardedSection();

        __try
        {
            GameThreadTick_Inner();
        }
        __except (EXCEPTION_EXECUTE_HANDLER)
        {
            SehFaultCount++;
            if (SehFaultCount <= 20)
            {
                LogLine("[GUARD] Caught an access violation inside the bot tick (fault #%d). Server kept alive; "
                        "this tick was skipped. Repeated faults point at Epic's native AI running with "
                        "missing skillset data.", SehFaultCount);
            }
            if (SehFaultCount == 21)
                LogLine("[GUARD] Further faults will not be logged individually.");
        }

        FCrashReporter::LeaveGuardedSection();
    }

    static void GameThreadTick_Inner()
    {
        static bool bAnnounced = false;
        if (!bAnnounced)
        {
            bAnnounced = true;
            LogLine("[BotAI] Game-thread tick is running.\n");

            FindObject<UObject>(L"/Game/Athena/GameplayCueNotifies/GCN_Athena_OutsideSafeZoneDamage.GCN_Athena_OutsideSafeZoneDamage_C");
            FindObject<UObject>(L"/Game/Athena/GameplayCueNotifies/GCN_Athena_PetrolPickup_EnterWater.GCN_Athena_PetrolPickup_EnterWater_C");
            FindObject<UObject>(L"/Game/Athena/GameplayCueNotifies/GCN_Athena_PetrolPickup_HitWorld.GCN_Athena_PetrolPickup_HitWorld_C");
            LogLine("[BotAI] Preloaded storm-damage gameplay cue.\n");
        }

        KeepTheClockHonest();

        NPCs::GameThreadTick();

        if (FConfiguration::bDisableStormSurge)
        {
            auto SurgeWorld = UWorld::GetWorld();

            if (SurgeWorld && SurgeWorld->AuthorityGameMode)
            {
                auto SurgeMode = (AFortGameMode*)SurgeWorld->AuthorityGameMode;

                static int Reported = 0;
                static bool bFoundAny = false;

                if (SurgeMode->HasbDisableStormCapSystem() && !SurgeMode->bDisableStormCapSystem)
                {
                    SurgeMode->bDisableStormCapSystem = true;
                    bFoundAny = true;
                }

                if (SurgeMode->HasbStormCapSystemEnabled() && SurgeMode->bStormCapSystemEnabled)
                {
                    SurgeMode->bStormCapSystemEnabled = false;
                    bFoundAny = true;
                }

                if (SurgeMode->HasPlaylistSupportsStormCapSystem() && SurgeMode->PlaylistSupportsStormCapSystem)
                {
                    SurgeMode->PlaylistSupportsStormCapSystem = false;
                    bFoundAny = true;
                }

                if (SurgeWorld->GameState)
                {
                    auto SurgeState = (AFortGameStateAthena*)SurgeWorld->GameState;
                    if (SurgeState->HasStormCapState() && SurgeState->StormCapState != 0)
                    {
                        uint8 Off = 0;
                        SurgeState->StormCapState = Off;
                        bFoundAny = true;
                    }
                }

                if (bFoundAny && ++Reported == 1)
                    LogLine("[SURGE] Storm surge turned off (Epic calls it the Storm Cap system). It will be held off for the whole match.");
            }
        }

        if (FConfiguration::AutoFillTargetPlayerCount <= 0)
            return;

        auto World = UWorld::GetWorld();
        if (!World || !World->GameState || !World->AuthorityGameMode)
            return;

        auto GameState = (AFortGameStateAthena*)World->GameState;
        auto GameMode = (AFortGameMode*)World->AuthorityGameMode;
        if (!GameState->HasPlayerArray() || !GameMode->HasAlivePlayers())
            return;

        float Now = (float)UGameplayStatics::GetTimeSeconds(World);

        GStormTick++;

        PlaceDump::EnsureLoaded();
        PlaceDump::ReportOnce();
        PlaceDump::AnnounceHotkey();
        PlaceDump::RecordTick();

        if (!FConfiguration::bDisableStorm)
        {
            SilenceDistantStorm();
            EnsureStormTimingData(GameState);
            ApplyCustomZoneTimings(GameMode);
            LogStormState(GameMode, GameState);

            WipeTheSlateAtBusTime(GameState);

            TrackEveryBotAlive(GameState, GStormTick);

            ApplyStormDamage(GameMode, GameState, GStormTick);
        }

        if (FConfiguration::bDisableStorm)
        {
            float FarFutureTime = Now + 999999999.f;

            if (GameState->HasSafeZonesStartTime())
                GameState->SafeZonesStartTime = Now + 60.f;

            if (GameMode->HasSafeZonePhase())
                GameMode->SafeZonePhase = 0;

            if (GameMode->HasbSafeZonePaused())
                GameMode->bSafeZonePaused = true;
            if (GameMode->HasbSafeZoneActive())
                GameMode->bSafeZoneActive = false;

            if (GameMode->SafeZoneIndicator)
            {
                auto Ind = GameMode->SafeZoneIndicator;

                if (Ind->HasSafeZoneStartShrinkTime())
                    Ind->SafeZoneStartShrinkTime = FarFutureTime;
                if (Ind->HasSafeZoneFinishShrinkTime())
                    Ind->SafeZoneFinishShrinkTime = FarFutureTime;
                if (Ind->HasCurrentPhase())
                    Ind->CurrentPhase = 0;

                if (Ind->HasNextRadius())
                    Ind->NextRadius = 1000000.f;
                if (Ind->HasNextNextRadius())
                    Ind->NextNextRadius = 1000000.f;
                if (Ind->HasPreviousRadius())
                    Ind->PreviousRadius = 1000000.f;
                if (Ind->HasLastRadius())
                    Ind->LastRadius = 1000000.f;
            }

            static bool bAnnouncedStormOff = false;
            if (!bAnnouncedStormOff)
            {
                bAnnouncedStormOff = true;
                LogLine("[STORM] Freeze active. SafeZonesStartTime=%s SafeZonePhase=%s Paused=%s Active=%s Indicator=%s",
                        GameState->HasSafeZonesStartTime() ? "pinned" : "PROPERTY MISSING",
                        GameMode->HasSafeZonePhase()       ? "pinned" : "PROPERTY MISSING",
                        GameMode->HasbSafeZonePaused()     ? "set"    : "PROPERTY MISSING",
                        GameMode->HasbSafeZoneActive()     ? "set"    : "PROPERTY MISSING",
                        GameMode->SafeZoneIndicator        ? "present": "null (warmup)");
            }
        }
        else if (GameMode->SafeZoneIndicator)
        {
            auto Ind = GameMode->SafeZoneIndicator;

            static int LastStormPhaseSeen = -1;

            if (GameMode->HasSafeZonePhase() && Ind->HasSafeZoneStartShrinkTime() && Ind->HasSafeZoneFinishShrinkTime())
            {
                const int StormPhaseNow = (int)GameMode->SafeZonePhase;

                if (StormPhaseNow != LastStormPhaseSeen)
                {
                    LastStormPhaseSeen = StormPhaseNow;

                    const bool bDegenerate = !((float)Ind->SafeZoneFinishShrinkTime > Now + 1.f);

                    if (bDegenerate)
                    {
                        const float RescueWait = 30.f;
                        const float RescueShrink = 20.f;

                        Ind->SafeZoneStartShrinkTime = Now + RescueWait;
                        Ind->SafeZoneFinishShrinkTime = Now + RescueWait + RescueShrink;

                        printf("[Storm] Phase %d arrived with a schedule that had already expired -- given %.0fs hold and %.0fs close so the storm cannot run away.\n",
                               StormPhaseNow, RescueWait, RescueShrink);
                        fflush(stdout);
                    }
                    else if (FConfiguration::bStormLog)
                    {
                        printf("[Storm] Tick check on phase %d: schedule looks fine -- starts closing in %.1fs, done in %.1fs. Left alone.\n",
                               StormPhaseNow,
                               (float)Ind->SafeZoneStartShrinkTime - Now,
                               (float)Ind->SafeZoneFinishShrinkTime - Now);
                        fflush(stdout);
                    }
                }
            }
        }

        {
            static bool bAnnouncedWaiting = false;
            int humanCount = 0;
            for (int i = 0; i < GameState->PlayerArray.Num(); i++)
            {
                auto PS = (AFortPlayerStateAthena*)GameState->PlayerArray[i];
                if (PS && !(PS->HasbIsABot() && PS->bIsABot))
                    humanCount++;
            }

            if (humanCount == 0 && !bBusDeparted)
            {
                if (!bAnnouncedWaiting)
                {
                    bAnnouncedWaiting = true;
                    LogLine("Waiting for a player to join before spawning bots...");
                }
                return;
            }
        }

        if (!bBusDeparted && bBotSpawningPaused)
        {
            static bool bSaidPaused = false;
            static int LastSeenCount = -1;

            const int WaitingCount = GameMode->AlivePlayers.Num();

            if (!bSaidPaused || WaitingCount != LastSeenCount)
            {
                bSaidPaused = true;
                LastSeenCount = WaitingCount;
                LogLine("[BotAI] Bot spawning is paused -- %d player(s) in the lobby, waiting for \"cheat resume bot spawn\".",
                        WaitingCount);
            }

            BusCountdownStart = -1.f;
            LastBusSecondAnnounced = -1;
        }
        else if (!bBusDeparted)
        {
            int currentCount = GameMode->AlivePlayers.Num();

            if (currentCount >= FConfiguration::AutoFillTargetPlayerCount)
            {
                if (BusCountdownStart < 0.f)
                {
                    BusCountdownStart = Now;
                    LogLine("[BotAI] Lobby full (%d players). Bus starting in 10 seconds...", currentCount);
                    ServerConsole::Say("Player count reached %d! Starting bus...", currentCount);

                    if (GameState->HasWarmupCountdownStartTime())
                        GameState->WarmupCountdownStartTime = Now;
                    if (GameState->HasWarmupCountdownEndTime())
                        GameState->WarmupCountdownEndTime = Now + 10.f;

                    if (GameMode->HasWarmupCountdownDuration())
                        GameMode->WarmupCountdownDuration = 10.f;
                    if (GameMode->HasWarmupEarlyCountdownDuration())
                        GameMode->WarmupEarlyCountdownDuration = 10.f;

                    LogLine("[BotAI] HUD countdown set (WarmupCountdownEndTime = now + 10s).");
                }

                float remaining = 10.f - (Now - BusCountdownStart);

                int secsLeft = (int)ceil(remaining);
                if (secsLeft != LastBusSecondAnnounced && secsLeft >= 1 && secsLeft <= 10)
                {
                    LastBusSecondAnnounced = secsLeft;
                    LogLine("[BotAI] Bus starting in %d...", secsLeft);
                }

                if (remaining <= 0.f)
                {
                    LogLine("[BotAI] Bus launching now.");
                    ClearBusForLaunch("the lobby filled up and the countdown finished");
                    UKismetSystemLibrary::ExecuteConsoleCommand(World, FString(L"startaircraft"), nullptr);
                    bBusDeparted = true;
                    BusStartedTime = Now;
                }
            }
            else if ((Now - LastFillTime) >= (float)FConfiguration::AutoFillIntervalSeconds)
            {
                LastFillTime = Now;
                int remaining = FConfiguration::AutoFillTargetPlayerCount - currentCount;
                int batch = remaining < FConfiguration::AutoFillBatchSize ? remaining : FConfiguration::AutoFillBatchSize;
                SpawnBots(FVector(0, 0, 0), batch);
                LogLine("[BotAI] AutoFill: %d/%d players.\n", currentCount, FConfiguration::AutoFillTargetPlayerCount);
            }
        }

        int BotsInLobby = 0;
        for (int i = 0; i < GameState->PlayerArray.Num(); i++)
        {
            auto PS = (AFortPlayerStateAthena*)GameState->PlayerArray[i];

            if (PS && PS->HasbIsABot() && PS->bIsABot && !NPCs::IsNPC(PS))
                BotsInLobby++;
        }

        const bool bEveryBotThanked = (BotsInLobby > 0) && ((int)ThankedBots.size() >= BotsInLobby);
        const bool bWaitedLongEnough = bBusDeparted && (Now - BusStartedTime) >= 12.f;

        if (bBusDeparted && !bMovedToIsland && !bEveryBotThanked && bWaitedLongEnough)
        {
            static bool bSaidSo = false;
            if (!bSaidSo)
            {
                bSaidSo = true;
                LogLine("[BotAI] %d of %d bot(s) thanked the driver before the drop -- going anyway so nobody is left on the bus.",
                        (int)ThankedBots.size(), BotsInLobby);
            }
        }

        const bool bThanksDone = bEveryBotThanked || bWaitedLongEnough;

        if (bBusDeparted && !bMovedToIsland && bThanksDone)
        {
            if (MoveSomeBotsToIsland(4))
            {
                bMovedToIsland = true;
                LogLine("[BotAI] Finished moving bots onto the map.\n");
            }
        }

        {
            static int lastLoggedPhase = -999;
            if (GameMode->HasSafeZonePhase())
            {
                int phase = GameMode->SafeZonePhase;
                if (phase != lastLoggedPhase)
                {
                    lastLoggedPhase = phase;
                    LogLine("[BotAI] === STORM PHASE -> %d | AlivePlayers=%d PlayerArray=%d PlayersLeft=%d eliminated=%zu ===\n",
                           phase,
                           GameMode->AlivePlayers.Num(),
                           GameState->PlayerArray.Num(),
                           GameState->HasPlayersLeft() ? (int)GameState->PlayersLeft : -1,
                           EliminatedBots.size());
                }
            }
        }

        HoldTheStorm(GameMode, Now);
        ReleaseTheStorm(GameMode, Now);

        ApplyWeaponHotfixes();
        TopUpGrapplers(GameState, GStormTick);

        ClaimServerOwner(GameState);

        LogHudState();
        UpdateHudCounts();

        TrackBotDamage();
        TryBotReactiveBuild(GameState);
        RemoveDeadBots();

        CheckForVictory();
        ApplyDamageProtection();
        RegisterBotsAsAlive();
        AssignBotTeams();
        GiveBotPickaxes();
        GiveBotLoadouts();
        ThankAllBots();
        RenameUnnamedBots();
        DressUndressedBots();
        TickMovement(World, GameState, GameMode);
        TickParachutes(World);
        TickLandings(World);
    }
}
