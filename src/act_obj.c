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
 *                         Object manipulation module                       *
 ****************************************************************************/

#include <stdio.h>
#include <string.h>
#include <bsd/string.h>
#include "mud.h"
#include "bet.h"

/*
 * External functions
 */
void write_corpses( CHAR_DATA * ch, const char *name, OBJ_DATA * objrem );
void save_house_by_vnum( int vnum );

/*
 * how resistant an object is to damage				-Thoric
 */
short get_obj_resistance( OBJ_DATA * obj )
{
   short resist;

   resist = number_fuzzy( MAX_ITEM_IMPACT );

   /*
    * magical items are more resistant 
    */
   if( IS_OBJ_STAT( obj, ITEM_MAGIC ) )
      resist += number_fuzzy( 12 );

   /*
    * metal objects are definately stronger 
    */
   if( IS_OBJ_STAT( obj, ITEM_METAL ) )
      resist += number_fuzzy( 5 );

   /*
    * organic objects are most likely weaker 
    */
   if( IS_OBJ_STAT( obj, ITEM_ORGANIC ) )
      resist -= number_fuzzy( 5 );

   /*
    * blessed objects should have a little bonus 
    */
   if( IS_OBJ_STAT( obj, ITEM_BLESS ) )
      resist += number_fuzzy( 5 );

   /*
    * lets make store inventory pretty tough 
    */
   if( IS_OBJ_STAT( obj, ITEM_INVENTORY ) )
      resist += 20;

   /*
    * okay... let's add some bonus/penalty for item level... 
    */
   resist += ( obj->level / 10 ) - 2;

   /*
    * and lasty... take armor or weapon's condition into consideration 
    */
   if( obj->item_type == ITEM_ARMOR || obj->item_type == ITEM_WEAPON )
      resist += ( obj->value[0] / 2 ) - 2;

   return URANGE( 10, resist, 99 );
}

void get_obj( CHAR_DATA * ch, OBJ_DATA * obj, OBJ_DATA * container )
{
   VAULT_DATA *vault;
   int weight;
   int amt; /* gold per-race multipliers */

   if( !CAN_WEAR( obj, ITEM_TAKE ) && ( ch->level < sysdata.level_getobjnotake ) )
   {
      send_to_char( "You can't take that.\r\n", ch );
      return;
   }

   /* ============================================
      Wowzers Mud: Soulbinding Get Checks -Hansth
      ============================================ */
   if ( obj->soulbound && obj->soulbound[0] != '\0' && str_cmp( obj->soulbound, ch->name ) && !IS_IMMORTAL(ch) )
   {
      ch_printf( ch, "%s is soulbound to someone else.\r\n", obj->short_descr );
      return;
   }

   /* Bind on Pickup (BoP) */
   if ( !IS_NPC( ch ) && IS_OBJ_STAT( obj, ITEM_BOP ) && ( !obj->soulbound || obj->soulbound[0] == '\0' ) )
   {
      obj->soulbound = STRALLOC( ch->name );
      act( AT_MAGIC, "$p binds to your soul!", ch, obj, NULL, TO_CHAR );
   }

   if( IS_SET( obj->magic_flags, ITEM_PKDISARMED ) && !IS_NPC( ch ) )
   {
      if( CAN_PKILL( ch ) && !get_timer( ch, TIMER_PKILLED ) )
      {
         if( !is_name( ch->name, obj->action_desc ) && !IS_IMMORTAL( ch ) )
         {
            send_to_char_color( "\r\n&bA godly force freezes your outstretched hand.\r\n", ch );
            return;
         }
         else
         {
            REMOVE_BIT( obj->magic_flags, ITEM_PKDISARMED );
            STRFREE( obj->action_desc );
            obj->action_desc = STRALLOC( "" );
         }
      }
      else
      {
         send_to_char_color( "\r\n&BA godly force freezes your outstretched hand.\r\n", ch );
         return;
      }
   }

   if( IS_OBJ_STAT( obj, ITEM_PROTOTYPE ) && !can_take_proto( ch ) )
   {
      send_to_char( "A godly force prevents you from getting close to it.\r\n", ch );
      return;
   }

   if( ch->carry_number + get_obj_number( obj ) > can_carry_n( ch ) )
   {
      act( AT_PLAIN, "$d: you can't carry that many items.", ch, NULL, obj->short_descr, TO_CHAR );
      return;
   }

   if( IS_OBJ_STAT( obj, ITEM_COVERING ) )
      weight = obj->weight;
   else
      weight = get_obj_weight( obj );

   /*
    * Money weight shouldn't count 
    */
   if( obj->item_type != ITEM_MONEY )
   {
      if( obj->in_obj )
      {
         OBJ_DATA *tobj = obj->in_obj;
         int inobj = 1;
         bool checkweight = FALSE;

         /*
          * need to make it check weight if its in a magic container 
          */
         if( tobj->item_type == ITEM_CONTAINER && IS_OBJ_STAT( tobj, ITEM_MAGIC ) )
            checkweight = TRUE;

         while( tobj->in_obj )
         {
            tobj = tobj->in_obj;
            inobj++;

            /*
             * need to make it check weight if its in a magic container 
             */
            if( tobj->item_type == ITEM_CONTAINER && IS_OBJ_STAT( tobj, ITEM_MAGIC ) )
               checkweight = TRUE;
         }

         /*
          * need to check weight if not carried by ch or in a magic container. 
          */
         if( !tobj->carried_by || tobj->carried_by != ch || checkweight )
         {
            if( ( ch->carry_weight + weight ) > can_carry_w( ch ) )
            {
               act( AT_PLAIN, "$d: you can't carry that much weight.", ch, NULL, obj->short_descr, TO_CHAR );
               return;
            }
         }
      }
      else if( ( ch->carry_weight + weight ) > can_carry_w( ch ) )
      {
         act( AT_PLAIN, "$d: you can't carry that much weight.", ch, NULL, obj->short_descr, TO_CHAR );
         return;
      }
   }

   if( container )
   {
      if( container->item_type == ITEM_KEYRING && !IS_OBJ_STAT( container, ITEM_COVERING ) )
      {
         act( AT_ACTION, "You remove $p from $P", ch, obj, container, TO_CHAR );
         act( AT_ACTION, "$n removes $p from $P", ch, obj, container, TO_ROOM );
      }
      else
      {
         act( AT_ACTION, IS_OBJ_STAT( container, ITEM_COVERING ) ?
              "You get $p from beneath $P." : "You get $p from $P", ch, obj, container, TO_CHAR );
         act( AT_ACTION, IS_OBJ_STAT( container, ITEM_COVERING ) ?
              "$n gets $p from beneath $P." : "$n gets $p from $P", ch, obj, container, TO_ROOM );
      }
      if( IS_OBJ_STAT( container, ITEM_CLANCORPSE ) && !IS_NPC( ch ) && str_cmp( container->name + 7, ch->name ) )
         container->value[5]++;
      obj_from_obj( obj );
   }
   else
   {
      act( AT_ACTION, "You get $p.", ch, obj, container, TO_CHAR );
      act( AT_ACTION, "$n gets $p.", ch, obj, container, TO_ROOM );
      obj_from_room( obj );
   }

   if( xIS_SET( ch->in_room->room_flags, ROOM_HOUSE ) )
      save_house_by_vnum( ch->in_room->vnum ); /* House Object Saving */

   /*
    * Clan storeroom checks
    */
   if( xIS_SET( ch->in_room->room_flags, ROOM_CLANSTOREROOM ) && ( !container || container->carried_by == NULL ) )
   {
      for( vault = first_vault; vault; vault = vault->next )
         if( vault->vnum == ch->in_room->vnum )
            save_storeroom( ch, vault->vnum );
   }

   check_for_trap( ch, obj, TRAP_GET );
   if( char_died( ch ) )
      return;

   if( obj->item_type == ITEM_MONEY )
   {
      amt = obj->value[0] * obj->count;

      ch->gold += amt;
      extract_obj( obj );
   }
   else
      obj = obj_to_char( obj, ch );

   if( char_died( ch ) || obj_extracted( obj ) )
      return;
   oprog_get_trigger( ch, obj );
}

void do_connect( CHAR_DATA *ch, const char *argument )
{
   OBJ_DATA *first_ob;
   OBJ_DATA *second_ob;
   OBJ_DATA *new_ob;
   char arg1[MAX_STRING_LENGTH], arg2[MAX_STRING_LENGTH];

   argument = one_argument( argument, arg1 );
   argument = one_argument( argument, arg2 );

   if( arg1[0] == '\0' || arg2[0] == '\0' )
   {
      send_to_char( "Connect what to what?\r\n", ch );
      return;
   }

   if( ( first_ob = get_obj_carry( ch, arg1 ) )  == NULL )
   {
      send_to_char( "You aren't holding the necessary objects.\r\n", ch );
      return;
   }

   if( ( second_ob = get_obj_carry( ch, arg2 ) )  == NULL )
   {
      send_to_char( "You aren't holding the necessary objects.\r\n", ch );
      return;
   }

   separate_obj( first_ob );
   separate_obj( second_ob );

   if( first_ob->item_type != ITEM_PIECE || second_ob->item_type !=ITEM_PIECE )
   {
      send_to_char( "You stare at them for a moment, but these items clearly don't go together.\r\n", ch );
      return;
   }

   /* check to see if the pieces connect */
   if( ( first_ob->value[0] == second_ob->pIndexData->vnum ) || ( first_ob->value[1] == second_ob->pIndexData->vnum ) )
   /* good connection  */
   {
      new_ob = create_object( get_obj_index( first_ob->value[2] ), ch->level );
      extract_obj( first_ob );
      extract_obj( second_ob );
      obj_to_char( new_ob , ch );
      act( AT_ACTION, "$n cobbles some objects together.... suddenly they snap together, creating $p!", ch, new_ob,NULL, TO_ROOM );
      act( AT_ACTION, "You cobble the objects together.... suddenly they snap together, creating $p!", ch, new_ob, NULL, TO_CHAR );
   }
   else
   {
      act( AT_ACTION, "$n jiggles some objects against each other, but can't seem to make them connect.", ch, NULL, NULL, TO_ROOM );
      act( AT_ACTION, "You try to fit them together every which way, but they just don't want to connect.", ch, NULL, NULL, TO_CHAR );
   }
}

