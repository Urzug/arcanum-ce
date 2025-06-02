#ifndef ARCANUM_GAME_MP_UTILS_H_
#define ARCANUM_GAME_MP_UTILS_H_

#include "game/anim_private.h"
#include "game/broadcast.h"
#include "game/context.h"
#include "game/combat.h"
#include "game/dialog.h"
#include "game/obj.h"
#include "game/target.h"
#include "game/magictech.h"
#include "game/script.h"
#include "game/ui.h"

typedef struct PacketGamePlayerList {
    /* 0000 */ int type;
    /* 0004 */ ObjectID oids[8];
} PacketGamePlayerList;

static_assert(sizeof(PacketGamePlayerList) == 0xC8, "wrong size");

typedef struct PacketGameTime {
    /* 0000 */ int type;
    /* 0008 */ uint64_t game_time;
    /* 0010 */ uint64_t anim_time;
} PacketGameTime;

static_assert(sizeof(PacketGameTime) == 0x18, "wrong size");

typedef struct Packet4 {
    /* 0000 */ int type;
    /* 0004 */ int subtype;
    /* 0008 */ ObjectID oid;
    /* 0020 */ int64_t loc;
} Packet4;

static_assert(sizeof(Packet4) == 0x28, "wrong size");

typedef struct Packet5 {
    /* 0000 */ int type;
    /* 0008 */ DateTime field_8;
    /* 0010 */ AnimGoalData field_10;
    /* 0188 */ int64_t loc;
    /* 0190 */ int offset_x;
    /* 0194 */ int offset_y;
    /* 0198 */ AnimID field_198;
    /* 01A4 */ int field_1A4;
} Packet5;

static_assert(sizeof(Packet5) == 0x1A8, "wrong size");

typedef struct Packet6 {
    /* 0000 */ int type;
    /* 0004 */ int subtype;
    /* 0008 */ tig_art_id_t art_id;
    /* 0010 */ ObjectID self_oid;
    /* 0028 */ ObjectID target_oid;
    /* 0040 */ int64_t loc;
    /* 0048 */ int64_t target_loc;
    /* 0050 */ int spell;
    /* 0054 */ AnimID anim_id;
    /* 0060 */ ObjectID obj_oid;
} Packet6;

static_assert(sizeof(Packet6) == 0x78, "wrong size");

typedef struct Packet7 {
    /* 0000 */ int type;
    /* 0004 */ AnimID anim_id;
    /* 0010 */ AnimGoalData goal_data;
    /* 0188 */ int64_t loc;
    /* 0190 */ int offset_x;
    /* 0194 */ int offset_y;
} Packet7;

static_assert(sizeof(Packet7) == 0x198, "wrong size");

typedef struct Packet8 {
    /* 0000 */ int type;
    /* 0004 */ int field_4;
    /* 0008 */ AGModifyData modify_data;
    /* 0038 */ int offset_x;
    /* 003C */ int offset_y;
    /* 0040 */ int64_t field_40;
    /* 0048 */ int field_48;
    /* 004C */ int field_4C;
} Packet8;

static_assert(sizeof(Packet8) == 0x50, "wrong size");

typedef struct Packet9 {
    /* 0000 */ int type;
    /* 0004 */ int field_4;
    /* 0008 */ int priority_level;
    /* 000C */ int field_C;
    /* 0010 */ int field_10;
    /* 0014 */ int field_14;
    /* 0018 */ FollowerInfo field_18;
    /* 0048 */ int field_48;
    /* 004C */ int field_4C;
    /* 0050 */ int64_t loc;
    /* 0058 */ int offset_x;
    /* 005C */ int offset_y;
    /* 0060 */ int art_id;
    /* 0064 */ int field_64;
} Packet9;

static_assert(sizeof(Packet9) == 0x68, "wrong size");

typedef struct Packet10 {
    /* 0000 */ int type;
    /* 0004 */ AnimID anim_id;
    /* 0010 */ ObjectID oid;
    /* 0028 */ int64_t loc;
    /* 0030 */ int offset_x;
    /* 0034 */ int offset_y;
    /* 0038 */ tig_art_id_t art_id;
    /* 003C */ int flags;
} Packet10;

static_assert(sizeof(Packet10) == 0x40, "wrong size");

typedef struct Packet16 {
    /* 0000 */ int type;
    /* 0004 */ AnimID anim_id;
    /* 0010 */ int64_t loc;
    /* 0018 */ int offset_x;
    /* 001C */ int offset_y;
    /* 0020 */ int art_id;
    /* 0024 */ int anim_flags;
    /* 0028 */ int path_flags;
    /* 002C */ int field_2C;
    /* 0030 */ int path_base_rot;
    /* 0034 */ int path_curr;
    /* 0038 */ int path_max;
    /* 003C */ int path_subsequence;
    /* 0040 */ int path_max_path_length;
    /* 0044 */ int path_abs_max_path_length;
    /* 0048 */ int64_t field_48;
    /* 0050 */ int64_t field_50;
} Packet16;

// NOTE: May be wrong, see 0x4ED510.
static_assert(sizeof(Packet16) == 0x58, "wrong size");

typedef struct PacketBroadcastMsg {
    int type;
    ObjectID oid;
    Broadcast bcast;
} PacketBroadcastMsg;

static_assert(sizeof(PacketBroadcastMsg) == 0xA8, "wrong size");

typedef struct PacketCombatModeSet {
    /* 0000 */ int type;
    /* 0008 */ ObjectID oid;
    /* 0020 */ bool active;
} PacketCombatModeSet;

static_assert(sizeof(PacketCombatModeSet) == 0x28, "wrong size");

