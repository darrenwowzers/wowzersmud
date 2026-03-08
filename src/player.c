/****************************************************************************
 * [S]imulated [M]edieval [A]dventure multi[U]ser [G]ame      |   \\._.//   *
 * -----------------------------------------------------------|   (0...0)   *
 * SMAUG 1.8 (C) 1994, 1995, 1996, 1998  by Derek Snider      |    ).:.(    *
 * -----------------------------------------------------------|    {o o}    *
 * SMAUG code team: Thoric, Altrag, Blodkai, Narn, Haus,      |   / ' ' \   *
 * Scryn, Rennard, Swordbearer, Gorog, Grishnakh, Nivek,      |~'~.VxvxV.~'~*
 * Tricops, Fireblade, Edmond, Conran                         |             *
 * ------------------------------------------------------------------------ *
 * Merc 2.1 Diku Mud improvments copyright (C) 1992, 1993 by Michael        *
 * Chastain, Michael Quan, and Mitchell Tse.                                *
 * Original Diku Mud copyright (C) 1990, 1991 by Sebastian Hammer,          *
 * Michael Seifert, Hans Henrik St{rfeldt, Tom Madsen, and Katja Nyboe.     *
 * ------------------------------------------------------------------------ *
 *             Commands for personal player settings/statictics             *
 ****************************************************************************/

#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <time.h>
#include "mud.h"

/*
 *  Locals
 */
const char *tiny_affect_loc_name( int location );

void do_gold( CHAR_DATA* ch, const char* argument )
{
   set_char_color( AT_GOLD, ch );
   ch_printf( ch, "You have %s gold pieces.\r\n", num_punct( ch->gold ) );
}

void do_worth( CHAR_DATA* ch, const char* argument )
{
   char buf[MAX_STRING_LENGTH];
   char buf2[MAX_STRING_LENGTH];

   if( IS_NPC( ch ) )
      return;

   set_pager_color( AT_SCORE, ch );
   pager_printf( ch, "\r\nWorth for %s%s.\r\n", ch->name, ch->pcdata->title );
   send_to_pager( " ----------------------------------------------------------------------------\r\n", ch );
   if( !ch->pcdata->deity )
      snprintf( buf, MAX_STRING_LENGTH, "%s", "N/A" );
   else if( ch->pcdata->favor > 2250 )
      snprintf( buf, MAX_STRING_LENGTH, "%s", "loved" );
   else if( ch->pcdata->favor > 2000 )
      snprintf( buf, MAX_STRING_LENGTH, "%s", "cherished" );
   else if( ch->pcdata->favor > 1750 )
      snprintf( buf, MAX_STRING_LENGTH, "%s", "honored" );
   else if( ch->pcdata->favor > 1500 )
      snprintf( buf, MAX_STRING_LENGTH, "%s", "praised" );
   else if( ch->pcdata->favor > 1250 )
      snprintf( buf, MAX_STRING_LENGTH, "%s", "favored" );
   else if( ch->pcdata->favor > 1000 )
      snprintf( buf, MAX_STRING_LENGTH, "%s", "respected" );
   else if( ch->pcdata->favor > 750 )
      snprintf( buf, MAX_STRING_LENGTH, "%s", "liked" );
   else if( ch->pcdata->favor > 250 )
      snprintf( buf, MAX_STRING_LENGTH, "%s", "tolerated" );
   else if( ch->pcdata->favor > -250 )
      snprintf( buf, MAX_STRING_LENGTH, "%s", "ignored" );
   else if( ch->pcdata->favor > -750 )
      snprintf( buf, MAX_STRING_LENGTH, "%s", "shunned" );
   else if( ch->pcdata->favor > -1000 )
      snprintf( buf, MAX_STRING_LENGTH, "%s", "disliked" );
   else if( ch->pcdata->favor > -1250 )
      snprintf( buf, MAX_STRING_LENGTH, "%s", "dishonored" );
   else if( ch->pcdata->favor > -1500 )
      snprintf( buf, MAX_STRING_LENGTH, "%s", "disowned" );
   else if( ch->pcdata->favor > -1750 )
      snprintf( buf, MAX_STRING_LENGTH, "%s", "abandoned" );
   else if( ch->pcdata->favor > -2000 )
      snprintf( buf, MAX_STRING_LENGTH, "%s", "despised" );
   else if( ch->pcdata->favor > -2250 )
      snprintf( buf, MAX_STRING_LENGTH, "%s", "hated" );
   else
      snprintf( buf, MAX_STRING_LENGTH, "%s", "damned" );

   if( ch->level < 10 )
   {
      if( ch->alignment > 900 )
         snprintf( buf2, MAX_STRING_LENGTH, "%s", "devout" );
      else if( ch->alignment > 700 )
         snprintf( buf2, MAX_STRING_LENGTH, "%s", "noble" );
      else if( ch->alignment > 350 )
         snprintf( buf2, MAX_STRING_LENGTH, "%s", "honorable" );
      else if( ch->alignment > 100 )
         snprintf( buf2, MAX_STRING_LENGTH, "%s", "worthy" );
      else if( ch->alignment > -100 )
         snprintf( buf2, MAX_STRING_LENGTH, "%s", "neutral" );
      else if( ch->alignment > -350 )
         snprintf( buf2, MAX_STRING_LENGTH, "%s", "base" );
      else if( ch->alignment > -700 )
         snprintf( buf2, MAX_STRING_LENGTH, "%s", "evil" );
      else if( ch->alignment > -900 )
         snprintf( buf2, MAX_STRING_LENGTH, "%s", "ignoble" );
      else
         snprintf( buf2, MAX_STRING_LENGTH, "%s", "fiendish" );
   }
   else
      snprintf( buf2, MAX_STRING_LENGTH, "%d", ch->alignment );
   pager_printf( ch, "|Level: %-4d |Favor: %-10s |Alignment: %-9s |Experience: %-9d|\r\n", ch->level, buf, buf2, ch->exp );
   send_to_pager( " ----------------------------------------------------------------------------\r\n", ch );
   switch ( ch->style )
   {
      case STYLE_EVASIVE:
         snprintf( buf, MAX_STRING_LENGTH, "%s", "evasive" );
         break;
      case STYLE_DEFENSIVE:
         snprintf( buf, MAX_STRING_LENGTH, "%s", "defensive" );
         break;
      case STYLE_AGGRESSIVE:
         snprintf( buf, MAX_STRING_LENGTH, "%s", "aggressive" );
         break;
      case STYLE_BERSERK:
         snprintf( buf, MAX_STRING_LENGTH, "%s", "berserk" );
         break;
      default:
         snprintf( buf, MAX_STRING_LENGTH, "%s", "standard" );
         break;
   }
   pager_printf( ch, "|Glory: %-4d |Weight: %-9d |Style: %-13s |Gold: %-14s |\r\n",
                 ch->pcdata->quest_curr, ch->carry_weight, buf, num_punct( ch->gold ) );
   send_to_pager( " ----------------------------------------------------------------------------\r\n", ch );
/* Wowzers Mud: Advanced Currencies injected into empty spaces -Hansth */
   if( ch->level < 15 && !IS_PKILL( ch ) )
      pager_printf( ch, "|Honor: %-5d |Hitroll: -------- |Damroll: ----------- |Conquest: %-10d |\r\n",
                    ch->pcdata->honor_points, ch->pcdata->conquest_points );
   else
      pager_printf( ch, "|Honor: %-5d |Hitroll: %-8d |Damroll: %-11d |Conquest: %-10d |\r\n", 
                    ch->pcdata->honor_points, GET_HITROLL( ch ), GET_DAMROLL( ch ), ch->pcdata->conquest_points );
   send_to_pager( " ----------------------------------------------------------------------------\r\n", ch );
}

