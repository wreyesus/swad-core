// swad_chat.c: chat

/*
    SWAD (Shared Workspace At a Distance),
    is a web platform developed at the University of Granada (Spain),
    and used to support university teaching.

    This file is part of SWAD core.
    Copyright (C) 1999-2017 Antonio Ca�as Vargas

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU Affero General Public License as
    published by the Free Software Foundation, either version 3 of the
    License, or (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU Affero General Public License for more details.

    You should have received a copy of the GNU Affero General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/
/*****************************************************************************/
/********************************* Headers ***********************************/
/*****************************************************************************/

#include <linux/stddef.h>	// For NULL
#include <stdbool.h>		// For boolean type
#include <stdio.h>		// For sprintf
#include <string.h>

#include "swad_chat.h"
#include "swad_config.h"
#include "swad_database.h"
#include "swad_global.h"
#include "swad_logo.h"
#include "swad_parameter.h"
#include "swad_string.h"
#include "swad_text.h"

/*****************************************************************************/
/************** External global variables from others modules ****************/
/*****************************************************************************/

extern struct Globals Gbl;

/*****************************************************************************/
/***************************** Private constants *****************************/
/*****************************************************************************/

#define Cht_CHAT_MAX_LEVELS 3

#define Cht_MAX_CHARS_ROOM_CODE	16	// 16, maximum number of chars of the code of a chat room
#define Cht_MAX_BYTES_ROOM_CODE	Cht_MAX_CHARS_ROOM_CODE	// 16

#define Cht_MAX_CHARS_ROOM_SHRT_NAME	(128 - 1)	// 127, maximum number of chars of the short name of a chat room
#define Cht_MAX_BYTES_ROOM_SHRT_NAME	((Cht_MAX_CHARS_ROOM_SHRT_NAME + 1) * Str_MAX_BYTES_PER_CHAR - 1)	// 2047

#define Cht_MAX_CHARS_ROOM_FULL_NAME	(256 - 1)	// 255, maximum number of chars of the full name of a chat room
#define Cht_MAX_BYTES_ROOM_FULL_NAME	((Cht_MAX_CHARS_ROOM_FULL_NAME + 1) * Str_MAX_BYTES_PER_CHAR - 1)	// 4095

/*****************************************************************************/
/***************************** Private prototypes ****************************/
/*****************************************************************************/

static void Cht_WriteLinkToChat1 (const char *RoomCode,const char *RoomShrtName,const char *RoomFullName,
                                  unsigned Level,bool IsLastItemInLevel[1 + Cht_CHAT_MAX_LEVELS]);
static void Cht_WriteLinkToChat2 (const char *RoomCode,const char *RoomFullName);
static unsigned Cht_GetNumUsrsInChatRoom (const char *RoomCode);

/*****************************************************************************/
/****************** List available whiteboard/chat rooms *********************/
/*****************************************************************************/

void Cht_ShowChatRooms (void)
  {
   extern const char *Txt_To_use_chat_you_must_have_installed_the_software_X_and_add_Y_;
   extern const char *Txt_Unfortunately_Firefox_and_Chrome_no_longer_allow_Java_to_run_;

   /***** Help message about software needed to use the whiteboard/chat *****/
   sprintf (Gbl.Alert.Txt,Txt_To_use_chat_you_must_have_installed_the_software_X_and_add_Y_,
            Cfg_JAVA_URL,Cfg_JAVA_NAME,
            Cfg_PLATFORM_SERVER);
   Ale_ShowAlert (Ale_INFO,Gbl.Alert.Txt);

   /***** List available chat rooms *****/
   Cht_ShowListOfAvailableChatRooms ();

   Ale_ShowAlert (Ale_WARNING,Txt_Unfortunately_Firefox_and_Chrome_no_longer_allow_Java_to_run_);

   if (Gbl.Usrs.Me.LoggedRole == Rol_SYS_ADM)
      Cht_ShowListOfChatRoomsWithUsrs ();
  }