typedef struct PacketCombatDmg {
    /* 0000 */ int type;
    /* 0008 */ CombatContext combat;
    /* 0070 */ ObjectID attacker_oid;
    /* 0088 */ ObjectID weapon_oid;
    /* 00A0 */ ObjectID target_oid;
    /* 00B8 */ ObjectID field_B8;
    /* 00D0 */ ObjectID field_D0;
} PacketCombatDmg;

static_assert(sizeof(PacketCombatDmg) == 0xE8, "wrong size");

typedef struct PacketCombatHeal {
    /* 0000 */ int type;
    /* 0008 */ CombatContext combat;
    /* 0070 */ ObjectID attacker_oid;
    /* 0088 */ ObjectID weapon_oid;
    /* 00A0 */ ObjectID target_oid;
    /* 00B8 */ ObjectID field_B8;
    /* 00D0 */ ObjectID field_D0;
} PacketCombatHeal;

static_assert(sizeof(PacketCombatHeal) == 0xE8, "wrong size");

typedef struct Packet22 {
    /* 0000 */ int type;
    /* 0008 */ ObjectID oid;
    /* 0020 */ int64_t loc;
} Packet22;

static_assert(sizeof(Packet22) == 0x28, "wrong size");

typedef struct Packet23 {
    /* 0000 */ int type;
    /* 0008 */ ObjectID item_oid;
    /* 0020 */ ObjectID parent_oid;
} Packet23;

static_assert(sizeof(Packet23) == 0x38, "wrong size");

typedef struct Packet24 {
    /* 0000 */ int type;
    /* 0008 */ ObjectID item_oid;
    /* 0020 */ ObjectID parent_oid;
    /* 0038 */ int inventory_location;
} Packet24;

static_assert(sizeof(Packet24) == 0x40, "wrong size");

typedef struct Packet25 {
    /* 0000 */ int type;
    /* 0004 */ int field_4;
    /* 0008 */ ObjectID item_oid;
    /* 0020 */ ObjectID parent_oid;
    /* 0038 */ int field_38;
    /* 003C */ int field_3C;
} Packet25;

typedef struct Packet26 {
    /* 0000 */ int type;
    /* 0008 */ ObjectID oid;
    /* 0020 */ int field_20;
} Packet26;

static_assert(sizeof(Packet26) == 0x28, "wrong size");

typedef struct Packet27 {
    /* 0000 */ int type;
    /* 0008 */ ObjectID oid;
    /* 0020 */ int64_t loc;
} Packet27;

static_assert(sizeof(Packet27) == 0x28, "wrong size");

typedef struct Packet28 {
    /* 0000 */ int type;
    /* 0008 */ ObjectID item_oid;
    /* 0020 */ ObjectID critter_oid;
    /* 0038 */ int inventory_location;
} Packet28;

static_assert(sizeof(Packet28) == 0x40, "wrong size");

typedef struct Packet29 {
    /* 0000 */ int type;
    /* 0008 */ ObjectID oid;
} Packet29;

static_assert(sizeof(Packet29) == 0x20, "wrong size");

typedef struct Packet30 {
    /* 0000 */ int type;
    /* 0004 */ int field_4;
    /* 0008 */ FollowerInfo field_8;
    /* 0038 */ int field_38;
} Packet30;

static_assert(sizeof(Packet30) == 0x40, "wrong size");

typedef struct Packet31 {
    /* 0000 */ int type;
    /* 0004 */ int field_4;
    /* 0008 */ FollowerInfo field_8;
    /* 0038 */ int field_38;
    /* 003C */ int field_3C;
    /* 0040 */ S4F2810 field_40;
    /* 0050 */ ObjectID field_50;
} Packet31;

static_assert(sizeof(Packet31) == 0x68, "wrong size");

typedef struct PacketCritterConcealSet {
    /* 0000 */ int type;
    /* 0008 */ FollowerInfo field_8;
    /* 0038 */ int concealed;
} PacketCritterConcealSet;

static_assert(sizeof(PacketCritterConcealSet) == 0x40, "wrong size");

typedef struct PacketCritterRestingHeal {
    /* 0000 */ int type;
    /* 0008 */ ObjectID oid;
    /* 0020 */ int hours;
} PacketCritterRestingHeal;

static_assert(sizeof(PacketCritterRestingHeal) == 0x28, "wrong size");

typedef struct PacketCritterFatigueDamageSet {
    /* 0000 */ int type;
    /* 0008 */ ObjectID oid;
    /* 0020 */ int dam;
} PacketCritterFatigueDamageSet;

static_assert(sizeof(PacketCritterFatigueDamageSet) == 0x28, "wrong size");

#define FATE_STATE_ACTION_DEACTIVATE 0
#define FATE_STATE_ACTION_ACTIVATE 1

typedef struct PacketFateStateSet {
    /* 0000 */ int type;
    /* 0008 */ FollowerInfo oid;
    /* 0038 */ int fate;
    /* 003C */ int action;
} PacketFateStateSet;

static_assert(sizeof(PacketFateStateSet) == 0x40, "wrong size");

typedef struct PacketRumorQStateSet {
    int type;
    int rumor;
} PacketRumorQStateSet;

static_assert(sizeof(PacketRumorQStateSet) == 0x8, "wrong size");

typedef struct PacketRumorKnownSet {
    /* 0000 */ int type;
    /* 0004 */ int rumor;
    /* 0008 */ ObjectID oid;
    /* 0020 */ DateTime datetime;
} PacketRumorKnownSet;

static_assert(sizeof(PacketRumorKnownSet) == 0x28, "wrong size");

typedef struct PacketQuestStateSet {
    /* 0000 */ int type;
    /* 0008 */ FollowerInfo field_8;
    /* 0038 */ int quest;
    /* 003C */ int state;
    /* 0040 */ FollowerInfo field_40;
} PacketQuestStateSet;

