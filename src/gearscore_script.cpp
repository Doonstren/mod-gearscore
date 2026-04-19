#include "ScriptMgr.h"
#include "Player.h"
#include "Config.h"
#include "ItemTemplate.h"
#include "Item.h"
#include "WorldSession.h"
#include "DatabaseEnv.h"
#include "Log.h"
#include "Chat.h"
#include "CommandScript.h"

// Static Helper Functions
static float GetSlotModifier(uint32 equipLoc)
{
    switch (equipLoc)
    {
        case INVTYPE_RELIC:
        case INVTYPE_RANGED:
        case INVTYPE_THROWN:
        case INVTYPE_RANGEDRIGHT:
            return 0.3164f;
        case INVTYPE_TRINKET:
        case INVTYPE_NECK:
        case INVTYPE_FINGER:
        case INVTYPE_CLOAK:
        case INVTYPE_WRISTS:
            return 0.5625f;
        case INVTYPE_2HWEAPON:
            return 2.000f;
        case INVTYPE_WEAPONMAINHAND:
        case INVTYPE_WEAPONOFFHAND:
        case INVTYPE_SHIELD:
        case INVTYPE_WEAPON:
        case INVTYPE_HOLDABLE:
        case INVTYPE_HEAD:
        case INVTYPE_CHEST:
        case INVTYPE_ROBE:
        case INVTYPE_LEGS:
            return 1.0000f;
        case INVTYPE_SHOULDERS:
        case INVTYPE_WAIST:
        case INVTYPE_FEET:
        case INVTYPE_HANDS:
            return 0.7500f;
        case INVTYPE_BODY:
        case INVTYPE_TABARD:
        default:
            return 0.0f;
    }
}

static uint32 CalculateGearScore(Player* player)
{
    float gearScore = 0.0f;
    float titanGrip = 1.0f;

    Item* mainHand = player->GetItemByPos(INVENTORY_SLOT_BAG_0, EQUIPMENT_SLOT_MAINHAND);
    Item* offHand = player->GetItemByPos(INVENTORY_SLOT_BAG_0, EQUIPMENT_SLOT_OFFHAND);

    if (mainHand && offHand)
    {
        if (mainHand->GetTemplate()->InventoryType == INVTYPE_2HWEAPON ||
            offHand->GetTemplate()->InventoryType == INVTYPE_2HWEAPON)
        {
            titanGrip = 0.5f;
        }
    }

    for (uint8 i = EQUIPMENT_SLOT_START; i < EQUIPMENT_SLOT_END; ++i)
    {
        if (i == EQUIPMENT_SLOT_TABARD || i == EQUIPMENT_SLOT_BODY)
            continue;

        Item* item = player->GetItemByPos(INVENTORY_SLOT_BAG_0, i);
        if (!item)
            continue;

        ItemTemplate const* proto = item->GetTemplate();
        if (!proto)
            continue;

        uint32 rarity = proto->Quality;
        float itemLevel = proto->ItemLevel;
        uint32 equipLoc = proto->InventoryType;

        float qualityScale = 1.0f;
        float scale = 1.8618f;

        if (rarity == ITEM_QUALITY_LEGENDARY)
        {
            qualityScale = 1.3f;
            rarity = ITEM_QUALITY_EPIC;
        }
        else if (rarity == ITEM_QUALITY_NORMAL || rarity == ITEM_QUALITY_POOR)
        {
            qualityScale = 0.005f;
            rarity = ITEM_QUALITY_UNCOMMON;
        }
        else if (rarity == ITEM_QUALITY_HEIRLOOM)
        {
            rarity = ITEM_QUALITY_RARE;
            itemLevel = 187.05f;
        }

        float formulaA = 0.0f;
        float formulaB = 0.0f;

        if (itemLevel < 100.0f && rarity == ITEM_QUALITY_EPIC)
        {
            formulaA = 0.2500f; formulaB = 1.6275f;
        }
        else if (itemLevel < 168.0f && rarity == ITEM_QUALITY_EPIC)
        {
            formulaA = 26.0000f; formulaB = 1.2000f;
        }
        else if (itemLevel < 148.0f && rarity == ITEM_QUALITY_RARE)
        {
            formulaA = 0.7500f; formulaB = 1.8000f;
        }
        else if (itemLevel < 138.0f && rarity == ITEM_QUALITY_UNCOMMON)
        {
            formulaA = 8.0000f; formulaB = 2.0000f;
        }
        else
        {
            if (rarity == ITEM_QUALITY_EPIC) { formulaA = 91.4500f; formulaB = 0.6500f; }
            else if (rarity == ITEM_QUALITY_RARE) { formulaA = 81.3750f; formulaB = 0.8125f; }
            else if (rarity == ITEM_QUALITY_UNCOMMON) { formulaA = 73.0000f; formulaB = 1.0000f; }
        }

        if (rarity >= ITEM_QUALITY_UNCOMMON && rarity <= ITEM_QUALITY_EPIC)
        {
            float slotMod = GetSlotModifier(equipLoc);
            float tempScore = std::floor(((itemLevel - formulaA) / formulaB) * slotMod * scale * qualityScale);

            if (player->getClass() == CLASS_HUNTER)
            {
                if (equipLoc == INVTYPE_2HWEAPON || equipLoc == INVTYPE_WEAPONMAINHAND || equipLoc == INVTYPE_WEAPONOFFHAND || equipLoc == INVTYPE_WEAPON || equipLoc == INVTYPE_HOLDABLE)
                    tempScore *= 0.3164f;
                else if (equipLoc == INVTYPE_RANGEDRIGHT || equipLoc == INVTYPE_RANGED)
                    tempScore *= 5.3224f;
            }

            if (tempScore < 0.0f)
                tempScore = 0.0f;

            if (i == EQUIPMENT_SLOT_MAINHAND || i == EQUIPMENT_SLOT_OFFHAND)
            {
                if (equipLoc == INVTYPE_2HWEAPON)
                    tempScore *= titanGrip;
            }

            gearScore += tempScore;
        }
    }

    return static_cast<uint32>(std::floor(gearScore));
}

