/*
 * Copyright (C) 2008-2012 TrinityCore <http://www.trinitycore.org/>
 * Copyright (C) 2006-2009 ScriptDev2 <https://scriptdev2.svn.sourceforge.net/>
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

/* ScriptData
SDName: Bloodmyst_Isle
SD%Complete: 95
SDComment: Quest support: 9670, 9756(gossip items text needed), 9759(objects burning state needed).
SDCategory: Bloodmyst Isle
EndScriptData */

/* ContentData
mob_webbed_creature
npc_captured_sunhawk_agent
npc_sironas
npc_demolitionist_legoso
EndContentData */

#include "ScriptMgr.h"
#include "ScriptedCreature.h"
#include "ScriptedEscortAI.h"
#include "ScriptedGossip.h"
#include "Player.h"
#include "Group.h"

/*######
## mob_webbed_creature
######*/

//possible creatures to be spawned
uint32 const possibleSpawns[32] = {17322, 17661, 17496, 17522, 17340, 17352, 17333, 17524, 17654, 17348, 17339, 17345, 17359, 17353, 17336, 17550, 17330, 17701, 17321, 17680, 17325, 17320, 17683, 17342, 17715, 17334, 17341, 17338, 17337, 17346, 17344, 17327};

class mob_webbed_creature : public CreatureScript
{
public:
    mob_webbed_creature() : CreatureScript("mob_webbed_creature") { }

    CreatureAI* GetAI(Creature* creature) const
    {
        return new mob_webbed_creatureAI (creature);
    }

    struct mob_webbed_creatureAI : public ScriptedAI
    {
        mob_webbed_creatureAI(Creature* creature) : ScriptedAI(creature) {}

        void Reset() {}

        void EnterCombat(Unit* /*who*/) {}

        void JustDied(Unit* killer)
        {
            uint32 spawnCreatureID = 0;

            switch (urand(0, 2))
            {
                case 0:
                    spawnCreatureID = 17681;
                    if (Player* player = killer->ToPlayer())
                        player->KilledMonsterCredit(spawnCreatureID, ObjectGuid::Empty);
                    break;
                case 1:
                case 2:
                    spawnCreatureID = possibleSpawns[urand(0, 30)];
                    break;
            }

            if (spawnCreatureID)
                me->SummonCreature(spawnCreatureID, 0.0f, 0.0f, 0.0f, me->GetOrientation(), TEMPSUMMON_TIMED_DESPAWN_OUT_OF_COMBAT, 60000);
        }
    };

};

/*######
## npc_captured_sunhawk_agent
######*/

#define C_SUNHAWK_TRIGGER 17974

#define GOSSIP_HELLO_CSA     "[PH] "
#define GOSSIP_SELECT_CSA1   "[PH] "
#define GOSSIP_SELECT_CSA2   "[PH] "
#define GOSSIP_SELECT_CSA3   "[PH] "
#define GOSSIP_SELECT_CSA4   "[PH] "
#define GOSSIP_SELECT_CSA5   "[PH] "

class npc_captured_sunhawk_agent : public CreatureScript
{
public:
    npc_captured_sunhawk_agent() : CreatureScript("npc_captured_sunhawk_agent") { }

    bool OnGossipSelect(Player* player, Creature* creature, uint32 /*sender*/, uint32 action)
    {
        player->PlayerTalkClass->ClearMenus();
        switch (action)
        {
            case GOSSIP_ACTION_INFO_DEF+1:
                player->ADD_GOSSIP_ITEM(GossipOptionNpc::None, GOSSIP_SELECT_CSA1, GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF+2);
                player->SEND_GOSSIP_MENU(9137, creature->GetGUID());
                break;
            case GOSSIP_ACTION_INFO_DEF+2:
                player->ADD_GOSSIP_ITEM(GossipOptionNpc::None, GOSSIP_SELECT_CSA2, GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF+3);
                player->SEND_GOSSIP_MENU(9138, creature->GetGUID());
                break;
            case GOSSIP_ACTION_INFO_DEF+3:
                player->ADD_GOSSIP_ITEM(GossipOptionNpc::None, GOSSIP_SELECT_CSA3, GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF+4);
                player->SEND_GOSSIP_MENU(9139, creature->GetGUID());
                break;
            case GOSSIP_ACTION_INFO_DEF+4:
                player->ADD_GOSSIP_ITEM(GossipOptionNpc::None, GOSSIP_SELECT_CSA4, GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF+5);
                player->SEND_GOSSIP_MENU(9140, creature->GetGUID());
                break;
            case GOSSIP_ACTION_INFO_DEF+5:
                player->ADD_GOSSIP_ITEM(GossipOptionNpc::None, GOSSIP_SELECT_CSA5, GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF+6);
                player->SEND_GOSSIP_MENU(9141, creature->GetGUID());
                break;
            case GOSSIP_ACTION_INFO_DEF+6:
                player->CLOSE_GOSSIP_MENU();
                player->TalkedToCreature(C_SUNHAWK_TRIGGER, creature->GetGUID());
                break;
        }
        return true;
    }