static_assert(sizeof(PacketQuestStateSet) == 0x70, "wrong size");

typedef struct PacketQuestUnbotch {
    /* 0000 */ int type;
    /* 0008 */ FollowerInfo field_8;
    /* 0038 */ int quest;
} PacketQuestUnbotch;

static_assert(sizeof(PacketQuestUnbotch) == 0x40, "wrong size");

typedef struct PacketQuestGlobalStateSet {
    /* 0000 */ int type;
    /* 0004 */ int quest;
    /* 0008 */ int state;
} PacketQuestGlobalStateSet;

static_assert(sizeof(PacketQuestGlobalStateSet) == 0xC, "wrong size");

typedef struct ChangeBlessPacket {
    /* 0000 */ int type;
    /* 0004 */ int field_4;
    /* 0008 */ FollowerInfo field_8;
    /* 0038 */ int bless;
    /* 003C */ int add;
} ChangeBlessPacket;

static_assert(sizeof(ChangeBlessPacket) == 0x40, "wrong size");

typedef struct ChangeCursePacket {
    /* 0000 */ int type;
    /* 0004 */ int field_4;
    /* 0008 */ FollowerInfo field_8;
    /* 0038 */ int curse;
    /* 003C */ int add;
} ChangeCursePacket;

static_assert(sizeof(ChangeCursePacket) == 0x40, "wrong size");

typedef struct PacketDialog {
    /* 0000 */ int type;
    /* 0004 */ int subtype;
    union {
        struct {
            /* 0008 */ ObjectID field_8;
        } b;
        struct {
            /* 0008 */ ObjectID field_8;
            /* 0020 */ ObjectID field_20;
            /* 0038 */ int field_38;
            /* 003C */ int field_3C;
            /* 0040 */ int field_40;
            /* 0044 */ char field_44[1000];
        } d;
        struct {
            /* 0008 */ int field_8;
            /* 000C */ int field_C;
            /* 0010 */ int field_10;
            /* 0014 */ int field_14;
            /* 0018 */ DialogSerializedData serialized_data;
        } e;
        struct {
            /* 0008 */ ObjectID field_8;
            /* 0020 */ int field_20;
            /* 0024 */ int field_24;
        } f;
    } d;
} PacketDialog;

static_assert(sizeof(PacketDialog) == 0x430, "wrong size");

typedef struct Packet46 {
    /* 0000 */ int type;
    /* 0004 */ int player;
    /* 0008 */ int field_8;
    /* 000C */ int field_C;
    /* 0010 */ int field_10;
    /* 0014 */ int field_14;
    /* 0018 */ int field_18;
    /* 001C */ int field_1C;
} Packet46;

static_assert(sizeof(Packet46) == 0x20, "wrong size");

typedef struct PlayerBuySpellPacket {
    /* 0000 */ int type;
    /* 0004 */ int player;
    /* 0008 */ int spell;
    /* 000C */ bool force;
} PlayerBuySpellPacket;

static_assert(sizeof(PlayerBuySpellPacket) == 0x10, "wrong size");

typedef struct SetBaseStatPacket {
    /* 0000 */ int type;
    /* 0004 */ int stat;
    /* 0008 */ int value;
    /* 0010 */ ObjectID oid;
} SetBaseStatPacket;

static_assert(sizeof(SetBaseStatPacket) == 0x28, "wrong size");

typedef struct Packet51 {
    /* 0000 */ int type;
    /* 0004 */ int field_4;
    /* 0008 */ ObjectID field_8;
    /* 0020 */ int field_20;
} Packet51;

static_assert(sizeof(Packet51) == 0x28, "wrong size");

typedef struct SetSkillTrainingPacket {
    /* 0000 */ int type;
    /* 0008 */ FollowerInfo field_8;
    /* 0038 */ int skill;
    /* 003C */ int training;
} SetSkillTrainingPacket;

static_assert(sizeof(SetSkillTrainingPacket) == 0x40, "wrong size");

typedef struct Packet54 {
    /* 0000 */ int type;
    /* 0004 */ int magictech_id;
} Packet54;

static_assert(sizeof(Packet54) == 0x08, "wrong size");

typedef struct PacketPlayerRequestCastSpell {
    /* 0000 */ int type;
    /* 0004 */ int player;
    /* 0008 */ MagicTechInvocation invocation;
} PacketPlayerRequestCastSpell;

static_assert(sizeof(PacketPlayerRequestCastSpell) == 0xE8, "wrong size");

typedef struct PacketPlayerCastSpell {
    /* 0000 */ int type;
    /* 0004 */ int player;
    /* 0008 */ MagicTechInvocation invocation;
} PacketPlayerCastSpell;

static_assert(sizeof(PacketPlayerCastSpell) == 0xE8, "wrong size");

typedef struct PacketPlayerInterruptSpell {
    /* 0000 */ int type;
    /* 0004 */ int mt_id;
} PacketPlayerInterruptSpell;

static_assert(sizeof(PacketPlayerInterruptSpell) == 0x08, "wrong size");

typedef struct PacketPlayerSpellMaintainAdd {
    /* 0000 */ int type;
    /* 0004 */ int mt_id;
    /* 0008 */ int player;
} PacketPlayerSpellMaintainAdd;

static_assert(sizeof(PacketPlayerSpellMaintainAdd) == 0xC, "wrong size");

typedef struct PacketPlayerSpellMaintainEnd {
    /* 0000 */ int type;
    /* 0004 */ int mt_id;
    /* 0008 */ int player;
} PacketPlayerSpellMaintainEnd;

static_assert(sizeof(PacketPlayerSpellMaintainEnd) == 0xC, "wrong size");