/* ============================================
   Wowzers Mud: BORDERED SCORE DASHBOARD -Hansth
   ============================================ */
void do_score( CHAR_DATA *ch, const char *argument )
{
    if ( IS_NPC(ch) )
    {
       send_to_char("Mobiles do not have a detailed score sheet.\r\n", ch);
       return;
    }

    set_pager_color( AT_SCORE, ch );
    
    /* Header Border - Manually padded to 74 visible chars -Hansth */
    pager_printf( ch, "\r\n&z+--------------------------------------------------------------------------+\r\n" );
    pager_printf( ch, "&z|  &WCharacter Information: &Y%-48s &z|\r\n", ch->name );
    pager_printf( ch, "&z+--------------------------------------------------------------------------+\r\n" );
    
    /* Line 1: Basic Info -Hansth */
    const char *faction_name = (ch->faction == FACTION_ALLIANCE) ? "&BAlliance " : (ch->faction == FACTION_HORDE) ? "&RHorde    " : "&wNeutral  ";
    pager_printf( ch, "&z| &wLevel: &W%-3d  &wRace: &W%-10.10s  &wClass: &W%-10.10s  &wFaction: %s &z|\r\n",
        ch->level, race_table[ch->race]->race_name, class_table[ch->Class]->who_name, faction_name );
    
    pager_printf( ch, "&z+-----------------------------------+--------------------------------------+\r\n" );

    /* STATS COLUMNS - Manual spacing for alignment -Hansth */
    pager_printf( ch, "&z| &G[ Primary Stats ]                 &z| &G[ Vitality ]                       &z|\r\n" );
    
    pager_printf( ch, "&z| &wStrength:  &W%4d                  &z| &wHealth: &G%6d &w/ &G%-6d          &z|\r\n", 
        get_curr_str(ch), ch->hit, ch->max_hit );

    /* Dynamic Resource Logic -Hansth */
    const char *resource_name = "Mana  ";
    if ( ch->power_type == POWER_ENERGY )      resource_name = "Energy";
    else if ( ch->power_type == POWER_RAGE )   resource_name = "Rage  ";

    pager_printf( ch, "&z| &wAgility:   &W%4d                  &z| &w%s: &C%6d &w/ &C%-6d          &z|\r\n", 
        get_curr_agi(ch), resource_name, ch->mana, ch->max_mana );
        
    pager_printf( ch, "&z| &wStamina:   &W%4d                  &z| &wExperience: &Y%-10d             &z|\r\n", 
        get_curr_sta(ch), ch->exp );
        
    pager_printf( ch, "&z| &wIntellect: &W%4d                  &z| &wGold:       &Y%-10s             &z|\r\n", 
        get_curr_int(ch), num_punct(ch->gold) );
        
    pager_printf( ch, "&z| &wSpirit:    &W%4d                  &z| &wPractice:   &W%-10d             &z|\r\n", 
        get_curr_spi(ch), ch->practice );

    pager_printf( ch, "&z+-----------------------------------+--------------------------------------+\r\n" );

    /* SECONDARY STATS -Hansth */
    pager_printf( ch, "&z| &C[ Combat Statistics ]                                                  &z|\r\n" );
    
    int ap = get_attack_power(ch);
    int crit = 5 + (get_curr_agi(ch) * 5 / 100); 
    int dodge = 5 + (get_curr_agi(ch) * 2 / 100);

    pager_printf( ch, "&z| &wAttack Power: &W%-6d              &wCrit Chance:  &G%d.00%%               &z|\r\n", ap, crit );
    pager_printf( ch, "&z| &wDodge Chance: &G%2d.00%%              &wParry Chance: &G 5.00%%               &z|\r\n", dodge );

    /* Combo Points Check -Hansth */
    if ( ch->Class == CLASS_ROGUE && ch->combo_points > 0 )
    {
        pager_printf( ch, "&z| &wCombo Points: &R%d &w(Target: &W%-20.20s&w)                   &z|\r\n", 
            ch->combo_points, IS_NPC(ch->combo_target) ? ch->combo_target->short_descr : ch->combo_target->name );
    }

    pager_printf( ch, "&z+--------------------------------------------------------------------------+\r\n" );

    /* FOOTER/CONFIGS - Fixed spacing -Hansth */
    pager_printf( ch, "&z| &wConfig: %s %s %s %s %s %s  &z|\r\n",
        xIS_SET(ch->act, PLR_AUTOLOOT) ? "&G+Loot" : "&r-Loot",
        xIS_SET(ch->act, PLR_AUTOGOLD) ? "&G+Gold" : "&r-Gold",
        xIS_SET(ch->act, PLR_AUTOSAC)  ? "&G+Sac " : "&r-Sac ",
        xIS_SET(ch->act, PLR_AUTOEXIT) ? "&G+Exit" : "&r-Exit",
        xIS_SET(ch->act, PLR_ANSI)     ? "&G+Ansi" : "&r-Ansi",
        xIS_SET(ch->act, PLR_COMPASS)  ? "&G+Comp" : "&r-Comp" );

    pager_printf( ch, "&z+--------------------------------------------------------------------------+\r\n" );
}