    bool OnGossipHello(Player* player, Creature* creature)
    {
        if (player->HasAura(31609) && player->GetQuestStatus(9756) == QUEST_STATUS_INCOMPLETE)
        {
            player->ADD_GOSSIP_ITEM(GossipOptionNpc::None, GOSSIP_HELLO_CSA, GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF+1);
            player->SEND_GOSSIP_MENU(9136, creature->GetGUID());
        }
        else
            player->SEND_GOSSIP_MENU(9134, creature->GetGUID());

        return true;
    }

};

/*######
## Quest 9667: Saving Princess Stillpine
######*/

enum Stillpine
{
    QUEST_SAVING_PRINCESS_STILLPINE               = 9667,
    NPC_PRINCESS_STILLPINE                        = 17682,
    GO_PRINCESS_STILLPINES_CAGE                   = 181928,
    SPELL_OPENING_PRINCESS_STILLPINE_CREDIT       = 31003,
    SAY_DIRECTION                                 = 1
};

class go_princess_stillpines_cage : public GameObjectScript
{
public:
    go_princess_stillpines_cage() : GameObjectScript("go_princess_stillpines_cage") {}

    bool OnGossipHello(Player* player, GameObject* go)
    {
        if (Creature* stillpine = go->FindNearestCreature(NPC_PRINCESS_STILLPINE, 25, true))
        {
            go->SetGoState(GO_STATE_ACTIVE);
            stillpine->GetMotionMaster()->MovePoint(1, go->GetPositionX(), go->GetPositionY()-15, go->GetPositionZ());
            player->KillCreditGO(NPC_PRINCESS_STILLPINE, ObjectGuid::Empty);
        }
        return true;
    }
};

class npc_princess_stillpine : public CreatureScript
{
public:
    npc_princess_stillpine() : CreatureScript("npc_princess_stillpine") {}

    struct npc_princess_stillpineAI : public ScriptedAI
    {
        npc_princess_stillpineAI(Creature* creature) : ScriptedAI(creature) {}

        void MovementInform(uint32 type, uint32 id)
        {
            if (type == POINT_MOTION_TYPE && id == 1)
            {
                Talk(SAY_DIRECTION);
                me->DespawnOrUnsummon();
            }
        }
    };

    CreatureAI* GetAI(Creature* creature) const
    {
        return new npc_princess_stillpineAI(creature);
    }
};

/*######
## Quest 9759: Ending Their World
######*/

enum EndingTheirWorld
{
    SAY_SIRONAS_1                   = 0,  // "Petulant children, pray to your gods for you are about to meet them!"

    SAY_LEGOSO_1                    = 0,  // "There's no turning back now, %s. Stay close and watch my back."
    SAY_LEGOSO_2                    = 1,  // "There it is! Do you see where the lagre red crystal is jutting out from the Vector Coil? That's where I need to plant the first set of explosives."
    SAY_LEGOSO_3                    = 2,  // "Cover me!"
    SAY_LEGOSO_4                    = 3,  // "It won't be much longer, %s. Just keep them off me while I work."
    SAY_LEGOSO_5                    = 4,  // "That'll do it! Quickly, take cover!"
    SAY_LEGOSO_6                    = 5,  // "3..."
    SAY_LEGOSO_7                    = 6,  // "2..."
    SAY_LEGOSO_8                    = 7,  // "1..."
    SAY_LEGOSO_9                    = 8,  // "Don't get too excited, hero, that was the easy part. The challenge lies ahead! Let's go."
    SAY_LEGOSO_10                   = 9,  // "What in the Nether is that?!?!"
    SAY_LEGOSO_11                   = 10, // "Be ready for anything, %s."
    SAY_LEGOSO_12                   = 11, // "Blessed Light! She's siphoning energy right out of the Vector Coil!"
    SAY_LEGOSO_13                   = 12, // "Cover me, we have to do this quickly. Once I blow the support on this side, it will disrupt the energy beams and she'll break out! I doubt very much that she'll be happy to see us."
    SAY_LEGOSO_14                   = 13, // "I've almost got it! Just a little more time..."
    SAY_LEGOSO_15                   = 14, // "Take cover and be ready for the fight of your life, %s!"
    SAY_LEGOSO_16                   = 15, // "3..."
    SAY_LEGOSO_17                   = 16, // "2..."
    SAY_LEGOSO_18                   = 17, // "1..."
    SAY_LEGOSO_19                   = 18, // "Holy mother of O'ros!"
    SAY_LEGOSO_20                   = 19, // "I... I can't believe it's over. You did it! You've destoyed the blood elves and their leader!"
    SAY_LEGOSO_21                   = 20, // "Get back to Blood Watch. I'll see you there..."

    SPELL_BLOODMYST_TESLA           = 31611,
    SPELL_SIRONAS_CHANNELING        = 31612,

    SPELL_UPPERCUT                  = 10966,
    SPELL_IMMOLATE                  = 12742,
    SPELL_CURSE_OF_BLOOD            = 8282,

    SPELL_FROST_SHOCK               = 8056,
    SPELL_HEALING_SURGE             = 8004, // 3.3.5 Lesser Healing Wave
    SPELL_SEARING_TOTEM             = 38116,
    SPELL_STRENGTH_OF_EARTH_TOTEM   = 31633,