typedef struct Packet64 {
    /* 0000 */ int type;
    /* 0004 */ int player;
    /* 0008 */ int map;
    /* 000C */ char name[TIG_MAX_PATH];
    /* 0110 */ int field_110;
    /* 0114 */ int field_114;
    /* 0118 */ int field_118;
    /* 011C */ int field_11C;
} Packet64;

static_assert(sizeof(Packet64) == 0x120, "wrong size");

typedef struct Packet67 {
    /* 0000 */ int type;
    /* 0004 */ int field_4;
} Packet67;

static_assert(sizeof(Packet67) == 0x08, "wrong size");

typedef struct PacketNotifyPlayerLagging {
    /* 0000 */ int type;
    /* 0008 */ FollowerInfo field_8;
} PacketNotifyPlayerLagging;

static_assert(sizeof(PacketNotifyPlayerLagging) == 0x38, "wrong size");

typedef struct PacketNotifyPlayerRecovered {
    /* 0000 */ int type;
    /* 0008 */ FollowerInfo field_8;
} PacketNotifyPlayerRecovered;

static_assert(sizeof(PacketNotifyPlayerRecovered) == 0x38, "wrong size");

typedef struct Packet70 {
    /* 0000 */ int type;
    /* 0004 */ int subtype;
    union {
        struct {
            /* 0008 */ int name;
            /* 000C */ int field_C;
            /* 0010 */ ObjectID oid;
            /* 0028 */ int64_t loc;
            /* 0030 */ int field_30;
            /* 0034 */ int field_34;
            /* 0038 */ ObjectID field_38;
            /* 0050 */ int field_50;
            /* 0054 */ int field_54;
            /* 0058 */ int field_58;
            /* 005C */ int field_5C;
            /* 0060 */ int field_60;
            /* 0064 */ int field_64;
        } s0;
        struct {
            /* 0008 */ ObjectID oid;
            /* 0020 */ int64_t loc;
            /* 0028 */ int offset_x;
            /* 002C */ int offset_y;
        } s5;
    };
} Packet70;

static_assert(sizeof(Packet70) == 0x68, "wrong size");

typedef struct PacketPartyUpdate {
    int type;
    int party[8];
} PacketPartyUpdate;

static_assert(sizeof(PacketPartyUpdate) == 0x24, "wrong size");

typedef struct PacketObjectDestroy {
    /* 0000 */ int type;
    /* 0008 */ ObjectID oid;
} PacketObjectDestroy;

static_assert(sizeof(PacketObjectDestroy) == 0x20, "wrong size");

typedef struct PacketSummon {
    /* 0000 */ int type;
    /* 0008 */ MagicTechSummonInfo summon_info;
} PacketSummon;

static_assert(sizeof(PacketSummon) == 0xD8, "wrong size");

typedef struct Packet74 {
    /* 0000 */ int type;
    /* 0004 */ int subtype;
    /* 0008 */ ObjectID oid;
    union {
        /* 0020 */ int mt_id;
        struct {
            /* 0020 */ MagicTechComponentTrait trait;
            /* 0038 */ int obj_type;
        };
    };
} Packet74;

static_assert(sizeof(Packet74) == 0x40, "wrong size");

typedef struct PacketMagicTechObjFlag {
    /* 0000 */  int type;
    /* 0008 */  ObjectID field_8;
    /* 0020 */  ObjectID self_oid;
    /* 0038 */  int fld;
    /* 003C */  int value;
    /* 0040 */  int state;
    /* 0048 */  ObjectID parent_oid;
    /* 0060 */  ObjectID source_oid;
} PacketMagicTechObjFlag;

static_assert(sizeof(PacketMagicTechObjFlag) == 0x78, "wrong size");

typedef struct Packet76 {
    /* 0000 */ int type;
    /* 0008 */ ObjectID oid;
    /* 0020 */ int cost;
    /* 0024 */ bool field_24;
    /* 0028 */ int magictech;
} Packet76;

static_assert(sizeof(Packet76) == 0x30, "wrong size");

typedef struct PacketMagicTechEyeCandy {
    /* 0000 */ int type;
    /* 0004 */ int subtype;
    /* 0008 */ ObjectID oid;
    /* 0020 */ int fx_id;
    /* 0024 */ int mt_id;
} PacketMagicTechEyeCandy;

static_assert(sizeof(PacketMagicTechEyeCandy) == 0x28, "wrong size");

typedef struct Packet79 {
    /* 0000 */ int type;
    /* 0004 */ int field_4;
    /* 0008 */ ObjectID field_8;
    /* 0020 */ ObjectID field_20;
    /* 003C */ int field_3C;
} Packet79;

static_assert(sizeof(Packet79) == 0x40, "wrong size");

typedef struct Packet80 {
    /* 0000 */ int type;
    /* 0008 */ ObjectID item_oid;
    /* 0020 */ ObjectID parent_oid;
    /* 0038 */ int idx;
    /* 003C */ int field_3C;
} Packet80;

typedef struct Packet81 {
    /* 0000 */ int type;
    /* 0004 */ int field_4;
    /* 0008 */ int field_8;
    /* 000C */ int field_C;
    /* 0010 */ ObjectID field_10;
    /* 0028 */ ObjectID field_28;
    /* 0040 */ ObjectID field_40;
    /* 0058 */ int field_58;
    /* 005C */ int field_5C;
    /* 0060 */ int field_60;
    /* 0064 */ int field_64;
    /* 0068 */ int field_68;
    /* 006C */ int field_6C;
    /* 0070 */ ObjectID field_70;
    /* 0088 */ ObjectID field_88;
    /* 00A0 */ ObjectID field_A0;
} Packet81;

static_assert(sizeof(Packet81) == 0xB8, "wrong size");

