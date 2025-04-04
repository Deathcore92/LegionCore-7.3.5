/*
 * Copyright (C) 2008-2012 TrinityCore <http://www.trinitycore.org/>
 * Copyright (C) 2005-2009 MaNGOS <http://getmangos.com/>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#include "ArtifactPackets.h"
#include "CollectionMgr.h"
#include "CombatLogPackets.h"
#include "DatabaseEnv.h"
#include "GameTables.h"
#include "Item.h"
#include "ObjectMgr.h"
#include "ScriptMgr.h"
#include "ItemPackets.h"

void AddItemsSetItem(Player* player, Item* item)
{
    ItemTemplate const* proto = item->GetTemplate();
    uint32 setid = proto->GetItemSet();

    ItemSetEntry const* set = sItemSetStore.LookupEntry(setid);
    if (!set)
        return;

    if (set->RequiredSkill && player->GetSkillValue(set->RequiredSkill) < set->RequiredSkill)
        return;

    if (set->SetFlags & ITEM_SET_FLAG_LEGACY_INACTIVE)
        return;

    ItemSetEffect* eff = nullptr;
    for (auto const& v : *player->ItemSetEff)
        if (v && v->ItemSetID == setid)
        {
            eff = v;
            break;
        }

    if (!eff)
    {
        eff = new ItemSetEffect();
        eff->ItemSetID = setid;
        eff->EquippedItemCount = 0;

        size_t x = 0;
        for (; x < player->ItemSetEff->size(); ++x)
            if (!(*player->ItemSetEff)[x])
                break;

        if (x < player->ItemSetEff->size())
            (*player->ItemSetEff)[x] = eff;
        else
            player->ItemSetEff->push_back(eff);
    }

    ++eff->EquippedItemCount;

    DB2Manager::ItemSetSpells& spells = sDB2Manager._itemSetSpells[setid];
    for (ItemSetSpellEntry const* itemSetSpell : spells)
    {
        if (itemSetSpell->Threshold > eff->EquippedItemCount)
            continue;

        if (eff->SetBonuses.count(itemSetSpell))
            continue;

        SpellInfo const* spellInfo = sSpellMgr->GetSpellInfo(itemSetSpell->SpellID);
        if (!spellInfo)
            continue;

        eff->SetBonuses.insert(itemSetSpell);
        if (!itemSetSpell->ChrSpecID || itemSetSpell->ChrSpecID == player->GetUInt32Value(PLAYER_FIELD_CURRENT_SPEC_ID))
            player->ApplyEquipSpell(spellInfo, nullptr, true);
    }
}

void RemoveItemsSetItem(Player*player, ItemTemplate const* proto)
{
    uint32 setid = proto->GetItemSet();

    ItemSetEntry const* set = sItemSetStore.LookupEntry(setid);
    if (!set)
        return;

    ItemSetEffect* eff = nullptr;
    size_t setindex = 0;
    for (; setindex < player->ItemSetEff->size(); setindex++)
    {
        if ((*player->ItemSetEff)[setindex] && (*player->ItemSetEff)[setindex]->ItemSetID == setid)
        {
            eff = (*player->ItemSetEff)[setindex];
            break;
        }
    }

    if (!eff)
        return;

    --eff->EquippedItemCount;

    DB2Manager::ItemSetSpells const& spells = sDB2Manager._itemSetSpells[setid];
    for (ItemSetSpellEntry const* itemSetSpell : spells)
    {
        if (itemSetSpell->Threshold <= eff->EquippedItemCount)
            continue;

        if (!eff->SetBonuses.count(itemSetSpell))
            continue;

        player->ApplyEquipSpell(sSpellMgr->AssertSpellInfo(itemSetSpell->SpellID), nullptr, false);
        eff->SetBonuses.erase(itemSetSpell);
    }

    if (!eff->EquippedItemCount)
    {
        ASSERT(eff == (*player->ItemSetEff)[setindex]);
        delete eff;
        (*player->ItemSetEff)[setindex] = nullptr;
    }
}

bool ItemCanGoIntoBag(ItemTemplate const* pProto, ItemTemplate const* pBagProto)
{
    if (!pProto || !pBagProto)
        return false;

    switch (pBagProto->GetClass())
    {
        case ITEM_CLASS_CONTAINER:
            switch (pBagProto->GetSubClass())
            {
                case ITEM_SUBCLASS_CONTAINER:
                    return true;
                case ITEM_SUBCLASS_SOUL_CONTAINER:
                    if (!(pProto->GetBagFamily() & BAG_FAMILY_MASK_SOUL_SHARDS))
                        return false;
                    return true;
                case ITEM_SUBCLASS_HERB_CONTAINER:
                    if (!(pProto->GetBagFamily() & BAG_FAMILY_MASK_HERBS))
                        return false;
                    return true;
                case ITEM_SUBCLASS_ENCHANTING_CONTAINER:
                    if (!(pProto->GetBagFamily() & BAG_FAMILY_MASK_ENCHANTING_SUPP))
                        return false;
                    return true;
                case ITEM_SUBCLASS_MINING_CONTAINER:
                    if (!(pProto->GetBagFamily() & BAG_FAMILY_MASK_MINING_SUPP))
                        return false;
                    return true;
                case ITEM_SUBCLASS_ENGINEERING_CONTAINER:
                    if (!(pProto->GetBagFamily() & BAG_FAMILY_MASK_ENGINEERING_SUPP))
                        return false;
                    return true;
                case ITEM_SUBCLASS_GEM_CONTAINER:
                    if (!(pProto->GetBagFamily() & BAG_FAMILY_MASK_GEMS))
                        return false;
                    return true;
                case ITEM_SUBCLASS_LEATHERWORKING_CONTAINER:
                    if (!(pProto->GetBagFamily() & BAG_FAMILY_MASK_LEATHERWORKING_SUPP))
                        return false;
                    return true;
                case ITEM_SUBCLASS_INSCRIPTION_CONTAINER:
                    if (!(pProto->GetBagFamily() & BAG_FAMILY_MASK_INSCRIPTION_SUPP))
                        return false;
                    return true;
                case ITEM_SUBCLASS_TACKLE_CONTAINER:
                    if (!(pProto->GetBagFamily() & BAG_FAMILY_MASK_FISHING_SUPP))
                        return false;
                    return true;
                case ITEM_SUBCLASS_COOKING_BAG:
                    if (!(pProto->GetBagFamily() & BAG_FAMILY_MASK_COOKING_SUPP))
                        return false;
                    return true;
                default:
                    return false;
            }
        case ITEM_CLASS_QUIVER:
            switch (pBagProto->GetSubClass())
            {
                case ITEM_SUBCLASS_QUIVER:
                    if (!(pProto->GetBagFamily() & BAG_FAMILY_MASK_ARROWS))
                        return false;
                    return true;
                case ITEM_SUBCLASS_AMMO_POUCH:
                    if (!(pProto->GetBagFamily() & BAG_FAMILY_MASK_BULLETS))
                        return false;
                    return true;
                default:
                    return false;
            }
        default:
            break;
    }
    return false;
}

ItemModifier const AppearanceModifierSlotBySpec[MAX_SPECIALIZATIONS] =
{
    ITEM_MODIFIER_TRANSMOG_APPEARANCE_SPEC_1,
    ITEM_MODIFIER_TRANSMOG_APPEARANCE_SPEC_2,
    ITEM_MODIFIER_TRANSMOG_APPEARANCE_SPEC_3,
    ITEM_MODIFIER_TRANSMOG_APPEARANCE_SPEC_4
};

static uint32 const AppearanceModifierMaskSpecSpecific =
(1 << ITEM_MODIFIER_TRANSMOG_APPEARANCE_SPEC_1) |
(1 << ITEM_MODIFIER_TRANSMOG_APPEARANCE_SPEC_2) |
(1 << ITEM_MODIFIER_TRANSMOG_APPEARANCE_SPEC_3) |
(1 << ITEM_MODIFIER_TRANSMOG_APPEARANCE_SPEC_4);

ItemModifier const IllusionModifierSlotBySpec[MAX_SPECIALIZATIONS] =
{
    ITEM_MODIFIER_ENCHANT_ILLUSION_SPEC_1,
    ITEM_MODIFIER_ENCHANT_ILLUSION_SPEC_2,
    ITEM_MODIFIER_ENCHANT_ILLUSION_SPEC_3,
    ITEM_MODIFIER_ENCHANT_ILLUSION_SPEC_4
};

static uint32 const IllusionModifierMaskSpecSpecific =
(1 << ITEM_MODIFIER_ENCHANT_ILLUSION_SPEC_1) |
(1 << ITEM_MODIFIER_ENCHANT_ILLUSION_SPEC_2) |
(1 << ITEM_MODIFIER_ENCHANT_ILLUSION_SPEC_3) |
(1 << ITEM_MODIFIER_ENCHANT_ILLUSION_SPEC_4);

Item::Item()
{
    m_objectType |= TYPEMASK_ITEM;
    m_objectTypeId = TYPEID_ITEM;

    m_updateFlag = UPDATEFLAG_NONE;

    m_valuesCount = ITEM_END;
    _dynamicValuesCount = ITEM_DYNAMIC_END;

    m_lastPlayedTimeUpdate = GameTime::GetGameTime();

    m_refundRecipient.Clear();

    m_scaleLvl = 0;
    m_artIlvlBonus = 0;
    memset(&_bonusData, 0, sizeof(_bonusData));

    m_slot = 0;
    uState = ITEM_NEW;
    uQueuePos = -1;
    m_container = nullptr;
    m_lootGenerated = false;
    mb_in_trade = false;
    m_in_use = false;
    m_paidMoney = 0;
    m_paidExtendedCost = 0;
    m_artifactPowerCount = 0;
    m_ownerLevel = 0;
    dungeonEncounterID = 0;
    createdTime = 0;
    DescriptionID = 0;
    m_gemScalingLevels.fill(0);
    objectCountInWorld[uint8(HighGuid::Item)]++;
}

Item::~Item()
{
    objectCountInWorld[uint8(HighGuid::Item)]--;
}

bool Item::Create(ObjectGuid::LowType const& guidlow, uint32 itemid, Player const* owner)
{
    Object::_Create(ObjectGuid::Create<HighGuid::Item>(guidlow));

    SetEntry(itemid);
    SetObjectScale(1.0f);

    if (owner)
    {
        SetGuidValue(ITEM_FIELD_OWNER, owner->GetGUID());
        SetGuidValue(ITEM_FIELD_CONTAINED_IN, owner->GetGUID());
        SetOwnerLevel(owner->getLevel());
        SetMap(owner->GetMap());
    }

    ItemTemplate const* itemProto = sObjectMgr->GetItemTemplate(itemid);
    if (!itemProto)
        return false;

    DescriptionID = itemProto->GetItemNameDescriptionID();
    _bonusData.Initialize(itemProto);

    SetUInt32Value(ITEM_FIELD_STACK_COUNT, 1);
    SetUInt32Value(ITEM_FIELD_MAX_DURABILITY, itemProto->MaxDurability);
    SetUInt32Value(ITEM_FIELD_DURABILITY, itemProto->MaxDurability);

    for (std::size_t i = 0; i < itemProto->Effects.size(); ++i)
    {
        if (i < MAX_ITEM_PROTO_SPELLS)
            SetSpellCharges(itemProto->Effects[i]->LegacySlotIndex, itemProto->Effects[i]->Charges);

        if (owner)
            if (SpellInfo const* spellInfo = sSpellMgr->GetSpellInfo(itemProto->Effects[i]->SpellID))
                if (spellInfo->HasEffect(SPELL_EFFECT_GIVE_ARTIFACT_POWER))
                    if (uint32 artifactKnowledgeLevel = owner->GetCurrency(CURRENCY_TYPE_ARTIFACT_KNOWLEDGE))
                        SetModifier(ITEM_MODIFIER_ARTIFACT_KNOWLEDGE_LEVEL, artifactKnowledgeLevel + 1);
    }

    SetUInt32Value(ITEM_FIELD_EXPIRATION, itemProto->GetDurationInInventory());
    SetUInt32Value(ITEM_FIELD_CREATE_PLAYED_TIME, 0);

    if (itemProto->GetArtifactID())
    {
        InitArtifactPowers(itemProto->GetArtifactID());

        for (ArtifactAppearanceEntry const* artifactAppearance : sArtifactAppearanceStore)
        {
            if (ArtifactAppearanceSetEntry const* artifactAppearanceSet = sArtifactAppearanceSetStore.LookupEntry(artifactAppearance->ArtifactAppearanceSetID))
            {
                if (!owner || itemProto->GetArtifactID() != artifactAppearanceSet->ArtifactID)
                    continue;

                if (!sConditionMgr->IsPlayerMeetingCondition(const_cast<Player*>(owner), artifactAppearance->UnlockPlayerConditionID))
                    continue;

                SetModifier(ITEM_MODIFIER_ARTIFACT_APPEARANCE_ID, artifactAppearance->ID);
                SetAppearanceModId(artifactAppearance->ItemAppearanceModifierID);
                break;
            }
        }
    }          

    return true;
}

// Returns true if Item is a bag AND it is not empty.
// Returns false if Item is not a bag OR it is an empty bag.
bool Item::IsNotEmptyBag() const
{
    if (Bag const* bag = ToBag())
        return !bag->IsEmpty();
    return false;
}

bool Item::IsBroken() const
{
    return GetUInt32Value(ITEM_FIELD_MAX_DURABILITY) > 0 && GetUInt32Value(ITEM_FIELD_DURABILITY) == 0;
}

bool Item::IsDisable() const
{
    return HasFlag(ITEM_FIELD_DYNAMIC_FLAGS, ItemFieldFlags::ITEM_FLAG_DISABLE);
}

bool Item::CantBeUse() const
{
    return (IsBroken() || IsDisable());
}

void Item::UpdateDuration(Player* owner, uint32 diff)
{
    if (!GetUInt32Value(ITEM_FIELD_EXPIRATION))
        return;

    TC_LOG_DEBUG("entities.player.items", "Item::UpdateDuration Item (Entry: %u Duration %u Diff %u)", GetEntry(), GetUInt32Value(ITEM_FIELD_EXPIRATION), diff);

    if (GetUInt32Value(ITEM_FIELD_EXPIRATION) <= diff)
    {
        sScriptMgr->OnItemExpire(owner, GetTemplate());
        owner->DestroyItem(GetBagSlot(), GetSlot(), true);
        return;
    }

    UpdateUInt32Value(ITEM_FIELD_EXPIRATION, GetUInt32Value(ITEM_FIELD_EXPIRATION) - diff);
    SetState(ITEM_CHANGED, owner);                          // save new time in database
}

int32 Item::GetSpellCharges(uint8 index) const
{
    return GetInt32Value(ITEM_FIELD_SPELL_CHARGES + index);
}

void Item::SetSpellCharges(uint8 index, int32 value)
{
    SetInt32Value(ITEM_FIELD_SPELL_CHARGES + index, value);
}

void Item::SaveToDB(CharacterDatabaseTransaction& trans)
{
    auto isInTransaction = bool(trans);
    if (!isInTransaction)
        trans = CharacterDatabase.BeginTransaction();
    LoginDatabaseTransaction transs = LoginDatabase.BeginTransaction();

    ObjectGuid::LowType guid = GetGUIDLow();
    switch (uState)
    {
        case ITEM_NEW:
        case ITEM_CHANGED:
        {
            uint8 index = 0;
            CharacterDatabasePreparedStatement* stmt = CharacterDatabase.GetPreparedStatement(uState == ITEM_NEW ? CHAR_REP_ITEM_INSTANCE : CHAR_UPD_ITEM_INSTANCE);
            stmt->setUInt32(index, GetEntry());
            stmt->setUInt64(++index, GetOwnerGUID().GetCounter());
            stmt->setUInt64(++index, GetGuidValue(ITEM_FIELD_CREATOR).GetCounter());
            stmt->setUInt64(++index, GetGuidValue(ITEM_FIELD_GIFT_CREATOR).GetCounter());
            stmt->setUInt32(++index, GetCount());
            stmt->setUInt32(++index, GetUInt32Value(ITEM_FIELD_EXPIRATION));

            std::ostringstream ssSpells;
            if (auto itemProto = sObjectMgr->GetItemTemplate(GetEntry()))
                for (auto const& itemEffect : itemProto->Effects)
                    ssSpells << GetSpellCharges(itemEffect->LegacySlotIndex) << ' ';

            stmt->setString(++index, ssSpells.str());

            stmt->setUInt32(++index, GetUInt32Value(ITEM_FIELD_DYNAMIC_FLAGS));

            std::ostringstream ssEnchants;
            for (uint8 i = 0; i < MAX_ENCHANTMENT_SLOT; ++i)
            {
                ssEnchants << GetEnchantmentId(EnchantmentSlot(i)) << ' ';
                ssEnchants << GetEnchantmentDuration(EnchantmentSlot(i)) << ' ';
                ssEnchants << GetEnchantmentCharges(EnchantmentSlot(i)) << ' ';
            }
            stmt->setString(++index, ssEnchants.str());

            stmt->setUInt8(++index, uint8(GetItemRandomEnchantmentId().Type));
            stmt->setUInt32(++index, GetItemRandomEnchantmentId().Id);
            stmt->setUInt16(++index, GetUInt32Value(ITEM_FIELD_DURABILITY));
            stmt->setUInt32(++index, GetUInt32Value(ITEM_FIELD_CREATE_PLAYED_TIME));
            stmt->setString(++index, ""); // m_text
            stmt->setUInt32(++index, GetModifier(ITEM_MODIFIER_UPGRADE_ID));
            stmt->setUInt32(++index, GetModifier(ITEM_MODIFIER_BATTLE_PET_SPECIES_ID));
            stmt->setUInt32(++index, GetModifier(ITEM_MODIFIER_BATTLE_PET_BREED_DATA));
            stmt->setUInt32(++index, GetModifier(ITEM_MODIFIER_BATTLE_PET_LEVEL));
            stmt->setUInt32(++index, GetModifier(ITEM_MODIFIER_BATTLE_PET_DISPLAY_ID));

            std::ostringstream bonusListIDs;
            for (uint32 bonusListID : GetDynamicValues(ITEM_DYNAMIC_FIELD_BONUS_LIST_IDS))
                bonusListIDs << bonusListID << ' ';
            stmt->setString(++index, bonusListIDs.str());

            stmt->setUInt16(++index, GetItemLevel(GetOwnerLevel()));
            stmt->setUInt16(++index, dungeonEncounterID);
            stmt->setUInt8(++index, GetUInt32Value(ITEM_FIELD_CONTEXT));

            if (uState == ITEM_NEW)
                stmt->setUInt32(++index, GameTime::GetGameTime());
            
            stmt->setUInt64(++index, guid);

            trans->Append(stmt);

            if ((uState == ITEM_CHANGED) && HasFlag(ITEM_FIELD_DYNAMIC_FLAGS, ITEM_FLAG_WRAPPED))
            {
                stmt = CharacterDatabase.GetPreparedStatement(CHAR_UPD_GIFT_OWNER);
                stmt->setUInt64(0, GetOwnerGUID().GetCounter());
                stmt->setUInt64(1, guid);
                trans->Append(stmt);
            }

            if (GetGems().size())
            {
                stmt = CharacterDatabase.GetPreparedStatement(CHAR_INS_ITEM_INSTANCE_GEMS);
                stmt->setUInt64(0, GetGUID().GetCounter());
                uint32 i = 0;
                uint32 const gemFields = 4;
                for (ItemDynamicFieldGems const& gemData : GetGems())
                {
                    if (gemData.ItemId)
                    {
                        stmt->setUInt32(1 + i * gemFields, gemData.ItemId);
                        std::ostringstream gemBonusListIDs;
                        for (uint16 bonusListID : gemData.BonusListIDs)
                            if (bonusListID)
                                gemBonusListIDs << bonusListID << ' ';

                        stmt->setString(2 + i * gemFields, gemBonusListIDs.str());
                        stmt->setUInt8(3 + i * gemFields, gemData.Context);
                        stmt->setUInt32(4 + i * gemFields, m_gemScalingLevels[i]);
                    }
                    else
                    {
                        stmt->setUInt32(1 + i * gemFields, 0);
                        stmt->setString(2 + i * gemFields, "");
                        stmt->setUInt8(3 + i * gemFields, 0);
                        stmt->setUInt32(4 + i * gemFields, 0);
                    }
                    ++i;
                }
                for (; i < MAX_GEM_SOCKETS; ++i)
                {
                    stmt->setUInt32(1 + i * gemFields, 0);
                    stmt->setString(2 + i * gemFields, "");
                    stmt->setUInt8(3 + i * gemFields, 0);
                    stmt->setUInt32(4 + i * gemFields, 0);
                }
                trans->Append(stmt);
            }
            else if (!GetTemplate()->GetArtifactID())
            {
                stmt = CharacterDatabase.GetPreparedStatement(CHAR_DEL_ITEM_INSTANCE_GEMS);
                stmt->setUInt64(0, guid);
                trans->Append(stmt);
            }

            static ItemModifier const transmogMods[10] =
            {
                ITEM_MODIFIER_TRANSMOG_APPEARANCE_ALL_SPECS,
                ITEM_MODIFIER_TRANSMOG_APPEARANCE_SPEC_1,
                ITEM_MODIFIER_TRANSMOG_APPEARANCE_SPEC_2,
                ITEM_MODIFIER_TRANSMOG_APPEARANCE_SPEC_3,
                ITEM_MODIFIER_TRANSMOG_APPEARANCE_SPEC_4,

                ITEM_MODIFIER_ENCHANT_ILLUSION_ALL_SPECS,
                ITEM_MODIFIER_ENCHANT_ILLUSION_SPEC_1,
                ITEM_MODIFIER_ENCHANT_ILLUSION_SPEC_2,
                ITEM_MODIFIER_ENCHANT_ILLUSION_SPEC_3,
                ITEM_MODIFIER_ENCHANT_ILLUSION_SPEC_4,
            };

            stmt = CharacterDatabase.GetPreparedStatement(CHAR_DEL_ITEM_INSTANCE_TRANSMOG);
            stmt->setUInt64(0, guid);
            trans->Append(stmt);

            if (std::find_if(std::begin(transmogMods), std::end(transmogMods), [this] (ItemModifier modifier)
            {
                return GetModifier(modifier) != 0;
            }) != std::end(transmogMods))
            {
                stmt = CharacterDatabase.GetPreparedStatement(CHAR_INS_ITEM_INSTANCE_TRANSMOG);
                stmt->setUInt64(0, guid);
                stmt->setUInt32(1, GetModifier(ITEM_MODIFIER_TRANSMOG_APPEARANCE_ALL_SPECS));
                stmt->setUInt32(2, GetModifier(ITEM_MODIFIER_TRANSMOG_APPEARANCE_SPEC_1));
                stmt->setUInt32(3, GetModifier(ITEM_MODIFIER_TRANSMOG_APPEARANCE_SPEC_2));
                stmt->setUInt32(4, GetModifier(ITEM_MODIFIER_TRANSMOG_APPEARANCE_SPEC_3));
                stmt->setUInt32(5, GetModifier(ITEM_MODIFIER_TRANSMOG_APPEARANCE_SPEC_4));
                stmt->setUInt32(6, GetModifier(ITEM_MODIFIER_ENCHANT_ILLUSION_ALL_SPECS));
                stmt->setUInt32(7, GetModifier(ITEM_MODIFIER_ENCHANT_ILLUSION_SPEC_1));
                stmt->setUInt32(8, GetModifier(ITEM_MODIFIER_ENCHANT_ILLUSION_SPEC_2));
                stmt->setUInt32(9, GetModifier(ITEM_MODIFIER_ENCHANT_ILLUSION_SPEC_3));
                stmt->setUInt32(10, GetModifier(ITEM_MODIFIER_ENCHANT_ILLUSION_SPEC_4));
                trans->Append(stmt);
            }

            // stmt = CharacterDatabase.GetPreparedStatement(CHAR_DEL_ITEM_INSTANCE_ARTIFACT);
            // stmt->setUInt64(0, guid);
            // trans->Append(stmt);

            // stmt = CharacterDatabase.GetPreparedStatement(CHAR_DEL_ITEM_INSTANCE_ARTIFACT_POWERS);
            // stmt->setUInt64(0, guid);
            // trans->Append(stmt);

            if (GetTemplate()->GetArtifactID())
            {
              //  QueryResult check_rank = CharacterDatabase.PQuery("SELECT totalrank, tier FROM item_instance_artifact where char_guid = %u AND itemEntry = %u", GetOwnerGUID().GetCounter(), GetEntry());
              //  bool need_save = true;
              //  if (check_rank)
              //  {
              //      Field* fields = check_rank->Fetch();

              //      uint32 total_rank = fields[0].GetUInt32();
              //      uint32 tier = fields[1].GetUInt32();
              //      if ((total_rank > GetTotalPurchasedArtifactPowers() && tier == GetModifier(ITEM_MODIFIER_ARTIFACT_TIER)) || tier > GetModifier(ITEM_MODIFIER_ARTIFACT_TIER)) // otkat only if not activate new tier
              //          need_save = false;
              //  }
              //  if (need_save)
                {
                    stmt = CharacterDatabase.GetPreparedStatement(CHAR_REP_ITEM_INSTANCE_ARTIFACT);
                    stmt->setUInt64(0, guid);
                    stmt->setUInt64(1, GetUInt64Value(ITEM_FIELD_ARTIFACT_XP));
                    stmt->setUInt32(2, GetModifier(ITEM_MODIFIER_ARTIFACT_APPEARANCE_ID));
                    stmt->setUInt32(3, GetEntry());
                    stmt->setUInt32(4, GetModifier(ITEM_MODIFIER_ARTIFACT_TIER));
                    stmt->setUInt64(5, GetOwnerGUID().GetCounter());
                    stmt->setUInt32(6, GetTotalPurchasedArtifactPowers());
                    trans->Append(stmt);

                    for (ItemDynamicFieldArtifactPowers const& artifactPower : GetArtifactPowers())
                    {
                        stmt = CharacterDatabase.GetPreparedStatement(CHAR_REP_ITEM_INSTANCE_ARTIFACT_POWERS);
                        stmt->setUInt64(0, guid);
                        stmt->setUInt32(1, artifactPower.ArtifactPowerId);
                        stmt->setUInt8(2, artifactPower.PurchasedRank);
                        stmt->setUInt32(3, GetEntry());
                        stmt->setUInt64(4, GetOwnerGUID().GetCounter());
                        trans->Append(stmt);
                    }
                }
            }

            auto relics = GetArtifactSockets();
            if (!relics.empty())
            {
                std::stringstream output[3];
                for (uint8 i = 0; i < relics.size() && i < 3; ++i)
                {
                    output[i] << relics[i+2].unk1 << " " << relics[i+2].socketIndex << " " << relics[i+2].firstTier << " " << relics[i+2].secondTier << " " << relics[i+2].thirdTier << " " << relics[i+2].additionalThirdTier;
                }

                stmt = CharacterDatabase.GetPreparedStatement(CHAR_REP_ITEM_INSTANCE_RELICS);
                stmt->setUInt64(0, guid);
                stmt->setUInt64(1, GetOwnerGUID().GetCounter());
                for (uint8 i = 0; i < 3; ++i)
                    stmt->setString(i+2, output[i].str().c_str());

                trans->Append(stmt);
            }

            static ItemModifier const modifiersTable[] =
            {
                ITEM_MODIFIER_SCALING_STAT_DISTRIBUTION_FIXED_LEVEL,
                ITEM_MODIFIER_ARTIFACT_KNOWLEDGE_LEVEL
            };

            stmt = CharacterDatabase.GetPreparedStatement(CHAR_DEL_ITEM_INSTANCE_MODIFIERS);
            stmt->setUInt64(0, guid);
            trans->Append(stmt);

            if (_bonusData.HasFixedLevel && !GetModifier(ITEM_MODIFIER_SCALING_STAT_DISTRIBUTION_FIXED_LEVEL))
                SetModifier(ITEM_MODIFIER_SCALING_STAT_DISTRIBUTION_FIXED_LEVEL, GetOwnerLevel());

            if (std::find_if(std::begin(modifiersTable), std::end(modifiersTable), [this] (ItemModifier modifier)
            {
                return GetModifier(modifier) != 0;
            }) != std::end(modifiersTable))
            {
                stmt = CharacterDatabase.GetPreparedStatement(CHAR_INS_ITEM_INSTANCE_MODIFIERS);
                stmt->setUInt64(0, guid);
                stmt->setUInt32(1, GetModifier(ITEM_MODIFIER_SCALING_STAT_DISTRIBUTION_FIXED_LEVEL));
                stmt->setUInt32(2, GetModifier(ITEM_MODIFIER_ARTIFACT_KNOWLEDGE_LEVEL));
                trans->Append(stmt);
            }

            break;
        }
        case ITEM_REMOVED:
        {
            if (GetTemplate()->GetArtifactID()) // info about art will not delete
                break;
            
            CharacterDatabasePreparedStatement* stmt = CharacterDatabase.GetPreparedStatement(CHAR_DEL_ITEM_INSTANCE);
            stmt->setUInt64(0, guid);
            trans->Append(stmt);
            
            stmt = CharacterDatabase.GetPreparedStatement(CHAR_DEL_ITEM_INSTANCE_RELICS);
            stmt->setUInt64(0, guid);
            trans->Append(stmt);

            // stmt = CharacterDatabase.GetPreparedStatement(CHAR_DEL_ITEM_INSTANCE_ARTIFACT);
            // stmt->setUInt64(0, guid);
            // trans->Append(stmt);

            // stmt = CharacterDatabase.GetPreparedStatement(CHAR_DEL_ITEM_INSTANCE_ARTIFACT_POWERS);
            // stmt->setUInt64(0, guid);
            // trans->Append(stmt);

            stmt = CharacterDatabase.GetPreparedStatement(CHAR_DEL_ITEM_INSTANCE_MODIFIERS);
            stmt->setUInt64(0, guid);
            trans->Append(stmt);

            stmt = CharacterDatabase.GetPreparedStatement(CHAR_DEL_ITEM_INSTANCE_GEMS);
            stmt->setUInt64(0, guid);
            trans->Append(stmt);

            stmt = CharacterDatabase.GetPreparedStatement(CHAR_DEL_ITEM_INSTANCE_TRANSMOG);
            stmt->setUInt64(0, guid);
            trans->Append(stmt);

            if (HasFlag(ITEM_FIELD_DYNAMIC_FLAGS, ITEM_FLAG_WRAPPED))
            {
                stmt = CharacterDatabase.GetPreparedStatement(CHAR_DEL_GIFT);
                stmt->setUInt64(0, guid);
                trans->Append(stmt);
            }

            if (!isInTransaction)
                CharacterDatabase.CommitTransaction(trans);
            LoginDatabase.CommitTransaction(transs);

            delete this;
            return;
        }
        case ITEM_UNCHANGED:
            break;
    }

    SetState(ITEM_UNCHANGED);

    if (!isInTransaction)
        CharacterDatabase.CommitTransaction(trans);
}

bool Item::LoadFromDB(ObjectGuid::LowType const& guid, ObjectGuid const& owner_guid, Field* fields, uint32 entry, uint8 oLevel)
{
    //! WARNING! IF CHANGE FIELD ORDER - DO IT ON GuildMgr::LoadGuilds FOR GB Loading Items, AND CHAR_SEL_MAILITEMS CHAR_SEL_AUCTION_ITEMS 

    //0     1          2            3                4      5         6        7      8             9                   10                11          12          13
    //guid, itemEntry, creatorGuid, giftCreatorGuid, count, duration, charges, flags, enchantments, randomPropertyType, randomPropertyId, durability, playedTime, text,
    //14         15                  16                  17              18                  19            20              21              22              23               24               25
    //upgradeId, battlePetSpeciesId, battlePetBreedData, battlePetLevel, battlePetDisplayId, bonusListIDs, challengeMapID, challengeLevel, challengeAffix, challengeAffix1, challengeAffix2, challengeKeyIsCharded
    //26                              27                           28                           29                           30
    //itemModifiedAppearanceAllSpecs, itemModifiedAppearanceSpec1, itemModifiedAppearanceSpec2, itemModifiedAppearanceSpec3, itemModifiedAppearanceSpec4,
    //31                            32                         33                         34                         35
    //spellItemEnchantmentAllSpecs, spellItemEnchantmentSpec1, spellItemEnchantmentSpec2, spellItemEnchantmentSpec3, spellItemEnchantmentSpec4,
    //36          37           38           39                40          41           42           43                44          45           46           47                
    //gemItemId1, gemBonuses1, gemContext1, gemScalingLevel1, gemItemId2, gemBonuses2, gemContext2, gemScalingLevel2, gemItemId3, gemBonuses3, gemContext3, gemScalingLevel3,
    //48                 49                      50            51                   52             53
    //fixedScalingLevel, artifactKnowledgeLevel, xp, artifactAppearanceId, dungeonEncounterID, contextID FROM item_instance

    // create item before any checks for store correct guid
    // and allow use "FSetState(ITEM_REMOVED); SaveToDB();" for deleting item from DB
    Object::_Create(ObjectGuid::Create<HighGuid::Item>(guid));

    // Set entry, MUST be before proto check
    SetEntry(entry);
    SetObjectScale(1.0f);
    SetOwnerLevel(oLevel);

    ItemTemplate const* proto = GetTemplate();
    if (!proto)
        return false;

    _bonusData.Initialize(proto);

    // set owner (not if item is only loaded for gbank/auction/mail
    if (!owner_guid.IsEmpty())
        SetOwnerGUID(owner_guid);

    uint32 itemFlags = fields[7].GetUInt32();
    bool need_save = false;                                 // need explicit save data at load fixes
    if (uint64 creator = fields[2].GetUInt64())
    {
        if (!(itemFlags & ITEM_FLAG_CHILD))
            SetGuidValue(ITEM_FIELD_CREATOR, ObjectGuid::Create<HighGuid::Player>(creator));
        else
            SetGuidValue(ITEM_FIELD_CREATOR, ObjectGuid::Create<HighGuid::Item>(creator));
    }
    if (uint64 giftCreator = fields[3].GetUInt64())
        SetGuidValue(ITEM_FIELD_GIFT_CREATOR, ObjectGuid::Create<HighGuid::Player>(giftCreator));
    SetCount(fields[4].GetUInt32());

    uint32 duration = fields[5].GetUInt32();
    SetUInt32Value(ITEM_FIELD_EXPIRATION, duration);
    // update duration if need, and remove if not need
    if ((proto->GetDurationInInventory() == 0) != (duration == 0))
    {
        if (proto->GetFlags() & ITEM_FLAG_REAL_DURATION)
        {
            if (proto->GetDurationInInventory() != 0)
                SetUInt32Value(ITEM_FIELD_EXPIRATION, proto->GetDurationInInventory());
        }
        else
            SetUInt32Value(ITEM_FIELD_EXPIRATION, proto->GetDurationInInventory());
        need_save = true;
    }

    Tokenizer tokens(fields[6].GetString(), ' ', proto->Effects.size());
    if (tokens.size() == proto->Effects.size())
        for (uint8 i = 0; i < proto->Effects.size(); ++i)
            SetSpellCharges(proto->Effects[i]->LegacySlotIndex, atoi(tokens[i]));

    SetUInt32Value(ITEM_FIELD_DYNAMIC_FLAGS, itemFlags);

    _LoadIntoDataField(fields[8].GetString(), ITEM_FIELD_ENCHANTMENT, MAX_ENCHANTMENT_SLOT * MAX_ENCHANTMENT_OFFSET);
    m_randomEnchantment.Type = ItemRandomEnchantmentType(fields[9].GetUInt8());
    m_randomEnchantment.Id = fields[10].GetUInt32();
    if (m_randomEnchantment.Type == ItemRandomEnchantmentType::Property)
        SetUInt32Value(ITEM_FIELD_RANDOM_PROPERTIES_ID, m_randomEnchantment.Id);
    else if (m_randomEnchantment.Type == ItemRandomEnchantmentType::Suffix)
    {
        SetInt32Value(ITEM_FIELD_RANDOM_PROPERTIES_ID, -int32(m_randomEnchantment.Id));
        UpdateItemSuffixFactor();
    }


    uint32 durability = fields[11].GetUInt16();
    SetUInt32Value(ITEM_FIELD_DURABILITY, durability);
    // update max durability (and durability) if need
    SetUInt32Value(ITEM_FIELD_MAX_DURABILITY, proto->MaxDurability);
    if (durability > proto->MaxDurability)
    {
        SetUInt32Value(ITEM_FIELD_DURABILITY, proto->MaxDurability);
        need_save = true;
    }

    SetUInt32Value(ITEM_FIELD_CREATE_PLAYED_TIME, fields[12].GetUInt32());
    SetText(fields[13].GetString());

    uint32 upgradeId = fields[14].GetUInt32();
    if (auto const& v = sObjectMgr->GetItemTemplate(entry))
    {
        if ((v->GetFlags3() & ITEM_FLAG3_ITEM_CAN_BE_UPGRADED) != 0)
        {
            ItemUpgradeEntry const* rulesetUpgrade = sItemUpgradeStore.LookupEntry(sDB2Manager.GetRulesetItemUpgrade(entry));
            ItemUpgradeEntry const* upgrade = sItemUpgradeStore.LookupEntry(upgradeId);
            if (!rulesetUpgrade || !upgrade || rulesetUpgrade->ItemUpgradePathID != upgrade->ItemUpgradePathID)
                upgradeId = 0;

            if (rulesetUpgrade && !upgradeId)
                upgradeId = rulesetUpgrade->ID;
        }
        else
            upgradeId = 0;
    }      

    SetModifier(ITEM_MODIFIER_UPGRADE_ID, upgradeId);

    SetModifier(ITEM_MODIFIER_BATTLE_PET_SPECIES_ID, fields[15].GetUInt32());
    SetModifier(ITEM_MODIFIER_BATTLE_PET_BREED_DATA, fields[16].GetUInt32());
    SetModifier(ITEM_MODIFIER_BATTLE_PET_LEVEL, fields[17].GetUInt16());
    SetModifier(ITEM_MODIFIER_BATTLE_PET_DISPLAY_ID, fields[18].GetUInt32());

    Tokenizer tokenListIDs(fields[19].GetString(), ' ');
    std::vector<uint32> bonusListIDs;
    for (char const* token : tokenListIDs)
        bonusListIDs.push_back(uint32(atoul(token)));
    sObjectMgr->DeleteBugBonus(bonusListIDs, proto->HasEnchantment(), proto->GetQuality() < ITEM_QUALITY_LEGENDARY, proto->HasStats());
    for (auto& token : bonusListIDs)
        AddBonuses(token);

    SetModifier(ITEM_MODIFIER_TRANSMOG_APPEARANCE_ALL_SPECS, fields[20].GetUInt32());
    SetModifier(ITEM_MODIFIER_TRANSMOG_APPEARANCE_SPEC_1, fields[21].GetUInt32());
    SetModifier(ITEM_MODIFIER_TRANSMOG_APPEARANCE_SPEC_2, fields[22].GetUInt32());
    SetModifier(ITEM_MODIFIER_TRANSMOG_APPEARANCE_SPEC_3, fields[23].GetUInt32());
    SetModifier(ITEM_MODIFIER_TRANSMOG_APPEARANCE_SPEC_4, fields[24].GetUInt32());

    SetModifier(ITEM_MODIFIER_ENCHANT_ILLUSION_ALL_SPECS, fields[25].GetUInt32());
    SetModifier(ITEM_MODIFIER_ENCHANT_ILLUSION_SPEC_1, fields[26].GetUInt32());
    SetModifier(ITEM_MODIFIER_ENCHANT_ILLUSION_SPEC_2, fields[27].GetUInt32());
    SetModifier(ITEM_MODIFIER_ENCHANT_ILLUSION_SPEC_3, fields[28].GetUInt32());
    SetModifier(ITEM_MODIFIER_ENCHANT_ILLUSION_SPEC_4, fields[29].GetUInt32());

    uint32 const gemFields = 4;
    ItemDynamicFieldGems gemData[MAX_GEM_SOCKETS];
    memset(gemData, 0, sizeof(gemData));
    for (uint32 i = 0; i < MAX_GEM_SOCKETS; ++i)
    {
        gemData[i].ItemId = fields[30 + i * gemFields].GetUInt32();
        Tokenizer gemBonusListIDs(fields[31 + i * gemFields].GetString(), ' ');
        uint32 b = 0;
        for (char const* token : gemBonusListIDs)
            if (uint32 bonusListID = atoul(token))
                gemData[i].BonusListIDs[b++] = bonusListID;

        gemData[i].Context = fields[32 + i * gemFields].GetUInt8();
        if (gemData[i].ItemId)
            SetGem(i, &gemData[i], fields[33 + i * gemFields].GetUInt32());
    }

    SetModifier(ITEM_MODIFIER_SCALING_STAT_DISTRIBUTION_FIXED_LEVEL, fields[42].GetUInt32());
    SetModifier(ITEM_MODIFIER_ARTIFACT_KNOWLEDGE_LEVEL, fields[43].GetUInt32());

    SetUInt64Value(ITEM_FIELD_ARTIFACT_XP, fields[44].GetUInt64());
    SetModifier(ITEM_MODIFIER_ARTIFACT_APPEARANCE_ID, fields[45].GetUInt32());
    if (ArtifactAppearanceEntry const* artifactAppearance = sArtifactAppearanceStore.LookupEntry(fields[45].GetUInt32()))
        SetAppearanceModId(artifactAppearance->ItemAppearanceModifierID);

    SetModifier(ITEM_MODIFIER_ARTIFACT_TIER, fields[46].GetUInt32());
    dungeonEncounterID = fields[47].GetUInt32();
    SetUInt32Value(ITEM_FIELD_CONTEXT, fields[48].GetUInt8());
    createdTime = fields[49].GetUInt32();

    // Remove bind flag for items vs NO_BIND set
    if (IsSoulBound() && GetBonding() == NO_BIND)
    {
        ApplyModFlag(ITEM_FIELD_DYNAMIC_FLAGS, ITEM_FLAG_SOULBOUND, false);
        need_save = true;
    }

    if (need_save)                                           // normal item changed state set not work at loading
    {
        CharacterDatabasePreparedStatement* stmt = CharacterDatabase.GetPreparedStatement(CHAR_UPD_ITEM_INSTANCE_ON_LOAD);
        stmt->setUInt32(0, GetUInt32Value(ITEM_FIELD_EXPIRATION));
        stmt->setUInt32(1, GetUInt32Value(ITEM_FIELD_DYNAMIC_FLAGS));
        stmt->setUInt32(2, GetUInt32Value(ITEM_FIELD_DURABILITY));
        stmt->setUInt64(3, guid);
        CharacterDatabase.Execute(stmt);
    }

    return true;
}

void Item::LoadArtifactData(Player* owner, std::vector<ItemDynamicFieldArtifactPowers>& powers)
{
    InitArtifactPowers(GetTemplate()->GetArtifactID());

    uint8 totalPurchasedRanks = 0;
    for (ItemDynamicFieldArtifactPowers& power : powers)
    {
        ArtifactPowerEntry const* artifactPower = sArtifactPowerStore.AssertEntry(power.ArtifactPowerId);

        uint32 maxRank = artifactPower->MaxPurchasableRank;
        if (GetModifier(ITEM_MODIFIER_ARTIFACT_TIER) && (artifactPower->Flags & ARTIFACT_POWER_FLAG_HAS_RANK) && !artifactPower->Tier)
            ++maxRank;
        if (maxRank < power.PurchasedRank)
            power.PurchasedRank = maxRank;

        power.CurrentRankWithBonus += power.PurchasedRank;
        totalPurchasedRanks += power.PurchasedRank;

        for (uint32 e = SOCK_ENCHANTMENT_SLOT; e <= SOCK_ENCHANTMENT_SLOT_3; ++e)
        {
            if (SpellItemEnchantmentEntry const* enchant = sSpellItemEnchantmentStore.LookupEntry(GetEnchantmentId(EnchantmentSlot(e))))
            {
                for (uint32 i = 0; i < MAX_ITEM_ENCHANTMENT_EFFECTS; ++i)
                {
                    switch (enchant->Effect[i])
                    {
                        case ITEM_ENCHANTMENT_TYPE_ARTIFACT_POWER_BONUS_RANK_BY_TYPE:
                            if (artifactPower->Label == enchant->EffectArg[i])
                                power.CurrentRankWithBonus += enchant->EffectPointsMin[i];
                            break;
                        case ITEM_ENCHANTMENT_TYPE_ARTIFACT_POWER_BONUS_RANK_BY_ID:
                            if (artifactPower->ID == enchant->EffectArg[i])
                                power.CurrentRankWithBonus += enchant->EffectPointsMin[i];
                            break;
                        case ITEM_ENCHANTMENT_TYPE_ARTIFACT_POWER_BONUS_RANK_PICKER:
                            if (_bonusData.GemRelicType[e - SOCK_ENCHANTMENT_SLOT] != -1)
                                if (auto artifactPowerPicker = sArtifactPowerPickerStore.LookupEntry(enchant->EffectArg[i]))
                                    if (sConditionMgr->IsPlayerMeetingCondition(owner, artifactPowerPicker->PlayerConditionID))
                                        if (artifactPower->Label == _bonusData.GemRelicType[e - SOCK_ENCHANTMENT_SLOT])
                                            power.CurrentRankWithBonus += enchant->EffectPointsMin[i];
                            break;
                        default:
                            break;
                    }
                }
            }
        }

        SetArtifactPower(&power);
    }

    for (ItemDynamicFieldArtifactPowers& power : powers)
    {
        ArtifactPowerEntry const* scaledArtifactPowerEntry = sArtifactPowerStore.AssertEntry(power.ArtifactPowerId);
        if (!(scaledArtifactPowerEntry->Flags & ARTIFACT_POWER_FLAG_SCALES_WITH_NUM_POWERS) || scaledArtifactPowerEntry->Tier)
            continue;

        power.CurrentRankWithBonus = totalPurchasedRanks + 1;
        SetArtifactPower(&power);
    }
}

/*static*/
void Item::DeleteFromDB(CharacterDatabaseTransaction& trans, ObjectGuid::LowType itemGuid)
{
    CharacterDatabasePreparedStatement* stmt = CharacterDatabase.GetPreparedStatement(CHAR_DEL_ITEM_INSTANCE);
    stmt->setUInt64(0, itemGuid);
    trans->Append(stmt);
}