/*								-Thoric
 * Display your current exp, level, and surrounding level exp requirements
 */
void do_level( CHAR_DATA* ch, const char* argument )
{
   char buf[MAX_STRING_LENGTH];
   char buf2[MAX_STRING_LENGTH];
   int x, lowlvl, hilvl;

   if( ch->level == 1 )
      lowlvl = 1;
   else
      lowlvl = UMAX( 2, ch->level - 5 );
   hilvl = URANGE( ch->level, ch->level + 5, MAX_LEVEL );
   set_char_color( AT_SCORE, ch );
   ch_printf( ch, "\r\nExperience required, levels %d to %d:\r\n______________________________________________\r\n\r\n",
              lowlvl, hilvl );
   snprintf( buf, MAX_STRING_LENGTH, " exp  (Current: %12s)", num_punct( ch->exp ) );
   snprintf( buf2, MAX_STRING_LENGTH, " exp  (Needed:  %12s)", num_punct( exp_level( ch, ch->level + 1 ) - ch->exp ) );
   for( x = lowlvl; x <= hilvl; x++ )
      ch_printf( ch, " (%2d) %12s%s\r\n", x, num_punct( exp_level( ch, x ) ),
                 ( x == ch->level ) ? buf : ( x == ch->level + 1 ) ? buf2 : " exp" );
   send_to_char( "______________________________________________\r\n", ch );
}