typedef struct Packet82 {
    /* 0000 */ int type;
    /* 0004 */ int field_4;
    /* 0008 */ ObjectID field_8;
    /* 0020 */ ObjectID field_20;
    /* 0038 */ int field_38;
    /* 003C */ int field_3C;
    /* 0040 */ ObjectID field_40;
} Packet82;

static_assert(sizeof(Packet82) == 0x58, "wrong size");

typedef struct Packet83 {
    /* 0000 */ int type;
    /* 0004 */ int field_4;
    /* 0008 */ int field_8;
    /* 000C */ char field_C[128];
} Packet83;

static_assert(sizeof(Packet83) == 0x8C, "wrong size");

typedef struct Packet84 {
    /* 0000 */ int type;
    /* 0004 */ int extra_length;
    /* 0008 */ int field_8;
    /* 0010 */ UiMessage ui_message;
} Packet84;

static_assert(sizeof(Packet84) == 0x28, "wrong size");

typedef struct PacketTextFloater {
    /* 0000 */ int type;
    /* 0004 */ int extra_length;
    /* 0008 */ ObjectID oid;
    /* 0020 */ int tf_type;
} PacketTextFloater;

static_assert(sizeof(PacketTextFloater) == 0x28, "wrong size");

#define PACKET_EFFECT_ADD 0
#define PACKET_EFFECT_REMOVE_ONE_BY_TYPE 1
#define PACKET_EFFECT_REMOVE_ALL_BY_TYPE 2
#define PACKET_EFFECT_REMOVE_ONE_BY_CAUSE 3
#define PACKET_EFFECT_REMOVE_ALL_BY_CAUSE 4

typedef struct PacketEffect {
    /* 0000 */ int type;
    /* 0004 */ int subtype;
    /* 0008 */ ObjectID oid;
    union {
        struct {
            /* 0020 */ int effect;
            /* 0024 */ int cause;
        } add;
        union {
            /* 0020 */ int effect;
            /* 0020 */ int cause;
        } remove;
    };
} PacketEffect;

static_assert(sizeof(PacketEffect) == 0x28, "wrong size");

typedef struct PacketReactionAdj {
    /* 0000 */ int type;
    /* 0008 */ ObjectID npc_oid;
    /* 0020 */ ObjectID pc_oid;
    /* 0038 */ int value;
} PacketReactionAdj;

static_assert(sizeof(PacketReactionAdj) == 0x40, "wrong size");

#define REPAIR_INVOCATION_HP_DAM 0x01u
#define REPAIR_INVOCATION_FIX 0x02u
#define REPAIR_INVOCATION_HP_ADJ 0x04u
#define REPAIR_INVOCATION_NOTIFY_UI 0x08u
#define REPAIR_INVOCATION_NO_REPAIR 0x10u

typedef struct RepairInvocation {
    /* 0000 */ int type;
    /* 0004 */ int flags;
    /* 0008 */ int hp_dam;
    /* 000C */ int hp_adj;
    /* 0010 */ ObjectID target_oid;
    /* 0028 */ ObjectID source_oid;
    /* 0040 */ int success;
} RepairInvocation;

static_assert(sizeof(RepairInvocation) == 0x48, "wrong size");

#define PICK_LOCK_INVOCATION_LOCK 0x01u
#define PICK_LOCK_INVOCATION_JAM 0x02u
#define PICK_LOCK_INVOCATION_UNLOCK 0x04u
#define PICK_LOCK_INVOCATION_NOTIFY_UI 0x08u

typedef struct PacketPickLockInvocation {
    /* 0000 */ int type;
    /* 0004 */ unsigned int flags;
    /* 0008 */ ObjectID source_oid;
    /* 0020 */ ObjectID target_oid;
    /* 0038 */ int success;
} PacketPickLockInvocation;

static_assert(sizeof(PacketPickLockInvocation) == 0x40, "wrong size");

typedef struct PacketTrapMarkKnown {
    /* 0000 */ int type;
    /* 0008 */ ObjectID pc_oid;
    /* 0020 */ ObjectID trap_oid;
    /* 0038 */ int reason;
} PacketTrapMarkKnown;

static_assert(sizeof(PacketTrapMarkKnown) == 0x40, "wrong size");

#define DISARM_TRAP_INVOCATION_0x01 0x01u
#define DISARM_TRAP_INVOCATION_DISARM 0x02u

typedef struct PacketDisarmTrapInvocation {
    /* 0000 */ int type;
    /* 0008 */ ObjectID source_oid;
    /* 0020 */ ObjectID target_oid;
    /* 0038 */ int success;
    /* 003C */ unsigned int flags;
} PacketDisarmTrapInvocation;

static_assert(sizeof(PacketDisarmTrapInvocation) == 0x40, "wrong size");

typedef struct Packet92 {
    /* 0000 */ int type;
    /* 0008 */ ObjectID oid;
    /* 0020 */ tig_art_id_t art_id;
} Packet92;

static_assert(sizeof(Packet92) == 0x28, "wrong size");

typedef struct Packet93 {
    /* 0000 */ int type;
    /* 0000 */ int field_4;
    /* 0008 */ ObjectID oid;
    /* 0020 */ int field_20;
} Packet93;

static_assert(sizeof(Packet93) == 0x28, "wrong size");

typedef struct PacketUpdateInven {
    /* 0000 */ int type;
    /* 0008 */ ObjectID oid;
} PacketUpdateInven;

static_assert(sizeof(PacketUpdateInven) == 0x20, "wrong size");

typedef struct Packet95 {
    /* 0000 */ int type;
    /* 0008 */ ObjectID oid;
} Packet95;

static_assert(sizeof(Packet95) == 0x20, "wrong size");

