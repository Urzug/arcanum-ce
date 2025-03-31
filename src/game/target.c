#include "game/target.h"

#include "game/critter.h"
#include "game/material.h"
#include "game/obj.h"
#include "game/object.h"
#include "game/path.h"
#include "game/player.h"
#include "game/random.h"
#include "game/stat.h"
#include "game/tile.h"

static bool sub_4F28A0(int x, int y, S4F2810* a3);
static void sub_4F3F10(S603CB8_F50* a1, int64_t obj);
static void sub_4F3FD0(S603CB8_F50* a1, int64_t obj);
static void sub_4F4050(S603CB8_F50* a1, int64_t loc);

// 0x603B88
static PathCreateInfo stru_603B88;

// 0x603BB0
static TigRect target_iso_content_rect;

// 0x603BC4
static int8_t byte_603BC4[200];

// 0x603C94
static int dword_603C94;

// 0x603C98
static int dword_603C98;

// 0x603CA8
static int64_t qword_603CA8;

// 0x603CB0
static int64_t qword_603CB0;

// 0x603CB8
static S603CB8 stru_603CB8;

// 0x603D20
static S603D20 stru_603D20;

// 0x603D38
static bool target_initialized;

// 0x603D40
static int64_t qword_603D40;

// 0x4F24F0
bool target_init(GameInitInfo* init_info)
{
    TigWindowData window_data;

    if (tig_window_data(init_info->iso_window_handle, &window_data) != TIG_OK) {
        return false;
    }

    target_iso_content_rect = window_data.rect;
    sub_4F2600(&stru_603CB8, &stru_603D20, OBJ_HANDLE_NULL);
    target_initialized = true;

    return true;
}

// 0x4F2560
void target_exit()
{
    target_initialized = false;
}

// 0x4F2570
void target_reset()
{
    qword_603D40 = OBJ_HANDLE_NULL;
}

// 0x4F2580
void target_resize(GameResizeInfo* resize_info)
{
    target_iso_content_rect = resize_info->content_rect;
}

// 0x4F25B0
void sub_4F25B0(uint64_t flags)
{
    stru_603D20.aoe_flags = flags;
}

// 0x4F25D0
uint64_t sub_4F25D0()
{
    return stru_603D20.aoe_flags;
}

// 0x4F25E0
void sub_4F25E0(S603D20* a1)
{
    a1->aoe_flags = Tgt_None;
    a1->aoe_spell_flags = 0;
    a1->aoe_no_spell_flags = 0;
    a1->radius = 0;
    a1->count = 0;
}

// 0x4F2600
void sub_4F2600(S603CB8 *a1, S603D20 *a2, int64_t a3)
{
    a1->field_38 = 0;
    a1->field_30 = 0;
    a1->field_40 = 0;
    a1->source_obj = a3;
    a1->self_obj = a3;
    a1->field_0 = a2;
    a1->field_54 = 0;
    a1->field_58 = 0;
    a1->field_50 = 0;
    if (a3 != OBJ_HANDLE_NULL) {
        a1->field_18 = obj_field_int64_get(a3, OBJ_F_LOCATION);
    }
    a1->field_48 = 0;
    a1->field_28 = 0;
    a1->field_20 = 0;
    a1->field_60 = 0;

    if (a2 != NULL) {
        sub_4F25E0(a2);
    }
}

// 0x4F2680
bool sub_4F2680(S4F2680* a1)
{
    stru_603CB8.source_obj = a1->field_0;

    if (a1->field_10->is_loc) {
        stru_603CB8.field_38 = a1->field_10->loc;
        stru_603CB8.field_28 = a1->field_10->loc;
        stru_603CB8.field_20 = 0;
        stru_603CB8.field_30 = 0;
    } else {
        if ((obj_field_int32_get(a1->field_10->obj, OBJ_F_FLAGS) & OF_CLICK_THROUGH) != 0) {
            return false;
        }

        stru_603CB8.field_30 = a1->field_10->obj;
        stru_603CB8.field_20 = a1->field_10->obj;
        stru_603CB8.field_28 = OBJ_HANDLE_NULL;
        stru_603CB8.field_38 = OBJ_HANDLE_NULL;
    }

    stru_603CB8.self_obj = a1->field_8;

    if (sub_4F2D20(&stru_603CB8)) {
        if (stru_603D20.aoe_flags) {
            return true;
        }
        return false;
    }

    if (!a1->field_10->is_loc) {
        if (a1->field_10->obj != OBJ_HANDLE_NULL) {
            stru_603CB8.field_28 = obj_field_int64_get(a1->field_10->obj, OBJ_F_LOCATION);
            stru_603CB8.field_38 = stru_603CB8.field_28;
        }
        if (sub_4F2D20(&stru_603CB8) && stru_603D20.aoe_flags) {
            sub_4F27F0(a1->field_10, stru_603CB8.field_28);
            return true;
        }
    }

    return false;
}

// 0x4F27F0
void sub_4F27F0(S4F2810* a1, int64_t loc)
{
    a1->loc = loc;
    a1->is_loc = 1;
}

// 0x4F2810
void sub_4F2810(S4F2810* a1, int64_t obj)
{
    a1->obj = obj;
    a1->is_loc = 0;
}

// 0x4F2830
bool sub_4F2830(TigMouseMessageData* mouse, S4F2810* a2, bool fullscreen)
{
    int x;
    int y;

    if (mouse->x < target_iso_content_rect.x
        || mouse->x >= target_iso_content_rect.x + target_iso_content_rect.width
        || mouse->y < target_iso_content_rect.y
        || mouse->y >= target_iso_content_rect.y + target_iso_content_rect.height) {
        return false;
    }

    x = mouse->x - target_iso_content_rect.x;
    y = mouse->y;

    // FIXME: Looks odd, why `x` is not treat in the same way?
    if (!fullscreen) {
        y -= target_iso_content_rect.y;
    }

    return sub_4F28A0(x, y, a2);
}