/*****************************************************************************/
/*********************** Show list of available chat rooms *******************/
/*****************************************************************************/

void Cht_ShowListOfAvailableChatRooms (void)
  {
   extern const char *Txt_Chat_rooms;
   extern const char *Txt_General;
   extern const char *Txt_SEX_PLURAL_Abc[Usr_NUM_SEXS];
   extern const char *Txt_SEX_PLURAL_abc[Usr_NUM_SEXS];
   extern const char *Txt_ROLES_PLURAL_abc[Rol_NUM_ROLES][Usr_NUM_SEXS];
   extern const char *Txt_Students_ABBREVIATION;
   extern const char *Txt_Teachers_ABBREVIATION;
   extern const char *Txt_Degree;
   extern const char *Txt_Course;
   bool IsLastItemInLevel[1 + Cht_CHAT_MAX_LEVELS];
   unsigned NumMyDeg;
   struct Degree Deg;
   struct Course Crs;
   MYSQL_RES *mysql_res;
   MYSQL_ROW row;
   unsigned long NumRow;
   unsigned long NumRows;
   char ThisRoomCode    [Cht_MAX_BYTES_ROOM_CODE + 1];
   char ThisRoomShrtName[Cht_MAX_BYTES_ROOM_SHRT_NAME + 1];
   char ThisRoomFullName[Cht_MAX_BYTES_ROOM_FULL_NAME + 1];

   /***** Fill the list with the degrees I belong to *****/ 
   Usr_GetMyDegrees ();

   /***** Table start *****/
   Lay_StartRoundFrame (NULL,Txt_Chat_rooms,NULL,NULL);
   fprintf (Gbl.F.Out,"<ul class=\"LIST_LEFT\">");

   /***** Title of top level *****/
   fprintf (Gbl.F.Out,"<li class=\"DAT\" style=\"height:25px;\">"
                      "<img src=\"%s/chat64x64.gif\""
	              " alt=\"%s\" title=\"%s\""
	              " class=\"ICO20x20\" />"
                      " %s"
                      "</li>",
            Gbl.Prefs.IconsURL,
            Txt_Chat_rooms,
            Txt_Chat_rooms,
            Txt_Chat_rooms);

   /***** Link to chat available for all the users *****/
   IsLastItemInLevel[1] = (!Gbl.Usrs.Me.IBelongToCurrentCrs &&
                           !Gbl.Usrs.Me.MyDegs.Num);
   sprintf (ThisRoomFullName,"%s (%s)",Txt_General,Txt_SEX_PLURAL_abc[Usr_SEX_ALL]);
   Cht_WriteLinkToChat1 ("GBL_USR",Txt_SEX_PLURAL_Abc[Usr_SEX_ALL],ThisRoomFullName,1,IsLastItemInLevel);
   fprintf (Gbl.F.Out,"<img src=\"%s/chat64x64.gif\""
	              " alt=\"%s\" title=\"%s\""
	              " class=\"ICO20x20\" />",
            Gbl.Prefs.IconsURL,
            ThisRoomFullName,
            ThisRoomFullName);
   Cht_WriteLinkToChat2 ("GBL_USR",ThisRoomFullName);

   IsLastItemInLevel[1] = !Gbl.Usrs.Me.MyDegs.Num;
   switch (Gbl.Usrs.Me.LoggedRole)
     {
      case Rol_STD:
         sprintf (ThisRoomFullName,"%s (%s)",
                  Txt_General,Txt_ROLES_PLURAL_abc[Rol_STD][Usr_SEX_ALL]);
         Cht_WriteLinkToChat1 ("GBL_STD",Txt_Students_ABBREVIATION,ThisRoomFullName,1,IsLastItemInLevel);
	 fprintf (Gbl.F.Out,"<img src=\"%s/chat64x64.gif\""
			    " alt=\"%s\" title=\"%s\""
			    " class=\"ICO20x20\" />",
		  Gbl.Prefs.IconsURL,
		  ThisRoomFullName,ThisRoomFullName);
	 Cht_WriteLinkToChat2 ("GBL_STD",ThisRoomFullName);
         break;
      case Rol_NED_TCH:
      case Rol_TCH:
         sprintf (ThisRoomFullName,"%s (%s)",
                  Txt_General,Txt_ROLES_PLURAL_abc[Rol_TCH][Usr_SEX_ALL]);
         Cht_WriteLinkToChat1 ("GBL_TCH",Txt_Teachers_ABBREVIATION,ThisRoomFullName,1,IsLastItemInLevel);
	 fprintf (Gbl.F.Out,"<img src=\"%s/chat64x64.gif\""
			    " alt=\"%s\" title=\"%s\""
			    " class=\"ICO20x20\" />",
		  Gbl.Prefs.IconsURL,
		  ThisRoomFullName,ThisRoomFullName);
	 Cht_WriteLinkToChat2 ("GBL_TCH",ThisRoomFullName);
         break;
      default:
         break;
     }

   /***** Link to chat of users from my degrees *****/
   for (NumMyDeg = 0;
	NumMyDeg < Gbl.Usrs.Me.MyDegs.Num;
	NumMyDeg++)
     {
      /* Get data of this degree */
      Deg.DegCod = Gbl.Usrs.Me.MyDegs.Degs[NumMyDeg].DegCod;
      if (!Deg_GetDataOfDegreeByCod (&Deg))
         Lay_ShowErrorAndExit ("Degree not found.");

      /* Link to the room of this degree */
      IsLastItemInLevel[1] = (NumMyDeg == Gbl.Usrs.Me.MyDegs.Num - 1);
      sprintf (ThisRoomCode,"DEG_%ld",Deg.DegCod);
      sprintf (ThisRoomShrtName,"%s",Deg.ShrtName);
      sprintf (ThisRoomFullName,"%s %s",Txt_Degree,Deg.ShrtName);
      Cht_WriteLinkToChat1 (ThisRoomCode,ThisRoomShrtName,ThisRoomFullName,1,IsLastItemInLevel);
      Log_DrawLogo (Sco_SCOPE_DEG,Deg.DegCod,Deg.ShrtName,20,NULL,true);
      Cht_WriteLinkToChat2 (ThisRoomCode,ThisRoomFullName);

      /* Get my courses in this degree from database */
      if ((NumRows = Usr_GetCrssFromUsr (Gbl.Usrs.Me.UsrDat.UsrCod,Deg.DegCod,&mysql_res)) > 0) // Courses found in this degree
         for (NumRow = 0;
              NumRow < NumRows;
              NumRow++)
	   {
	    /* Get next course */
	    row = mysql_fetch_row (mysql_res);

            /* Get course code */
	    if ((Crs.CrsCod = Str_ConvertStrCodToLongCod (row[0])) > 0)
	      {
               /* Get data of this course */
               Crs_GetDataOfCourseByCod (&Crs);

               /* Link to the room of this course */
               IsLastItemInLevel[2] = (NumRow == NumRows - 1);
               sprintf (ThisRoomCode,"CRS_%ld",Crs.CrsCod);
               sprintf (ThisRoomShrtName,"%s",Crs.ShrtName);
               sprintf (ThisRoomFullName,"%s %s",Txt_Course,Crs.ShrtName);
               Cht_WriteLinkToChat1 (ThisRoomCode,ThisRoomShrtName,ThisRoomFullName,2,IsLastItemInLevel);
               fprintf (Gbl.F.Out,"<img src=\"%s/dot64x64.png\""
        	                  " alt=\"%s\" title=\"%s\""
        	                  " class=\"ICO20x20\" />",
                        Gbl.Prefs.IconsURL,
                        ThisRoomFullName,ThisRoomFullName);
               Cht_WriteLinkToChat2 (ThisRoomCode,ThisRoomFullName);
	      }
	   }

      /***** Free structure that stores the query result *****/
      DB_FreeMySQLResult (&mysql_res);
     }

   /***** End table *****/
   fprintf (Gbl.F.Out,"</ul>");
   Lay_EndRoundFrame ();
  }