void Item::DeleteFromDB(CharacterDatabaseTransaction& trans)
{
    DeleteFromDB(trans, GetGUIDLow());
}

/*static*/
void Item::DeleteFromInventoryDB(CharacterDatabaseTransaction& trans, ObjectGuid::LowType itemGuid)
{
    CharacterDatabasePreparedStatement* stmt = CharacterDatabase.GetPreparedStatement(CHAR_DEL_CHAR_INVENTORY_BY_ITEM);
    stmt->setUInt64(0, itemGuid);
    trans->Append(stmt);
}

void Item::DeleteFromInventoryDB(CharacterDatabaseTransaction& trans)
{
    DeleteFromInventoryDB(trans, GetGUIDLow());
}

ItemTemplate const* Item::GetTemplate() const
{
    return sObjectMgr->GetItemTemplate(GetEntry());
}

Player* Item::GetOwner()const
{
    return ObjectAccessor::FindPlayer(GetOwnerGUID());
}

ItemBondingType Item::GetBonding() const
{
    return _bonusData.Bonding;
}

bool Item::IsSturdiness() const
{
    return _bonusData.HasSturdiness;
}

uint32 Item::GetSkill()
{
    const static uint32 item_weapon_skills[MAX_ITEM_SUBCLASS_WEAPON] =
    {
        SKILL_AXES,     SKILL_2H_AXES,  SKILL_BOWS,          SKILL_GUNS,            SKILL_MACES,
        SKILL_2H_MACES, SKILL_POLEARMS, SKILL_SWORDS,        SKILL_2H_SWORDS,       SKILL_WARGLAIVES,
        SKILL_STAVES,   0,              0,                   SKILL_FIST_WEAPONS,    0,
        SKILL_DAGGERS,  SKILL_THROWN,   SKILL_ASSASSINATION, SKILL_CROSSBOWS,       SKILL_WANDS,
        SKILL_FISHING
    };

    const static uint32 item_armor_skills[MAX_ITEM_SUBCLASS_ARMOR] =
    {
        0, SKILL_CLOTH, SKILL_LEATHER, SKILL_MAIL, SKILL_PLATE_MAIL, 0, SKILL_SHIELD, 0, 0, 0, 0
    };

    ItemTemplate const* proto = GetTemplate();

    switch (proto->GetClass())
    {
        case ITEM_CLASS_WEAPON:
        {
            if (proto->GetSubClass() >= MAX_ITEM_SUBCLASS_WEAPON)
                return 0;
            return item_weapon_skills[proto->GetSubClass()];
        }

        case ITEM_CLASS_ARMOR:
        {
            if (proto->GetSubClass() >= MAX_ITEM_SUBCLASS_ARMOR)
                return 0;
            return item_armor_skills[proto->GetSubClass()];
        }

        default:
            return 0;
    }
}