// 0x4F28A0
bool sub_4F28A0(int x, int y, S4F2810* a3)
{
    int64_t pc_obj;
    S4F2680 v1;
    ObjectList party_members;
    ObjectList mp_party_members;
    ObjectList dead_critters;
    ObjectList objects;
    ObjectNode* node;
    int64_t v2;
    int64_t loc;
    bool ret = false;

    pc_obj = player_get_pc_obj();

    // Initialize heads to silence compiler warnings.
    party_members.head = NULL;
    mp_party_members.head = NULL;
    dead_critters.head = NULL;
    objects.head = NULL;

    v1.field_0 = pc_obj;
    v1.field_8 = pc_obj;
    v1.field_10 = a3;

    if ((stru_603D20.aoe_flags & Tgt_No_Self) != 0) {
        object_flags_set(pc_obj, OF_CLICK_THROUGH);
    }

    if ((stru_603D20.aoe_flags & Tgt_Non_Party) != 0) {
        object_list_all_followers(pc_obj, &party_members);
        node = party_members.head;
        while (node != NULL) {
            object_flags_set(node->obj, OF_CLICK_THROUGH);
            node = node->next;
        }

        if (tig_net_is_host()) {
            object_list_party(pc_obj, &mp_party_members);
            node = mp_party_members.head;
            while (node != NULL) {
                object_flags_set(node->obj, OF_CLICK_THROUGH);
                node = node->next;
            }
        }
    }

    if ((stru_603D20.aoe_flags & Tgt_No_ST_Critter_Dead) != 0) {
        object_list_vicinity(pc_obj, OBJ_TM_PC | OBJ_TM_NPC, &dead_critters);
        node = mp_party_members.head;
        while (node != NULL) {
            if (critter_is_dead(node->obj)) {
                object_flags_set(node->obj, OF_CLICK_THROUGH);
            }
            node = node->next;
        }
    }

    if (sub_43D9F0(x, y, &v2, 0x3)) {
        sub_4F2C60(&v2);
        sub_4F2810(a3, v2);
        if (sub_4F2680(&v1)) {
            ret = true;
        }
    }

    if (!ret) {
        if (location_at(x, y, &loc)) {
            object_list_location(loc, OBJ_TM_ALL & ~OBJ_TM_PROJECTILE, &objects);
            node = objects.head;
            while (node != NULL) {
                v2 = node->obj;
                sub_4F2C60(&v2);
                sub_4F2810(a3, v2);
                if (sub_4F2680(&v1)) {
                    ret = true;
                    break;
                }
                node = node->next;
            }
            object_list_destroy(&objects);

            if (!ret) {
                sub_4F27F0(a3, loc);
                if (sub_4F2680(&v1)) {
                    ret = true;
                }
            }
        }
    }


    if ((stru_603D20.aoe_flags & Tgt_No_Self) != 0) {
        object_flags_unset(pc_obj, OF_CLICK_THROUGH);
    }

    if ((stru_603D20.aoe_flags & Tgt_Non_Party) != 0) {
        node = party_members.head;
        while (node != NULL) {
            object_flags_unset(node->obj, OF_CLICK_THROUGH);
            node = node->next;
        }

        if (tig_net_is_host()) {
            node = mp_party_members.head;
            while (node != NULL) {
                object_flags_unset(node->obj, OF_CLICK_THROUGH);
                node = node->next;
            }
        }
    }

    if ((stru_603D20.aoe_flags & Tgt_No_ST_Critter_Dead) != 0) {
        node = mp_party_members.head;
        while (node != NULL) {
            if (critter_is_dead(node->obj)) {
                object_flags_unset(node->obj, OF_CLICK_THROUGH);
            }
            node = node->next;
        }
    }

    if (!ret) {
        sub_4F2810(a3, OBJ_HANDLE_NULL);
        return false;
    }

    return true;
}

// 0x4F2C60
int sub_4F2C60(int64_t* obj_ptr)
{
    int type;
    int64_t whos_in_me_obj;

    type = obj_field_int32_get(*obj_ptr, OBJ_F_TYPE);
    if (type == OBJ_TYPE_SCENERY) {
        whos_in_me_obj = obj_field_handle_get(*obj_ptr, OBJ_F_SCENERY_WHOS_IN_ME);
        if (whos_in_me_obj != OBJ_HANDLE_NULL) {
            *obj_ptr = whos_in_me_obj;
            return OBJ_TYPE_NPC;
        }
    }

    return type;
}

// 0x4F2CB0
bool sub_4F2CB0(int x, int y, S4F2810* a3, uint64_t tgt, bool fullscreen)
{
    uint64_t old_tgt;
    bool rc;

    x -= target_iso_content_rect.x;
    if (!fullscreen) {
        y -= target_iso_content_rect.y;
    }

    old_tgt = sub_4F25D0();
    sub_4F25B0(tgt);
    rc = sub_4F28A0(x, y, a3);
    sub_4F25B0(old_tgt);
    return rc;
}

// 0x4F2D10
int64_t sub_4F2D10()
{
    return qword_603D40;
}