/*****************************************************************************/
/********************** Show list of chat rooms with users *******************/
/*****************************************************************************/

void Cht_ShowListOfChatRoomsWithUsrs (void)
  {
   extern const char *Txt_Rooms_with_users;
   extern const char *Txt_CHAT_Room_code;
   extern const char *Txt_No_of_users;
   char Query[256];
   MYSQL_RES *mysql_res;
   MYSQL_ROW row;
   unsigned long NumRow,NumRows;

   /***** Get chat rooms with connected users from database *****/
   sprintf (Query,"SELECT RoomCode,NumUsrs FROM chat"
                  " WHERE NumUsrs>0 ORDER BY NumUsrs DESC,RoomCode");
   NumRows = DB_QuerySELECT (Query,&mysql_res,"can not get chat rooms with connected users");

   if (NumRows > 0) // If not empty chat rooms found
     {
      /***** Table start *****/
      Lay_StartRoundFrameTable (NULL,Txt_Rooms_with_users,NULL,NULL,2);
      fprintf (Gbl.F.Out,"<tr>"
	                 "<th class=\"CENTER_MIDDLE LIGHT_BLUE\">"
	                 "%s"
	                 "</th>"
                         "<th class=\"LEFT_MIDDLE LIGHT_BLUE\">"
                         "%s"
                         "</th>"
                         "</tr>",
               Txt_CHAT_Room_code,
               Txt_No_of_users);

      /***** Loop over chat rooms *****/
      for (NumRow = 0;
	   NumRow < NumRows;
	   NumRow++)
	{
	 /* Get next chat room */
	 row = mysql_fetch_row (mysql_res);
         fprintf (Gbl.F.Out,"<tr>"
                            "<td class=\"DAT LEFT_MIDDLE\">"
                            "%s"
                            "</td>"
                            "<td class=\"DAT RIGHT_MIDDLE\">"
                            "%s"
                            "</td>"
                            "</tr>",
                  row[0],row[1]);
        }

      /***** End table *****/
      Lay_EndRoundFrameTable ();
     }

   /***** Free structure that stores the query result *****/
   DB_FreeMySQLResult (&mysql_res);
  }