/* 1997, Blodkai */
void do_remains( CHAR_DATA* ch, const char* argument )
{
   char buf[MAX_STRING_LENGTH];
   OBJ_DATA *obj;
   bool found = FALSE;

   if( IS_NPC( ch ) )
      return;

   set_char_color( AT_MAGIC, ch );
   if( !ch->pcdata->deity )
   {
      send_to_pager( "You have no deity from which to seek such assistance...\r\n", ch );
      return;
   }

   if( ch->pcdata->favor < ch->level * 2 )
   {
      send_to_pager( "Your favor is insufficient for such assistance...\r\n", ch );
      return;
   }

   pager_printf( ch, "%s appears in a vision, revealing that your remains... ", ch->pcdata->deity->name );
   snprintf( buf, MAX_STRING_LENGTH, "the corpse of %s", ch->name );
   for( obj = first_object; obj; obj = obj->next )
   {
      if( obj->in_room && !str_cmp( buf, obj->short_descr ) && ( obj->pIndexData->vnum == OBJ_VNUM_CORPSE_PC ) )
      {
         found = TRUE;
         pager_printf( ch, "\r\n  - at %s will endure for %d ticks", obj->in_room->name, obj->timer );
      }
   }
   if( !found )
      send_to_pager( " no longer exist.\r\n", ch );
   else
   {
      send_to_pager( "\r\n", ch );
      ch->pcdata->favor -= ch->level * 2;
   }
}

/* Affects-at-a-glance, Blodkai */
void do_affected( CHAR_DATA* ch, const char* argument )
{
   char arg[MAX_INPUT_LENGTH];
   AFFECT_DATA *paf;
   SKILLTYPE *skill;

   if( IS_NPC( ch ) )
      return;

   set_char_color( AT_SCORE, ch );

   argument = one_argument( argument, arg );
   if( !str_cmp( arg, "by" ) )
   {
      send_to_char_color( "\r\n&BImbued with:\r\n", ch );
      ch_printf_color( ch, "&C%s\r\n", !xIS_EMPTY( ch->affected_by ) ? affect_bit_name( &ch->affected_by ) : "nothing" );
      if( ch->level >= 20 )
      {
         send_to_char( "\r\n", ch );
         if( ch->resistant > 0 )
         {
            send_to_char_color( "&BResistances:  ", ch );
            ch_printf_color( ch, "&C%s\r\n", flag_string( ch->resistant, ris_flags ) );
         }
         if( ch->immune > 0 )
         {
            send_to_char_color( "&BImmunities:   ", ch );
            ch_printf_color( ch, "&C%s\r\n", flag_string( ch->immune, ris_flags ) );
         }
         if( ch->susceptible > 0 )
         {
            send_to_char_color( "&BSuscepts:     ", ch );
            ch_printf_color( ch, "&C%s\r\n", flag_string( ch->susceptible, ris_flags ) );
         }
      }
      return;
   }

   if( !ch->first_affect )
   {
      send_to_char_color( "\r\n&CNo cantrip or skill affects you.\r\n", ch );
   }
   else
   {
      send_to_char( "\r\n", ch );
      for( paf = ch->first_affect; paf; paf = paf->next )
         if( ( skill = get_skilltype( paf->type ) ) != NULL )
         {
            set_char_color( AT_BLUE, ch );
            send_to_char( "Affected:  ", ch );
            set_char_color( AT_SCORE, ch );
            if( ch->level >= 20 || IS_PKILL( ch ) )
            {
               if( paf->duration < 25 )
                  set_char_color( AT_WHITE, ch );
               if( paf->duration < 6 )
                  set_char_color( AT_WHITE + AT_BLINK, ch );
               ch_printf( ch, "(%5d)   ", paf->duration );
            }
            ch_printf( ch, "%-18s\r\n", skill->name );
         }
   }
}

void do_inventory( CHAR_DATA* ch, const char* argument )
{
   CHAR_DATA *victim;

   if( !argument || argument[0] == '\0' || !IS_IMMORTAL( ch ) )
      victim = ch;
   else
   {
      if( !( victim = get_char_world( ch, argument ) ) )
      {
         ch_printf( ch, "There is nobody named %s online.\r\n", argument );
         return;
      }
   }

   if( victim != ch )
      ch_printf( ch, "&R%s is carrying:\r\n", IS_NPC( victim ) ? victim->short_descr : victim->name );
   else
      send_to_char( "&RYou are carrying:\r\n", ch );

   show_list_to_char( victim->first_carrying, ch, TRUE, TRUE );
}