int32 Item::GetItemRandomPropertyId() const
{
    return GetInt32Value(ITEM_FIELD_RANDOM_PROPERTIES_ID);
}

uint32 Item::GetItemSuffixFactor() const
{
    return GetUInt32Value(ITEM_FIELD_PROPERTY_SEED);
}

ItemRandomEnchantmentId Item::GenerateItemRandomPropertyId(uint32 item_id, uint32 spec_id)
{
    ItemTemplate const* itemProto = sObjectMgr->GetItemTemplate(item_id);
    if (!itemProto || !itemProto->HasEnchantment())
        return{};

    if (itemProto->GetRandomSelect())
        return GetItemEnchantMod(itemProto->GetRandomSelect(), ItemRandomEnchantmentType::Property, item_id, spec_id);
    return GetItemEnchantMod(itemProto->GetItemRandomSuffixGroupID(), ItemRandomEnchantmentType::Suffix, item_id, spec_id);
}

ItemRandomEnchantmentId Item::GetItemRandomEnchantmentId() const
{
    return m_randomEnchantment;
}

void Item::SetItemRandomProperties(ItemRandomEnchantmentId const& randomPropId)
{
    if (!randomPropId.Id)
        return;

    switch (randomPropId.Type)
    {
        case ItemRandomEnchantmentType::Property:
            if (auto const& entry = sItemRandomPropertiesStore.LookupEntry(randomPropId.Id))
            {
                if (GetUInt32Value(ITEM_FIELD_RANDOM_PROPERTIES_ID) != randomPropId.Id)
                {
                    SetUInt32Value(ITEM_FIELD_RANDOM_PROPERTIES_ID, randomPropId.Id);
                    SetState(ITEM_CHANGED, GetOwner());
                }
                for (uint32 i = PROP_ENCHANTMENT_SLOT_0; i <= PROP_ENCHANTMENT_SLOT_4; ++i)
                    SetEnchantment(EnchantmentSlot(i), entry->Enchantment[i - PROP_ENCHANTMENT_SLOT_0], 0, 0);
            }
            m_randomEnchantment = randomPropId;
            break;
        case ItemRandomEnchantmentType::Suffix:
            if (auto const& entry = sItemRandomSuffixStore.LookupEntry(randomPropId.Id))
            {
                if (GetInt32Value(ITEM_FIELD_RANDOM_PROPERTIES_ID) != -int32(randomPropId.Id) || !GetItemSuffixFactor())
                {
                    SetInt32Value(ITEM_FIELD_RANDOM_PROPERTIES_ID, -int32(randomPropId.Id));
                    UpdateItemSuffixFactor();
                    SetState(ITEM_CHANGED, GetOwner());
                }

                for (uint32 i = PROP_ENCHANTMENT_SLOT_0; i <= PROP_ENCHANTMENT_SLOT_4; ++i)
                    SetEnchantment(EnchantmentSlot(i), entry->Enchantment[i - PROP_ENCHANTMENT_SLOT_0], 0, 0);
            }
            m_randomEnchantment = randomPropId;
            break;
        case ItemRandomEnchantmentType::BonusList:
        case ItemRandomEnchantmentType::BonusListAddition:
            AddBonuses(randomPropId.Id);
            break;
        default:
            break;
    }
}

void Item::UpdateItemSuffixFactor()
{
    ItemTemplate const* proto = GetTemplate();
    uint32 suffixFactor = proto && proto->GetItemRandomSuffixGroupID() ? GenerateEnchSuffixFactor(GetTemplate()) : 0;
    if (GetItemSuffixFactor() == suffixFactor)
        return;
    SetUInt32Value(ITEM_FIELD_PROPERTY_SEED, suffixFactor);
}

ItemUpdateState Item::GetState() const
{
    return uState;
}

void Item::SetState(ItemUpdateState state, Player* forplayer)
{
    if (uState == ITEM_NEW && state == ITEM_REMOVED)
    {
        // pretend the item never existed
        RemoveFromUpdateQueueOf(forplayer);
        forplayer->AddDeleteItem(this);
        // delete this;
        return;
    }
    if (state != ITEM_UNCHANGED)
    {
        // new items must stay in new state until saved
        if (uState != ITEM_NEW)
            uState = state;

        AddToUpdateQueueOf(forplayer);
    }
    else
    {
        // unset in queue
        // the item must be removed from the queue manually
        uQueuePos = -1;
        uState = ITEM_UNCHANGED;
    }
}

void Item::FSetState(ItemUpdateState state)               // forced
{
    if (state == ITEM_REMOVED)
        RemoveFromWorld();

    uState = state;
}

bool Item::hasQuest(uint32 quest_id) const
{
    return GetTemplate()->GetStartQuestID() == quest_id;
}

bool Item::hasInvolvedQuest(uint32) const
{
    return false;
}

void Item::AddToUpdateQueueOf(Player* player)
{
    if (IsInUpdateQueue())
        return;

    if (!player)
        return;

    if (player->GetGUID() != GetOwnerGUID())
    {
        TC_LOG_DEBUG("entities.player.items", "Item::AddToUpdateQueueOf - Owner's guid (%u) and player's guid (%u) don't match!", GetOwnerGUID().GetGUIDLow(), player->GetGUIDLow());
        return;
    }

    if (player->m_itemUpdateQueueBlocked)
        return;

    player->m_itemUpdateQueue.insert(this);
    uQueuePos = player->m_itemUpdateQueue.size() - 1;
}