typedef struct PacketArrangeInventory {
    /* 0000 */ int type;
    /* 0008 */ ObjectID oid;
    /* 0020 */ bool vertical;
} PacketArrangeInventory;

static_assert(sizeof(PacketArrangeInventory) == 0x28, "wrong size");

typedef struct Packet97 {
    /* 0000 */ int type;
    /* 0008 */ ObjectID field_8;
    /* 0020 */ ObjectID field_20;
} Packet97;

static_assert(sizeof(Packet97) == 0x38, "wrong size");

typedef struct PacketMultiplayerFlagsChange {
    /* 0000 */ int type;
    /* 0004 */ int client_id;
    /* 0008 */ unsigned int flags;
} PacketMultiplayerFlagsChange;

static_assert(sizeof(PacketMultiplayerFlagsChange) == 0xC, "wrong size");

// typedef struct Packet100 {
//     /* 0000 */ int type;
//     /* 0004 */ int subtype;
//     /* 0008 */ ObjectID field_8;
//     /* 0020 */ ObjectID field_20;
//     /* 0038 */ int field_38;
//     /* 003C */ int field_3C;
// } Packet100;

// static_assert(sizeof(Packet100) == 0x40, "wrong size");

typedef struct Packet99 {
    /* 0000 */ int type;
    /* 0008 */ ObjectID oid;
    /* 0020 */ int64_t location;
    /* 0028 */ int dx;
    /* 002C */ int dy;
    /* 0030 */ int field_30;
} Packet99;

static_assert(sizeof(Packet99) == 0x38, "wrong size");

typedef struct Packet100 {
    /* 0000 */ int type;
    /* 0004 */ int subtype;
    union {
        struct {
            /* 0008 */ int field_8;
            /* 000C */ int field_C;
        } a;
        struct {
            /* 0008 */ ObjectID field_8;
        } b;
        struct {
            /* 0008 */ int field_8;
            /* 0010 */ ObjectID field_10;
            /* 0028 */ ObjectID field_28;
        } c;
        struct {
            /* 0008 */ ObjectID field_8;
            /* 0020 */ ObjectID field_20;
        } s;
        struct {
            /* 0008 */ ObjectID field_8;
            /* 0020 */ int field_20;
        } x;
        struct {
            /* 0008 */ ObjectID field_8;
            /* 0020 */ ObjectID field_20;
            /* 0038 */ int field_38;
            /* 003C */ int field_3C;
        } z;
    } d;
} Packet100;

static_assert(sizeof(Packet100) == 0x40, "wrong size");

typedef struct PacketAreaKnownSet {
    /* 0000 */ int type;
    /* 0008 */ ObjectID oid;
    /* 0020 */ int area;
} PacketAreaKnownSet;

static_assert(sizeof(PacketAreaKnownSet) == 0x28, "wrong size");

typedef struct PacketAreaResetLastKnown {
    /* 0000 */ int type;
    /* 0008 */ ObjectID oid;
} PacketAreaResetLastKnown;

static_assert(sizeof(PacketAreaResetLastKnown) == 0x20, "wrong size");

typedef struct PacketObjectLock {
    /* 0000 */ int type;
    /* 0008 */ ObjectID oid;
    /* 0020 */ int locked;
} PacketObjectLock;

static_assert(sizeof(PacketObjectLock) == 0x28, "wrong size");

typedef struct Packet104 {
    /* 0000 */ int type;
    /* 0008 */ ObjectID oid;
} Packet104;

static_assert(sizeof(Packet104) == 0x20, "wrong size");

typedef struct PacketPlaySound {
    /* 0000 */ int type;
    /* 0004 */ int subtype;
    union {
        struct {
        /* 0008 */ int sound_id;
        /* 000C */ int loops;
        /* 0010 */ ObjectID oid;
        };
        struct {
        /* 0008 */ int music_scheme_idx;
        /* 000C */ int ambient_scheme_idx;
        };
    };
} PacketPlaySound;

static_assert(sizeof(PacketPlaySound) == 0x28, "wrong size");

typedef struct PacketPortalToggle {
    /* 0000 */ int type;
    /* 0008 */ ObjectID oid;
} PacketPortalToggle;

static_assert(sizeof(PacketPortalToggle) == 0x20, "wrong size");

typedef struct PacketSectorBlockSet {
    /* 0000 */ int type;
    /* 0008 */ int64_t sec;
    /* 0010 */ bool blocked;
} PacketSectorBlockSet;

static_assert(sizeof(PacketSectorBlockSet) == 0x18, "wrong size");

typedef struct PacketSpellMasterySet {
    /* 0000 */ int type;
    /* 0008 */ ObjectID oid;
    /* 0020 */ int college;
} PacketSpellMasterySet;

static_assert(sizeof(PacketSpellMasterySet) == 0x28, "wrong size");

typedef struct PacketTownmapSetKnown {
    /* 0000 */ int type;
    /* 0004 */ int map;
    /* 0008 */ bool known;
} PacketTownmapSetKnown;

static_assert(sizeof(PacketTownmapSetKnown) == 0xC, "wrong size");

typedef struct PacketArtTouch {
    /* 0000 */ int type;
    /* 0004 */ tig_art_id_t art_id;
} PacketArtTouch;

static_assert(sizeof(PacketArtTouch) == 0x8, "wrong size");

typedef struct PacketMapPrecacheSectors {
    /* 0000 */ int type;
    /* 0004 */ int field_4;
    /* 0008 */ int64_t loc;
    /* 0010 */ ObjectID oid;
} PacketMapPrecacheSectors;

static_assert(sizeof(PacketMapPrecacheSectors) == 0x28, "wrong size");