void do_get( CHAR_DATA* ch, const char* argument )
{
   char arg1[MAX_INPUT_LENGTH];
   char arg2[MAX_INPUT_LENGTH];
   OBJ_DATA *obj;
   OBJ_DATA *obj_next;
   OBJ_DATA *container;
   short number;
   bool found;

   argument = one_argument( argument, arg1 );

   if( ch->carry_number < 0 || ch->carry_weight < 0 )
   {
      send_to_char( "Uh oh ... better contact an immortal about your number or weight of items carried.\r\n", ch );
      log_printf( "%s has negative carry_number or carry_weight!", ch->name );
      return;
   }

   if( is_number( arg1 ) )
   {
      number = atoi( arg1 );
      if( number < 1 )
      {
         send_to_char( "That was easy...\r\n", ch );
         return;
      }
      if( ( ch->carry_number + number ) > can_carry_n( ch ) )
      {
         send_to_char( "You can't carry that many.\r\n", ch );
         return;
      }
      argument = one_argument( argument, arg1 );
   }
   else
      number = 0;
   argument = one_argument( argument, arg2 );
   /*
    * munch optional words 
    */
   if( !str_cmp( arg2, "from" ) && argument[0] != '\0' )
      argument = one_argument( argument, arg2 );

   /*
    * Get type. 
    */
   if( arg1[0] == '\0' )
   {
      send_to_char( "Get what?\r\n", ch );
      return;
   }

   if( ms_find_obj( ch ) )
      return;

   if( arg2[0] == '\0' )
   {
      if( number <= 1 && str_cmp( arg1, "all" ) && str_prefix( "all.", arg1 ) )
      {
         /*
          * 'get obj' 
          */
         obj = get_obj_list( ch, arg1, ch->in_room->first_content );
         if( !obj )
         {
            act( AT_PLAIN, "I see no $T here.", ch, NULL, arg1, TO_CHAR );
            return;
         }
         separate_obj( obj );
         get_obj( ch, obj, NULL );
         if( char_died( ch ) )
            return;
         if( IS_SET( sysdata.save_flags, SV_GET ) )
            save_char_obj( ch );
      }
      else
      {
         short cnt = 0;
         bool fAll;
         char *chk;

         if( xIS_SET( ch->in_room->room_flags, ROOM_DONATION ) )
         {
            send_to_char( "The gods frown upon such a display of greed!\r\n", ch );
            return;
         }
         if( !str_cmp( arg1, "all" ) )
            fAll = TRUE;
         else
            fAll = FALSE;
         if( number > 1 )
            chk = arg1;
         else
            chk = &arg1[4];
         /*
          * 'get all' or 'get all.obj' 
          */
         found = FALSE;
         for( obj = ch->in_room->last_content; obj; obj = obj_next )
         {
            obj_next = obj->prev_content;
            if( ( fAll || nifty_is_name( chk, obj->name ) ) && can_see_obj( ch, obj ) )
            {
               found = TRUE;
               if( number && ( cnt + obj->count ) > number )
                  split_obj( obj, number - cnt );
               cnt += obj->count;
               get_obj( ch, obj, NULL );
               if( char_died( ch )
                   || ch->carry_number >= can_carry_n( ch )
                   || ch->carry_weight >= can_carry_w( ch ) || ( number && cnt >= number ) )
               {
                  if( IS_SET( sysdata.save_flags, SV_GET ) && !char_died( ch ) )
                     save_char_obj( ch );
                  return;
               }
            }
         }

         if( !found )
         {
            if( fAll )
               send_to_char( "I see nothing here.\r\n", ch );
            else
               act( AT_PLAIN, "I see no $T here.", ch, NULL, chk, TO_CHAR );
         }
         else if( IS_SET( sysdata.save_flags, SV_GET ) )
            save_char_obj( ch );
      }
   }
   else
   {
      /*
       * 'get ... container' 
       */
      if( !str_cmp( arg2, "all" ) || !str_prefix( "all.", arg2 ) )
      {
         send_to_char( "You can't do that.\r\n", ch );
         return;
      }

      if( ( container = get_obj_here( ch, arg2 ) ) == NULL )
      {
         act( AT_PLAIN, "I see no $T here.", ch, NULL, arg2, TO_CHAR );
         return;
      }

      switch ( container->item_type )
      {
         default:
            if( !IS_OBJ_STAT( container, ITEM_COVERING ) )
            {
               send_to_char( "That's not a container.\r\n", ch );
               return;
            }
            if( ch->carry_weight + container->weight > can_carry_w( ch ) )
            {
               send_to_char( "It's too heavy for you to lift.\r\n", ch );
               return;
            }
            break;

         case ITEM_CONTAINER:
         case ITEM_CORPSE_NPC:
         case ITEM_KEYRING:
         case ITEM_QUIVER:
            break;

         case ITEM_CORPSE_PC:
         {
            char name[MAX_INPUT_LENGTH];
            CHAR_DATA *gch;
            const char *pd;

            if( IS_NPC( ch ) )
            {
               send_to_char( "You can't do that.\r\n", ch );
               return;
            }

            pd = container->short_descr;
            pd = one_argument( pd, name );
            pd = one_argument( pd, name );
            pd = one_argument( pd, name );
            pd = one_argument( pd, name );

            if( IS_OBJ_STAT( container, ITEM_CLANCORPSE )
                && !IS_NPC( ch ) && ( get_timer( ch, TIMER_PKILLED ) > 0 ) && str_cmp( name, ch->name ) )
            {
               send_to_char( "You cannot loot from that corpse...yet.\r\n", ch );
               return;
            }

            /*
             * Killer/owner loot only if die to pkill blow --Blod 
             */
            /*
             * Added check for immortal so IMMS can get things out of
             * * corpses --Shaddai 
             */

            if( IS_OBJ_STAT( container, ITEM_CLANCORPSE )
                && !IS_NPC( ch ) && !IS_IMMORTAL( ch )
                && container->action_desc[0] != '\0'
                && str_cmp( name, ch->name ) && str_cmp( container->action_desc, ch->name ) )
            {
               send_to_char( "You did not inflict the death blow upon this corpse.\r\n", ch );
               return;
            }

            if( IS_OBJ_STAT( container, ITEM_CLANCORPSE ) && !IS_IMMORTAL( ch )
                && !IS_NPC( ch ) && str_cmp( name, ch->name ) && container->value[5] >= 3 )
            {
               send_to_char( "Frequent looting has left this corpse protected by the gods.\r\n", ch );
               return;
            }

            if( IS_OBJ_STAT( container, ITEM_CLANCORPSE )
                && !IS_NPC( ch ) && IS_SET( ch->pcdata->flags, PCFLAG_DEADLY )
                && container->value[4] - ch->level < 6 && container->value[4] - ch->level > -6 )
               break;

            if( !str_cmp( name, ch->name ) && !IS_IMMORTAL( ch ) )
            {
               bool fGroup;

               fGroup = FALSE;
               for( gch = first_char; gch; gch = gch->next )
               {
                  if( !IS_NPC( gch ) && is_same_group( ch, gch ) && !str_cmp( name, gch->name ) )
                  {
                     fGroup = TRUE;
                     break;
                  }
               }

               if( !fGroup )
               {
                  send_to_char( "That's someone else's corpse.\r\n", ch );
                  return;
               }
            }
         }
      }

      if( !IS_OBJ_STAT( container, ITEM_COVERING ) && IS_SET( container->value[1], CONT_CLOSED ) )
      {
         act( AT_PLAIN, "The $d is closed.", ch, NULL, container->name, TO_CHAR );
         return;
      }

      if( number <= 1 && str_cmp( arg1, "all" ) && str_prefix( "all.", arg1 ) )
      {
         /*
          * 'get obj container' 
          */
         obj = get_obj_list( ch, arg1, container->first_content );
         if( !obj )
         {
            act( AT_PLAIN, IS_OBJ_STAT( container, ITEM_COVERING ) ?
               "I see nothing like that beneath the $T." : "I see nothing like that in the $T.", ch, NULL, container->short_descr, TO_CHAR );
            return;
         }
         separate_obj( obj );
         get_obj( ch, obj, container );
         /*
          * Oops no wonder corpses were duping oopsie did I do that
          * * --Shaddai
          */
         if( container->item_type == ITEM_CORPSE_PC )
            write_corpses( NULL, container->short_descr + 14, NULL );
         check_for_trap( ch, container, TRAP_GET );
         if( char_died( ch ) )
            return;
         if( IS_SET( sysdata.save_flags, SV_GET ) )
            save_char_obj( ch );
      }
      else
      {
         int cnt = 0;
         bool fAll;
         char *chk;

         /*
          * 'get all container' or 'get all.obj container' 
          */
         if( IS_OBJ_STAT( container, ITEM_DONATION ) )
         {
            send_to_char( "The gods frown upon such an act of greed!\r\n", ch );
            return;
         }

         if( IS_OBJ_STAT( container, ITEM_CLANCORPSE )
             && !IS_IMMORTAL( ch ) && !IS_NPC( ch ) && str_cmp( ch->name, container->name + 7 ) )
         {
            send_to_char( "The gods frown upon such wanton greed!\r\n", ch );
            return;
         }

         if( !str_cmp( arg1, "all" ) )
            fAll = TRUE;
         else
            fAll = FALSE;
         if( number > 1 )
            chk = arg1;
         else
            chk = &arg1[4];
         found = FALSE;
         for( obj = container->first_content; obj; obj = obj_next )
         {
            obj_next = obj->next_content;
            if( ( fAll || nifty_is_name( chk, obj->name ) ) && can_see_obj( ch, obj ) )
            {
               found = TRUE;
               if( number && ( cnt + obj->count ) > number )
                  split_obj( obj, number - cnt );
               cnt += obj->count;
               get_obj( ch, obj, container );
               if( char_died( ch )
                   || ch->carry_number >= can_carry_n( ch )
                   || ch->carry_weight >= can_carry_w( ch ) || ( number && cnt >= number ) )
               {
                  if( container->item_type == ITEM_CORPSE_PC )
                     write_corpses( NULL, container->short_descr + 14, NULL );
                  if( found && IS_SET( sysdata.save_flags, SV_GET ) )
                     save_char_obj( ch );
                  return;
               }
            }
         }

         if( !found )
         {
            if( fAll )
            {
               if( container->item_type == ITEM_KEYRING && !IS_OBJ_STAT( container, ITEM_COVERING ) )
                  act( AT_PLAIN, "The $T holds no keys.", ch, NULL, container->short_descr, TO_CHAR );
               else
                  act( AT_PLAIN, IS_OBJ_STAT( container, ITEM_COVERING ) ?
                       "I see nothing beneath the $T." : "I see nothing in the $T.", ch, NULL, container->short_descr, TO_CHAR );
            }
            else
            {
               if( container->item_type == ITEM_KEYRING && !IS_OBJ_STAT( container, ITEM_COVERING ) )
                  act( AT_PLAIN, "The $T does not hold that key.", ch, NULL, container->short_descr, TO_CHAR );
               else
                  act( AT_PLAIN, IS_OBJ_STAT( container, ITEM_COVERING ) ?
                       "I see nothing like that beneath the $T." :
                       "I see nothing like that in the $T.", ch, NULL, container->short_descr, TO_CHAR );
            }
         }
         else
            check_for_trap( ch, container, TRAP_GET );
         if( char_died( ch ) )
            return;
         /*
          * Oops no wonder corpses were duping oopsie did I do that
          * * --Shaddai
          */
         if( container->item_type == ITEM_CORPSE_PC )
            write_corpses( NULL, container->short_descr + 14, NULL );
         if( found && IS_SET( sysdata.save_flags, SV_GET ) )
            save_char_obj( ch );
      }
   }
}

void do_put( CHAR_DATA* ch, const char* argument )
{
   char arg1[MAX_INPUT_LENGTH];
   char arg2[MAX_INPUT_LENGTH];
   OBJ_DATA *container;
   OBJ_DATA *obj;
   OBJ_DATA *obj_next;
   VAULT_DATA *vault;
   short count;
   int number;
   bool save_char = FALSE;

   argument = one_argument( argument, arg1 );
   if( is_number( arg1 ) )
   {
      number = atoi( arg1 );
      if( number < 1 )
      {
         send_to_char( "That was easy...\r\n", ch );
         return;
      }
      argument = one_argument( argument, arg1 );
   }
   else
      number = 0;
   argument = one_argument( argument, arg2 );
   /*
    * munch optional words 
    */
   if( ( !str_cmp( arg2, "into" ) || !str_cmp( arg2, "inside" )
         || !str_cmp( arg2, "in" ) || !str_cmp( arg2, "under" )
         || !str_cmp( arg2, "onto" ) || !str_cmp( arg2, "on" ) ) && argument[0] != '\0' )
      argument = one_argument( argument, arg2 );

   if( arg1[0] == '\0' || arg2[0] == '\0' )
   {
      send_to_char( "Put what in what?\r\n", ch );
      return;
   }

   if( ms_find_obj( ch ) )
      return;

   if( !str_cmp( arg2, "all" ) || !str_prefix( "all.", arg2 ) )
   {
      send_to_char( "You can't do that.\r\n", ch );
      return;
   }

   if( ( container = get_obj_here( ch, arg2 ) ) == NULL )
   {
      act( AT_PLAIN, "I see no $T here.", ch, NULL, arg2, TO_CHAR );
      return;
   }

   if( !container->carried_by && IS_SET( sysdata.save_flags, SV_PUT ) )
      save_char = TRUE;

   if( IS_OBJ_STAT( container, ITEM_COVERING ) )
   {
      if( ch->carry_weight + container->weight > can_carry_w( ch ) )
      {
         send_to_char( "It's too heavy for you to lift.\r\n", ch );
         return;
      }
   }
   else
   {
      if( container->item_type != ITEM_CONTAINER
          && container->item_type != ITEM_KEYRING && container->item_type != ITEM_QUIVER )
      {
         send_to_char( "That's not a container.\r\n", ch );
         return;
      }

      if( IS_SET( container->value[1], CONT_CLOSED ) )
      {
         act( AT_PLAIN, "The $d is closed.", ch, NULL, container->name, TO_CHAR );
         return;
      }
   }

   if( number <= 1 && str_cmp( arg1, "all" ) && str_prefix( "all.", arg1 ) )
   {
      /*
       * 'put obj container' 
       */
      if( ( obj = get_obj_carry( ch, arg1 ) ) == NULL )
      {
         send_to_char( "You do not have that item.\r\n", ch );
         return;
      }

      if( obj == container )
      {
         send_to_char( "You can't fold it into itself.\r\n", ch );
         return;
      }

      if( !can_drop_obj( ch, obj ) )
      {
         send_to_char( "You can't let go of it.\r\n", ch );
         return;
      }

      if( container->item_type == ITEM_KEYRING && obj->item_type != ITEM_KEY )
      {
         send_to_char( "That's not a key.\r\n", ch );
         return;
      }

      if( container->item_type == ITEM_QUIVER && obj->item_type != ITEM_PROJECTILE )
      {
         send_to_char( "That's not a projectile.\r\n", ch );
         return;
      }

      // Fix by Luc - Sometime in 2000?
      {
         int tweight = ( get_real_obj_weight( container ) / container->count ) + ( get_real_obj_weight( obj ) / obj->count );

         if( IS_OBJ_STAT( container, ITEM_COVERING ) )
         {
            if( container->item_type == ITEM_CONTAINER
                ? tweight > container->value[0] : tweight - container->weight > container->weight )
            {
               send_to_char( "It won't fit under there.\r\n", ch );
               return;
            }
         }
         else if( tweight - container->weight > container->value[0] )
         {
            send_to_char( "It won't fit.\r\n", ch );
            return;
         }
      }
      // Fix end

      if( container->in_room && container->in_room->max_weight
         && container->in_room->max_weight < get_real_obj_weight( obj ) / obj->count + container->in_room->weight )
      {
         send_to_char( "It won't fit.\r\n", ch );
         return;
      }

      separate_obj( obj );
      separate_obj( container );
      obj_from_char( obj );
      obj = obj_to_obj( obj, container );
      check_for_trap( ch, container, TRAP_PUT );
      if( char_died( ch ) )
         return;
      count = obj->count;
      obj->count = 1;
      if( container->item_type == ITEM_KEYRING && !IS_OBJ_STAT( container, ITEM_COVERING ) )
      {
         act( AT_ACTION, "$n slips $p onto $P.", ch, obj, container, TO_ROOM );
         act( AT_ACTION, "You slip $p onto $P.", ch, obj, container, TO_CHAR );
      }
      else
      {
         act( AT_ACTION, IS_OBJ_STAT( container, ITEM_COVERING )
              ? "$n hides $p beneath $P." : "$n puts $p in $P.", ch, obj, container, TO_ROOM );
         act( AT_ACTION, IS_OBJ_STAT( container, ITEM_COVERING )
              ? "You hide $p beneath $P." : "You put $p in $P.", ch, obj, container, TO_CHAR );
      }
      obj->count = count;

      /*
       * Oops no wonder corpses were duping oopsie did I do that
       * * --Shaddai
       */
      if( container->item_type == ITEM_CORPSE_PC )
         write_corpses( NULL, container->short_descr + 14, NULL );

      if( save_char )
         save_char_obj( ch );

      if( xIS_SET( ch->in_room->room_flags, ROOM_HOUSE ) )
         save_house_by_vnum( ch->in_room->vnum ); /* House Object Saving */

      /*
       * Clan storeroom check 
       */
      if( xIS_SET( ch->in_room->room_flags, ROOM_CLANSTOREROOM ) && container->carried_by == NULL )
      {
         for( vault = first_vault; vault; vault = vault->next )
            if( vault->vnum == ch->in_room->vnum )
               save_storeroom( ch, vault->vnum );
      }
   }
   else
   {
      bool found = FALSE;
      int cnt = 0;
      bool fAll;
      char *chk;

      if( !str_cmp( arg1, "all" ) )
         fAll = TRUE;
      else
         fAll = FALSE;
      if( number > 1 )
         chk = arg1;
      else
         chk = &arg1[4];

      separate_obj( container );
      /*
       * 'put all container' or 'put all.obj container' 
       */
      for( obj = ch->first_carrying; obj; obj = obj_next )
      {
         obj_next = obj->next_content;

         if( ( fAll || nifty_is_name( chk, obj->name ) )
             && can_see_obj( ch, obj )
             && obj->wear_loc == WEAR_NONE
             && obj != container
             && can_drop_obj( ch, obj )
             && ( container->item_type != ITEM_KEYRING || obj->item_type == ITEM_KEY )
             && ( container->item_type != ITEM_QUIVER || obj->item_type == ITEM_PROJECTILE )
             && get_obj_weight( obj ) + get_obj_weight( container ) <= container->value[0] )
         {
            if( number && ( cnt + obj->count ) > number )
               split_obj( obj, number - cnt );
            cnt += obj->count;
            obj_from_char( obj );
            if( container->item_type == ITEM_KEYRING )
            {
               act( AT_ACTION, "$n slips $p onto $P.", ch, obj, container, TO_ROOM );
               act( AT_ACTION, "You slip $p onto $P.", ch, obj, container, TO_CHAR );
            }
            else
            {
               act( AT_ACTION, "$n puts $p in $P.", ch, obj, container, TO_ROOM );
               act( AT_ACTION, "You put $p in $P.", ch, obj, container, TO_CHAR );
            }
            obj = obj_to_obj( obj, container );
            found = TRUE;

            check_for_trap( ch, container, TRAP_PUT );
            if( char_died( ch ) )
               return;
            if( number && cnt >= number )
               break;
         }
      }

      /*
       * Don't bother to save anything if nothing was dropped   -Thoric
       */
      if( !found )
      {
         if( fAll )
            act( AT_PLAIN, "You are not carrying anything.", ch, NULL, NULL, TO_CHAR );
         else
            act( AT_PLAIN, "You are not carrying any $T.", ch, NULL, chk, TO_CHAR );
         return;
      }

      if( save_char )
         save_char_obj( ch );
      /*
       * Oops no wonder corpses were duping oopsie did I do that
       * * --Shaddai
       */
      if( container->item_type == ITEM_CORPSE_PC )
         write_corpses( NULL, container->short_descr + 14, NULL );

      if( xIS_SET( ch->in_room->room_flags, ROOM_HOUSE ) )
         save_house_by_vnum( ch->in_room->vnum ); /* House Object Saving */

      /*
       * Clan storeroom check 
       */
      if( xIS_SET( ch->in_room->room_flags, ROOM_CLANSTOREROOM ) && container->carried_by == NULL )
      {
         for( vault = first_vault; vault; vault = vault->next )
            if( vault->vnum == ch->in_room->vnum )
               save_storeroom( ch, vault->vnum );
      }
   }
}