    NPC_SIRONAS                     = 17678,
    NPC_BLOODMYST_TESLA_COIL        = 17979,
    NPC_LEGOSO                      = 17982,

    GO_DRAENEI_EXPLOSIVES_1         = 182088,
    GO_DRAENEI_EXPLOSIVES_2         = 182091,

    ACTION_SIRONAS_CHANNEL_START    = 1,
    ACTION_SIRONAS_CHANNEL_STOP     = 2,

    ACTION_LEGOSO_SIRONAS_KILLED    = 1,

    EVENT_UPPERCUT                  = 1,
    EVENT_IMMOLATE                  = 2,
    EVENT_CURSE_OF_BLOOD            = 3,

    EVENT_FROST_SHOCK               = 1,
    EVENT_HEALING_SURGE             = 2,
    EVENT_SEARING_TOTEM             = 3,
    EVENT_STRENGTH_OF_EARTH_TOTEM   = 4,

    WP_START                        = 1,
    WP_EXPLOSIVES_FIRST_POINT       = 21,
    WP_EXPLOSIVES_FIRST_PLANT       = 22,
    WP_EXPLOSIVES_FIRST_RUNOFF      = 23,
    WP_EXPLOSIVES_FIRST_DETONATE    = 24,
    WP_DEBUG_1                      = 25,
    WP_DEBUG_2                      = 26,
    WP_SIRONAS_HILL                 = 33,
    WP_EXPLOSIVES_SECOND_BATTLEROAR = 35,
    WP_EXPLOSIVES_SECOND_PLANT      = 36,
    WP_EXPLOSIVES_SECOND_DETONATE   = 37,

    PHASE_NONE                      = 0,
    PHASE_CONTINUE                  = -1,
    PHASE_WP_26                     = 1,
    PHASE_WP_22                     = 2,
    PHASE_PLANT_FIRST_KNEEL         = 3,
    PHASE_PLANT_FIRST_STAND         = 4,
    PHASE_PLANT_FIRST_WORK          = 5,
    PHASE_PLANT_FIRST_FINISH        = 6,
    PHASE_PLANT_FIRST_TIMER_1       = 7,
    PHASE_PLANT_FIRST_TIMER_2       = 8,
    PHASE_PLANT_FIRST_TIMER_3       = 9,
    PHASE_PLANT_FIRST_DETONATE      = 10,
    PHASE_PLANT_FIRST_SPEECH        = 11,
    PHASE_PLANT_FIRST_ROTATE        = 12,
    PHASE_PLANT_FIRST_POINT         = 13,
    PHASE_FEEL_SIRONAS_1            = 14,
    PHASE_FEEL_SIRONAS_2            = 15,
    PHASE_MEET_SIRONAS_ROAR         = 16,
    PHASE_MEET_SIRONAS_TURN         = 17,
    PHASE_MEET_SIRONAS_SPEECH       = 18,
    PHASE_PLANT_SECOND_KNEEL        = 19,
    PHASE_PLANT_SECOND_SPEECH       = 20,
    PHASE_PLANT_SECOND_STAND        = 21,
    PHASE_PLANT_SECOND_FINISH       = 22,
    PHASE_PLANT_SECOND_WAIT         = 23,
    PHASE_PLANT_SECOND_TIMER_1      = 24,
    PHASE_PLANT_SECOND_TIMER_2      = 25,
    PHASE_PLANT_SECOND_TIMER_3      = 26,
    PHASE_PLANT_SECOND_DETONATE     = 27,
    PHASE_FIGHT_SIRONAS_STOP        = 28,
    PHASE_FIGHT_SIRONAS_SPEECH_1    = 29,
    PHASE_FIGHT_SIRONAS_SPEECH_2    = 30,
    PHASE_FIGHT_SIRONAS_START       = 31,
    PHASE_SIRONAS_SLAIN_SPEECH_1    = 32,
    PHASE_SIRONAS_SLAIN_EMOTE_1     = 33,
    PHASE_SIRONAS_SLAIN_EMOTE_2     = 34,
    PHASE_SIRONAS_SLAIN_SPEECH_2    = 35,

    DATA_EVENT_STARTER_GUID         = 0,

    MAX_EXPLOSIVES                  = 5,

    QUEST_ENDING_THEIR_WORLD        = 9759
};

Position const ExplosivesPos[2][MAX_EXPLOSIVES] =
{
    {
        {-1954.946f, -10654.714f, 110.448f},
        {-1956.331f, -10654.494f, 110.869f},
        {-1955.906f, -10656.221f, 110.791f},
        {-1957.294f, -10656.000f, 111.219f},
        {-1954.462f, -10656.451f, 110.404f}
    },
    {
        {-1915.137f, -10583.651f, 178.365f},
        {-1914.006f, -10582.964f, 178.471f},
        {-1912.717f, -10582.398f, 178.658f},
        {-1915.056f, -10582.251f, 178.162f},
        {-1913.883f, -10581.778f, 178.346f}
    }
};

/*######
## npc_sironas
######*/