typedef struct PacketTextRemove {
    /* 0000 */ int type;
    /* 0004 */ int subtype;
    /* 0008 */ int action;
    /* 0010 */ ObjectID oid;
} PacketTextRemove;

static_assert(sizeof(PacketTextRemove) == 0x28, "wrong size");

typedef struct PacketItemUse {
    /* 0000 */ int type;
    /* 0008 */ ObjectID source_oid;
    /* 0020 */ ObjectID item_oid;
    /* 0038 */ ObjectID target_oid;
} PacketItemUse;

static_assert(sizeof(PacketItemUse) == 0x50, "wrong size");

typedef struct Packet118 {
    /* 0000 */ int type;
    /* 0008 */ ObjectID field_8;
    /* 0020 */ ObjectID field_20;
} Packet118;

static_assert(sizeof(Packet118) == 0x38, "wrong size");

typedef struct Packet119 {
    /* 0000 */ int type;
    /* 0008 */ ObjectID oid;
    /* 0020 */ int64_t loc;
    /* 0028 */ int field_28;
    /* 002C */ int field_2C;
} Packet119;

static_assert(sizeof(Packet119) == 0x30, "wrong size");

typedef struct Packet120 {
    /* 0000 */ int type;
    /* 0004 */ AnimID anim_id;
} Packet120;

static_assert(sizeof(Packet120) == 0x10, "wrong size");

typedef struct Packet121 {
    /* 0000 */ int type;
    /* 0004 */ int field_4;
    /* 0008 */ ObjectID oid;
    /* 0020 */ int field_20;
} Packet121;

static_assert(sizeof(Packet121) == 0x28, "wrong size");

typedef struct Packet122 {
    /* 0000 */ int type;
    /* 0004 */ int field_4;
    /* 0008 */ ObjectID oid;
} Packet122;

static_assert(sizeof(Packet122) == 0x20, "wrong size");

typedef struct Packet123 {
    /* 0000 */ int type;
    /* 0004 */ int total_size;
    /* 0008 */ int player;
    /* 000C */ int rule_length;
    /* 0010 */ int name_length;
} Packet123;

static_assert(sizeof(Packet123) == 0x14, "wrong size");

#define CHANGE_REPUTATION_ACTION_ADD 0
#define CHANGE_REPUTATION_ACTION_REMOVE 1

typedef struct PacketChangeReputation {
    /* 0000 */ int type;
    /* 0008 */ ObjectID pc_oid;
    /* 0020 */ int reputation;
    /* 0024 */ int action;
} PacketChangeReputation;

static_assert(sizeof(PacketChangeReputation) == 0x28, "wrong size");

#define SCRIPT_FUNC_SET_STORY_STATE 0
#define SCRIPT_FUNC_SET_GLOBAL_VAR 1
#define SCRIPT_FUNC_SET_GLOBAL_FLAG 2
#define SCRIPT_FUNC_END_GAME 3

typedef struct PacketScriptFunc {
    /* 0000 */ int type;
    /* 0004 */ int subtype;
    union {
        /* 0008 */ int story_state;
        struct {
            /* 0008 */ int index;
            /* 000C */ int value;
        };
    };
} PacketScriptFunc;

static_assert(sizeof(PacketScriptFunc) == 0x10, "wrong size");

typedef struct PacketPerformIdentifyService {
    /* 0000 */ int type;
    /* 0008 */ ObjectID item_oid;
    /* 0020 */ ObjectID npc_oid;
    /* 0038 */ ObjectID pc_oid;
    /* 0050 */ int cost;
} PacketPerformIdentifyService;

static_assert(sizeof(PacketPerformIdentifyService) == 0x58, "wrong size");

typedef struct PacketPerformRepairService {
    /* 0000 */ int type;
    /* 0008 */ ObjectID item_oid;
    /* 0020 */ ObjectID npc_oid;
    /* 0038 */ ObjectID pc_oid;
    /* 0050 */ int cost;
} PacketPerformRepairService;

static_assert(sizeof(PacketPerformRepairService) == 0x58, "wrong size");

typedef enum PacketChareditTraitChangeSubtype {
    PACKET_CHAREDIT_TRAIT_CHANGE_SUBTYPE_CACHE,
    PACKET_CHAREDIT_TRAIT_CHANGE_SUBTYPE_INC,
    PACKET_CHAREDIT_TRAIT_CHANGE_SUBTYPE_DEC,
} PacketChareditTraitChangeSubtype;

typedef struct PacketChareditTraitChange {
    /* 0000 */ int type;
    /* 0004 */ int subtype;
    /* 0008 */ int trait;
    /* 000C */ int param;
} PacketChareditTraitChange;

static_assert(sizeof(PacketChareditTraitChange) == 0x10, "wrong size");

typedef struct Packet128 {
    /* 0000 */ int type;
    /* 0008 */ ObjectID target_oid;
    /* 0020 */ ObjectID item_oid;
} Packet128;

static_assert(sizeof(Packet128) == 0x38, "wrong size");

#define P129_SUBTYPE_SCRIPT 6
#define P129_SUBTYPE_INT32_ARRAY 7

typedef struct Packet129 {
    /* 0000 */ int type;
    /* 0004 */ int field_4;
    /* 0008 */ ObjectID oid;
    /* 0020 */ int fld;
    /* 0024 */ int subtype;
    union {
        union {
            struct {
                /* 0028 */ int field_28;
            } a;
            struct {
                /* 0028 */ int field_28;
            } b;
            struct {
                /* 0028 */ int field_28;
            } c;
            struct {
                /* 0028 */ ObjectID oid;
            } d;
            struct {
                /* 0028 */ int field_28;
                /* 0030 */ ObjectID oid;
            } e;
            struct {
                /* 0028 */ int field_28;
                /* 002C */ int field_2C;
            } f;
            struct {
                /* 0028 */ int field_28;
                /* 002C */ int field_2C;
            } g;
            struct {
                /* 0028 */ tig_art_id_t art_id;
            } h;
            struct {
                /* 0028 */ int field_28;
                /* 002C */ int field_2C;
                /* 0030 */ int field_30;
            } i;
            struct {
                /* 0028 */ unsigned int flags;
            } j;
        } d;
        /* 0028 */ int val;
        /* 0028 */ int64_t val64;
        struct {
            /* 0028 */ int idx;
            /* 002C */ int value;
        } int32_array_val;
        struct {
            /* 0028 */ int idx;
            /* 002C */ Script scr;
        } scr_val;
    };
} Packet129;