// 0x4F2D20
bool sub_4F2D20(S603CB8* a1)
{
    uint64_t tgt;
    ObjectList objects;
    ObjectNode* node;
    int obj_type;
    unsigned int flags;
    unsigned int spell_flags;
    unsigned int critter_flags;

    tgt = a1->field_0->aoe_flags;

    if (tgt == 0) {
        return true;
    }

    if ((tgt & (Tgt_Obj_Radius | Tgt_Object | Tgt_Self)) != 0) {
        if (a1->field_20 != OBJ_HANDLE_NULL) {
            obj_type = obj_field_int32_get(a1->field_20, OBJ_F_TYPE);
            flags = obj_field_int32_get(a1->field_20, OBJ_F_FLAGS);

            if ((flags & (OF_DESTROYED | OF_OFF)) != 0) {
                if (!obj_type_is_critter(obj_type)) {
                    return false;
                }

                if ((obj_field_int32_get(a1->field_20, OBJ_F_CRITTER_FLAGS2) & OCF2_SAFE_OFF) == 0) {
                    return false;
                }
            }

            if ((flags & OF_DONTDRAW) != 0) {
                if (!obj_type_is_critter(obj_type)) {
                    return false;
                }

                if (!critter_is_sleeping(a1->field_20)) {
                    return false;
                }
            }

            spell_flags = obj_field_int32_get(a1->field_20, OBJ_F_SPELL_FLAGS);

            if ((spell_flags & a1->field_0->aoe_spell_flags) != a1->field_0->aoe_spell_flags) {
                return false;
            }

            if ((spell_flags & a1->field_0->aoe_no_spell_flags) != 0) {
                return false;
            }

            if ((tgt & 0x9) != 0) {
                if ((tgt & 0x10000000) == 0
                    && a1->field_20 != a1->self_obj) {
                    return false;
                }
            } else {
                if ((tgt & 0x10000000) != 0
                    && a1->field_20 == a1->self_obj) {
                    return false;
                }
            }

            if ((tgt & Tgt_Source) != 0
                && (tgt & 0x10000000) == 0
                && a1->field_20 != a1->source_obj) {
                return false;
            }

            if ((tgt & 0x40) != 0) {
                if (!obj_type_is_critter(obj_type)) {
                    return false;
                }

                if ((tgt & 0x20) != 0
                    && obj_type != OBJ_TYPE_PC) {
                    return false;
                }

                if ((tgt & 0x20000000) != 0
                    && obj_type == OBJ_TYPE_PC) {
                    return false;
                }

                if ((tgt & 0x100) != 0) {
                    if (!critter_is_dead(a1->field_20)) {
                        return false;
                    }
                } else if ((tgt & 0x80000000) != 0) {
                    if (critter_is_dead(a1->field_20)) {
                        return false;
                    }
                }

                critter_flags = obj_field_int32_get(a1->field_20, OBJ_F_CRITTER_FLAGS);
                if ((tgt & 0x80) != 0) {
                    if ((critter_flags & OCF_ANIMAL) == 0) {
                        return false;
                    }
                } else if ((tgt & 0x40000000) != 0) {
                    if ((critter_flags & OCF_ANIMAL) != 0) {
                        return false;
                    }
                }

                if ((tgt & 0x200) != 0
                    && (critter_flags & OCF_UNDEAD) == 0) {
                    return false;
                }

                if ((tgt & 0x400) != 0
                    && (critter_flags & OCF_DEMON) == 0) {
                    return false;
                }

                if ((tgt & 0x800) != 0
                    && (critter_flags & OCF_MECHANICAL) == 0) {
                    return false;
                }

                if ((tgt & 0x1000) != 0) {
                    if (stat_level_get(a1->field_20, STAT_ALIGNMENT) < 0) {
                        return false;
                    }
                } else if ((tgt & 0x2000) != 0) {
                    if (stat_level_get(a1->field_20, STAT_ALIGNMENT) >= 0) {
                        return false;
                    }
                }

                if ((tgt & 0x4000) != 0
                    && (critter_flags & OCF_UNREVIVIFIABLE) == 0) {
                    return false;
                }

                if ((tgt & 0x8000) != 0
                    && (critter_flags & OCF_UNRESSURECTABLE) == 0) {
                    return false;
                }

                if ((tgt & 0x100000000) != 0
                    && (critter_flags & OCF_UNDEAD) != 0) {
                    return false;
                }

                if ((tgt & 0x200000000) != 0
                    && (critter_flags & OCF_DEMON) != 0) {
                    return false;
                }

                if ((tgt & 0x400000000) != 0
                    && (critter_flags & OCF_MECHANICAL) != 0) {
                    return false;
                }

                if ((tgt & 0x800000000) != 0) {
                    if (stat_level_get(a1->field_20, STAT_ALIGNMENT) >= 0) {
                        return false;
                    }
                } else if ((tgt & 0x1000000000) != 0) {
                    if (stat_level_get(a1->field_20, STAT_ALIGNMENT) < 0) {
                        return false;
                    }
                }

                if ((tgt & 0x2000000000) != 0
                    && (critter_flags & OCF_UNREVIVIFIABLE) != 0) {
                    return false;
                }

                if ((tgt & 0x4000000000) != 0
                    && (critter_flags & OCF_UNRESSURECTABLE) != 0) {
                    return false;
                }

                if ((tgt & 0x400000) != 0) {
                    if (stat_level_get(a1->field_20, STAT_POISON_LEVEL) <= 0) {
                        return false;
                    }
                }

                if ((tgt & 0xC00000000000000) != 0) {
                    if (critter_pc_leader_get(a1->field_20) == OBJ_HANDLE_NULL
                        && !sub_40DA20(a1->field_20)) {
                        return false;
                    }
                } else if ((tgt & 0x1000000000000000) != 0) {
                    // TODO: Looks the same as the code below, probably one of
                    // it should be inverted, check.
                    if (a1->self_obj != OBJ_HANDLE_NULL) {
                        int64_t v1;
                        int64_t v2;

                        v1 = critter_pc_leader_get(a1->field_20);
                        v2 = critter_pc_leader_get(a1->self_obj);
                        if (v1 == a1->self_obj
                            || (v1 == v2 && v1 != OBJ_HANDLE_NULL)
                            || v2 == a1->field_20) {
                            return false;
                        }

                        if (tig_net_is_active()
                            && obj_field_int32_get(v2, OBJ_F_TYPE) == OBJ_TYPE_PC) {
                            object_list_party(a1->self_obj, &objects);
                            node = objects.head;
                            while (node != NULL) {
                                if (node->obj == v1
                                    || node->obj == a1->field_20) {
                                    break;
                                }
                                node = node->next;
                            }
                            object_list_destroy(&objects);

                            if (node != NULL) {
                                return false;
                            }
                        }
                    }

                    if (a1->field_40 != OBJ_HANDLE_NULL) {
                        int64_t v1;
                        int64_t v2;

                        v1 = critter_pc_leader_get(a1->field_20);
                        v2 = critter_pc_leader_get(a1->field_40);
                        if (v1 == a1->field_40
                            || (v1 == v2 && v1 != OBJ_HANDLE_NULL)
                            || v2 == a1->field_20) {
                            return false;
                        }

                        if (tig_net_is_active()
                            && obj_field_int32_get(v2, OBJ_F_TYPE) == OBJ_TYPE_PC) {
                            object_list_party(a1->field_40, &objects);
                            node = objects.head;
                            while (node != NULL) {
                                if (node->obj == v1
                                    || node->obj == a1->field_20) {
                                    break;
                                }
                                node = node->next;
                            }
                            object_list_destroy(&objects);

                            if (node != NULL) {
                                return false;
                            }
                        }
                    }
                }
            } else if ((tgt & 0x10000) != 0) {
                if (obj_type != OBJ_TYPE_PORTAL) {
                    // TODO: Check, looks odd.
                    if ((tgt & 0x20000) == 0) {
                        return false;
                    }
                    if (obj_type != OBJ_TYPE_CONTAINER) {
                        return false;
                    }
                }
            } else if ((tgt & 0x20000) != 0) {
                if (obj_type != OBJ_TYPE_CONTAINER) {
                    return false;
                }
            } else if ((tgt & 0x80000) != 0) {
                if (obj_type != OBJ_TYPE_WALL) {
                    return false;
                }
            } else if ((tgt & 0x20000000000) != 0) {
                if (obj_type == OBJ_TYPE_WALL) {
                    return false;
                }
            }

            if ((tgt & 0x40) == 0) {
                if ((tgt & 0x80000000) != 0
                    && critter_is_dead(a1->field_20)) {
                    return false;
                }

                if ((tgt & 0x1000000000000000) != 0
                    && obj_type_is_critter(obj_type)) {
                    // TODO: Looks the same as the code above, probably one of
                    // it should be inverted, check.
                    if (a1->self_obj != OBJ_HANDLE_NULL) {
                        int64_t v1;
                        int64_t v2;

                        v1 = critter_pc_leader_get(a1->field_20);
                        v2 = critter_pc_leader_get(a1->self_obj);
                        if (v1 == a1->self_obj
                            || (v1 == v2 && v1 != OBJ_HANDLE_NULL)
                            || v2 == a1->field_20) {
                            return false;
                        }

                        if (tig_net_is_active()
                            && obj_field_int32_get(v2, OBJ_F_TYPE) == OBJ_TYPE_PC) {
                            object_list_party(a1->self_obj, &objects);
                            node = objects.head;
                            while (node != NULL) {
                                if (node->obj == v1
                                    || node->obj == a1->field_20) {
                                    break;
                                }
                                node = node->next;
                            }
                            object_list_destroy(&objects);

                            if (node != NULL) {
                                return false;
                            }
                        }
                    }

                    if (a1->field_40 != OBJ_HANDLE_NULL) {
                        int64_t v1;
                        int64_t v2;

                        v1 = critter_pc_leader_get(a1->field_20);
                        v2 = critter_pc_leader_get(a1->field_40);
                        if (v1 == a1->field_40
                            || (v1 == v2 && v1 != OBJ_HANDLE_NULL)
                            || v2 == a1->field_20) {
                            return false;
                        }

                        if (tig_net_is_active()
                            && obj_field_int32_get(v2, OBJ_F_TYPE) == OBJ_TYPE_PC) {
                            object_list_party(a1->field_40, &objects);
                            node = objects.head;
                            while (node != NULL) {
                                if (node->obj == v1
                                    || node->obj == a1->field_20) {
                                    break;
                                }
                                node = node->next;
                            }
                            object_list_destroy(&objects);

                            if (node != NULL) {
                                return false;
                            }
                        }
                    }
                }
            }

            if ((tgt & 0x100000) != 0) {
                if (object_hp_damage_get(a1->field_20) <= 0
                    && (!obj_type_is_critter(obj_type)
                        || (obj_field_int32_get(a1->field_20, OBJ_F_CRITTER_FLAGS) & OCF_INJURED) == 0)) {
                    qword_603D40 = 0x100000;
                    return false;
                }
            } else if ((tgt & 0x40000000000) != 0) {
                if (object_hp_damage_get(a1->field_20) > 0
                    && (!obj_type_is_critter(obj_type)
                        || (obj_field_int32_get(a1->field_20, OBJ_F_CRITTER_FLAGS) & OCF_INJURED) != 0)) {
                    qword_603D40 = 0x40000000000;
                    return false;
                }
            }

            if ((tgt & 0x40000) != 0) {
                if (obj_type == OBJ_TYPE_PORTAL) {
                    if ((obj_field_int32_get(a1->field_20, OBJ_F_PORTAL_FLAGS) & OPF_LOCKED) == 0) {
                        return false;
                    }
                } else if (obj_type == OBJ_TYPE_CONTAINER) {
                    if ((obj_field_int32_get(a1->field_20, OBJ_F_CONTAINER_FLAGS) & OCOF_LOCKED) == 0) {
                        return false;
                    }
                }
            } else if ((tgt & 0x8000000000) != 0) {
                if (obj_type == OBJ_TYPE_PORTAL) {
                    if ((obj_field_int32_get(a1->field_20, OBJ_F_PORTAL_FLAGS) & OPF_LOCKED) != 0) {
                        return false;
                    }
                } else if (obj_type == OBJ_TYPE_CONTAINER) {
                    if ((obj_field_int32_get(a1->field_20, OBJ_F_CONTAINER_FLAGS) & OCOF_LOCKED) != 0) {
                        return false;
                    }
                }
            }

            if ((tgt & 0x10000000000) != 0) {
                if (obj_type == OBJ_TYPE_PORTAL) {
                    if ((obj_field_int32_get(a1->field_20, OBJ_F_PORTAL_FLAGS) & OPF_MAGICALLY_HELD) != 0) {
                        return false;
                    }
                } else if (obj_type == OBJ_TYPE_CONTAINER) {
                    if ((obj_field_int32_get(a1->field_20, OBJ_F_CONTAINER_FLAGS) & OCOF_MAGICALLY_HELD) != 0) {
                        return false;
                    }
                }
            }

            if ((tgt & 0x200000) != 0
                && object_hp_damage_get(a1->field_20) <= 0) {
                if (!obj_type_is_critter(obj_type)) {
                    return false;
                }
                if (stat_level_get(a1->field_20, STAT_POISON_LEVEL) <= 0) {
                    return false;
                }
            }

            if ((tgt & 0x1000000) != 0
                && obj_field_int32_get(a1->field_20, OBJ_F_MATERIAL) != MATERIAL_FLESH) {
                return false;
            }

            if ((tgt & 0x10) != 0
                && a1->field_20 != OBJ_HANDLE_NULL) {
                int64_t loc1;
                int64_t loc2;

                loc1 = a1->field_38;
                if (loc1 == 0) {
                    if (a1->source_obj == OBJ_HANDLE_NULL) {
                        return false;
                    }

                    loc1 = obj_field_int64_get(a1->source_obj, OBJ_F_LOCATION);
                }

                loc2 = obj_field_int64_get(a1->field_20, OBJ_F_LOCATION);
                if (location_dist(loc1, loc2) > a1->field_0->radius) {
                    return false;
                }
            }

            if ((tgt & 0x400000000000) != 0
                && (flags & OSF_SUMMONED) == 0) {
                return false;
            }

            if ((tgt & 0x100002000000) != 0) {
                if ((tgt & 0x2000000) != 0) {
                    if ((flags & OF_INVENTORY) == 0) {
                        return false;
                    }
                } else if ((tgt & 0x100000000000) != 0) {
                    if ((flags & OF_INVENTORY) != 0) {
                        return false;
                    }
                }
            }

            if ((tgt & 0x200000000000) != 0
                && (flags & OF_INVULNERABLE) != 0) {
                return false;
            }

            if ((tgt & 0x4000000) != 0) {
                return obj_type_is_item(obj_type)
                    ? obj_field_int32_get(a1->field_20, OBJ_F_ITEM_WEIGHT) < 5
                    : false;
            }

            return true;
        }

        if ((tgt & 0x8000800000000000) == 0) {
            return false;
        }
    }

    if ((tgt & 0x800000000000) == 0) {
        return true;
    }

    if (a1->field_28 == 0) {
        return (tgt & 0x8000000000000000) != 0;
    }

    if ((tgt & 0x1000000000000) != 0) {
        if (a1->source_obj == OBJ_HANDLE_NULL
            || a1->field_28 != obj_field_int64_get(a1->field_20, OBJ_F_LOCATION)) {
            return false;
        }
    }

    if ((tgt & 0x20000) != 0
        && a1->source_obj != OBJ_HANDLE_NULL) {
        stru_603B88.obj = a1->source_obj;
        stru_603B88.from = obj_field_int64_get(a1->source_obj, OBJ_F_LOCATION);
        stru_603B88.to = a1->field_28;
        stru_603B88.max_rotations = sizeof(byte_603BC4);
        stru_603B88.rotations = byte_603BC4;
        stru_603B88.flags = PATH_FLAG_0x0010;

        dword_603C98 = sub_41F3C0(&stru_603B88);
        qword_603CA8 = stru_603B88.from;
        qword_603CB0 = stru_603B88.to;

        if (dword_603C98 == 0) {
            return false;
        }
    }

    if ((tgt & 0x4000000000000) != 0) {
        bool v51 = true;

        if (tile_is_blocking(a1->field_28, false)) {
            return false;
        }

        object_list_location(a1->field_28, OBJ_TM_ALL & ~OBJ_TM_PROJECTILE, &objects);
        node = objects.head;
        while (node != NULL) {
            if (node->obj != OBJ_HANDLE_NULL) {
                if (!obj_type_is_item(obj_field_int32_get(node->obj, OBJ_F_TYPE))
                    && (obj_field_int32_get(node->obj, OBJ_F_FLAGS) & OF_NO_BLOCK) == 0) {
                    v51 = false;
                    break;
                }
            }
            node = node->next;
        }
        object_list_destroy(&objects);

        if (!v51) {
            return false;
        }
    }

    if ((tgt & 0x8000000000000) != 0) {
        bool v54 = true;

        if (tile_is_blocking(a1->field_28, false)) {
            return false;
        }

        object_list_location(a1->field_28, OBJ_TM_TRAP | OBJ_TM_SCENERY | OBJ_TM_PORTAL | OBJ_TM_WALL, &objects);
        node = objects.head;
        while (node != NULL) {
            if (node->obj != OBJ_HANDLE_NULL) {
                if (!obj_type_is_item(obj_field_int32_get(node->obj, OBJ_F_TYPE))
                    && (obj_field_int32_get(node->obj, OBJ_F_FLAGS) & OF_NO_BLOCK) == 0) {
                    v54 = false;
                    break;
                }
            }
            node = node->next;
        }
        object_list_destroy(&objects);

        if (!v54) {
            return false;
        }
    }

    if ((tgt & 0x20000000000000) != 0
        // TODO: Sames args looks wrong, check.
        && location_dist(a1->field_38, a1->field_38) > a1->field_0->radius) {
        return false;
    }

    if ((tgt & 0x100000000000000) != 0
        && a1->field_18 != 0
        && a1->field_28 != 0) {
        if (tig_art_tile_id_type_get(tile_art_id_at(a1->field_28)) != tig_art_tile_id_type_get(tile_art_id_at(a1->field_18))) {
            return false;
        }
    }

    return true;
}