class npc_sironas : public CreatureScript
{
public:
    npc_sironas() : CreatureScript("npc_sironas") {}

    CreatureAI* GetAI(Creature* creature) const
    {
        return new npc_sironasAI(creature);
    }

    struct npc_sironasAI : public ScriptedAI
    {
        npc_sironasAI(Creature* creature) : ScriptedAI(creature) { }

        void Reset()
        {
            _events.Reset();
            me->SetDisplayId(me->GetCreatureTemplate()->Modelid[1]);
        }

        void EnterCombat(Unit* /*who*/)
        {
            _events.ScheduleEvent(EVENT_UPPERCUT, 15 * IN_MILLISECONDS);
            _events.ScheduleEvent(EVENT_IMMOLATE, 10 * IN_MILLISECONDS);
            _events.ScheduleEvent(EVENT_CURSE_OF_BLOOD, 5 * IN_MILLISECONDS);
        }

        void JustDied(Unit* killer)
        {
            me->SetObjectScale(1.0f);
            _events.Reset();
            if (Creature* legoso = me->FindNearestCreature(NPC_LEGOSO, SIZE_OF_GRIDS))
            {
                Group* group = me->GetLootRecipientGroup();

                if (killer->GetGUID() == legoso->GetGUID() ||
                    (group && group->IsMember(killer->GetGUID())) ||
                    killer->GetGUID().GetCounter() == legoso->AI()->GetData(DATA_EVENT_STARTER_GUID))
                    legoso->AI()->DoAction(ACTION_LEGOSO_SIRONAS_KILLED);
            }
        }

        void UpdateAI(uint32 diff)
        {
            if (!UpdateVictim())
                return;

            _events.Update(diff);

            while (uint32 eventId = _events.ExecuteEvent())
            {
                switch (eventId)
                {
                    case EVENT_UPPERCUT:
                        DoCastVictim(SPELL_UPPERCUT);
                        _events.ScheduleEvent(EVENT_UPPERCUT, urand(10, 12) * IN_MILLISECONDS);
                        break;
                    case EVENT_IMMOLATE:
                        DoCastVictim(SPELL_IMMOLATE);
                        _events.ScheduleEvent(EVENT_IMMOLATE, urand(15, 20) * IN_MILLISECONDS);
                        break;
                    case EVENT_CURSE_OF_BLOOD:
                        DoCastVictim(SPELL_CURSE_OF_BLOOD);
                        _events.ScheduleEvent(EVENT_CURSE_OF_BLOOD, urand(20, 25) * IN_MILLISECONDS);
                        break;
                    default:
                        break;
                }
            }

            DoMeleeAttackIfReady();
        }

        void DoAction(int32 const param)
        {
            switch (param)
            {
                case ACTION_SIRONAS_CHANNEL_START:
                {
                    DoCast(me, SPELL_SIRONAS_CHANNELING);
                    std::list<Creature*> BeamList;
                    _beamGuidList.clear();
                    me->GetCreatureListWithEntryInGrid(BeamList, NPC_BLOODMYST_TESLA_COIL, SIZE_OF_GRIDS);
                    for (std::list<Creature*>::const_iterator itr = BeamList.begin(); itr != BeamList.end(); ++itr)
                    {
                        _beamGuidList.push_back((*itr)->GetGUID());
                        (*itr)->CastSpell(*itr, SPELL_BLOODMYST_TESLA);
                    }
                    break;
                }
                case ACTION_SIRONAS_CHANNEL_STOP:
                {
                    me->InterruptNonMeleeSpells(true, SPELL_SIRONAS_CHANNELING);
                    for (GuidList::const_iterator itr = _beamGuidList.begin(); itr != _beamGuidList.end(); ++itr)
                        if (Creature* beam = ObjectAccessor::GetCreature(*me, *itr))
                            beam->InterruptNonMeleeSpells(true, SPELL_BLOODMYST_TESLA);
                    break;
                }
                default:
                    break;
            }
        }

    private:
        GuidList _beamGuidList;
        EventMap _events;
    };
};

/*######
## npc_demolitionist_legoso
######*/

class npc_demolitionist_legoso : public CreatureScript
{
public:
    npc_demolitionist_legoso() : CreatureScript("npc_demolitionist_legoso") {}

    bool OnQuestAccept(Player* player, Creature* creature, Quest const* quest)
    {
        if (quest->GetQuestId() == QUEST_ENDING_THEIR_WORLD)
        {
            creature->AI()->SetData(DATA_EVENT_STARTER_GUID, player->GetGUID().GetCounter());
            CAST_AI(npc_demolitionist_legoso::npc_demolitionist_legosoAI, creature->AI())->Start(true, true, player->GetGUID(), quest);
        }

        return false;
    }

    CreatureAI* GetAI(Creature* creature) const
    {
        return new npc_demolitionist_legosoAI(creature);
    }

    struct npc_demolitionist_legosoAI : public npc_escortAI
    {
        npc_demolitionist_legosoAI(Creature* creature) : npc_escortAI(creature) {}

        uint32 GetData(uint32 id) const
        {
            switch (id)
            {
                case DATA_EVENT_STARTER_GUID:
                    return _eventStarterGuidLow;
                default:
                    return 0;
            }
        }