void do_equipment( CHAR_DATA* ch, const char* argument )
{
   CHAR_DATA *victim = ch;
   OBJ_DATA *obj;
   int iWear;
   bool found;

   if( argument[0] == '\0' || get_trust( ch ) <= LEVEL_GOD )
      victim = ch;
   else
   {
      if( ( victim = get_char_world( ch, argument ) ) == NULL )
      {
         send_to_char( "They're not here.\r\n", ch );
         return;
      }
   }

   if( victim != ch )
      ch_printf( ch, "&R%s is using:\r\n", IS_NPC( victim ) ? victim->short_descr : victim->name );
   else
      send_to_char( "&RYou are using:\r\n", ch );

   found = FALSE;
   set_char_color( AT_OBJECT, ch );
   for( iWear = 0; iWear < MAX_WEAR; iWear++ )
   {
      for( obj = victim->first_carrying; obj; obj = obj->next_content )
      {
         if( obj->wear_loc == iWear )
         {
            if( ( !IS_NPC( victim ) ) && ( victim->race > 0 ) && ( victim->race < MAX_PC_RACE ) )
               send_to_char( race_table[victim->race]->where_name[iWear], ch );
            else
               send_to_char( where_name[iWear], ch );

            if( can_see_obj( ch, obj ) )
            {
               send_to_char( format_obj_to_char( obj, ch, TRUE ), ch );
               send_to_char( "\r\n", ch );
            }
            else
               send_to_char( "something.\r\n", ch );
            found = TRUE;
         }
      }
   }

   if( !found )
      send_to_char( "Nothing.\r\n", ch );
/* ============================================
      Wowzers Mud: Display Active Item Sets -Hansth
      ============================================ */
   {
      int defias_count     = count_set_pieces( ch, ITEM_SET_DEFIAS_LEATHER );
      int devout_count     = count_set_pieces( ch, ITEM_SET_DEVOUT );
      int lightforge_count = count_set_pieces( ch, ITEM_SET_LIGHTFORGE );
      int gladiator_count  = count_set_pieces( ch, ITEM_SET_GLADIATOR );

      if ( defias_count > 0 || devout_count > 0 || lightforge_count > 0 || gladiator_count > 0 )
      {
         send_to_char( "\r\n&Y--- Active Item Sets ---&w\r\n", ch );
         
         if ( defias_count > 0 )
            ch_printf( ch, "&O Defias Leather&w (%d/5 pieces)\r\n", defias_count );
         if ( devout_count > 0 )
            ch_printf( ch, "&O Westfall Devout&w (%d/5 pieces)\r\n", devout_count );
         if ( lightforge_count > 0 )
            ch_printf( ch, "&O Lightforge Armor&w (%d/5 pieces)\r\n", lightforge_count );
         if ( gladiator_count > 0 )
            ch_printf( ch, "&O Gladiator's Pursuit&w (%d/5 pieces)\r\n", gladiator_count );
      }
   }
}

void set_title( CHAR_DATA * ch, const char *title )
{
   char buf[MAX_STRING_LENGTH];

   if( IS_NPC( ch ) )
   {
      bug( "%s: NPC.", __func__ );
      return;
   }

   if( isalpha( title[0] ) || isdigit( title[0] ) )
   {
      buf[0] = ' ';
      strlcpy( buf + 1, title, MAX_STRING_LENGTH - 1 );
   }
   else
      strlcpy( buf, title, MAX_STRING_LENGTH );

   STRFREE( ch->pcdata->title );
   ch->pcdata->title = STRALLOC( buf );
}

void do_title( CHAR_DATA* ch, const char* argument )
{
   if( IS_NPC( ch ) )
      return;

   set_char_color( AT_SCORE, ch );

   if( IS_SET( ch->pcdata->flags, PCFLAG_NOTITLE ) )
   {
      set_char_color( AT_IMMORT, ch );
      send_to_char( "The Gods prohibit you from changing your title.\r\n", ch );
      return;
   }

   if( argument[0] == '\0' )
   {
      send_to_char( "Change your title to what?\r\n", ch );
      return;
   }

   char title[50];
   strlcpy(title, argument, 50);

   smash_tilde( title );
   set_title( ch, title );
   send_to_char( "Ok.\r\n", ch );
}

void do_homepage( CHAR_DATA* ch, const char* argument )
{
   char buf[MAX_STRING_LENGTH];

   if( IS_NPC( ch ) )
      return;

   if( IS_SET( ch->pcdata->flags, PCFLAG_NOHOMEPAGE ) )
   {
      send_to_char( "The Gods prohibit you from changing your homepage.\r\n", ch );
      return;
   }

   if( argument[0] == '\0' )
   {
      if( !ch->pcdata->homepage )
         ch->pcdata->homepage = strdup( "" );
      ch_printf( ch, "Your homepage is: %s\r\n", show_tilde( ch->pcdata->homepage ) );
      return;
   }

   if( !str_cmp( argument, "clear" ) )
   {
      if( ch->pcdata->homepage )
         DISPOSE( ch->pcdata->homepage );
      ch->pcdata->homepage = strdup( "" );
      send_to_char( "Homepage cleared.\r\n", ch );
      return;
   }

   if( strstr( argument, "://" ) )
      strlcpy( buf, argument, MAX_STRING_LENGTH );
   else
      snprintf( buf, MAX_STRING_LENGTH, "http://%s", argument );
   if( strlen( buf ) > 70 )
      buf[70] = '\0';

   hide_tilde( buf );
   if( ch->pcdata->homepage )
      DISPOSE( ch->pcdata->homepage );
   ch->pcdata->homepage = strdup( buf );
   send_to_char( "Homepage set.\r\n", ch );
}

/*
 * Set your personal description				-Thoric
 */