/*****************************************************************************/
/******************** Write title and link to a chat room ********************/
/*****************************************************************************/

static void Cht_WriteLinkToChat1 (const char *RoomCode,const char *RoomShrtName,const char *RoomFullName,
                                  unsigned Level,bool IsLastItemInLevel[1 + Cht_CHAT_MAX_LEVELS])
  {
   extern const char *The_ClassForm[The_NUM_THEMES];

   fprintf (Gbl.F.Out,"<li style=\"height:25px;\">");
   Lay_IndentDependingOnLevel (Level,IsLastItemInLevel);
   Act_FormStart (ActCht);
   Cht_WriteParamsRoomCodeAndNames (RoomCode,RoomShrtName,RoomFullName);
   Act_LinkFormSubmit (RoomFullName,The_ClassForm[Gbl.Prefs.Theme],NULL);
  }

static void Cht_WriteLinkToChat2 (const char *RoomCode,const char *RoomFullName)
  {
   extern const char *Txt_connected_PLURAL;
   extern const char *Txt_connected_SINGULAR;
   unsigned NumUsrsInRoom = Cht_GetNumUsrsInChatRoom (RoomCode);

   if (NumUsrsInRoom)
      fprintf (Gbl.F.Out,"<strong>");
   fprintf (Gbl.F.Out," %s",RoomFullName);
   if (NumUsrsInRoom > 1)
      fprintf (Gbl.F.Out," [%d %s]",
               NumUsrsInRoom,Txt_connected_PLURAL);
   else if (NumUsrsInRoom == 1)
      fprintf (Gbl.F.Out," [1 %s]",
               Txt_connected_SINGULAR);
   if (NumUsrsInRoom)
      fprintf (Gbl.F.Out,"</strong>");
   fprintf (Gbl.F.Out,"</a>");
   Act_FormEnd ();
   fprintf (Gbl.F.Out,"</li>");
  }