void Item::RemoveFromUpdateQueueOf(Player* player)
{
    if (!IsInUpdateQueue())
        return;

    //ASSERT(player != NULL);
    if (!player)
        return;

    if (player->GetGUID() != GetOwnerGUID())
    {
        TC_LOG_DEBUG("entities.player.items", "Item::RemoveFromUpdateQueueOf - Owner's guid (%u) and player's guid (%u) don't match!", GetOwnerGUID().GetGUIDLow(), player->GetGUIDLow());
        return;
    }

    if (player->m_itemUpdateQueueBlocked)
        return;

    player->m_itemUpdateQueue.erase(this);
    uQueuePos = -1;
}

bool Item::IsInUpdateQueue() const
{
    return uQueuePos != -1;
}

uint16 Item::GetQueuePos() const
{
    return uQueuePos;
}

uint8 Item::GetBagSlot() const
{
    return m_container ? m_container->GetSlot() : uint8(INVENTORY_SLOT_BAG_0);
}

void Item::SetSlot(uint8 slot)
{
    m_slot = slot;
}

uint16 Item::GetPos() const
{
    return uint16(GetBagSlot()) << 8 | GetSlot();
}

void Item::SetContainer(Bag* container)
{
    m_container = container;
}

bool Item::IsInBag() const
{
    return m_container != nullptr;
}

bool Item::IsEquipped() const
{
    return !IsInBag() && m_slot < EQUIPMENT_SLOT_END;
}

bool Item::CanBeTraded(bool mail, bool trade) const
{
    if (m_lootGenerated)
        return false;

    if ((!mail || !IsBoundAccountWide()) && (IsSoulBound() && (!HasFlag(ITEM_FIELD_DYNAMIC_FLAGS, ITEM_FLAG_BOP_TRADEABLE) || !trade)))
        return false;

    if (IsBag() && (Player::IsBagPos(GetPos()) || !ToBag()->IsEmpty()))
        return false;

    if (Player* owner = GetOwner())
    {
        if (owner->CanUnequipItem(GetPos(), false) != EQUIP_ERR_OK)
            return false;
        if (owner->GetLootGUID() == GetGUID())
            return false;
    }

    if (IsBoundByEnchant())
        return false;

    return true;
}

void Item::SetInTrade(bool b)
{
    mb_in_trade = b;
}

bool Item::IsInTrade() const
{
    return mb_in_trade;
}

void Item::SetInUse(bool u)
{
    m_in_use = u;
}

bool Item::IsInUse() const
{
    return m_in_use;
}

bool Item::HasEnchantRequiredSkill(Player const* player) const
{
    for (uint32 i = PERM_ENCHANTMENT_SLOT; i < MAX_ENCHANTMENT_SLOT; ++i)
        if (uint32 id = GetEnchantmentId(EnchantmentSlot(i)))
            if (SpellItemEnchantmentEntry const* enchantEntry = sSpellItemEnchantmentStore.LookupEntry(id))
                if (enchantEntry->RequiredSkillID && const_cast<Player*>(player)->GetSkillValue(enchantEntry->RequiredSkillID) < enchantEntry->RequiredSkillRank)
                    return false;

    return true;
}

uint32 Item::GetEnchantRequiredLevel() const
{
    uint32 level = 0;

    for (uint32 i = PERM_ENCHANTMENT_SLOT; i < MAX_ENCHANTMENT_SLOT; ++i)
        if (uint32 id = GetEnchantmentId(EnchantmentSlot(i)))
            if (SpellItemEnchantmentEntry const* enchantEntry = sSpellItemEnchantmentStore.LookupEntry(id))
                if (enchantEntry->MinLevel > level)
                    level = enchantEntry->MinLevel;

    return level;
}

bool Item::IsBoundByEnchant() const
{
    for (uint32 i = PERM_ENCHANTMENT_SLOT; i < MAX_ENCHANTMENT_SLOT; ++i)
        if (uint32 id = GetEnchantmentId(EnchantmentSlot(i)))
            if (SpellItemEnchantmentEntry const* enchantEntry = sSpellItemEnchantmentStore.LookupEntry(id))
                if (enchantEntry->Flags & ENCHANTMENT_CAN_SOULBOUND)
                    return true;

    return false;
}

InventoryResult Item::CanBeMergedPartlyWith(ItemTemplate const* proto) const
{
    // not allow merge looting currently items
    if (m_lootGenerated)
        return EQUIP_ERR_LOOT_GONE;

    // check item type
    if (GetEntry() != proto->GetId())
        return EQUIP_ERR_CANT_STACK;

    // check free space (full stacks can't be target of merge
    if (GetCount() >= proto->GetMaxStackSize())
        return EQUIP_ERR_CANT_STACK;

    return EQUIP_ERR_OK;
}

bool Item::IsFitToSpellRequirements(SpellInfo const* spellInfo) const
{
    ItemTemplate const* proto = GetTemplate();

    bool isEnchantSpell = spellInfo->HasEffect(SPELL_EFFECT_ENCHANT_ITEM) || spellInfo->HasEffect(SPELL_EFFECT_ENCHANT_ITEM_TEMPORARY) || spellInfo->HasEffect(SPELL_EFFECT_ENCHANT_ITEM_PRISMATIC);
    if (spellInfo->EquippedItemClass != -1)                 // -1 == any item class
    {
        if (isEnchantSpell && proto->GetFlags3() & ITEM_FLAG3_CAN_STORE_ENCHANTS)
            return true;

        if (spellInfo->EquippedItemClass != int32(proto->GetClass()))
            return false;                                   //  wrong item class

        if (spellInfo->EquippedItemSubClassMask != 0)        // 0 == any subclass
        {
            if ((spellInfo->EquippedItemSubClassMask & (1 << proto->GetSubClass())) == 0)
                return false;                               // subclass not present in mask
        }
    }

    if (isEnchantSpell && spellInfo->EquippedItemInventoryTypeMask != 0)       // 0 == any inventory type
    {
        // Special case - accept weapon type for main and offhand requirements
        if ((proto->GetInventoryType() == INVTYPE_WEAPON || proto->GetInventoryType() == INVTYPE_2HWEAPON) &&
            (spellInfo->EquippedItemInventoryTypeMask & (1 << INVTYPE_WEAPONMAINHAND) ||
            spellInfo->EquippedItemInventoryTypeMask & (1 << INVTYPE_WEAPONOFFHAND)))
            return true;
        if ((spellInfo->EquippedItemInventoryTypeMask & (1 << proto->GetInventoryType())) == 0)
            return false;
        // inventory type not present in mask
    }

    return true;
}

void Item::SetEnchantment(EnchantmentSlot slot, uint32 id, uint32 duration, uint32 charges, ObjectGuid caster /*= ObjectGuid::Empty*/)
{
    // Better lost small time at check in comparison lost time at item save to DB.
    if ((GetEnchantmentId(slot) == id) && (GetEnchantmentDuration(slot) == duration) && (GetEnchantmentCharges(slot) == charges))
        return;

    Player* owner = GetOwner();
    if (slot < MAX_INSPECTED_ENCHANTMENT_SLOT)
    {
        if (uint32 oldEnchant = GetEnchantmentId(slot))
            SendEnchantmentLog(owner, ObjectGuid::Empty, GetGUID(), GetEntry(), oldEnchant, slot);

        if (id)
            SendEnchantmentLog(owner, caster, GetGUID(), GetEntry(), id, slot);
    }

    ApplyArtifactPowerEnchantmentBonuses(slot, GetEnchantmentId(slot), false, owner);
    ApplyArtifactPowerEnchantmentBonuses(slot, id, true, owner);

    SetUInt32Value(ITEM_FIELD_ENCHANTMENT + slot * MAX_ENCHANTMENT_OFFSET + ENCHANTMENT_ID_OFFSET, id);
    SetUInt32Value(ITEM_FIELD_ENCHANTMENT + slot * MAX_ENCHANTMENT_OFFSET + ENCHANTMENT_DURATION_OFFSET, duration);
    SetUInt32Value(ITEM_FIELD_ENCHANTMENT + slot * MAX_ENCHANTMENT_OFFSET + ENCHANTMENT_CHARGES_OFFSET, charges);
    ForceValuesUpdateAtIndex(ITEM_FIELD_ENCHANTMENT + slot * MAX_ENCHANTMENT_OFFSET + ENCHANTMENT_ID_OFFSET);
    ForceValuesUpdateAtIndex(ITEM_FIELD_ENCHANTMENT + slot * MAX_ENCHANTMENT_OFFSET + ENCHANTMENT_DURATION_OFFSET);
    ForceValuesUpdateAtIndex(ITEM_FIELD_ENCHANTMENT + slot * MAX_ENCHANTMENT_OFFSET + ENCHANTMENT_CHARGES_OFFSET);

    SetState(ITEM_CHANGED, owner);
}

void Item::SendEnchantmentLog(Player* player, ObjectGuid const& caster, ObjectGuid const& item, uint32 itemID, uint32 spellID, EnchantmentSlot slot)
{
    WorldPackets::Item::EnchantmentLog log;
    log.Caster = caster;
    log.Owner = player->GetGUID();
    log.ItemGUID = item;
    log.ItemID = itemID;
    log.EnchantSlot = spellID;
    log.Enchantment = slot;
    player->SendDirectMessage(log.Write());
}

void Item::SendItemEnchantTimeUpdate(Player* player, ObjectGuid const& Itemguid, uint32 slot, uint32 duration)
{
    WorldPackets::Item::ItemEnchantTimeUpdate timeUpdate;
    timeUpdate.ItemGuid = Itemguid;
    timeUpdate.PlayerGuid = player->GetGUID();
    timeUpdate.Duration = duration;
    timeUpdate.Slot = slot;
    player->SendDirectMessage(timeUpdate.Write());
}

void Item::SetEnchantmentDuration(EnchantmentSlot slot, uint32 duration, Player* owner)
{
    if (GetEnchantmentDuration(slot) == duration)
        return;

    SetUInt32Value(ITEM_FIELD_ENCHANTMENT + slot*MAX_ENCHANTMENT_OFFSET + ENCHANTMENT_DURATION_OFFSET, duration);
    SetState(ITEM_CHANGED, owner);
    // Cannot use GetOwner() here, has to be passed as an argument to avoid freeze due to hashtable locking
}

void Item::SetEnchantmentCharges(EnchantmentSlot slot, uint32 charges)
{
    if (GetEnchantmentCharges(slot) == charges)
        return;

    SetUInt32Value(ITEM_FIELD_ENCHANTMENT + slot*MAX_ENCHANTMENT_OFFSET + ENCHANTMENT_CHARGES_OFFSET, charges);
    SetState(ITEM_CHANGED, GetOwner());
}

void Item::ClearEnchantment(EnchantmentSlot slot)
{
    if (!GetEnchantmentId(slot))
        return;

    for (uint8 x = 0; x < MAX_ITEM_ENCHANTMENT_EFFECTS; ++x)
        SetUInt32Value(ITEM_FIELD_ENCHANTMENT + slot*MAX_ENCHANTMENT_OFFSET + x, 0);
    SetState(ITEM_CHANGED, GetOwner());
}

uint32 Item::GetEnchantmentId(EnchantmentSlot slot) const
{
    return GetUInt32Value(ITEM_FIELD_ENCHANTMENT + slot * MAX_ENCHANTMENT_OFFSET + ENCHANTMENT_ID_OFFSET);
}

uint32 Item::GetEnchantmentDuration(EnchantmentSlot slot) const
{
    return GetUInt32Value(ITEM_FIELD_ENCHANTMENT + slot * MAX_ENCHANTMENT_OFFSET + ENCHANTMENT_DURATION_OFFSET);
}

uint32 Item::GetEnchantmentCharges(EnchantmentSlot slot) const
{
    return GetUInt32Value(ITEM_FIELD_ENCHANTMENT + slot * MAX_ENCHANTMENT_OFFSET + ENCHANTMENT_CHARGES_OFFSET);
}

std::string const& Item::GetText() const
{
    return m_text;
}

void Item::SetText(std::string const& text)
{
    m_text = text;
}

DynamicFieldStructuredView<ItemDynamicFieldGems> Item::GetGems() const
{
    if (ITEM_DYNAMIC_FIELD_GEMS >= GetDynamicValuesCount())
        return DynamicFieldStructuredView<ItemDynamicFieldGems>(std::vector<uint32>());
    return GetDynamicStructuredValues<ItemDynamicFieldGems>(ITEM_DYNAMIC_FIELD_GEMS);
}

std::map<uint8, ItemSocketInfo> Item::GetArtifactSockets() const
{
    std::map<uint8, ItemSocketInfo> result{};
    auto values = GetDynamicValues(ITEM_DYNAMIC_FIELD_RELIC_TALENT_DATA);
    if (values.size() < 6 || values.size() % 6 != 0)
        return result;
 
    uint8 i = 0;
    while (i < values.size())
    {
        ItemSocketInfo info;
        info.unk1 = values[i++];
        info.socketIndex = values[i++];
        info.firstTier = values[i++];
        info.secondTier = values[i++];
        info.thirdTier = values[i++];
        info.additionalThirdTier = values[i++];

        result[info.socketIndex] = info;
    }

    return result;
}

ItemDynamicFieldGems const* Item::GetGem(uint16 slot) const
{
    ASSERT(slot < MAX_GEM_SOCKETS);
    return GetDynamicStructuredValue<ItemDynamicFieldGems>(ITEM_DYNAMIC_FIELD_GEMS, slot);
}

void Item::SetGem(uint16 slot, ItemDynamicFieldGems const* gem, uint32 gemScalingLevel)
{
    ASSERT(slot < MAX_GEM_SOCKETS);
    m_gemScalingLevels[slot] = gemScalingLevel;
    _bonusData.GemItemLevelBonus[slot] = 0;

    if (ItemTemplate const* gemTemplate = sObjectMgr->GetItemTemplate(gem->ItemId))
    {
        if (GemPropertiesEntry const* gemProperties = sGemPropertiesStore.LookupEntry(gemTemplate->GetGemProperties()))
        {
            if (SpellItemEnchantmentEntry const* gemEnchant = sSpellItemEnchantmentStore.LookupEntry(gemProperties->EnchantID))
            {
                BonusData gemBonus;
                gemBonus.Initialize(gemTemplate);
                for (uint16 bonusListId : gem->BonusListIDs)
                    if (DB2Manager::ItemBonusList const* bonuses = sDB2Manager.GetItemBonusList(bonusListId))
                        for (ItemBonusEntry const* itemBonus : *bonuses)
                            gemBonus.AddBonus(itemBonus->Type, itemBonus->Value);

                uint32 gemBaseItemLevel = gemTemplate->GetBaseItemLevel();
                if (ScalingStatDistributionEntry const* ssd = sScalingStatDistributionStore.LookupEntry(gemBonus.ScalingStatDistribution))
                    if (auto scaledIlvl = uint32(sDB2Manager.GetCurveValueAt(ssd->PlayerLevelToItemLevelCurveID, gemScalingLevel)))
                        gemBaseItemLevel = scaledIlvl;

                _bonusData.GemRelicType[slot] = gemBonus.RelicType;

                for (uint32 i = 0; i < MAX_ITEM_ENCHANTMENT_EFFECTS; ++i)
                {
                    switch (gemEnchant->Effect[i])
                    {
                        case ITEM_ENCHANTMENT_TYPE_BONUS_LIST_ID:
                            if (DB2Manager::ItemBonusList const* bonuses = sDB2Manager.GetItemBonusList(gemEnchant->EffectArg[i]))
                                for (ItemBonusEntry const* itemBonus : *bonuses)
                                    if (itemBonus->Type == ITEM_BONUS_ITEM_LEVEL)
                                        _bonusData.GemItemLevelBonus[slot] += itemBonus->Value[0];
                            break;
                        case ITEM_ENCHANTMENT_TYPE_BONUS_LIST_CURVE:
                            if (uint32 bonusListId = sDB2Manager.GetItemBonusListForItemLevelDelta(int16(sDB2Manager.GetCurveValueAt(CURVE_ID_ARTIFACT_RELIC_ITEM_LEVEL_BONUS, gemBaseItemLevel + gemBonus.ItemLevelBonus))))
                                if (DB2Manager::ItemBonusList const* bonuses = sDB2Manager.GetItemBonusList(bonusListId))
                                    for (ItemBonusEntry const* itemBonus : *bonuses)
                                        if (itemBonus->Type == ITEM_BONUS_ITEM_LEVEL)
                                            _bonusData.GemItemLevelBonus[slot] += itemBonus->Value[0];
                            break;
                        default:
                            break;
                    }
                }
            }
        }
    }

    SetDynamicStructuredValue(ITEM_DYNAMIC_FIELD_GEMS, slot, gem);
}

void Item::CreateSocketTalents(uint8 socketIndex)
{
    std::set<uint32> bannedLables{};
    uint32 itemId = 0;
    if (GetTemplate()->GetArtifactID())
    {
        auto gems = GetGems();
        if (gems.size() > (socketIndex - 2))
            itemId = gems[socketIndex - 2]->ItemId;
    }
    else
        itemId = GetEntry();

    if (itemId)
        if (ItemTemplate const* gemTemplate = sObjectMgr->GetItemTemplate(itemId))
            if (GemPropertiesEntry const* gemProperties = sGemPropertiesStore.LookupEntry(gemTemplate->GetGemProperties()))
                if (SpellItemEnchantmentEntry const* gemEnchant = sSpellItemEnchantmentStore.LookupEntry(gemProperties->EnchantID))
                    for (auto labelId : gemEnchant->EffectArg)
                        if (labelId)
                            bannedLables.insert(labelId);

    SetState(ITEM_CHANGED, GetOwner());

    uint8 offset = (socketIndex - 2) * 6;
    SetDynamicValue(ITEM_DYNAMIC_FIELD_RELIC_TALENT_DATA, offset++, 1);
    SetDynamicValue(ITEM_DYNAMIC_FIELD_RELIC_TALENT_DATA, offset++, socketIndex);
    SetDynamicValue(ITEM_DYNAMIC_FIELD_RELIC_TALENT_DATA, offset++, 65536);

    std::vector<uint32> darkSpells{};
    std::vector<uint32> holySpells{};
    std::vector<uint32> thirdTierSpells{};
    for (auto itr = sRelicTalentStore.begin(); itr != sRelicTalentStore.end(); ++itr)
    {
        switch ((*itr)->Type)
        {
        case 1:
            darkSpells.push_back((*itr)->ID);
            break;
        case 2:
            holySpells.push_back((*itr)->ID);
            break;
        case 3:
            if (bannedLables.find((*itr)->ArtifactPowerLabel) == bannedLables.end())
                thirdTierSpells.push_back((*itr)->ID);
            break;
        }
    }
    SocketTier secondTier(darkSpells[urand(0, darkSpells.size() - 1)], holySpells[urand(0, holySpells.size() - 1)] );

    SetDynamicStructuredValue(ITEM_DYNAMIC_FIELD_RELIC_TALENT_DATA, offset++, &secondTier);

    Trinity::Containers::RandomResizeList(thirdTierSpells, 3);
    SocketTier thirdTier( thirdTierSpells[0], thirdTierSpells[1] );

    SetDynamicStructuredValue(ITEM_DYNAMIC_FIELD_RELIC_TALENT_DATA, offset++, &thirdTier);
    SetDynamicValue(ITEM_DYNAMIC_FIELD_RELIC_TALENT_DATA, offset++, thirdTierSpells[2]);
}