void do_description( CHAR_DATA* ch, const char* argument )
{
   if( IS_NPC( ch ) )
   {
      send_to_char( "Monsters are too dumb to do that!\r\n", ch );
      return;
   }

   if( IS_SET( ch->pcdata->flags, PCFLAG_NODESC ) )
   {
      send_to_char( "You cannot set your description.\r\n", ch );
      return;
   }

   if( !ch->desc )
   {
      bug( "%s: no descriptor", __func__ );
      return;
   }

   switch ( ch->substate )
   {
      default:
         bug( "%s: illegal substate", __func__ );
         return;

      case SUB_RESTRICTED:
         send_to_char( "You cannot use this command from within another command.\r\n", ch );
         return;

      case SUB_NONE:
         ch->substate = SUB_PERSONAL_DESC;
         ch->dest_buf = ch;
         start_editing( ch, ch->description );
         return;

      case SUB_PERSONAL_DESC:
         STRFREE( ch->description );
         ch->description = copy_buffer( ch );
         stop_editing( ch );
         return;
   }
}

/* Ripped off do_description for whois bio's -- Scryn*/
void do_bio( CHAR_DATA* ch, const char* argument )
{
   if( IS_NPC( ch ) )
   {
      send_to_char( "Mobs cannot set a bio.\r\n", ch );
      return;
   }

   if( IS_SET( ch->pcdata->flags, PCFLAG_NOBIO ) )
   {
      set_char_color( AT_RED, ch );
      send_to_char( "The gods won't allow you to do that!\r\n", ch );
      return;
   }

   if( !ch->desc )
   {
      bug( "%s: no descriptor", __func__ );
      return;
   }

   switch ( ch->substate )
   {
      default:
         bug( "%s: illegal substate", __func__ );
         return;

      case SUB_RESTRICTED:
         send_to_char( "You cannot use this command from within another command.\r\n", ch );
         return;

      case SUB_NONE:
         ch->substate = SUB_PERSONAL_BIO;
         ch->dest_buf = ch;
         start_editing( ch, ch->pcdata->bio );
         return;

      case SUB_PERSONAL_BIO:
         STRFREE( ch->pcdata->bio );
         ch->pcdata->bio = copy_buffer( ch );
         stop_editing( ch );
         return;
   }
}

/*
 * New stat and statreport command coded by Morphina
 * Bug fixes by Shaddai
 */
void do_statreport( CHAR_DATA* ch, const char* argument )
{
   char buf[MAX_STRING_LENGTH];

   if( IS_NPC( ch ) )
   {
      send_to_char( "Huh?\r\n", ch );
      return;
   }

   if( IS_VAMPIRE( ch ) )
   {
      ch_printf( ch, "You report: %d/%d hp %d/%d blood %d/%d mv %d xp.\r\n",
                 ch->hit, ch->max_hit, ch->pcdata->condition[COND_BLOODTHIRST],
                 10 + ch->level, ch->move, ch->max_move, ch->exp );
      snprintf( buf, MAX_STRING_LENGTH, "$n reports: %d/%d hp %d/%d blood %d/%d mv %d xp.",
                ch->hit, ch->max_hit, ch->pcdata->condition[COND_BLOODTHIRST],
                10 + ch->level, ch->move, ch->max_move, ch->exp );
      act( AT_REPORT, buf, ch, NULL, NULL, TO_ROOM );
   }
   else
   {
      ch_printf( ch, "You report: %d/%d hp %d/%d mana %d/%d mv %d xp.\r\n",
                 ch->hit, ch->max_hit, ch->mana, ch->max_mana, ch->move, ch->max_move, ch->exp );
      snprintf( buf, MAX_STRING_LENGTH, "$n reports: %d/%d hp %d/%d mana %d/%d mv %d xp.",
                ch->hit, ch->max_hit, ch->mana, ch->max_mana, ch->move, ch->max_move, ch->exp );
      act( AT_REPORT, buf, ch, NULL, NULL, TO_ROOM );
   }

   ch_printf( ch, "Your base stats:    %-2d str %-2d wis %-2d int %-2d dex %-2d con %-2d cha %-2d lck.\r\n",
              ch->perm_str, ch->perm_wis, ch->perm_int, ch->perm_dex, ch->perm_con, ch->perm_cha, ch->perm_lck );
   snprintf( buf, MAX_STRING_LENGTH, "$n's base stats:    %-2d str %-2d wis %-2d int %-2d dex %-2d con %-2d cha %-2d lck.",
             ch->perm_str, ch->perm_wis, ch->perm_int, ch->perm_dex, ch->perm_con, ch->perm_cha, ch->perm_lck );
   act( AT_REPORT, buf, ch, NULL, NULL, TO_ROOM );

   ch_printf( ch, "Your current stats: %-2d str %-2d spi %-2d int %-2d agi %-2d sta.\r\n",
              get_curr_str( ch ), get_curr_spi( ch ), get_curr_int( ch ),
              get_curr_agi( ch ), get_curr_sta( ch ));
   snprintf( buf, MAX_STRING_LENGTH, "$n's current stats: %-2d str %-2d spi %-2d int %-2d agi %-2d sta.",
             get_curr_str( ch ), get_curr_spi( ch ), get_curr_int( ch ),
             get_curr_agi( ch ), get_curr_sta( ch ));
   act( AT_REPORT, buf, ch, NULL, NULL, TO_ROOM );
}