/*****************************************************************************/
/*** Write parameters with code and names (short and full) of a chat room ****/
/*****************************************************************************/

void Cht_WriteParamsRoomCodeAndNames (const char *RoomCode,const char *RoomShrtName,const char *RoomFullName)
  {
   Par_PutHiddenParamString ("RoomCode",RoomCode);
   Par_PutHiddenParamString ("RoomShrtName",RoomShrtName);
   Par_PutHiddenParamString ("RoomFullName",RoomFullName);
  }

/*****************************************************************************/
/*************** Get number of users connected to a chat room ****************/
/*****************************************************************************/

static unsigned Cht_GetNumUsrsInChatRoom (const char *RoomCode)
  {
   char Query[128 + Cht_MAX_BYTES_ROOM_CODE];
   MYSQL_RES *mysql_res;
   MYSQL_ROW row;
   unsigned NumUsrs = 0;

   /***** Get number of users connected to chat rooms from database *****/
   sprintf (Query,"SELECT NumUsrs FROM chat WHERE RoomCode='%s'",
            RoomCode);
   if (DB_QuerySELECT (Query,&mysql_res,"can not get number of users connected to a chat room"))
     {
      /* Get number of users connected to the chat room */
      row = mysql_fetch_row (mysql_res);
      if (row[0])
         if (sscanf (row[0],"%u",&NumUsrs) != 1)
            NumUsrs = 0;
     }

   /***** Free structure that stores the query result *****/
   DB_FreeMySQLResult (&mysql_res);

   return NumUsrs;
  }

/*****************************************************************************/
/******************************* Enter a chat room ***************************/
/*****************************************************************************/

#define Cht_MAX_BYTES_ROOM_CODES      ((2 + Deg_MAX_DEGREES_PER_USR + Crs_MAX_COURSES_PER_USR) * Cht_MAX_BYTES_ROOM_CODE)
#define Cht_MAX_BYTES_ROOM_SHRT_NAMES ((2 + Deg_MAX_DEGREES_PER_USR + Crs_MAX_COURSES_PER_USR) * Cht_MAX_BYTES_ROOM_SHRT_NAME)
#define Cht_MAX_BYTES_ROOM_FULL_NAMES ((2 + Deg_MAX_DEGREES_PER_USR + Crs_MAX_COURSES_PER_USR) * Cht_MAX_BYTES_ROOM_FULL_NAME)