void do_drop( CHAR_DATA* ch, const char* argument )
{
   char arg[MAX_INPUT_LENGTH];
   OBJ_DATA *obj;
   OBJ_DATA *obj_next;
   bool found;
   VAULT_DATA *vault;
   int number;

   argument = one_argument( argument, arg );
   if( is_number( arg ) )
   {
      number = atoi( arg );
      if( number < 1 )
      {
         send_to_char( "That was easy...\r\n", ch );
         return;
      }
      argument = one_argument( argument, arg );
   }
   else
      number = 0;

   if( arg[0] == '\0' )
   {
      send_to_char( "Drop what?\r\n", ch );
      return;
   }

   if( ms_find_obj( ch ) )
      return;

   if( !IS_NPC( ch ) && xIS_SET( ch->act, PLR_LITTERBUG ) )
   {
      set_char_color( AT_YELLOW, ch );
      send_to_char( "A godly force prevents you from dropping anything...\r\n", ch );
      return;
   }

   if( xIS_SET( ch->in_room->room_flags, ROOM_NODROP ) && ch != supermob )
   {
      set_char_color( AT_MAGIC, ch );
      send_to_char( "A magical force stops you!\r\n", ch );
      set_char_color( AT_TELL, ch );
      send_to_char( "Someone tells you, 'No littering here!'\r\n", ch );
      return;
   }

   if( number > 0 )
   {
      /*
       * 'drop NNNN coins' 
       */
      if( !str_cmp( arg, "coins" ) || !str_cmp( arg, "coin" ) )
      {
         if( ch->gold < number )
         {
            send_to_char( "You haven't got that many coins.\r\n", ch );
            return;
         }

         ch->gold -= number;

         for( obj = ch->in_room->first_content; obj; obj = obj_next )
         {
            obj_next = obj->next_content;

            switch ( obj->pIndexData->vnum )
            {
               case OBJ_VNUM_MONEY_ONE:
                  number += 1;
                  extract_obj( obj );
                  break;

               case OBJ_VNUM_MONEY_SOME:
                  number += obj->value[0];
                  extract_obj( obj );
                  break;
            }
         }

         act( AT_ACTION, "$n drops some gold.", ch, NULL, NULL, TO_ROOM );
         obj_to_room( create_money( number ), ch->in_room );
         send_to_char( "You let the gold slip from your hand.\r\n", ch );
         if( IS_SET( sysdata.save_flags, SV_DROP ) )
            save_char_obj( ch );
         return;
      }
   }

   if( number <= 1 && str_cmp( arg, "all" ) && str_prefix( "all.", arg ) )
   {
      /*
       * 'drop obj' 
       */
      if( ( obj = get_obj_carry( ch, arg ) ) == NULL )
      {
         send_to_char( "You do not have that item.\r\n", ch );
         return;
      }

      if( !can_drop_obj( ch, obj ) )
      {
         send_to_char( "You can't let go of it.\r\n", ch );
         return;
      }

      if( ch->in_room->max_weight > 0
         && ch->in_room->max_weight < get_real_obj_weight( obj ) / obj->count + ch->in_room->weight )
      {
         send_to_char( "There is not enough room on the ground for that.\r\n", ch );
         return;
      }

      separate_obj( obj );
      act( AT_ACTION, "$n drops $p.", ch, obj, NULL, TO_ROOM );
      act( AT_ACTION, "You drop $p.", ch, obj, NULL, TO_CHAR );

      obj_from_char( obj );
      obj = obj_to_room( obj, ch->in_room );
      oprog_drop_trigger( ch, obj );   /* mudprogs */

      if( char_died( ch ) || obj_extracted( obj ) )
         return;

      if( xIS_SET( ch->in_room->room_flags, ROOM_HOUSE ) )
         save_house_by_vnum( ch->in_room->vnum ); /* House Object Saving */

      /*
       * Clan storeroom saving 
       */
      if( xIS_SET( ch->in_room->room_flags, ROOM_CLANSTOREROOM ) )
      {
         for( vault = first_vault; vault; vault = vault->next )
            if( vault->vnum == ch->in_room->vnum )
               save_storeroom( ch, vault->vnum );
      }
   }
   else
   {
      int cnt = 0;
      char *chk;
      bool fAll;

      if( !str_cmp( arg, "all" ) )
         fAll = TRUE;
      else
         fAll = FALSE;
      if( number > 1 )
         chk = arg;
      else
         chk = &arg[4];
      /*
       * 'drop all' or 'drop all.obj' 
       */
      if( xIS_SET( ch->in_room->room_flags, ROOM_NODROPALL ) || xIS_SET( ch->in_room->room_flags, ROOM_CLANSTOREROOM ) )
      {
         send_to_char( "You can't seem to do that here...\r\n", ch );
         return;
      }

      found = FALSE;
      for( obj = ch->first_carrying; obj; obj = obj_next )
      {
         obj_next = obj->next_content;

         if( ( fAll || nifty_is_name( chk, obj->name ) )
            && can_see_obj( ch, obj ) && obj->wear_loc == WEAR_NONE && can_drop_obj( ch, obj )
            && ( !ch->in_room->max_weight || ch->in_room->max_weight > get_real_obj_weight( obj ) / obj->count + ch->in_room->weight ) )
         {
            found = TRUE;
            if( HAS_PROG( obj->pIndexData, DROP_PROG ) && obj->count > 1 )
            {
               ++cnt;
               separate_obj( obj );
               obj_from_char( obj );
               if( !obj_next )
                  obj_next = ch->first_carrying;
            }
            else
            {
               if( number && ( cnt + obj->count ) > number )
                  split_obj( obj, number - cnt );
               cnt += obj->count;
               obj_from_char( obj );
            }
            act( AT_ACTION, "$n drops $p.", ch, obj, NULL, TO_ROOM );
            act( AT_ACTION, "You drop $p.", ch, obj, NULL, TO_CHAR );
            obj = obj_to_room( obj, ch->in_room );
            oprog_drop_trigger( ch, obj );   /* mudprogs */
            if( char_died( ch ) )
               return;
            if( number && cnt >= number )
               break;
         }
      }

      if( !found )
      {
         if( fAll )
            act( AT_PLAIN, "You are not carrying anything.", ch, NULL, NULL, TO_CHAR );
         else
            act( AT_PLAIN, "You are not carrying any $T.", ch, NULL, chk, TO_CHAR );
      }
   }

   if( xIS_SET( ch->in_room->room_flags, ROOM_HOUSE ) )
      save_house_by_vnum( ch->in_room->vnum ); /* House Object Saving */

   /*
    * Clan storeroom saving 
    */
   if( xIS_SET( ch->in_room->room_flags, ROOM_CLANSTOREROOM ) )
   {
      for( vault = first_vault; vault; vault = vault->next )
         if( vault->vnum == ch->in_room->vnum )
            save_storeroom( ch, vault->vnum );
   }

   if( IS_SET( sysdata.save_flags, SV_DROP ) )
      save_char_obj( ch ); /* duping protector */
}

void do_give( CHAR_DATA* ch, const char* argument )
{
   char arg1[MAX_INPUT_LENGTH];
   char arg2[MAX_INPUT_LENGTH];
   char buf[MAX_INPUT_LENGTH];
   CHAR_DATA *victim;
   OBJ_DATA *obj;

   argument = one_argument( argument, arg1 );
   argument = one_argument( argument, arg2 );
   if( !str_cmp( arg2, "to" ) && argument[0] != '\0' )
      argument = one_argument( argument, arg2 );

   if( arg1[0] == '\0' || arg2[0] == '\0' )
   {
      send_to_char( "Give what to whom?\r\n", ch );
      return;
   }

   if( ms_find_obj( ch ) )
      return;

   if( is_number( arg1 ) )
   {
      /*
       * 'give NNNN coins victim' 
       */
      int amount;

      amount = atoi( arg1 );
      if( amount <= 0 || ( str_cmp( arg2, "coins" ) && str_cmp( arg2, "coin" ) ) )
      {
         send_to_char( "Sorry, you can't do that.\r\n", ch );
         return;
      }

      argument = one_argument( argument, arg2 );
      if( !str_cmp( arg2, "to" ) && argument[0] != '\0' )
         argument = one_argument( argument, arg2 );
      if( arg2[0] == '\0' )
      {
         send_to_char( "Give what to whom?\r\n", ch );
         return;
      }

      if( ( victim = get_char_room( ch, arg2 ) ) == NULL )
      {
         send_to_char( "They aren't here.\r\n", ch );
         return;
      }

      if( ch->gold < amount )
      {
         send_to_char( "Very generous of you, but you haven't got that much gold.\r\n", ch );
         return;
      }

      ch->gold -= amount;
      victim->gold += amount;
      strlcpy( buf, "$n gives you ", MAX_INPUT_LENGTH );
      strlcat( buf, arg1, MAX_INPUT_LENGTH );
      strlcat( buf, ( amount > 1 ) ? " coins." : " coin.", MAX_INPUT_LENGTH );

      set_char_color( AT_GOLD, victim );
      act( AT_ACTION, buf, ch, NULL, victim, TO_VICT );
      act( AT_ACTION, "$n gives $N some gold.", ch, NULL, victim, TO_NOTVICT );
      act( AT_ACTION, "You give $N some gold.", ch, NULL, victim, TO_CHAR );
      mprog_bribe_trigger( victim, ch, amount );
      if( IS_SET( sysdata.save_flags, SV_GIVE ) && !char_died( ch ) )
         save_char_obj( ch );
      if( IS_SET( sysdata.save_flags, SV_RECEIVE ) && !char_died( victim ) )
         save_char_obj( victim );
      return;
   }

   if( ( obj = get_obj_carry( ch, arg1 ) ) == NULL )
   {
      send_to_char( "You do not have that item.\r\n", ch );
      return;
   }

   if( obj->wear_loc != WEAR_NONE )
   {
      send_to_char( "You must remove it first.\r\n", ch );
      return;
   }

   if( ( victim = get_char_room( ch, arg2 ) ) == NULL )
   {
      send_to_char( "They aren't here.\r\n", ch );
      return;
   }

   if( !can_drop_obj( ch, obj ) )
   {
      send_to_char( "You can't let go of it.\r\n", ch );
      return;
   }

   /* Wowzers Mud: Prevent trading soulbound items -Hansth */
   if ( obj->soulbound && obj->soulbound[0] != '\0' && !IS_IMMORTAL(ch) )
   {
      ch_printf( ch, "You cannot trade %s because it is soulbound to you.\r\n", obj->short_descr );
      return;
   }

   if( victim->carry_number + ( get_obj_number( obj ) / obj->count ) > can_carry_n( victim ) )
   {
      act( AT_PLAIN, "$N has $S hands full.", ch, NULL, victim, TO_CHAR );
      return;
   }

   if( victim->carry_weight + ( get_obj_weight( obj ) / obj->count ) > can_carry_w( victim ) )
   {
      act( AT_PLAIN, "$N can't carry that much weight.", ch, NULL, victim, TO_CHAR );
      return;
   }

   if( !can_see_obj( victim, obj ) )
   {
      act( AT_PLAIN, "$N can't see it.", ch, NULL, victim, TO_CHAR );
      return;
   }

   if( IS_OBJ_STAT( obj, ITEM_PROTOTYPE ) && !can_take_proto( victim ) )
   {
      act( AT_PLAIN, "You cannot give that to $N!", ch, NULL, victim, TO_CHAR );
      return;
   }

   separate_obj( obj );
   obj_from_char( obj );
   act( AT_ACTION, "$n gives $p to $N.", ch, obj, victim, TO_NOTVICT );
   act( AT_ACTION, "$n gives you $p.", ch, obj, victim, TO_VICT );
   act( AT_ACTION, "You give $p to $N.", ch, obj, victim, TO_CHAR );
   obj = obj_to_char( obj, victim );
   mprog_give_trigger( victim, ch, obj );
   if( IS_SET( sysdata.save_flags, SV_GIVE ) && !char_died( ch ) )
      save_char_obj( ch );
   if( IS_SET( sysdata.save_flags, SV_RECEIVE ) && !char_died( victim ) )
      save_char_obj( victim );
}

/*
 * Damage an object.						-Thoric
 * Affect player's AC if necessary.
 * Make object into scraps if necessary.
 * Send message about damaged object.
 */