        void SetData(uint32 data, uint32 value)
        {
            switch (data)
            {
                case DATA_EVENT_STARTER_GUID:
                    _eventStarterGuidLow = value;
                    me->SetReactState(REACT_AGGRESSIVE);
                    break;
                default:
                    break;
            }
        }

        void Reset()
        {
            _phase = PHASE_NONE;
            _moveTimer = 0;
            ResetEvents();
            _eventStarterGuidLow = 0;

            me->SetCanDualWield(true);
            me->SetBaseWeaponDamage(OFF_ATTACK, MINDAMAGE, me->GetWeaponDamageRange(BASE_ATTACK, MINDAMAGE));
            me->SetBaseWeaponDamage(OFF_ATTACK, MAXDAMAGE, me->GetWeaponDamageRange(BASE_ATTACK, MAXDAMAGE));
            me->UpdateAttackPowerAndDamage();
        }

        void UpdateAI(uint32 diff)
        {
            _events.Update(diff);

            if (UpdateVictim())
            {
                while (uint32 eventId = _events.ExecuteEvent())
                {
                    switch (eventId)
                    {
                        case EVENT_FROST_SHOCK:
                            DoCastVictim(SPELL_FROST_SHOCK);
                            _events.DelayEvents(1 * IN_MILLISECONDS);
                            _events.ScheduleEvent(EVENT_FROST_SHOCK, urand(10, 15) * IN_MILLISECONDS);
                            break;
                        case EVENT_SEARING_TOTEM:
                            DoCast(me, SPELL_SEARING_TOTEM);
                            _events.DelayEvents(1 * IN_MILLISECONDS);
                            _events.ScheduleEvent(EVENT_SEARING_TOTEM, urand(110, 130) * IN_MILLISECONDS);
                            break;
                        case EVENT_STRENGTH_OF_EARTH_TOTEM:
                            DoCast(me, SPELL_STRENGTH_OF_EARTH_TOTEM);
                            _events.DelayEvents(1 * IN_MILLISECONDS);
                            _events.ScheduleEvent(EVENT_STRENGTH_OF_EARTH_TOTEM, urand(110, 130) * IN_MILLISECONDS);
                            break;
                        case EVENT_HEALING_SURGE:
                        {
                            Unit* target = NULL;
                            if (me->GetHealthPct() < 85)
                                target = me;
                            else if (Player* player = GetPlayerForEscort())
                                if (player->GetHealthPct() < 85)
                                    target = player;
                            if (target)
                            {
                                DoCast(target, SPELL_HEALING_SURGE);
                                _events.ScheduleEvent(EVENT_HEALING_SURGE, 10 * IN_MILLISECONDS);
                            }
                            else
                                _events.ScheduleEvent(EVENT_HEALING_SURGE, 2 * IN_MILLISECONDS);
                            break;
                        }
                        default:
                            break;
                    }
                }

                DoMeleeAttackIfReady();
            }

            if (HasEscortState(STATE_ESCORT_NONE))
                return;

            npc_escortAI::UpdateAI(diff);

            if (_phase)
            {
                if (_moveTimer <= diff)
                {
                    switch (_phase)
                    {
                        case PHASE_WP_26: //debug skip path to point 26, buggy path calculation
                            me->GetMotionMaster()->MovePoint(WP_DEBUG_2, -2021.77f, -10648.8f, 129.903f);
                            _moveTimer = 2000;
                            _phase = PHASE_CONTINUE;
                            break;
                        case PHASE_CONTINUE: // continue escort
                            SetEscortPaused(false);
                            _moveTimer = 0;
                            _phase = PHASE_NONE;
                            break;
                        case PHASE_WP_22: //debug skip path to point 22, buggy path calculation
                            me->GetMotionMaster()->MovePoint(WP_EXPLOSIVES_FIRST_PLANT, -1958.026f, -10660.465f, 111.547f);
                            Talk(SAY_LEGOSO_3);
                            _moveTimer = 2000;
                            _phase = PHASE_PLANT_FIRST_KNEEL;
                            break;
                        case PHASE_PLANT_FIRST_KNEEL: // plant first explosives stage 1 kneel
                            me->SetStandState(UNIT_STAND_STATE_KNEEL);
                            _moveTimer = 10000;
                            _phase = PHASE_PLANT_FIRST_STAND;
                            break;
                        case PHASE_PLANT_FIRST_STAND: // plant first explosives stage 1 stand
                            me->SetStandState(UNIT_STAND_STATE_STAND);
                            _moveTimer = 500;
                            _phase = PHASE_PLANT_FIRST_WORK;
                            break;
                        case PHASE_PLANT_FIRST_WORK: // plant first explosives stage 2 work
                            Talk(SAY_LEGOSO_4, GetEventStarterGUID());
                            _moveTimer = 17500;
                            _phase = PHASE_PLANT_FIRST_FINISH;
                            break;
                        case PHASE_PLANT_FIRST_FINISH: // plant first explosives finish
                            _explosives.clear();
                            for (uint8 i = 0; i != MAX_EXPLOSIVES; ++i)
                            {
                                if (GameObject* explosive = me->SummonGameObject(GO_DRAENEI_EXPLOSIVES_1, ExplosivesPos[0][i].m_positionX, ExplosivesPos[0][i].m_positionY, ExplosivesPos[0][i].m_positionZ, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0))
                                    _explosives.push_back(explosive);
                            }
                            me->HandleEmoteCommand(EMOTE_ONESHOT_NONE); // reset anim state
                            // force runoff movement so he will not screw up next waypoint
                            me->GetMotionMaster()->MovePoint(WP_EXPLOSIVES_FIRST_RUNOFF, -1955.6f, -10669.8f, 110.65f);
                            Talk(SAY_LEGOSO_5);
                            _moveTimer = 1500;
                            _phase = PHASE_CONTINUE;
                            break;
                        case PHASE_PLANT_FIRST_TIMER_1: // first explosives detonate timer 1
                            Talk(SAY_LEGOSO_6);
                            _moveTimer = 1000;
                            _phase = PHASE_PLANT_FIRST_TIMER_2;
                            break;
                        case PHASE_PLANT_FIRST_TIMER_2: // first explosives detonate timer 2
                            Talk(SAY_LEGOSO_7);
                            _moveTimer = 1000;
                            _phase = PHASE_PLANT_FIRST_TIMER_3;
                            break;
                        case PHASE_PLANT_FIRST_TIMER_3: // first explosives detonate timer 3
                            Talk(SAY_LEGOSO_8);
                            _moveTimer = 1000;
                            _phase = PHASE_PLANT_FIRST_DETONATE;
                            break;
                        case PHASE_PLANT_FIRST_DETONATE: // first explosives detonate finish
                            for (std::list<GameObject*>::iterator itr = _explosives.begin(); !_explosives.empty();)
                            {
                                me->RemoveGameObject(*itr, true);
                                _explosives.erase(itr);
                                itr = _explosives.begin();
                            }
                            me->HandleEmoteCommand(EMOTE_ONESHOT_CHEER);
                            _moveTimer = 2000;
                            _phase = PHASE_PLANT_FIRST_SPEECH;
                            break;
                        case PHASE_PLANT_FIRST_SPEECH: // after detonation 1 speech
                            Talk(SAY_LEGOSO_9);
                            _moveTimer = 4000;
                            _phase = PHASE_PLANT_FIRST_ROTATE;
                            break;
                        case PHASE_PLANT_FIRST_ROTATE: // after detonation 1 rotate to next point
                            me->SetFacingTo(2.272f);
                            _moveTimer = 1000;
                            _phase = PHASE_PLANT_FIRST_POINT;
                            break;
                        case PHASE_PLANT_FIRST_POINT: // after detonation 1 send point anim and go on to next point
                            me->HandleEmoteCommand(EMOTE_ONESHOT_POINT);
                            _moveTimer = 2000;
                            _phase = PHASE_CONTINUE;
                            break;
                        case PHASE_FEEL_SIRONAS_1: // legoso exclamation before sironas 1.1
                            Talk(SAY_LEGOSO_10);
                            _moveTimer = 4000;
                            _phase = PHASE_FEEL_SIRONAS_2;
                            break;
                        case PHASE_FEEL_SIRONAS_2: // legoso exclamation before sironas 1.2
                            Talk(SAY_LEGOSO_11, GetEventStarterGUID());
                            _moveTimer = 4000;
                            _phase = PHASE_CONTINUE;
                            break;
                        case PHASE_MEET_SIRONAS_ROAR: // legoso exclamation before sironas 2.1
                            Talk(SAY_LEGOSO_12);
                            _moveTimer = 4000;
                            _phase = PHASE_MEET_SIRONAS_TURN;
                            break;
                        case PHASE_MEET_SIRONAS_TURN: // legoso exclamation before sironas 2.2
                            if (Player* player = GetPlayerForEscort())
                                me->SetFacingTo(me->GetAngle(player));
                            _moveTimer = 1000;
                            _phase = PHASE_MEET_SIRONAS_SPEECH;
                            break;
                        case PHASE_MEET_SIRONAS_SPEECH: // legoso exclamation before sironas 2.3
                            Talk(SAY_LEGOSO_13);
                            _moveTimer = 7000;
                            _phase = PHASE_CONTINUE;
                            break;
                        case PHASE_PLANT_SECOND_KNEEL: // plant second explosives stage 1 kneel
                            me->SetStandState(UNIT_STAND_STATE_KNEEL);
                            _moveTimer = 11000;
                            _phase = PHASE_PLANT_SECOND_SPEECH;
                            break;
                        case PHASE_PLANT_SECOND_SPEECH: // plant second explosives stage 2 kneel
                            Talk(SAY_LEGOSO_14);
                            _moveTimer = 13000;
                            _phase = PHASE_PLANT_SECOND_STAND;
                            break;
                        case PHASE_PLANT_SECOND_STAND: // plant second explosives finish
                            me->SetStandState(UNIT_STAND_STATE_STAND);
                            _moveTimer = 1000;
                            _phase = PHASE_PLANT_SECOND_FINISH;
                            break;
                        case PHASE_PLANT_SECOND_FINISH: // plant second explosives finish - create explosives
                            for (uint8 i = 0; i != MAX_EXPLOSIVES; ++i)
                            {
                                if (GameObject* explosive = me->SummonGameObject(GO_DRAENEI_EXPLOSIVES_2, ExplosivesPos[1][i].m_positionX, ExplosivesPos[1][i].m_positionY, ExplosivesPos[1][i].m_positionZ, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0))
                                    _explosives.push_back(explosive);
                            }
                            Talk(SAY_LEGOSO_15, GetEventStarterGUID());
                            _moveTimer = 1000;
                            _phase = PHASE_PLANT_SECOND_WAIT;
                            break;
                        case PHASE_PLANT_SECOND_WAIT: // plant second explosives finish - proceed to next point
                            _moveTimer = 1000;
                            _phase = PHASE_CONTINUE;
                            break;
                        case PHASE_PLANT_SECOND_TIMER_1: // second explosives detonate timer 1
                            Talk(SAY_LEGOSO_16);
                            _moveTimer = 1000;
                            _phase = PHASE_PLANT_SECOND_TIMER_2;
                            break;
                        case PHASE_PLANT_SECOND_TIMER_2: // second explosives detonate timer 2
                            Talk(SAY_LEGOSO_17);
                            _moveTimer = 1000;
                            _phase = PHASE_PLANT_SECOND_TIMER_3;
                            break;
                        case PHASE_PLANT_SECOND_TIMER_3: // second explosives detonate timer 3
                            Talk(SAY_LEGOSO_18);
                            _moveTimer = 1000;
                            _phase = PHASE_PLANT_SECOND_DETONATE;
                            break;
                        case PHASE_PLANT_SECOND_DETONATE: // second explosives detonate finish
                            if (Creature* sironas = me->FindNearestCreature(NPC_SIRONAS, SIZE_OF_GRIDS))
                                sironas->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_IMMUNE_TO_PC | UNIT_FLAG_IMMUNE_TO_NPC);
                            for (std::list<GameObject*>::iterator itr = _explosives.begin(); !_explosives.empty();)
                            {
                                me->RemoveGameObject(*itr, true);
                                _explosives.erase(itr);
                                itr = _explosives.begin();
                            }
                            me->SetFacingTo(0.64f);
                            _moveTimer = 1000;
                            _phase = PHASE_FIGHT_SIRONAS_STOP;
                            break;
                        case PHASE_FIGHT_SIRONAS_STOP: // sironas channel stop
                            if (Creature* sironas = me->FindNearestCreature(NPC_SIRONAS, SIZE_OF_GRIDS))
                                sironas->AI()->DoAction(ACTION_SIRONAS_CHANNEL_STOP);
                            _moveTimer = 1000;
                            _phase = PHASE_FIGHT_SIRONAS_SPEECH_1;
                            break;
                        case PHASE_FIGHT_SIRONAS_SPEECH_1: // sironas exclamation before aggro
                            if (Creature* sironas = me->FindNearestCreature(NPC_SIRONAS, SIZE_OF_GRIDS))
                                sironas->AI()->Talk(SAY_SIRONAS_1);
                            _moveTimer = 1000;
                            _phase = PHASE_FIGHT_SIRONAS_SPEECH_2;
                            break;
                        case PHASE_FIGHT_SIRONAS_SPEECH_2: // legoso exclamation before aggro
                            if (Creature* sironas = me->FindNearestCreature(NPC_SIRONAS, SIZE_OF_GRIDS))
                                sironas->SetObjectScale(3.0f);
                            Talk(SAY_LEGOSO_19);
                            _moveTimer = 1000;
                            _phase = PHASE_FIGHT_SIRONAS_START;
                            break;
                        case PHASE_FIGHT_SIRONAS_START: // legoso exclamation at aggro
                            if (Creature* sironas = me->FindNearestCreature(NPC_SIRONAS, SIZE_OF_GRIDS))
                            {
                                Unit* target = GetPlayerForEscort();
                                if (!target)
                                    target = me;

                                target->AddThreat(sironas, 0.001f);
                                sironas->Attack(target, true);
                                sironas->GetMotionMaster()->MoveChase(target);
                            }
                            _moveTimer = 10000;
                            _phase = PHASE_CONTINUE;
                            break;
                        case PHASE_SIRONAS_SLAIN_SPEECH_1: // legoso exclamation after battle - stage 1.1
                            Talk(SAY_LEGOSO_20);
                            _moveTimer = 2000;
                            _phase = PHASE_SIRONAS_SLAIN_EMOTE_1;
                            break;
                        case PHASE_SIRONAS_SLAIN_EMOTE_1: // legoso exclamation after battle - stage 1.2
                            me->HandleEmoteCommand(EMOTE_ONESHOT_EXCLAMATION);
                            _moveTimer = 2000;
                            _phase = PHASE_SIRONAS_SLAIN_EMOTE_2;
                            break;
                        case PHASE_SIRONAS_SLAIN_EMOTE_2: // legoso exclamation after battle - stage 1.3
                            if (Player* player = GetPlayerForEscort())
                                player->GroupEventHappens(QUEST_ENDING_THEIR_WORLD, me);
                            me->HandleEmoteCommand(EMOTE_ONESHOT_CHEER);
                            _moveTimer = 5000;
                            _phase = PHASE_SIRONAS_SLAIN_SPEECH_2;
                            break;
                        case PHASE_SIRONAS_SLAIN_SPEECH_2: // legoso exclamation after battle - stage 2
                            Talk(SAY_LEGOSO_21);
                            _moveTimer = 30000;
                            _phase = PHASE_CONTINUE;
                            break;
                        default:
                            break;
                    }
                }
                else if (!me->isInCombat())
                    _moveTimer -= diff;
            }
        }