// 0x4F3F10
void sub_4F3F10(S603CB8_F50* a1, int64_t obj)
{
    int idx;

    if (obj == OBJ_HANDLE_NULL) {
        return;
    }

    if ((obj_field_int32_get(obj, OBJ_F_FLAGS) & (OF_DESTROYED | OF_OFF)) != 0) {
        if (!obj_type_is_critter(obj_field_int32_get(obj, OBJ_F_TYPE))) {
            return;
        }

        if ((obj_field_int32_get(obj, OBJ_F_CRITTER_FLAGS2) & OCF2_SAFE_OFF) == 0) {
            return;
        }
    }

    for (idx = 0; idx < a1->cnt; idx++) {
        if (a1->entries[idx].obj == obj) {
            return;
        }
    }

    a1->entries[a1->cnt].obj = obj;
    a1->entries[a1->cnt].loc = 0;
    a1->cnt++;
}

// 0x4F3FD0
void sub_4F3FD0(S603CB8_F50* a1, int64_t obj)
{
    int idx;

    if (obj == OBJ_HANDLE_NULL) {
        return;
    }

    if ((obj_field_int32_get(obj, OBJ_F_FLAGS) & OF_DESTROYED) != 0) {
        return;
    }

    for (idx = 0; idx < a1->cnt; idx++) {
        if (a1->entries[idx].obj == obj) {
            return;
        }
    }

    a1->entries[a1->cnt].obj = obj;
    a1->entries[a1->cnt].loc = 0;
    a1->cnt++;
}