obj_ret damage_obj( OBJ_DATA * obj )
{
   CHAR_DATA *ch;
   obj_ret objcode;

   if( IS_OBJ_STAT( obj, ITEM_PERMANENT ) )
      return rNONE;

   ch = obj->carried_by;
   objcode = rNONE;

   separate_obj( obj );
   if( !IS_NPC( ch ) && ( !IS_PKILL( ch ) || ( IS_PKILL( ch ) && !IS_SET( ch->pcdata->flags, PCFLAG_GAG ) ) ) )
      act( AT_OBJECT, "($p gets damaged)", ch, obj, NULL, TO_CHAR );
   else if( obj->in_room && ( ch = obj->in_room->first_person ) != NULL )
   {
      act( AT_OBJECT, "($p gets damaged)", ch, obj, NULL, TO_ROOM );
      act( AT_OBJECT, "($p gets damaged)", ch, obj, NULL, TO_CHAR );
      ch = NULL;
   }

   if( obj->item_type != ITEM_LIGHT )
      oprog_damage_trigger( ch, obj );
   else if( !in_arena( ch ) )
      oprog_damage_trigger( ch, obj );

   if( obj_extracted( obj ) )
      return global_objcode;

   switch ( obj->item_type )
   {
      default:
         make_scraps( obj );
         objcode = rOBJ_SCRAPPED;
         break;
      case ITEM_CONTAINER:
      case ITEM_KEYRING:
      case ITEM_QUIVER:
         if( --obj->value[3] <= 0 )
         {
            if( !in_arena( ch ) )
            {
               make_scraps( obj );
               objcode = rOBJ_SCRAPPED;
            }
            else
               obj->value[3] = 1;
         }
         break;
      case ITEM_LIGHT:
         if( --obj->value[0] <= 0 )
         {
            if( !in_arena( ch ) )
            {
               make_scraps( obj );
               objcode = rOBJ_SCRAPPED;
            }
            else
               obj->value[0] = 1;
         }
         break;
      case ITEM_ARMOR:
         if( ch && obj->value[0] >= 1 )
            ch->armor += apply_ac( obj, obj->wear_loc );
         if( --obj->value[0] <= 0 )
         {
            if( !IS_PKILL( ch ) && !in_arena( ch ) )
            {
               make_scraps( obj );
               objcode = rOBJ_SCRAPPED;
            }
            else
            {
               obj->value[0] = 1;
               ch->armor -= apply_ac( obj, obj->wear_loc );
            }
         }
         else if( ch && obj->value[0] >= 1 )
            ch->armor -= apply_ac( obj, obj->wear_loc );
         break;
      case ITEM_WEAPON:
         if( --obj->value[0] <= 0 )
         {
            if( !IS_PKILL( ch ) && !in_arena( ch ) )
            {
               make_scraps( obj );
               objcode = rOBJ_SCRAPPED;
            }
            else
               obj->value[0] = 1;
         }
         break;
   }
   if( ch != NULL )
      save_char_obj( ch ); /* Stop scrap duping - Samson 1-2-00 */

   if( objcode == rOBJ_SCRAPPED && !IS_NPC( ch ) )
    	log_printf( "%s scrapped %s (vnum: %d)", ch->name, obj->short_descr, obj->pIndexData->vnum );

   return objcode;
}

/*
 * Remove an object.
 */
bool remove_obj( CHAR_DATA * ch, int iWear, bool fReplace )
{
   OBJ_DATA *obj, *tmpobj;

   if( ( obj = get_eq_char( ch, iWear ) ) == NULL )
      return TRUE;

   if( !fReplace && ch->carry_number + get_obj_number( obj ) > can_carry_n( ch ) )
   {
      act( AT_PLAIN, "$d: you can't carry that many items.", ch, NULL, obj->short_descr, TO_CHAR );
      return FALSE;
   }

   if( !fReplace )
      return FALSE;

   if( IS_OBJ_STAT( obj, ITEM_NOREMOVE ) )
   {
      act( AT_PLAIN, "You can't remove $p.", ch, obj, NULL, TO_CHAR );
      return FALSE;
   }

   if( obj == get_eq_char( ch, WEAR_WIELD ) && ( tmpobj = get_eq_char( ch, WEAR_DUAL_WIELD ) ) != NULL )
      tmpobj->wear_loc = WEAR_WIELD;

   unequip_char( ch, obj );

   act( AT_ACTION, "$n stops using $p.", ch, obj, NULL, TO_ROOM );
   act( AT_ACTION, "You stop using $p.", ch, obj, NULL, TO_CHAR );
   oprog_remove_trigger( ch, obj );
   /*
    * Added check in case, the trigger forces them to rewear the item
    * * --Shaddai
    */
   if( !( obj = get_eq_char( ch, iWear ) ) )
      return TRUE;
   else
      return FALSE;
}

/*
 * See if char could be capable of dual-wielding		-Thoric
 */
bool could_dual( CHAR_DATA * ch )
{
   if( IS_NPC( ch ) || ch->pcdata->learned[gsn_dual_wield] )
      return TRUE;

   return FALSE;
}

bool can_dual( CHAR_DATA * ch )
{
   bool wield = FALSE, nwield = FALSE;

   if( !could_dual( ch ) )
      return FALSE;
   if( get_eq_char( ch, WEAR_WIELD ) )
      wield = TRUE;
   /*
    * Check for missile wield or dual wield 
    */
   if( get_eq_char( ch, WEAR_MISSILE_WIELD ) || get_eq_char( ch, WEAR_DUAL_WIELD ) )
      nwield = TRUE;
   if( wield && nwield )
   {
      send_to_char( "You are already wielding two weapons... grow some more arms!\r\n", ch );
      return FALSE;
   }
   if( ( wield || nwield ) && get_eq_char( ch, WEAR_SHIELD ) )
   {
      send_to_char( "You cannot dual wield, you're already holding a shield!\r\n", ch );
      return FALSE;
   }
   if( ( wield || nwield ) && get_eq_char( ch, WEAR_HOLD ) )
   {
      send_to_char( "You cannot hold another weapon, you're already holding something in that hand!\r\n", ch );
      return FALSE;
   }
   return TRUE;
}

/*
 * Check to see if there is room to wear another object on this location
 * (Layered clothing support)
 */
bool can_layer( CHAR_DATA * ch, OBJ_DATA * obj, short wear_loc )
{
   OBJ_DATA *otmp;
   short bitlayers = 0;
   short objlayers = obj->pIndexData->layers;

   for( otmp = ch->first_carrying; otmp; otmp = otmp->next_content )
   {
      if( otmp->wear_loc == wear_loc )
      {
         if( !otmp->pIndexData->layers )
            return FALSE;
         else
            bitlayers |= otmp->pIndexData->layers;
      }
   }

   if( ( bitlayers && !objlayers ) || bitlayers > objlayers )
      return FALSE;
   if( !bitlayers || ( ( bitlayers & ~objlayers ) == bitlayers ) )
      return TRUE;
   return FALSE;
}

/*
 * Wear one object.
 * Optional replacement of existing objects.
 * Big repetitive code, ick.
 *
 * Restructured a bit to allow for specifying body location	-Thoric
 * & Added support for layering on certain body locations
 */