        void WaypointReached(uint32 waypointId)
        {
            Player* player = GetPlayerForEscort();
            if (!player)
                return;

            switch (waypointId)
            {
                case WP_START:
                    SetEscortPaused(true);
                    me->SetFacingTo(me->GetAngle(player));
                    Talk(SAY_LEGOSO_1, player->GetGUID());
                    _moveTimer = 2500;
                    _phase = PHASE_CONTINUE;
                    break;
                case WP_EXPLOSIVES_FIRST_POINT:
                    SetEscortPaused(true);
                    Talk(SAY_LEGOSO_2);
                    _moveTimer = 8000;
                    _phase = PHASE_WP_22;
                    break;
                case WP_EXPLOSIVES_FIRST_PLANT:
                    me->SetFacingTo(1.46f);
                    break;
                case WP_EXPLOSIVES_FIRST_DETONATE:
                    SetEscortPaused(true);
                    me->SetFacingTo(1.05f);
                    _moveTimer = 1000;
                    _phase = PHASE_PLANT_FIRST_TIMER_1;
                    break;
                case WP_DEBUG_1:
                    SetEscortPaused(true);
                    _moveTimer = 500;
                    _phase = PHASE_WP_26;
                    break;
                case WP_SIRONAS_HILL:
                {
                    SetEscortPaused(true);

                    //Find Sironas and make it respawn if needed
                    CellCoord p(Trinity::ComputeCellCoord(me->GetPositionX(), me->GetPositionY()));
                    Cell cell(p);
                    cell.SetNoCreate();

                    Creature* sironas = NULL;
                    Trinity::AllCreaturesOfEntryInRange check(me, NPC_SIRONAS, SIZE_OF_GRIDS);
                    Trinity::CreatureSearcher<Trinity::AllCreaturesOfEntryInRange> searcher(me, sironas, check);

                    cell.Visit(p, Trinity::makeGridVisitor(searcher), *(me->GetMap()), *me, SIZE_OF_GRIDS);

                    if (sironas)
                    {
                        if (!sironas->IsAlive())
                            sironas->Respawn(true);

                        sironas->AI()->DoAction(ACTION_SIRONAS_CHANNEL_START);
                        me->SetFacingTo(me->GetAngle(sironas));
                    }
                    _moveTimer = 1000;
                    _phase = PHASE_FEEL_SIRONAS_1;
                    break;
                }
                case WP_EXPLOSIVES_SECOND_BATTLEROAR:
                    SetEscortPaused(true);
                    _moveTimer = 200;
                    _phase = PHASE_MEET_SIRONAS_ROAR;
                    break;
                case WP_EXPLOSIVES_SECOND_PLANT:
                    SetEscortPaused(true);
                    _moveTimer = 500;
                    _phase = PHASE_PLANT_SECOND_KNEEL;
                    break;
                case WP_EXPLOSIVES_SECOND_DETONATE:
                    SetEscortPaused(true);
                    me->SetFacingTo(5.7f);
                    _moveTimer = 2000;
                    _phase = PHASE_PLANT_SECOND_TIMER_1;
                    break;
                default:
                    break;
            }
        }