// 0x4F4050
void sub_4F4050(S603CB8_F50* a1, int64_t loc)
{
    int idx;

    for (idx = 0; idx < a1->cnt; idx++) {
        if (a1->entries[idx].loc == loc) {
            return;
        }
    }

    a1->entries[a1->cnt].obj = OBJ_HANDLE_NULL;
    a1->entries[a1->cnt].loc = loc;
    a1->cnt++;
}

// 0x4F40B0
void sub_4F40B0(S603CB8* a1)
{
    S603CB8_F50* v1;
    S603D20* v2;
    S603CB8 v3;
    S603D20 v4;
    ObjectList objects;
    ObjectNode* obj_node;
    MagicTechObjectNode* mt_obj_node;
    int idx;
    int64_t origin;

    v1 = a1->field_50;
    if (v1 == NULL) {
        return;
    }

    v1->cnt = 0;
    v2 = a1->field_0;

    if ((v2->aoe_flags & Tgt_Self) != 0
        && (v2->aoe_flags & Tgt_No_Self) == 0) {
        sub_4F3F10(v1, a1->self_obj);
    }

    if ((v2->aoe_flags & Tgt_Source) != 0
        && (v2->aoe_flags & Tgt_No_Self) == 0) {
        sub_4F3F10(v1, a1->source_obj);
    }

    if ((v2->aoe_flags & Tgt_Object) != 0 && a1->field_30 != OBJ_HANDLE_NULL) {
        sub_4F3FD0(v1, a1->field_30);
    }

    if ((v2->aoe_flags & Tgt_Summoned_No_Obj) != 0) {
        if (a1->field_48 != OBJ_HANDLE_NULL) {
            sub_4F3FD0(v1, a1->field_48);
        }

        if (a1->field_58 != NULL) {
            mt_obj_node = *a1->field_58;
            while (mt_obj_node != NULL) {
                sub_4F3FD0(v1, mt_obj_node->obj);
                mt_obj_node = mt_obj_node->next;
            }
        }
    }

    if ((v2->aoe_flags & Tgt_Tile) != 0 && a1->field_38 != 0) {
        sub_4F4050(v1, a1->field_38);
    }

    if ((v2->aoe_flags & Tgt_Tile_Self) != 0 && a1->source_obj != OBJ_HANDLE_NULL) {
        sub_4F4050(v1, obj_field_int64_get(a1->source_obj, OBJ_F_LOCATION));
    }

    if ((v2->aoe_flags & Tgt_Obj_Radius) != 0) {
        LocRect loc_rect;
        unsigned int obj_type_mask;
        bool all;

        sub_4F2600(&v3, &v4, a1->source_obj);
        origin = a1->field_38;
        v3.field_38 = a1->field_38;
        v3.field_40 = a1->field_40;

        v4.aoe_flags = v2->aoe_flags & ~Tgt_Tile;
        v4.aoe_spell_flags = v2->aoe_spell_flags;
        v4.aoe_no_spell_flags = v2->aoe_no_spell_flags;
        v4.radius = v2->radius;
        v4.count = v2->count;

        if (a1->field_30 != OBJ_HANDLE_NULL) {
            origin = obj_field_int64_get(a1->field_30, OBJ_F_LOCATION);
        }

        loc_rect.x1 = location_get_x(origin) - v2->radius;
        loc_rect.y1 = location_get_y(origin) - v2->radius;
        loc_rect.x2 = location_get_x(origin) + v2->radius;
        loc_rect.y2 = location_get_y(origin) + v2->radius;

        obj_type_mask = OBJ_TM_SCENERY;
        all = false;

        if ((v2->aoe_flags & Tgt_Obj_T_Critter_Naked) != 0) {
            obj_type_mask |= OBJ_TM_CRITTER;
        }

        if ((v2->aoe_flags & Tgt_Obj_T_Portal_Naked) != 0) {
            obj_type_mask |= OBJ_TM_PORTAL;
        }

        if ((v2->aoe_flags & Tgt_Obj_T_Container_Naked) != 0) {
            obj_type_mask |= OBJ_TM_CONTAINER;
        }

        if ((v2->aoe_flags & Tgt_Obj_T_Wall_Naked) != 0) {
            obj_type_mask |= OBJ_TM_WALL;
        }

        if (obj_type_mask == OBJ_TM_SCENERY
            && (v2->aoe_flags & (Tgt_Obj_Radius | Tgt_Object)) != 0) {
            obj_type_mask = OBJ_TM_ALL & ~OBJ_TM_PROJECTILE;
            all = true;
        }

        object_list_rect(&loc_rect, obj_type_mask, &objects);
        obj_node = objects.head;
        while (obj_node != NULL) {
            int64_t tmp_obj = obj_node->obj;
            if (sub_4F2C60(&tmp_obj) != OBJ_TYPE_SCENERY || all) {
                v3.field_20 = tmp_obj;
                if (sub_4F2D20(&v3)) {
                    sub_4F3F10(v1, tmp_obj);

                    if (v4.count > 0) {
                        if (--v4.count == 0) {
                            break;
                        }
                    }
                }
            }
            obj_node = obj_node->next;
        }
        object_list_destroy(&objects);
    }

    if ((v2->aoe_flags & Tgt_Tile_Radius_Naked) != 0) {
        int x;
        int y;
        bool done;

        sub_4F2600(&v3, &v4, a1->source_obj);
        v4.aoe_flags = v2->aoe_flags & ~Tgt_Object;
        v4.radius = v2->radius;
        v4.count = v2->count;

        origin = a1->field_38;
        if (origin == 0) {
            origin = obj_field_int64_get(a1->field_30, OBJ_F_LOCATION);
        }

        done = false;
        for (y = -v2->radius; y <= v2->radius; y++) {
            for (x = -v2->radius; x <= v2->radius; x++) {
                v3.field_28 = location_make(location_get_x(origin) + x, location_get_y(origin) + y);
                if (sub_4F2D20(&v3)) {
                    sub_4F4050(v1, v3.field_28);

                    if (v4.count > 0) {
                        if (--v4.count == 0) {
                            // To break out of the outer loop.
                            done = true;
                            break;
                        }
                    }
                }
            }

            // NOTE: There is a bug in the original code related to tracking
            // count. It checks for `count > 0 && count == 0` to break out of
            // this outer loop, which is obviously wrong.
            if (done) {
                break;
            }
        }
    }

    if ((v2->aoe_flags & Tgt_Tile_Radius_Wall_Naked) != 0) {
        int rot;
        int sx;
        int sy;
        int range;
        int dx1;
        int dy1;
        int dx2;
        int dy2;

        sub_4F2600(&v3, &v4, a1->source_obj);
        v3.field_40 = a1->field_40;
        v4.aoe_flags = v2->aoe_flags & ~Tgt_Object;
        v4.radius = v2->radius;
        v4.count = v2->count;

        origin = a1->field_38;
        if (origin == 0) {
            origin = obj_field_int64_get(a1->field_30, OBJ_F_LOCATION);
        }

        if (a1->field_18 != 0 && a1->field_38 != 0) {
            rot = location_rot(a1->field_18, a1->field_38);
        } else {
            rot = 0;
        }

        switch (rot) {
        case 1:
            sx = 0;
            sy = -1;
            break;
        case 2:
            sx = 0;
            sy = -1;
            break;
        case 3:
            sx = -1;
            sy = 0;
            break;
        case 4:
            sx = -1;
            sy = 1;
            break;
        case 5:
            sx = 0;
            sy = 1;
            break;
        case 6:
            sx = 1;
            sy = 1;
            break;
        case 7:
            sx = 1;
            sy = 0;
            break;
        default:
            sx = 1;
            sy = -1;
            break;
        }

        dx1 = 0;
        dy1 = 0;
        dx2 = 0;
        dy2 = 0;
        for (range = 0; range < v2->radius; range++) {
            v3.field_28 = location_make(location_get_x(origin) + dx1, location_get_y(origin) + dy1);
            if (sub_4F2D20(&v3)) {
                sub_4F4050(v1, v3.field_28);
                if (v4.count > 0) {
                    if (--v4.count == 0) {
                        break;
                    }
                }
            }

            v3.field_28 = location_make(location_get_x(origin) + dx2, location_get_y(origin) + dy2);
            if (sub_4F2D20(&v3)) {
                sub_4F4050(v1, v3.field_28);
                if (v4.count > 0) {
                    if (--v4.count == 0) {
                        break;
                    }
                }
            }

            dx1 += sx;
            dy1 += sy;
            dx2 -= sx;
            dy2 -= sy;
        }
    }

    if ((v2->aoe_flags & Tgt_Tile_Offscreen_Naked) != 0 && a1->field_20 != OBJ_HANDLE_NULL) {
        sub_4F4050(v1, obj_field_int64_get(a1->field_20, OBJ_F_LOCATION));
    }

    // FIXME: The code below does not look like implementation of cone.
    if ((v2->aoe_flags & Tgt_Cone) != 0) {
        sub_4F2600(&v3, &v4, a1->source_obj);
        v3.field_40 = a1->field_40;
        v4.aoe_flags = v2->aoe_flags & ~Tgt_Object;
        v4.radius = v2->radius;
        v4.count = v2->count;

        if (a1->source_obj != OBJ_HANDLE_NULL) {
            origin = a1->field_38;
            if (origin == 0) {
                origin = obj_field_int64_get(a1->source_obj, OBJ_F_LOCATION);
            }

            if ((v2->aoe_flags & Tgt_Self) != 0) {
                unsigned int obj_type_mask;
                bool all;
                int x;
                int y;
                int64_t loc;
                bool done;

                obj_type_mask = OBJ_TM_SCENERY;
                all = false;

                if ((v2->aoe_flags & Tgt_Obj_T_Critter_Naked) != 0) {
                    obj_type_mask |= OBJ_TM_CRITTER;
                }

                if ((v2->aoe_flags & Tgt_Obj_T_Portal_Naked) != 0) {
                    obj_type_mask |= OBJ_TM_PORTAL;
                }

                if ((v2->aoe_flags & Tgt_Obj_T_Container_Naked) != 0) {
                    obj_type_mask |= OBJ_TM_CONTAINER;
                }

                if ((v2->aoe_flags & Tgt_Obj_T_Wall_Naked) != 0) {
                    obj_type_mask |= OBJ_TM_WALL;
                }

                if (obj_type_mask == OBJ_TM_SCENERY
                    && (v2->aoe_flags & Tgt_Object) != 0) {
                    obj_type_mask = OBJ_TM_ALL & ~OBJ_TM_PROJECTILE;
                    all = true;
                }

                done = false;
                for (y = -v2->radius; y <= v2->radius; y++) {
                    for (x = -v2->radius; x <= v2->radius; x++) {
                        loc = location_make(location_get_x(origin) + x, location_get_y(origin) + y);
                        object_list_location(loc, obj_type_mask, &objects);
                        obj_node = objects.head;
                        while (obj_node != NULL) {
                            int64_t tmp_obj = obj_node->obj;
                            if (sub_4F2C60(&tmp_obj) != OBJ_TYPE_SCENERY
                                || all) {
                                v3.field_20 = tmp_obj;
                                if (sub_4F2D20(&v3)) {
                                    sub_4F3F10(v1, tmp_obj);
                                    if (v4.count > 0) {
                                        if (--v4.count == 0) {
                                            done = true;
                                            break;
                                        }
                                    }
                                }
                            }
                            obj_node = obj_node->next;
                        }
                        object_list_destroy(&objects);
                    }

                    // NOTE: There is a bug in the original code related to tracking
                    // count. It checks for `count > 0 && count == 0` to break out of
                    // this outer loop, which is obviously wrong.
                    if (done) {
                        break;
                    }
                }
            } else {
                int x;
                int y;

                for (y = -v2->radius; y <= v2->radius; y++) {
                    for (x = -v2->radius; x <= v2->radius; x++) {
                        v3.field_28 = location_make(location_get_x(origin) + x, location_get_y(origin) + y);
                        if (sub_4F2D20(&v3)) {
                            sub_4F4050(v1, v3.field_28);

                            // FIXME: Missing count tracking.
                        }
                    }
                }
            }
        }
    }

    if ((v2->aoe_flags & Tgt_No_ST_Critter_Dead) != 0 && a1->field_54 != NULL) {
        mt_obj_node = *a1->field_54;
        if (mt_obj_node != NULL) {
            while (mt_obj_node != NULL) {
                sub_4F3FD0(v1, mt_obj_node->obj);
                mt_obj_node = mt_obj_node->next;
            }
        } else {
            for (idx = 0; idx < v1->cnt; idx++) {
                // FIXME: Leaking node when object handle is null.
                mt_obj_node = mt_obj_node_create();
                if (v1->entries[idx].obj != OBJ_HANDLE_NULL) {
                    mt_obj_node->obj = v1->entries[idx].obj;
                    mt_obj_node->next = *a1->field_54;
                    sub_443EB0(mt_obj_node->obj, &(mt_obj_node->field_8));
                    if (mt_obj_node->obj != OBJ_HANDLE_NULL) {
                        mt_obj_node->type = obj_field_int32_get(mt_obj_node->obj, OBJ_F_TYPE);
                        if (obj_type_is_critter(mt_obj_node->type)) {
                            mt_obj_node->aptitude = stat_level_get(mt_obj_node->obj, STAT_MAGICK_TECH_APTITUDE);
                        }
                    }
                    *a1->field_54 = mt_obj_node;
                }
            }
        }
    }

    if ((v2->aoe_flags & Tgt_All_Party_Critters_Naked) != 0 && a1->source_obj != OBJ_HANDLE_NULL) {
        object_list_team(a1->source_obj, &objects);
        obj_node = objects.head;
        while (obj_node != NULL) {
            sub_4F3FD0(v1, obj_node->obj);
            obj_node = obj_node->next;
        }
        object_list_destroy(&objects);

        if (tig_net_is_active()
            && obj_field_int32_get(a1->source_obj, OBJ_F_TYPE) == OBJ_TYPE_PC) {
            object_list_party(a1->source_obj, &objects);
            obj_node = objects.head;
            while (obj_node != NULL) {
                sub_4F3FD0(v1, obj_node->obj);
                obj_node = obj_node->next;
            }
            object_list_destroy(&objects);
        }
    }

    if ((v2->aoe_flags & Tgt_Parent) != 0
        && obj_type_is_item(obj_field_int32_get(a1->source_obj, OBJ_F_TYPE))) {
        int64_t parent_obj = obj_field_handle_get(a1->source_obj, OBJ_F_ITEM_PARENT);
        if (parent_obj != OBJ_HANDLE_NULL) {
            sub_4F3FD0(v1, parent_obj);
        }
    }
}