void wear_obj( CHAR_DATA * ch, OBJ_DATA * obj, bool fReplace, short wear_bit )
{
   OBJ_DATA *tmpobj = NULL;
   short bit, tmp;

   separate_obj( obj );

   /* ============================================
      Wowzers Mud: Soulbinding Wear Checks -Hansth
      ============================================ */
   if ( obj->soulbound && obj->soulbound[0] != '\0' && str_cmp( obj->soulbound, ch->name ) && !IS_IMMORTAL(ch) )
   {
      ch_printf( ch, "You cannot equip %s; it is soulbound to someone else.\r\n", obj->short_descr );
      return;
   }

   /* Bind on Equip (BoE) */
   if ( !IS_NPC( ch ) && IS_OBJ_STAT( obj, ITEM_BOE ) && ( !obj->soulbound || obj->soulbound[0] == '\0' ) )
   {
      obj->soulbound = STRALLOC( ch->name );
      act( AT_MAGIC, "$p binds to your soul as you equip it!", ch, obj, NULL, TO_CHAR );
   }

   if( get_trust( ch ) < obj->level )
   {
      ch_printf( ch, "You must be level %d to use this object.\r\n", obj->level );
      act( AT_ACTION, "$n tries to use $p, but is too inexperienced.", ch, obj, NULL, TO_ROOM );
      return;
   }

   if( !IS_IMMORTAL( ch )
       && ( ( IS_OBJ_STAT( obj, ITEM_ANTI_WARRIOR ) && ch->Class == CLASS_WARRIOR )
            || ( IS_OBJ_STAT( obj, ITEM_ANTI_WARRIOR ) && ch->Class == CLASS_PALADIN )
            || ( IS_OBJ_STAT( obj, ITEM_ANTI_MAGE ) && ch->Class == CLASS_MAGE )
            || ( IS_OBJ_STAT( obj, ITEM_ANTI_MAGE ) && ch->Class == CLASS_SHAMAN )
            || ( IS_OBJ_STAT( obj, ITEM_ANTI_THIEF ) && ch->Class == CLASS_ROGUE )
            || ( IS_OBJ_STAT( obj, ITEM_ANTI_DRUID ) && ch->Class == CLASS_DRUID )
            || ( IS_OBJ_STAT( obj, ITEM_ANTI_WARRIOR ) && ch->Class == CLASS_HUNTER )
            || ( IS_OBJ_STAT( obj, ITEM_ANTI_MAGE ) && ch->Class == CLASS_WARLOCK )
            || ( IS_OBJ_STAT( obj, ITEM_ANTI_CLERIC ) && ch->Class == CLASS_PRIEST )
            || ( IS_OBJ_STAT( obj, ITEM_ANTI_GOOD ) && ch->alignment > 350 )
            || ( IS_OBJ_STAT( obj, ITEM_ANTI_NEUTRAL ) && ch->alignment >= -350 && ch->alignment <= 350 )
            || ( IS_OBJ_STAT( obj, ITEM_ANTI_EVIL ) && ch->alignment < -350 ) ) )
   {
      act( AT_MAGIC, "You are forbidden to use that item.", ch, NULL, NULL, TO_CHAR );
      act( AT_ACTION, "$n tries to use $p, but is forbidden to do so.", ch, obj, NULL, TO_ROOM );
      return;
   }

   if( IS_OBJ_STAT( obj, ITEM_PERSONAL ) && str_cmp( ch->name, obj->owner ) )
   {
      send_to_char( "That item is personalized and belongs to someone else.\r\n", ch );
      if( obj->carried_by )
         obj_from_char( obj );
      obj_to_room( obj, ch->in_room );
      return;
   }

   if( wear_bit > -1 )
   {
      bit = wear_bit;
      if( !CAN_WEAR( obj, 1 << bit ) )
      {
         if( fReplace )
         {
            switch ( 1 << bit )
            {
               case ITEM_HOLD:
                  send_to_char( "You cannot hold that.\r\n", ch );
                  break;
               case ITEM_WIELD:
               case ITEM_MISSILE_WIELD:
                  send_to_char( "You cannot wield that.\r\n", ch );
                  break;
               default:
                  ch_printf( ch, "You cannot wear that on your %s.\r\n", w_flags[bit] );
                  break;
            }
         }
         return;
      }
   }
   else
   {
      for( bit = -1, tmp = 1; tmp < 31; tmp++ )
      {
         if( CAN_WEAR( obj, 1 << tmp ) )
         {
            bit = tmp;
            break;
         }
      }
   }

   /*
    * currently cannot have a light in non-light position 
    */
   if( obj->item_type == ITEM_LIGHT )
   {
      if( !remove_obj( ch, WEAR_LIGHT, fReplace ) )
         return;
      if( !oprog_use_trigger( ch, obj, NULL, NULL ) )
      {
         act( AT_ACTION, "$n holds $p as a light.", ch, obj, NULL, TO_ROOM );
         act( AT_ACTION, "You hold $p as your light.", ch, obj, NULL, TO_CHAR );
      }
      equip_char( ch, obj, WEAR_LIGHT );
      oprog_wear_trigger( ch, obj );
      return;
   }

   if( bit == -1 )
   {
      if( fReplace )
         send_to_char( "You can't wear, wield, or hold that.\r\n", ch );
      return;
   }

   switch ( 1 << bit )
   {
      default:
         bug( "%s: uknown/unused item_wear bit %d", __func__, bit );
         if( fReplace )
            send_to_char( "You can't wear, wield, or hold that.\r\n", ch );
         return;

      case ITEM_WEAR_FINGER:
         if( get_eq_char( ch, WEAR_FINGER_L )
             && get_eq_char( ch, WEAR_FINGER_R )
             && !remove_obj( ch, WEAR_FINGER_L, fReplace ) && !remove_obj( ch, WEAR_FINGER_R, fReplace ) )
            return;

         if( !get_eq_char( ch, WEAR_FINGER_L ) )
         {
            if( !oprog_use_trigger( ch, obj, NULL, NULL ) )
            {
               act( AT_ACTION, "$n slips $s left finger into $p.", ch, obj, NULL, TO_ROOM );
               act( AT_ACTION, "You slip your left finger into $p.", ch, obj, NULL, TO_CHAR );
            }
            equip_char( ch, obj, WEAR_FINGER_L );
            oprog_wear_trigger( ch, obj );
            return;
         }

         if( !get_eq_char( ch, WEAR_FINGER_R ) )
         {
            if( !oprog_use_trigger( ch, obj, NULL, NULL ) )
            {
               act( AT_ACTION, "$n slips $s right finger into $p.", ch, obj, NULL, TO_ROOM );
               act( AT_ACTION, "You slip your right finger into $p.", ch, obj, NULL, TO_CHAR );
            }
            equip_char( ch, obj, WEAR_FINGER_R );
            oprog_wear_trigger( ch, obj );
            return;
         }

         bug( "%s: no free finger.", __func__ );
         send_to_char( "You already wear something on both fingers.\r\n", ch );
         return;

      case ITEM_WEAR_NECK:
         if( get_eq_char( ch, WEAR_NECK_1 ) != NULL
             && get_eq_char( ch, WEAR_NECK_2 ) != NULL
             && !remove_obj( ch, WEAR_NECK_1, fReplace ) && !remove_obj( ch, WEAR_NECK_2, fReplace ) )
            return;

         if( !get_eq_char( ch, WEAR_NECK_1 ) )
         {
            if( !oprog_use_trigger( ch, obj, NULL, NULL ) )
            {
               act( AT_ACTION, "$n wears $p around $s neck.", ch, obj, NULL, TO_ROOM );
               act( AT_ACTION, "You wear $p around your neck.", ch, obj, NULL, TO_CHAR );
            }
            equip_char( ch, obj, WEAR_NECK_1 );
            oprog_wear_trigger( ch, obj );
            return;
         }

         if( !get_eq_char( ch, WEAR_NECK_2 ) )
         {
            if( !oprog_use_trigger( ch, obj, NULL, NULL ) )
            {
               act( AT_ACTION, "$n wears $p around $s neck.", ch, obj, NULL, TO_ROOM );
               act( AT_ACTION, "You wear $p around your neck.", ch, obj, NULL, TO_CHAR );
            }
            equip_char( ch, obj, WEAR_NECK_2 );
            oprog_wear_trigger( ch, obj );
            return;
         }

         bug( "%s: no free neck.", __func__ );
         send_to_char( "You already wear two neck items.\r\n", ch );
         return;

      case ITEM_WEAR_BODY:
         if( !can_layer( ch, obj, WEAR_BODY ) )
         {
            send_to_char( "It won't fit overtop of what you're already wearing.\r\n", ch );
            return;
         }
         if( !oprog_use_trigger( ch, obj, NULL, NULL ) )
         {
            act( AT_ACTION, "$n fits $p on $s body.", ch, obj, NULL, TO_ROOM );
            act( AT_ACTION, "You fit $p on your body.", ch, obj, NULL, TO_CHAR );
         }
         equip_char( ch, obj, WEAR_BODY );
         oprog_wear_trigger( ch, obj );
         return;

      case ITEM_WEAR_HEAD:
         if( !remove_obj( ch, WEAR_HEAD, fReplace ) )
            return;
         if( !oprog_use_trigger( ch, obj, NULL, NULL ) )
         {
            act( AT_ACTION, "$n dons $p upon $s head.", ch, obj, NULL, TO_ROOM );
            act( AT_ACTION, "You don $p upon your head.", ch, obj, NULL, TO_CHAR );
         }
         equip_char( ch, obj, WEAR_HEAD );
         oprog_wear_trigger( ch, obj );
         return;

      case ITEM_WEAR_EYES:
         if( !remove_obj( ch, WEAR_EYES, fReplace ) )
            return;
         if( !oprog_use_trigger( ch, obj, NULL, NULL ) )
         {
            act( AT_ACTION, "$n places $p on $s eyes.", ch, obj, NULL, TO_ROOM );
            act( AT_ACTION, "You place $p on your eyes.", ch, obj, NULL, TO_CHAR );
         }
         equip_char( ch, obj, WEAR_EYES );
         oprog_wear_trigger( ch, obj );
         return;

      case ITEM_WEAR_FACE:
         if( !remove_obj( ch, WEAR_FACE, fReplace ) )
            return;
         if( !oprog_use_trigger( ch, obj, NULL, NULL ) )
         {
            act( AT_ACTION, "$n places $p on $s face.", ch, obj, NULL, TO_ROOM );
            act( AT_ACTION, "You place $p on your face.", ch, obj, NULL, TO_CHAR );
         }
         equip_char( ch, obj, WEAR_FACE );
         oprog_wear_trigger( ch, obj );
         return;

      case ITEM_WEAR_EARS:
         if( !remove_obj( ch, WEAR_EARS, fReplace ) )
            return;
         if( !oprog_use_trigger( ch, obj, NULL, NULL ) )
         {
            act( AT_ACTION, "$n wears $p on $s ears.", ch, obj, NULL, TO_ROOM );
            act( AT_ACTION, "You wear $p on your ears.", ch, obj, NULL, TO_CHAR );
         }
         equip_char( ch, obj, WEAR_EARS );
         oprog_wear_trigger( ch, obj );
         return;

      case ITEM_WEAR_LEGS:
         if( !can_layer( ch, obj, WEAR_LEGS ) )
         {
            send_to_char( "It won't fit overtop of what you're already wearing.\r\n", ch );
            return;
         }
         if( !oprog_use_trigger( ch, obj, NULL, NULL ) )
         {
            act( AT_ACTION, "$n slips into $p.", ch, obj, NULL, TO_ROOM );
            act( AT_ACTION, "You slip into $p.", ch, obj, NULL, TO_CHAR );
         }
         equip_char( ch, obj, WEAR_LEGS );
         oprog_wear_trigger( ch, obj );
         return;

      case ITEM_WEAR_FEET:
         if( !can_layer( ch, obj, WEAR_FEET ) )
         {
            send_to_char( "It won't fit overtop of what you're already wearing.\r\n", ch );
            return;
         }
         if( !oprog_use_trigger( ch, obj, NULL, NULL ) )
         {
            act( AT_ACTION, "$n wears $p on $s feet.", ch, obj, NULL, TO_ROOM );
            act( AT_ACTION, "You wear $p on your feet.", ch, obj, NULL, TO_CHAR );
         }
         equip_char( ch, obj, WEAR_FEET );
         oprog_wear_trigger( ch, obj );
         return;

      case ITEM_WEAR_HANDS:
         if( !can_layer( ch, obj, WEAR_HANDS ) )
         {
            send_to_char( "It won't fit overtop of what you're already wearing.\r\n", ch );
            return;
         }
         if( !oprog_use_trigger( ch, obj, NULL, NULL ) )
         {
            act( AT_ACTION, "$n wears $p on $s hands.", ch, obj, NULL, TO_ROOM );
            act( AT_ACTION, "You wear $p on your hands.", ch, obj, NULL, TO_CHAR );
         }
         equip_char( ch, obj, WEAR_HANDS );
         oprog_wear_trigger( ch, obj );
         return;

      case ITEM_WEAR_ARMS:
         if( !can_layer( ch, obj, WEAR_ARMS ) )
         {
            send_to_char( "It won't fit overtop of what you're already wearing.\r\n", ch );
            return;
         }
         if( !oprog_use_trigger( ch, obj, NULL, NULL ) )
         {
            act( AT_ACTION, "$n wears $p on $s arms.", ch, obj, NULL, TO_ROOM );
            act( AT_ACTION, "You wear $p on your arms.", ch, obj, NULL, TO_CHAR );
         }
         equip_char( ch, obj, WEAR_ARMS );
         oprog_wear_trigger( ch, obj );
         return;

      case ITEM_WEAR_ABOUT:
         if( !can_layer( ch, obj, WEAR_ABOUT ) )
         {
            send_to_char( "It won't fit overtop of what you're already wearing.\r\n", ch );
            return;
         }
         if( !oprog_use_trigger( ch, obj, NULL, NULL ) )
         {
            act( AT_ACTION, "$n wears $p about $s body.", ch, obj, NULL, TO_ROOM );
            act( AT_ACTION, "You wear $p about your body.", ch, obj, NULL, TO_CHAR );
         }
         equip_char( ch, obj, WEAR_ABOUT );
         oprog_wear_trigger( ch, obj );
         return;

      case ITEM_WEAR_BACK:
         if( !remove_obj( ch, WEAR_BACK, fReplace ) )
            return;
         if( !oprog_use_trigger( ch, obj, NULL, NULL ) )
         {
            act( AT_ACTION, "$n slings $p on $s back.", ch, obj, NULL, TO_ROOM );
            act( AT_ACTION, "You sling $p on your back.", ch, obj, NULL, TO_CHAR );
         }
         equip_char( ch, obj, WEAR_BACK );
         oprog_wear_trigger( ch, obj );
         return;

      case ITEM_WEAR_WAIST:
         if( !can_layer( ch, obj, WEAR_WAIST ) )
         {
            send_to_char( "It won't fit overtop of what you're already wearing.\r\n", ch );
            return;
         }
         if( !oprog_use_trigger( ch, obj, NULL, NULL ) )
         {
            act( AT_ACTION, "$n wears $p about $s waist.", ch, obj, NULL, TO_ROOM );
            act( AT_ACTION, "You wear $p about your waist.", ch, obj, NULL, TO_CHAR );
         }
         equip_char( ch, obj, WEAR_WAIST );
         oprog_wear_trigger( ch, obj );
         return;

      case ITEM_WEAR_WRIST:
         if( get_eq_char( ch, WEAR_WRIST_L )
             && get_eq_char( ch, WEAR_WRIST_R )
             && !remove_obj( ch, WEAR_WRIST_L, fReplace ) && !remove_obj( ch, WEAR_WRIST_R, fReplace ) )
            return;

         if( !get_eq_char( ch, WEAR_WRIST_L ) )
         {
            if( !oprog_use_trigger( ch, obj, NULL, NULL ) )
            {
               act( AT_ACTION, "$n fits $p around $s left wrist.", ch, obj, NULL, TO_ROOM );
               act( AT_ACTION, "You fit $p around your left wrist.", ch, obj, NULL, TO_CHAR );
            }
            equip_char( ch, obj, WEAR_WRIST_L );
            oprog_wear_trigger( ch, obj );
            return;
         }

         if( !get_eq_char( ch, WEAR_WRIST_R ) )
         {
            if( !oprog_use_trigger( ch, obj, NULL, NULL ) )
            {
               act( AT_ACTION, "$n fits $p around $s right wrist.", ch, obj, NULL, TO_ROOM );
               act( AT_ACTION, "You fit $p around your right wrist.", ch, obj, NULL, TO_CHAR );
            }
            equip_char( ch, obj, WEAR_WRIST_R );
            oprog_wear_trigger( ch, obj );
            return;
         }

         bug( "%s: no free wrist.", __func__ );
         send_to_char( "You already wear two wrist items.\r\n", ch );
         return;

      case ITEM_WEAR_ANKLE:
         if( get_eq_char( ch, WEAR_ANKLE_L )
             && get_eq_char( ch, WEAR_ANKLE_R )
             && !remove_obj( ch, WEAR_ANKLE_L, fReplace ) && !remove_obj( ch, WEAR_ANKLE_R, fReplace ) )
            return;

         if( !get_eq_char( ch, WEAR_ANKLE_L ) )
         {
            if( !oprog_use_trigger( ch, obj, NULL, NULL ) )
            {
               act( AT_ACTION, "$n fits $p around $s left ankle.", ch, obj, NULL, TO_ROOM );
               act( AT_ACTION, "You fit $p around your left ankle.", ch, obj, NULL, TO_CHAR );
            }
            equip_char( ch, obj, WEAR_ANKLE_L );
            oprog_wear_trigger( ch, obj );
            return;
         }

         if( !get_eq_char( ch, WEAR_ANKLE_R ) )
         {
            if( !oprog_use_trigger( ch, obj, NULL, NULL ) )
            {
               act( AT_ACTION, "$n fits $p around $s right ankle.", ch, obj, NULL, TO_ROOM );
               act( AT_ACTION, "You fit $p around your right ankle.", ch, obj, NULL, TO_CHAR );
            }
            equip_char( ch, obj, WEAR_ANKLE_R );
            oprog_wear_trigger( ch, obj );
            return;
         }

         bug( "%s: no free ankle.", __func__ );
         send_to_char( "You already wear two ankle items.\r\n", ch );
         return;

      case ITEM_WEAR_SHIELD:
         if( get_eq_char( ch, WEAR_DUAL_WIELD )
             || ( get_eq_char( ch, WEAR_WIELD ) && get_eq_char( ch, WEAR_MISSILE_WIELD ) )
             || ( get_eq_char( ch, WEAR_WIELD ) && get_eq_char( ch, WEAR_HOLD ) ) )
         {
            if( get_eq_char( ch, WEAR_HOLD ) )
               send_to_char( "You can't use a shield while using a weapon and holding something!\r\n", ch );
            else
               send_to_char( "You can't use a shield AND two weapons!\r\n", ch );
            return;
         }
         if( !remove_obj( ch, WEAR_SHIELD, fReplace ) )
            return;
         if( !oprog_use_trigger( ch, obj, NULL, NULL ) )
         {
            act( AT_ACTION, "$n uses $p as a shield.", ch, obj, NULL, TO_ROOM );
            act( AT_ACTION, "You use $p as a shield.", ch, obj, NULL, TO_CHAR );
         }
         equip_char( ch, obj, WEAR_SHIELD );
         oprog_wear_trigger( ch, obj );
         return;

      case ITEM_MISSILE_WIELD:
      case ITEM_WIELD:
         if( !could_dual( ch ) )
         {
            if( !remove_obj( ch, WEAR_MISSILE_WIELD, fReplace ) )
               return;
            if( !remove_obj( ch, WEAR_WIELD, fReplace ) )
               return;
            tmpobj = NULL;
         }
         else
         {
            OBJ_DATA *mw, *dw, *hd, *sd;
            tmpobj = get_eq_char( ch, WEAR_WIELD );
            mw = get_eq_char( ch, WEAR_MISSILE_WIELD );
            dw = get_eq_char( ch, WEAR_DUAL_WIELD );
            hd = get_eq_char( ch, WEAR_HOLD );
            sd = get_eq_char( ch, WEAR_SHIELD );

            if( hd && sd )
            {
               send_to_char( "You are already holding something and wearing a shield.\r\n", ch );
               return;
            }

            if( tmpobj )
            {
               if( !can_dual( ch ) )
                  return;

//               if( get_obj_weight( obj ) + get_obj_weight( tmpobj ) > str_app[get_curr_str( ch )].wield )
//               {
//                  send_to_char( "It is too heavy for you to wield.\r\n", ch );
//                  return;
//               }

               if( mw || dw )
               {
                  send_to_char( "You're already wielding two weapons.\r\n", ch );
                  return;
               }

               if( hd || sd )
               {
                  send_to_char( "You're already wielding a weapon AND holding something.\r\n", ch );
                  return;
               }

               if( !oprog_use_trigger( ch, obj, NULL, NULL ) )
               {
                  act( AT_ACTION, "$n dual-wields $p.", ch, obj, NULL, TO_ROOM );
                  act( AT_ACTION, "You dual-wield $p.", ch, obj, NULL, TO_CHAR );
               }
               if( 1 << bit == ITEM_MISSILE_WIELD )
                  equip_char( ch, obj, WEAR_MISSILE_WIELD );
               else
                  equip_char( ch, obj, WEAR_DUAL_WIELD );
               oprog_wear_trigger( ch, obj );
               return;
            }

            if( mw )
            {
               if( !can_dual( ch ) )
                  return;

               if( 1 << bit == ITEM_MISSILE_WIELD )
               {
                  send_to_char( "You're already wielding a missile weapon.\r\n", ch );
                  return;
               }

//               if( get_obj_weight( obj ) + get_obj_weight( mw ) > str_app[get_curr_str( ch )].wield )
//               {
//                  send_to_char( "It is too heavy for you to wield.\r\n", ch );
//                  return;
//               }

               if( tmpobj || dw )
               {
                  send_to_char( "You're already wielding two weapons.\r\n", ch );
                  return;
               }

               if( hd || sd )
               {
                  send_to_char( "You're already wielding a weapon AND holding something.\r\n", ch );
                  return;
               }

               if( !oprog_use_trigger( ch, obj, NULL, NULL ) )
               {
                  act( AT_ACTION, "$n wields $p.", ch, obj, NULL, TO_ROOM );
                  act( AT_ACTION, "You wield $p.", ch, obj, NULL, TO_CHAR );
               }
               equip_char( ch, obj, WEAR_WIELD );
               oprog_wear_trigger( ch, obj );
               return;
            }
         }

//         if( get_obj_weight( obj ) > str_app[get_curr_str( ch )].wield )
//         {
//            send_to_char( "It is too heavy for you to wield.\r\n", ch );
//            return;
//         }

         if( !oprog_use_trigger( ch, obj, NULL, NULL ) )
         {
            act( AT_ACTION, "$n wields $p.", ch, obj, NULL, TO_ROOM );
            act( AT_ACTION, "You wield $p.", ch, obj, NULL, TO_CHAR );
         }
         if( 1 << bit == ITEM_MISSILE_WIELD )
            equip_char( ch, obj, WEAR_MISSILE_WIELD );
         else
            equip_char( ch, obj, WEAR_WIELD );
         oprog_wear_trigger( ch, obj );
         return;

      case ITEM_HOLD:
         if( get_eq_char( ch, WEAR_DUAL_WIELD )
             || ( get_eq_char( ch, WEAR_WIELD )
                  && ( get_eq_char( ch, WEAR_MISSILE_WIELD ) || get_eq_char( ch, WEAR_SHIELD ) ) ) )
         {
            if( get_eq_char( ch, WEAR_SHIELD ) )
               send_to_char( "You cannot hold something while using a weapon and a shield!\r\n", ch );
            else
               send_to_char( "You cannot hold something AND two weapons!\r\n", ch );
            return;
         }
         if( !remove_obj( ch, WEAR_HOLD, fReplace ) )
            return;
         if( obj->item_type == ITEM_WAND
             || obj->item_type == ITEM_STAFF
             || obj->item_type == ITEM_FOOD
             || obj->item_type == ITEM_COOK
             || obj->item_type == ITEM_PILL
             || obj->item_type == ITEM_POTION
             || obj->item_type == ITEM_SCROLL
             || obj->item_type == ITEM_DRINK_CON
             || obj->item_type == ITEM_BLOOD
             || obj->item_type == ITEM_PIPE
             || obj->item_type == ITEM_HERB || obj->item_type == ITEM_KEY || !oprog_use_trigger( ch, obj, NULL, NULL ) )
         {
            act( AT_ACTION, "$n holds $p in $s hands.", ch, obj, NULL, TO_ROOM );
            act( AT_ACTION, "You hold $p in your hands.", ch, obj, NULL, TO_CHAR );
         }
         equip_char( ch, obj, WEAR_HOLD );
         oprog_wear_trigger( ch, obj );
         return;
   }
}