void Item::AddOrRemoveSocketTalent(uint8 talentIndex, bool add, uint8 socketIndex)
{
    uint32 powerId = 0;

    auto relicks = GetArtifactSockets();
    if (relicks.find(socketIndex) == relicks.end()) // can it?
        return;

    if (talentIndex == 0)
        powerId = 1739;
    else
    {
        uint32 talentId = 0;
        if (talentIndex != 5)
        {
            SocketTier tier;
            if (talentIndex == 1 || talentIndex == 2)
                tier = *reinterpret_cast<SocketTier*>(&relicks[socketIndex].secondTier);
            else
                tier = *reinterpret_cast<SocketTier*>(&relicks[socketIndex].thirdTier);

            talentId = talentIndex % 2 == 1 ? tier.FirstSpell : tier.SecondSpell;
        }
        else
            talentId = relicks[socketIndex].additionalThirdTier;

        auto relicTalent = sRelicTalentStore.LookupEntry(talentId);
        if (!relicTalent)
            return;

        if (relicTalent->ArtifactPowerID != 0)
            powerId = relicTalent->ArtifactPowerID;
        else
        {
            auto artifactPowers = sDB2Manager.GetArtifactPowers(GetTemplate()->GetArtifactID());
            for (ArtifactPowerEntry const* artifactPower : artifactPowers)
            {
                if (artifactPower->Label == relicTalent->ArtifactPowerLabel)
                {
                    powerId = artifactPower->ID;
                    break;
                }
            }
        }
    }
    if (!powerId)
        return;

    ItemDynamicFieldArtifactPowers const* artifactPower = GetArtifactPower(powerId);
    if (!artifactPower)
        return;

    ArtifactPowerEntry const* artifactPowerEntry = sArtifactPowerStore.LookupEntry(artifactPower->ArtifactPowerId);
    if (!artifactPowerEntry)
        return;

    uint8 rank = artifactPower->CurrentRankWithBonus + 1 - 1;

    if (rank && !add)
        rank -= 1;

    ArtifactPowerRankEntry const* artifactPowerRank = sDB2Manager.GetArtifactPowerRank(artifactPower->ArtifactPowerId, rank); // need data for next rank, but -1 because of how db2 data is structured
    if (!artifactPowerRank)
        return;

    ItemDynamicFieldArtifactPowers newPower = *artifactPower;
    newPower.CurrentRankWithBonus += add ? 1 : (newPower.CurrentRankWithBonus > 0 ? -1 : 0);
    SetArtifactPower(&newPower);

    Player* _player = GetOwner();
    if (_player)
        if (IsEquipped())
        {
            bool needAReqpply = GetModsApplied();
            if (needAReqpply)
                _player->_ApplyItemBonuses(this, GetSlot(), false);

            _player->ApplyArtifactPowerRank(this, artifactPowerRank, !!newPower.CurrentRankWithBonus);

            if (needAReqpply)
                _player->_ApplyItemBonuses(this, GetSlot(), true);
        }
}

uint8 Item::GetSlot() const
{
    return m_slot;
}

Bag* Item::GetContainer()
{
    return m_container;
}

bool Item::GemsFitSockets() const
{
    uint32 gemSlot = 0;
    for (ItemDynamicFieldGems const& gemData : GetGems())
    {
        uint8 SocketColor = GetTemplate()->GetSocketType(gemSlot);
        if (!SocketColor) // no socket slot
            continue;

        uint32 GemColor = 0;

        if (ItemTemplate const* gemProto = sObjectMgr->GetItemTemplate(gemData.ItemId))
            if (GemPropertiesEntry const* gemProperty = sGemPropertiesStore.LookupEntry(gemProto->GetGemProperties()))
                GemColor = gemProperty->Type;

        if (!(GemColor & SocketColorToGemTypeMask[SocketColor])) // bad gem color on this socket
            return false;
    }
    return true;
}

uint32 Item::GetCount() const
{
    return GetUInt32Value(ITEM_FIELD_STACK_COUNT);
}

void Item::SetCount(uint32 value)
{
    SetUInt32Value(ITEM_FIELD_STACK_COUNT, value);
}

uint32 Item::GetMaxStackCount() const
{
    return GetTemplate()->GetMaxStackSize();
}

uint8 Item::GetGemCountWithID(uint32 GemID) const
{
    return std::count_if(GetGems().begin(), GetGems().end(), [GemID] (ItemDynamicFieldGems const& gemData)
    {
        return gemData.ItemId == GemID;
    });
}

uint8 Item::GetGemCountWithLimitCategory(uint32 limitCategory) const
{
    return std::count_if(GetGems().begin(), GetGems().end(), [limitCategory] (ItemDynamicFieldGems const& gemData)
    {
        ItemTemplate const* gemProto = sObjectMgr->GetItemTemplate(gemData.ItemId);
        if (!gemProto)
            return false;

        return gemProto->GetLimitCategory() == limitCategory;
    });
}

bool Item::IsLimitedToAnotherMapOrZone(uint32 cur_mapId, uint32 cur_zoneId) const
{
    ItemTemplate const* proto = GetTemplate();
    return proto && ((proto->GetMap() && proto->GetMap() != cur_mapId) || (proto->GetArea() && proto->GetArea() != cur_zoneId));
}

void Item::SendTimeUpdate(Player* owner)
{
    uint32 duration = GetUInt32Value(ITEM_FIELD_EXPIRATION);
    if (!duration)
        return;

    WorldPackets::Item::ItemTimeUpdate update;
    update.ItemGuid = GetGUID();
    update.DurationLeft = duration;
    owner->SendDirectMessage(update.Write());
}

Item* Item::CreateItem(uint32 item, uint32 count, Player const* player)
{
    if (count < 1)
        return nullptr;                                        //don't create item at zero count

    if (ItemTemplate const* pProto = sObjectMgr->GetItemTemplate(item))
    {
        if (count > pProto->GetMaxStackSize())
            count = pProto->GetMaxStackSize();

        ASSERT(count != 0 && "pProto->GetStackable() == 0 but checked at loading already");

        Item* pItem = NewItemOrBag(pProto);
        if (pItem->Create(sObjectMgr->GetGenerator<HighGuid::Item>()->Generate(), item, player))
        {
            pItem->SetCount(count);

            if (player)
                sScriptMgr->OnItemCreate(const_cast<Player*>(player), pProto, pItem);
            return pItem;
        }
        delete pItem;
    }
    else
        ASSERT(false);

    return nullptr;
}

Item* Item::CloneItem(uint32 count, Player const* player) const
{
    Item* newItem = CreateItem(GetEntry(), count, player);
    if (!newItem)
        return nullptr;

    newItem->SetGuidValue(ITEM_FIELD_CREATOR, GetGuidValue(ITEM_FIELD_CREATOR));
    newItem->SetGuidValue(ITEM_FIELD_GIFT_CREATOR, GetGuidValue(ITEM_FIELD_GIFT_CREATOR));
    newItem->SetUInt32Value(ITEM_FIELD_DYNAMIC_FLAGS, GetUInt32Value(ITEM_FIELD_DYNAMIC_FLAGS));
    newItem->SetUInt32Value(ITEM_FIELD_EXPIRATION, GetUInt32Value(ITEM_FIELD_EXPIRATION));
    for (uint32 bonusListID : GetDynamicValues(ITEM_DYNAMIC_FIELD_BONUS_LIST_IDS))
        newItem->AddBonuses(bonusListID);

    // player CAN be NULL in which case we must not update random properties because that accesses player's item update queue
    if (player)
        newItem->SetItemRandomProperties(GetItemRandomEnchantmentId());

    return newItem;
}

bool Item::IsBindedNotWith(Player const* player) const
{
    // not binded item
    if (!IsSoulBound())
        return false;

    // own item
    if (GetOwnerGUID() == player->GetGUID())
        return false;

    if (HasFlag(ITEM_FIELD_DYNAMIC_FLAGS, ITEM_FLAG_BOP_TRADEABLE))
        if (allowedGUIDs.find(player->GetGUID()) != allowedGUIDs.end())
            return false;

    // BOA item case
    if (IsBoundAccountWide())
        return false;

    return true;
}

void Item::RemoveFromWorld()
{
    if (!IsInWorld())
        return;

    m_inWorld = 0;

    if (Player* owner = GetOwner())
        owner->RemoveUpdateItem(this);

    // if we remove from world then sending changes not required
    ClearUpdateMask(true);
}

ObjectGuid Item::GetOwnerGUID() const
{
    return GetGuidValue(ITEM_FIELD_OWNER);
}

void Item::SetOwnerGUID(ObjectGuid guid)
{
    SetGuidValue(ITEM_FIELD_OWNER, guid);
}

void Item::BuildUpdate(UpdateDataMapType& /*data_map*/)
{
    ClearUpdateMask(false);
}

void Item::AddToObjectUpdateIfNeeded()
{
    if (IsInWorld() && !m_objectUpdated)
    {
        if (Player* owner = GetOwner())
            owner->AddUpdateItem(this);
        m_objectUpdated = true;
    }
}

uint32 Item::GetScriptId() const
{
    return GetTemplate()->ScriptId;
}

void Item::BuildDynamicValuesUpdate(uint8 updateType, ByteBuffer* data, Player* target) const
{
    if (!target)
        return;

    std::size_t blockCount = UpdateMask::GetBlockCount(_dynamicValuesCount);

    uint32* flags = nullptr;
    uint32 visibleFlag = GetDynamicUpdateFieldData(target, flags);

    *data << uint8(blockCount);
    std::size_t maskPos = data->wpos();
    data->resize(data->size() + blockCount * sizeof(UpdateMask::BlockType));

    using DynamicFieldChangeTypeUT = std::underlying_type<UpdateMask::DynamicFieldChangeType>::type;

    for (uint16 index = 0; index < _dynamicValuesCount; ++index)
    {
        std::vector<uint32> const& values = _dynamicValues[index];
        if (_fieldNotifyFlags & flags[index] ||
            ((updateType == UPDATETYPE_VALUES ? _dynamicChangesMask[index] != UpdateMask::UNCHANGED : !values.empty()) && (flags[index] & visibleFlag)))
        {
            UpdateMask::SetUpdateBit(data->contents() + maskPos, index);

            std::size_t arrayBlockCount = UpdateMask::GetBlockCount(values.size());
            *data << DynamicFieldChangeTypeUT(UpdateMask::EncodeDynamicFieldChangeType(arrayBlockCount, _dynamicChangesMask[index], updateType));
            if (updateType == UPDATETYPE_VALUES && _dynamicChangesMask[index] == UpdateMask::VALUE_AND_SIZE_CHANGED)
                *data << uint32(values.size());

            std::size_t arrayMaskPos = data->wpos();

            data->resize(data->size() + arrayBlockCount * sizeof(UpdateMask::BlockType));
            if (index != ITEM_DYNAMIC_FIELD_MODIFIERS)
            {
                for (std::size_t v = 0; v < values.size(); ++v)
                {
                    if (updateType == UPDATETYPE_VALUES ? _dynamicChangesArrayMask[index][v] : values[v])
                    {
                        UpdateMask::SetUpdateBit(data->contents() + arrayMaskPos, v);
                        *data << uint32(values[v]);
                    }
                }
            }
            else
            {
                uint32 m = 0;

                // work around stupid item modifier field requirements - push back values mask by sizeof(m) bytes if size was not appended yet
                // if (updateType == UPDATETYPE_VALUES && _dynamicChangesMask[index] != UpdateMask::VALUE_AND_SIZE_CHANGED && _changesMask[ITEM_FIELD_MODIFIERS_MASK])
                // {
                    // *data << m;
                    // arrayMaskPos += sizeof(m);
                // }

                // in case of ITEM_DYNAMIC_FIELD_MODIFIERS it is ITEM_FIELD_MODIFIERS_MASK that controls index of each value, not updatemask
                // so we just have to write this starting from 0 index
                for (std::size_t v = 0; v < values.size(); ++v)
                {
                    if (values[v])
                    {
                        UpdateMask::SetUpdateBit(data->contents() + arrayMaskPos, m++);
                        *data << uint32(values[v]);
                    }
                }

                // if (updateType == UPDATETYPE_VALUES && _changesMask[ITEM_FIELD_MODIFIERS_MASK])
                    // data->put(arrayMaskPos - sizeof(m), m);
            }
        }
    }
}

void Item::SaveRefundDataToDB()
{
    CharacterDatabaseTransaction trans = CharacterDatabase.BeginTransaction();

    CharacterDatabasePreparedStatement* stmt = CharacterDatabase.GetPreparedStatement(CHAR_DEL_ITEM_REFUND_INSTANCE);
    stmt->setUInt64(0, GetGUIDLow());
    trans->Append(stmt);

    stmt = CharacterDatabase.GetPreparedStatement(CHAR_INS_ITEM_REFUND_INSTANCE);
    stmt->setUInt64(0, GetGUIDLow());
    stmt->setUInt64(1, GetRefundRecipient().GetCounter());
    stmt->setUInt64(2, GetPaidMoney());
    stmt->setUInt16(3, uint16(GetPaidExtendedCost()));
    trans->Append(stmt);

    CharacterDatabase.CommitTransaction(trans);
}

void Item::DeleteRefundDataFromDB(CharacterDatabaseTransaction* trans)
{
    if (trans)
    {
        CharacterDatabasePreparedStatement* stmt = CharacterDatabase.GetPreparedStatement(CHAR_DEL_ITEM_REFUND_INSTANCE);
        stmt->setUInt64(0, GetGUIDLow());
        (*trans)->Append(stmt);

    }
}

Bag* Item::ToBag()
{
    if (IsBag())
        return reinterpret_cast<Bag*>(this);
    return nullptr;
}

const Bag* Item::ToBag() const
{
    if (IsBag())
        return reinterpret_cast<const Bag*>(this);
    return nullptr;
}

bool Item::IsEquipable() const
{
    return GetTemplate()->GetInventoryType() != INVTYPE_NON_EQUIP;
}

bool Item::IsSuitableForItemLevelCalulcation(bool includeOffHand) const
{
    return (GetTemplate()->GetClass() == ITEM_CLASS_WEAPON || GetTemplate()->GetClass() == ITEM_CLASS_ARMOR) && GetTemplate()->GetInventoryType() != INVTYPE_NON_EQUIP &&
           GetTemplate()->GetInventoryType() != INVTYPE_TABARD && GetTemplate()->GetInventoryType() != INVTYPE_RANGED && GetTemplate()->GetInventoryType() != INVTYPE_ROBE &&
           (!includeOffHand || GetTemplate()->GetInventoryType() != INVTYPE_TABARD);
}

bool Item::IsLocked() const
{
    return !HasFlag(ITEM_FIELD_DYNAMIC_FLAGS, ITEM_FLAG_UNLOCKED);
}

bool Item::IsBag() const
{
    return GetTemplate()->GetInventoryType() == INVTYPE_BAG;
}

bool Item::IsCurrencyToken() const
{
    return GetTemplate()->IsCurrencyToken();
}

void Item::SetNotRefundable(Player* owner, bool changestate /*=true*/, CharacterDatabaseTransaction* trans /*=NULL*/)
{
    if (!HasFlag(ITEM_FIELD_DYNAMIC_FLAGS, ITEM_FLAG_REFUNDABLE))
        return;

    WorldPackets::Item::ItemExpirePurchaseRefund itemExpirePurchaseRefund;
    itemExpirePurchaseRefund.ItemGUID = GetGUID();
    owner->SendDirectMessage(itemExpirePurchaseRefund.Write());

    RemoveFlag(ITEM_FIELD_DYNAMIC_FLAGS, ITEM_FLAG_REFUNDABLE);
    // Following is not applicable in the trading procedure
    if (changestate)
        SetState(ITEM_CHANGED, owner);

    SetRefundRecipient(ObjectGuid::Empty);
    SetPaidMoney(0);
    SetPaidExtendedCost(0);
    DeleteRefundDataFromDB(trans);

    owner->DeleteRefundReference(GetGUID());

    if (!IsBag())
    {
        uint32 transmogId = sDB2Manager.GetTransmogId(GetEntry(), _bonusData.AppearanceModID);
        if (transmogId && owner->GetCollectionMgr()->HasItemAppearance(transmogId))
            owner->GetCollectionMgr()->RemoveTransmogCondition(transmogId, trans == nullptr);
    }
}

void Item::SetRefundRecipient(ObjectGuid const& pGuidLow)
{
    m_refundRecipient = pGuidLow;
}

void Item::SetPaidMoney(uint64 money)
{
    m_paidMoney = money;
}

void Item::SetPaidExtendedCost(uint32 iece)
{
    m_paidExtendedCost = iece;
}

ObjectGuid Item::GetRefundRecipient()
{
    return m_refundRecipient;
}

uint64 Item::GetPaidMoney()
{
    return m_paidMoney;
}

uint32 Item::GetPaidExtendedCost()
{
    return m_paidExtendedCost;
}

