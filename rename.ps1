param(
  [string]$SearchDir = ".",
  [switch]$DryRun
)

Write-Host "Starting FULL batch rename operation across: $SearchDir"
Write-Host "Targeting ALL .c and .h files recursively..."
Write-Host "--------------------------------------------------------"

# --- Mapping old -> new ---
$RenameMap = @{
# Struct Definitions
"prototype_obj" = "cached_proto_handle" # obj.h: Caches the int64_t handle of the prototype, looked up from prototype_oid.

# Public Function Declarations
"sub_405250" = "obj_system_reset" # obj.h: Shuts down and re-initializes all object subsystems.
"sub_405790" = "obj_debug_print_difs" # obj.h: Prints debug information about which fields on an object are marked as modified.
"sub_4058E0" = "obj_create_instance" # obj.h: The primary function for creating a new object instance from a prototype.
"sub_405B30" = "obj_create_instance_with_id" # obj.h: Calls obj_create_instance but then overwrites the new object's OID.
"sub_405CC0" = "obj_clear_data" # obj.h: Deallocates all field data for an object but does *not* deallocate the object itself.
"sub_405D60" = "obj_duplicate" # obj.h: Creates a full, deep copy of an object (handles both prototypes and instances).
"sub_4063A0" = "obj_get_inventory_ids" # obj.h: Allocates and returns an array of ObjectIDs for an object and its inventory items.
"sub_4064B0" = "obj_handles_to_ids" # obj.h: Converts all object handle fields in an object to permanent IDs.
"sub_406520" = "obj_ids_to_handles" # obj.h: Converts all permanent ID fields in an object back to object handles.
"sub_406B80" = "obj_clear_modified_flags" # obj.h: Clears the "changed" bitmap (field_4C) and the modified flag.
"sub_407100" = "obj_field_oid_get" # obj.h: Gets the raw ObjectID structure from a field (which could be a handle or an ID).
"sub_407D50" = "obj_field_reset" # obj.h: Resets a single field on an instance back to its prototype's value.
"sub_408020" = "obj_set_id_from_legacy" # obj.h: Assigns a special (likely legacy or map-specific) ID to an object.
"sub_408D60" = "obj_arrayfield_get_count_internal" # obj.h: Internal implementation for obj_arrayfield_length_get.
"sub_408E70" = "obj_arrayfield_set_count_internal" # obj.h: Internal implementation for obj_arrayfield_length_set.
"sub_40C030" = "obj_get_type_change_array_count" # obj.h: Returns the total number of ints required for the "change" bitmap for a given ObjectType.

# Internal Struct Definitions
"simple_array_idx" = "proto_data_index" # obj.c: (ObjectFieldInfo) For prototypes, this is the direct index into the object->data array.
"complex_array_idx" = "array_type_index" # obj.c: (ObjectFieldInfo) An index for array-like types.
"change_array_idx" = "bitmap_index" # obj.c: (ObjectFieldInfo) The index into the present_field_bitmap or changed_field_bitmap array.
"mask" = "bitmap_mask" # obj.c: (ObjectFieldInfo) The bitmask (e.g., 1 << bitmap_bit) to use at bitmap_index for this field.
"bit" = "bitmap_bit" # obj.c: (ObjectFieldInfo) The bit number (0-31) within the int at bitmap_index.
"cnt" = "fixed_array_size" # obj.c: (ObjectFieldInfo) Used for fixed-size arrays (like OBJ_F_RESISTANCE_IDX).
"type" = "storage_type" # obj.c: (ObjectFieldInfo) The SA_TYPE_ enum defining how the field's data is stored.

# Static Global Variables
"dword_59BE00" = "g_obj_field_block_start_enums" # obj.c: Maps an index to the OBJ_F_*_BEGIN enum for each field block.
"dword_5D10F0" = "g_field_block_change_idx_map" # obj.c: Stores the starting index into the "change bitmap" (field_4C) for each field block.
"dword_5D10F4" = "s_field_enum_idx" # obj.c: A static global used as a temporary counter/index during field enumeration callbacks.
"word_5D10FC" = "s_field_count_temp" # obj.c: A static global used as a temporary counter during obj_init to count total fields.
"dword_5D1100" = "g_field_block_simple_idx_map" # obj.c: Stores the starting index into the "simple array" (object->data) for each field block.
"dword_5D1108" = "s_copy_src_obj_temp" # obj.c: A temporary global pointer to a source Object struct, used in copy callbacks.
"dword_5D110C" = "s_stream_temp" # obj.c: A temporary global TigFile* pointer used by serialization callbacks.
"dword_5D1110" = "s_proto_copy_src_obj_temp" # obj.c: A temporary global pointer, used for prototype-specific copying.
"dword_5D1118" = "s_mem_write_buf_temp" # obj.c: A temporary global MemoryWriteBuffer* used by memory-writing serialization callbacks.
"dword_5D111C" = "s_mem_read_buf_temp" # obj.c: A temporary global uint8_t* (as a read cursor) used by memory-reading callbacks.
"dword_5D1128" = "g_obj_handle_field_list" # obj.c: An array of ObjectField enums that are of type SA_TYPE_HANDLE.
"dword_5D112C" = "g_obj_handle_array_field_list" # obj.c: An array of ObjectField enums that are of type SA_TYPE_HANDLE_ARRAY.
"dword_5D1130" = "g_obj_handle_field_count" # obj.c: The count of fields stored in g_obj_handle_field_list.
"dword_5D1134" = "g_obj_handle_array_field_count" # obj.c: The count of fields stored in g_obj_handle_array_field_list.

# Static Functions
"sub_408430" = "obj_debug_print_art_id" # obj.c: Debug function that prints a human-readable description of a tig_art_id_t.
"sub_408760" = "object_field_set_value_impl" # obj.c: Core internal implementation for setting a non-array field's value.
"sub_4088B0" = "object_arrayfield_set_value_impl" # obj.c: Core internal implementation for setting an array field's value at an index.
"sub_408A20" = "object_field_get_value_impl" # obj.c: Core internal implementation for getting a non-array field's value.
"sub_408BB0" = "object_arrayfield_get_value_impl" # obj.c: Core internal implementation for getting an array field's value.
"sub_408F40" = "obj_arrayfield_get_sa_ptr" # obj.c: Resolves the SizeableArray** for a field, handling prototype fallback.
"sub_409000" = "obj_proto_set_defaults" # obj.c: Sets all default field values for a newly created prototype.
"sub_409640" = "obj_proto_set_item_defaults" # obj.c: Helper for obj_proto_set_defaults that sets item-specific defaults.
"sub_40A400" = "obj_field_info_init" # obj.c: Initializes the entire object_fields global array (types, counts, indices, masks).
"sub_40A740" = "obj_field_get_block_offset" # obj.c: Finds a field's _BEGIN enum and its relative offset from that start.
"sub_40A790" = "obj_field_get_block_index" # obj.c: Gets the index in g_obj_field_block_start_enums for a given _BEGIN enum.
"sub_40A7B0" = "obj_field_info_set_counts" # obj.c: Part of obj_field_info_init. Fills in the .cnt member for fixed-size arrays.
"sub_40A8A0" = "obj_field_info_set_types" # obj.c: Part of obj_field_info_init. Fills in the .type (the SA_TYPE_) for every field.
"sub_40B8E0" = "obj_field_info_set_change_idx_start" # obj.c: Part of obj_field_info_init. Fills in the g_field_block_change_idx_map global.
"sub_40BAC0" = "obj_handle_field_lists_init" # obj.c: Builds the g_obj_handle_field_list and g_obj_handle_array_field_list globals.
"sub_40BBB0" = "obj_handle_field_lists_free" # obj.c: Frees the lists built by obj_handle_field_lists_init.
"sub_40BBF0" = "obj_fields_handles_to_ids" # obj.c: Internal worker that iterates g_obj_handle_field_list to convert handles to IDs.
"sub_40BD20" = "obj_arrayfields_handles_to_ids" # obj.c: Internal worker that iterates g_obj_handle_array_field_list to convert handles to IDs.
"sub_40BDB0" = "obj_fields_ids_to_handles" # obj.c: Internal worker that iterates g_obj_handle_field_list to convert IDs to handles.
"sub_40BE70" = "obj_arrayfields_ids_to_handles" # obj.c: Internal worker that iterates g_obj_handle_array_field_list to convert IDs to handles.
"sub_40BF00" = "obj_array_entry_handle_to_id_callback" # obj.c: A callback for sa_enumerate that converts a single ObjectID handle to an ID.
"sub_40BFC0" = "obj_array_entry_id_to_handle_callback" # obj.c: A callback for sa_enumerate that converts a single ObjectID ID to a handle.
"sub_40C560" = "obj_init_count_fields_callback" # obj.c: A callback for obj_init used to count the number of fields for each object type.
"sub_40C580" = "obj_inst_alloc_field_bitmaps" # obj.c: Allocates the field_48 (present) and field_4C (changed) bitmaps for an instance.
"sub_40C5B0" = "obj_inst_free_field_bitmaps" # obj.c: Frees the bitmaps for an instance.
"sub_40C5C0" = "obj_inst_copy_field_bitmaps" # obj.c: Copies the bitmaps from one instance to another.
"sub_40C610" = "obj_proto_alloc_field_bitmap" # obj.c: Allocates the field_4C (changed) bitmap for a prototype.
"sub_40C640" = "obj_proto_free_field_bitmap" # obj.c: Frees the bitmap for a prototype.
"sub_40C650" = "obj_proto_copy_field_bitmap" # obj.c: Copies the bitmap from one prototype to another.
"sub_40C690" = "obj_proto_clear_field_bitmap" # obj.c: Clears (zeros) the bitmap for a prototype.
"sub_40C6B0" = "obj_proto_init_field_data_callback" # obj.c: Callback for obj_create_proto to initialize all field data pointers to 0.
"sub_40C6E0" = "obj_proto_dealloc_field_data_callback" # obj.c: Callback for deallocating all field data for a prototype.
"sub_40C730" = "obj_proto_copy_field_data_callback" # obj.c: Callback for obj_duplicate to deep-copy field data for a prototype.
"sub_40C7A0" = "obj_inst_copy_field_data_callback" # obj.c: Callback for obj_duplicate to deep-copy field data for an instance.
"sub_40C7F0" = "obj_transient_field_copy" # obj.c: Deep-copies a single transient field's value.
"sub_40C840" = "obj_transient_field_dealloc" # obj.c: Deallocates a single transient field's value.
"sub_40CB40" = "obj_field_get_simple_array_idx" # obj.c: A helper that just returns object_fields[fld].simple_array_idx.
"sub_40CB60" = "obj_inst_dealloc_field_data_callback" # obj.c: Callback for deallocating all modified field data for an instance.
"sub_40CBA0" = "obj_inst_enumerate_modified_fields" # obj.c: Enumerates all fields that are present in the instance (in the field_48 bitmap).
"sub_40CE20" = "obj_inst_enumerate_modified_fields_in_range" # obj.c: Helper for the function above; enumerates in a specific field range.
"sub_40CEF0" = "obj_inst_enumerate_all_fields" # obj.c: Enumerates *all* possible fields for an instance, whether modified or not.
"sub_40D170" = "obj_inst_enumerate_all_fields_in_range" # obj.c: Helper for the function above; enumerates in a specific field range.
"sub_40D230" = "obj_inst_get_field_data_index" # obj.c: Calculates the actual index into the object->data array for a given field.
"sub_40D2A0" = "obj_inst_materialize_field" # obj.c: Implements "copy-on-write." Copies a field's data from the prototype.
"sub_40D320" = "obj_inst_is_field_present" # obj.c: Checks if a field is present in the instance's field_48 bitmap.
"sub_40D350" = "obj_inst_is_field_present2" # obj.c: A redundant copy of sub_40D320.
"sub_40D370" = "obj_inst_set_field_present_bit" # obj.c: Sets the "field present" bit in the field_48 bitmap.
"sub_40D3A0" = "obj_inst_set_field_present_bit_impl" # obj.c: The worker implementation for the function above.
"sub_40D3D0" = "obj_field_is_changed" # obj.c: Checks if a field is present in the field_4C (changed) bitmap.
"sub_40D400" = "obj_field_set_changed_bit" # obj.c: Sets the "field changed" bit in the field_4C bitmap.
"sub_40D450" = "obj_inst_alloc_data_slot" # obj.c: Wrapper around obj_inst_alloc_data_slot_impl.
"sub_40D470" = "obj_inst_alloc_data_slot_impl" # obj.c: Allocates a new slot in object->data by REALLOCing and shifting data.
"sub_40D4D0" = "obj_inst_free_data_slot" # obj.c: Deallocates a field's data and shrinks the object->data array.
"sub_40D670" = "obj_debug_print_changed_field_callback" # obj.c: Callback for obj_debug_print_difs. Prints the field index if it's changed.

# Wall Editor and Drawing Functions
"sub_4E1490" = "wall_editor_update" # wall.c: Function used to update wall properties, restricted to editor mode, currently noted as incomplete.
"sub_4E1C00" = "wall_draw_rects_top_down" # wall.c: Draws solid rectangles representing walls when the game view is in top-down mode.
"sub_4E1EB0" = "wall_draw_grid_overlay_top_down" # wall.c: Draws the visual grid overlay for the wall system in top-down view using a specific color (`dword_603434`).
"sub_4E20A0" = "wall_delete_piece_group_A" # wall.c: Handles the deletion and cleanup logic for the first group of wall piece types (pieces 9 through 20).
"sub_4E25B0" = "wall_delete_piece_group_B" # wall.c: Handles the deletion and cleanup logic for the second group of wall piece types (pieces 21 through 33).
"sub_4E2C50" = "wall_update_adjacent_on_delete" # wall.c: Updates neighboring wall pieces (pieces 34 through 45) after a wall deletion to ensure structural and visual continuity.

# Roof Management and Query Functions
"sub_439890" = "roof_hit_test_screen_xy" # roof.c: Checks if a given screen coordinate hits a non-transparent pixel of a visible roof piece.
"sub_439FF0" = "roof_is_tile_covered_world_xy" # roof.c: Helper function that converts screen coordinates to a location and checks if that location is covered by a roof.
"sub_43A030" = "roof_is_tile_covered" # roof.c: Determines if a tile is covered by an unfaded, non-filled, indoor roof piece.
"sub_4395C0" = "roof_grid_index_from_tile_id" # roof.c: Calculates the index into the roof grid data structure based on the tile ID, using bit shifting operations.
"sub_43A140" = "roof_get_art_screen_rect" # roof.c: Calculates the screen coordinates and dimensions (`TigRect`) for a roof Art ID based on its hot spot and world coordinates.

# Wallcheck (Roof Fading) and Utility Functions
"sub_438530" = "wallcheck_notify_player_moved" # wallcheck.c: Called when the local PC moves to update the last known location (`qword_5E0A08`) and sets the `wallcheck_dirty` flag, prompting a recalculation.
"sub_502FD0" = "tig_art_is_pixel_transparent" # object.c: Engine utility used by hit detection functions (`object_find_by_screen_coords`, `roof_hit_test_screen_xy`) to check if a specific pixel in an Art ID is transparent.
"sub_438570" = "wallcheck_add_or_update_wall" # wallcheck.c: Adds a wall object to the tracking list (`stru_5E0E20`) or updates its status, calling `wallcheck_roof_ref_add` for roof tracking.
"sub_4386B0" = "wallcheck_find_wall_in_list" # wallcheck.c: Internal helper that uses a binary search to quickly locate a specific wall object handle within the internal tracking list.
"sub_438720" = "wallcheck_roof_ref_add" # wallcheck.c: Increments the reference count for a specific roof location (`a1`). If the count reaches one, it calls `roof_fade_on` to make the roof fade out.
"sub_4387C0" = "wallcheck_find_roof_in_list" # wallcheck.c: Internal helper that uses a binary search to locate a specific roof location (`a1`) in the roof reference list (`stru_5E0A10`).
"sub_438830" = "wallcheck_apply_changes" # wallcheck.c: Iterates through all dirty wall entries and applies calculated wall flags and render flags to modify their visibility, followed by invalidating the affected screen area.

# AI Configuration Data Access
"sub_404480" = "ai_get_ai_packet_field" # ai.c: Retrieves a field value from the NPC's AI configuration data (referred to as the "AI packet" in proto definitions).
"sub_4044F0" = "ai_get_ai_packet_flag" # ai.c: Retrieves the state of a specific flag within the NPC's AI configuration data.
"sub_404550" = "ai_get_ai_packet_secondary_flag" # ai.c: Retrieves the state of a secondary flag within the NPC's AI configuration data.
"sub_4045B0" = "ai_set_ai_packet_flag" # ai.c: Sets or modifies a flag within the NPC's AI configuration data.

# Targeting and Hostility Management
"sub_404C70" = "ai_aggro_add" # ai.c: Adds a target to the NPC's aggression or "shitlist".
"sub_404E20" = "ai_set_target" # ai.c: Sets the current primary target for the AI critter (e.g., setting `OBJ_F_NPC_COMBAT_FOCUS`).
"sub_404E80" = "ai_find_target_in_list" # ai.c: Searches a predefined list of objects (such as the shitlist) for a valid combat target.
"sub_404F70" = "ai_find_target_in_sorted_list_bsearch" # ai.c: Performs an efficient binary search on a pre-sorted list of potential targets to find the optimal one.
"sub_4068D0" = "ai_consider_target" # ai.c: Performs checks on a potential target candidate before selecting it as the primary target.
"sub_406930" = "ai_find_best_target" # ai.c: Logic used to select the highest priority or "best" target among candidates (related to `ai_choose_target`).
"sub_406AE0" = "ai_can_see_obj" # ai.c: Checks if the source critter has line of sight to the target object.
"sub_404280" = "ai_find_new_target" # ai.c: High-level function to locate a dangerous or hostile object within perception range.
"sub_404610" = "ai_process_list" # ai.c: Generic function for iterating over and processing an `ObjectList`.

# Status Checks and Core Decisions
"sub_4057A0" = "ai_should_flee" # ai.c: Determines if a critter meets the necessary conditions (HP ratio, party level, etc.) to trigger fleeing behavior.
"sub_405820" = "ai_is_pc_or_follower" # ai.c: Checks if an object is either a player character or a critter currently following a player.
"sub_405860" = "ai_is_berserk" # ai.c: Checks if a critter is under the influence of the Berserk effect or similar states.
"sub_4058A0" = "ai_update_obj" # ai.c: A generic wrapper to update a critter's state, likely called before the main `ai_process` loop.
"sub_404390" = "ai_execute_strategy" # ai.c: The primary dispatch function that runs the AI strategy determined by the critter's current `danger_type`.

# AI Behavioral Strategies
"sub_405020" = "ai_flee" # ai.c: Initiates the general fleeing state by setting the danger source to `AI_DANGER_SOURCE_TYPE_FLEE`.
"sub_405AC0" = "ai_strategy_wander" # ai.c: Executes the AI logic for aimless movement or patrol, often involving waypoints or standpoints.
"sub_405D20" = "ai_strategy_wander_flee" # ai.c: Executes a mix of wandering and conditioned fleeing logic (e.g., fleeing momentarily upon hearing a sound).
"sub_406210" = "ai_strategy_flee_from_object" # obj.h: A specialized strategy function used during object duplication (e.g., polymorph) to calculate a destination location that is safely far away from hostile objects.
"sub_4064F0" = "ai_strategy_berserk" # ai.c: Executes the AI logic when the critter is berserk, typically prioritizing mindless aggression.
"sub_406730" = "ai_strategy_stand_still" # ai.c: Executes the AI logic when commanded to remain stationary (e.g., following a `DIALOG_ACTION_WA` command or due to combat lock).

# Animation Status & Priority Queries
"sub_423300" = "anim_get_current_id" # anim.c: Checks if an active (non-passive) animation goal is currently running for an object.
"sub_4233D0" = "anim_get_priority_level" # anim.c: Retrieves the priority level (0-6) of the object's currently running active animation goal.
"sub_423470" = "anim_is_running" # anim.c: Checks if an animation slot is active and the object's Art ID is set to an animated state.
"sub_4234F0" = "anim_is_combat_anim" # anim.c: Determines if the currently running animation goal is related to combat (i.e., not a passive fidget animation).
"sub_435CE0" = "anim_goal_is_projectile" # anim.c: Checks if the animation goal running on the object is specifically a projectile (`AG_PROJECTILE`) goal.
"sub_4372B0" = "anim_can_move" # anim.c: Checks if any current animation goals impose movement restrictions that prevent the object from moving to a given destination.

# Animation Control & Priority Setting
"sub_423E60" = "anim_set_debug_error_msg" # anim.c: Sets an internal string message for animation debugging status.
"sub_423FE0" = "anim_set_completion_callback" # anim.c: Sets the function invoked when all active animation goals across the system have completed.
"sub_423FF0" = "anim_clear_goals" # anim.c: Attempts to cancel and clean up all animation goals currently running on a specific object.
"sub_424070" = "anim_set_priority_level" # anim.c: Sets the priority level for the primary goal slot of an object, potentially triggering interruption logic.
"sub_436C20" = "anim_goals_push_state" # anim.c: Sets an internal flag indicating the saving of the current animation stack state, typically before running sub-goals.
"sub_436C50" = "anim_goals_set_current" # anim.c: Sets the global scratch animation ID (`g_anim_slot_scratch`) to the provided `anim_id`.
"sub_436C80" = "anim_goals_pop_state" # anim.c: Restores the animation stack state previously saved by `anim_goals_push_state`.
"sub_436CF0" = "anim_goals_clear_current" # anim.c: Clears the global scratch animation ID.
"sub_436D20" = "anim_set_flags" # anim.c: Modifies flags (e.g., `flags1`, `flags2`) on the current animation run info slot.
"sub_436ED0" = "anim_run" # anim.c: Explicitly sets the running flag (`0x40`) on the animation slot to ensure running animations are used during movement.
"sub_437980" = "anim_init_run_info_params" # anim.c: Function used during system initialization to reset the temporary parameters array within the animation system.
"sub_437CF0" = "anim_run_info_param_set" # anim.c: A stub function designed to set animation run info parameters, but currently performs no action.

# Pathfinding and Movement Core
"sub_4261E0" = "anim_path_get_dist" # anim.c: Calculates the distance between two specified world locations.
"sub_426250" = "anim_path_get_rot_to" # anim.c: Calculates the rotation (direction) required for an object to face from location `a1` toward location `a2`.
"sub_426560" = "anim_path_find" # anim.c: The core interface function for requesting pathfinding (`PathCreate`) to a specific destination.
"sub_427000" = "anim_path_is_clear" # anim.c: Checks if the object's current path (movement queue) is clear of obstructions or movement interruptions.

# Movement and Combat Goal Initiation
"sub_433640" = "anim_is_obj_convinced_to_move" # anim.c: High-level decision function that checks object state and initiates a movement goal (`AG_MOVE_TO_TILE` or `AG_RUN_TO_TILE`) if permitted.
"sub_433A00" = "anim_goal_move_to" # anim.c: Initiates an `AG_MOVE_TO_TILE` goal, typically instructing the object to walk to a location.
"sub_433C80" = "anim_goal_run_to" # anim.c: Initiates an `AG_RUN_TO_TILE` goal, forcing the object to use running animations toward a location.
"sub_434030" = "anim_goal_move_to_combat" # anim.c: Initiates a combat movement goal, prioritizing running towards the destination location.
"sub_4341C0" = "anim_goal_move_near_loc" # anim.c: Initiates an `AG_MOVE_NEAR_TILE` goal, instructing the object to move within a specified range of a target location.
"sub_434400" = "anim_goal_move_near_loc_combat" # anim.c: Initiates an `AG_RUN_NEAR_TILE` combat goal, moving the object within range of a target, prioritizing running.
"sub_4348E0" = "anim_deduct_action_points" # anim.c: Checks and consumes action points (`AP`) during turn-based movement or action execution.
"sub_435A00" = "anim_goal_projectile_fire" # anim.c: Starts the trajectory or movement phase of an existing projectile animation goal (`AG_PROJECTILE`).
"sub_4364D0" = "anim_goal_death" # anim.c: Internal utility function that forcefully terminates an animation goal and flags the slot for cleanup.

# Visual Effects (Eye Candy) and Drawing
"sub_4243E0" = "anim_play_eye_candy_on_obj" # anim.c: Initiates an `AG_EYE_CANDY` goal (Priority 5), often used for non-interruptible visual effects like blood or spells.
"sub_424560" = "anim_play_eye_candy_on_obj_ex" # anim.c: Checks if an identical `AG_EYE_CANDY` effect is already running on the object.
"sub_430460" = "anim_draw_all_overlays" # anim.c: Draws all visual overlays managed by the animation system, including Eye Candy effects.
"sub_430490" = "anim_draw_obj_overlay" # anim.c: Draws the specific animation overlay (FX layer) for a single object.
"sub_432D90" = "anim_process_obj_goals" # anim.c: The central dispatcher that processes the current state of all animation goals (movement, combat, visual) on an object.

# Multiplayer Core State Structures and Arrays (Initialized at Startup)
"stru_5E8AD0" = "g_multiplayer_player_slots" # multiplayer.c: An array of `MultiplayerPlayerSlot` structures (size `NUM_PLAYERS`, i.e., 8) storing per-player data like flags and object IDs. Used to map network clients to player characters.
"qword_5E8D78" = "g_multiplayer_trade_partners" # multiplayer.c: An array of `int64_t` object handles (size `NUM_PLAYERS`) tracking the active trade partner for each player.
"qword_5F0E20" = "g_multiplayer_hidden_items" # multiplayer.c: An array of `int64_t` object handles (size `NUM_PLAYERS`) used to track items that are visually hidden from specific clients (e.g., when moving an item during drag-and-drop or transfer).
"off_5F0BC8" = "g_multiplayer_char_data_slots" # multiplayer.c: An array of pointers (`MultiplayerCharDataHeader*`) storing the serialized character data buffer for each player slot.
"stru_5B3FF0" = "g_multiplayer_save_modules" # multiplayer.c: A static array of structures defining modules that must be saved and loaded during multiplayer state synchronization (e.g., Anim, MagicTech, Quest).
"dword_5B3FD8" = "g_multiplayer_current_difficulty" # multiplayer.c: Static integer storing the currently selected game difficulty level in multiplayer (default 10).

# Synchronization and Locking Management
"dword_5F0E1C" = "g_multiplayer_object_lock_list" # multiplayer.c: Head pointer (`MultiplayerObjectLock*`) for the linked list that tracks objects currently locked by a player, preventing simultaneous modification.
"multiplayer_lock_cnt" = "g_multiplayer_lock_count" # multiplayer.c: Static integer (`g_multiplayer_lock_count`) tracking the recursive lock depth for multiplayer-critical operations. A value greater than zero indicates the system is locked, often used to suppress network packets during internal state changes.
"dword_5F0E10" = "g_multiplayer_reset_lock_count" # multiplayer.c: Integer (`g_multiplayer_reset_lock_count`) used to control when the multiplayer system can be reset, preventing unintended resets during critical operations.
"stru_5E8940" = "g_multiplayer_async_callbacks" # multiplayer.c: A dynamic index table (`TigIdxTable`) storing asynchronous callbacks (`MultiplayerAsyncCallback`) needed to resolve object locking requests initiated by clients.
"dword_5F0E18" = "g_multiplayer_async_callback_next_idx" # multiplayer.c: Integer tracking the next available index in the asynchronous callback table.

# Global State and Status Flags
"dword_5F0E00" = "g_multiplayer_session_active" # multiplayer.c: Boolean flag indicating whether a multiplayer session (client or host) is currently running.
"dword_5F0DE0" = "g_multiplayer_join_request_count" # multiplayer.c: Integer counter tracking the number of pending client join requests.
"dword_5F0DE8" = "g_multiplayer_pregen_char_count" # multiplayer.c: Integer storing the count of pre-generated character objects loaded into memory.
"dword_5F0E14" = "g_multiplayer_disconnect_pending" # multiplayer.c: Boolean flag indicating that a network disconnection sequence is currently underway.
"byte_5E8838" = "g_multiplayer_module_path_stripped" # multiplayer.c: A string buffer (`TIG_MAX_PATH` size) storing the path to the current module file, stripped of its GUID.
"byte_5F0BEC" = "g_multiplayer_module_path_guid" # multiplayer.c: A string buffer (`TIG_MAX_PATH` size) storing the full path of the current module file, including the GUID for network identity.
"stru_5F0D98" = "g_multiplayer_current_module_guid" # multiplayer.c: A `TigGuid` structure storing the unique identifier of the module currently loaded for the session.
"multiplayer_mes_file" = "g_multiplayer_mes_file" # multiplayer.c: The file handle (`mes_file_handle_t`) for the "mes\\MultiPlayer.mes" message file, used for dialog and UI text in network interactions.

# Time Events, Iteration, and Message Queuing
"dword_5F0DEC" = "g_multiplayer_save_obj_list" # multiplayer.c: Head pointer (`MultiplayerObjectNode*`) for a linked list that tracks permanent objects that need to be synchronized and saved during multiplayer save operations.
"dword_5F0DF4" = "g_multiplayer_level_scheme_next_idx" # multiplayer.c: Integer used as a cycling index for the next available slot in the `multiplayer_level_scheme_tbl` array.
"dword_5F0DFC" = "g_multiplayer_message_queue_head" # multiplayer.c: Head pointer (`MultiplayerMessageNode*`) for a queue of network messages awaiting processing.
"dword_5B4070" = "g_multiplayer_timeevent_filter_player_idx" # multiplayer.c: Integer used to temporarily store a player slot index when filtering time events (type `TIMEEVENT_TYPE_MULTIPLAYER`) during map or state transfers.
"dword_5B40D8" = "g_multiplayer_player_find_iterator" # multiplayer.c: Integer iterator used by `multiplayer_player_find_first` and `multiplayer_player_find_next` to traverse active player slots efficiently.
"dword_5F0DE4" = "g_multiplayer_pregen_char_handles" # multiplayer.c: Pointer to an array of `int64_t` object handles storing the loaded pre-generated characters.

# Callback Function Pointers
"off_5F0E04" = "g_multiplayer_game_update_override" # multiplayer.c: Function pointer (`void (*)()`) allowing external systems to override the default game state update loop (`multiplayer_update_game_state`).
"dword_5F0E08" = "g_multiplayer_client_joined_callback" # multiplayer.c: Function pointer (`MultiplayerClientJoinedCallback*`) set by UI to receive notification when a client successfully joins the game.
"dword_5F0DF8" = "g_multiplayer_disconnect_callback" # multiplayer.c: Function pointer (`MultiplayerDisconnectCallback*`) invoked when a disconnection occurs, usually after saving the local character.
"dword_5B3FEC" = "g_multiplayer_disconnect_button_handle" # multiplayer.c: Button handle (`tig_button_handle_t`) passed to the disconnect callback function, typically storing the UI button that initiated the disconnection.

# Multiplayer Structures and Callbacks
"S5F0DEC" = "MultiplayerObjectNode" # multiplayer.c: The structure type used for nodes in the linked list (`g_multiplayer_save_obj_list`) that tracks permanent objects requiring synchronization during save operations.
"S5E8AD0" = "MultiplayerPlayerSlot" # multiplayer.c: The structure type used for the array `g_multiplayer_player_slots` which stores persistent, per-player data and flags for up to 8 players.
"S5F0DFC" = "MultiplayerMessageNode" # multiplayer.c: The structure type used for nodes in the linked list `g_multiplayer_message_queue_head`, storing queued network messages awaiting processing.
"S5F0E1C" = "MultiplayerObjectLock" # multiplayer.c: The structure type used for nodes in the linked list `g_multiplayer_object_lock_list`, which tracks objects currently locked by a player to prevent race conditions during item transfers.
"S5F0BC8" = "MultiplayerCharDataHeader" # multiplayer.c: The header structure used to store metadata (OID, level, size) for serialized character data buffers (`g_multiplayer_char_data_slots`).
"S5E8940" = "MultiplayerAsyncCallback" # multiplayer.c: The structure type used to store success and failure function pointers and user data when requesting an object lock asynchronously from the host.
"Func5F0E08" = "MultiplayerClientJoinedCallback" # multiplayer.h: The function pointer type stored in `g_multiplayer_client_joined_callback`. This callback is invoked when a client successfully joins the session.
"Func5F0DF8" = "MultiplayerDisconnectCallback" # multiplayer.h: The function pointer type stored in `g_multiplayer_disconnect_callback`. This callback is run when the disconnection process is complete.

# Multiplayer Session and Core Functions
"sub_49CB80" = "multiplayer_player_slot_init" # multiplayer.c: Initializes a single `MultiplayerPlayerSlot` structure by zeroing its fields and setting default values like `field_24 = 1`.
"sub_49CC50" = "multiplayer_start_server" # multiplayer.c: Initiates the network server functionality by calling `tig_net_start_server`.
"sub_49CC70" = "multiplayer_load_module_and_start" # multiplayer.c: Handles loading the specified module file and initiating play, checking for `.mpc`, `.bmp`, or `_b.bmp` files.
"sub_4A38B0" = "multiplayer_disconnect" # multiplayer.c: Initiates the disconnection process, saving the local character data and either resetting the connection (host) or sending a disconnect packet to the host (client).
"sub_4A39D0" = "multiplayer_set_disconnect_callback" # multiplayer.c: Sets the global function pointer `g_multiplayer_disconnect_callback` and the button handle (`g_multiplayer_disconnect_button_handle`) to be used upon completion of disconnection.
"sub_4A5610" = "multiplayer_apply_settings" # multiplayer.c: Applies currently loaded game settings that affect multiplayer operation (e.g., difficulty, flags).
"sub_4A1F60" = "multiplayer_get_player_obj" # multiplayer.c: Retrieves the object handle (`int64_t`) for the player character associated with a specific player slot index by performing an OID lookup via `objp_perm_lookup` on `g_multiplayer_player_slots[player].field_8`.
"sub_4A2AE0" = "multiplayer_reset_player_slot" # multiplayer.c: Resets the data in a specified player slot by calling `multiplayer_player_slot_init`.
"sub_4A2B60" = "multiplayer_get_player_obj_by_slot" # multiplayer.c: Retrieves the player character object handle (`int64_t`) for a specific player slot index, checking if the client slot is active first.
"sub_4A3D00" = "multiplayer_clear_pregen_char_list" # multiplayer.c: Frees the memory allocated for the list of pre-generated characters (`g_multiplayer_pregen_char_handles`) and optionally destroys the associated objects if flag `a1` is true.
"sub_4A3D70" = "multiplayer_load_pregen_chars" # multiplayer.c: Loads pre-generated characters from `Players\\G_*.mpc` files, populating the internal `g_multiplayer_pregen_char_handles` array.

# Multiplayer Character Data Management (Serialization/Cache)
"sub_4A40D0" = "multiplayer_has_char_data" # multiplayer.c: Checks if a serialized character data buffer (`g_multiplayer_char_data_slots`) is present for the specified player slot.
"sub_4A40F0" = "multiplayer_set_char_data" # multiplayer.c: Allocates and populates the character data buffer (`MultiplayerCharDataHeader` + serialized data) for a player slot, including the OID and level.
"sub_4A4180" = "multiplayer_get_char_data_ptr" # multiplayer.c: Returns a pointer to the start of the raw serialized character data (located after the header).
"sub_4A41B0" = "multiplayer_get_char_data_size" # multiplayer.c: Retrieves the size of the character's serialized data payload from its header.
"sub_4A41E0" = "multiplayer_get_char_data_oid" # multiplayer.c: Retrieves the `ObjectID` of the character stored in the slot's header.
"sub_4A4230" = "multiplayer_get_char_data_header" # multiplayer.c: Returns a pointer to the entire `MultiplayerCharDataHeader` structure for the player slot.
"sub_4A4240" = "multiplayer_get_char_data_total_size" # multiplayer.c: Calculates the total allocated memory size for the character data buffer (header + data + 8 extra bytes).
"sub_4A4320" = "multiplayer_save_local_char" # multiplayer.c: Saves the local PC's serialized character data to a file (`Players\\*.mpc`). Sets the animation priority to `PRIORITY_HIGHEST` (6) before invoking `save_char`.
"sub_4A6470" = "multiplayer_save_char_with_prompt" # multiplayer.c: Saves the character file after checking if the file exists (`tig_file_exists`) and conditionally displaying a prompt (`sub_460BE0`) before overwriting. Sets animation priority to `PRIORITY_HIGHEST` (6).

# Game State and Session Management
"sub_4A6010" = "multiplayer_reset_critter_state" # multiplayer.c: Resets a critter's transient state by clearing HP/Fatigue damage, removing various status flags (like `OCF_PARALYZED` or `OCF_STUNNED`), and resetting poison/disease arrays.
"sub_4A6560" = "multiplayer_strip_guid_from_filename" # multiplayer.c: Removes the unique GUID string from a module file path, storing the result in `g_multiplayer_module_path_stripped`.
"sub_4A2A30" = "multiplayer_update_game_state" # multiplayer.c: The function responsible for running the main game state loop update, utilizing the `g_multiplayer_game_update_override` function pointer if set.
"sub_4A2B00" = "multiplayer_set_client_joined_callback" # multiplayer.c: Sets the `g_multiplayer_client_joined_callback` function pointer, which is executed when a client successfully joins the host session.
"sub_4A2CD0" = "multiplayer_queue_message" # multiplayer.c: Allocates a `MultiplayerMessageNode` structure and prepends it to the `g_multiplayer_message_queue_head` linked list for deferred network message processing.
"sub_4A2D00" = "multiplayer_process_message_queue" # multiplayer.c: Processes the messages stored in the `g_multiplayer_message_queue_head` queue (Note: marked as incomplete in sources).
"sub_4A54A0" = "multiplayer_anim_ping_handler" # multiplayer.c: A handler run during the game `ping` process that attempts to restart active animation goals (`anim_goal_restart`) for synchronization purposes.
"sub_4A54E0" = "multiplayer_magictech_ping_handler" # multiplayer.c: A handler run during the game `ping` process that checks and reschedules maintained spell time events (`magictech_reschedule_timeevent_after_load`) to ensure synchronization.

# Network Transfer and Synchronization
"sub_49D570" = "multiplayer_timeevent_is_xfer" # multiplayer.c: A time event enumeration callback function used to filter `TIMEEVENT_TYPE_MULTIPLAYER` events if the event's parameter matches the player index currently undergoing a file transfer (used during map loading synchronization).
"sub_4A2040" = "multiplayer_send_loading_state" # multiplayer.c: Sends Packet 67 to all connected clients to notify them that the host is entering or exiting a loading state (`a1`).
"sub_4A33F0" = "multiplayer_send_map_data_to_player" # multiplayer.c: Host function responsible for saving the current map state (including serialized modules defined in `g_multiplayer_save_modules`) and initiating the file transfer (`tig_net_xfer_send_as`) of all associated map files to a specific player.
"sub_4A3660" = "multiplayer_send_player_files" # multiplayer.c: Host function that initiates the network transfer of the character's primary files (`.mpc`, `.bmp`, `_b.bmp`) to a specific client.
"sub_4A3780" = "multiplayer_send_local_player_files_to_host" # multiplayer.c: Client function that copies its own character files to a temporary location and initiates a network transfer of those files to the host.

# Multiplayer Object Locking and Synchronization
"sub_4A2E90" = "multiplayer_clear_object_locks" # multiplayer.c: Function used to clear the entire object lock linked list (`g_multiplayer_object_lock_list`), called during multiplayer initialization.
"sub_4A2EC0" = "multiplayer_acquire_object_lock" # multiplayer.c: **Host function.** Attempts to acquire a lock on an object (`item_oid`) for a specific player. If the object is not already locked (`OF_MULTIPLAYER_LOCK` is unset), it sets the flag and calls `multiplayer_add_object_lock`. If already locked by the same player, it increments the lock count (`field_34`).
"sub_4A3030" = "multiplayer_add_object_lock" # multiplayer.c: Allocates a new `MultiplayerObjectLock` node, initializes it with the object IDs and player index, and adds it to the head of the lock list (`g_multiplayer_object_lock_list`).
"sub_4A3080" = "multiplayer_find_object_lock" # multiplayer.c: Searches the object lock list for a node matching the specified `ObjectID`.
"sub_4A30D0" = "multiplayer_release_object_lock" # multiplayer.c: Decrements the reference count (`field_34`) on a lock. If the count reaches zero, it unsets the `OF_MULTIPLAYER_LOCK` flag on the object and calls `multiplayer_remove_object_lock`.
"sub_4A3170" = "multiplayer_remove_object_lock" # multiplayer.c: Removes a specific `MultiplayerObjectLock` node (identified by `ObjectID`) from the linked list.
"sub_4A3230" = "multiplayer_request_object_lock" # multiplayer.h, "item.c, "object.c: **Client function.** Sends a network packet requesting the host to lock an object. It registers success/failure callback functions (`success_func`, `failure_func`) in `g_multiplayer_async_callbacks` to handle the host's response.

# Item Visibility and Hidden Item Management
"sub_4A50D0" = "multiplayer_hide_item" # multiplayer.c: Hides an item (`item_obj`) from a specific player (`pc_obj`). It sets the item's flag `OIF_NO_DISPLAY` and adds the item to the `g_multiplayer_hidden_items` array for the corresponding client slot. It then sends Packet 93 to synchronize the visibility change across the network.
"sub_4A51C0" = "multiplayer_show_item" # multiplayer.c: Makes a previously hidden item visible to a player (`pc_obj`). It clears the item's flag `OIF_NO_DISPLAY` and removes the item from the `g_multiplayer_hidden_items` array. It sends Packet 93 to notify other clients. (Used in inventory drag-and-drop logic).
"sub_4A5290" = "multiplayer_init_hidden_items" # multiplayer.c: Initializes the `g_multiplayer_hidden_items` array (8 slots) by setting all entries to `OBJ_HANDLE_NULL`.
"sub_4A52C0" = "multiplayer_add_hidden_item" # multiplayer.c: Sets the `OIF_NO_DISPLAY` flag on the item and stores its handle in the player's slot within `g_multiplayer_hidden_items`.
"sub_4A5320" = "multiplayer_remove_hidden_item" # multiplayer.c: Clears the `OIF_NO_DISPLAY` flag on the item stored in the player's hidden slot and sets the slot to `OBJ_HANDLE_NULL`.

# Trade Partner Management
"sub_4A5380" = "multiplayer_init_trade_list" # multiplayer.c: Initializes the `g_multiplayer_trade_partners` array (8 slots) by setting all entries to `OBJ_HANDLE_NULL`.
"sub_4A53B0" = "multiplayer_set_active_trade_partner" # multiplayer.c, "inven_ui.c: Sets the active trading partner (`a2`) for the critter (`a1`) by updating the corresponding entry in `g_multiplayer_trade_partners`. Called when opening or closing the Barter/Inventory UI.
"sub_4A5460" = "multiplayer_get_trade_partner_count" # multiplayer.h, "item.c, "inven_ui.c: Counts how many active players currently have the specified object (`a1`) set as their active trade partner. Used to prevent destruction of containers currently involved in trade.

# Auto-Equip and Character Setup (Initial Inventory) 
# These functions are executed when a player character is newly created or loaded into a multiplayer session to ensure they have appropriate starting equipment. The logic reads from `Rules\\AutoEquip.mes`.
"sub_4A5670" = "multiplayer_auto_equip_char" # multiplayer.c: The master function for auto-equipping a character (`obj`). It loads `Rules\\AutoEquip.mes` and calls helper functions to give money, background items, schematic items, armor, weapons, and skill items. It concludes by calling `item_wield_best_all` to equip them.
"sub_4A5710" = "multiplayer_auto_equip_give_money" # multiplayer.c: Calculates money based on the character's level and background adjustments, creates gold object (BP_GOLD), and transfers it to the character's inventory.
"sub_4A57F0" = "multiplayer_auto_equip_give_background_items" # multiplayer.c: Retrieves the item list string based on the character's background and passes it to `multiplayer_auto_equip_give_items_from_string` for creation.
"sub_4A5840" = "multiplayer_auto_equip_give_skill_items" # multiplayer.c: Provides items (like lockpicks or bandages) based on the character's basic or tech skill levels (e.g., SKILL\_PICK\_LOCKS, SKILL\_HEAL), looking up item prototypes in the MES file (messages 1100-1399).
"sub_4A5920" = "multiplayer_auto_equip_give_items_from_mes" # multiplayer.c: Reads item prototype IDs from the `AutoEquip.mes` file entry (`num`). It first checks if the player already has an item from an exclusive list (messages `num + 10`).
"sub_4A59F0" = "multiplayer_auto_equip_give_weapon_items" # multiplayer.c: Determines which weapons to give based on the character's level and skill in Melee, Bow, Throwing, or Firearms. It contains logic for failsafe weapons (e.g., Daggers, Staves) if specialized items are not found.
"sub_4A5CA0" = "multiplayer_auto_equip_give_armor" # multiplayer.c: Determines the best armor to give based on the character's Strength and Level, looking up the appropriate item prototype index in the MES file (messages 1001-1005).
"sub_4A5D80" = "multiplayer_auto_equip_check_has_items" # multiplayer.c: Checks if a character (`obj`) already possesses any item listed in the input string (`str`), typically used to enforce "one per character" rules for certain starting items.
"sub_4A5E10" = "multiplayer_auto_equip_give_items_from_string" # multiplayer.c: Parses a space-separated string of prototype IDs (`basic_proto`). For each ID, it creates the object (`mp_object_create`) and transfers it to the character's inventory (`item_transfer`).
"sub_4A5EE0" = "multiplayer_auto_equip_give_schematic_items" # multiplayer.c: Creates and transfers schematic-related items (produced items) based on the character's known tech skills and degrees, ensuring they are transferable to the critter.

# Object Teleportation
"sub_4A1F30" = "multiplayer_teleport_obj" # multiplayer.c: Performs the local movement of an object (`obj`) to a new location (`location`) with offsets (`dx`, `dy`) using `object_move`. This function is wrapped by multiplayer logic (along with `multiplayer_teleport_obj` at `0x4D3D60`) to ensure network synchronization.

# Multiplayer NOP and Status Queries
"sub_4A3890" = "multiplayer_nop" # multiplayer.c: This function is explicitly defined as `void multiplayer_nop() {}`. It serves as a No Operation (NOP) placeholder.
"sub_4A38A0" = "multiplayer_get_join_request_count" # multiplayer.c: This function returns the value of the global counter `g_multiplayer_join_request_count`.

# Multiplayer Reset Lock Counter Management
# The `reset lock count` (g_multiplayer_reset_lock_count) is used to protect the multiplayer system from being unexpectedly reset (`multiplayer_reset`) during critical operations.
"sub_4A4270" = "multiplayer_inc_lock_reset" # multiplayer.c: Increments the global integer `g_multiplayer_reset_lock_count`.
"sub_4A4280" = "multiplayer_dec_lock_reset" # multiplayer.c: Decrements the global integer `g_multiplayer_reset_lock_count`.

# Multiplayer Difficulty Management
# These functions manage the global difficulty setting used exclusively by the multiplayer session.
"sub_4A55F0" = "multiplayer_get_difficulty" # multiplayer.c: Returns the value of the static global integer `g_multiplayer_current_difficulty`.
"sub_4A5600" = "multiplayer_set_difficulty" # multiplayer.c: Sets the value of the static global integer `g_multiplayer_current_difficulty`.

# Multiplayer PvP Damage Rule
"sub_4A6190" = "multiplayer_is_pvp_damage_disallowed" # multiplayer.c: This function checks if Player vs. Player (PvP) damage should be blocked. It returns `true` if the server options (`TIG_NET_SERVER_PLAYER_KILLING`) are unset (disallowing PvP) AND the attacker (`a1`) and target (`a2`) are different objects, checking specifically if both are PCs. This function is explicitly referenced in the `combat.c.txt` damage checks table `s_mp_damage_checks`.

# Palette and Color Management
"sub_5022B0" = "tig_art_set_palette_adjust_callback" # light.c: This function sets a callback (`sub_4DE0B0`) that is executed when the TIG system needs to adjust the palette of an Art ID based on game state (e.g., indoor vs. outdoor lighting).
"sub_5022C0" = "tig_art_get_palette_adjust_callback" # light.c: Retrieves the currently set palette adjustment callback function.
"sub_5022D0" = "tig_art_cache_invalidate_palettes" # light.c: A crucial function used after changing global lighting colors (indoor/outdoor) to force the graphics engine to re-process and reload palettes for affected Art IDs.
"sub_502E00" = "tig_art_palette_exists" # object.c, "name.c: Checks if an Art ID has an associated palette defined in the art resources. Used during object palette cycling (`object_cycle_palette`) and asset normalization (`sub_41E200`).
"sub_502E50" = "tig_art_frame_get_pixel_color" # light.c: Retrieves the color of a specific pixel within an Art ID frame. Used extensively in light calculation routines to determine if a pixel in a light map is transparent or colored.
"sub_505000" = "tig_art_apply_palette_adjustment" # light.c: Applies the tint or modification defined by the global palette adjustment callback (e.g., tinting tiles based on `light_indoor_color` or `light_outdoor_color`).

# Animation and Critter Properties
"sub_503340" = "tig_art_frame_get_raw_pixels" # object.c: Retrieves the raw pixel data for a specific frame of an Art ID.
"sub_5033E0" = "tig_art_anim_get_frames_for_distance" # anim.c: Used by the animation system, likely to calculate the number of frames required for a movement animation based on distance.
"sub_503F50" = "tig_art_anim_get_property" # anim.c: Retrieves a generic property (like action frame, sound ID, or attack distance) from the animation data associated with an Art ID.
"sub_503F60" = "tig_art_critter_get_size_class" # anim.c: Retrieves the size class (e.g., normal, large) of a critter art ID. This value influences movement animation selection.

# Light Art ID Functions
"sub_504700" = "tig_art_light_id_rotation_get" # name.c: Retrieves the rotation value embedded within a Light Art ID. Used during Art ID normalization/cleanup.
"sub_504730" = "tig_art_light_id_rotation_set" # light.c: Sets the rotation value embedded within a Light Art ID. Used during shadow application (`shadow_apply`).
"sub_504790" = "tig_art_light_id_get_rotation_mode_flag" # name.c: Checks a flag within a Light Art ID that determines if a specific rotation mode is used. Used during Art ID normalization.

# Tile Art ID Special Data
# These functions manage specialized data stored within Tile Art IDs, typically concerning tile blending or variation.

"sub_503700" = "tig_art_tile_id_get_edge_data" # tile.c: Retrieves the edge data embedded in a Tile Art ID (likely used for edge blending).
"sub_503740" = "tig_art_tile_id_set_edge_data" # tile.c: Sets the edge data embedded in a Tile Art ID.
"sub_5037B0" = "tig_art_tile_id_get_variation_data" # tile.c: Retrieves the variation index embedded in a Tile Art ID.
"sub_503800" = "tig_art_tile_id_set_variation_data" # tile.c: Sets the variation index embedded in a Tile Art ID, used when iterating through tile variations.

# Interface and Item Art ID Utilities
"sub_502830" = "tig_art_item_id_get_max_num" # a_name.c: Returns the maximum Art ID number available for item art (likely iterating to find `item_ground.mes` boundaries).
"sub_504390" = "tig_art_interface_id_get_flag" # charedit_ui.c: Retrieves a generic flag associated with an Interface Art ID. This type of Art ID is used extensively by the UI systems (e.g., CharEdit, Inventory, Combat).

# TIG Art Blitting and Rendering Core
"sub_505940" = "art_blit_flags_to_video_blit_flags" # object.c: A conversion utility function that translates the specialized `TigArtBlitInfo` flags (which include blending and palette overrides, like `TIG_ART_BLT_BLEND_ALPHA_CONST`) into raw video blitting flags used by the underlying video buffer system.
"sub_5059F0" = "art_blit_software" # tile.c: The core blitting function that executes the rendering specified in a `TigArtBlitInfo` structure, likely handling the software rendering path or a unified wrapper for blit operations, utilized by functions like `tig_art_blit`.

# TIG Art Cache and File Management
"sub_51AA90" = "tig_art_cache_get_or_load_entry" # animfx.c: This function is crucial for resource management. It attempts to retrieve an existing Art ID cache entry or, if not found, initiates the loading process for the corresponding art file (e.g., when checking `tig_art_exists`).
"sub_51B650" = "tig_art_cache_entry_free_video_buffers" # light.c: Frees the video memory associated with a cached Art ID entry, typically triggered during resource cleanup (like `light_buffers_destroy`) or when the engine switches rendering modes.
"sub_51B710" = "tig_art_file_load" # animfx.c: The low-level function that reads the raw data for a specific `.art` file from disk and parses its header and frame data before caching.
"sub_51BE30" = "art_header_get_num_rotations" # name.c: Retrieves the number of rotations defined in the art file header, used during Art ID creation validation (e.g., when creating Critter, Monster, or Unique NPC Art IDs).
"sub_51BE50" = "tig_art_file_load_cleanup" # name.c: Performs cleanup actions after an Art ID file load attempt (successful or failed), such as closing file handles.
"sub_51BF20" = "art_header_free_frame_data" # name.c: Frees the memory allocated to hold the frame data within an Art ID header structure.

# Global Art Property Tables
# These read-only arrays store lookup data derived from `.art` file headers, indexed by specialized fields within the Art IDs.
"dword_5BE880" = "tig_art_tile_edge_map_1" # tile.c: Global lookup table used for tile edge mapping (tile blending).
"dword_5BE8C0" = "tig_art_tile_edge_map_2" # tile.c: Second global lookup table used for tile edge mapping (tile blending).
"dword_5BE900" = "tig_art_anim_properties" # anim.c: Global table storing properties associated with specific animation numbers (like Action Frame or Sound ID), accessed indirectly by `tig_art_anim_get_property`.
"dword_5BE980" = "tig_art_critter_body_type_properties" # anim.c: Global table storing properties (like movement speed modifiers) associated with specific critter body types (e.g., Dwarf, Halfling).
"dword_5BE994" = "tig_art_monster_specie_properties" # magictech.c: Global table storing properties associated with specific monster species IDs (e.g., elementals).
"dword_5BEA18" = "tig_art_wall_rotation_map_1" # wall.c: Global lookup table used for wall art rotation management.
"dword_5BEA28" = "tig_art_wall_rotation_map_2" # wall.c: Second global lookup table used for wall art rotation management.

# TIG Art System Globals and Configuration
"dword_604714" = "tig_art_cache_last_index" # a_name.c: Global variable holding the index of the last used entry in the Art ID cache array.
"dword_604718" = "tig_art_use_3d_textures" # object.c: Global flag that dictates whether the TIG engine should utilize hardware-accelerated 3D textures (`g_video_is_3d_accelerated`) for rendering or fall back to software/palette-based methods.
"dword_604744" = "tig_art_palette_adjust_callback" # light.c: Global function pointer (set via `tig_art_set_palette_adjust_callback`) storing the address of `sub_4DE0B0`. This callback is dynamically invoked to modify an Art ID's palette based on environmental factors (indoor/outdoor lighting).
"dword_604754" = "tig_art_check_stretch_blit_divide_by_zero" # object.c: Global flag used for debugging or boundary checks during blitting operations involving stretching or scaling to prevent division by zero errors.

"sub_4E6540" = "objid_create_a"
"sub_4E67A0" = "objid_parse_guid_str"
"sub_4E6970" = "objid_parse_p_str"
"sub_4E6A60" = "objid_parse_a_str"
"sub_4E6AA0" = "parse_hex_str_len"
"sub_4E6B00" = "validate_and_copy_hex_str_len"
"sub_4E4FD0" = "objp_perm_lookup_set"
"sub_4E5280" = "objp_perm_get_oid"
"sub_4E52F0" = "objp_perm_lookup_remove"
"sub_4E5300" = "objp_perm_lookup_compact"
"sub_4E57E0" = "objp_perm_lookup_find_index"
"dword_6036B8" = "objp_perm_lookup_table"
"dword_6036C0" = "objp_perm_lookup_capacity"
"dword_6036DC" = "objp_perm_lookup_size"
"sub_4B2210" = "combat_context_init"
"sub_4B2650" = "combat_projectile_hit_callback"
"sub_4B2690" = "combat_projectile_finish"
"sub_4B2870" = "combat_projectile_update"
"sub_4B2F60" = "combat_process_projectile_hit"
"sub_4B3170" = "combat_attack_resolve"
"sub_4B3BB0" = "combat_attack"
"sub_4B4320" = "combat_fidget_check"
"sub_4B5520" = "combat_multiplayer_is_damage_handled"
"sub_4B6C90" = "combat_set_turn_based_mode"
"sub_4B7010" = "combat_tb_process_npc_turn"
"sub_4B7080" = "combat_tb_queue_next_subturn"
"sub_4B7580" = "combat_tb_should_skip_critter"
"sub_4B7DC0" = "combat_tb_critter_is_hidden_and_active"
"sub_4B8040" = "combat_tb_npc_should_fidget"
"sub_4440E0" = "follower_info_init"
"sub_444110" = "follower_info_find"
"sub_444130" = "follower_info_validate"
"sub_43C690" = "object_draw_hover_overlay"
"sub_43CF70" = "object_remove_from_sector"
"sub_43D940" = "object_is_static_type"
"sub_43D9F0" = "object_find_by_screen_coords"
"sub_43E770" = "object_move"
"sub_43F030" = "object_invalidate_render_flags"
"sub_43F710" = "object_scenery_update_animation"
"sub_443770" = "object_clear_render_data"
"sub_4437E0" = "object_get_hover_rect_total"
"sub_443880" = "object_get_hover_art_rect"
"sub_443EB0" = "object_save_ref_init"
"sub_443F80" = "object_save_ref_find"
"sub_444020" = "object_save_ref_validate"
"sub_4445A0" = "object_move_to_actor"
"sub_43C5C0" = "object_draw_hover_underlay"
"sub_43CEA0" = "object_editor_delete"
"sub_43D690" = "object_get_hp_from_points"
"sub_43D630" = "object_get_hp_from_stats"
"sub_43FE00" = "object_check_los_recursive"
"sub_43FDC0" = "object_check_los"
"sub_43FD70" = "object_is_los_blocked"
"sub_442050" = "object_serialize_to_memory"
"sub_4420D0" = "object_deserialize_from_memory"
"sub_442260" = "object_insert_into_world"
"sub_4423E0" = "object_set_offset_internal"
"sub_442520" = "object_update_render_state"
"sub_442D90" = "object_apply_render_colors"
"sub_443620" = "object_get_eye_candy_rect"
"sub_4437C0" = "object_shadow_clear"
"sub_43AAA0" = "object_editor_is_culling_enabled"
"sub_43AAB0" = "object_editor_toggle_culling"
"sub_43F130" = "object_set_palette"
"sub_440700" = "object_is_portal_blocked"
"sub_441FC0" = "object_notify_npc_seen"
"sub_441540" = "object_get_party_size"
"dword_5A548C" = "g_eye_candy_scale_table"
"dword_5E2E70" = "g_object_dirty_rect_list"
"dword_5E2EC8" = "g_object_render_skip_flags"
"dword_5E2F28" = "g_object_editor_culling_enabled"
"dword_5E2F98" = "g_object_list_ref_count"
"dword_5E2E68" = "g_object_hover_overlay_art_slot"
"dword_5E2E6C" = "g_object_hover_underlay_art_slot"
"dword_5E2F48" = "g_video_is_3d_accelerated"
"stru_5E2F60" = "g_object_hover_save_ref"
"dword_5E2E94" = "g_object_hover_is_critter"
"qword_5E2E60" = "g_object_vis_range_y"
"qword_5E2F50" = "g_object_vis_range_x"
"sub_4E5AA0" = "field_metadata_acquire"
"sub_4E5B40" = "field_metadata_release"
"sub_4E5BF0" = "field_metadata_clone"
"sub_4E5C60" = "field_metadata_set_or_clear_bit"
"sub_4E5CE0" = "field_metadata_test_bit"
"sub_4E5D30" = "field_metadata_count_set_bits_up_to"
"sub_4E5DB0" = "field_metadata_iterate_set_bits"
"sub_4E5E20" = "field_metadata_serialize_to_tig_file"
"sub_4E5E80" = "field_metadata_deserialize_from_tig_file"
"sub_4E5F10" = "field_metadata_calculate_export_size"
"sub_4E5F30" = "field_metadata_export_to_memory"
"sub_4E5F70" = "field_metadata_import_from_memory"
"sub_4E4C50" = "memory_read_from_cursor"
"sub_4EB0C0" = "GetTileNameByType"
"sub_4EC4B0" = "InitializePortalNames"
"sub_4EC8F0" = "GetPortalNameFromArtID"
"sub_4EC940" = "GetWallArtIDFromFilename"
"sub_4ECB80" = "ParseWallProtoEntry"
"sub_4ECC00" = "GetWallProtoDamageArtId"
"dword_687680" = "g_tileCategoryBufferSizes"
"dword_603AF8" = "g_tileEdgeData"
"sub_4EB860" = "CheckTileConnectivitySetup"
"sub_4EB770" = "ParseTileEdgeName"
"sub_4EB7D0" = "FindTileIndexByName"
"sub_4EB8D0" = "FindPathBetweenTiles"
"sub_4D9240" = "GetTileHeight"
"sub_426320" = "CalculateMaxPathRotations"
"sub_4A8570" = "AICheckActionPreconditions"
"sub_460C20" = "GetUIActiveObject"
"sub_4AA620" = "AIInitiateFollow"
"sub_4AABE0" = "AISetOrUpdateDangerSource"
"sub_4A9AD0" = "InitiateCombat"
"dword_5F848C" = "g_onCombatInitiationCallback"
"sub_4A9F10" = "AINotifyNearbyNPCsOfEvent"
"sub_4ADE00" = "FindLineOfSightBlocker"
"sub_4AF640" = "CalculateHearingObstructionPenalty"
"dword_5B50C0" = "g_loudnessDistanceTable"
"sub_4AB990" = "AIIsValidShitlistTarget"
"sub_4C1110" = "GetPCWithHighestReaction"
"sub_4C0CE0" = "GetReactionScore"
"sub_4C1290" = "CalculateReactionModifier"
"sub_4C12F0" = "GetReactionLevelForPC"
"sub_4C1360" = "SetReactionLevelForPC"
"sub_42A490" = "AnimGoalUsePicklockSkill"
"sub_4246D0" = "AnimGoalMoveToPause"
"sub_42A440" = "AnimGoalUsePickpocketSkill"
"sub_42B090" = "AnimGoalGenericSelfObj"
"sub_421CE0" = "IsAnimIDMatchForRunInfo"
"sub_422430" = "SaveAnimationGoalParameter"
"sub_422A50" = "LoadAnimationGoalParameter"
"dword_5A597C" = "g_animGoalParamTypes"
"sub_423D10" = "UpdateAnimationGoal"
"sub_425760" = "IsValidAnimationMoveTarget"
"sub_42ACD0" = "AnimGoalCheckTarget"
"sub_437990" = "GetAnimationRate"
"sub_42EF60" = "CheckDirectionalCondition"
"sub_42F000" = "UpdateProjectileAnimation"
"sub_437C50" = "GetPathCoordsAtIndex"
"sub_44E2C0" = "InterruptAnimation"
"sub_4CCD20" = "GetAnimFXNodeByID"
"sub_4CD7A0" = "AddAnimFX"
"sub_44D520" = "StartAnimationGoal"
"sub_44D4E0" = "AnimGoalDataInit"
"sub_44E8C0" = "IsObjectAnimating"
"dword_5B770C" = "g_animFXScaleValues"
"off_5B0530" = "g_animGoalDataParamNames"
"sub_44C840" = "AnimRunSetGoalNode"
"sub_44E6F0" = "AnimGoalAddWithArgs"
"sub_44E940" = "FindAnimationByGoalType"
"sub_44EB40" = "IsPathingToObject"
"sub_44EBD0" = "AnimPathReset"
"sub_44EBE0" = "AnimPathClear"
"sub_44EBF0" = "AnimRunInfoAdvanceGoal"
"sub_4AE720" = "CanCastSpellOrUseTech"
"sub_4503A0" = "IsTechnologicalSchematic"
"sub_450B40" = "ItemHasCharges"
"sub_459040" = "GetObjectSummonerIfSpellFlag"
"sub_455960" = "MagicTechRunInfoFree"
"sub_45BAF0" = "TimeEventAdd"
"sub_45DC90" = "AdjustAlignmentOnKill"
"sub_4574D0" = "ProcessItemUseCompletion"
"sub_461620" = "CalculateMagicTechEffectivenessModifier"
"sub_461F60" = "IsItemWorthless"
"sub_463E20" = "SpawnInventory"
"sub_4EFAE0" = "SetInventorySpawnedFlag"
"sub_4685A0" = "GetProtoHandleFromID"
"sub_4640C0" = "RefreshInventoryForNearbyPlayers"
"sub_4642C0" = "IsItemInInventory"
"sub_464C50" = "WieldItemToFirstAvailableSlot"
"sub_464D20" = "CheckCanWieldItem"
"sub_466A50" = "AddKeyToKeyring"
"sub_467E70" = "GetItemForceRemoveStatus"
"sub_4F2CB0" = "TargetFindAtScreenCoords"
"sub_4F2D10" = "TargetGetHoverTarget"
"sub_4F2D20" = "ValidateTargetEligibility"
"sub_4F4E40" = "FindLocationNearObject"
"sub_41F3C0" = "PathCreate"
"sub_41F840" = "PathfindDirect"
"sub_41F9F0" = "PathfindAStar"
"sub_420330" = "PathBresenhamLineProcessor"
"sub_420660" = "PathCreateProjectile"
"dword_5DE5F8" = "g_pathfinding_timer_start"
"dword_5DE5FC" = "g_pathfinding_op_count"
"dword_5DE600" = "g_pathfinding_time_limit_ms"
"sub_41AFB0" = "SoundSchemeInit"
"sub_41BAF0" = "UpdateAmbientSound"
"sub_41BCD0" = "ResolveSoundPath"
"sub_41BD10" = "FindActiveSoundScheme"
"sub_41BF70" = "ParseAndAddSoundToScheme"
"dword_5D1A30" = "g_preCombatMusicSchemes"
"sub_40DAB0" = "PlayerDestroyLocalPC"
"sub_40DAF0" = "PlayerSetLocalPC"
"sub_40FED0" = "UIEnterTurnBasedMode"
"sub_412F60" = "DialogFileFreeEntries"
"sub_414F50" = "FindPCReplyOptions"
"sub_417590" = "ParseDialogResponseValue"
"sub_41D390" = "GetMissingArtCount"
"sub_44BC60" = "ScriptFindInitBlock"
"sub_45B300" = "TimeEventIsProcessingAllowed"
"sub_45B320" = "TimeEventPause"
"sub_45B340" = "TimeEventResume"
"sub_45B360" = "TimeEventResetPause"
"sub_45A7F0" = "GetCurrentGameTimeMs"
"sub_45A950" = "DateTimeAddMilliseconds"
"sub_44E2A0" = "IsGlobalTimeEvent"
"sub_4B9130" = "ScreenRectToLocationRect"
"sub_4D0090" = "GetSectorDataForLocationRect"
"dword_5B0CBC" = "g_magictech_test_obj_field_map"
"dword_5B32A0" = "g_ammo_type_to_obj_field_map"
"dword_5B32B0" = "g_ammo_type_to_proto_map"
"dword_5B772C" = "g_scale_index_map"
"in_reset" = "g_is_in_reset"
"in_load" = "g_is_loading_game"
"dword_5D10C4" = "g_module_guid_is_set"
"dword_5DE640..5DE668" = "g_anim_state_counters"
"dword_5DE69C..5DE6B0" = "g_anim_state_flags"
"qword_5FC248..5FC270" = "g_combat_turn_state_vars"
"dword_60170C" = "g_animfx_list_registry"
"stru_601700" = "g_animfx_current_anim_id"
"dword_601734" = "g_animfx_mes_handle"
"stru_5B0DF8" = "g_summon_type_table_wild"
"dword_5FC630" = "g_inven_source_initialized"

"qword_596140" = "g_magictech_target_flag_values"
"dword_5B0C30" = "g_magictech_info_flag_map"
"dword_5B0C58" = "g_magictech_anim_goal_map"
"dword_5B0CD4" = "g_magictech_trait_idx_field_map"
"dword_5B0CE4" = "g_magictech_trait_idx_field_count"
"dword_5B0D3C" = "g_magictech_damage_flag_map"
"dword_5B0DC8" = "g_spell_eye_candy_sound_map"
"dword_5B0DE0" = "g_magictech_default_willpower_req"
"dword_5B0BA0" = "g_magictech_timeevent_find_id"
"dword_5B0F20" = "g_magictech_timeevent_find_recharge_id"
"dword_5E6D20" = "g_magictech_spell_list_mes_file"
"dword_5E6D24" = "g_magictech_invalidate_rect_func"
"dword_5E6D90" = "g_magictech_branch_component_idx"
"dword_5E75A0" = "g_magictech_cur_resistance_amount"
"dword_5E75CC" = "g_magictech_cur_eyecandy_has_callback"
"dword_5E75D0" = "g_magictech_cur_target_flags_after_damage"
"dword_5E75D4" = "g_magictech_cur_spell_has_callback"
"dword_5E75D8" = "g_magictech_cur_spell_has_end_callback"
"dword_5E75DC" = "g_magictech_cur_spell_processed_successfully"
"qword_5E75E0" = "g_magictech_cur_ai_attack_target"
"dword_5E75E4" = "g_magictech_unused_5E75E4"
"dword_5E75E8" = "g_magictech_cur_spell_action"
"dword_5E7600" = "g_magictech_disable_iq_requirement"
"dword_5E7604" = "g_magictech_interrupt_lock"
"dword_5E760C" = "g_magictech_recharge_schedule_time"
"dword_5E7624" = "g_magictech_component_branch_flag"
"dword_5E7628" = "g_magictech_parsing_spell_id"
"dword_5E762C" = "g_magictech_cur_spell_cost"
"dword_5E7630" = "g_magictech_callback_lock"
"dword_5E7634" = "g_magictech_timeevent_find_subtype"
"dword_6876DC" = "g_magictech_active_spell_count"
"off_5B0C0C" = "g_magictech_action_names"
"off_5B0C20" = "g_magictech_info_flag_names"
"off_5B0C40" = "g_magictech_anim_goal_names"
"off_5B0C70" = "g_magictech_damage_type_names"
"off_5B0C88" = "g_magictech_add_remove_names"
"off_5B0C90" = "g_magictech_flag_state_names"
"off_5B0C98" = "g_magictech_test_op_names"
"off_5B0CAC" = "g_magictech_test_obj_field_names"
"off_5B0CCC" = "g_magictech_trait_idx_field_names"
"off_5B0CDC" = "g_magictech_trait_idx_lookup_tables"
"off_5B0CF8" = "g_magictech_trait64_source_names"
"off_5B0D04" = "g_magictech_movement_location_names"
"off_5B0D14" = "g_magictech_damage_flag_names"
"off_5B0D64" = "g_magictech_action_aoe_mes_keys"
"off_5BBD70" = "g_magictech_target_flag_names"
"stru_5B0E28" = "g_summon_type_table_peaceful"
"stru_5B0E58" = "g_summon_type_table_elite_wild"
"stru_5B0E68" = "g_summon_type_table_familiar"
"stru_5B0E88" = "g_summon_type_table_wild_scaled"
"stru_5B0EB8" = "g_summon_type_table_undead_scaled"
"stru_5B0ED8" = "g_magictech_summon_tables"
"stru_5E3518" = "g_magictech_cur_target_list"
"stru_5E6D28" = "g_magictech_cur_target_context"
"stru_5E75C0" = "g_magictech_cur_anim_id"

# Initialization and Setup
"sub_44FE30" = "magictech_load_spell_definitions" # magictech.c: Loads spell definitions from MES files and initializes `MagicTechInfo` structures by calling `magictech_build_spell_info_from_mes`.
"sub_44FFA0" = "magictech_load_spell_effects" # magictech.c: Runs during post-initialization to load effect components for all spells by calling `magictech_build_spell_effects_from_mes`.
"sub_450090" = "magictech_build_spell_info_from_mes" # magictech.c: Parses MES strings to populate core spell data like AOE info, stats, and animation details into the `MagicTechInfo` structure.
"sub_4501D0" = "magictech_build_spell_effects_from_mes" # magictech.c: Iterates through MES indices to gather and build specific spell component lists (effects).
"sub_450240" = "magictech_free_spell_components" # magictech.c: Iterates through all spell components across all actions (BEGIN, MAINTAIN, END, etc.) and frees the dynamically allocated memory.
"sub_455710" = "magictech_init_run_info_pool" # magictech.c: Initializes the array of `MagicTechRunInfo` structures (512 slots) used to track active spell instances.
"sub_457100" = "magictech_reset_run_info_pool" # magictech.c: Calls `magictech_init_run_info_pool` to clear all active spell run info data, typically used during map reset.
"sub_457580" = "magictech_init_spell_info_struct" # magictech.c: Sets default requirement fields (e.g., Willpower/Intelligence minimums) and clears duration/component list pointers when initializing a new `MagicTechInfo` structure.

# Spell Execution Flow and State Management
"sub_451070" = "magictech_process_spell_action" # magictech.c: The main function that drives spell execution for a specific action (BEGIN, MAINTAIN, END). It sets the global context and calls the internal `magictech_process`.
"sub_452F20" = "magictech_process_pre_checks" # magictech.c: Performs initial validation checks, ensuring the caster is active (not dead/unconscious) and checking against non-stacking spells and Intelligence requirements.
"sub_4534E0" = "magictech_process_spell_cancellation" # magictech.c: Executes logic to interrupt or end other active spells based on the current spell's cancellation flags.
"sub_453630" = "magictech_init_target_context" # magictech.c: Sets up the global targeting context structure (`g_magictech_cur_target_context`), linking it to the spell's run info lists and objects before component processing begins.
"sub_453710" = "magictech_process_check_caster_antimagic" # magictech.c: Checks if the caster is inhibited by `OSF_ANTI_MAGIC_SHELL`, which results in immediate spell failure if found.
"sub_453FA0" = "magictech_process_state_transition" # magictech.c: Manages the advancement of a spell's state (e.g., from BEGIN to MAINTAIN, or to END) and handles scheduling maintenance or duration time events.
"sub_455C30" = "magictech_internal_run_invocation" # magictech.c: The non-networked function called to start a spell: acquires a run info lock, populates the structure, validates prerequisites, and starts the necessary animation goal (AG_ATTEMPT_SPELL) or time event.
"sub_456FA0" = "magictech_abort_spell_cast" # magictech.c: Forcefully terminates a spell instance (`mt_id`), optionally refunding/charging fatigue cost if requested (flag 0x1).
"sub_457000" = "magictech_run_action" # magictech.c: Sets a new action state (`action`) on a spell instance and immediately processes it via `magictech_process_spell_action`.
"sub_457030" = "magictech_run_action_delayed" # magictech.c: Sets a new action state and processes it using `magictech_process_spell_action_callback`, intended for asynchronous triggers like animation completion.
"sub_457060" = "magictech_process_spell_action_callback" # magictech.c: The dedicated processing path for actions initiated by callbacks (sets specific internal flags like `g_magictech_cur_spell_has_callback = 1`).
"sub_457270" = "magictech_force_end_spell" # magictech.c: Used by system hooks (e.g., object removal) to forcibly transition a running spell instance to the `MAGICTECH_ACTION_END` state without checking interrupt locks.
"sub_4573D0" = "magictech_interrupt_spell_instance" # magictech.c: Searches for a specific spell instance matching the details in `MagicTechInvocation` (spell, source, target) and interrupts it.

# Cost, Resistance, and Validation
"sub_450420" = "magictech_deduct_cost" # magictech.c: The core function for deducting spell cost, handling multiplayer sync, and checking various resource types (fatigue, mana store, power cells).
"sub_4507B0" = "magictech_charge_spell" # magictech.c: A wrapper function that initiates cost deduction by calling `magictech_charge_spell_fatigue`.
"sub_4507D0" = "magictech_charge_spell_fatigue" # magictech.c: Calculates the final fatigue cost for a cast, applying mastery/dwarf modifiers, and passes the cost to `magictech_deduct_cost`.
"sub_450940" = "magictech_check_can_pay_cost" # magictech.c: Checks if the caster possesses sufficient resources (mana/fatigue/charges) to pay the cost of the requested spell action.
"sub_4532F0" = "magictech_charge_maintenance_cost" # magictech.c: Calculates and deducts the standard cost required to maintain a persistent spell.
"sub_453370" = "magictech_charge_maintenance_cost_resisted" # magictech.c: Calculates and deducts the increased maintenance cost incurred when a spell is being actively resisted.
"sub_4537B0" = "magictech_calculate_spell_resistance" # magictech.c: Determines if the spell is resisted by the target during the `MAGICTECH_ACTION_BEGIN` phase, setting resistance flags on the run info.
"sub_4545E0" = "magictech_check_maintenance_conditions" # magictech.c: Verifies that the caster is able to maintain the spell (e.g., checks against the maximum maintained spell count based on Intelligence).
"sub_454700" = "magictech_check_range" # magictech.c: Checks if the distance between source and target is within the spell's defined range.
"sub_4551C0" = "magictech_check_unlock_difficulty" # magictech.c: Checks if the difficulty of a target (e.g., a lockable object) is successfully bypassed by a relevant spell component.

# Targeting, Reflection, and Prerequisite Checks
"sub_455550" = "magictech_check_target_reflection_and_antimagic" # magictech.c: Checks the target for the presence of spell flags like Anti-Magic Shell and Full Reflection during processing.
"sub_456430" = "magictech_check_disallowed_spell_flags" # magictech.c: Verifies that the caster or target does not have flags disallowed by the spell definition (e.g., `disallowed_sf`, `disallowed_tcf`).
"sub_456A10" = "magictech_check_tech_aptitude_for_item" # magictech.c: Checks the critter's aptitude level against the magic/tech complexity of an item to determine usability.
"sub_456A90" = "magictech_is_spell_target_still_valid" # magictech.c: Re-validates the target object against the spell's AOE definition just prior to action execution.
"sub_45A580" = "magictech_is_target_visible" # magictech.c: Checks visibility between source and target, accounting for invisibility and detection flags.

# Time Event Management (Duration and Recharge)
"sub_454790" = "magictech_schedule_maintenance_timeevent" # magictech.c: Calculates the maintenance period and schedules a time event to trigger the next `MAGICTECH_ACTION_MAINTAIN` action.
"sub_4547F0" = "magictech_add_maintenance_timeevent_with_ui" # magictech.c: Adds the maintenance time event and triggers UI updates/synchronization for maintained spells.
"sub_4548D0" = "magictech_add_timeevent_if_not_exists" # magictech.c: Ensures that only one maintenance or duration time event for a specific MT ID is active before adding a new one.
"sub_4570E0" = "magictech_timeevent_find_by_id" # magictech.c: Callback utilized by the time event system to locate a specific duration/maintenance event based on the MT ID.
"sub_459490" = "magictech_process_maintenance_timeevent" # magictech.c: Processes the spell action when a maintenance time event fires.
"sub_4594D0" = "magictech_timeevent_find_item_recharge" # magictech.c: Callback used to locate active magic item recharge events.
"sub_459500" = "magictech_reschedule_timeevent_after_load" # magictech.c: Reschedules persistent spell time events after loading a map or save game.
"sub_459640" = "magictech_recharge_timeevent_add_mana" # magictech.c: Aggregates new recharge costs into an existing recharge time event for a magic item.
"sub_459680" = "magictech_recharge_timeevent_process" # magictech.c: Processes the logic for recharging mana into an item's `OBJ_F_ITEM_MANA_STORE`.

# Information Retrieval and Queries
"sub_4502B0" = "magictech_get_begin_aoe_radius" # magictech.c: Retrieves the Area of Effect radius for the spell's primary (`BEGIN`) action.
"sub_450A50" = "magictech_get_spell_owner" # magictech.c: Determines the actual critter object that owns/wields the casting item, if applicable.
"sub_450AC0" = "magictech_get_active_maintained_spell_count" # magictech.c: Counts the number of spells an object is actively maintaining.
"sub_450B90" = "magictech_get_max_maintained_spells" # magictech.c: Calculates the limit of maintained spells based on the critter's Intelligence score (`STAT_INTELLIGENCE / 4`).
"sub_453410" = "magictech_find_active_spell_on_obj" # magictech.c: Searches the run info pool to find an active spell instance matching a given spell ID and object.
"sub_453B20" = "magictech_get_spell_resistance_chance" # magictech.c: Calculates the resistance percentage against a specific spell ID.
"sub_453CC0" = "magictech_get_item_spell_resistance_chance" # magictech.c: Calculates resistance chance using the spell ID sourced from an item.
"sub_455100" = "magictech_count_spells_applying_flag" # magictech.c: Counts active spells contributing a specific flag to an object's field (e.g., `OBJ_F_SPELL_FLAGS`).
"sub_459170" = "magictech_find_first_spell_with_flag" # magictech.c: Finds the first active spell instance applying a particular status flag to the target.
"sub_459290" = "magictech_find_first_spell_on_obj" # magictech.c: Finds the first active spell instance tied to a specific object, regardless of whether it's the source or target.
"sub_459F20" = "magictech_get_begin_aoe_flags_ptr" # magictech.c: Returns a pointer to the AOE flags used during the spell's initialization/BEGIN action.
"sub_459FF0" = "magictech_is_run_info_id_valid" # magictech.c: Checks if the ID refers to a currently active and properly initialized spell instance.
"sub_45A030" = "magictech_has_item_triggers" # magictech.c: Checks if the spell definition has item triggers, affecting UI/activation logic.
"sub_45A060" = "magictech_get_sector_aptitude_adj" # magictech.c: Retrieves the aptitude adjustment modifier stored on the current sector data structure.

# Serialization and Memory Management
"sub_44F3C0" = "magictech_run_info_save" # magictech.c: Saves the current state of a `MagicTechRunInfo` (active spell instance) to a file stream.
"sub_44F620" = "magictech_run_info_load" # magictech.c: Loads `MagicTechRunInfo` data from a file stream, reconstituting the active spell instance, including detached object references.
"sub_455820" = "magictech_run_info_validate_objects" # magictech.c: Validates all associated object handles within a spell instance after loading or persistence operations.
"sub_4559E0" = "magictech_run_info_reset_temp" # magictech.c: Clears out transient fields in a `MagicTechRunInfo` structure.

# FX and Projectile Handling
"sub_456D20" = "magictech_get_casting_fx_art" # anim.c: Retrieves Art IDs, light IDs, and overlay indices for the primary casting FX, used by the animation goal system.
"sub_456E00" = "magictech_play_secondary_casting_fx" # magictech.h.txt: Plays the secondary 'eye candy' FX when a spell invocation begins.
"sub_456E60" = "magictech_play_fx_by_id" # magictech.h.txt: Represents the `magictech_fx_add` function, which adds a visual effect (FX) onto an object.
"sub_456EC0" = "magictech_remove_fx_by_id" # magictech.h.txt: Represents the `magictech_fx_remove` function, removing a specific FX from an object.
"sub_458AE0" = "magictech_get_spell_icon_art" # magictech.h.txt: Retrieves the UI Art ID used for the spell's icon, typically for maintained spells.
"sub_458B70" = "magictech_get_projectile_art" # magictech.h.txt: Retrieves the visual Art ID required for the spell's missile or projectile.
"sub_458C00" = "magictech_play_projectile_fx" # magictech.h.txt: Plays necessary sound and light effects on a projectile object when it is spawned.
"sub_458CA0" = "magictech_get_projectile_speed" # magictech.h.txt: Retrieves the speed defined for the spell's projectile.
"sub_45A4F0" = "magictech_remove_fx_node" # magictech.c: Utility to remove a visual FX node, likely from the `spell_eye_candies` list.
"sub_45A520" = "magictech_play_resist_fx" # magictech.h.txt: Plays a visual effect, often signaling that a spell attempt was resisted or failed.

# System Hooks and Spell Cleanup
"sub_457530" = "magictech_disassociate_source_item" # magictech.c: Cleans up item associations related to a spell instance, called after spell termination.
"sub_459740" = "magictech_handle_object_destroyed" # magictech.h.txt: System hook called when an object is destroyed, triggering spell cleanup for related spell instances.
"sub_4598D0" = "magictech_handle_object_pre_remove" # magictech.h.txt: System hook called right before an object is removed from the map (e.g., teleport or deletion), forcing spell cleanup.
"sub_459A20" = "magictech_handle_object_fleeing" # magictech.h.txt: System hook to handle spell interaction or cleanup when an object starts fleeing.
"sub_459C10" = "magictech_check_spell_break_on_hit" # magictech.h.txt: Checks if a maintained spell on an object should be forcibly terminated due to damage received.
"sub_459EA0" = "magictech_remove_tempus_fugit_and_familiar_flags" # magictech.h.txt: Utility to specifically remove major status flags (e.g., related to polymorph or summoned familiars) and revert critter art.
"sub_459F50" = "magictech_reset_ai_data" # magictech.h.txt: Clears AI-specific spell targeting and threat data.
"sub_45A480" = "magictech_free_run_info_and_lists" # magictech.c: Deallocates the `MagicTechRunInfo` structure and associated dynamic memory (like summoned object lists).

# Core Mechanics and Components
"sub_450C10" = "magictech_obj_set_spell_flags" # magictech.c: Sets (ORs) spell-related flags (`OBJ_F_SPELL_FLAGS`) on an object.
"sub_452650" = "magictech_component_movement_teleport_to_area" # magictech.h.txt: Implements the spell component logic for teleportation, specifically between areas.
"sub_452CD0" = "magictech_revert_polymorph_art" # magictech.c: Reverts the object's visual appearance and Art ID after a polymorph effect ends.
"sub_453D40" = "magictech_apply_component_target_override" # magictech.c: Executes internal logic to change the target object/location context during component processing.
"sub_453EE0" = "magictech_notify_ai_if_aggressive_spell" # magictech.c: Checks if the spell is aggressive and signals the AI system to potentially react.
"sub_453F20" = "magictech_notify_ai_attack" # magictech.c: Explicitly notifies the AI system that an attack has occurred, contributing to reputation/reaction changes.
"sub_455250" = "magictech_adjust_spell_duration_by_aptitude" # magictech.c: Adjusts the datetime duration of a spell based on the caster's aptitude.
"sub_455350" = "magictech_teleport_obj_with_tile_scripts" # magictech.c: Performs object teleportation while ensuring tile scripts are appropriately triggered.
"sub_4554B0" = "magictech_add_summoned_obj_to_list" # magictech.c: Adds a newly created summoned object to the spell's persistent run info tracking list.

# Data Building (Loading Spells)
"sub_4578F0" = "magictech_build_spell_stats" # magictech.c: Parses and initializes the spell's statistical data (e.g., range, resistance stats) from a definition string.
"sub_457B20" = "magictech_build_spell_anim_info" # magictech.c: Parses and builds the animation configuration data for the spell (e.g., casting and projectile Art IDs).
"sub_457D00" = "magictech_build_spell_flags" # magictech.c: Parses the text definitions of spell flags (e.g., aggressive, tech flags) into the `MagicTechInfo` structure.
"sub_458060" = "magictech_build_effect_info" # magictech.c: Core function responsible for parsing individual spell component parameters (MTC\_DAMAGE, MTC\_EFFECT, etc.) from MES strings.
"sub_45A760" = "magictech_debug_print_obj_info" # magictech.c: Likely an internal helper for printing object handles and related spell status during debugging.

# Core Dialogue Flow Functions
"sub_412FD0" = "dialog_begin" # dialog.c: The primary function to start a dialogue sequence.
"sub_413130" = "dialog_select_option" # dialog.c: Processes the player's choice of a dialogue option, executing necessary actions.
"sub_413280" = "dialog_end" # dialog.c: Terminates the active dialogue session, calling `ai_exit_dialog`.
"sub_4132A0" = "dialog_get_greeting_msg" # dialog.c: Retrieves the initial greeting message the NPC should display to the PC.
"sub_413A30" = "dialog_goto_line" # dialog.c: Forces the conversation to jump to a specific line number in the dialogue file.

# Dialogue Internal Processing
"sub_414810" = "dialog_handle_pc_action" # dialog.c: Handles special actions triggered by PC input, such as starting bartering or performing payments.
"sub_414E60" = "dialog_find_replies" # dialog.c: Determines and builds the list of available PC response options based on current dialogue conditions.
"sub_4150D0" = "dialog_check_conditions" # dialog.c: Evaluates conditions (`conditions` field of `DialogFileEntry`) that must be met for a dialogue line to be available.
"sub_415BA0" = "dialog_execute_actions" # dialog.c: Executes the actions (`actions` field of `DialogFileEntry`) associated with a chosen dialogue line (e.g., setting flags, adjusting reputation).
"sub_4167C0" = "dialog_parse_action_value" # dialog.c: Utility function to parse the numerical value parameter attached to a dialogue action or condition code.
"sub_416840" = "dialog_process_npc_line" # dialog.c: Searches for, loads, and prepares the NPC's text response for display.
"sub_416B00" = "dialog_substitute_tokens" # dialog.c: Replaces placeholders (tokens) in the dialogue text with dynamic values (like names).
"sub_416C10" = "dialog_build_pc_option" # dialog.c: Creates the data structure and display text for a selectable PC dialogue option.

# Speech and Dialogue Utilities
"sub_4189C0" = "dialog_create_speech_id" # dialog.c: Generates the unified ID used for triggering voiced dialogue playback (combines script and line number).
"sub_418A00" = "dialog_parse_speech_id" # dialog.c: Decomposes the unified speech ID back into its script number and line number components.
"sub_418B30" = "dialog_perform_payment" # dialog.c: Manages the transfer of gold from the PC to the NPC, checking for sufficient funds.
"sub_418C40" = "dialog_show_failure_msg" # dialog.c: Displays a canned error response to the player (e.g., failed skill check, insufficient funds).
"sub_419260" = "dialog_handle_greetings" # dialog.c: Runs the hierarchy of checks to determine the appropriate first line of dialogue.
"sub_4197D0" = "dialog_handle_greeting_override" # dialog.c: Handles forced dialogue jumps or specific logic triggered by a met greeting condition.

# AI, Commerce, and Global System Integrations
"sub_4C1020" = "ai_enter_dialog" # ai.c: Notifies the AI system that a critter is entering dialogue, affecting its behavior and reaction values.
"sub_4C10A0" = "ai_exit_dialog" # ai.c: Notifies the AI system that a critter is exiting dialogue.
"sub_45A7C0" = "datetime_get_current" # logbook_ui.c: Utility to retrieve the current game date and time (`DateTime` structure), often used for timestamping events.
"sub_466E50" = "world_drop_object_at" # item.c: Places an item object at a specified world location (used when item transfer is rejected).
"sub_4C1150" = "barter_get_haggled_price" # barter.c: Calculates the final price of an item or service after applying haggling modifiers based on reaction level.

# Facade (Map Overlay) System Data and Functions

"dword_5FF570" = "facade_data_height" # facade.c: Static global variable defining the height (Y dimension) of the façade data grid.
"dword_5FF574" = "facade_data_width" # facade.c: Static global variable defining the width (X dimension) of the façade data grid.
"qword_5FF578" = "facade_data_origin_loc" # facade.c: Static global storing the map location of the origin point for the façade grid (QWORD).
"dword_5FF588" = "facade_current_map_id" # facade.c: Static global storing the Map ID the current façade data is associated with.
"dword_5FF5A0" = "facade_art_id_grid" # facade.c: Pointer to the grid array storing `tig_art_id_t` values for each façade tile location. This grid is allocated via `walkmask_load`.
"dword_5FF5A4" = "facade_top_down_buffers" # facade.c: Pointer to an array of `TigVideoBuffer` pointers used specifically for rendering the façade in top-down view.
"sub_4CA0F0" = "facade_load_data" # facade.c: Function responsible for loading the façade data for a given map (`a1`) and coordinates (`a2`, `a3`), which involves calling `walkmask_load` and allocating video buffers if in top-down view.
"sub_4CA240" = "facade_free_data" # facade.c: Function responsible for freeing memory allocated for the façade grid (`facade_art_id_grid`) and destroying top-down video buffers (`facade_top_down_buffers`).
"sub_4CA6B0" = "facade_clip_rect_and_get_start_indices" # facade.c: Function that clips the drawing rectangle (`loc_rect`) to the bounds of the loaded façade data and calculates the starting `x` and `y` indices for the drawing loop.
"sub_4CA7C0" = "facade_get_data_screen_rect" # facade.c: Retrieves the screen rectangle occupied by the currently loaded façade data, differentiating between isometric and top-down views.
"sub_4CA2C0" = "facade_set-or-update_palette" # facade.c: Function related to managing the façade's color palette, explicitly noted in the source as **"// TODO: Incomplete."**.

# Combat Lookups and Tables
"stru_5B5790" = "s_mp_damage_checks" # combat.c: Static structure or array located near other core combat tables, likely related to checks specific to multiplayer damage handling (based on proximity to other combat tables).
"dword_5B57A8" = "s_dodge_crit_chance_tbl" # combat.c: Static integer array defining chance values for dodging or scoring a critical hit, indexed by training level (`TRAINING_COUNT`).
"dword_5B57FC" = "s_multi_arrow_anim_params" # combat.c: Static integer array containing parameters (likely animation indices or counts) for multi-arrow attacks.
"dword_5B5818" = "s_reaction_to_flag_tbl" # combat.c: Static table used to map NPC/critter reactions (e.g., FEAR, FRIENDLY) to specific `OBJ_F_CRITTER_FLAGS2` values (based on context with other combat tables).

# Turn-Based (TB) and Damage State Globals
"qword_5FC238" = "s_tb_saved_target_obj" # combat.c: Global storing a temporary or saved object handle, typically used within the turn-based system.
"dword_5FC240" = "s_tb_current_turn_node" # combat.c: Global pointer or index storing the current position in the combat turn order list.
"qword_5FC248" = "s_tb_prev_turn_obj" # combat.c: Global storing the object handle of the critter that completed the previous turn.
"dword_5FC250" = "s_tb_current_turn_index" # combat.c: Global storing the numerical index of the current turn within the turn sequence.
"qword_5FC258" = "s_tb_stuck_check_obj" # combat.c: Global storing the object handle of the critter currently being checked for movement blockage or being stuck.
"dword_5FC260" = "s_tb_stuck_check_last_ap" # combat.c: Global storing the last known action points of the stuck check object before the check occurred.
"dword_5FC264" = "s_tb_critter_is_stuck" # combat.c: Global flag indicating the result (Boolean) of a stuck check on a critter.
"dword_5FC26C" = "s_in_damage_script" # combat.c: Global Boolean flag that is set to `true` immediately before executing the `SAP_TAKING_DAMAGE` script hook and set to `false` afterward, used to prevent recursive damage processing.
"qword_5FC270" = "s_tb_debug_obj" # combat.c: Global storing an object handle dedicated for debugging purposes within the turn-based combat system.

# Item and Inventory Management
"sub_4EDF20" = "item_drop_at_loc" # item.c: Function for dropping an item object at a specified location.
"sub_467520" = "player_hotkey_items_to_inventory" # item.c: Moves items from a player's hotkey bar back into the main inventory, typically triggered upon critter death.
"sub_463860" = "critter_drop_inventory_on_decay" # item.c: Forces a critter's inventory to be dropped to the ground, used when critters decay or are destroyed.

# Critter and Object State Management
"sub_465020" = "critter_update_weapon_art" # critter.c: Calculates and updates the critter's visual Art ID based on currently wielded weapons or shields.
"sub_4F0640" = "obj_get_oid" # multiplayer.c: Retrieves the Object ID structure (`ObjectID`) from a 64-bit object handle, used heavily for synchronization and serialization.
"sub_4AD6E0" = "npc_resurrect_ai_init" # ai.c: Initializes or resets AI scheduling for an NPC, specifically invoked during resurrection or revival.
"sub_45E180" = "critter_remove_from_party" # party.h: Handles the critter-level logic for removing an object from the active party, likely calling `party_remove`.
"sub_45E460" = "critter_num_all_followers" # party.c: Utility function to count the total number of followers associated with a critter.
"sub_45F710" = "critter_teleport_map_init" # critter.c: Function used during the cleanup phase of NPC teleportation to manage state data related to map transitions.
"sub_4F0070" = "obj_arrayfield_handle_set" # obj_private.c: A core function for setting an object handle (`int64_t`) value at a specific index within a dynamically sized object array field.
"sub_4F0570" = "obj_arrayfield_set_length" # obj_private.c: Sets the maximum valid length (`cnt`) of an object's array field, managing its allocated data size.

# UI and Synchronization
"sub_4AA580" = "ui_update_combat_hotkeys" # ai.c: Updates the hotkey interface elements when entering or exiting combat mode.
"sub_4EDDE0" = "player_xp_changed_notify" # mp_utils.h: Notifies the UI and network clients of a change in the player's experience points (via Packet 95).
"sub_460630" = "ui_message_post" # ui.h: Posts a message structure to the UI system for immediate display.

# Turn-Based Cursor State and Time Events
"sub_4BB900" = "ui_is_tb_cursor_locked" # ui.h: Checks the current state of the UI cursor lock used during turn-based combat and complex animations.
"sub_4BB8E0" = "ui_tb_lock_cursor" # ui.h: Explicitly locks the UI cursor, preventing normal interaction, typically at the start of a combat action.
"sub_4BB910" = "ui_tb_unlock_cursor" # ui.h: Unlocks the UI cursor state after a locked turn-based operation is complete.
"sub_4BB8F0" = "ui_tb_lock_cursor_maybe" # ui.h: Conditionally locks the UI cursor, usually checking if combat is active or if other preconditions are met.
"sub_45C0E0" = "timeevent_is_queued" # timeevent.c: Checks if a time event of a specific type is currently active or scheduled in the time event system.
"sub_45A820" = "time_get_elapsed_seconds" # timeevent.c: Utility function to retrieve the elapsed game time since the game start (often used for time event and logbook calculations, complementing `datetime_get_current`).

## Core State Machine & Movement
"sub_42CA90" = "AG_CheckAnimNotRunning" # Context: Returns true if no animation is active, allowing a new goal to start.
"sub_42CC80" = "AG_UpdateAnimFrame" # Context: Advances the current animation frame; returns false when the animation is finished.
"sub_42CB10" = "AG_BeginAnim" # Context: Starts the standard animation specified in AGDATA_ANIM_ID.
"sub_426F10" = "AG_CheckIsProne" # Context: Checks if the critter is prone/knocked down, leading to AG_ANIM_GET_UP.
"sub_426E80" = "AG_CheckIsConcealed" # Context: Checks if the critter is hidden, leading to AG_UNCONCEAL.
"sub_433270" = "AG_EndAnimAction" # Context: Common final state; likely consumes Action Points and signals the goal is complete.
"sub_42BD40" = "AG_SetAnim" # Context: A generic state that sets the object's animation to a specific ID.
"sub_42CAA0" = "AG_CheckActionFramePassed" # Context: Checks if the animation's "action frame" (e.g., impact of a swing) has been reached.
"sub_424D90" = "AG_CheckIsNotAtTargetTile" # Context: Returns false if the object is at the goal tile, true otherwise.
"sub_425740" = "AG_CheckPathNotBuilt" # Context: Checks if run_info->path has been populated yet.
"sub_426040" = "AG_BuildPathToTile" # Context: Calculates the path to AGDATA_TARGET_TILE.
"sub_42C610" = "AG_SetRotationFromPath" # Context: Rotates the object to face the first step in its path.
"sub_42B940" = "AG_SetRunningFlag" # Context: Sets the "running" flag for a move goal if the critter can run.
"sub_4305D0" = "AG_UpdateMoveFrame" # Context: The main "tick" for a movement animation, updating the object's offsets.
"sub_42E9B0" = "AG_BeginMoveAnim" # Context: Selects and starts the correct walk/run/sneak animation.
"sub_430F20" = "AG_EndMoveCleanup" # Context: Final state for movement goals to clear path data.
"sub_427730" = "AG_BuildPathNearTile" # Context: Calculates a path to a tile *near* the target.
"sub_425130" = "AG_CheckIsNearObject" # Context: Checks if the object is within the desired range of AGDATA_TARGET_OBJ.
"sub_427990" = "AG_BuildPathNearObject" # Context: Calculates a path to a location near the target object.
"sub_4280D0" = "AG_BuildPathNearObjectCombat" # Context: Same as above, but uses combat pathfinding rules.
"sub_425270" = "AG_CheckIsNearTile" # Context: Checks if the object is within the specified range of a *tile*.
"sub_425340" = "AG_CheckIsNearObject_Follow" # Context: A specific range check used by the AG_FOLLOW goal.
"sub_4254C0" = "AG_CheckPathfindingFailed_MoveNear" # Context: Handles a pathfinding failure for "move near" goals.
"sub_425590" = "AG_CheckPathfindingFailed_MoveNearCombat" # Context: Handles a pathfinding failure for "move near combat" goals.
"sub_426A80" = "AG_BuildPathAwayFromObject" # Context: Calculates a path to flee from a target.
"sub_42C440" = "AG_UpdateRotation" # Context: Turns the object one "step" towards the target rotation.
"sub_42C650" = "AG_EndRotation" # Context: Final cleanup state for rotation.
"sub_42C240" = "AG_RotateAwayFromTargetObj" # Context: Rotates the object to face *away* from the target.
"sub_42C390" = "AG_RotateToTargetTile" # Context: Rotates the object to face a specific location/tile.
"sub_42C780" = "AG_RotateToPathDirection" # Context: Rotates to the current path direction, used in jump_window.
"sub_42BEA0" = "AG_SetRange" # Context: A simple state that sets the AGDATA_RANGE_DATA parameter.
"sub_42BF40" = "AG_SetFollowRange" # Context: Sets the follow distance based on AI flags like ONF_AI_SPREAD_OUT.
"sub_42BFD0" = "AG_CheckIsTooClose_Follow" # Context: Checks if the object is *too close* to its follow target.
"sub_425930" = "AG_BuildPathWander" # Context: Calculates a random path for the wander goal.
"sub_425D60" = "AG_BuildPathWanderSeekDarkness" # Context: Calculates a wander path, preferring darker tiles.
"sub_424E00" = "AG_FindMoveAwayTile" # Context: Finds a free tile for a "please move" request.
"sub_4270B0" = "AG_CheckNextStepBlocked" # Context: Checks if the next tile in the path is blocked by a critter or object.
"sub_425430" = "AG_CheckPathfindingFailed" # Context: Checks if the pathfinding result was a failure.
"sub_427640" = "AG_CheckIsWindow" # Context: Checks if the path is blocked by a jumpable window.
"sub_4272E0" = "AG_CheckIsDoor" # Context: Checks if the path is blocked by a door.
"sub_4288A0" = "AG_CheckCanJumpWindow" # Context: Checks if the object can jump through the window.

# Combat & Spells
"sub_429450" = "AG_CheckTargetNotDead" # Context: Checks if the target object is not dead or destroyed.
"sub_4294A0" = "AG_CheckAttackLOS" # Context: Checks if the attacker has a line of sight to the target.
"sub_429960" = "AG_CheckIsInRange" # Context: Checks if the target is within weapon range.
"sub_432990" = "AG_UpdateAttackAnim" # Context: Advances the attack animation frame, with special logic for ranged weapons.
"sub_42C0F0" = "AG_RotateToTargetObj" # Context: Instantly rotates the object to face its target.
"sub_42B9C0" = "AG_BeginAttackAnim" # Context: Selects the correct attack animation based on weapon type.
"sub_432700" = "AG_BeginAttackAnimWithSound" # Context: More complex version that also plays attack sounds.
"sub_42A630" = "AG_PerformAttackHit" # Context: Called on the "action frame" to calculate and apply damage.
"sub_42A930" = "AG_CheckIsRangedWeapon" # Context: Checks if the critter's equipped weapon is ranged.
"sub_429AD0" = "AG_CheckIsMeleeWeapon" # Context: Checks if the critter's equipped weapon is melee (range <= 1).
"sub_429760" = "AG_CheckAutoAttack" # Context: Checks if the object is set to auto-attack in real-time.
"sub_429B50" = "AG_CheckSpellPrereqs" # Context: Checks Action Points, mana/fatigue cost for a spell.
"sub_429BC0" = "AG_CheckSpellLOS" # Context: Checks line of sight for a spell.
"sub_42C850" = "AG_RotateToSpellTarget" # Context: Rotates to face the spell's target.
"sub_429CD0" = "AG_SpellCleanup" # Context: Cleans up spell data, removes overlays, aborts spell if needed.
"sub_429C40" = "AG_CheckSpellHasProjectile" # Context: Checks if the spell definition includes a projectile.
"sub_424820" = "AG_CreateProjectile" # Context: Wrapper function that calls CreateProjectileInternal.
"sub_429C80" = "AG_CastSpell" # Context: Performs the actual spell effect (e.g., applies damage/status).
"sub_429F00" = "AG_GetSpellCastingArt" # Context: Gets the "eye candy" visual effect for the spell.
"sub_42A010" = "AG_BeginAnimWithPrevId" # Context: Restores the previous animation after a spell cast.
"sub_42A200" = "AG_AbortSpellCast" # Context: Aborts the spell, likely due to interruption or invalid target.
"sub_42BE80" = "AG_SetSkillData" # Context: Stores the skill ID in a scratch parameter for later use.
"sub_42A2A0" = "AG_PerformSkillCheck" # Context: Runs the core skill invocation logic.
"sub_42A430" = "AG_CheckSkillSuccess" # Context: Checks the flag set by AG_PerformSkillCheck.

# Object Interaction
"sub_4284A0" = "AG_CheckTargetIsDoor" # Context: Checks if the target object is a portal/door.
"sub_4284F0" = "AG_CheckDoorIsClosed" # Context: Checks if the door is currently closed.
"sub_428750" = "AG_CheckDoorIsLocked" # Context: Checks if the door is locked.
"sub_4287E0" = "AG_TryUnlockDoor" # Context: Attempts to unlock the door (may fail).
"sub_428690" = "AG_CheckCanOpenDoor" # Context: Checks if the object (critter) is allowed to open doors.
"sub_428550" = "AG_TryOpenDoor" # Context: Performs the logic to open a door (plays sound if locked).
"sub_4246E0" = "AG_PushGoal" # Context: Generic function to push a new goal onto the stack.
"sub_428620" = "AG_CheckDoorNotMagicallyHeld" # Context: Checks if the door is held by a spell.
"sub_428A10" = "AG_PerformUseObject" # Context: The key state that handles using portals, containers, scenery, etc.
"sub_428930" = "AG_SetUseRange" # Context: Sets the interaction range based on the target object's type.
"sub_42BE50" = "AG_SetScratchObjToTargetObj" # Context: Stores the target object in a scratch parameter for later use.
"sub_42A9B0" = "AG_CheckCanPickupItem" # Context: Checks if the item can be picked up.
"sub_428CD0" = "AG_PerformUseItemOnObject" # Context: The action state for using an item on an object.
"sub_428E10" = "AG_PerformUseItemOnObjectWithSkill" # Context: The action state for using an item on an object with a skill.
"sub_429040" = "AG_PerformUseItemOnTile" # Context: The action state for using an item on a location.
"sub_429160" = "AG_PerformUseItemOnTileWithSkill" # Context: The action state for using an item on a location with a skill.
"sub_42A4E0" = "AG_PerformPicklockSkill" # Context: The action state for the picklock skill.
"sub_42D440" = "AG_UpdatePicklockAnim" # Context: Advances the picklock animation, looping it.
"sub_42D300" = "AG_BeginPicklockAnim" # Context: Starts the picklock animation.
"sub_42AB90" = "AG_PerformThrowItem" # Context: The state that actually performs the item throw logic.
"sub_42B440" = "AG_EndThrowItemCleanup" # Context: Cleanup state for the throw item goal.
"sub_42B640" = "AG_EndDoorOpenCleanup" # Context: Final state for the animate_door_open goal.
"sub_42B6F0" = "AG_EndDoorCloseCleanup" # Context: Final state for the animate_door_closed goal.
"sub_42B790" = "AG_CheckDoorNotSticky" # Context: Checks if the door has the OPF_STICKY flag (prevents auto-close).
"sub_42B7F0" = "AG_CheckDoorNotBlocked" # Context: Checks if a critter is in the doorway, preventing it from closing.

# Projectiles & Effects
"sub_42F6A0" = "AG_UpdateProjectileMove" # Context: Advances the projectile's position frame by frame.
"sub_424BC0" = "AG_RotateProjectile" # Context: Sets the projectile's art rotation to match its trajectory.
"sub_424D00" = "AG_DestroySelf" # Context: Destroys the projectile object.
"sub_42A180" = "AG_CheckProjectileAtTarget" # Context: Checks if the projectile has reached its target location.
"sub_429ED0" = "AG_GetProjectileSpeed" # Context: Gets the projectile speed from the spell definition.
"sub_42BEC0" = "AG_UpdateProjectileTargetLoc" # Context: Re-aims the projectile if the target object has moved.
"sub_429E70" = "AG_DestroySelfCleanup" # Context: Final cleanup state for a projectile.
"sub_42CAC0" = "AG_ProjectileHitCleanup" # Context: Calls the projectile hit callback and destroys the projectile.
"sub_4269D0" = "AG_BuildPathStraight_Projectile" # Context: Builds a straight-line path for a projectile.
"sub_42F5C0" = "AG_BeginProjectileMove" # Context: Starts the projectile movement.
"sub_431320" = "AG_CheckEyeCandyNotStarted" # Context: Checks if the eye candy effect has already been started.
"sub_431130" = "AG_CheckFloatingNotStarted" # Context: Checks if the floating effect has already been started.
"sub_431150" = "AG_UpdateFloating" # Context: Moves the object up/down for the floating effect.
"sub_4311F0" = "AG_BeginFloating" # Context: Starts the floating effect.
"sub_431290" = "AG_EndFloating" # Context: Resets the object's vertical offset.
"sub_431B20" = "AG_EndEyeCandyCleanup" # Context: Removes overlays and stops sounds.
"sub_4296D0" = "AG_PlaySoundFromScratch" # Context: Plays a sound ID that was stored in a scratch parameter.

# Status, Fidgets, & Dying
"sub_42D6F0" = "AG_UpdateDyingAnim" # Context: Advances the death animation frame.
"sub_42BC10" = "AG_SetDeathAnimation" # Context: Selects the correct death animation (normal, fire, acid, etc.).
"sub_42D570" = "AG_BeginDyingAnim" # Context: Starts the death animation.
"sub_42FED0" = "AG_ProcessDeath" # Context: Core death logic, applies blood, sets flags.
"sub_42FF40" = "AG_SetDeadState" # Context: Sets object flags like OF_FLAT, OF_NO_BLOCK.
"sub_42FFE0" = "AG_DeadCleanup" # Context: Final cleanup state for a dead object.
"sub_42E070" = "AG_UpdateStunAnim" # Context: Advances the stun animation frame.
"sub_426F60" = "AG_CheckStunFinished" # Context: Checks if the stun duration has expired.
"sub_42B250" = "AG_EndStunCleanup" # Context: Cleanup state for the stun goal.
"sub_42E460" = "AG_CheckNotUnconscious" # Context: Checks if the critter is still unconscious.
"sub_42E8B0" = "AG_UpdateAnimReverse" # Context: Plays an animation backwards one frame.
"sub_42E720" = "AG_BeginAnimReverse" # Context: Starts an animation from its last frame.
"sub_42AFB0" = "AG_EndUnconceal" # Context: Cleanup state for the unconceal goal.
"sub_42E6B0" = "AG_GetUnconcealAnimId" # Context: Gets the correct animation ID for unconcealing.
"sub_4293D0" = "AG_SetNoFleeFlag" # Context: Used to prevent an AI from fleeing.
"sub_4298D0" = "AG_CheckFidgetConditions" # Context: Checks if the object is idle and close to the player.
"sub_42CF40" = "AG_UpdateFidgetAnim" # Context: Advances the fidget animation frame and plays a sound.
"sub_42DCF0" = "AG_CheckAnimLoopConditions" # Context: Checks if a looping animation (like scenery) should animate.
"sub_42DDE0" = "AG_UpdateAnimLoopFrame" # Context: Advances the frame of a looping animation.
"sub_42DED0" = "AG_StopLoopSound" # Context: Stops the looping sound associated with an animation.
"sub_42D7D0" = "AG_BeginJumpAnim" # Context: Starts the jump animation.
"sub_42D910" = "AG_UpdateJumpAnim" # Context: Advances the jump animation frame.
"sub_42F140" = "AG_EndJump" # Context: Final state for the jump goal, resets animation.
"sub_42E1B0" = "AG_BeginKneelMagicHandsAnim" # Context: Starts the "kneel magic hands" animation.
"sub_42E2D0" = "AG_UpdateKneelMagicHandsAnim" # Context: Advances/reverses the "kneel magic hands" animation.

# Static Helper Functions (Utilities)
"sub_423C80" = "anim_schedule_next_tick" # Context: Schedules the next anim_timeevent_process call.
"sub_436220" = "anim_goal_use_picklock_skill_internal" # Context: Specialized setup for the picklock skill goal.
"sub_436720" = "anim_find_blocker_on_tile" # Context: Checks if multiple critters are on the same tile to see who should move.
"sub_436CB0" = "anim_goal_set_flee_flag" # Context: Sets a flag on the animation to indicate it's for fleeing.
"sub_437460" = "anim_modify_data_init" # Context: Initializes the AGModifyData struct for network messages.
"sub_4248A0" = "CreateProjectileInternal" # Context: The core function that creates a new projectile object.
"sub_4257E0" = "anim_get_movement_path_flags" # Context: Gets pathfinding flags based on critter status (polymorph, etc.).
"sub_425840" = "anim_check_los_ignoring_target" # Context: A special LOS check that temporarily makes the target non-blocking.
"sub_425BF0" = "anim_set_pathfinding_flags" # Context: Sets up PathCreateInfo flags based on critter state before pathing.
"sub_426500" = "anim_path_find_from_obj" # Context: A wrapper for anim_path_find that gets the object's current location.
"sub_427110" = "anim_check_next_step_blocked_internal" # Context: The core logic for checking if the next tile is blocked.
"sub_4273B0" = "anim_find_jumpable_window_at_loc" # Context: Checks for a wall "P-piece" that signifies a window.
"sub_4294F0" = "CheckLOS" # Context: The core Line-of-Sight check function.
"sub_42AA70" = "PerformItemPickup" # Context: Handles the logic of transferring an item to an object's inventory.
"sub_42EDC0" = "anim_select_move_animation" # Context: Selects the correct animation (walk, run, sneak) based on flags.
"sub_42EE90" = "anim_calculate_move_delay" # Context: Calculates animation delay for movement based on STAT_SPEED.
"sub_42FD70" = "anim_check_knockback_path_blocked" # Context: Checks if a knockback path is blocked by a wall or critter.
"sub_4302D0" = "anim_fidget_find_candidates" # Context: Finds all valid critters in a rectangle that can perform a fidget animation.
"sub_4303D0" = "anim_fidget_is_candidate" # Context: Checks if a single critter is eligible to fidget.
"sub_430FC0" = "anim_deduct_move_cost" # Context: Deducts Action Points and/or Fatigue for taking a step.
"sub_431550" = "anim_play_eyecandy_looping_sound" # Context: Starts a looping sound associated with a visual effect.
"sub_4319F0" = "anim_update_looping_sound_pos" # Context: Updates the 3D position of a looping sound.
"sub_432CF0" = "anim_check_ammo" # Context: Checks if a critter has enough ammo for their ranged weapon.
"sub_4339A0" = "anim_can_obj_be_commanded" # Context: A general check to see if an object is idle and controllable.
"sub_437D00" = "anim_goal_pop_stack_internal" # Context: Pops one goal from the stack and calls its cleanup function.
"sub_423FB0" = "anim_recount_active_goals" # Context: Recalculates the global counter for active goals.
"sub_42AE10" = "AG_CheckIsInTalkRange" # Context: Checks if the object is within ai_max_dialog_distance.

# Stub Functions
"sub_423530" = "AnimGoal_PathInitCheck1" # Inferred: Likely an early, internal check or initialization function within the Pathfinding/Animation Goal setup routines.
"sub_423540" = "AnimGoal_PathInitCheck2" # Inferred: Likely a related internal path/animation helper function.
"sub_423550" = "AnimGoal_PathInitCheck3" # Inferred: Likely a related internal path/animation helper function.
"sub_4246C0" = "AnimGoal_CheckMovementLimits" # Inferred: Positioned near core animation and pathfinding functions (e.g., `AG_CheckPathfindingFailed_MoveNearCombat` at 0x425590).
"sub_427710" = "AnimGoal_MoveStateSetup" # Inferred: Internal function dealing with setting up state data for movement or rotation goals.
"sub_427720" = "AnimGoal_MoveStateCleanup" # Inferred: Likely cleans up or finalizes state data related to animation movement.
"sub_428890" = "AnimGoal_CheckTargetObjectValid" # Inferred: Often in this range are functions that validate the target's handle or status before proceeding with a complex action goal.
"sub_429370" = "AnimGoal_SetupGenericAction1" # Inferred: Start of a sequence of complex action setup routines in the AG system.
"sub_429380" = "AnimGoal_SetupGenericAction2" # Inferred: Part of a complex action setup routine.
"sub_429390" = "AnimGoal_SetupGenericAction3" # Inferred: Part of a complex action setup routine.
"sub_4293A0" = "AnimGoal_AttackStartCheck" # Referenced at Index 5. This function starts a sequence leading to `AG_KILL` or `AG_MOVE_NEAR_OBJ`, indicating an initial validity or action sequence check for combat engagement.
"sub_4293B0" = "AnimGoal_TransitionToKill" # Referenced at Index 6. Follows the initial attack check and leads directly to executing `AG_KILL`, suggesting it handles final transition logic before the attack opcode runs.
"sub_4293C0" = "AnimGoal_TransitionToPickpocket" # Referenced at Index 7. Leads directly to executing `AG_PICKPOCKET`.
"sub_429420" = "AnimGoal_CheckKillPreparation" # Referenced at Index 8. Involved in another kill preparation sequence (using `AGDATA_SELF_OBJ`).
"sub_429430" = "AnimGoal_CheckPickpocketPreparation" # Referenced at Index 9. Involved in another pickpocket preparation sequence.
"sub_429440" = "AnimGoal_SpellTargetSetup" # Referenced at Index 1 within the `AG_ATTEMPT_SPELL` sequence table. This is the very first step in spell casting, indicating target resolution and setup.
"sub_429B40" = "AnimGoal_SpellSetupHelper" # Inferred: Located near core spell/weapon handling functions (`sub_429BB0`).
"sub_429BB0" = "AnimGoal_CheckWeaponForSpellCast" # Referenced at Index 5 in the spell sequence. This check precedes `AG_PICK_WEAPON`, implying it validates item status or selects the appropriate weapon/hand item for casting preparation.
"sub_42FEA0" = "AnimGoal_CleanupPathing" # Inferred: Located in the general animation/path system area, possibly handling final cleanup tasks for finished goals.
"sub_42FEB0" = "AnimGoal_CheckTargetPosition" # Inferred: A low-level check, possibly comparing the object's current location to a target location in the animation pipeline.
"sub_42FEC0" = "AnimGoal_UpdateRunInfoFlags" # Inferred: Handles modification or querying of flags within the core `AnimRunInfo` structure.
"sub_432D50" = "AnimGoal_GetCritterAnimationID" # Inferred: The address is near `anim_goal_make_knockdown` (`0x435B30`), suggesting it handles retrieving or manipulating the critter's Art ID (`tig_art_id_t`) or animation status.

# Misleading Function Renames
"anim_goal_death" = "anim_goal_resolve_crowding" # Context: Does not handle death. It checks for multiple critters on one tile and makes one move.
"anim_goal_is_projectile" = "anim_goal_try_fidget" # Context: Does not check for a projectile. It calls anim_goal_fidget.
"anim_can_move" = "anim_set_parent_obj_for_all_goals" # Context: Does not return a bool. It's a setter that iterates and modifies all of an object's goals.
"anim_run" = "anim_goal_set_run_near_flag" # Context: Does not "run" an animation. It sets a flag (0x4000) for the "run near" goal.

# Global Variable Suggestions
"dword_5E3500" = "g_anim_system_active" # Context: Flag to enable/disable the animation system.
"dword_6876E4" = "g_anim_save_version" # Context: Version number for the animation save-game data.
"dword_5DE6CC" = "g_anim_please_move_delay" # Context: Delay in ms used by the AG_PLEASE_MOVE goal.
"qword_5DE6D8" = "g_anim_last_fidget_obj" # Context: Stores the last object that fidgeted to avoid repetition.
"dword_5DE6E0" = "g_anim_disable_fidgets" # Context: A global flag to disable fidget animations.
"dword_5DE6E4" = "g_anim_float_offset_y" # Context: Vertical render offset set by the "floating" status effect.
"stru_5A1908" = "g_anim_slot_scratch" # Context: A globally used "scratchpad" AnimID variable.
"dword_5E34F4" = "g_anim_interrupt_priority_3_flag" # Context: A flag to interrupt all priority 3+ goals on the next tick.
"dword_5E34F8" = "g_anim_completion_callback" # Context: A global function pointer for a callback.
"dword_5A5978" = "g_anim_current_run_index" # Context: Stores the index of the animation slot currently being processed.

# Animation Goal System Globals
"dword_739E40" = "g_anim_unknown_739E40" # anim_private.c: Global integer variable explicitly defined at address 0x739E40.
"dword_739E44" = "g_anim_unknown_739E44" # anim_private.c: Global integer variable explicitly defined at address 0x739E44.

# Animation Slot and Run Info Management
"sub_44C9A0" = "AnimGoalIsPassive" # anim_private.h: Function prototype indicates it checks if a goal is passive. This check is used when calculating `animNumActiveGoals` to ensure passive animations (like fidgets) don't inflate the count.
"sub_44C8F0" = "AnimRunClearGoalNode" # anim_private.h: Function prototype indicates it clears the goal node associated with a run info structure. It decrements `animNumActiveGoals` if the goal being cleared was active (Priority 2 or higher). It is used when a goal finishes.
"sub_44CB60" = "AnimIsProcessingSlot" # anim_private.c: Returns whether the animation system is currently executing a goal time event (`g_anim_current_run_index != -1`). This is used to check if objects can perform certain operations, such as scenery updates.
"sub_44CCB0" = "AnimAllocateSlot" # anim_private.h: Function responsible for finding the first free slot (index 0 to 215) in the `anim_run_info` array. If successful, it initializes the slot's ID, sets the `flags` to active (0x1), and increments `g_anim_system_active`.
"sub_44D0C0" = "AnimStub_44D0C0" # anim_private.c: Function is defined as a stub that takes an `AnimRunInfo` pointer but performs no action (`(void)run_info;`). It is used as a placeholder during multiplayer synchronization (e.g., when updating path flags).
"sub_44D0D0" = "AnimSendFreeSlotPacket" # anim_private.c: Function responsible for notifying network clients when an animation slot is freed or killed. This is crucial for synchronizing slot states across multiplayer clients.

# Animation Goal Initialization and Starting
"sub_44D500" = "AnimGoalDataInitNoPriority" # anim_private.h: Internal function that initializes an `AnimGoalData` structure without automatically setting the object's priority level. It calls `AnimGoalDataInitInternal` with the flag to skip priority setting (`a4 = false`). This is used for sub-goals or goals where priority is managed externally.
"sub_44D540" = "AnimGoalStartEx" # anim_private.h: The primary function used to start an animation goal, providing optional control over initialization flags. If multiplayer is active, it handles serialization and packet transmission before calling `AnimGoalStartLocal`.
"sub_44D730" = "AnimGoalStartLocal" # anim_private.h: The local execution path for starting a goal. It either allocates a new slot via `AnimAllocateSlot` or uses an existing ID via `anim_allocate_this_run_index`, sets the initial goal data, and schedules the first time event.

# Goal Stacking and Interruption
"sub_44DBE0" = "AnimGoalAddSubGoal" # anim_private.h: The public facing function for pushing a new goal (`goal_data`) onto the current animation stack (`anim_id`). It wraps the local `anim_subgoal_add_func` and handles multiplayer synchronization (sending a Packet 7). This mechanism is used to chain multi-step actions like spell casting or complex movement.
"sub_44E050" = "InterruptAttackOnParty" # anim_private.h: Checks if a critter (`a1`) is performing an `AG_ATTACK` or `AG_ATTEMPT_ATTACK` against a specific target (`a2`) OR their leader. If a match is found, it calls `InterruptAnimation` with Priority 5 (`PRIORITY_CRITICAL`).
"sub_44E0E0" = "InterruptAttackOnTarget" # anim_private.h: Checks if a critter (`a1`) is performing an attack goal against a specific target (`a2`). If a match is found, it calls `InterruptAnimation` with Priority 5 (`PRIORITY_CRITICAL`).
"sub_44E160" = "AnimGoalCancel" # anim_private.h: Explicitly attempts to cancel and free a single animation slot (`anim_id`). It clears any associated time events and executes cleanup functions (`subnodes.func`) for all goals on the stack before calling `anim_free_run_index`.
"sub_44E4D0" = "InterruptAnimsByType" # anim_private.h: Iterates through all active animation slots for an object (`obj`) and interrupts any goal matching the specified `goal_type`. The interruption priority is derived from the goal type specified by `a3` (or `PRIORITY_HIGHEST` if `a3` is -1).
"sub_44E710" = "AnimGoalFindExisting" # anim_private.h: Searches for an active animation slot belonging to `obj` that matches a potential or sub-goal type (`goal_data->type`) and its object parameters (`params` to `params`). If found, it returns the associated `AnimID` via `anim_id`. This is used to prevent stacking identical goals.

# Animation Goal (AG) State and Utilities
"dword_5B052C" = "s_slotToClearTimeEvents" # anim_private.c: Static global integer variable (`s_slotToClearTimeEvents`) used to temporarily store an animation slot number when clearing associated time events.
"stru_5E3000" = "s_animIdToClearTimeEvents" # anim_private.c: Static global `AnimID` structure used to temporarily hold the `AnimID` of a slot whose time events need to be cleared via `timeevent_clear_one_ex`.
"stru_5E33F8" = "s_scratchPath" # anim_private.c: Static global `AnimPath` structure used as temporary storage for pathfinding data, defined explicitly at this address.
"sub_44D240" = "AnimResetSlot" # anim_private.c: Function (`AnimResetSlot`) that clears the data (`anim_obj`, `cur_stack_data`, `flags`, `current_goal`) for an animation slot index (`index`) and clears its associated time events using `TimeEventMatchesAnim`.
"sub_44CB70" = "TimeEventMatchesAnim" # anim_private.c: A `TimeEventEnumerateFunc` callback function used with `timeevent_clear_one_ex` that checks if the time event's parameter matches the slot number stored in `s_animIdToClearTimeEvents`.
"sub_44D3B0" = "AnimGoalDataInitInternal" # anim_private.h: Internal helper function called by `AnimGoalDataInit` and `AnimGoalDataInitNoPriority` to initialize an `AnimGoalData` structure with the object handle (`obj`) and goal type (`goal_type`), optionally setting the priority.
"sub_44EAD0" = "AnimGoalTypeIsPersistent" # anim_private.h: Function used to check if a specific animation goal type is considered persistent, meaning it should not be interrupted by normal combat goal flow (e.g., AG_ANIM_FIDGET).

# AI Global Data Structures
"stru_5B5088" = "g_npcWaitHereDelays" # ai.c: Static global array of `DateTime` structures that define the waiting duration delays for NPCs commanded to "Wait Here".
"dword_5F5CB0" = "g_aiParamsTable" # ai.c: Static global array of `AiParams` structures (150 entries), which contain numerical parameters governing NPC behavior, such as flee percentage and minimum combat distance.
"Func5F848C" = "AiCombatInitiationCallbackFunc" # ai.c: Static global function pointer (`g_onCombatInitiationCallback`) that can be set by external modules (like the UI) to receive notification when a critter initiates combat.

# AI Execution and State Management
"sub_4AA300" = "ai_clear_hostility" # ai.c: Function used to stop an NPC from attacking a target (`a2`). It clears `OBJ_F_NPC_COMBAT_FOCUS` and `OBJ_F_NPC_WHO_HIT_ME_LAST`, removes the target from the shitlist (`ai_shitlist_remove`), adjusts reaction, and interrupts ongoing attack animations (`InterruptAttackOnParty`).
"sub_4ABBC0" = "ai_get_or_find_combat_target" # ai.c: Retrieves the current `OBJ_F_NPC_COMBAT_FOCUS` target if it is still valid (checking the shitlist). If invalid, it unlocks the target and calls `ai_find_target` to find a new combat target.
"sub_4AD790" = "ai_schedule_process_immediate" # ai.c: Clears any existing AI time event for the object and immediately schedules a new `TIMEEVENT_TYPE_AI` event, forcing the NPC's AI processing to run immediately.
"sub_4A88D0" = "ai_context_init" # ai.c: Initializes the `Ai` context structure with the object's core data, including setting the `danger_type` to `AI_DANGER_SOURCE_TYPE_NONE` and looking up the current `danger_source` via `ai_danger_source`.
"sub_4A92D0" = "ai_update_state" # ai.c: The core AI state machine update function. Based on the current `danger_type`, it tries to find a new target (`ai_find_target`), finds appropriate spells (`ai_try_find_magictech_action`), updates the danger source, or attempts to cast a fleeing spell (`ai_try_cast_flee_spell`).
"sub_4A9B80" = "ai_notify_followers_to_attack" # ai.c: Iterates through the followers of the target object (`a1`). For each follower, it checks if they should also attack the source object (`a2`) based on internal rules and, if appropriate, signals them to join the combat.

# AI Combat Initiation and Hostility Management
"sub_4A9C00" = "ai_follower_join_attack" # ai.c: Function called by a leader to instruct a follower (`source_obj`) to join an attack against a target (`target_obj`). It checks if the follower is upset by attacking the target, handles rotation, and either sets a target lock or initiates following the target.
"sub_4A9E10" = "ai_notify_nearby_npcs_of_attack" # ai.c: Notifies NPCs within a radius (`radius`) determined by the attack's loudness, causing them to potentially wake up, check protection status, or flee if they hear the attack.
"sub_4AA420" = "ai_clear_target_from_combat_fields" # ai.c: Clears a specific object handle (`a2`) from the NPC's combat focus (`OBJ_F_NPC_COMBAT_FOCUS`) and who hit me last (`OBJ_F_NPC_WHO_HIT_ME_LAST`) fields, and removes it from the shitlist.
"sub_4AE450" = "ai_check_avenge_dead_ally" # ai.c: Checks if a critter should seek revenge on the killer of a dead ally. It looks at a dead NPC ally (`a2`) to see if they were in combat focus on another target, and if that target is visible/hearable.
"sub_4AF8C0" = "ai_shitlist_add_party" # ai.c: Adds the target object (`a2`) and all members of its team/party (`v1`) to the source NPC's (`a1`) shitlist (`OBJ_F_NPC_SHIT_LIST_IDX`).

# AI Fleeing and Combat Decision Logic
"sub_4AAF50" = "ai_try_cast_flee_spell" # ai.c: Checks if the critter knows a spell appropriate for the `MAGICTECH_AI_ACTION_FLEE` action list and attempts to cast it. If successful, it sets the `OCF_SPELL_FLEE` flag on the critter.
"sub_4AB030" = "ai_has_fled_far_enough" # ai.c: Checks if the fleeing critter (`a1`) has moved a sufficient distance from the danger source (`a2`) based on internal AI parameters, returning true if the flight is complete.
"sub_4AB2A0" = "ai_decide_fight_or_flee" # ai.c: Core function that decides the NPC's fundamental action towards a target (`a2`) by calling `ai_should_flee`. It sets the danger source to either `AI_DANGER_SOURCE_TYPE_FLEE` or `AI_DANGER_SOURCE_TYPE_COMBAT_FOCUS`.

# AI Magic/Tech Action Selection
"sub_4ABC20" = "ai_try_find_magictech_action" # ai.c: Checks if the object is invulnerable and then calls `ai_select_combat_magictech_action` to attempt to select a spell or item action based on the current combat situation.
"sub_4ABC70" = "ai_select_combat_magictech_action" # ai.c: Main logic for combat magic/tech selection. It determines the chance of casting a spell (`ai_get_magictech_action_chance`), attempts to find summon spells, then defensive spells, and finally offensive spells, calling `ai_evaluate_action_list` for each category.
"sub_4ABE20" = "ai_get_magictech_action_chance" # ai.c: Retrieves the chance percentage that an AI will attempt to use a spell/tech item in combat, based on `AiParams`.
"sub_4ABF10" = "ai_evaluate_action_list" # ai.c: Iterates through the entries in an `AiActionRequest` list (`a2`). It checks spell/item prerequisites and, if successful, sets the `Ai` context (`ai->action_type`, `ai->spell`, `ai->item_obj`) for the selected action.
"S4ABF10" = "AiActionRequest" # ai.c: This identifier refers to the structure passed into `ai_evaluate_action_list`. The structure contains flags, a list of entries, and the count of entries (`cnt`).

# AI Follower Movement and Scheduling
"sub_4AD420" = "ai_is_near_player" # ai.c: Checks if the object is close to the local PC or any network player, often used to determine if the object should run detailed AI processing.
"sub_4AD4D0" = "ai_process_follower_catchup" # ai.c: Checks if a follower is too far away from their leader (PC or NPC). If so, it forces the follower to move to the leader's location, often used when entering a new area or after movement glitches.
"sub_4AD700" = "ai_schedule_process_delay_ms" # ai.c: Schedules a `TIMEEVENT_TYPE_AI` time event for the specified object (`obj`) with a delay measured in milliseconds.
"sub_4AD730" = "ai_schedule_process_at_datetime" # ai.c: Schedules a `TIMEEVENT_TYPE_AI` event for the specified object (`obj`) to fire at an exact `DateTime`.

}  # <-- important: hashtable CLOSED here

# Build a fast combined regex:  \b(?:key1|key2|...)\b
$escapedKeys = $RenameMap.Keys | ForEach-Object { [Regex]::Escape($_) }
$pattern = '\b(?:' + ($escapedKeys -join '|') + ')\b'
$regex = [System.Text.RegularExpressions.Regex]::new($pattern)

# Files
$files = Get-ChildItem -Recurse -Include *.c, *.h -Path $SearchDir -File
$changedCount = 0

foreach ($f in $files) {
    $text = Get-Content -LiteralPath $f.FullName -Raw
    $newText = $regex.Replace($text, { param($m) $RenameMap[$m.Value] })
    if ($newText -ne $text) {
        Write-Host "Updated: $($f.FullName)"
        if (-not $DryRun) {
            Set-Content -LiteralPath $f.FullName -Value $newText -NoNewline -Encoding UTF8
        }
        $changedCount++
    }
}

Write-Host "--------------------------------------------------------"
if ($DryRun) {
    Write-Host "Dry run complete. Files that would change: $changedCount"
} else {
    Write-Host "Batch rename complete. Files changed: $changedCount"
}
Read-Host "Press Enter to exit..."