void do_wear( CHAR_DATA* ch, const char* argument )
{
   char arg1[MAX_INPUT_LENGTH];
   char arg2[MAX_INPUT_LENGTH];
   OBJ_DATA *obj;
   short wear_bit;

   argument = one_argument( argument, arg1 );
   argument = one_argument( argument, arg2 );
   if( ( !str_cmp( arg2, "on" ) || !str_cmp( arg2, "upon" ) || !str_cmp( arg2, "around" ) ) && argument[0] != '\0' )
      argument = one_argument( argument, arg2 );

   if( arg1[0] == '\0' )
   {
      send_to_char( "Wear, wield, or hold what?\r\n", ch );
      return;
   }

   if( ms_find_obj( ch ) )
      return;

   if( !str_cmp( arg1, "all" ) )
   {
      OBJ_DATA *obj_next;

      for( obj = ch->first_carrying; obj; obj = obj_next )
      {
         obj_next = obj->next_content;
         if( obj->wear_loc == WEAR_NONE && can_see_obj( ch, obj ) )
         {
            wear_obj( ch, obj, FALSE, -1 );
            if( char_died( ch ) )
               return;
         }
      }
      return;
   }
   else
   {
      if( ( obj = get_obj_carry( ch, arg1 ) ) == NULL )
      {
         send_to_char( "You do not have that item.\r\n", ch );
         return;
      }
      if( arg2[0] != '\0' )
         wear_bit = get_wflag( arg2 );
      else
         wear_bit = -1;
      wear_obj( ch, obj, TRUE, wear_bit );
   }
}

void do_remove( CHAR_DATA* ch, const char* argument )
{
   char arg[MAX_INPUT_LENGTH];
   OBJ_DATA *obj, *obj_next;


   one_argument( argument, arg );

   if( arg[0] == '\0' )
   {
      send_to_char( "Remove what?\r\n", ch );
      return;
   }

   if( ms_find_obj( ch ) )
      return;

   if( !str_cmp( arg, "all" ) )  /* SB Remove all */
   {
      for( obj = ch->first_carrying; obj != NULL; obj = obj_next )
      {
         obj_next = obj->next_content;
         if( obj->wear_loc != WEAR_NONE && can_see_obj( ch, obj ) )
            remove_obj( ch, obj->wear_loc, TRUE );
      }
      return;
   }

   if( ( obj = get_obj_wear( ch, arg ) ) == NULL )
   {
      send_to_char( "You are not using that item.\r\n", ch );
      return;
   }
   if( ( obj_next = get_eq_char( ch, obj->wear_loc ) ) != obj )
   {
      act( AT_PLAIN, "You must remove $p first.", ch, obj_next, NULL, TO_CHAR );
      return;
   }

   remove_obj( ch, obj->wear_loc, TRUE );
}

void do_bury( CHAR_DATA* ch, const char* argument )
{
   char arg[MAX_INPUT_LENGTH];
   OBJ_DATA *obj;
   bool shovel;
   short move;

   one_argument( argument, arg );

   if( arg[0] == '\0' )
   {
      send_to_char( "What do you wish to bury?\r\n", ch );
      return;
   }

   if( ms_find_obj( ch ) )
      return;

   shovel = FALSE;
   for( obj = ch->first_carrying; obj; obj = obj->next_content )
      if( obj->item_type == ITEM_SHOVEL )
      {
         shovel = TRUE;
         break;
      }

   obj = get_obj_list_rev( ch, arg, ch->in_room->last_content );
   if( !obj )
   {
      send_to_char( "You can't find it.\r\n", ch );
      return;
   }

   separate_obj( obj );

   if( !CAN_WEAR( obj, ITEM_TAKE ) )
   {
      act( AT_PLAIN, "You cannot bury $p.", ch, obj, NULL, TO_CHAR );
      return;
   }

   switch( ch->in_room->sector_type )
   {
      case SECT_CITY:
      case SECT_INSIDE:
         send_to_char( "The floor is too hard to dig through.\r\n", ch );
         return;
      case SECT_WATER_SWIM:
      case SECT_WATER_NOSWIM:
      case SECT_UNDERWATER:
         send_to_char( "You cannot bury something here.\r\n", ch );
         return;
      case SECT_AIR:
         send_to_char( "What?  In the air?!\r\n", ch );
         return;
   }

   if( obj->weight > ( UMAX( 5, ( can_carry_w( ch ) / 10 ) ) ) && !shovel )
   {
      send_to_char( "You'd need a shovel to bury something that big.\r\n", ch );
      return;
   }

   move = ( obj->weight * 50 * ( shovel ? 1 : 5 ) ) / UMAX( 1, can_carry_w( ch ) );
   move = URANGE( 2, move, 1000 );
   if( move > ch->move )
   {
      send_to_char( "You don't have the energy to bury something of that size.\r\n", ch );
      return;
   }
   ch->move -= move;
   if( obj->item_type == ITEM_CORPSE_NPC || obj->item_type == ITEM_CORPSE_PC )
      adjust_favor( ch, 6, 1 );

   act( AT_ACTION, "You solemnly bury $p...", ch, obj, NULL, TO_CHAR );
   act( AT_ACTION, "$n solemnly buries $p...", ch, obj, NULL, TO_ROOM );
   xSET_BIT( obj->extra_flags, ITEM_BURIED );
   WAIT_STATE( ch, URANGE( 10, move / 2, 100 ) );
}

void do_sacrifice( CHAR_DATA* ch, const char* argument )
{
   char arg[MAX_INPUT_LENGTH];
   char buf[MAX_STRING_LENGTH];
   char name[50];
   OBJ_DATA *obj;

   one_argument( argument, arg );

   if( arg[0] == '\0' || !str_cmp( arg, ch->name ) )
   {
      act( AT_ACTION, "$n offers $mself to $s deity, who graciously declines.", ch, NULL, NULL, TO_ROOM );
      send_to_char( "Your deity appreciates your offer and may accept it later.\r\n", ch );
      return;
   }

   if( ms_find_obj( ch ) )
      return;

   obj = get_obj_list_rev( ch, arg, ch->in_room->last_content );
   if( !obj )
   {
      send_to_char( "You can't find it.\r\n", ch );
      return;
   }

   separate_obj( obj );

   if( !CAN_WEAR( obj, ITEM_TAKE ) )
   {
      act( AT_PLAIN, "$p is not an acceptable sacrifice.", ch, obj, NULL, TO_CHAR );
      return;
   }

   if( xIS_SET( ch->in_room->room_flags, ROOM_CLANSTOREROOM ) )
   {
      send_to_char( "The gods would not accept such a foolish sacrifice.\r\n", ch );
      return;
   }

   if( IS_SET( obj->magic_flags, ITEM_PKDISARMED ) && !IS_NPC( ch ) && !IS_IMMORTAL( ch ) )
   {
      if( CAN_PKILL( ch ) && !get_timer( ch, TIMER_PKILLED ) )
      {
         if( ch->level - obj->value[5] > 5 || obj->value[5] - ch->level > 5 )
         {
            send_to_char_color( "\r\n&bA godly force freezes your outstretched hand.\r\n", ch );
            return;
         }
      }
   }
   if( !IS_NPC( ch ) && ch->pcdata->deity && ch->pcdata->deity->name[0] != '\0' )
   {
      strlcpy( name, ch->pcdata->deity->name, 50 );
   }
   else if( !IS_NPC( ch ) && IS_GUILDED( ch ) && sysdata.guild_overseer[0] != '\0' )
   {
      strlcpy( name, sysdata.guild_overseer, 50 );
   }
   else if( !IS_NPC( ch ) && ch->pcdata->clan && ch->pcdata->clan->deity[0] != '\0' )
   {
      strlcpy( name, ch->pcdata->clan->deity, 50 );
   }
   else
   {
      strlcpy( name, "Thoric", 50 );
   }
   ch->gold += 1;
   if( obj->item_type == ITEM_CORPSE_NPC || obj->item_type == ITEM_CORPSE_PC )
      adjust_favor( ch, 5, 1 );
   ch_printf( ch, "%s gives you one gold coin for your sacrifice.\r\n", name );

   if( obj->item_type == ITEM_PAPER )
	   snprintf( buf, MAX_STRING_LENGTH, "$n sacrifices a note to %s.", name );
   else
      snprintf( buf, MAX_STRING_LENGTH, "$n sacrifices $p to %s.", name );
   act( AT_ACTION, buf, ch, obj, NULL, TO_ROOM );

   if( obj->item_type != ITEM_PAPER )
      oprog_sac_trigger( ch, obj );

   if( obj_extracted( obj ) )
   {
      if( xIS_SET( ch->in_room->room_flags, ROOM_HOUSE ) )
         save_house_by_vnum( ch->in_room->vnum ); /* Prevent House Object Duplication */
      return;
   }

   if( cur_obj == obj->serial )
      global_objcode = rOBJ_SACCED;
   /* Separate again.  There was a problem here with sac_progs in that if the
      object respawned a copy of itself, it would sometimes link it to the
      one that was being extracted, resulting in them both getting that evil
      extraction :) -- Alty */
   separate_obj( obj );
   extract_obj( obj );

   if( xIS_SET( ch->in_room->room_flags, ROOM_HOUSE ) )
      save_house_by_vnum( ch->in_room->vnum ); /* Prevent House Object Duplication */
}

void do_brandish( CHAR_DATA* ch, const char* argument )
{
   CHAR_DATA *vch;
   CHAR_DATA *vch_next;
   OBJ_DATA *staff;
   ch_ret retcode;
   int sn;

   if( ( staff = get_eq_char( ch, WEAR_HOLD ) ) == NULL )
   {
      send_to_char( "You hold nothing in your hand.\r\n", ch );
      return;
   }

   if( staff->item_type != ITEM_STAFF )
   {
      send_to_char( "You can brandish only with a staff.\r\n", ch );
      return;
   }

   if( ( sn = staff->value[3] ) < 0 || sn >= num_skills || skill_table[sn]->spell_fun == NULL )
   {
      bug( "%s: bad sn %d.", __func__, sn );
      return;
   }

   WAIT_STATE( ch, 2 * PULSE_VIOLENCE );

   if( staff->value[2] > 0 )
   {
      if( !oprog_use_trigger( ch, staff, NULL, NULL ) )
      {
         act( AT_MAGIC, "$n brandishes $p.", ch, staff, NULL, TO_ROOM );
         act( AT_MAGIC, "You brandish $p.", ch, staff, NULL, TO_CHAR );
      }

      for( vch = ch->in_room->first_person; vch; vch = vch_next )
      {
         vch_next = vch->next_in_room;

         if( !IS_NPC( vch ) && xIS_SET( vch->act, PLR_WIZINVIS ) && vch->pcdata->wizinvis >= LEVEL_IMMORTAL )
            continue;
         else
            switch ( skill_table[sn]->target )
            {
               default:
                  bug( "%s: bad target for sn %d.", __func__, sn );
                  return;

               case TAR_IGNORE:
                  if( vch != ch )
                     continue;
                  break;

               case TAR_CHAR_OFFENSIVE:
                  if( IS_NPC( ch ) ? IS_NPC( vch ) : !IS_NPC( vch ) )
                     continue;
                  break;

               case TAR_CHAR_DEFENSIVE:
                  if( IS_NPC( ch ) ? !IS_NPC( vch ) : IS_NPC( vch ) )
                     continue;
                  break;

               case TAR_CHAR_SELF:
                  if( vch != ch )
                     continue;
                  break;
            }

         retcode = obj_cast_spell( staff->value[3], staff->value[0], ch, vch, NULL );
         if( retcode == rCHAR_DIED || retcode == rBOTH_DIED )
         {
            bug( "%s: char died", __func__ );
            return;
         }
      }
   }

   if( --staff->value[2] <= 0 )
   {
      act( AT_MAGIC, "$p blazes bright and vanishes from $n's hands!", ch, staff, NULL, TO_ROOM );
      act( AT_MAGIC, "$p blazes bright and is gone!", ch, staff, NULL, TO_CHAR );
      if( staff->serial == cur_obj )
         global_objcode = rOBJ_USED;
      extract_obj( staff );
   }
}