void Item::UpdatePlayedTime(Player* owner)
{
    /*  Here we update our played time
        We simply add a number to the current played time,
        based on the time elapsed since the last update hereof.
    */
    // Get current played time
    uint32 current_playtime = GetUInt32Value(ITEM_FIELD_CREATE_PLAYED_TIME);
    // Calculate time elapsed since last played time update
    time_t curtime = GameTime::GetGameTime();
    auto elapsed = uint32(curtime - m_lastPlayedTimeUpdate);
    uint32 new_playtime = current_playtime + elapsed;
    // Check if the refund timer has expired yet
    if (new_playtime <= 2 * HOUR)
    {
        // No? Proceed.
        // Update the data field
        SetUInt32Value(ITEM_FIELD_CREATE_PLAYED_TIME, new_playtime);
        // Flag as changed to get saved to DB
        SetState(ITEM_CHANGED, owner);
        // Speaks for itself
        m_lastPlayedTimeUpdate = curtime;
        return;
    }
    // Yes
    SetNotRefundable(owner);
}

uint32 Item::GetPlayedTime()
{
    return GetUInt32Value(ITEM_FIELD_CREATE_PLAYED_TIME) + uint32(GameTime::GetGameTime() - m_lastPlayedTimeUpdate);
}

bool Item::IsRefundExpired()
{
    return (GetPlayedTime() > 2 * HOUR);
}

void Item::SetSoulboundTradeable(GuidSet const& allowedLooters)
{
    SetFlag(ITEM_FIELD_DYNAMIC_FLAGS, ITEM_FLAG_BOP_TRADEABLE);
    allowedGUIDs = allowedLooters;
}

void Item::ClearSoulboundTradeable(Player* currentOwner)
{
    RemoveFlag(ITEM_FIELD_DYNAMIC_FLAGS, ITEM_FLAG_BOP_TRADEABLE);
    if (allowedGUIDs.empty())
        return;

    allowedGUIDs.clear();
    SetState(ITEM_CHANGED, currentOwner);
    CharacterDatabasePreparedStatement* stmt = CharacterDatabase.GetPreparedStatement(CHAR_DEL_ITEM_BOP_TRADE);
    stmt->setUInt64(0, GetGUIDLow());
    CharacterDatabase.Execute(stmt);
}

bool Item::CheckSoulboundTradeExpire()
{
    if (!GetOwner())
        return false;

    // called from owner's update - GetOwner() MUST be valid
    if (GetUInt32Value(ITEM_FIELD_CREATE_PLAYED_TIME) + 2 * HOUR < GetOwner()->GetTotalPlayedTime())
    {
        ClearSoulboundTradeable(GetOwner());
        return true; // remove from tradeable list
    }

    return false;
}

bool Item::HasStats(WorldPackets::Item::ItemInstance const& itemInstance, BonusData const* bonus)
{
    if (itemInstance.RandomPropertiesID != 0)
        return true;

    for (auto i : bonus->ItemStatValue)
        if (i != 0)
            return true;

    return false;
}

bool Item::IsPotion() const
{
    return GetTemplate()->IsPotion();
}

bool Item::IsVellum() const
{
    return GetTemplate()->IsVellum();
}

bool Item::IsConjuredConsumable() const
{
    return GetTemplate()->IsConjuredConsumable();
}

bool Item::IsCraftingReagent() const
{
    return GetTemplate()->IsCraftingReagent();
}

bool Item::IsRangedWeapon() const
{
    return GetTemplate()->IsRangedWeapon();
}

BonusData const* Item::GetBonus() const
{
    return &_bonusData;
}

uint32 Item::GetQuality() const
{
    return _bonusData.Quality;
}

bool Item::IsValidTransmogrificationTarget() const
{
    ItemTemplate const* proto = GetTemplate();
    if (!proto)
        return false;

    // if (proto->GetQuality() == ITEM_QUALITY_LEGENDARY)
        // return false;

    if (proto->GetClass() != ITEM_CLASS_ARMOR && proto->GetClass() != ITEM_CLASS_WEAPON)
        return false;

    if (proto->GetClass() == ITEM_CLASS_WEAPON && proto->GetSubClass() == ITEM_SUBCLASS_WEAPON_FISHING_POLE)
        return false;

    if (proto->GetFlags2() & ITEM_FLAG2_NO_ALTER_ITEM_VISUAL)
        return false;

    if (proto->GetInventoryType() == INVTYPE_TABARD)
        return true;

    if (!HasStats())
        return false;

    return true;
}

bool Item::HasStats() const
{
    if (GetItemRandomPropertyId() != 0)
        return true;

    ItemTemplate const* proto = GetTemplate();
    for (uint8 i = 0; i < MAX_ITEM_PROTO_STATS; ++i)
        if (proto->GetItemStatType(i) > 0)
            return true;

    return false;
}

// used by mail items, transmog cost, stationeryinfo and others
uint32 Item::GetBuyPrice(bool &success)
{
    ItemTemplate const* proto = GetTemplate();
    if (!proto)
        return 0;

    if (proto->GetFlags2() & ITEM_FLAG2_OVERRIDE_GOLD_COST)
        return proto->GetBuyPrice();

    ImportPriceQualityEntry const* qualityPrice = sImportPriceQualityStore.LookupEntry(GetQuality() + 1);
    ItemPriceBaseEntry const* basePrice = sItemPriceBaseStore.LookupEntry(GetItemLevel(GetOwnerLevel()));

    if (!qualityPrice || !basePrice)
        return 0;

    float baseFactor = 0.0f;

    uint32 inventoryType = proto->GetInventoryType();
    uint32 itemClass = proto->GetClass();
    uint32 itemSubClass = proto->GetSubClass();

    if (inventoryType == INVTYPE_WEAPON || inventoryType == INVTYPE_2HWEAPON || inventoryType == INVTYPE_WEAPONMAINHAND || inventoryType == INVTYPE_WEAPONOFFHAND ||
        inventoryType == INVTYPE_RANGED || inventoryType == INVTYPE_THROWN || inventoryType == INVTYPE_RANGEDRIGHT)
        baseFactor = basePrice->Weapon;
    else
        baseFactor = basePrice->Armor;

    if (inventoryType == INVTYPE_ROBE)
        inventoryType = INVTYPE_CHEST;

    if (itemClass == ITEM_CLASS_GEM && itemSubClass == ITEM_SUBCLASS_GEM_ARTIFACT_RELIC)
    {
        inventoryType = INVTYPE_WEAPON;
        baseFactor = basePrice->Weapon * 0.33333334f;
    }

    float typeFactor = 0.0f;
    int8 weaponType = -1;

    switch (inventoryType)
    {
        case INVTYPE_HEAD:
        case INVTYPE_NECK:
        case INVTYPE_SHOULDERS:
        case INVTYPE_CHEST:
        case INVTYPE_WAIST:
        case INVTYPE_LEGS:
        case INVTYPE_FEET:
        case INVTYPE_WRISTS:
        case INVTYPE_HANDS:
        case INVTYPE_FINGER:
        case INVTYPE_TRINKET:
        case INVTYPE_CLOAK:
        case INVTYPE_HOLDABLE:
        {
            ImportPriceArmorEntry const* armorPrice = sImportPriceArmorStore.LookupEntry(inventoryType);
            if (!armorPrice)
                return 0;

            switch (proto->GetSubClass())
            {
                case ITEM_SUBCLASS_ARMOR_MISCELLANEOUS:
                case ITEM_SUBCLASS_ARMOR_CLOTH:
                    typeFactor = armorPrice->ClothModifier;
                    break;
                case ITEM_SUBCLASS_ARMOR_LEATHER:
                    typeFactor = armorPrice->LeatherModifier;
                    break;
                case ITEM_SUBCLASS_ARMOR_MAIL:
                    typeFactor = armorPrice->ChainModifier;
                    break;
                case ITEM_SUBCLASS_ARMOR_PLATE:
                    typeFactor = armorPrice->PlateModifier;
                    break;
                default:
                    typeFactor = 1.0f;
                    break;
            }

            break;
        }
        case INVTYPE_WEAPON:
            weaponType = 2;
            break;
        case INVTYPE_RANGED:
        case INVTYPE_RANGEDRIGHT:
        case INVTYPE_RELIC:
            weaponType = 4;
            break;
        case INVTYPE_2HWEAPON:
            weaponType = 3;
            break;
        case INVTYPE_WEAPONMAINHAND:
            weaponType = 0;
            break;
        case INVTYPE_WEAPONOFFHAND:
            weaponType = 1;
            break;
        case INVTYPE_SHIELD:
        {
            ImportPriceShieldEntry const* shieldPrice = sImportPriceShieldStore.LookupEntry(2); // it only has two rows, it's unclear which is the one used
            if (!shieldPrice)
                return 0;

            typeFactor = shieldPrice->Data;
            break;
        }
        default:
            return proto->GetBuyPrice();
    }

    if (weaponType >= 0)
    {
        ImportPriceWeaponEntry const* weaponPrice = sImportPriceWeaponStore.LookupEntry(weaponType + 1);
        if (!weaponPrice)
            return 0;

        typeFactor = weaponPrice->Data;
    }

    success = true;

    return static_cast<uint32>(qualityPrice->Data * proto->GetPriceVariance() * proto->GetPriceRandomValue() * typeFactor * baseFactor);
}

uint32 Item::GetSellPrice()
{
    ItemTemplate const* proto = GetTemplate();
    if (!proto)
        return 0;

    if (proto->GetFlags2() & ITEM_FLAG2_OVERRIDE_GOLD_COST)
        return proto->GetSellPrice();

    bool normalPrice = false;
    uint32 cost = const_cast<Item*>(this)->GetBuyPrice(normalPrice);

    if (!normalPrice)
        return proto->GetSellPrice();

    ItemClassEntry const* classEntry = sDB2Manager.GetItemClassByOldEnum(proto->GetClass());
    if (!classEntry)
        return 0;

    uint32 buyCount = proto->VendorStackCount;

    if (buyCount < 1)
        buyCount = 1;

    return static_cast<uint32>((cost * classEntry->PriceModifier) / buyCount);
}

void Item::SetScaleIlvl(int64 ilvl)
{
    if (ilvl < 0)
        ilvl = 0;

    m_scaleLvl = ilvl;
}

uint32 Item::GetScaleIlvl() const
{
    return m_scaleLvl;
}

void Item::SetArtIlvlBonus(uint32 ilvl)
{
    m_artIlvlBonus = ilvl;
}

uint32 Item::GetArtIlvlBonus() const
{
    return m_artIlvlBonus;
}

enum class ItemTransmogrificationWeaponCategory : uint8
{
    // Two-handed
    MELEE_2H,
    RANGED,

    // One-handed
    AXE_MACE_SWORD_1H,
    DAGGER,
    FIST,

    INVALID
};

static ItemTransmogrificationWeaponCategory GetTransmogrificationWeaponCategory(ItemTemplate const* proto)
{
    if (proto->GetClass() == ITEM_CLASS_WEAPON)
    {
        switch (proto->GetSubClass())
        {
            case ITEM_SUBCLASS_WEAPON_AXE2:
            case ITEM_SUBCLASS_WEAPON_MACE2:
            case ITEM_SUBCLASS_WEAPON_SWORD2:
            case ITEM_SUBCLASS_WEAPON_STAFF:
            case ITEM_SUBCLASS_WEAPON_POLEARM:
                return ItemTransmogrificationWeaponCategory::MELEE_2H;
            case ITEM_SUBCLASS_WEAPON_BOW:
            case ITEM_SUBCLASS_WEAPON_GUN:
            case ITEM_SUBCLASS_WEAPON_CROSSBOW:
                return ItemTransmogrificationWeaponCategory::RANGED;
            case ITEM_SUBCLASS_WEAPON_AXE:
            case ITEM_SUBCLASS_WEAPON_MACE:
            case ITEM_SUBCLASS_WEAPON_SWORD:
            case ITEM_SUBCLASS_WEAPON_WARGLAIVES:
                return ItemTransmogrificationWeaponCategory::AXE_MACE_SWORD_1H;
            case ITEM_SUBCLASS_WEAPON_DAGGER:
                return ItemTransmogrificationWeaponCategory::DAGGER;
            case ITEM_SUBCLASS_WEAPON_FIST_WEAPON:
                return ItemTransmogrificationWeaponCategory::FIST;
            default:
                break;
        }
    }

    return ItemTransmogrificationWeaponCategory::INVALID;
}

int32 const ItemTransmogrificationSlots[MAX_INVTYPE] =
{
    -1,                                                     // INVTYPE_NON_EQUIP
    EQUIPMENT_SLOT_HEAD,                                    // INVTYPE_HEAD
    -1,                                                     // INVTYPE_NECK
    EQUIPMENT_SLOT_SHOULDERS,                               // INVTYPE_SHOULDERS
    EQUIPMENT_SLOT_BODY,                                    // INVTYPE_BODY
    EQUIPMENT_SLOT_CHEST,                                   // INVTYPE_CHEST
    EQUIPMENT_SLOT_WAIST,                                   // INVTYPE_WAIST
    EQUIPMENT_SLOT_LEGS,                                    // INVTYPE_LEGS
    EQUIPMENT_SLOT_FEET,                                    // INVTYPE_FEET
    EQUIPMENT_SLOT_WRISTS,                                  // INVTYPE_WRISTS
    EQUIPMENT_SLOT_HANDS,                                   // INVTYPE_HANDS
    -1,                                                     // INVTYPE_FINGER
    -1,                                                     // INVTYPE_TRINKET
    EQUIPMENT_SLOT_MAINHAND,                                // INVTYPE_WEAPON
    EQUIPMENT_SLOT_OFFHAND,                                 // INVTYPE_SHIELD
    EQUIPMENT_SLOT_MAINHAND,                                // INVTYPE_RANGED
    EQUIPMENT_SLOT_BACK,                                    // INVTYPE_CLOAK
    EQUIPMENT_SLOT_MAINHAND,                                // INVTYPE_2HWEAPON
    -1,                                                     // INVTYPE_BAG
    EQUIPMENT_SLOT_TABARD,                                  // INVTYPE_TABARD
    EQUIPMENT_SLOT_CHEST,                                   // INVTYPE_ROBE
    EQUIPMENT_SLOT_MAINHAND,                                // INVTYPE_WEAPONMAINHAND
    EQUIPMENT_SLOT_MAINHAND,                                // INVTYPE_WEAPONOFFHAND
    EQUIPMENT_SLOT_OFFHAND,                                 // INVTYPE_HOLDABLE
    -1,                                                     // INVTYPE_AMMO
    -1,                                                     // INVTYPE_THROWN
    EQUIPMENT_SLOT_MAINHAND,                                // INVTYPE_RANGEDRIGHT
    -1                                                      // INVTYPE_RELIC
};


bool Item::CanTransmogrifyItemWithItem(Item const* item, ItemModifiedAppearanceEntry const* itemModifiedAppearance)
{
    ItemTemplate const* source = sObjectMgr->GetItemTemplate(itemModifiedAppearance->ItemID); // source
    ItemTemplate const* target = item->GetTemplate(); // dest

    if (!source || !target)
        return false;

    if (itemModifiedAppearance == item->GetItemModifiedAppearance())
        return false;

    if (!item->IsValidTransmogrificationTarget())
        return false;

    if (source->GetClass() != target->GetClass())
        return false;

    if (source->GetFlags3() & ITEM_FLAG2_IGNORE_QUALITY_FOR_ITEM_VISUAL_SOURCE) // Invisible item don`t check
        return true;

    if (source->GetInventoryType() == INVTYPE_BAG || source->GetInventoryType() == INVTYPE_RELIC || source->GetInventoryType() == INVTYPE_FINGER || source->GetInventoryType() == INVTYPE_TRINKET || source->GetInventoryType() == INVTYPE_AMMO)
        return false;

    if (source->GetSubClass() != target->GetSubClass())
    {
        switch (source->GetClass())
        {
            case ITEM_CLASS_WEAPON:
                if (GetTransmogrificationWeaponCategory(source) != GetTransmogrificationWeaponCategory(target))
                    return false;
                break;
            case ITEM_CLASS_ARMOR:
                if (source->GetSubClass() != ITEM_SUBCLASS_ARMOR_COSMETIC)
                    return false;
                if (source->GetInventoryType() != target->GetInventoryType())
                    if (ItemTransmogrificationSlots[source->GetInventoryType()] != ItemTransmogrificationSlots[target->GetInventoryType()])
                        return false;
                break;
            default:
                return false;
        }
    }

    return true;
}

ObjectGuid Item::GetChildItem() const
{
    return m_childItem;
}

void Item::SetChildItem(ObjectGuid childItem)
{
    m_childItem = childItem;
}

uint32 Item::GetItemLevel(uint8 ownerLevel, bool isPvP) const
{
    ItemTemplate const* stats = GetTemplate();
    if (!stats)
        return MIN_ITEM_LEVEL;

    Player* owner = GetOwner();
    if (owner)
        ownerLevel = owner->GetEffectiveLevel();

    uint32 itemLevel = stats->GetBaseItemLevel();
    itemLevel += GetScaleIlvl(); // can return 0!

    if (stats->GetArtifactID())
        itemLevel += GetArtIlvlBonus();

    if (ScalingStatDistributionEntry const* ssd = sScalingStatDistributionStore.LookupEntry(GetScalingStatDistribution()))
    {
        uint32 level = ownerLevel;
        if (uint32 fixedLevel = GetModifier(ITEM_MODIFIER_SCALING_STAT_DISTRIBUTION_FIXED_LEVEL))
            level = fixedLevel;
        else
            level = std::min(std::max(level, ssd->MinLevel), ssd->MaxLevel);

        if (SandboxScalingEntry const* sandbox = sSandboxScalingStore.LookupEntry(_bonusData.SandboxScalingId))
            if ((sandbox->Flags & 2 || sandbox->MinLevel || sandbox->MaxLevel) && !(sandbox->Flags & 4))
                level = std::min(std::max(uint32(level), sandbox->MinLevel), sandbox->MaxLevel);

        if (auto heirloomIlvl = uint32(sDB2Manager.GetCurveValueAt(ssd->PlayerLevelToItemLevelCurveID, level)))
            itemLevel = heirloomIlvl;
    }

    itemLevel += _bonusData.ItemLevelBonus;

    if (ItemUpgradeEntry const* upgrade = sItemUpgradeStore.LookupEntry(GetModifier(ITEM_MODIFIER_UPGRADE_ID)))
        itemLevel += upgrade->ItemLevelIncrement;

    // if (!isPvP)
        for (auto gemItemLevelBonus : _bonusData.GemItemLevelBonus)
            itemLevel += gemItemLevelBonus;

    if (owner)
    {
        if (owner->HasTournamentRules())
            if (stats->GetFlags3() & ITEM_FLAG3_PVP_TOURNAMENT_GEAR)
                itemLevel = MIN_ITEM_LEVEL;

        if (uint32 maxItemLevel = owner->GetUInt32Value(UNIT_FIELD_MAX_ITEM_LEVEL))
            itemLevel = std::min(itemLevel, maxItemLevel);

        if (uint32 minItemLevel = owner->GetUInt32Value(UNIT_FIELD_MIN_ITEM_LEVEL))
            if (itemLevel >= owner->GetUInt32Value(UNIT_FIELD_MIN_ITEM_LEVEL_CUTOFF))
                itemLevel = std::max(itemLevel, minItemLevel);
    }

    return std::min(std::max(itemLevel, uint32(MIN_ITEM_LEVEL)), uint32(MAX_ITEM_LEVEL));
}