void Cht_OpenChatWindow (void)
  {
   extern const char *Txt_SEX_PLURAL_Abc[Usr_NUM_SEXS];
   extern const char *Txt_SEX_PLURAL_abc[Usr_NUM_SEXS];
   extern const char *Txt_General;
   extern const char *Txt_ROLES_PLURAL_abc[Rol_NUM_ROLES][Usr_NUM_SEXS];
   extern const char *Txt_Students_ABBREVIATION;
   extern const char *Txt_Teachers_ABBREVIATION;
   extern const char *Txt_Degree;
   extern const char *Txt_Course;
   char RoomCode[Cht_MAX_BYTES_ROOM_CODE + 1];
   char RoomShrtName[Cht_MAX_BYTES_ROOM_SHRT_NAME + 1];
   char RoomFullName [Cht_MAX_BYTES_ROOM_FULL_NAME + 1];
   char UsrName[Usr_MAX_BYTES_FULL_NAME + 1];
   unsigned NumMyDeg;
   unsigned NumMyCrs;
   struct Degree Deg;
   struct Course Crs;
   char ThisRoomCode[Cht_MAX_BYTES_ROOM_CODE + 1];
   char ThisRoomShortName[Cht_MAX_BYTES_ROOM_SHRT_NAME + 1];
   char ThisRoomFullName [Cht_MAX_BYTES_ROOM_FULL_NAME + 1];
   char ListRoomCodes     [Cht_MAX_BYTES_ROOM_CODES      + 1];
   char ListRoomShrtNames[Cht_MAX_BYTES_ROOM_SHRT_NAMES + 1];
   char ListRoomFullNames [Cht_MAX_BYTES_ROOM_FULL_NAMES + 1];
   FILE *FileChat;

   /***** Get the code and the nombre of the room *****/
   Par_GetParToText ("RoomCode",RoomCode,Cht_MAX_BYTES_ROOM_CODE);

   Par_GetParToText ("RoomShrtName",RoomShrtName,Cht_MAX_BYTES_ROOM_SHRT_NAME);

   Par_GetParToText ("RoomFullName",RoomFullName,Cht_MAX_BYTES_ROOM_FULL_NAME);

   if (!RoomCode[0] || !RoomShrtName[0] || !RoomFullName[0])
      Lay_ShowErrorAndExit ("Wrong code or name of chat room.");
   if (strcspn (RoomCode," \t\n\r") != strlen (RoomCode)) // If RoomCode contiene espacios
      Lay_ShowErrorAndExit ("Wrong code of chat room.");

   /***** Fill the lists with the degrees and courses I belong to *****/ 
   Usr_GetMyDegrees ();
   Usr_GetMyCourses ();

   /***** Build my user's name *****/
   Str_Copy (UsrName,Gbl.Usrs.Me.UsrDat.Surname1,
             Usr_MAX_BYTES_FULL_NAME);
   if (Gbl.Usrs.Me.UsrDat.Surname2[0])
     {
      Str_Concat (UsrName," ",
                  Usr_MAX_BYTES_FULL_NAME);
      Str_Concat (UsrName,Gbl.Usrs.Me.UsrDat.Surname2,
                  Usr_MAX_BYTES_FULL_NAME);
     }
   Str_Concat (UsrName,", ",
               Usr_MAX_BYTES_FULL_NAME);
   Str_Concat (UsrName,Gbl.Usrs.Me.UsrDat.FirstName,
               Usr_MAX_BYTES_FULL_NAME);

   /***** Build the lists of available rooms *****/
   sprintf (ListRoomCodes,"#%s",RoomCode);
   Str_Copy (ListRoomShrtNames,RoomShrtName,
             Cht_MAX_BYTES_ROOM_SHRT_NAMES);
   Str_Copy (ListRoomFullNames ,RoomFullName,
             Cht_MAX_BYTES_ROOM_FULL_NAMES);

   if (strcmp (RoomCode,"GBL_USR"))
     {
      Str_Concat (ListRoomCodes,"|#GBL_USR",
                  Cht_MAX_BYTES_ROOM_CODES);

      sprintf (RoomShrtName,"|%s",Txt_SEX_PLURAL_Abc[Usr_SEX_ALL]);
      Str_Concat (ListRoomShrtNames,RoomShrtName,
                  Cht_MAX_BYTES_ROOM_SHRT_NAMES);

      sprintf (RoomFullName,"|%s (%s)",
               Txt_General,Txt_SEX_PLURAL_abc[Usr_SEX_ALL]);
      Str_Concat (ListRoomFullNames,RoomFullName,
                  Cht_MAX_BYTES_ROOM_FULL_NAMES);
     }

   if (Gbl.Usrs.Me.LoggedRole == Rol_STD)
      if (strcmp (RoomCode,"GBL_STD"))
        {
         Str_Concat (ListRoomCodes,"|#GBL_STD",
                     Cht_MAX_BYTES_ROOM_CODES);

         sprintf (RoomShrtName,"|%s",Txt_Students_ABBREVIATION);
         Str_Concat (ListRoomShrtNames,RoomShrtName,
                     Cht_MAX_BYTES_ROOM_SHRT_NAMES);

         sprintf (RoomFullName,"|%s (%s)",Txt_General,
                  Txt_ROLES_PLURAL_abc[Rol_STD][Usr_SEX_ALL]);
	 Str_Concat (ListRoomFullNames,RoomFullName,
	             Cht_MAX_BYTES_ROOM_FULL_NAMES);
        }

   if (Gbl.Usrs.Me.LoggedRole == Rol_NED_TCH ||
       Gbl.Usrs.Me.LoggedRole == Rol_TCH)
      if (strcmp (RoomCode,"GBL_TCH"))
        {
         Str_Concat (ListRoomCodes,"|#GBL_TCH",
                     Cht_MAX_BYTES_ROOM_CODES);

         sprintf (RoomShrtName,"|%s",Txt_Teachers_ABBREVIATION);
         Str_Concat (ListRoomShrtNames,RoomShrtName,
                     Cht_MAX_BYTES_ROOM_SHRT_NAMES);

         sprintf (RoomFullName,"|%s (%s)",
                  Txt_General,Txt_ROLES_PLURAL_abc[Rol_TCH][Usr_SEX_ALL]);
	 Str_Concat (ListRoomFullNames,RoomFullName,
	             Cht_MAX_BYTES_ROOM_FULL_NAMES);
        }

   for (NumMyDeg = 0;
	NumMyDeg < Gbl.Usrs.Me.MyDegs.Num;
	NumMyDeg++)
     {
      sprintf (ThisRoomCode,"DEG_%ld",Gbl.Usrs.Me.MyDegs.Degs[NumMyDeg].DegCod);
      if (strcmp (RoomCode,ThisRoomCode))
        {
         Str_Concat (ListRoomCodes,"|#",
                     Cht_MAX_BYTES_ROOM_CODES);
         Str_Concat (ListRoomCodes,ThisRoomCode,
                     Cht_MAX_BYTES_ROOM_CODES);

         /* Get data of this degree */
         Deg.DegCod = Gbl.Usrs.Me.MyDegs.Degs[NumMyDeg].DegCod;
         Deg_GetDataOfDegreeByCod (&Deg);

         sprintf (ThisRoomShortName,"%s",Deg.ShrtName);
         Str_Concat (ListRoomShrtNames,"|",
                     Cht_MAX_BYTES_ROOM_SHRT_NAMES);
         Str_Concat (ListRoomShrtNames,ThisRoomShortName,
                     Cht_MAX_BYTES_ROOM_SHRT_NAMES);

         sprintf (ThisRoomFullName,"%s %s",Txt_Degree,Deg.ShrtName);
         Str_Concat (ListRoomFullNames,"|",
                     Cht_MAX_BYTES_ROOM_FULL_NAMES);
         Str_Concat (ListRoomFullNames,ThisRoomFullName,
                     Cht_MAX_BYTES_ROOM_FULL_NAMES);
        }
     }

   for (NumMyCrs = 0;
	NumMyCrs < Gbl.Usrs.Me.MyCrss.Num;
	NumMyCrs++)
     {
      sprintf (ThisRoomCode,"CRS_%ld",Gbl.Usrs.Me.MyCrss.Crss[NumMyCrs].CrsCod);
      if (strcmp (RoomCode,ThisRoomCode))
        {
         Str_Concat (ListRoomCodes,"|#",
                     Cht_MAX_BYTES_ROOM_CODES);
         Str_Concat (ListRoomCodes,ThisRoomCode,
                     Cht_MAX_BYTES_ROOM_CODES);

         /* Get data of this course */
         Crs.CrsCod = Gbl.Usrs.Me.MyCrss.Crss[NumMyCrs].CrsCod;
         Crs_GetDataOfCourseByCod (&Crs);

         sprintf (ThisRoomShortName,"%s",Crs.ShrtName);
         Str_Concat (ListRoomShrtNames,"|",
                     Cht_MAX_BYTES_ROOM_SHRT_NAMES);
         Str_Concat (ListRoomShrtNames,ThisRoomShortName,
                     Cht_MAX_BYTES_ROOM_SHRT_NAMES);

         sprintf (ThisRoomFullName,"%s %s",Txt_Course,Crs.ShrtName);
         Str_Concat (ListRoomFullNames,"|",
                     Cht_MAX_BYTES_ROOM_FULL_NAMES);
         Str_Concat (ListRoomFullNames,ThisRoomFullName,
                     Cht_MAX_BYTES_ROOM_FULL_NAMES);
        }
     }

   /***** Open index.html file with the HTML page for the chat *****/
   if ((FileChat = fopen (Cfg_PATH_AND_FILE_REL_CHAT_PRIVATE,"rb")) == NULL)
      Lay_ShowErrorAndExit ("Can not open chat.");

   /***** Start writing the index.html file to the output *****/
   fprintf (Gbl.F.Out,"Content-type: text/html; charset=windows-1252\n\n");	// Two carriage returns at the end of the line are mandatory!

   Gbl.Layout.HTMLStartWritten = true;

   /***** Copy index.html file until the end of the applet code *****/
   Str_WriteUntilStrFoundInFileIncludingStr (Gbl.F.Out,FileChat,"<applet",
                                             Str_NO_SKIP_HTML_COMMENTS);
   Str_WriteUntilStrFoundInFileIncludingStr (Gbl.F.Out,FileChat,">",
                                             Str_NO_SKIP_HTML_COMMENTS);

   /***** Write parameters *****/
   fprintf (Gbl.F.Out,"\n<param name=\"nick\" value=\"N%s\">",
	    Gbl.Session.Id);
   fprintf (Gbl.F.Out,"\n<param name=\"realname\" value=\"%s\">",
	    UsrName);
   fprintf (Gbl.F.Out,"\n<param name=\"host\" value=\"%s\">",
	    Gbl.IP);
   fprintf (Gbl.F.Out,"\n<param name=\"server_name\" value=\"%s\">",
	    Cfg_PLATFORM_SERVER);
   fprintf (Gbl.F.Out,"\n<param name=\"port\" value=\"5000\">");
   fprintf (Gbl.F.Out,"\n<param name=\"image_bl\" value=\"%s/%s/usr_bl.jpg\">",
	    Cfg_URL_SWAD_PUBLIC,Cfg_FOLDER_PUBLIC_ICON);
   fprintf (Gbl.F.Out,"\n<param name=\"image_url\" value=\"%s/%s/%s.jpg\">",
	    Cfg_URL_SWAD_PUBLIC,Cfg_FOLDER_PHOTO,Gbl.Usrs.Me.UsrDat.Photo);
   fprintf (Gbl.F.Out,"\n<param name=\"channel_name\" value=\"%s\">",
	    ListRoomCodes);
   fprintf (Gbl.F.Out,"\n<param name=\"tab\" value=\"%s\">",
	    ListRoomShrtNames);
   fprintf (Gbl.F.Out,"\n<param name=\"topic\" value=\"%s\">",
	    ListRoomFullNames);

   /***** Copy index.html file until the end *****/
   Str_WriteUntilStrFoundInFileIncludingStr (Gbl.F.Out,FileChat,"</html>",
                                             Str_NO_SKIP_HTML_COMMENTS);

   /***** Close index.html file *****/
   fclose (FileChat);

   Gbl.Layout.DivsEndWritten = Gbl.Layout.HTMLEndWritten = true;
  }