void do_stat( CHAR_DATA* ch, const char* argument )
{
   if( IS_NPC( ch ) )
   {
      send_to_char( "Huh?\r\n", ch );
      return;
   }

   if( IS_VAMPIRE( ch ) )
      ch_printf( ch, "You report: %d/%d hp %d/%d blood %d/%d mv %d xp.\r\n",
                 ch->hit, ch->max_hit, ch->pcdata->condition[COND_BLOODTHIRST],
                 10 + ch->level, ch->move, ch->max_move, ch->exp );
   else
      ch_printf( ch, "You report: %d/%d hp %d/%d mana %d/%d mv %d xp.\r\n",
                 ch->hit, ch->max_hit, ch->mana, ch->max_mana, ch->move, ch->max_move, ch->exp );

   ch_printf( ch, "Your base stats:    %-2d str %-2d wis %-2d int %-2d dex %-2d con %-2d cha %-2d lck.\r\n",
              ch->perm_str, ch->perm_wis, ch->perm_int, ch->perm_dex, ch->perm_con, ch->perm_cha, ch->perm_lck );

   ch_printf( ch, "Your current stats: %-2d str %-2d spi %-2d int %-2d agi %-2d sta.\r\n",
              get_curr_str( ch ), get_curr_spi( ch ), get_curr_int( ch ),
              get_curr_agi( ch ), get_curr_sta( ch ));
}

void do_report( CHAR_DATA* ch, const char* argument )
{
   char buf[MAX_INPUT_LENGTH];

   if( IS_NPC( ch ) && ch->fighting )
      return;

   if( IS_AFFECTED( ch, AFF_POSSESS ) )
   {
      send_to_char( "You can't do that in your current state of mind!\r\n", ch );
      return;
   }

   if( IS_VAMPIRE( ch ) )
      ch_printf( ch,
                 "You report: %d/%d hp %d/%d blood %d/%d mv %d xp.\r\n",
                 ch->hit, ch->max_hit,
                 ch->pcdata->condition[COND_BLOODTHIRST], 10 + ch->level, ch->move, ch->max_move, ch->exp );
   else
      ch_printf( ch,
                 "You report: %d/%d hp %d/%d mana %d/%d mv %d xp.\r\n",
                 ch->hit, ch->max_hit, ch->mana, ch->max_mana, ch->move, ch->max_move, ch->exp );

   if( IS_VAMPIRE( ch ) )
      snprintf( buf, MAX_INPUT_LENGTH, "$n reports: %d/%d hp %d/%d blood %d/%d mv %d xp.\r\n",
                ch->hit, ch->max_hit,
                ch->pcdata->condition[COND_BLOODTHIRST], 10 + ch->level, ch->move, ch->max_move, ch->exp );
   else
      snprintf( buf, MAX_INPUT_LENGTH, "$n reports: %d/%d hp %d/%d mana %d/%d mv %d xp.",
                ch->hit, ch->max_hit, ch->mana, ch->max_mana, ch->move, ch->max_move, ch->exp );

   act( AT_REPORT, buf, ch, NULL, NULL, TO_ROOM );
}

void do_fprompt( CHAR_DATA* ch, const char* argument )
{
   char arg[MAX_INPUT_LENGTH];

   set_char_color( AT_GREY, ch );

   if( IS_NPC( ch ) )
   {
      send_to_char( "NPC's can't change their prompt..\r\n", ch );
      return;
   }

   smash_tilde( argument );
   one_argument( argument, arg );
   if( !*arg || !str_cmp( arg, "display" ) )
   {
      send_to_char( "Your current fighting prompt string:\r\n", ch );
      set_char_color( AT_WHITE, ch );
      ch_printf( ch, "%s\r\n", !str_cmp( ch->pcdata->fprompt, "" ) ? "(default prompt)" : ch->pcdata->fprompt );
      set_char_color( AT_GREY, ch );
      send_to_char( "Type 'help prompt' for information on changing your prompt.\r\n", ch );
      return;
   }

   send_to_char( "Replacing old prompt of:\r\n", ch );
   set_char_color( AT_WHITE, ch );
   ch_printf( ch, "%s\r\n", !str_cmp( ch->pcdata->fprompt, "" ) ? "(default prompt)" : ch->pcdata->fprompt );
   if( ch->pcdata->fprompt )
      STRFREE( ch->pcdata->fprompt );

   char prompt[128];
   strlcpy(prompt, argument, 128);

   /*
    * Can add a list of pre-set prompts here if wanted.. perhaps
    * 'prompt 1' brings up a different, pre-set prompt 
    */
   if( !str_cmp( arg, "default" ) )
      ch->pcdata->fprompt = STRALLOC( "" );
   else if( !str_cmp( arg, "none" ) )
      ch->pcdata->fprompt = STRALLOC( ch->pcdata->prompt );
   else
      ch->pcdata->fprompt = STRALLOC( argument );
}