void Item::SetFixedLevel(uint8 level)
{
    if (!_bonusData.HasFixedLevel || GetModifier(ITEM_MODIFIER_SCALING_STAT_DISTRIBUTION_FIXED_LEVEL))
        return;

    if (ScalingStatDistributionEntry const* ssd = sScalingStatDistributionStore.LookupEntry(_bonusData.ScalingStatDistribution))
    {
        level = std::min(std::max(uint32(level), ssd->MinLevel), ssd->MaxLevel);

        if (SandboxScalingEntry const* sandbox = sSandboxScalingStore.LookupEntry(_bonusData.SandboxScalingId))
            if ((sandbox->Flags & 2 || sandbox->MinLevel || sandbox->MaxLevel) && !(sandbox->Flags & 4))
                level = std::min(std::max(uint32(level), sandbox->MinLevel), sandbox->MaxLevel);

        SetModifier(ITEM_MODIFIER_SCALING_STAT_DISTRIBUTION_FIXED_LEVEL, level);
    }
}

int32 Item::GetRequiredLevel() const
{
    if (_bonusData.RequiredLevelOverride)
        return _bonusData.RequiredLevelOverride;
    else if (_bonusData.HasFixedLevel)
        return GetModifier(ITEM_MODIFIER_SCALING_STAT_DISTRIBUTION_FIXED_LEVEL);
    else
        return _bonusData.RequiredLevel;
}

int32 Item::GetItemStatType(uint32 index) const
{
    ASSERT(index < MAX_ITEM_PROTO_STATS);
    return _bonusData.ItemStatType[index];
}

uint32 Item::GetArmor() const
{
    return GetTemplate()->GetArmor(GetItemLevel(GetOwnerLevel()));
}

void Item::GetDamage(float& minDamage, float& maxDamage) const
{
    GetTemplate()->GetDamage(GetItemLevel(GetOwnerLevel()), minDamage, maxDamage);
}

int32 Item::GetItemStatValue(uint32 index, bool isPvP) const
{
    ASSERT(index < MAX_ITEM_PROTO_STATS);
    uint32 itemLevel = GetItemLevel(GetOwnerLevel(), isPvP);

    if (float randomPropPoints = GetRandomPropertyPoints(itemLevel, GetQuality(), GetTemplate()->GetInventoryType(), GetTemplate()->GetSubClass()))
    {
        float statValue = _bonusData.StatPercentEditor[index] * randomPropPoints * 0.0001f;
        if (GtItemSocketCostPerLevelEntry const* gtCost = sItemSocketCostPerLevelGameTable.GetRow(itemLevel))
            statValue -= float(int32(_bonusData.StatPercentageOfSocket[index] * gtCost->SocketCost));

        return int32(std::floor(statValue + 0.5f));
    }

    return _bonusData.ItemStatValue[index];
}

bool Item::CanBeDisenchanted()
{
    ItemTemplate const* itemTemplate = GetTemplate();

    if (!itemTemplate)
        return false;

    if (itemTemplate->GetFlags2() & ITEM_FLAG2_DISENCHANT_TO_LOOT_TABLE)
        return true;

    if (itemTemplate->GetFlags() & (ITEM_FLAG_CONJURED | ITEM_FLAG_NO_DISENCHANT) || itemTemplate->GetBonding() == BIND_QUEST_ITEM)
        return false;

    if (itemTemplate->GetArea() || itemTemplate->GetMaxStackSize() > 1)
        return false;

    if (!GetSellPrice() && !sDB2Manager.HasItemCurrencyCost(itemTemplate->GetId()))
        return false;

    return true;
}

ItemDisenchantLootEntry const* Item::GetDisenchantLoot(Player const* owner)
{
    return owner ? GetDisenchantLoot(GetTemplate(), GetQuality(), GetItemLevel(owner->getLevel())) : nullptr;
}

ItemDisenchantLootEntry const* Item::GetDisenchantLoot(ItemTemplate const* itemTemplate, uint32 quality, uint32 itemLevel)
{
    if (!CanBeDisenchanted())
        return nullptr;

    uint32 itemClass = itemTemplate->GetClass();
    int8 itemSubClass = itemTemplate->GetSubClass();
    uint8 expansion = itemTemplate->GetExpansion();

    for (ItemDisenchantLootEntry const* disenchant : sItemDisenchantLootStore)
    {
        if (disenchant->ItemClass != itemClass)
            continue;

        if (disenchant->Subclass >= 0 && disenchant->Subclass != 11 && itemSubClass)
            continue;

        if (disenchant->Quality != quality)
            continue;

        if (disenchant->MinLevel > itemLevel || disenchant->MaxLevel < itemLevel)
            continue;

        if (disenchant->ExpansionID != -2 && disenchant->ExpansionID != expansion)
            continue;

        return disenchant;
    }

    return nullptr;
}

uint8 Item::GetSocketColor(uint8 index) const
{
    ASSERT(index < MAX_ITEM_PROTO_SOCKETS);
    return _bonusData.SocketColor[index];
}

uint32 Item::GetAppearanceModId() const
{
    return GetUInt32Value(ITEM_FIELD_ITEM_APPEARANCE_MOD_ID);
}

void Item::SetAppearanceModId(uint32 appearanceModId)
{
    SetUInt32Value(ITEM_FIELD_ITEM_APPEARANCE_MOD_ID, appearanceModId);
}

uint32 Item::GetDisplayId(Player const* owner) const
{
    ItemModifier transmogModifier = ITEM_MODIFIER_TRANSMOG_APPEARANCE_ALL_SPECS;
    if (HasFlag(ITEM_FIELD_MODIFIERS_MASK, AppearanceModifierMaskSpecSpecific))
        transmogModifier = AppearanceModifierSlotBySpec[owner->GetActiveTalentGroup()];

    if (ItemModifiedAppearanceEntry const* transmog = sItemModifiedAppearanceStore.LookupEntry(GetModifier(transmogModifier)))
        return sDB2Manager.GetItemDisplayId(transmog->ItemID, transmog->ItemAppearanceModifierID);

    if (GetAppearanceModId())
        return sDB2Manager.GetItemDisplayId(GetEntry(), GetAppearanceModId());

    return sDB2Manager.GetItemDisplayId(GetEntry(), _bonusData.AppearanceModID);
}

ItemModifiedAppearanceEntry const* Item::GetItemModifiedAppearance() const
{
    return sDB2Manager.GetItemModifiedAppearance(GetEntry(), _bonusData.AppearanceModID);
}

float Item::GetRepairCostMultiplier() const
{
    return _bonusData.RepairCostMultiplier;
}

uint32 Item::GetScalingStatDistribution() const
{
    return _bonusData.ScalingStatDistribution;
}

uint8 Item::GetDisplayToastMethod(uint8 value) const
{
    return _bonusData.DisplayToastMethod[value];
}

int32 Item::GetDisenchantLootID() const
{
    return _bonusData.DisenchantLootId;
}

uint32 Item::GetModifier(ItemModifier modifier) const
{
    return GetDynamicValue(ITEM_DYNAMIC_FIELD_MODIFIERS, modifier);
}

void Item::SetModifier(ItemModifier modifier, uint32 value)
{
    ApplyModFlag(ITEM_FIELD_MODIFIERS_MASK, 1 << modifier, value != 0);
    SetDynamicValue(ITEM_DYNAMIC_FIELD_MODIFIERS, modifier, value);
}

uint32 Item::GetVisibleEntry(Player const* owner) const
{
    ItemModifier transmogModifier = ITEM_MODIFIER_TRANSMOG_APPEARANCE_ALL_SPECS;
    if (HasFlag(ITEM_FIELD_MODIFIERS_MASK, AppearanceModifierMaskSpecSpecific))
        transmogModifier = AppearanceModifierSlotBySpec[owner->GetActiveTalentGroup()];

    if (ItemModifiedAppearanceEntry const* transmog = sItemModifiedAppearanceStore.LookupEntry(GetModifier(transmogModifier)))
        return transmog->ItemID;

    return GetEntry();
}

uint16 Item::GetVisibleAppearanceModId(Player const* owner) const
{
    ItemModifier transmogModifier = ITEM_MODIFIER_TRANSMOG_APPEARANCE_ALL_SPECS;
    if (HasFlag(ITEM_FIELD_MODIFIERS_MASK, AppearanceModifierMaskSpecSpecific))
        transmogModifier = AppearanceModifierSlotBySpec[owner->GetActiveTalentGroup()];

    if (ItemModifiedAppearanceEntry const* transmog = sItemModifiedAppearanceStore.LookupEntry(GetModifier(transmogModifier)))
        return transmog->ItemAppearanceModifierID;

    if (GetAppearanceModId())
        return uint16(GetAppearanceModId());

    return _bonusData.AppearanceModID;
}

uint32 Item::GetVisibleEnchantmentId(Player const* owner) const
{
    ItemModifier illusionModifier = ITEM_MODIFIER_ENCHANT_ILLUSION_ALL_SPECS;
    if (HasFlag(ITEM_FIELD_MODIFIERS_MASK, IllusionModifierMaskSpecSpecific))
        illusionModifier = IllusionModifierSlotBySpec[owner->GetActiveTalentGroup()];

    if (uint32 enchantIllusion = GetModifier(illusionModifier))
        return enchantIllusion;

    return GetEnchantmentId(PERM_ENCHANTMENT_SLOT);
}

uint16 Item::GetVisibleItemVisual(Player const* owner) const
{
    if (SpellItemEnchantmentEntry const* enchant = sSpellItemEnchantmentStore.LookupEntry(GetVisibleEnchantmentId(owner)))
        return enchant->ItemVisual;

    return 0;
}

void Item::AddBonuses(uint32 bonusListID)
{
    if (DB2Manager::ItemBonusList const* bonuses = sDB2Manager.GetItemBonusList(bonusListID))
    {
        AddDynamicValue(ITEM_DYNAMIC_FIELD_BONUS_LIST_IDS, bonusListID);
        for (ItemBonusEntry const* bonus : *bonuses)
            _bonusData.AddBonus(bonus->Type, bonus->Value);

        SetUInt32Value(ITEM_FIELD_ITEM_APPEARANCE_MOD_ID, _bonusData.AppearanceModID);
    }
}

void Item::SetBinding(bool val)
{
    if (Player* owner = GetOwner())
    {
        if (!IsBag())
        {
            uint32 transmogId = sDB2Manager.GetTransmogId(GetEntry(), _bonusData.AppearanceModID);
            if (transmogId && !owner->GetCollectionMgr()->HasTransmog(transmogId))
            {
                if (HasFlag(ITEM_FIELD_DYNAMIC_FLAGS, ITEM_FLAG_REFUNDABLE))
                    owner->GetCollectionMgr()->AddTransmog(transmogId, transmogId);
                else
                    owner->GetCollectionMgr()->AddTransmog(transmogId, 0);
            }
        }
    }

    ApplyModFlag(ITEM_FIELD_DYNAMIC_FLAGS, ITEM_FLAG_SOULBOUND, val);
}

bool Item::IsSoulBound() const
{
    return HasFlag(ITEM_FIELD_DYNAMIC_FLAGS, ITEM_FLAG_SOULBOUND);
}

bool Item::IsBoundAccountWide() const
{
    return (GetTemplate()->GetFlags() & ITEM_FLAG_IS_BOUND_TO_ACCOUNT) != 0;
}

void BonusData::Initialize(ItemTemplate const* proto)
{
    Quality = proto->GetQuality();
    ItemLevelBonus = 0;
    RequiredLevel = proto->GetBaseRequiredLevel();
    for (uint32 i = 0; i < MAX_ITEM_PROTO_STATS; ++i)
        ItemStatType[i] = proto->GetItemStatType(i);

    for (uint32 i = 0; i < MAX_ITEM_PROTO_STATS; ++i)
        ItemStatValue[i] = proto->GetItemStatValue(i);

    for (uint32 i = 0; i < MAX_ITEM_PROTO_STATS; ++i)
        StatPercentEditor[i] = proto->GetItemStatAllocation(i);

    for (uint32 i = 0; i < MAX_ITEM_PROTO_STATS; ++i)
        StatPercentageOfSocket[i] = proto->GetItemStatSocketCostMultiplier(i);

    for (uint32 i = 0; i < MAX_ITEM_PROTO_SOCKETS; ++i)
    {
        SocketColor[i] = proto->GetSocketType(i);
        GemItemLevelBonus[i] = 0;
        GemRelicType[i] = -1;
        GemRelicRankBonus[i] = 0;
    }

    Bonding = static_cast<ItemBondingType>(proto->GetBonding());
    RelicType = -1;
    AppearanceModID = 0;
    if (ItemModifiedAppearanceEntry const* defaultAppearance = sDB2Manager.GetDefaultItemModifiedAppearance(proto->GetId()))
        AppearanceModID = defaultAppearance->ItemAppearanceModifierID;

    RepairCostMultiplier = 1.0f;
    ScalingStatDistribution = proto->GetScalingStatDistribution();
    memset(DisplayToastMethod, 0, sizeof(DisplayToastMethod));
    DisplayToastMethod[0] = 3; // SHOW_LOOT_TOAST_RARE
    DisplayToastMethod[1] = 0;
    DescriptionID = 0;
    SandboxScalingId = 0;
    DisenchantLootId = 0;
    HasFixedLevel = false;
    HasSturdiness = false;
    RequiredLevelOverride = 0;

    _state.AppearanceModPriority = std::numeric_limits<int32>::max();
    _state.ScalingStatDistributionPriority = std::numeric_limits<int32>::max();
    _state.HasQualityBonus = false;
}

void BonusData::Initialize(WorldPackets::Item::ItemInstance const& itemInstance)
{
    ItemTemplate const* proto = sObjectMgr->GetItemTemplate(itemInstance.ItemID);
    if (!proto)
        return;

    Initialize(proto);

    if (itemInstance.ItemBonus)
        for (uint32 bonusListID : itemInstance.ItemBonus->BonusListIDs)
            if (DB2Manager::ItemBonusList const* bonuses = sDB2Manager.GetItemBonusList(bonusListID))
                for (ItemBonusEntry const* bonus : *bonuses)
                    AddBonus(bonus->Type, bonus->Value);
}

void BonusData::AddBonus(uint32 type, int32 const (&values)[3])
{
    switch (type)
    {
        case ITEM_BONUS_ITEM_LEVEL:
            ItemLevelBonus += values[0];
            break;
        case ITEM_BONUS_STAT:
        {
            uint32 statIndex = 0;
            for (statIndex = 0; statIndex < MAX_ITEM_PROTO_STATS; ++statIndex)
                if (ItemStatType[statIndex] == values[0] || ItemStatType[statIndex] == -1)
                    break;

            if (statIndex < MAX_ITEM_PROTO_STATS)
            {
                ItemStatType[statIndex] = values[0];
                StatPercentEditor[statIndex] += values[1];
                if (ItemStatType[statIndex] == ITEM_MOD_CR_STURDINESS) // items with this should not lose durability on player death
                    HasSturdiness = true;
            }
            break;
        }
        case ITEM_BONUS_QUALITY:
            if (!_state.HasQualityBonus)
            {
                Quality = static_cast<uint32>(values[0]);
                _state.HasQualityBonus = true;
            }
            else if (Quality < static_cast<uint32>(values[0]))
                Quality = static_cast<uint32>(values[0]);
            break;
        case ITEM_BONUS_SOCKET:
        {
            uint32 socketCount = values[0];
            for (uint32 i = 0; i < MAX_ITEM_PROTO_SOCKETS && socketCount; ++i)
                if (!SocketColor[i])
                {
                    SocketColor[i] = values[1];
                    --socketCount;
                }
            break;
        }
        case ITEM_BONUS_APPEARANCE:
            if (values[1] < _state.AppearanceModPriority)
            {
                AppearanceModID = static_cast<uint32>(values[0]);
                _state.AppearanceModPriority = values[1];
            }
            break;
        case ITEM_BONUS_REQUIRED_LEVEL:
            RequiredLevel += values[0];
            break;
        case ITEM_BONUS_REPAIR_COST_MULTIPLIER:
            RepairCostMultiplier *= static_cast<float>(values[0]) * 0.01f;
            break;
        case ITEM_BONUS_SCALING_STAT_DISTRIBUTION:
        case ITEM_BONUS_SCALING_STAT_DISTRIBUTION_FIXED:
            if (values[1] < _state.ScalingStatDistributionPriority)
            {
                ScalingStatDistribution = static_cast<uint32>(values[0]);
                SandboxScalingId = static_cast<uint32>(values[2]);
                _state.ScalingStatDistributionPriority = values[1];
                HasFixedLevel = type == ITEM_BONUS_SCALING_STAT_DISTRIBUTION_FIXED;
            }
            break;
        case ITEM_BONUS_DISPLAY_TOAST_METHOD:
            if (values[0] < 0xB)
                if (static_cast<uint32>(values[1]) < DisplayToastMethod[1])
                {
                    DisplayToastMethod[0] = static_cast<uint32>(values[0]);
                    DisplayToastMethod[1] = static_cast<uint32>(values[1]);
                }
            break;
        case ITEM_BONUS_BONDING:
            Bonding = ItemBondingType(values[0]);
            break;
        case ITEM_BONUS_RELIC_TYPE:
            RelicType = values[0];
            break;
        //case ITEM_BONUS_DISENCHANT_LOOT_ID:
            //DisenchantLootId = values[0];
            //break;
        case ITEM_BONUS_DESCRIPTION:
            DescriptionID = values[0];
            break;
        case ITEM_BONUS_SUFFIX:
        case ITEM_BONUS_RANDOM_ENCHANTMENT:
            break;
        case ITEM_BONUS_OVERRIDE_REQUIRED_LEVEL:
            RequiredLevelOverride = values[0];
            break;
        default:
            break;
    }
}