static_assert(sizeof(Packet129) == 0x48, "wrong size");

typedef struct Packet130 {
    /* 0000 */ int type;
    /* 0004 */ int field_4;
    /* 0008 */ ObjectID oid;
    /* 0020 */ int fld;
    /* 0024 */ int length;
} Packet130;

static_assert(sizeof(Packet130) == 0x28, "wrong size");

void sub_4ED510(AnimID anim_id, int64_t loc, AnimRunInfo* run_info);
bool sub_4ED6C0(int64_t obj);
void mp_critter_fatigue_damage_set(int64_t obj, int damage);
bool sub_4ED780(int64_t obj, int quest, int state, int64_t a4);
bool mp_object_create(int name, int64_t loc, int64_t* obj_ptr);
void mp_object_destroy(int64_t obj);
void sub_4EDA60(UiMessage* ui_message, int player, int a3);
void mp_tf_add(int64_t obj, int tf_type, const char* str);
void mp_reaction_adj(int64_t npc_obj, int64_t pc_obj, int value);
void mp_trap_mark_known(int64_t pc_obj, int64_t trap_obj, int reason);
void sub_4EDCE0(int64_t obj, tig_art_id_t art_id);
void mp_ui_update_inven(int64_t obj);
void sub_4EDDE0(int64_t obj);
void mp_item_arrange_inventory(int64_t obj, bool vertical);
void mp_handle_item_arrange_inventory(PacketArrangeInventory* pkt);
void sub_4EDF20(int64_t obj, int64_t location, int dx, int dy, bool a7);
void sub_4EE060(int64_t a1, int64_t a2);
void sub_4EE0F0(int a1, int64_t a2, int64_t a3);
void mp_ui_follower_refresh();
void sub_4EE1D0(int64_t obj);
void mp_ui_toggle_primary_button(UiPrimaryButton btn, bool on, int client_id);
void mp_ui_written_start_type(int64_t obj, WrittenType written_type, int num);
void mp_ui_show_inven_loot(int64_t obj, int64_t a2);
void sub_4EE3A0(int64_t obj, int64_t a2);
void mp_ui_show_inven_identify(int64_t pc_obj, int64_t target_obj);
void sub_4EE4C0(int64_t obj, int64_t a2);
void mp_ui_show_inven_npc_identify(int64_t pc_obj, int64_t target_obj);
void mp_container_close(int64_t obj);
void mp_container_open(int64_t obj);
void mp_portal_toggle(int64_t obj);
void mp_sector_block_set(int64_t sec, bool blocked);
void mp_spell_mastery_set(int64_t obj, int college);
void mp_townmap_set_known(int map, bool known);
void mp_art_touch(tig_art_id_t art_id);
void map_map_precache_sectors(int64_t loc, int64_t obj);
void mp_tf_remove(int64_t obj);
void mp_tb_remove(int64_t obj);
void mp_item_use(int64_t source_obj, int64_t item_obj, int64_t target_obj);
void sub_4EF830(int64_t a1, int64_t a2);
bool sub_4EF920(int64_t obj, int64_t loc, int64_t* obj_ptr);
void sub_4EFAE0(int64_t obj, int a2);
void sub_4EFBA0(int64_t obj);
void mp_object_locked_set(int64_t obj, int a2);
void sub_4EFC30(int64_t pc_obj, const char* name, const char* rule);
void mp_gsound_play_sfx(int sound_id);
void sub_4EED00(int64_t obj, int sound_id);
void mp_gsound_play_sfx_on_obj(int sound_id, int loops, int64_t obj);
void mp_gsound_play_scheme(int music_scheme_idx, int ambient_scheme_idx);
void mp_obj_field_int32_set(int64_t obj, int fld, int value);
void mp_obj_field_int64_set(int64_t obj, int fld, int64_t value);
void mp_object_flags_unset(int64_t obj, unsigned int flags);
void mp_object_flags_set(int64_t obj, unsigned int flags);
void mp_obj_field_obj_set(int64_t obj, int fld, int64_t value);
void sub_4F0070(int64_t obj, int fld, int index, int64_t value);
void mp_obj_arrayfield_int32_set(int64_t obj, int fld, int index, int value);
void mp_obj_arrayfield_script_set(int64_t obj, int fld, int index, Script* value);
void mp_obj_arrayfield_uint32_set(int64_t obj, int fld, int index, int value);
void mp_object_set_current_aid(int64_t obj, tig_art_id_t art_id);
void mp_object_overlay_set(int64_t obj, int fld, int index, tig_art_id_t aid);
void mp_item_flags_set(int64_t obj, unsigned int flags_to_add);
void mp_item_flags_unset(int64_t obj, unsigned int flags_to_remove);
void sub_4F0500(int64_t obj, int fld);
void sub_4F0570(int64_t obj, int fld, int length);
void sub_4F0640(int64_t obj, ObjectID* oid_ptr);
void sub_4F0690(ObjectID oid, int64_t* obj_ptr);

#endif /* ARCANUM_GAME_MP_UTILS_H_ */