void do_zap( CHAR_DATA* ch, const char* argument )
{
   char arg[MAX_INPUT_LENGTH];
   CHAR_DATA *victim;
   OBJ_DATA *wand;
   OBJ_DATA *obj;
   ch_ret retcode;

   one_argument( argument, arg );
   if( arg[0] == '\0' && !ch->fighting )
   {
      send_to_char( "Zap whom or what?\r\n", ch );
      return;
   }

   if( ( wand = get_eq_char( ch, WEAR_HOLD ) ) == NULL )
   {
      send_to_char( "You hold nothing in your hand.\r\n", ch );
      return;
   }

   if( wand->item_type != ITEM_WAND )
   {
      send_to_char( "You can zap only with a wand.\r\n", ch );
      return;
   }

   obj = NULL;
   if( arg[0] == '\0' )
   {
      if( ch->fighting )
      {
         victim = who_fighting( ch );
      }
      else
      {
         send_to_char( "Zap whom or what?\r\n", ch );
         return;
      }
   }
   else
   {
      if( ( victim = get_char_room( ch, arg ) ) == NULL && ( obj = get_obj_here( ch, arg ) ) == NULL )
      {
         send_to_char( "You can't find it.\r\n", ch );
         return;
      }
   }

   WAIT_STATE( ch, 1 * PULSE_VIOLENCE );

   if( wand->value[2] > 0 )
   {
      if( victim )
      {
         if( !oprog_use_trigger( ch, wand, victim, NULL ) )
         {
            act( AT_MAGIC, "$n aims $p at $N.", ch, wand, victim, TO_ROOM );
            act( AT_MAGIC, "You aim $p at $N.", ch, wand, victim, TO_CHAR );
         }
      }
      else
      {
         if( !oprog_use_trigger( ch, wand, NULL, obj ) )
         {
            act( AT_MAGIC, "$n aims $p at $P.", ch, wand, obj, TO_ROOM );
            act( AT_MAGIC, "You aim $p at $P.", ch, wand, obj, TO_CHAR );
         }
      }

      retcode = obj_cast_spell( wand->value[3], wand->value[0], ch, victim, obj );
      if( retcode == rCHAR_DIED || retcode == rBOTH_DIED )
      {
         bug( "%s: char died", __func__ );
         return;
      }
   }

   if( --wand->value[2] <= 0 )
   {
      act( AT_MAGIC, "$p explodes into fragments.", ch, wand, NULL, TO_ROOM );
      act( AT_MAGIC, "$p explodes into fragments.", ch, wand, NULL, TO_CHAR );
      if( wand->serial == cur_obj )
         global_objcode = rOBJ_USED;
      extract_obj( wand );
   }
}

/* ============================================
   Wowzers Mud: PATCH 4.18.3 - The Global Auction House
   Replaces legacy synchronous FUSS auction channel. -Hansth
   ============================================ */
void do_auction( CHAR_DATA *ch, const char *argument )
{
   char arg1[MAX_INPUT_LENGTH];
   char arg2[MAX_INPUT_LENGTH];
   char arg3[MAX_INPUT_LENGTH];
   char arg4[MAX_INPUT_LENGTH];
   AH_DATA *ah;
   OBJ_DATA *obj;

   if ( IS_NPC( ch ) )
      return;

   argument = one_argument( argument, arg1 );

   if ( arg1[0] == '\0' )
   {
      send_to_char( "&WAuction House Commands:\r\n", ch );
      send_to_char( "&Yauction list&w                             - View all active auctions\r\n", ch );
      send_to_char( "&Yauction sell <item> <start_bid> [buyout]&w - List an item for 24 hours\r\n", ch );
      return;
   }

   /* ============================================
      Command: AUCTION LIST -Hansth
      ============================================ */
   if ( !str_cmp( arg1, "list" ) )
   {
      int count = 0;

      if ( !first_ah )
      {
         send_to_char( "The Auction House is currently empty.\r\n", ch );
         return;
      }

      send_to_char( "&WID   Seller         Item                               Bid      Buyout&w\r\n", ch );
      send_to_char( "&W----------------------------------------------------------------------&w\r\n", ch );

      /* ============================================
         Wowzers Mud: Display Auction List (Gold Only) -Hansth
         ============================================ */
      for ( ah = first_ah; ah; ah = ah->next )
      {
         count++;
         ch_printf( ch, "&W%3d> &Y%-14s &G%-33s &Y%-8d &Y%-8d&w\r\n",
            count,
            ah->seller,
            ah->item ? ah->item->short_descr : "ERROR: NO ITEM",
            ah->bid,
            ah->buyout );
      }
      return;
   }

/* ============================================
      Wowzers Mud: AUCTION BUYOUT <ID> -Hansth
      ============================================ */
   if ( !str_cmp( arg1, "buyout" ) )
   {
      int id, count = 0;
      bool found = FALSE;
      /* Declare the variables the compiler is missing! -Hansth */
      char subj_buf[MAX_STRING_LENGTH];
      char body_buf[MAX_STRING_LENGTH];
      char item_name[MAX_STRING_LENGTH];
      OBJ_DATA *temp_item = NULL;

      one_argument( argument, arg2 );

      if ( arg2[0] == '\0' || !is_number( arg2 ) )
      {
         send_to_char( "Syntax: auction buyout <ID number>\r\n", ch );
         return;
      }

      id = atoi( arg2 );
      /* ... the rest of your buyout code ... */
      for ( ah = first_ah; ah; ah = ah->next )
      {
         if ( ++count == id )
         {
            found = TRUE;
            break;
         }
      }

      if ( !found )
      {
         send_to_char( "There is no auction listed under that ID.\r\n", ch );
         return;
      }

      if ( ah->buyout <= 0 )
      {
         send_to_char( "That item does not have a buyout price. You must place a bid.\r\n", ch );
         return;
      }

      if ( !str_cmp( ch->name, ah->seller ) )
      {
         send_to_char( "You cannot buy out your own auction!\r\n", ch );
         return;
      }

      if ( ch->gold < ah->buyout )
      {
         send_to_char( "You do not have enough gold to buy that.\r\n", ch );
         return;
      }

      /* ============================================
         Wowzers Mud: The Transaction -Hansth
         ============================================ */
      temp_item = ah->item; /* Move item to temp pointer first */
      
      if ( !temp_item )
      {
         bug( "%s: NULL item on auction ID %d", __func__, id );
         send_to_char( "Error: This item no longer exists.\r\n", ch );
         return;
      }

      strlcpy( item_name, temp_item->short_descr, MAX_STRING_LENGTH );

      /* Subtract Gold -Hansth */
      ch->gold -= ah->buyout;

      /* Deliver Item to Buyer -Hansth */
      snprintf( subj_buf, MAX_STRING_LENGTH, "Auction Won: %s", item_name );
      snprintf( body_buf, MAX_STRING_LENGTH, "You purchased %s from the Auction House.", item_name );
      ah_mail_delivery( "Auction House", ch->name, temp_item, 0, subj_buf, body_buf );

      /* Deliver Gold to Seller -Hansth */
      snprintf( subj_buf, MAX_STRING_LENGTH, "Auction Sold: %s", item_name );
      snprintf( body_buf, MAX_STRING_LENGTH, "Your auction for %s has sold for %d gold.", item_name, ah->buyout );
      ah_mail_delivery( "Auction House", ah->seller, NULL, ah->buyout, subj_buf, body_buf );

      /* Cleanup the AH structure -Hansth */
      UNLINK( ah, first_ah, last_ah, next, prev );
      if ( ah->seller ) STRFREE( ah->seller );
      if ( ah->buyer )  STRFREE( ah->buyer );
      
      DISPOSE( ah );
      save_auctions();
      
      ch_printf( ch, "You have purchased &G%s&w! Check your mail.\r\n", item_name );
      return;
   }

   /* ============================================
      Command: AUCTION SELL <item> <bid> <buyout> -Hansth
      ============================================ */
   if ( !str_cmp( arg1, "sell" ) )
   {
      int bid = 0;
      int buyout = 0;

      argument = one_argument( argument, arg2 ); /* Item */
      argument = one_argument( argument, arg3 ); /* Bid */
      argument = one_argument( argument, arg4 ); /* Buyout (Optional) */

      if ( arg2[0] == '\0' || arg3[0] == '\0' )
      {
         send_to_char( "Syntax: auction sell <item> <starting_bid> [buyout]\r\n", ch );
         return;
      }

      if ( ( obj = get_obj_carry( ch, arg2 ) ) == NULL )
      {
         send_to_char( "You are not carrying that item.\r\n", ch );
         return;
      }

      /* Patch 4.18.1: Soulbinding Roadblocks! -Hansth */ 
      if ( obj->soulbound && obj->soulbound[0] != '\0' )
      {
         ch_printf( ch, "You cannot auction %s; it is soulbound.\r\n", obj->short_descr );
         return;
      }

      bid = atoi( arg3 );
      buyout = atoi( arg4 );

      if ( bid <= 0 )
      {
         send_to_char( "The starting bid must be greater than 0 copper.\r\n", ch );
         return;
      }

      if ( buyout > 0 && buyout <= bid )
      {
         send_to_char( "The buyout price must be higher than the starting bid.\r\n", ch );
         return;
      }

      /* Take the item from the player and put it into the AH Void -Hansth*/
      obj_from_char( obj );
      /* THE MEMORY SEVER: Prevent the item from dragging the rest of the inventory with it! -Hansth*/
      obj->next_content = NULL;
      obj->prev_content = NULL;

      CREATE( ah, AH_DATA, 1 );
      ah->seller  = STRALLOC( ch->name );
      ah->buyer   = NULL;
      ah->item    = obj;
      ah->bid     = bid;
      ah->buyout  = buyout;
      
      /* Set expiration for 24 real-world hours from right now */
      ah->expires = current_time + (24 * 3600); 

      /* Attach to the Global Master List */
      LINK( ah, first_ah, last_ah, next, prev );

      /* Force an immediate save to protect the item from server crashes */
      save_auctions();
      save_char_obj( ch );

      ch_printf( ch, "You have listed &G%s&w on the Auction House for %d gold.\r\n", obj->short_descr, bid );
      return;
   }

   /* Catch-all for bad syntax */
   do_auction( ch, "" );
}

/* Make objects in rooms that are nofloor fall - Scryn 1/23/96 */
void obj_fall( OBJ_DATA * obj, bool through )
{
   EXIT_DATA *pexit;
   ROOM_INDEX_DATA *to_room;
   static int fall_count;
   static bool is_falling; /* Stop loops from the call to obj_to_room()  -- Altrag */

   if( !obj->in_room || is_falling )
      return;

   if( fall_count > 30 )
   {
      bug( "%s: object falling in loop more than 30 times", __func__ );
      extract_obj( obj );
      fall_count = 0;
      return;
   }

   if( xIS_SET( obj->in_room->room_flags, ROOM_NOFLOOR ) && CAN_GO( obj, DIR_DOWN ) && !IS_OBJ_STAT( obj, ITEM_MAGIC ) )
   {
      pexit = get_exit( obj->in_room, DIR_DOWN );
      to_room = pexit->to_room;

      if( through )
         fall_count++;
      else
         fall_count = 0;

      if( obj->in_room == to_room )
      {
         bug( "%s: Object falling into same room, room %d", __func__, to_room->vnum );
         extract_obj( obj );
         return;
      }

      if( obj->in_room->first_person )
      {
         act( AT_PLAIN, "$p falls far below...", obj->in_room->first_person, obj, NULL, TO_ROOM );
         act( AT_PLAIN, "$p falls far below...", obj->in_room->first_person, obj, NULL, TO_CHAR );
      }
      obj_from_room( obj );
      is_falling = TRUE;
      obj = obj_to_room( obj, to_room );
      is_falling = FALSE;

      if( obj->in_room->first_person )
      {
         act( AT_PLAIN, "$p falls from above...", obj->in_room->first_person, obj, NULL, TO_ROOM );
         act( AT_PLAIN, "$p falls from above...", obj->in_room->first_person, obj, NULL, TO_CHAR );
      }

      if( !xIS_SET( obj->in_room->room_flags, ROOM_NOFLOOR ) && through )
      {
/*		int dam = (int)9.81*sqrt(fall_count*2/9.81)*obj->weight/2;
*/ int dam = fall_count * obj->weight / 2;
         /*
          * Damage players 
          */
         if( obj->in_room->first_person && number_percent(  ) > 15 )
         {
            CHAR_DATA *rch;
            CHAR_DATA *vch = NULL;
            int chcnt = 0;

            for( rch = obj->in_room->first_person; rch; rch = rch->next_in_room, chcnt++ )
               if( number_range( 0, chcnt ) == 0 )
                  vch = rch;

            if( vch )
            {
               act( AT_WHITE, "$p falls on $n!", vch, obj, NULL, TO_ROOM );
               act( AT_WHITE, "$p falls on you!", vch, obj, NULL, TO_CHAR );

               if( IS_NPC( vch ) && xIS_SET( vch->act, ACT_HARDHAT ) )
                  act( AT_WHITE, "$p bounces harmlessly off your head!", vch, obj, NULL, TO_CHAR );
               else
                  damage( vch, vch, dam * vch->level, TYPE_UNDEFINED );
            }
         }
         /*
          * Damage objects 
          */
         switch ( obj->item_type )
         {
            case ITEM_WEAPON:
            case ITEM_ARMOR:
               if( ( obj->value[0] - dam ) <= 0 )
               {
                  if( obj->in_room->first_person )
                  {
                     act( AT_PLAIN, "$p is destroyed by the fall!", obj->in_room->first_person, obj, NULL, TO_ROOM );
                     act( AT_PLAIN, "$p is destroyed by the fall!", obj->in_room->first_person, obj, NULL, TO_CHAR );
                  }
                  make_scraps( obj );
               }
               else
                  obj->value[0] -= dam;
               break;
            default:
               if( ( dam * 15 ) > get_obj_resistance( obj ) )
               {
                  if( obj->in_room->first_person )
                  {
                     act( AT_PLAIN, "$p is destroyed by the fall!", obj->in_room->first_person, obj, NULL, TO_ROOM );
                     act( AT_PLAIN, "$p is destroyed by the fall!", obj->in_room->first_person, obj, NULL, TO_CHAR );
                  }
                  make_scraps( obj );
               }
               break;
         }
      }
      obj_fall( obj, TRUE );
   }
}