void Item::ApplyItemChildEquipment(Player* owner, bool apply)
{
    //TC_LOG_DEBUG("network", "Item::ApplyItemChildEquipment apply %u", apply);

    ItemChildEquipmentEntry const* childEquipement = sDB2Manager.GetItemChildEquipment(GetEntry());
    if (!childEquipement)
        return;

    Item* item = owner->GetItemByEntry(childEquipement->ChildItemID);
    if (!item)
        return;

    if (apply)
    {
        uint16 dest;
        if (owner->CanEquipItem(NULL_SLOT, dest, item, true) == EQUIP_ERR_OK)
        {
            owner->EquipItem(dest, item, true);
            owner->AutoUnequipOffhandIfNeed();
        }
    }
    else
    {
        ItemPosCountVec dest;
        if (owner->CanStoreItem(NULL_BAG, NULL_SLOT, dest, item, false) == EQUIP_ERR_OK)
            owner->StoreItem(dest, item, true);
    }
}

void Item::ApplyArtifactPowerEnchantmentBonuses(EnchantmentSlot slot, uint32 enchantId, bool apply, Player* owner)
{
    if (SpellItemEnchantmentEntry const* enchant = sSpellItemEnchantmentStore.LookupEntry(enchantId))
    {
        for (uint8 i = 0; i < MAX_ITEM_ENCHANTMENT_EFFECTS; ++i)
        {
            switch (enchant->Effect[i])
            {
                case ITEM_ENCHANTMENT_TYPE_ARTIFACT_POWER_BONUS_RANK_BY_TYPE:
                    for (ItemDynamicFieldArtifactPowers const& artifactPower : GetArtifactPowers())
                    {
                        if (sArtifactPowerStore.AssertEntry(artifactPower.ArtifactPowerId)->Label == enchant->EffectArg[i])
                        {
                            ItemDynamicFieldArtifactPowers newPower = artifactPower;
                            if (apply)
                                newPower.CurrentRankWithBonus += enchant->EffectPointsMin[i];
                            else
                            {
                                if (newPower.CurrentRankWithBonus == newPower.PurchasedRank)
                                    continue;

                                newPower.CurrentRankWithBonus -= enchant->EffectPointsMin[i];
                            }

                            if (IsEquipped())
                            {
                                if (ArtifactPowerRankEntry const* artifactPowerRank = sDB2Manager.GetArtifactPowerRank(artifactPower.ArtifactPowerId, newPower.CurrentRankWithBonus ? newPower.CurrentRankWithBonus - 1 : 0))
                                {
                                    owner->ApplyArtifactPowerRank(this, artifactPowerRank, newPower.CurrentRankWithBonus != 0);

                                    int8 CheckRankForDiffSpells = apply ? (artifactPowerRank->RankIndex - 1) : (artifactPowerRank->RankIndex + 1);

                                    if (CheckRankForDiffSpells >= 0)
                                    {
                                        if (ArtifactPowerRankEntry const* oldArtifactPowerRank = sDB2Manager.GetArtifactPowerRank(artifactPower.ArtifactPowerId, CheckRankForDiffSpells))
                                        {
                                            if (oldArtifactPowerRank->SpellID != artifactPowerRank->SpellID)
                                                owner->RemoveAurasDueToSpell(oldArtifactPowerRank->SpellID);
                                        }
                                    }
                                }
                            }

                            SetArtifactPower(&newPower);
                        }
                    }
                    break;
                case ITEM_ENCHANTMENT_TYPE_ARTIFACT_POWER_BONUS_RANK_BY_ID:
                    if (ItemDynamicFieldArtifactPowers const* artifactPower = GetArtifactPower(enchant->EffectArg[i]))
                    {
                        ItemDynamicFieldArtifactPowers newPower = *artifactPower;
                        if (apply)
                            newPower.CurrentRankWithBonus += enchant->EffectPointsMin[i];
                        else
                            newPower.CurrentRankWithBonus -= enchant->EffectPointsMin[i];

                        if (IsEquipped())
                            if (ArtifactPowerRankEntry const* artifactPowerRank = sDB2Manager.GetArtifactPowerRank(artifactPower->ArtifactPowerId, newPower.CurrentRankWithBonus ? newPower.CurrentRankWithBonus - 1 : 0))
                                owner->ApplyArtifactPowerRank(this, artifactPowerRank, newPower.CurrentRankWithBonus != 0);

                        SetArtifactPower(&newPower);
                    }
                    break;
                case ITEM_ENCHANTMENT_TYPE_ARTIFACT_POWER_BONUS_RANK_PICKER:
                    if (slot >= SOCK_ENCHANTMENT_SLOT && slot <= SOCK_ENCHANTMENT_SLOT_3 && _bonusData.GemRelicType[slot - SOCK_ENCHANTMENT_SLOT] != -1)
                    {
                        if (ArtifactPowerPickerEntry const* artifactPowerPicker = sArtifactPowerPickerStore.LookupEntry(enchant->EffectArg[i]))
                        {
                            if (sConditionMgr->IsPlayerMeetingCondition(owner, artifactPowerPicker->PlayerConditionID))
                            {
                                for (ItemDynamicFieldArtifactPowers const& artifactPower : GetArtifactPowers())
                                {
                                    if (sArtifactPowerStore.AssertEntry(artifactPower.ArtifactPowerId)->Label == _bonusData.GemRelicType[slot - SOCK_ENCHANTMENT_SLOT])
                                    {
                                        auto newPower = artifactPower;
                                        if (apply)
                                            newPower.CurrentRankWithBonus += enchant->EffectPointsMin[i];
                                        else
                                            newPower.CurrentRankWithBonus -= enchant->EffectPointsMin[i];

                                        if (IsEquipped())
                                            if (auto artifactPowerRank = sDB2Manager.GetArtifactPowerRank(artifactPower.ArtifactPowerId, newPower.CurrentRankWithBonus ? newPower.CurrentRankWithBonus - 1 : 0))
                                                owner->ApplyArtifactPowerRank(this, artifactPowerRank, newPower.CurrentRankWithBonus != 0);

                                        SetArtifactPower(&newPower);
                                    }
                                }
                            }
                        }
                    }
                    break;
                default:
                    break;
            }
        }
    }
}

void Item::HandleAllBonusTraits(Player * owner, bool apply)
{
    for (ItemDynamicFieldArtifactPowers const& artifactPower : GetArtifactPowers())
    {
        ItemDynamicFieldArtifactPowers newPower = artifactPower;

        ArtifactPowerEntry const* artifactPowerEntry = sArtifactPowerStore.AssertEntry(newPower.ArtifactPowerId);

        if (!(artifactPowerEntry->Flags & (ARTIFACT_POWER_FLAG_RELIC_TALENT | ARTIFACT_POWER_FLAG_HAS_RANK)))
            continue;

        uint8 oldRank = newPower.CurrentRankWithBonus;

        if (apply)
        {
            auto itr = bSavedBonusTraits.find(artifactPower.ArtifactPowerId);

            if (itr == bSavedBonusTraits.end())
                continue;

            newPower.CurrentRankWithBonus = itr->second;
        }
        else
        {
            if (newPower.CurrentRankWithBonus == newPower.PurchasedRank)
                continue;

            bSavedBonusTraits[newPower.ArtifactPowerId] = newPower.CurrentRankWithBonus;
            newPower.CurrentRankWithBonus = newPower.PurchasedRank;
        }

        if (IsEquipped())
        {
            if (ArtifactPowerRankEntry const* artifactPowerRank = sDB2Manager.GetArtifactPowerRank(artifactPower.ArtifactPowerId, newPower.CurrentRankWithBonus ? newPower.CurrentRankWithBonus - 1 : 0))
            {
                owner->ApplyArtifactPowerRank(this, artifactPowerRank, newPower.CurrentRankWithBonus != 0);

                int8 CheckRankForDiffSpells = oldRank ? oldRank - 1 : 0;

                if (CheckRankForDiffSpells >= 0)
                {
                    if (ArtifactPowerRankEntry const* oldArtifactPowerRank = sDB2Manager.GetArtifactPowerRank(artifactPower.ArtifactPowerId, CheckRankForDiffSpells))
                    {
                        if (oldArtifactPowerRank->SpellID != artifactPowerRank->SpellID)
                            owner->RemoveAurasDueToSpell(oldArtifactPowerRank->SpellID);
                    }
                }
            }
        }

        SetArtifactPower(&newPower);
    }

    if (apply && !bSavedBonusTraits.empty())
        bSavedBonusTraits.clear();
}

void Item::InitArtifactPowers(uint8 artifactId)
{
    ArtifactCategory categoryID = ARTIFACT_CATEGORY_CLASS;
    if (ArtifactEntry const* entry = sArtifactStore.LookupEntry(artifactId))
        categoryID = static_cast<ArtifactCategory>(entry->ArtifactCategoryID);

    auto artifactPowers = sDB2Manager.GetArtifactPowers(artifactId);

    auto relicsPowers = sDB2Manager.GetArtifactPowers(0);
    for (auto power : relicsPowers)
        if (power->Flags & ARTIFACT_POWER_FLAG_RELIC_TALENT)
            artifactPowers.push_back(power);

    m_artifactPowerIdToIndex.assign(sArtifactPowerStore.GetNumRows() + 1, -1);
    for (ArtifactPowerEntry const* artifactPower : artifactPowers)
    {
        if (m_artifactPowerIdToIndex[artifactPower->ID] != -1)
            continue;

        if (artifactPower->Tier && !GetModifier(ITEM_MODIFIER_ARTIFACT_TIER))
            continue;

        ItemDynamicFieldArtifactPowers powerData;
        memset(&powerData, 0, sizeof(powerData));
        powerData.ArtifactPowerId = artifactPower->ID;
        powerData.PurchasedRank = 0;
        powerData.CurrentRankWithBonus = ((artifactPower->Flags & ARTIFACT_POWER_FLAG_NO_LINK_REQUIRED) && !artifactPower->Tier) && categoryID != ARTIFACT_CATEGORY_FISH ? 1 : 0;
        SetArtifactPower(&powerData, true);
    }
}

void Item::ActivateFishArtifact(uint8 /*artifactId*/)
{
    if (auto counterPower = const_cast<ItemDynamicFieldArtifactPowers*>(GetArtifactPower(1031)))
    {
        if (counterPower->PurchasedRank)
            return;

        ItemDynamicFieldArtifactPowers newPower = *counterPower;
        ++newPower.PurchasedRank;
        SetArtifactPower(&newPower);
    }

    if (auto* mainPowers = const_cast<ItemDynamicFieldArtifactPowers*>(GetArtifactPower(1021)))
    {
        ItemDynamicFieldArtifactPowers newPower = *mainPowers;
        ++newPower.PurchasedRank;
        ++newPower.CurrentRankWithBonus;
        SetArtifactPower(&newPower);
    }
    SetUInt64Value(ITEM_FIELD_ARTIFACT_XP, GetUInt64Value(ITEM_FIELD_ARTIFACT_XP) - 100);
    SetState(ITEM_CHANGED, GetOwner());
}

void Item::InitArtifactsTier(uint8 artifactId)
{
    std::vector<ArtifactPowerEntry const*> artifactPowers = sDB2Manager.GetArtifactPowers(artifactId);
    m_artifactPowerIdToIndex.assign(sArtifactPowerStore.GetNumRows() + 1, -1);
    for (ArtifactPowerEntry const* artifactPower : artifactPowers)
    {
        if (m_artifactPowerIdToIndex[artifactPower->ID] != -1)
            continue;

        if (!artifactPower->Tier || !GetModifier(ITEM_MODIFIER_ARTIFACT_TIER))
            continue;

        ItemDynamicFieldArtifactPowers powerData;
        memset(&powerData, 0, sizeof(powerData));
        powerData.ArtifactPowerId = artifactPower->ID;
        powerData.PurchasedRank = 0;
        powerData.CurrentRankWithBonus = ((artifactPower->Flags & ARTIFACT_POWER_FLAG_NO_LINK_REQUIRED) && !artifactPower->Tier) ? 1 : 0;
        SetArtifactPower(&powerData, true);
    }
}

void Item::CopyArtifactDataFromParent(Item* parent)
{
    memcpy(_bonusData.GemItemLevelBonus, parent->GetBonus()->GemItemLevelBonus, sizeof(_bonusData.GemItemLevelBonus));
    SetModifier(ITEM_MODIFIER_ARTIFACT_APPEARANCE_ID, parent->GetModifier(ITEM_MODIFIER_ARTIFACT_APPEARANCE_ID));
    SetAppearanceModId(parent->GetAppearanceModId());
}

DynamicFieldStructuredView<ItemDynamicFieldArtifactPowers> Item::GetArtifactPowers() const
{
    return GetDynamicStructuredValues<ItemDynamicFieldArtifactPowers>(ITEM_DYNAMIC_FIELD_ARTIFACT_POWERS);
}

ItemDynamicFieldArtifactPowers const* Item::GetArtifactPower(uint32 artifactPowerId) const
{
    if (artifactPowerId >= m_artifactPowerIdToIndex.size())
        return nullptr;

    int16 index = m_artifactPowerIdToIndex[artifactPowerId];
    if (index != -1)
        return GetDynamicStructuredValue<ItemDynamicFieldArtifactPowers>(ITEM_DYNAMIC_FIELD_ARTIFACT_POWERS, index);

    return nullptr;
}

void Item::SetArtifactPower(ItemDynamicFieldArtifactPowers const* artifactPower, bool createIfMissing /*= false*/)
{
    int16 index = m_artifactPowerIdToIndex[artifactPower->ArtifactPowerId];
    if (index == -1)
    {
        if (!createIfMissing)
            return;

        index = m_artifactPowerCount++;
        m_artifactPowerIdToIndex[artifactPower->ArtifactPowerId] = index;
    }

    SetDynamicStructuredValue(ITEM_DYNAMIC_FIELD_ARTIFACT_POWERS, index, artifactPower);
}

void Item::GiveArtifactXp(uint64 amount, Item* sourceItem, uint32 artifactCategoryId)
{
    Player* owner = GetOwner();
    if (!owner)
        return;

    if (artifactCategoryId)
    {
        if (ArtifactCategoryEntry const* artifactCategory = sArtifactCategoryStore.LookupEntry(artifactCategoryId))
        {
            uint32 artifactKnowledgeLevel = 1;
            if (sourceItem)
                artifactKnowledgeLevel = sourceItem->GetModifier(ITEM_MODIFIER_ARTIFACT_KNOWLEDGE_LEVEL);
            else
                artifactKnowledgeLevel = owner->GetCurrency(artifactCategory->XpMultCurrencyID) + 1;

            if (!artifactKnowledgeLevel)
                artifactKnowledgeLevel = 1;

            if (GtArtifactKnowledgeMultiplierEntry const* artifactKnowledge = sArtifactKnowledgeMultiplierGameTable.GetRow(artifactKnowledgeLevel))
                amount = uint64(amount * artifactKnowledge->Multiplier);

            if (amount >= 5000)
                amount = 50 * (amount / 50);
            else if (amount >= 1000)
                amount = 25 * (amount / 25);
            else if (amount >= 50)
                amount = 5 * (amount / 5);
        }
    }

    SetUInt64Value(ITEM_FIELD_ARTIFACT_XP, GetUInt64Value(ITEM_FIELD_ARTIFACT_XP) + amount);

    WorldPackets::Artifact::XpGain artifactXpGain;
    artifactXpGain.ArtifactGUID = GetGUID();
    artifactXpGain.Xp = amount;
    owner->SendDirectMessage(artifactXpGain.Write());

    SetState(ITEM_CHANGED, owner);

    owner->UpdateAchievementCriteria(CRITERIA_TYPE_GAIN_ARTIFACT_POWER, amount);
}

uint32 Item::GetTotalPurchasedArtifactPowers() const
{
    uint32 purchasedRanks = 0;
    for (ItemDynamicFieldArtifactPowers const& power : GetArtifactPowers())
        purchasedRanks += power.PurchasedRank;

    return purchasedRanks;
}

void Item::SetOwnerLevel(uint8 level)
{
    m_ownerLevel = level;
}

uint8 Item::GetOwnerLevel() const
{
    return m_ownerLevel;
}

void Item::UpgradeLegendary()
{
    ItemTemplate const* proto = GetTemplate();
    if (!proto)
        return;

    // Correct transmog bonus in craft item
    if (GetUInt32Value(ITEM_FIELD_CONTEXT) == 13 && !GetItemModifiedAppearance() && !_bonusData.AppearanceModID)
        AddBonuses(3408);

    if (!proto->IsLegionLegendary())
        return;

    if (GetItemLevel(GetOwnerLevel()) < sWorld->getIntConfig(CONFIG_ITEM_LEGENDARY_LEVEL)) // Only for legendary item from 7.0.3
    {
        std::set<uint32> bonusListID;
        for (uint32 bonusID : GetDynamicValues(ITEM_DYNAMIC_FIELD_BONUS_LIST_IDS))
            bonusListID.insert(bonusID);

        bonusListID.insert(3459);
        if (sWorld->getIntConfig(CONFIG_ITEM_LEGENDARY_LEVEL) == 940)
            bonusListID.insert(3530); // Upgrade Legendary item to 940 ilevel

        if (sWorld->getIntConfig(CONFIG_ITEM_LEGENDARY_LEVEL) == 970)
        {
            bonusListID.erase(3530); // Upgrade Legendary item to 940 ilevel
            bonusListID.insert(3570); // Upgrade Legendary item to 970 ilevel
        }

        ClearDynamicValue(ITEM_DYNAMIC_FIELD_BONUS_LIST_IDS);
        _bonusData.Initialize(GetTemplate());

        for (uint32 bonusID : bonusListID)
            AddBonuses(bonusID);

        SetState(ITEM_CHANGED, GetOwner());
    }
}

uint32 Item::GetDescriptionID()
{
    if (_bonusData.DescriptionID)
        return _bonusData.DescriptionID;

    return DescriptionID;
}