        void DoAction(int32 const param)
        {
            switch (param)
            {
                case ACTION_LEGOSO_SIRONAS_KILLED:
                    _phase = PHASE_SIRONAS_SLAIN_SPEECH_1;
                    _moveTimer = 5000;
                    break;
                default:
                    break;
            }
        }

    private:
        void ResetEvents()
        {
            _events.Reset();
            _events.ScheduleEvent(EVENT_FROST_SHOCK, 1 * IN_MILLISECONDS);
            _events.ScheduleEvent(EVENT_HEALING_SURGE, 5 * IN_MILLISECONDS);
            _events.ScheduleEvent(EVENT_SEARING_TOTEM, 15 * IN_MILLISECONDS);
            _events.ScheduleEvent(EVENT_STRENGTH_OF_EARTH_TOTEM, 20 * IN_MILLISECONDS);
        }

        int8 _phase;
        uint32 _moveTimer;
        uint32 _eventStarterGuidLow;
        std::list<GameObject*> _explosives;
        EventMap _events;
    };
};

void AddSC_bloodmyst_isle()
{
    new mob_webbed_creature();
    new npc_captured_sunhawk_agent();
    new npc_princess_stillpine();
    new go_princess_stillpines_cage();
    new npc_sironas();
    new npc_demolitionist_legoso();
}