void do_prompt( CHAR_DATA* ch, const char* argument )
{
   char arg[MAX_INPUT_LENGTH];

   set_char_color( AT_GREY, ch );

   if( IS_NPC( ch ) )
   {
      send_to_char( "NPC's can't change their prompt..\r\n", ch );
      return;
   }
   smash_tilde( argument );
   one_argument( argument, arg );
   if( !*arg || !str_cmp( arg, "display" ) )
   {
      send_to_char( "Your current prompt string:\r\n", ch );
      set_char_color( AT_WHITE, ch );
      ch_printf( ch, "%s\r\n", !str_cmp( ch->pcdata->prompt, "" ) ? "(default prompt)" : ch->pcdata->prompt );
      set_char_color( AT_GREY, ch );
      send_to_char( "Type 'help prompt' for information on changing your prompt.\r\n", ch );
      return;
   }
   send_to_char( "Replacing old prompt of:\r\n", ch );
   set_char_color( AT_WHITE, ch );
   ch_printf( ch, "%s\r\n", !str_cmp( ch->pcdata->prompt, "" ) ? "(default prompt)" : ch->pcdata->prompt );
   if( ch->pcdata->prompt )
      STRFREE( ch->pcdata->prompt );

   char prompt[128];
   strlcpy(prompt, argument, 128);

   /*
    * Can add a list of pre-set prompts here if wanted.. perhaps
    * 'prompt 1' brings up a different, pre-set prompt 
    */
   if( !str_cmp( arg, "default" ) )
      ch->pcdata->prompt = STRALLOC( "" );
   else if( !str_cmp( arg, "fprompt" ) )
      ch->pcdata->prompt = STRALLOC( ch->pcdata->fprompt );
   else
      ch->pcdata->prompt = STRALLOC( argument );
}

void do_compass( CHAR_DATA *ch, const char *argument )
{
   char arg[MAX_INPUT_LENGTH];

   one_argument( argument, arg );

   if( arg[0] == '\0' )
   {
      if( xIS_SET( ch->act, PLR_COMPASS ) )
      {
         xREMOVE_BIT( ch->act, PLR_COMPASS );
         send_to_char( "Compass is now off.\r\n", ch );
      }
      else
      {
         xSET_BIT( ch->act, PLR_COMPASS );
         do_look( ch, "auto" );
      }
   }
}

/*
 *  Command to show current favor by Edmond
 */
void do_favor( CHAR_DATA * ch, const char *argument )
{
   char buf[MAX_STRING_LENGTH];

   if( IS_NPC( ch ) )
   {
      send_to_char( "Huh?\r\n", ch );
      return;
   }

   set_char_color( AT_GREEN, ch );
   if( !ch->pcdata->deity )
   {
      send_to_char( "You have not yet chosen a deity.\r\n", ch );
      return;
   }
   else if( ch->pcdata->favor > 2250 )
      strlcpy( buf, "loved", MAX_STRING_LENGTH );
   else if( ch->pcdata->favor > 2000 )
      strlcpy( buf, "cherished", MAX_STRING_LENGTH );
   else if( ch->pcdata->favor > 1750 )
      strlcpy( buf, "honored", MAX_STRING_LENGTH );
   else if( ch->pcdata->favor > 1500 )
      strlcpy( buf, "praised", MAX_STRING_LENGTH );
   else if( ch->pcdata->favor > 1250 )
      strlcpy( buf, "favored", MAX_STRING_LENGTH );
   else if( ch->pcdata->favor > 1000 )
      strlcpy( buf, "respected", MAX_STRING_LENGTH );
   else if( ch->pcdata->favor > 750 )
      strlcpy( buf, "liked", MAX_STRING_LENGTH );
   else if( ch->pcdata->favor > 250 )
      strlcpy( buf, "tolerated", MAX_STRING_LENGTH );
   else if( ch->pcdata->favor > -250 )
      strlcpy( buf, "ignored", MAX_STRING_LENGTH );
   else if( ch->pcdata->favor > -750 )
      strlcpy( buf, "shunned", MAX_STRING_LENGTH );
   else if( ch->pcdata->favor > -1000 )
      strlcpy( buf, "disliked", MAX_STRING_LENGTH );
   else if( ch->pcdata->favor > -1250 )
      strlcpy( buf, "dishonored", MAX_STRING_LENGTH );
   else if( ch->pcdata->favor > -1500 )
      strlcpy( buf, "disowned", MAX_STRING_LENGTH );
   else if( ch->pcdata->favor > -1750 )
      strlcpy( buf, "abandoned", MAX_STRING_LENGTH );
   else if( ch->pcdata->favor > -2000 )
      strlcpy( buf, "despised", MAX_STRING_LENGTH );
   else if( ch->pcdata->favor > -2250 )
      strlcpy( buf, "hated", MAX_STRING_LENGTH );
   else
      strlcpy( buf, "damned", MAX_STRING_LENGTH );

   ch_printf( ch, "%s considers you to be %s.\r\n", ch->pcdata->deity->name, buf );
}
