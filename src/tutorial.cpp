#include "gamemode.h"
#include "game.h"
#include "output.h"
#include "action.h"
#include "tutorial.h"
#include "overmapbuffer.h"
#include "translations.h"
#include "monstergenerator.h"
#include "profession.h"

std::vector<std::string> tut_text;

bool tutorial_game::init()
{
    // TODO: clean up old tutorial

 calendar::turn = HOURS(12); // Start at noon
 for( auto &elem : tutorials_seen )
     elem = false;
// Set the scent map to 0
 for (int i = 0; i < SEEX * MAPSIZE; i++) {
  for (int j = 0; j < SEEX * MAPSIZE; j++)
   g->scent(i, j) = 0;
 }
 g->temperature = 65;
// We use a Z-factor of 10 so that we don't plop down tutorial rooms in the
// middle of the "real" game world
 g->u.normalize();
 g->u.str_cur = g->u.str_max;
 g->u.per_cur = g->u.per_max;
 g->u.int_cur = g->u.int_max;
 g->u.dex_cur = g->u.dex_max;
 
 for (int i = 0; i < num_hp_parts; i++) {
        g->u.hp_cur[i] = g->u.hp_max[i];
    }
 
 //~ default name for the tutorial
 g->u.name = _("John Smith");
 g->u.prof = profession::generic();
    int lx = 50, ly = 50; // overmap terrain coordinates
    g->cur_om = &overmap_buffer.get(0, 0);
    for (int i = 0; i < OMAPX; i++) {
        for (int j = 0; j < OMAPY; j++) {
            g->cur_om->ter( i, j, -1 ) = "rock";
            // Start with the overmap revealed
            g->cur_om->seen( i, j, 0 ) = true;
        }
    }
    g->cur_om->ter(lx, ly, 0) = "tutorial";
    g->cur_om->ter(lx, ly, -1) = "tutorial";
    g->cur_om->clear_mon_groups();
    // to submap coordinates as it is supposed to be
    g->levx = lx * 2;
    g->levy = ly * 2;

 g->u.toggle_trait("QUICK");
 item lighter("lighter", 0);
 lighter.invlet = 'e';
 g->u.inv.add_item(lighter);
 g->u.skillLevel("gun").level(5);
 g->u.skillLevel("melee").level(5);
 g->m.load(g->levx, g->levy, 0, true, g->cur_om);
 g->levz = 0;
 g->u.posx = 2;
 g->u.posy = 4;

 // This shifts the view to center the players pos
 g->update_map(g->u.posx, g->u.posy);
 return true;
}

void tutorial_game::per_turn()
{
 if (calendar::turn == HOURS(12)) {
  add_message(LESSON_INTRO);
  add_message(LESSON_INTRO);
 } else if (calendar::turn == HOURS(12) + 3)
  add_message(LESSON_INTRO);

 if (g->light_level() == 1) {
  if (g->u.has_amount("flashlight", 1))
   add_message(LESSON_DARK);
  else
   add_message(LESSON_DARK_NO_FLASH);
 }

 if (g->u.pain > 0)
  add_message(LESSON_PAIN);

 if (g->u.recoil >= MIN_RECOIL)
  add_message(LESSON_RECOIL);

 if (!tutorials_seen[LESSON_BUTCHER]) {
  for (size_t i = 0; i < g->m.i_at(g->u.posx, g->u.posy).size(); i++) {
   if (g->m.i_at(g->u.posx, g->u.posy)[i].is_corpse()) {
    add_message(LESSON_BUTCHER);
    i = g->m.i_at(g->u.posx, g->u.posy).size();
   }
  }
 }

 bool showed_message = false;
 for (int x = g->u.posx - 1; x <= g->u.posx + 1 && !showed_message; x++) {
  for (int y = g->u.posy - 1; y <= g->u.posy + 1 && !showed_message; y++) {
   if (g->m.ter(x, y) == t_door_o) {
    add_message(LESSON_OPEN);
    showed_message = true;
   } else if (g->m.ter(x, y) == t_door_c) {
    add_message(LESSON_CLOSE);
    showed_message = true;
   } else if (g->m.ter(x, y) == t_window) {
    add_message(LESSON_SMASH);
    showed_message = true;
   } else if (g->m.furn(x, y) == f_rack && !g->m.i_at(x, y).empty()) {
    add_message(LESSON_EXAMINE);
    showed_message = true;
   } else if (g->m.ter(x, y) == t_stairs_down) {
    add_message(LESSON_STAIRS);
    showed_message = true;
   } else if (g->m.ter(x, y) == t_water_sh) {
    add_message(LESSON_PICKUP_WATER);
    showed_message = true;
   }
  }
 }

 if (!g->m.i_at(g->u.posx, g->u.posy).empty())
  add_message(LESSON_PICKUP);
}