/* Scryn, by request of Darkur, 12/04/98 */
/* Reworked recursive_note_find to fix crash bug when the note was left 
 * blank.  7/6/98 -- Shaddai
 */
OBJ_DATA *recursive_note_find( OBJ_DATA * obj, const char *argument )
{
   OBJ_DATA *returned_obj;
   bool match = TRUE;
   const char *argcopy;
   const char *subject;

   char arg[MAX_INPUT_LENGTH];
   char subj[MAX_STRING_LENGTH];

   if( !obj )
      return NULL;

   switch ( obj->item_type )
   {
      case ITEM_PAPER:

         if( ( subject = get_extra_descr( "_subject_", obj->first_extradesc ) ) == NULL )
            break;
         snprintf( subj, MAX_STRING_LENGTH, "%s", strlower( subject ) );
         subject = strlower( subj );

         argcopy = argument;

         while( match )
         {
            argcopy = one_argument( argcopy, arg );

            if( arg[0] == '\0' )
               break;

            if( !strstr( subject, arg ) )
               match = FALSE;
         }


         if( match )
            return obj;
         break;

      case ITEM_CONTAINER:
      case ITEM_CORPSE_NPC:
      case ITEM_CORPSE_PC:
         if( obj->first_content )
         {
            returned_obj = recursive_note_find( obj->first_content, argument );
            if( returned_obj )
               return returned_obj;
         }
         break;

      default:
         break;
   }

   return recursive_note_find( obj->next_content, argument );
}

void do_findnote( CHAR_DATA* ch, const char* argument )
{
   OBJ_DATA *obj;

   if( IS_NPC( ch ) )
   {
      send_to_char( "Huh?\r\n", ch );
      return;
   }

   if( argument[0] == '\0' )
   {
      send_to_char( "You must specify at least one keyword.\r\n", ch );
      return;
   }

   obj = recursive_note_find( ch->first_carrying, argument );

   if( obj )
   {
      if( obj->in_obj )
      {
         obj_from_obj( obj );
         obj = obj_to_char( obj, ch );
      }
      wear_obj( ch, obj, TRUE, -1 );
   }
   else
      send_to_char( "Note not found.\r\n", ch );
}

const char *get_chance_verb( OBJ_DATA * obj )
{
   return ( obj->action_desc[0] != '\0' ) ? obj->action_desc : "roll$q";
}

const char *get_ed_number( OBJ_DATA * obj, int number )
{
   EXTRA_DESCR_DATA *ed;
   int count;

   for( ed = obj->first_extradesc, count = 1; ed; ed = ed->next, count++ )
   {
      if( count == number )
         return ed->description;
   }

   return NULL;
}

void do_rolldie( CHAR_DATA* ch, const char* argument )
{
   OBJ_DATA *die;

   char output_string[MAX_STRING_LENGTH];
   char roll_string[MAX_INPUT_LENGTH];
   char total_string[MAX_INPUT_LENGTH];

   const char *verb;

   int rollsum = 0;
   int roll_count = 0;

   int numsides;
   int numrolls;

   bool *face_seen_table = NULL;

   if( ( die = get_eq_char( ch, WEAR_HOLD ) ) == NULL || die->item_type != ITEM_CHANCE )
   {
      ch_printf( ch, "You must be holding an item of chance!\r\n" );
      return;
   }

   numrolls = ( is_number( argument ) ) ? atoi( argument ) : 1;
   verb = get_chance_verb( die );

   if( numrolls > 100 )
   {
      ch_printf( ch, "You can't %s more than 100 times!\r\n", verb );
      return;
   }

   numsides = die->value[0];

   if( numsides <= 1 )
   {
      ch_printf( ch, "There is no element of chance in this game!\r\n" );
      return;
   }

   if( die->value[3] == 1 )
   {
      if( numrolls > numsides )
      {
         ch_printf( ch, "Nice try, but you can only %s %d times.\r\n", verb, numsides );
         return;
      }
      face_seen_table = ( bool * ) calloc( numsides, sizeof( bool ) );
      if( !face_seen_table )
      {
         bug( "%s: cannot allocate memory for face_seen_table array, terminating.\r\n", __func__ );
         return;
      }
   }

   snprintf( roll_string, MAX_INPUT_LENGTH, "%s", " " );

   while( roll_count++ < numrolls )
   {
      int current_roll;
      char current_roll_string[MAX_STRING_LENGTH];

      do
      {
         current_roll = number_range( 1, numsides );
      }
      while( die->value[3] == 1 && face_seen_table[current_roll - 1] == TRUE );

      if( die->value[3] == 1 )
         face_seen_table[current_roll - 1] = TRUE;

      rollsum += current_roll;

      if( roll_count > 1 )
         strlcat( roll_string, ", ", MAX_INPUT_LENGTH );
      if( numrolls > 1 && roll_count == numrolls )
         strlcat( roll_string, "and ", MAX_INPUT_LENGTH );

      if( die->value[1] == 1 )
      {
         const char *face_name = get_ed_number( die, current_roll );
         if( face_name )
         {
            char *face_name_copy = strdup( face_name );  /* Since I want to tokenize without modifying the original string */
            snprintf( current_roll_string, MAX_STRING_LENGTH, "%s", strtok( face_name_copy, "\n" ) );
            free( face_name_copy );
         }
         else
            snprintf( current_roll_string, MAX_STRING_LENGTH, "%d", current_roll );
      }
      else
         snprintf( current_roll_string, MAX_STRING_LENGTH, "%d", current_roll );
      strlcat( roll_string, current_roll_string, MAX_INPUT_LENGTH );
   }

   if( numrolls > 1 && die->value[2] == 1 )
   {
      snprintf( total_string, MAX_INPUT_LENGTH, ", for a total of %d", rollsum );
      strlcat( roll_string, total_string, MAX_INPUT_LENGTH );
   }

   strlcat( roll_string, ".\r\n", MAX_INPUT_LENGTH );

   snprintf( output_string, MAX_STRING_LENGTH, "You %s%s", verb, roll_string );
   act( AT_GREEN, output_string, ch, NULL, NULL, TO_CHAR );

   snprintf( output_string, MAX_STRING_LENGTH, "$n %s%s", verb, roll_string );
   act( AT_GREEN, output_string, ch, NULL, NULL, TO_ROOM );

   if( face_seen_table )
      free( face_seen_table );
}
/*dice chance deal throw*/

/* ============================================
   Wowzers Mud: Need/Greed Loot Roll Engine -Hansth
   ============================================ */

/* Concrete global variables for the Loot Engine */
LOOT_ROLL_DATA * first_loot_roll = NULL;
LOOT_ROLL_DATA * last_loot_roll = NULL;

/* Helper function to check if a roll is finished and resolve it -Hansth */
void check_roll_resolution( LOOT_ROLL_DATA *roll )
{
   ROLL_MEMBER_DATA *member;
   ROLL_MEMBER_DATA *winner = NULL;
   int highest_roll = 0;
   int winning_type = ROLL_PASS;

   /* 1. Check if we are still waiting on anyone */
   for ( member = roll->first_member; member; member = member->next )
   {
      if ( member->roll_type == ROLL_PENDING )
         return; /* Someone hasn't rolled yet! Abort and wait. */
   }

   /* 2. Everyone has chosen. Calculate winner! */
   /* First, check if anyone rolled NEED */
   for ( member = roll->first_member; member; member = member->next )
   {
      if ( member->roll_type == ROLL_NEED )
      {
         if ( winning_type != ROLL_NEED || member->roll_value > highest_roll )
         {
            winner = member;
            highest_roll = member->roll_value;
            winning_type = ROLL_NEED;
         }
      }
   }

   /* 3. If no one needed, check GREED */
   if ( !winner )
   {
      for ( member = roll->first_member; member; member = member->next )
      {
         if ( member->roll_type == ROLL_GREED )
         {
            if ( winning_type != ROLL_GREED || member->roll_value > highest_roll )
            {
               winner = member;
               highest_roll = member->roll_value;
               winning_type = ROLL_GREED;
            }
         }
      }
   }

   /* 4. Distribute the item */
   if ( winner )
   {
      ch_printf( winner->ch, "&YYou won the roll for %s! (%d %s)&w\r\n", 
         roll->obj->short_descr, highest_roll, winning_type == ROLL_NEED ? "Need" : "Greed" );
      
      act( AT_YELL, "$n won the roll for $p!", winner->ch, roll->obj, NULL, TO_ROOM );
      
      /* Physically transfer the item to the winner */
      obj_to_char( roll->obj, winner->ch );
   }
   else
   {
      /* Everyone passed! Drop it on the ground */
      if ( roll->first_member && roll->first_member->ch->in_room )
      {
          obj_to_room( roll->obj, roll->first_member->ch->in_room );
          act( AT_ACTION, "Everyone passed on $p. It falls to the ground.", 
               roll->first_member->ch, roll->obj, NULL, TO_ROOM );
          ch_printf( roll->first_member->ch, "Everyone passed on %s. It falls to the ground.\r\n", 
               roll->obj->short_descr );
      }
   }

   /* 5. Memory Cleanup (Prevent memory leaks!) */
   UNLINK( roll, first_loot_roll, last_loot_roll, next, prev );
   
   while ( ( member = roll->first_member ) != NULL )
   {
      UNLINK( member, roll->first_member, roll->last_member, next, prev );
      DISPOSE( member );
   }
   DISPOSE( roll );
}

void handle_loot_roll( CHAR_DATA *ch, short roll_choice )
{
   LOOT_ROLL_DATA *roll;
   ROLL_MEMBER_DATA *member;
   bool found_roll = FALSE;

   /* Find the first roll this player is involved in that is still pending */
   for ( roll = first_loot_roll; roll; roll = roll->next )
   {
      for ( member = roll->first_member; member; member = member->next )
      {
         if ( member->ch == ch && member->roll_type == ROLL_PENDING )
         {
            found_roll = TRUE;
            member->roll_type = roll_choice;

            if ( roll_choice == ROLL_PASS )
            {
               ch_printf( ch, "You passed on %s.\r\n", roll->obj->short_descr );
               act( AT_ACTION, "$n passes on $p.", ch, roll->obj, NULL, TO_ROOM );
            }
            else
            {
               /* Generate the 1-100 roll! */
               member->roll_value = number_range( 1, 100 );
               
               ch_printf( ch, "You rolled %d for %s on %s.\r\n", 
                  member->roll_value, 
                  (roll_choice == ROLL_NEED) ? "Need" : "Greed", 
                  roll->obj->short_descr );
                  
               act( AT_ACTION, "$n rolls for $p.", ch, roll->obj, NULL, TO_ROOM );
            }

            /* Check if this was the last person we were waiting on */
            check_roll_resolution( roll );
            return; /* We only handle one pending roll at a time */
         }
      }
   }

   if ( !found_roll )
      send_to_char( "You have no pending loot rolls right now.\r\n", ch );
}

/* ============================================
   Wowzers Mud: Start Loot Roll -Hansth
   ============================================ */
bool start_loot_roll( OBJ_DATA *obj, CHAR_DATA *killer )
{
   CHAR_DATA *gch;
   LOOT_ROLL_DATA *roll;
   ROLL_MEMBER_DATA *member;
   bool grouped = FALSE;

   /* 1. Check if the killer is actually in a group */
   for ( gch = killer->in_room->first_person; gch; gch = gch->next_in_room )
   {
      if ( gch != killer && is_same_group( killer, gch ) && !IS_NPC( gch ) )
      {
         grouped = TRUE;
         break;
      }
   }

   if ( !grouped )
      return FALSE; /* Solo player! Let the item go to the corpse normally. */

   /* 2. Create the Roll Event */
   CREATE( roll, LOOT_ROLL_DATA, 1 );
   roll->obj = obj;
   roll->first_member = NULL;
   roll->last_member = NULL;
   roll->timer = 30; /* Optional: We can hook this into update.c later for AFK auto-passes */

   /* 3. Add eligible group members to the roll */
   for ( gch = killer->in_room->first_person; gch; gch = gch->next_in_room )
   {
      if ( is_same_group( killer, gch ) && !IS_NPC( gch ) )
      {
         CREATE( member, ROLL_MEMBER_DATA, 1 );
         member->ch = gch;
         member->roll_type = ROLL_PENDING;
         member->roll_value = 0;
         LINK( member, roll->first_member, roll->last_member, next, prev );
         
         /* Broadcast the popup window! */
         ch_printf( gch, "\r\n&Y[Loot]: %s has dropped! Type 'need', 'greed', or 'pass'.&w\r\n", obj->short_descr );
      }
   }

   LINK( roll, first_loot_roll, last_loot_roll, next, prev );
   return TRUE; /* Roll successfully started! */
}

/* ============================================
   Wowzers Mud: Loot Roll AFK Timer -Hansth
   ============================================ */
void roll_update( void )
{
   LOOT_ROLL_DATA *roll, *roll_next;
   ROLL_MEMBER_DATA *member;
   bool forced_pass;

   for ( roll = first_loot_roll; roll; roll = roll_next )
   {
      /* Grab the next pointer BEFORE resolving, in case this roll gets deleted! */
      roll_next = roll->next;

      /* Decrement the timer (Called once per second) */
      if ( --roll->timer <= 0 )
      {
         forced_pass = FALSE;

         /* Force anyone who hasn't rolled to pass */
         for ( member = roll->first_member; member; member = member->next )
         {
            if ( member->roll_type == ROLL_PENDING )
            {
               member->roll_type = ROLL_PASS;
               if ( member->ch )
               {
                  ch_printf( member->ch, "&R[Loot]: You took too long and automatically passed on %s.&w\r\n", roll->obj->short_descr );
               }
               forced_pass = TRUE;
            }
         }

         /* If we forced passes, trigger the resolution to give out the loot! */
         if ( forced_pass )
         {
            check_roll_resolution( roll );
         }
      }
   }
}

/* The actual player commands - Visible to dlsym */
__attribute__((visibility("default")))
void do_need( CHAR_DATA *ch, const char *argument )
{
   handle_loot_roll( ch, ROLL_NEED );
}

__attribute__((visibility("default")))
void do_greed( CHAR_DATA *ch, const char *argument )
{
   handle_loot_roll( ch, ROLL_GREED );
}

__attribute__((visibility("default")))
void do_pass( CHAR_DATA *ch, const char *argument )
{
   handle_loot_roll( ch, ROLL_PASS );
}