static void UpdateAndSaveGearScore(Player* player)
{
    if (!sConfigMgr->GetOption<bool>("GearScore.Enable", true))
        return;

    if (sConfigMgr->GetOption<bool>("GearScore.IgnoreBots", true) && player->GetSession()->IsBot())
        return;

    uint32 gs = CalculateGearScore(player);

    CharacterDatabase.Execute("REPLACE INTO character_gearscore (guid, name, class, gearscore) VALUES ({}, '{}', {}, {})", 
                              player->GetGUID().GetCounter(), player->GetName(), player->getClass(), gs);
}

class GearScorePlayerScript : public PlayerScript
{
public:
    GearScorePlayerScript() : PlayerScript("GearScorePlayerScript") { }

    void OnPlayerLogin(Player* player) override
    {
        UpdateAndSaveGearScore(player);
    }

    void OnPlayerEquip(Player* player, Item* /*it*/, uint8 /*bag*/, uint8 /*slot*/, bool /*update*/) override
    {
        UpdateAndSaveGearScore(player);
    }

    void OnPlayerUnequip(Player* player, Item* /*it*/) override
    {
        UpdateAndSaveGearScore(player);
    }
};

using namespace Acore::ChatCommands;

class gearscore_commandscript : public CommandScript
{
public:
    gearscore_commandscript() : CommandScript("gearscore_commandscript") { }

    ChatCommandTable GetCommands() const override
    {
        static ChatCommandTable commandTable =
        {
            { "gs", HandleGearScoreCommand, SEC_PLAYER, Console::No }
        };

        return commandTable;
    }