// 0x4F4E40
bool sub_4F4E40(int64_t obj, int distance, int64_t* loc_ptr)
{
    int64_t from;
    int64_t to;
    int obj_type;
    int rotation;
    int idx;
    ObjectList critters;
    ObjectNode* node;

    from = obj_field_int64_get(obj, OBJ_F_LOCATION);
    obj_type = obj_field_int32_get(obj, OBJ_F_TYPE);
    *loc_ptr = 0;
    to = from;
    rotation = random_between(0, 8);

    for (idx = 0; idx < distance; idx++) {
        if (!location_in_dir(to, rotation, &to)) {
            return false;
        }
    }

    stru_603B88.obj = obj;
    stru_603B88.from = from;
    stru_603B88.to = to;
    stru_603B88.max_rotations = sizeof(byte_603BC4);
    stru_603B88.rotations = byte_603BC4;
    stru_603B88.flags = PATH_FLAG_0x0010;

    dword_603C98 = sub_41F3C0(&stru_603B88);
    qword_603CB0 = stru_603B88.to;
    qword_603CA8 = stru_603B88.from;

    if ((dword_603C98 > 0 || stru_603B88.field_24 > 0)
        && dword_603C98 < distance + 2) {
        if (stru_603B88.field_24 == 0) {
            stru_603B88.field_24 = dword_603C98;
        }

        to = from;

        dword_603C94 = 0;
        while (dword_603C94 < stru_603B88.field_24) {
            if (obj_type_is_critter(obj_type)) {
                object_list_location(to, OBJ_TM_CRITTER, &critters);
                node = critters.head;
                object_list_destroy(&critters);
                if (node == NULL) {
                    break;
                }
            }

            if (!location_in_dir(to, byte_603BC4[dword_603C94], &to)) {
                return false;
            }
        }
        *loc_ptr = to;
        return true;
    } else {
        *loc_ptr = from;
        return true;
    }
}