void tutorial_game::pre_action( action_id &act )
{
    switch( act ) {
        case ACTION_SAVE:
        case ACTION_QUICKSAVE:
            popup( _( "You're saving a tutorial - the tutorial world lacks certain features of normal worlds. "
                      "Weird things might happen when you load this save. You have been warned." ) );
            break;
        default:
            // Other actions are fine.
            break;
    }
}

void tutorial_game::post_action(action_id act)
{
 switch (act) {
 case ACTION_RELOAD:
  if (g->u.weapon.is_gun() && !tutorials_seen[LESSON_GUN_FIRE]) {
   monster tmp(GetMType("mon_zombie"), g->u.posx, g->u.posy - 6);
   g->add_zombie(tmp);
   tmp.spawn(g->u.posx + 2, g->u.posy - 5);
   g->add_zombie(tmp);
   tmp.spawn(g->u.posx - 2, g->u.posy - 5);
   g->add_zombie(tmp);
   add_message(LESSON_GUN_FIRE);
  }
  break;

 case ACTION_OPEN:
  add_message(LESSON_CLOSE);
  break;

 case ACTION_CLOSE:
  add_message(LESSON_SMASH);
  break;

 case ACTION_USE:
  if (g->u.has_amount("grenade_act", 1))
   add_message(LESSON_ACT_GRENADE);
  for (int x = g->u.posx - 1; x <= g->u.posx + 1; x++) {
   for (int y = g->u.posy - 1; y <= g->u.posy + 1; y++) {
    if (g->m.tr_at(x, y) == tr_bubblewrap)
     add_message(LESSON_ACT_BUBBLEWRAP);
   }
  }
  break;

 case ACTION_EAT:
  if (g->u.last_item == "codeine")
   add_message(LESSON_TOOK_PAINKILLER);
  else if (g->u.last_item == "cig")
   add_message(LESSON_TOOK_CIG);
  else if (g->u.last_item == "water")
   add_message(LESSON_DRANK_WATER);
  break;

 case ACTION_WEAR: {
  item it( g->u.last_item, 0 );
  if (it.is_armor()) {
   if (it.get_coverage() >= 2 || it.get_thickness() >= 2)
    add_message(LESSON_WORE_ARMOR);
   if (it.get_storage() >= 20)
    add_message(LESSON_WORE_STORAGE);
   if (it.get_env_resist() >= 2)
    add_message(LESSON_WORE_MASK);
  }
 } break;

 case ACTION_WIELD:
  if (g->u.weapon.is_gun())
   add_message(LESSON_GUN_LOAD);
  break;

 case ACTION_EXAMINE:
  add_message(LESSON_INTERACT);
// Fall through to...
 case ACTION_PICKUP: {
  item it( g->u.last_item, 0 );
  if (it.is_armor())
   add_message(LESSON_GOT_ARMOR);
  else if (it.is_gun())
   add_message(LESSON_GOT_GUN);
  else if (it.is_ammo())
   add_message(LESSON_GOT_AMMO);
  else if (it.is_tool())
   add_message(LESSON_GOT_TOOL);
  else if (it.is_food())
   add_message(LESSON_GOT_FOOD);
  else if (it.is_weap())
   add_message(LESSON_GOT_WEAPON);

  if (g->u.volume_carried() > g->u.volume_capacity() - 2)
   add_message(LESSON_OVERLOADED);
 } break;

 default: //TODO: add more actions here
  break;

 }
}

void tutorial_game::add_message(tut_lesson lesson)
{
// Cycle through intro lessons
 if (lesson == LESSON_INTRO) {
  while (lesson != NUM_LESSONS && tutorials_seen[lesson]) {
   switch (lesson) {
    case LESSON_INTRO: lesson = LESSON_MOVE; break;
    case LESSON_MOVE:  lesson = LESSON_LOOK; break;
    default:  lesson = NUM_LESSONS; break;
   }
  }
  if (lesson == NUM_LESSONS)
   return;
 }
 if (tutorials_seen[lesson])
  return;
 tutorials_seen[lesson] = true;
 popup(tut_text[lesson], PF_ON_TOP);
 g->refresh_all();
}

void load_tutorial_messages(JsonObject &jo)
{
    // loading them all at once, as they have to be in exact order
    tut_text.clear();
    JsonArray messages = jo.get_array("messages");
    while (messages.has_more()) {
        tut_text.push_back(_(messages.next_string().c_str()));
    }
}

void clear_tutorial_messages()
{
    tut_text.clear();
}