    static bool HandleGearScoreCommand(ChatHandler* handler, char const* args)
    {
        if (!sConfigMgr->GetOption<bool>("GearScore.Enable", true))
        {
            handler->PSendSysMessage("GearScore system is currently disabled.");
            return true;
        }

        uint32 locale = handler->GetSession()->GetSessionDbLocaleIndex();
        bool isRu = (locale == LOCALE_ruRU);

        // Help command
        if (args && strcmp(args, "help") == 0)
        {
            if (isRu)
            {
                handler->PSendSysMessage("Использование команды .gs:");
                handler->PSendSysMessage("  .gs - показать свой GearScore или GearScore выбранной цели.");
                handler->PSendSysMessage("  .gs [ИмяИгрока] - показать GearScore указанного игрока (даже если он оффлайн).");
                handler->PSendSysMessage("  .gs help - показать эту справку.");
            }
            else
            {
                handler->PSendSysMessage("Usage of .gs command:");
                handler->PSendSysMessage("  .gs - show your own GearScore or the GearScore of the selected target.");
                handler->PSendSysMessage("  .gs [PlayerName] - show the GearScore of the specified player (even if offline).");
                handler->PSendSysMessage("  .gs help - show this help message.");
            }
            return true;
        }

        Player* targetPlayer = nullptr;
        std::string targetName = "";
        ObjectGuid targetGuid = ObjectGuid::Empty;

        // No arguments passed, check selected target or self
        if (!args || *args == '\0')
        {
            Unit* targetUnit = handler->getSelectedUnit();
            if (targetUnit)
            {
                targetPlayer = targetUnit->ToPlayer();
                if (!targetPlayer)
                {
                    handler->PSendSysMessage(isRu ? "Вы выбрали не игрока." : "You have selected an NPC, not a player.");
                    return true; // We don't return false to avoid syntax error popups
                }
            }
            else
            {
                targetPlayer = handler->GetSession()->GetPlayer();
            }
        }
        else
        {
            // Argument passed, find player by name
            targetName = args;
            targetPlayer = ObjectAccessor::FindPlayerByName(targetName.c_str(), false);
            if (!targetPlayer)
            {
                // Player not online, find GUID from DB
                targetGuid = sCharacterCache->GetCharacterGuidByName(targetName);
                if (!targetGuid)
                {
                    handler->PSendSysMessage(isRu ? "Игрок не найден." : "Player not found.");
                    return true;
                }
            }
        }

        // If player is online (targetPlayer is not null)
        if (targetPlayer)
        {
            targetName = targetPlayer->GetName();
            targetGuid = targetPlayer->GetGUID();

            if (targetPlayer->GetSession() && targetPlayer->GetSession()->IsBot() && sConfigMgr->GetOption<bool>("GearScore.IgnoreBots", true))
            {
                handler->PSendSysMessage(isRu ? "Выбранная цель - бот. Расчет GS для ботов отключен." : "The selected target is a bot. GS calculation for bots is disabled.");
                return true;
            }

            uint32 gs = CalculateGearScore(targetPlayer);
            handler->PSendSysMessage(isRu ? "Игрок {} GearScore: |cff00ffff{}|r" : "Player {} GearScore: |cff00ffff{}|r", handler->playerLink(targetName), gs);
        }
        else if (targetGuid)
        {
            // Player is offline
            QueryResult result = CharacterDatabase.Query("SELECT gearscore FROM character_gearscore WHERE guid = {}", targetGuid.GetCounter());
            if (result)
            {
                Field* fields = result->Fetch();
                uint32 gs = fields[0].Get<uint32>();
                handler->PSendSysMessage(isRu ? "Игрок {} (Оффлайн) GearScore: |cff00ffff{}|r" : "Player {} (Offline) GearScore: |cff00ffff{}|r", targetName, gs);
            }
            else
            {
                handler->PSendSysMessage(isRu ? "У игрока {} нет сохраненного GearScore." : "Player {} has no recorded GearScore.", targetName);
            }
        }

        return true;
    }
};

void Addmod_gearscoreScripts()
{
    new GearScorePlayerScript();
    new gearscore_commandscript();
}