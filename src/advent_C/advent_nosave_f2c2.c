/* advent_nosave_f2c2.f -- translated by f2c (version 20200916).
   You must link the resulting object file with libf2c:
	on Microsoft Windows system, link with libf2c.lib;
	on Linux or Unix systems, link with .../path/to/libf2c.a -lm
	or, if you install libf2c.a in a standard place, with -lf2c -lm
	-- in that order, at the end of the command line, as in
		cc *.o -lf2c -lm
	Source for libf2c is in /netlib/f2c/libf2c.zip, e.g.,

		http://www.netlib.org/f2c/libf2c.zip
*/

#include "f2c.h"
#include <stdio.h>
#include <emscripten.h>

/* --- JS peek hook: current location ------------------------------- */
/* Pointer to the f2c MAIN__-local "loc" variable */
static integer *advent_loc_ptr = 0;
static int advent_player_dead = 0;
static integer *advent_oldloc_ptr = 0;
static integer *advent_newloc_ptr = 0;
static integer *advent_dflag_ptr = 0;
static integer *advent_dloc_ptr = 0;   /* dloc[6] */
static integer *advent_odloc_ptr = 0;  /* odloc[6] */
static logical *advent_dseen_ptr = 0;  /* dseen[6] */

/* The object numbers are looked up dynamically from the advent.dat */
/* adding them here is duplication, but still more efficient than looking them up dynamically */
/* Especially since the variables are live in main() scope, not global */
const integer grateObjNum = 3;
const integer snakeObjNum = 11;
const integer doorObjNum = 9;
const integer fissurObjNum = 12;
const integer clamObjNum = 14;
const integer plantObjNum = 24;
const integer dragonObjNum = 31;
const integer trollObjNum = 33;
const integer bearObjNum = 35;
const integer chainObjNum = 64;

/* Exposed to JS: returns current room code, or -1 if not initialised yet */
EMSCRIPTEN_KEEPALIVE int get_current_location(void) {
    if (!advent_loc_ptr) {
        return -1;
    }
    return (int)*advent_loc_ptr;
}

/* Exposed to JS: returns 1 if the player is in the death flow, else 0 */
EMSCRIPTEN_KEEPALIVE int get_player_dead(void) {
    return advent_player_dead ? 1 : 0;
}

/* Exposed to JS: force current room code (debug). Returns new loc (>0) on success,
   -1 if not initialised, -2 if out of range. */
EMSCRIPTEN_KEEPALIVE int set_current_location(int newLoc) {
    if (!advent_loc_ptr) return -1;
    if (newLoc < 1 || newLoc > 150) return -2;
    *advent_loc_ptr = (integer)newLoc;
    /* Keep the main loop a bit more coherent if these are available. */
    if (advent_oldloc_ptr) *advent_oldloc_ptr = (integer)newLoc;
    if (advent_newloc_ptr) *advent_newloc_ptr = (integer)newLoc;
    return (int)*advent_loc_ptr;
}

/* Exposed to JS: spawn helpers (debug)
   - spawn_dwarf(which): which=1..5 for a normal dwarf, 6 for pirate.
     Returns 1 on success, negative on error.
*/
EMSCRIPTEN_KEEPALIVE int spawn_dwarf(int which) {
    if (!advent_loc_ptr || !advent_dloc_ptr || !advent_odloc_ptr || !advent_dseen_ptr || !advent_dflag_ptr) {
        return -1; /* not ready */
    }
    int w = which | 0;
    if (w < 1 || w > 6) return -2;

    /* Ensure dwarf system is active */
    if (*advent_dflag_ptr < 2) {
        *advent_dflag_ptr = 2;
    }

    int idx = w - 1;
    /* Force the dwarf/pirate into the current location and make it look like
       they've just arrived (so the normal "appears" messaging/logic triggers). */
    advent_odloc_ptr[idx] = 0;
    advent_dloc_ptr[idx]  = *advent_loc_ptr;
    advent_dseen_ptr[idx] = FALSE_;

    return 1;
}

EMSCRIPTEN_KEEPALIVE int spawn_all_dwarves(void) {
    int rc = 1;
    int i;
    for (i = 1; i <= 5; i++) {
        int r = spawn_dwarf(i);
        if (r < 0) rc = r;
    }
    return rc;
}

EMSCRIPTEN_KEEPALIVE int spawn_pirate(void) {
    return spawn_dwarf(6);
}



/* --- JS peek hook: object properties and key object numbers ---------- */
/* Pointer to the f2c MAIN__-local "prop" array (object states) */
static integer *advent_prop_ptr = 0;
static integer *advent_cond_ptr = 0;

/* Pointers to the f2c global runtime location arrays (place/fixed) */
static integer *advent_place_ptr = 0;
static integer *advent_fixed_ptr = 0;

/* Exposed to JS: return prop[obj-1] (object state), or -9999 on error */
EMSCRIPTEN_KEEPALIVE int get_object_prop(int obj) {
    if (!advent_prop_ptr) {
        return -9999;
    }
    /* Object numbers in this codebase are 1..100 (see prop[100]) */
    if (obj <= 0 || obj > 100) {
        return -9999;
    }
    return (int)advent_prop_ptr[obj - 1];
}

/* Exposed to JS: return cond[loc-1] (room condition flags), or -9999 on error */
EMSCRIPTEN_KEEPALIVE int get_location_cond(int loc) {
    if (!advent_cond_ptr) {
        return -9999;
    }
    if (loc <= 0 || loc > 150) {
        return -9999;
    }
    return (int)advent_cond_ptr[loc - 1];
}

/* Exposed to JS: return place[obj-1] (object's current location), or -9999 on error */
EMSCRIPTEN_KEEPALIVE int get_object_place(int obj) {
    if (!advent_place_ptr) {
        return -9999;
    }
    if (obj <= 0 || obj > 100) {
        return -9999;
    }
    return (int)advent_place_ptr[obj - 1];
}

/* Exposed to JS: return fixed[obj-1] (object's fixed location, if any), or -9999 on error */
EMSCRIPTEN_KEEPALIVE int get_object_fixed(int obj) {
    if (!advent_fixed_ptr) {
        return -9999;
    }
    if (obj <= 0 || obj > 100) {
        return -9999;
    }
    return (int)advent_fixed_ptr[obj - 1];
}

/* Exposed to JS: number of objects in this build (1..N) */
EMSCRIPTEN_KEEPALIVE int get_object_count(void) {
    return 100;
}

/* Exposed to JS: convenience accessor for the grate's prop value */
EMSCRIPTEN_KEEPALIVE int get_bear_prop(void) {
    return get_object_prop(bearObjNum);
}

/* Exposed to JS: convenience accessor for the grate's prop value */
EMSCRIPTEN_KEEPALIVE int get_chain_prop(void) {
    return get_object_prop(chainObjNum);
}

/* Exposed to JS: convenience accessor for the grate's prop value */
EMSCRIPTEN_KEEPALIVE int get_grate_prop(void) {
    return get_object_prop(grateObjNum);
}

/* Exposed to JS: convenience accessor for the fissure/bridge prop value
   (0 = bridge absent, 1 = crystal bridge present) */
EMSCRIPTEN_KEEPALIVE int get_fissure_prop(void) {
    return get_object_prop(fissurObjNum);
}

/* Exposed to JS: convenience accessor for the plant prop value
   (0 = small, 2 = grown, 4 = huge/climbable) */
EMSCRIPTEN_KEEPALIVE int get_plant_prop(void) {
    return get_object_prop(plantObjNum);
}

/* Exposed to JS: convenience accessor for the snake prop value
   (0 = present, 1 = removed) */
EMSCRIPTEN_KEEPALIVE int get_snake_prop(void) {
    return get_object_prop(snakeObjNum);
}


/*************************************************************
 * Exposed to JS: convenience accessor for the door prop value
 * (0 = stuck closed, 1 = oiled/free)
 *************************************************************/
EMSCRIPTEN_KEEPALIVE int get_door_prop(void) {
    return get_object_prop(doorObjNum);
}

/*************************************************************
 * Exposed to JS: convenience accessor for the clam prop value
 * (0 = closed, 1 = opened)
 *************************************************************/
EMSCRIPTEN_KEEPALIVE int get_clam_prop(void) {
    return get_object_prop(clamObjNum);
}

/* Exposed to JS: convenience accessor for the troll prop value
   (0 = normal/blocking, 1 = has been paid / stepped aside, 2 = gone with bridge) */
EMSCRIPTEN_KEEPALIVE int get_troll_prop(void) {
    return get_object_prop(trollObjNum);
}

/* Exposed to JS: convenience accessor for the dragon prop value
   (0 = alive, 2 = dead on rug, other values as per game logic) */
EMSCRIPTEN_KEEPALIVE int get_dragon_prop(void) {
    return get_object_prop(dragonObjNum);
}

/* -------------------------------------------------------------------- */

/* --- JS option hook: disable dwarves ----------------------------- */
static int advent_disable_dwarves = 0;

/* Exposed to JS: 0 = dwarves enabled (default), 1 = dwarves disabled */
EMSCRIPTEN_KEEPALIVE void set_dwarves_disabled(int disabled) {
    advent_disable_dwarves = disabled ? 1 : 0;
}

/* Optional (handy for debugging): returns 1 if disabled */
EMSCRIPTEN_KEEPALIVE int get_dwarves_disabled(void) {
    return advent_disable_dwarves;
}
/* --------------------------------------------------------------- */


/* Common Block Declarations */

struct {
    integer rtext[205], linidx[1000];
    char lintxt[48251];
} txtcom_;

#define txtcom_1 txtcom_

struct blkcom_1_ {
    logical blklin;
};

#define blkcom_1 (*(struct blkcom_1_ *) &blkcom_)

struct voccom_1_ {
    integer ktab[300];
    char atab[1500];
    integer tabsiz;
};

#define voccom_1 (*(struct voccom_1_ *) &voccom_)

struct {
    integer atloc[150], link[200], place[100], fixed[100], holdng;
} placom_;

#define placom_1 placom_

struct {
    integer mtext[35];
} mtxcom_;

#define mtxcom_1 mtxcom_

struct {
    integer ptext[100];
} ptxcom_;

#define ptxcom_1 ptxcom_

struct {
    integer abb[150];
} abbcom_;

#define abbcom_1 abbcom_

struct wizcom_1_ {
    integer wkday, wkend, holid, hbegin, hend, short__, magnm, latncy, saved,
	    savet, setup;
    char magic[5], hname[20];
};

#define wizcom_1 (*(struct wizcom_1_ *) &wizcom_)

struct motcom_1_ {
    integer msglin[50];
    char msg[500];
};

#define motcom_1 (*(struct motcom_1_ *) &motcom_)

/* Initialized data */

struct {
    integer e_1[50];
    char e_2[500];
    } motcom_ = { -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
	    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
	    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
	    -1, -1, -1, "                                                   "
	    "                                                                "
	    "                                                                "
	    "                                                                "
	    "                                                                "
	    "                                                                "
	    "                                                                "
	    "                                                                 "
	     };

struct {
    logical e_1;
    } blkcom_ = { TRUE_ };

struct {
    integer fill_1[675];
    integer e_2;
    } voccom_ = { {0}, 300 };

struct {
    integer fill_1[10];
    integer e_2;
    char fill_3[25];
    } wizcom_ = { {0}, 0 };


/* Table of constant values */

static integer c__1 = 1;
static integer c__9 = 9;
static integer c__0 = 0;
static integer c__6 = 6;
static integer c__2 = 2;
static integer c__20 = 20;
static integer c__3 = 3;
static integer c__4 = 4;
static integer c__8 = 8;
static integer c__7 = 7;
static integer c__48251 = 48251;
static integer c__1000 = 1000;
static logical c_false = FALSE_;
static integer c__65 = 65;
static integer c__130 = 130;
static integer c__95 = 95;
static integer c__100 = 100;
static integer c__5 = 5;
static integer c__50 = 50;
static integer c__127 = 127;
static integer c__128 = 128;
static integer c__186 = 186;
static integer c__35 = 35;
static integer c__141 = 141;
static integer c__25 = 25;
static integer c__17 = 17;
static integer c_n1 = -1;
static integer c__22 = 22;
static integer c__23 = 23;
static integer c__24 = 24;
static integer c__26 = 26;
static integer c__117 = 117;
static integer c__162 = 162;
static integer c__91 = 91;
static integer c__140 = 140;
static integer c__15 = 15;
static integer c__57 = 57;
static integer c__58 = 58;
static integer c__54 = 54;
static integer c__131 = 131;
static integer c__92 = 92;
static integer c__27 = 27;
static integer c__30 = 30;
static integer c__154 = 154;
static integer c__163 = 163;
static integer c__105 = 105;
static integer c__39 = 39;
static integer c__40 = 40;
static integer c__16 = 16;
static integer c__49 = 49;
static integer c__99 = 99;
static integer c__145 = 145;
static integer c__143 = 143;
static integer c__192 = 192;
static integer c__193 = 193;
static integer c__197 = 197;
static integer c__199 = 199;
static integer c__200 = 200;
static integer c__175 = 175;
static integer c__129 = 129;
static integer c__115 = 115;
static integer c__116 = 116;
static integer c__132 = 132;
static integer c__188 = 188;
static integer c__184 = 184;
static integer c__185 = 185;
static integer c__136 = 136;
static integer c__300 = 300;
static integer c__205 = 205;
static integer c__150 = 150;
static integer c__750 = 750;
static integer c__12 = 12;
static integer c__80 = 80;
static integer c__21 = 21;
static integer c__10 = 10;
static integer c__11 = 11;
static integer c__28 = 28;
static integer c__29 = 29;
static integer c__13 = 13;
static integer c__14 = 14;
static logical c_true = TRUE_;
static integer c__18 = 18;
static integer c__19 = 19;

/*  ADVENTURES */
/*  CURRENT LIMITS: */
/*    9650 WORDS OF MESSAGE TEXT (LINES, LINSIZ). */
/*     750 TRAVEL OPTIONS (TRAVEL, TRVSIZ). */
/*     300 VOCABULARY WORDS (KTAB, ATAB, TABSIZ). */
/*     150 LOCATIONS (LTEXT, STEXT, KEY, COND, ABB, ATLOC, LOCSIZ). */
/*     100 OBJECTS (PLAC, PLACE, FIXD, FIXED, LINK (TWICE), PTEXT, PROP). */
/*      35 "ACTION" VERBS (ACTSPK, VRBSIZ). */
/*     205 RANDOM MESSAGES (RTEXT, RTXSIZ). */
/*      12 DIFFERENT PLAYER CLASSIFICATIONS (CTEXT, CVAL, CLSMAX). */
/*      20 HINTS, LESS 3 (HINTLC, HINTED, HINTS, HNTSIZ). */
/*      35 MAGIC MESSAGES (MTEXT, MAGSIZ). */
/*  THERE ARE ALSO LIMITS WHICH CANNOT BE EXCEEDED DUE TO THE STRUCTURE OF */
/*  THE DATABASE.  (E.G., THE VOCABULARY USES N/1000 TO DETERMINE WORD TYPE, */
/*  SO THERE CAN'T BE MORE THAN 1000 WORDS.)  THESE UPPER LIMITS ARE: */
/*     1000 NON-SYNONYMOUS VOCABULARY WORDS */
/*     300 LOCATIONS */
/*     100 OBJECTS */

/*  ==================================================================== */
/*  PORTING NOTES: */

/*   IN GENERAL, I LEFT EXISTING COMMENTS AS-IS, WHICH MEANS THAT MANY */
/*   OF THEM ARE NOW INCORRECT, BUT HISTORICAL INFORMATION HAS BEEN */
/*   PRESERVED. WHERE I ADDED MY OWN, THEY ARE PREFIXED BY 'PORTING */
/*   NOTE:' OR SOME SUCH. */

/*   BESIDES THAT, THE GOAL WAS TO LEAVE THE ORIGINAL CODE AS ORIGINAL */
/*   AS POSSIBLE. IN PARTICULAR, ANYTHING THAT CHANGED THE ACTUAL */
/*   BEHAVIOR OF THE GAME WAS STRICTLY OFF-LIMITS, AS I WANTED TO HAVE */
/*   A CLEAN PORT OF THE HISTORICAL ORIGINAL. */

/*   TO CONVERT THE CODE INTO STANDARD, PORTABLE FORTRAN REQUIRED THAT I */
/*   CHANGE 'G' FORMAT SPECIFIERS TO I8, AND THIS IN TURN REQUIRED THAT */
/*   I CONVERT THE DATA FILE FROM A TAB-DELIMITED AFFAIR TO A STANDARD */
/*   FORTRAN FORMAT WITH FIXED-SIZE FIELDS. A SMALL FORTRAN PROGRAM WAS */
/*   WRITTEN TO ACCOMPLISH THAT CONVERSION. */

/*   SPEAKING OF TABS, THIS SOURCE FILE WAS ALSO FULL OF THEM, AND FOR */
/*   SOME ODD REASON THE DATA FILE HAD THEM IN STRANGE PLACES, LIKE */
/*   BETWEEN SOME SENTENCES. ALL TABS IN THIS SOURCE WERE CONVERTED TO */
/*   SPACES, WITH CORRESPONDING FORMATTING CLEANUP, AND IN THE DATA */
/*   FILE, ALL TABS NOT DELIMITING FIELDS WERE CONVERTED TO TWO SPACES */
/*   EACH. */

/*   IT WAS NECESSARY TO REPLACE NUMERIC USES OF LOGICAL OPERATORS WITH */
/*   SOMETHING EQUIVALENT, AS THESE DON'T EXIST IN STANDARD FORTRAN. */
/*   ALSO, DEPRECATED I/O STATEMENTS LIKE TYPE WERE REPLACED WITH */
/*   FORTRAN 77 STANDARD EQUIVALENTS. */

/*   THE FOLLOWING NOTES PERTAIN TO CHANGES MADE TO VARIABLES, ETC. */

/*     LINES:  WAS AN ARRAY OF 9650 INTEGERS. USAGE WAS INTERSPERSED */
/*             TEXT AND INDICES, WITH AN INDEX AT THE BEGINNING OF EACH */
/*             LINE THAT MARKED THE LINES ARRAY INDEX OF THE FOLLOWING */
/*             LINE, AND WHICH WAS NEGATED IF THIS WAS THE FIRST LINE OF */
/*             A VALUE OR THE LAST INDEX OF THE DATA (-1). */

/*             ALL OF THE TEXT IS NOW STORED IN THE CHARACTER VARIABLE */
/*             LINTXT, AND AN ARRAY LINIDX STORES THE INDICES OF THE */
/*             LINES. GIVEN AN INDEX INTO LINIDX, YOU CAN GET THE */
/*             CHARACTER INDEX OF THE START OF THE LINE, AND THE INDEX */
/*             OF THE START OF THE FOLLOWING LINE IS AT THAT INDEX PLUS */
/*             ONE. A NEGATIVE ENTRY INDICATES THE FIRST LINE OF AN */
/*             ITEM. LINSIZ, WHICH WAS THE SIZE OF THE LINES ARRAY, HAS */
/*             BEEN REPLACED BY THE TWO CONSTANTS LINMAX AND TXTMAX. THE */
/*             FORMER IS THE MAXIMUM NUMBER OF LINES, WHILE THE LATTER */
/*             IS THE MAXIMUM CHARACTERS OF TEXT. LIDUSE HAS BEEN ADDED. */
/*             IT IS THE INDEX INTO LINIDX THAT CORRESPONDS WITH THE */
/*             NEXT LINE INDEX TO BE ADDED (WHICH IS ALSO THE INDEX OF */
/*             THE CHAR INDEX ONE GREATER THAN THE LAST CHARACTER OF */
/*             THE LAST LINE STORED IN LINTXT). THERE ARE LIDUSE-1 TOTAL */
/*             LINES OF TEXT IN LINTXT, WITH INDICES IN LINIDX. THE */
/*             VALUE OF LINIDX(LIDUSE) IS ONE CHARACTER POSITION PAST */
/*             THE END OF THE LAST CHARACTER IN LINTXT, NEGATED. */

/*             THE GENERAL DESIGN OF THIS PROGRAM IS THAT ALL MESSAGES */
/*             OF ANY KIND ARE STORED THIS WAY AND IDENTIFIED BY */
/*             NUMBERS. THE ORIGINAL INTERPRETATION OF THOSE NUMBERS */
/*             WAS AS INDICES INTO LINES. NOW THERE IS AN EXTRA LEVEL OF */
/*             INDIRECTION, AS THEY INDEX LINIDX. ONE UPSHOT IS THAT */
/*             CERTAIN THINGS BECOME EASIER. FOR EXAMPLE, TO SKIP A */
/*             MESSAGE USED TO REQUIRE WALKING THE ENTRIES IN LINES AS */
/*             A LINKED LIST. NOW IT CAN BE DONE SIMPLY BY SCANNING */
/*             FORWARD IN LINIDX FOR THE NEXT NEGATIVE ENTRY. */

/*     AA:     SINGLE CHARACTER READ AT THE END OF A LINE FROM THE DATA */
/*             FILE. WE EXPECT IT TO ALWAYS BE A PADDED BLANK. IF NOT, */
/*             THE LINE IS TOO LONG. WAS ORIGINALLY A NUMERIC VARIABLE. */

/*     ATAB:   THE TABLE OF VOCABULARY WORDS, WHICH WAS ORIGINALLY AN */
/*             ARRAY OF INTEGERS, WHERE 5 CHARACTERS WERE STORED IN EACH */
/*             INTEGER. THIS HAS BEEN CHANGED TO A CHARACTER TYPE. */

/*     PHROG:  A CHARACTER FUNCTION ADDED TO ACCOMPLISH THE OBFUSCATION */
/*             THAT THE ORIGINAL PROGRAM DID WITH THE NON-STANDARD .XOR. */
/*             OPERATOR. */

/*     GETIN:  THIS SUBROUTINED USED TO RETURN 4 INTEGER VALUES IN */
/*             VARIABLES TO REPRESENT INPUT BROKEN INTO WORDS. IT HAS */
/*             BEEN CHANGED TO RETURN 4 CHARACTER*5 VALUES INSTEAD. */

/*       WD1, WD1X, WD2, WD2X: */
/*             THESE WERE INTEGER VARIABLES HOLDING WORDS IN THE */
/*             ORIGINAL CODE. IN THE PORT THEY ARE CHARACTER*5 VALUES. */

/*     BLANKS: A CHARACTER*5 VALUE HOLDING 5 BLANKS. USED FOR */
/*             COMPARISON PURPOSES TO TELL IF WD2 IS EMPTY. */

/*         A1: PREVIOUSLY TK, A 20-ELEMENT INTEGER ARRAY, WAS USED FOR */
/*             LONG VERSIONS OF WORDS IN MESSAGES GENERATED WITH */
/*             A5TOA1. OUR PORTED VERSION NEEDS A CHARACTER*1 ARRAY, AND */
/*             TK CAN'T BE CHANGED, BECAUSE IT IS USED FOR OTHER */
/*             PURPOSES AS WELL. SO A1 WAS ADDED FOR THIS PURPOSE. */

/*         A5: USED AS A MULTI-PURPOSE 5-CHARACTER VARIABLE. TAKES THE */
/*             PLACE OF WHAT WERE MISCELLANEOUS USES OF NUMERIC */
/*             VARIABLES TO STORE SHORT STRINGS. */

/*      SHIFT: WAS A FUNCTION TO PERFORM A BITWISE  SHIFT. IT HAD TO BE */
/*             REWRITTEN SO AS NOT TO USE NON-PORTABLE OPERATORS. I */
/*             SIMPLIFIED IT TO A SIMPLE SHIFT (NOT ROTATE AS BEFORE) */
/*             THAT ONLY WORKS WITH POSITIVE NUMBERS. THIS WORKS GIVEN */
/*             THE USES OF THIS FUNCTION THAT REMAIN. */

/*  ONE MAJOR PORTING CHALLENGE WAS FILLING IN FOR THE MISSING PDP-10 */
/*  FUNCTIONALITY OF SAVING A CORE IMAGE OF A RUNNING PROGRAM. THIS */
/*  FEATURE OF THE SYSTEM EVIDENTLY ALLOWS A SNAPSHOT TO BE TAKEN OF THE */
/*  STATE OF A PROGRAM, AND THEN THE PROGRAM CAN LATER BE STARTED AGAIN */
/*  FROM THE BEGINNING WITH ALL OF THE VARIABLES RESTORED TO THEIR PRIOR */
/*  STATE. THIS FUNCTIONALITY WAS USED FOR TWO PURPOSES IN THE ORIGINAL. */
/*  FIRST, GENERALLY THE PROGRAM WAS ONLY INITIALIZED FROM THE DATA FILE */
/*  ON ITS FIRST "COLD" RUN. THEREAFTER A CORE IMAGE OF IT WAS SAVED, */
/*  AND THIS WAS RUN. THIS ALLOWED THE PROGRAM TO RUN WITHOUT NEEDING */
/*  ACCESS TO THE DATA FILE, WHICH, IN TURN, HELPED TO PREVENT IT FROM */
/*  BEING REVERSE ENGINEERED IN ORDER TO CHEAT IN THE GAME. ALSO, ALL */
/*  OF THE MODIFICATIONS MADE BY WIZARDS (CHANGING CAVE HOURS, ETC.) */
/*  WERE MADE PERSISTENT BY THIS MECHANISM. AFTER THE CHANGES WERE MADE */
/*  TO THE PROGRAM'S INTERNAL STATE, IT SIMPLY TERMINATED AND ALLOWED A */
/*  CORE IMAGE TO BE SAVED WITH THE NEW STATE. THE OTHER ASPECT OF THE */
/*  GAME THAT MADE USE OF THIS FEATURE WAS SUSPENDING THE USER'S GAME. */
/*  AGAIN, THE PROGRAM SIMPLY STOPPED, AND THE USER WOULD SAVE A CORE */
/*  IMAGE IN ORDER TO SAVE THEIR PROGRESS. */

/*  IN THIS PORT, BINARY FILES HAVE TAKEN THE PLACE OF CORE IMAGES. ON */
/*  A COLD RUN, OR AFTER A WIZARD HAS MADE MODIFICATIONS, THE GAME SAVES */
/*  THE STATE OF ALL ITS VARIABLES TO A FILE. ON STARTUP, THEN, IF THIS */
/*  FILE EXISTS, GAME STATE IS READ FROM IT, AND THE DATA FILE IS NOT */
/*  NEEDED. WHEN A GAME IS SUSPENDED, THE USER IS PROMPTED FOR THE PATH */
/*  TO A SAVE FILE, AND A RESUME COMMAND HAS BEEN ADDED IN ORDER TO */
/*  PERMIT THE GAME TO BE RESUMED FROM SUCH A FILE, SUBJECT TO THE */
/*  NORMAL CONSTRAINTS. */
/*  ==================================================================== */

/* Main program */ int MAIN__(void)
{
    /* Initialized data */

    static integer trvsiz = 750;
    static integer key[150] = { 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
	    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
	    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
	    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
	    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0 };
    static integer fixd[100] = { 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
	    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
	    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
	    0,0,0,0,0,0,0,0,0,0,0,0 };
    static integer plac[100] = { 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
	    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
	    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
	    0,0,0,0,0,0,0,0,0,0,0,0 };
    static integer locsiz = 150;
    static integer vrbsiz = 35;
    static integer rtxsiz = 205;
    static integer clsmax = 12;
    static integer hntsiz = 20;
    static integer magsiz = 35;
    static char blanks[5] = "     ";
    static logical needsv = FALSE_;

    /* Format strings */
    static char fmt_1000[] = "(\002 Initializing...\002)";
    static char fmt_1003[] = "(i8)";
    static char fmt_1005[] = "(i8,a70,a)";
    static char fmt_1031[] = "(99i8)";
    static char fmt_1041[] = "(i8,a5)";
    static char fmt_1999[] = "(\002 Table space used:\002/\002 \002,i6,\002 "
	    "OF \002,i6,\002 characters of messages\002/\002 \002,i6,\002 OF"
	    " \002,i6,\002 lines of messages\002/\002 \002,i6,\002 OF \002,i6,"
	    "\002 travel options\002/\002 \002,i6,\002 OF \002,i6,\002 vocabu"
	    "lary words\002/\002 \002,i6,\002 OF \002,i6,\002 locations\002"
	    "/\002 \002,i6,\002 OF \002,i6,\002 objects\002/\002 \002,i6,\002"
	    " OF \002,i6,\002 action verbs\002/\002 \002,i6,\002 OF \002,i6"
	    ",\002 RTEXT messages\002/\002 \002,i6,\002 OF \002,i6,\002 CLASS"
	    " messages\002/\002 \002,i6,\002 OF \002,i6,\002 hints\002/\002"
	    " \002,i6,\002 OF \002,i6,\002 MAGIC messages\002/)";
    static char fmt_67[] = "(/\002 There are \002,i1,\002 threatening little"
	    " dwarves in the\002,\002 room with you.\002)";
    static char fmt_78[] = "(/\002 \002,i1,\002 of them throw knives at you"
	    "!\002)";
    static char fmt_68[] = "(/\002 \002,i1,\002 of them get you!\002)";
    static char fmt_5015[] = "(/\002 What do you want to do with the \002,20"
	    "a1)";
    static char fmt_5199[] = "(/\002 I see no \002,20a1)";
    static char fmt_8002[] = "(/\002 \002,20a1)";
    static char fmt_9032[] = "(/\002 Okay, \"\002,20a1)";
    static char fmt_8243[] = "(/\002 If you were to quit now, you would sc"
	    "ore\002,i4,\002 out of a possible\002,i4,\002.\002)";
    static char fmt_8309[] = "(/\002Saving is disabled in this build.\002)";
    static char fmt_8302[] = "(/\002 I can suspend your adventure for you so"
	    " that you can\002,\002 resume later, but\002/\002 you will have "
	    "to wait at least\002,i3,\002 minutes before continuing.\002)";
    static char fmt_45678[] = "(/\002Please enter a file path:\002)";
    static char fmt_45687[] = "(/\002Okay, we won't suspend the game just no"
	    "w.\002)";
    static char fmt_8329[] = "(/\002Loading/resume is disabled in this bui"
	    "ld.\002)";
    static char fmt_8322[] = "(\002/Okay, we won't resume another game just "
	    "now.\002)";
    static char fmt_40012[] = "(/\002 I am prepared to give you a hint, but "
	    "it will cost you\002,i2,\002 points.\002)";
    static char fmt_20100[] = "(///\002 You scored\002,i4,\002 out of a poss"
	    "ible\002,i4,\002, using\002,i5,\002 turns.\002)";
    static char fmt_20202[] = "(/\002 You just went off my scale!!\002/)";
    static char fmt_20212[] = "(/\002 To achieve the next higher rating, you"
	    " need\002,i3,\002 more point\002,a2/)";
    static char fmt_20222[] = "(/\002 To achieve the next higher rating"
	    " \002,\002would be a neat trick!\002//\002 Congratulations!!\002"
	    "/)";
    static char fmt_45681[] = "(/\002ERROR: Unable to open \002,a)";
    static char fmt_56791[] = "(/\002ERROR: Unable to open \002,a)";

    /* System generated locals */
    integer i__1, i__2, i__3, i__4;
    char ch__1[5];
    olist o__1;
    cllist cl__1;
    alist al__1;

    /* Builtin functions */
    integer s_wsfe(cilist *), e_wsfe(void), f_open(olist *), f_rew(alist *),
	    s_rsfe(cilist *), do_fio(integer *, char *, ftnlen), e_rsfe(void);
    /* Subroutine */ int s_copy(char *, char *, ftnlen, ftnlen);
    integer pow_ii(integer *, integer *), s_cmp(char *, char *, ftnlen,
	    ftnlen);
    /* Subroutine */ int s_stop(char *, ftnlen);
    integer s_wsue(cilist *), do_uio(integer *, char *, ftnlen), e_wsue(void),
	     f_clos(cllist *), s_wsle(cilist *), do_lio(integer *, integer *,
	    char *, ftnlen), e_wsle(void), s_rsue(cilist *), e_rsue(void);

    /* Local variables */
    static integer rnlinuse, i__, j, k, l, q;
    static char a1[1*20], a5[5];
    static integer k2;
    static char aa[1];
    static integer kk, ll, kq, tk[20];
    static char wd1[5], wd2[5];
    static integer obj, loc;
    static logical yea;
    extern integer ran_(integer *);
    extern /* Subroutine */ int bug_(integer *);
    static integer axe, oil, rod, foo, say, rug, spk;
    extern logical yes_(integer *, integer *, integer *);
    extern integer put_(integer *, integer *, integer *);
    static integer rod2;
    static char wd1x[5], wd2x[5];
    static integer cage, back, bear;
    extern /* Subroutine */ int ciao_(void);
    static integer clam, dloc[6], cond[150], cval[12];
    static logical demo;
    static integer bird, lamp, food, vend, sect, verb, eggs, vase, cave, door;
    extern /* Subroutine */ int drop_(integer *, integer *);
    static integer look, lock, find;
    extern /* Subroutine */ int poof_(void), motd_(logical *);
    static integer null, keys;
    extern /* Subroutine */ int move_(integer *, integer *);
    static integer hint, ssvd, prop[100], trvs, ssvt, sstu, dflag, chain,
	    chloc;
    static logical panic;
    extern integer vocab_(char *, integer *, ftnlen);
    static integer knife, chasm;
    static logical dseen[6];
    static integer odloc[6], grate, snake, dwarf, chest, pearl, coins, dkill;
    extern integer shift_(integer *, integer *);
    static integer plant;
    extern /* Character */ VOID phrog_(char *, ftnlen, char *, ftnlen);
    static integer water, limit, hints[80]	/* was [20][4] */, tally,
	    ctext[12], pyram, bonus, troll;
    extern logical start_(integer *);
    static integer steps, ltext[150], throw, iwest, stext[150], stick;
    extern /* Subroutine */ int carry_(integer *, integer *);
    static integer chloc2, turns, clock1, clock2;
    extern /* Subroutine */ int speak_(integer *);
    static integer oldlc2;
    extern /* Subroutine */ int getin_(char *, char *, char *, char *, ftnlen,
	     ftnlen, ftnlen, ftnlen), maint_(logical *), a5toa1_(char *, char
	    *, char *, char *, integer *, ftnlen, ftnlen, ftnlen, ftnlen);
    static integer score;
    extern /* Subroutine */ int hours_(void);
    static integer plant2, tally2, troll2, emrald, daltlc, detail, abbnum,
	    maxdie;
    static logical closed, hinted[20];
    static integer oldloc, tablet, tabndx, hintlc[20];
    static char savfil[256];
    static integer actspk[35], idondx, travel[750];
    static logical closng, lmwarn, gaveup, wzdark, scorng;
    static integer bottle, linuse, liduse, clsses, newloc, hntmax, maxtrs,
	    pillow, fissur, magzin, oyster, mirror, dragon, messag, batter,
	    nugget, tridnt, entrnc, dprssn, invent, knfloc, numdie, foobar;
    extern /* Subroutine */ int rspeak_(integer *);
    static integer dtotal, attack;
    extern /* Subroutine */ int pspeak_(integer *, integer *), juggle_(
	    integer *);
    static integer spices;
    extern /* Subroutine */ int dstroy_(integer *);
    static integer mxscor;
    extern /* Subroutine */ int datime_(integer *, integer *), mspeak_(
	    integer *);

    /* Fortran I/O blocks */
    static cilist io___13 = { 0, 6, 0, fmt_1000, 0 };
    static cilist io___23 = { 0, 1, 0, fmt_1003, 0 };
    static cilist io___26 = { 0, 1, 0, fmt_1005, 0 };
    static cilist io___31 = { 0, 1, 0, fmt_1031, 0 };
    static cilist io___37 = { 0, 1, 0, fmt_1041, 0 };
    static cilist io___38 = { 0, 1, 0, fmt_1031, 0 };
    static cilist io___42 = { 0, 1, 0, fmt_1031, 0 };
    static cilist io___45 = { 0, 1, 0, fmt_1031, 0 };
    static cilist io___47 = { 0, 1, 0, fmt_1031, 0 };
    static cilist io___135 = { 0, 6, 0, fmt_1999, 0 };
    static cilist io___143 = { 0, 6, 0, fmt_67, 0 };
    static cilist io___144 = { 0, 6, 0, fmt_78, 0 };
    static cilist io___145 = { 0, 6, 0, fmt_68, 0 };
    static cilist io___156 = { 0, 6, 0, fmt_5015, 0 };
    static cilist io___157 = { 0, 6, 0, fmt_5199, 0 };
    static cilist io___162 = { 0, 6, 0, fmt_8002, 0 };
    static cilist io___163 = { 0, 6, 0, fmt_9032, 0 };
    static cilist io___164 = { 0, 6, 0, fmt_8243, 0 };
    static cilist io___167 = { 0, 6, 0, fmt_8309, 0 };
    static cilist io___168 = { 0, 6, 0, fmt_8302, 0 };
    static cilist io___169 = { 0, 6, 0, fmt_45678, 0 };
    static cilist io___170 = { 0, 5, 0, "(A)", 0 };
    static cilist io___172 = { 0, 6, 0, fmt_45687, 0 };
    static cilist io___176 = { 0, 6, 0, fmt_8329, 0 };
    static cilist io___177 = { 0, 5, 0, "(A)", 0 };
    static cilist io___178 = { 0, 6, 0, fmt_8322, 0 };
    static cilist io___179 = { 0, 6, 0, fmt_40012, 0 };
    static cilist io___181 = { 0, 6, 0, fmt_20100, 0 };
    static cilist io___182 = { 0, 6, 0, fmt_20202, 0 };
    static cilist io___184 = { 0, 6, 0, fmt_20212, 0 };
    static cilist io___185 = { 0, 6, 0, fmt_20222, 0 };
    static cilist io___186 = { 1, 2, 0, 0, 0 };
    static cilist io___188 = { 0, 6, 0, 0, 0 };
    static cilist io___189 = { 0, 6, 0, 0, 0 };
    static cilist io___190 = { 0, 6, 0, 0, 0 };
    static cilist io___191 = { 1, 2, 0, 0, 0 };
    static cilist io___192 = { 0, 6, 0, "(/,A)", 0 };
    static cilist io___193 = { 1, 2, 0, 0, 0 };
    static cilist io___195 = { 0, 6, 0, fmt_45681, 0 };
    static cilist io___196 = { 0, 6, 0, "(/,A)", 0 };
    static cilist io___197 = { 1, 2, 0, 0, 0 };
    static cilist io___198 = { 0, 6, 0, fmt_56791, 0 };
    static cilist io___199 = { 0, 6, 0, "(/,A)", 0 };

    /* Tell the global peek pointer where the main "loc" lives */
    /* ------ PEEK POINTERS TO LOCAL VARS FOR SNIFFING BY JS RUNTIME -------*/
    advent_loc_ptr = &loc; // Current location
    advent_oldloc_ptr = &oldloc;
    advent_newloc_ptr = &newloc;
    advent_dflag_ptr = &dflag;
    advent_dloc_ptr = &dloc[0];
    advent_odloc_ptr = &odloc[0];
    advent_dseen_ptr = &dseen[0];


    /* Additional peek pointers for JS overlays */
    advent_cond_ptr = &cond[0];
    advent_prop_ptr = &prop[0];
    advent_place_ptr = &placom_1.place[0];
    advent_fixed_ptr = &placom_1.fixed[0];

    /* -------------------------------------------------------------------- */

/*  CHARACTER VARIABLES, FUNCTIONS ADDED IN PORT */

/*  DIMENSION LINES(9650) */

/* AVOID MAKING THE COMPILER WORRY ABOUT MODIFYING THE DO INDEX */
/*  STATEMENT FUNCTIONS */

/*  TOTING(OBJ) = TRUE IF THE OBJ IS BEING CARRIED */
/*  HERE(OBJ)   = TRUE IF THE OBJ IS AT "LOC" (OR IS BEING CARRIED) */
/*  AT(OBJ)     = TRUE IF ON EITHER SIDE OF TWO-PLACED OBJECT */
/*  LIQ(DUMMY)  = OBJECT NUMBER OF LIQUID IN BOTTLE */
/*  LIQLOC(LOC) = OBJECT NUMBER OF LIQUID (IF ANY) AT LOC */
/*  ISSET(I,N)  = TRUE IF BIT N OF INTEGER I IS SET (ADDED IN PORT) */
/*  BITSET(L,N) = TRUE IF COND(L) HAS BIT N SET (BIT 0 IS UNITS BIT) */
/*  FORCED(LOC) = TRUE IF LOC MOVES WITHOUT ASKING FOR INPUT (COND=2) */
/*  DARK(DUMMY) = TRUE IF LOCATION "LOC" IS DARK */
/*  PCT(N)      = TRUE N% OF THE TIME (N INTEGER FROM 0 TO 100) */

/*  WZDARK SAYS WHETHER THE LOC HE'S LEAVING WAS DARK */
/*  LMWARN SAYS WHETHER HE'S BEEN WARNED ABOUT LAMP GOING DIM */
/*  CLOSNG SAYS WHETHER ITS CLOSING TIME YET */
/*  PANIC SAYS WHETHER HE'S FOUND OUT HE'S TRAPPED IN THE CAVE */
/*  CLOSED SAYS WHETHER WE'RE ALL THE WAY CLOSED */
/*  GAVEUP SAYS WHETHER HE EXITED VIA "QUIT" */
/*  SCORNG INDICATES TO THE SCORE ROUTINE WHETHER WE'RE DOING A "SCORE" COMMAND */
/*  DEMO IS TRUE IF THIS IS A PRIME-TIME DEMONSTRATION GAME */
/*  YEA IS RANDOM YES/NO REPLY */

/*     DATA LINSIZ/9650/,TRVSIZ/750/,TABSIZ/300/,LOCSIZ/150/, */

/*  DESCRIPTION OF THE DATABASE FORMAT */


/*  THE DATA FILE CONTAINS SEVERAL SECTIONS.  EACH BEGINS WITH A LINE CONTAINING */
/*  A NUMBER IDENTIFYING THE SECTION, AND ENDS WITH A LINE CONTAINING "-1". */

/*  SECTION 1: LONG FORM DESCRIPTIONS.  EACH LINE CONTAINS A LOCATION NUMBER, */
/*     A TAB, AND A LINE OF TEXT.  THE SET OF (NECESSARILY ADJACENT) LINES */
/*     WHOSE NUMBERS ARE X FORM THE LONG DESCRIPTION OF LOCATION X. */
/*  SECTION 2: SHORT FORM DESCRIPTIONS.  SAME FORMAT AS LONG FORM.  NOT ALL */
/*     PLACES HAVE SHORT DESCRIPTIONS. */
/*  SECTION 3: TRAVEL TABLE.  EACH LINE CONTAINS A LOCATION NUMBER (X), A SECOND */
/*     LOCATION NUMBER (Y), AND A LIST OF MOTION NUMBERS (SEE SECTION 4). */
/*     EACH MOTION REPRESENTS A VERB WHICH WILL GO TO Y IF CURRENTLY AT X. */
/*     Y, IN TURN, IS INTERPRETED AS FOLLOWS.  LET M=Y/1000, N=Y MOD 1000. */
/*       IF N<=300      IT IS THE LOCATION TO GO TO. */
/*       IF 300<N<=500  N-300 IS USED IN A COMPUTED GOTO TO */
/*                      A SECTION OF SPECIAL CODE. */
/*       IF N>500       MESSAGE N-500 FROM SECTION 6 IS PRINTED, */
/*                      AND HE STAYS WHEREVER HE IS. */
/*     MEANWHILE, M SPECIFIES THE CONDITIONS ON THE MOTION. */
/*       IF M=0         IT'S UNCONDITIONAL. */
/*       IF 0<M<100     IT IS DONE WITH M% PROBABILITY. */
/*       IF M=100       UNCONDITIONAL, BUT FORBIDDEN TO DWARVES. */
/*       IF 100<M<=200  HE MUST BE CARRYING OBJECT M-100. */
/*       IF 200<M<=300  MUST BE CARRYING OR IN SAME ROOM AS M-200. */
/*       IF 300<M<=400  PROP(M MOD 100) MUST *NOT* BE 0. */
/*       IF 400<M<=500  PROP(M MOD 100) MUST *NOT* BE 1. */
/*       IF 500<M<=600  PROP(M MOD 100) MUST *NOT* BE 2, ETC. */
/*     IF THE CONDITION (IF ANY) IS NOT MET, THEN THE NEXT *DIFFERENT* */
/*     "DESTINATION" VALUE IS USED (UNLESS IT FAILS TO MEET *ITS* CONDITIONS, */
/*     IN WHICH CASE THE NEXT IS FOUND, ETC.).  TYPICALLY, THE NEXT DEST WILL */
/*     BE FOR ONE OF THE SAME VERBS, SO THAT ITS ONLY USE IS AS THE ALTERNATE */
/*     DESTINATION FOR THOSE VERBS.  FOR INSTANCE: */
/*       15  110022  29  31  34  35  23  43 */
/*       15  14      29 */
/*     THIS SAYS THAT, FROM LOC 15, ANY OF THE VERBS 29, 31, ETC., WILL TAKE */
/*     HIM TO 22 IF HE'S CARRYING OBJECT 10, AND OTHERWISE WILL GO TO 14. */
/*       11  303008  49 */
/*       11  9       50 */
/*     THIS SAYS THAT, FROM 11, 49 TAKES HIM TO 8 UNLESS PROP(3)=0, IN WHICH */
/*     CASE HE GOES TO 9.  VERB 50 TAKES HIM TO 9 REGARDLESS OF PROP(3). */
/*  SECTION 4: VOCABULARY.  EACH LINE CONTAINS A NUMBER (N), A TAB, AND A */
/*     FIVE-LETTER WORD.  CALL M=N/1000.  IF M=0, THEN THE WORD IS A MOTION */
/*     VERB FOR USE IN TRAVELLING (SEE SECTION 3).  ELSE, IF M=1, THE WORD IS */
/*     AN OBJECT.  ELSE, IF M=2, THE WORD IS AN ACTION VERB (SUCH AS "CARRY" */
/*     OR "ATTACK").  ELSE, IF M=3, THE WORD IS A SPECIAL CASE VERB (SUCH AS */
/*     "DIG") AND N MOD 1000 IS AN INDEX INTO SECTION 6.  OBJECTS FROM 50 TO */
/*     (CURRENTLY, ANYWAY) 79 ARE CONSIDERED TREASURES (FOR PIRATE, CLOSEOUT). */
/*  SECTION 5: OBJECT DESCRIPTIONS.  EACH LINE CONTAINS A NUMBER (N), A TAB, */
/*     AND A MESSAGE.  IF N IS FROM 1 TO 100, THE MESSAGE IS THE "INVENTORY" */
/*     MESSAGE FOR OBJECT N.  OTHERWISE, N SHOULD BE 000, 100, 200, ETC., AND */
/*     THE MESSAGE SHOULD BE THE DESCRIPTION OF THE PRECEDING OBJECT WHEN ITS */
/*     PROP VALUE IS N/100.  THE N/100 IS USED ONLY TO DISTINGUISH MULTIPLE */
/*     MESSAGES FROM MULTI-LINE MESSAGES; THE PROP INFO ACTUALLY REQUIRES ALL */
/*     MESSAGES FOR AN OBJECT TO BE PRESENT AND CONSECUTIVE.  PROPERTIES WHICH */
/*     PRODUCE NO MESSAGE SHOULD BE GIVEN THE MESSAGE ">$<". */
/*  SECTION 6: ARBITRARY MESSAGES.  SAME FORMAT AS SECTIONS 1, 2, AND 5, EXCEPT */
/*     THE NUMBERS BEAR NO RELATION TO ANYTHING (EXCEPT FOR SPECIAL VERBS */
/*     IN SECTION 4). */
/*  SECTION 7: OBJECT LOCATIONS.  EACH LINE CONTAINS AN OBJECT NUMBER AND ITS */
/*     INITIAL LOCATION (ZERO (OR OMITTED) IF NONE).  IF THE OBJECT IS */
/*     IMMOVABLE, THE LOCATION IS FOLLOWED BY A "-1".  IF IT HAS TWO LOCATIONS */
/*     (E.G. THE GRATE) THE FIRST LOCATION IS FOLLOWED WITH THE SECOND, AND */
/*     THE OBJECT IS ASSUMED TO BE IMMOVABLE. */
/*  SECTION 8: ACTION DEFAULTS.  EACH LINE CONTAINS AN "ACTION-VERB" NUMBER AND */
/*     THE INDEX (IN SECTION 6) OF THE DEFAULT MESSAGE FOR THE VERB. */
/*  SECTION 9: LIQUID ASSETS, ETC.  EACH LINE CONTAINS A NUMBER (N) AND UP TO 20 */
/*     LOCATION NUMBERS.  BIT N (WHERE 0 IS THE UNITS BIT) IS SET IN COND(LOC) */
/*     FOR EACH LOC GIVEN.  THE COND BITS CURRENTLY ASSIGNED ARE: */
/*       0 LIGHT */
/*       1 IF BIT 2 IS ON: ON FOR OIL, OFF FOR WATER */
/*       2 LIQUID ASSET, SEE BIT 1 */
/*       3 PIRATE DOESN'T GO HERE UNLESS FOLLOWING PLAYER */
/*     OTHER BITS ARE USED TO INDICATE AREAS OF INTEREST TO "HINT" ROUTINES: */
/*       4 TRYING TO GET INTO CAVE */
/*       5 TRYING TO CATCH BIRD */
/*       6 TRYING TO DEAL WITH SNAKE */
/*       7 LOST IN MAZE */
/*       8 PONDERING DARK ROOM */
/*       9 AT WITT'S END */
/*     COND(LOC) IS SET TO 2, OVERRIDING ALL OTHER BITS, IF LOC HAS FORCED */
/*     MOTION. */
/*  SECTION 10: CLASS MESSAGES.  EACH LINE CONTAINS A NUMBER (N), A TAB, AND A */
/*     MESSAGE DESCRIBING A CLASSIFICATION OF PLAYER.  THE SCORING SECTION */
/*     SELECTS THE APPROPRIATE MESSAGE, WHERE EACH MESSAGE IS CONSIDERED TO */
/*     APPLY TO PLAYERS WHOSE SCORES ARE HIGHER THAN THE PREVIOUS N BUT NOT */
/*     HIGHER THAN THIS N.  NOTE THAT THESE SCORES PROBABLY CHANGE WITH EVERY */
/*     MODIFICATION (AND PARTICULARLY EXPANSION) OF THE PROGRAM. */
/*  SECTION 11: HINTS.  EACH LINE CONTAINS A HINT NUMBER (CORRESPONDING TO A */
/*     COND BIT, SEE SECTION 9), THE NUMBER OF TURNS HE MUST BE AT THE RIGHT */
/*     LOC(S) BEFORE TRIGGERING THE HINT, THE POINTS DEDUCTED FOR TAKING THE */
/*     HINT, THE MESSAGE NUMBER (SECTION 6) OF THE QUESTION, AND THE MESSAGE */
/*     NUMBER OF THE HINT.  THESE VALUES ARE STASHED IN THE "HINTS" ARRAY. */
/*     HNTMAX IS SET TO THE MAX HINT NUMBER (<= HNTSIZ).  NUMBERS 1-3 ARE */
/*     UNUSABLE SINCE COND BITS ARE OTHERWISE ASSIGNED, SO 2 IS USED TO */
/*     REMEMBER IF HE'S READ THE CLUE IN THE REPOSITORY, AND 3 IS USED TO */
/*     REMEMBER WHETHER HE ASKED FOR INSTRUCTIONS (GETS MORE TURNS, BUT LOSES */
/*     POINTS). */
/*  SECTION 12: MAGIC MESSAGES. IDENTICAL TO SECTION 6 EXCEPT PUT IN A SEPARATE */
/*     SECTION FOR EASIER REFERENCE.  MAGIC MESSAGES ARE USED BY THE STARTUP, */
/*     MAINTENANCE MODE, AND RELATED ROUTINES. */
/*  SECTION 0: END OF DATABASE. */
/*  READ THE DATABASE IF WE HAVE NOT YET DONE SO */
/*  ADDED IN PORT: READ STATE DATA FROM BIN FILE. THIS TAKES THE PLACE */
/*  OF USING CORE IMAGES ON THE PDP-10. */
    goto L23459;
L23459:
    if (wizcom_1.setup != 0) {
	goto L1100;
    }
    s_wsfe(&io___13);
    e_wsfe();
/*  CLEAR OUT THE VARIOUS TEXT-POINTER ARRAYS.  ALL TEXT IS STORED IN ARRAY */
/*  LINES; EACH LINE IS PRECEDED BY A WORD POINTING TO THE NEXT POINTER (I.E. */
/*  THE WORD FOLLOWING THE END OF THE LINE).  THE POINTER IS NEGATIVE IF THIS IS */
/*  FIRST LINE OF A MESSAGE.  THE TEXT-POINTER ARRAYS CONTAIN INDICES OF */
/*  POINTER-WORDS IN LINES.  STEXT(N) IS SHORT DESCRIPTION OF LOCATION N. */
/*  LTEXT(N) IS LONG DESCRIPTION.  PTEXT(N) POINTS TO MESSAGE FOR PROP(N)=0. */
/*  SUCCESSIVE PROP MESSAGES ARE FOUND BY CHASING POINTERS.  RTEXT CONTAINS */
/*  SECTION 6'S STUFF.  CTEXT(N) POINTS TO A PLAYER-CLASS MESSAGE.  MTEXT IS FOR */
/*  SECTION 12.  WE ALSO CLEAR COND.  SEE DESCRIPTION OF SECTION 9 FOR DETAILS. */
    for (i__ = 1; i__ <= 300; ++i__) {
	if (i__ <= 100) {
	    ptxcom_1.ptext[i__ - 1] = 0;
	}
	if (i__ <= rtxsiz) {
	    txtcom_1.rtext[i__ - 1] = 0;
	}
	if (i__ <= clsmax) {
	    ctext[i__ - 1] = 0;
	}
	if (i__ <= magsiz) {
	    mtxcom_1.mtext[i__ - 1] = 0;
	}
	if (i__ > locsiz) {
	    goto L1001;
	}
	stext[i__ - 1] = 0;
	ltext[i__ - 1] = 0;
	cond[i__ - 1] = 0;
L1001:
	;
    }
    o__1.oerr = 0;
    o__1.ounit = 1;
    o__1.ofnmlen = 10;
    o__1.ofnm = "advent.dat";
    o__1.orl = 0;
    o__1.osta = "OLD";
    o__1.oacc = "SEQUENTIAL";
    o__1.ofm = 0;
    o__1.oblnk = 0;
    f_open(&o__1);
    al__1.aerr = 0;
    al__1.aunit = 1;
    f_rew(&al__1);
    wizcom_1.setup = 1;
    linuse = 1;
    liduse = 1;
    txtcom_1.linidx[0] = -1;
    trvs = 1;
    clsses = 1;
/*  START NEW DATA SECTION.  SECT IS THE SECTION NUMBER. */
L1002:
    s_rsfe(&io___23);
    do_fio(&c__1, (char *)&sect, (ftnlen)sizeof(integer));
    e_rsfe();
    oldloc = -1;
    switch (sect + 1) {
	case 1:  goto L1100;
	case 2:  goto L1004;
	case 3:  goto L1004;
	case 4:  goto L1030;
	case 5:  goto L1040;
	case 6:  goto L1004;
	case 7:  goto L1004;
	case 8:  goto L1050;
	case 9:  goto L1060;
	case 10:  goto L1070;
	case 11:  goto L1004;
	case 12:  goto L1080;
	case 13:  goto L1004;
    }
/*           (0)  (1)  (2)  (3)  (4)  (5)  (6)  (7)  (8)  (9)  (10) */
/*          (11) (12) */
    bug_(&c__9);
/*  SECTIONS 1, 2, 5, 6, 10, 12.  READ MESSAGES AND SET UP POINTERS. */
L1004:
    s_rsfe(&io___26);
    do_fio(&c__1, (char *)&loc, (ftnlen)sizeof(integer));
    do_fio(&c__1, txtcom_1.lintxt + (linuse - 1), (ftnlen)70);
    do_fio(&c__1, aa, (ftnlen)1);
    e_rsfe();
    if (*(unsigned char *)aa != ' ') {
	bug_(&c__0);
    }
    if (loc == -1) {
	goto L1002;
    }
    i__1 = linuse;
    for (kk = linuse + 69; kk >= i__1; --kk) {
	if (*(unsigned char *)&txtcom_1.lintxt[kk - 1] != ' ') {
	    goto L1007;
	}
/* L1006: */
    }
    if (loc == 0) {
	goto L1004;
    }
/*  ABOVE KLUGE IS TO AVOID F40 BUG IF CRLF BROKEN ACROSS RECORD BOUNDARY */
    bug_(&c__1);
L1007:
    txtcom_1.linidx[liduse] = -(kk + 1);
    if (loc == oldloc) {
	goto L1021;
    }
    if (sect == 12) {
	goto L1013;
    }
    if (sect == 10) {
	goto L1012;
    }
    if (sect == 6) {
	goto L1011;
    }
    if (sect == 5) {
	goto L1010;
    }
    if (sect == 1) {
	goto L1008;
    }
    stext[loc - 1] = liduse;
    goto L1020;
L1008:
    ltext[loc - 1] = liduse;
    goto L1020;
L1010:
    if (loc > 0 && loc <= 100) {
	ptxcom_1.ptext[loc - 1] = liduse;
    }
    goto L1020;
L1011:
    if (loc > rtxsiz) {
	bug_(&c__6);
    }
    txtcom_1.rtext[loc - 1] = liduse;
    goto L1020;
L1012:
    ctext[clsses - 1] = liduse;
    cval[clsses - 1] = loc;
    ++clsses;
    goto L1020;
L1013:
    if (loc > magsiz) {
	bug_(&c__6);
    }
    mtxcom_1.mtext[loc - 1] = liduse;
L1020:
    linuse = kk + 1;
    ++liduse;
    oldloc = loc;
    if (linuse + 69 > 48251 || liduse >= 1000) {
	bug_(&c__2);
    }
    goto L1004;
L1021:
    txtcom_1.linidx[liduse - 1] = -txtcom_1.linidx[liduse - 1];
    goto L1020;
/*  THE STUFF FOR SECTION 3 IS ENCODED HERE.  EACH "FROM-LOCATION" GETS A */
/*  CONTIGUOUS SECTION OF THE "TRAVEL" ARRAY.  EACH ENTRY IN TRAVEL IS */
/*  NEWLOC*1000 + KEYWORD (FROM SECTION 4, MOTION VERBS), AND IS NEGATED IF */
/*  THIS IS THE LAST ENTRY FOR THIS LOCATION.  KEY(N) IS THE INDEX IN TRAVEL */
/*  OF THE FIRST OPTION AT LOCATION N. */
L1030:
    s_rsfe(&io___31);
    do_fio(&c__1, (char *)&loc, (ftnlen)sizeof(integer));
    do_fio(&c__1, (char *)&newloc, (ftnlen)sizeof(integer));
    do_fio(&c__20, (char *)&tk[0], (ftnlen)sizeof(integer));
    e_rsfe();
    if (loc == 0) {
	goto L1030;
    }
/*  ABOVE KLUGE IS TO AVOID AFOREMENTIONED F40 BUG */
    if (loc == -1) {
	goto L1002;
    }
    if (key[loc - 1] != 0) {
	goto L1033;
    }
    key[loc - 1] = trvs;
    goto L1035;
L1033:
    travel[trvs - 2] = -travel[trvs - 2];
L1035:
    for (l = 1; l <= 20; ++l) {
	if (tk[l - 1] == 0) {
	    goto L1039;
	}
	travel[trvs - 1] = newloc * 1000 + tk[l - 1];
	++trvs;
	if (trvs == trvsiz) {
	    bug_(&c__3);
	}
/* L1037: */
    }
L1039:
    travel[trvs - 2] = -travel[trvs - 2];
    goto L1030;
/*  HERE WE READ IN THE VOCABULARY.  KTAB(N) IS THE WORD NUMBER, ATAB(N) IS */
/*  THE CORRESPONDING WORD.  THE -1 AT THE END OF SECTION 4 IS LEFT IN KTAB */
/*  AS AN END-MARKER.  THE WORDS ARE GIVEN A MINIMAL HASH TO MAKE READING THE */
/*  CORE-IMAGE HARDER.  NOTE THAT '/7-08' HAD BETTER NOT BE IN THE LIST, SINCE */
/*  IT COULD HASH TO -1. */
L1040:
    i__1 = voccom_1.tabsiz;
    for (tabndx = 1; tabndx <= i__1; ++tabndx) {
L1043:
	s_rsfe(&io___37);
	do_fio(&c__1, (char *)&voccom_1.ktab[tabndx - 1], (ftnlen)sizeof(
		integer));
	do_fio(&c__1, voccom_1.atab + (tabndx - 1) * 5, (ftnlen)5);
	e_rsfe();
	if (voccom_1.ktab[tabndx - 1] == 0) {
	    goto L1043;
	}
/*  ABOVE KLUGE IS TO AVOID AFOREMENTIONED F40 BUG */
	if (voccom_1.ktab[tabndx - 1] == -1) {
	    goto L1002;
	}
/* L1042: */
	phrog_(ch__1, (ftnlen)5, voccom_1.atab + (tabndx - 1) * 5, (ftnlen)5);
	s_copy(voccom_1.atab + (tabndx - 1) * 5, ch__1, (ftnlen)5, (ftnlen)5);
    }
    bug_(&c__4);
/*  READ IN THE INITIAL LOCATIONS FOR EACH OBJECT.  ALSO THE IMMOVABILITY INFO. */
/*  PLAC CONTAINS INITIAL LOCATIONS OF OBJECTS.  FIXD IS -1 FOR IMMOVABLE */
/*  OBJECTS (INCLUDING THE SNAKE), OR = SECOND LOC FOR TWO-PLACED OBJECTS. */
L1050:
    s_rsfe(&io___38);
    do_fio(&c__1, (char *)&obj, (ftnlen)sizeof(integer));
    do_fio(&c__1, (char *)&j, (ftnlen)sizeof(integer));
    do_fio(&c__1, (char *)&k, (ftnlen)sizeof(integer));
    e_rsfe();
    if (obj == -1) {
	goto L1002;
    }
    plac[obj - 1] = j;
    fixd[obj - 1] = k;
    goto L1050;
/*  READ DEFAULT MESSAGE NUMBERS FOR ACTION VERBS, STORE IN ACTSPK. */
L1060:
    s_rsfe(&io___42);
    do_fio(&c__1, (char *)&verb, (ftnlen)sizeof(integer));
    do_fio(&c__1, (char *)&j, (ftnlen)sizeof(integer));
    e_rsfe();
    if (verb == -1) {
	goto L1002;
    }
    actspk[verb - 1] = j;
    goto L1060;
/*  READ INFO ABOUT AVAILABLE LIQUIDS AND OTHER CONDITIONS, STORE IN COND. */
L1070:
    s_rsfe(&io___45);
    do_fio(&c__1, (char *)&k, (ftnlen)sizeof(integer));
    do_fio(&c__20, (char *)&tk[0], (ftnlen)sizeof(integer));
    e_rsfe();
    if (k == -1) {
	goto L1002;
    }
    for (i__ = 1; i__ <= 20; ++i__) {
	loc = tk[i__ - 1];
	if (loc == 0) {
	    goto L1070;
	}
	if (cond[loc - 1] / pow_ii(&c__2, &k) % 2 != 0) {
	    bug_(&c__8);
	}
/* L1071: */
	cond[loc - 1] += shift_(&c__1, &k);
    }
    goto L1070;
/*  READ DATA FOR HINTS. */
L1080:
    hntmax = 0;
L1081:
    s_rsfe(&io___47);
    do_fio(&c__1, (char *)&k, (ftnlen)sizeof(integer));
    do_fio(&c__20, (char *)&tk[0], (ftnlen)sizeof(integer));
    e_rsfe();
    if (k == -1) {
	goto L1002;
    }
    if (k == 0) {
	goto L1081;
    }
    if (k < 0 || k > hntsiz) {
	bug_(&c__7);
    }
    for (i__ = 1; i__ <= 4; ++i__) {
/* L1083: */
	hints[k + i__ * 20 - 21] = tk[i__ - 1];
    }
    hntmax = max(hntmax,k);
    goto L1081;
/*  FINISH CONSTRUCTING INTERNAL DATA FORMAT */
/*  IF SETUP=2 WE DON'T NEED TO DO THIS.  IT'S ONLY NECESSARY IF WE HAVEN'T DONE */
/*  IT AT ALL OR IF THE PROGRAM HAS BEEN RUN SINCE THEN. */
L1100:
    if (wizcom_1.setup == 2) {
	goto L1;
    }
    if (wizcom_1.setup == -1) {
	goto L8305;
    }
/*  HAVING READ IN THE DATABASE, CERTAIN THINGS ARE NOW CONSTRUCTED.  PROPS ARE */
/*  SET TO ZERO.  WE FINISH SETTING UP COND BY CHECKING FOR FORCED-MOTION TRAVEL */
/*  ENTRIES.  THE PLAC AND FIXD ARRAYS ARE USED TO SET UP ATLOC(N) AS THE FIRST */
/*  OBJECT AT LOCATION N, AND LINK(OBJ) AS THE NEXT OBJECT AT THE SAME LOCATION */
/*  AS OBJ.  (OBJ>100 INDICATES THAT FIXED(OBJ-100)=LOC; LINK(OBJ) IS STILL THE */
/*  CORRECT LINK TO USE.)  ABB IS ZEROED; IT CONTROLS WHETHER THE ABBREVIATED */
/*  DESCRIPTION IS PRINTED.  COUNTS MOD 5 UNLESS "LOOK" IS USED. */
    for (i__ = 1; i__ <= 100; ++i__) {
	placom_1.place[i__ - 1] = 0;
	prop[i__ - 1] = 0;
	placom_1.link[i__ - 1] = 0;
/* L1101: */
	placom_1.link[i__ + 99] = 0;
    }
    i__1 = locsiz;
    for (i__ = 1; i__ <= i__1; ++i__) {
	abbcom_1.abb[i__ - 1] = 0;
	if (ltext[i__ - 1] == 0 || key[i__ - 1] == 0) {
	    goto L1102;
	}
	k = key[i__ - 1];
	if ((i__2 = travel[k - 1], abs(i__2)) % 1000 == 1) {
	    cond[i__ - 1] = 2;
	}
L1102:
	placom_1.atloc[i__ - 1] = 0;
    }
/*  SET UP THE ATLOC AND LINK ARRAYS AS DESCRIBED ABOVE.  WE'LL USE THE DROP */
/*  SUBROUTINE, WHICH PREFACES NEW OBJECTS ON THE LISTS.  SINCE WE WANT THINGS */
/*  IN THE OTHER ORDER, WE'LL RUN THE LOOP BACKWARDS.  IF THE OBJECT IS IN TWO */
/*  LOCS, WE DROP IT TWICE.  THIS ALSO SETS UP "PLACE" AND "FIXED" AS COPIES OF */
/*  "PLAC" AND "FIXD".  ALSO, SINCE TWO-PLACED OBJECTS ARE TYPICALLY BEST */
/*  DESCRIBED LAST, WE'LL DROP THEM FIRST. */
    for (i__ = 1; i__ <= 100; ++i__) {
	k = 101 - i__;
	if (fixd[k - 1] <= 0) {
	    goto L1106;
	}
	i__1 = k + 100;
	drop_(&i__1, &fixd[k - 1]);
	drop_(&k, &plac[k - 1]);
L1106:
	;
    }
    for (i__ = 1; i__ <= 100; ++i__) {
	k = 101 - i__;
	placom_1.fixed[k - 1] = fixd[k - 1];
/* L1107: */
	if (plac[k - 1] != 0 && fixd[k - 1] <= 0) {
	    drop_(&k, &plac[k - 1]);
	}
    }
/*  TREASURES, AS NOTED EARLIER, ARE OBJECTS 50 THROUGH MAXTRS (CURRENTLY 79). */
/*  THEIR PROPS ARE INITIALLY -1, AND ARE SET TO 0 THE FIRST TIME THEY ARE */
/*  DESCRIBED.  TALLY KEEPS TRACK OF HOW MANY ARE NOT YET FOUND, SO WE KNOW */
/*  WHEN TO CLOSE THE CAVE.  TALLY2 COUNTS HOW MANY CAN NEVER BE FOUND (E.G. IF */
/*  LOST BIRD OR BRIDGE). */
    maxtrs = 79;
    tally = 0;
    tally2 = 0;
    i__1 = maxtrs;
    for (i__ = 50; i__ <= i__1; ++i__) {
	if (ptxcom_1.ptext[i__ - 1] != 0) {
	    prop[i__ - 1] = -1;
	}
/* L1200: */
	tally -= prop[i__ - 1];
    }
/*  CLEAR THE HINT STUFF.  HINTLC(I) IS HOW LONG HE'S BEEN AT LOC WITH COND BIT */
/*  I.  HINTED(I) IS TRUE IFF HINT I HAS BEEN USED. */
    i__1 = hntmax;
    for (i__ = 1; i__ <= i__1; ++i__) {
	hinted[i__ - 1] = FALSE_;
/* L1300: */
	hintlc[i__ - 1] = 0;
    }
/*  DEFINE SOME HANDY MNEMONICS.  THESE CORRESPOND TO OBJECT NUMBERS. */
    keys = vocab_("KEYS", &c__1, (ftnlen)4);
    lamp = vocab_("LAMP", &c__1, (ftnlen)4);
    grate = vocab_("GRATE", &c__1, (ftnlen)5);
    cage = vocab_("CAGE", &c__1, (ftnlen)4);
    rod = vocab_("ROD", &c__1, (ftnlen)3);
    rod2 = rod + 1;
    steps = vocab_("STEPS", &c__1, (ftnlen)5);
    bird = vocab_("BIRD", &c__1, (ftnlen)4);
    door = vocab_("DOOR", &c__1, (ftnlen)4);
    pillow = vocab_("PILLO", &c__1, (ftnlen)5);
    snake = vocab_("SNAKE", &c__1, (ftnlen)5);
    fissur = vocab_("FISSU", &c__1, (ftnlen)5);
    tablet = vocab_("TABLE", &c__1, (ftnlen)5);
    clam = vocab_("CLAM", &c__1, (ftnlen)4);
    oyster = vocab_("OYSTE", &c__1, (ftnlen)5);
    magzin = vocab_("MAGAZ", &c__1, (ftnlen)5);
    dwarf = vocab_("DWARF", &c__1, (ftnlen)5);
    knife = vocab_("KNIFE", &c__1, (ftnlen)5);
    food = vocab_("FOOD", &c__1, (ftnlen)4);
    bottle = vocab_("BOTTL", &c__1, (ftnlen)5);
    water = vocab_("WATER", &c__1, (ftnlen)5);
    oil = vocab_("OIL", &c__1, (ftnlen)3);
    plant = vocab_("PLANT", &c__1, (ftnlen)5);
    plant2 = plant + 1;
    axe = vocab_("AXE", &c__1, (ftnlen)3);
    mirror = vocab_("MIRRO", &c__1, (ftnlen)5);
    dragon = vocab_("DRAGO", &c__1, (ftnlen)5);
    chasm = vocab_("CHASM", &c__1, (ftnlen)5);
    troll = vocab_("TROLL", &c__1, (ftnlen)5);
    troll2 = troll + 1;
    bear = vocab_("BEAR", &c__1, (ftnlen)4);
    messag = vocab_("MESSA", &c__1, (ftnlen)5);
    vend = vocab_("VENDI", &c__1, (ftnlen)5);
    batter = vocab_("BATTE", &c__1, (ftnlen)5);
/*  OBJECTS FROM 50 THROUGH WHATEVER ARE TREASURES.  HERE ARE A FEW. */
    nugget = vocab_("GOLD", &c__1, (ftnlen)4);
    coins = vocab_("COINS", &c__1, (ftnlen)5);
    chest = vocab_("CHEST", &c__1, (ftnlen)5);
    eggs = vocab_("EGGS", &c__1, (ftnlen)4);
    tridnt = vocab_("TRIDE", &c__1, (ftnlen)5);
    vase = vocab_("VASE", &c__1, (ftnlen)4);
    emrald = vocab_("EMERA", &c__1, (ftnlen)5);
    pyram = vocab_("PYRAM", &c__1, (ftnlen)5);
    pearl = vocab_("PEARL", &c__1, (ftnlen)5);
    rug = vocab_("RUG", &c__1, (ftnlen)3);
    chain = vocab_("CHAIN", &c__1, (ftnlen)5);
/*  THESE ARE MOTION-VERB NUMBERS. */
    back = vocab_("BACK", &c__0, (ftnlen)4);
    look = vocab_("LOOK", &c__0, (ftnlen)4);
    cave = vocab_("CAVE", &c__0, (ftnlen)4);
    null = vocab_("NULL", &c__0, (ftnlen)4);
    entrnc = vocab_("ENTRA", &c__0, (ftnlen)5);
    dprssn = vocab_("DEPRE", &c__0, (ftnlen)5);
/*  AND SOME ACTION VERBS. */
    say = vocab_("SAY", &c__2, (ftnlen)3);
    lock = vocab_("LOCK", &c__2, (ftnlen)4);
    throw = vocab_("THROW", &c__2, (ftnlen)5);
    find = vocab_("FIND", &c__2, (ftnlen)4);
    invent = vocab_("INVEN", &c__2, (ftnlen)5);
/*  INITIALISE THE DWARVES.  DLOC IS LOC OF DWARVES, HARD-WIRED IN.  ODLOC IS */
/*  PRIOR LOC OF EACH DWARF, INITIALLY GARBAGE.  DALTLC IS ALTERNATE INITIAL LOC */
/*  FOR DWARF, IN CASE ONE OF THEM STARTS OUT ON TOP OF THE ADVENTURER.  (NO 2 */
/*  OF THE 5 INITIAL LOCS ARE ADJACENT.)  DSEEN IS TRUE IF DWARF HAS SEEN HIM. */
/*  DFLAG CONTROLS THE LEVEL OF ACTIVATION OF ALL THIS: */
/*     0  NO DWARF STUFF YET (WAIT UNTIL REACHES HALL OF MISTS) */
/*     1  REACHED HALL OF MISTS, BUT HASN'T MET FIRST DWARF */
/*     2  MET FIRST DWARF, OTHERS START MOVING, NO KNIVES THROWN YET */
/*     3  A KNIFE HAS BEEN THROWN (FIRST SET ALWAYS MISSES) */
/*     3+ DWARVES ARE MAD (INCREASES THEIR ACCURACY) */
/*  SIXTH DWARF IS SPECIAL (THE PIRATE).  HE ALWAYS STARTS AT HIS CHEST'S */
/*  EVENTUAL LOCATION INSIDE THE MAZE.  THIS LOC IS SAVED IN CHLOC FOR REF. */
/*  THE DEAD END IN THE OTHER MAZE HAS ITS LOC STORED IN CHLOC2. */
    chloc = 114;
    chloc2 = 140;
    for (i__ = 1; i__ <= 6; ++i__) {
/* L1700: */
	dseen[i__ - 1] = FALSE_;
    }
    dflag = 0;
    dloc[0] = 19;
    dloc[1] = 27;
    dloc[2] = 33;
    dloc[3] = 44;
    dloc[4] = 64;
    dloc[5] = chloc;
    daltlc = 18;
/*  OTHER RANDOM FLAGS AND COUNTERS, AS FOLLOWS: */
/*     TURNS    TALLIES HOW MANY COMMANDS HE'S GIVEN (IGNORES YES/NO) */
/*     LIMIT    LIFETIME OF LAMP (NOT SET HERE) */
/*     IWEST    HOW MANY TIMES HE'S SAID "WEST" INSTEAD OF "W" */
/*     KNFLOC   0 IF NO KNIFE HERE, LOC IF KNIFE HERE, -1 AFTER CAVEAT */
/*     DETAIL   HOW OFTEN WE'VE SAID "NOT ALLOWED TO GIVE MORE DETAIL" */
/*     ABBNUM   HOW OFTEN WE SHOULD PRINT NON-ABBREVIATED DESCRIPTIONS */
/*     MAXDIE   NUMBER OF REINCARNATION MESSAGES AVAILABLE (UP TO 5) */
/*     NUMDIE   NUMBER OF TIMES KILLED SO FAR */
/*     HOLDNG   NUMBER OF OBJECTS BEING CARRIED */
/*     DKILL    NUMBER OF DWARVES KILLED (UNUSED IN SCORING, NEEDED FOR MSG) */
/*     FOOBAR   CURRENT PROGRESS IN SAYING "FEE FIE FOE FOO". */
/*     BONUS    USED TO DETERMINE AMOUNT OF BONUS IF HE REACHES CLOSING */
/*     CLOCK1   NUMBER OF TURNS FROM FINDING LAST TREASURE TILL CLOSING */
/*     CLOCK2   NUMBER OF TURNS FROM FIRST WARNING TILL BLINDING FLASH */
/*     LOGICALS WERE EXPLAINED EARLIER */
    turns = 0;
    lmwarn = FALSE_;
    iwest = 0;
    knfloc = 0;
    detail = 0;
    abbnum = 5;
    for (i__ = 0; i__ <= 4; ++i__) {
/* L1800: */
	if (txtcom_1.rtext[(i__ << 1) + 80] != 0) {
	    maxdie = i__ + 1;
	}
    }
    numdie = 0;
    advent_player_dead = 0;
    placom_1.holdng = 0;
    dkill = 0;
    foobar = 0;
    bonus = 0;
    clock1 = 30;
    clock2 = 50;
    wizcom_1.saved = 0;
    closng = FALSE_;
    panic = FALSE_;
    closed = FALSE_;
    gaveup = FALSE_;
    scorng = FALSE_;
/*  IF SETUP=1, REPORT ON AMOUNT OF ARRAYS ACTUALLY USED, TO PERMIT REDUCTIONS. */
    if (wizcom_1.setup != 1) {
	goto L1;
    }
    wizcom_1.setup = 2;
    i__1 = locsiz;
    for (k = 1; k <= i__1; ++k) {
	kk = locsiz + 1 - k;
	if (ltext[kk - 1] != 0) {
	    goto L1997;
	}
/* L1998: */
    }
    obj = 0;
L1997:
    for (k = 1; k <= 100; ++k) {
/* L1996: */
	if (ptxcom_1.ptext[k - 1] != 0) {
	    ++obj;
	}
    }
    i__1 = tabndx;
    for (k = 1; k <= i__1; ++k) {
/* L1995: */
	if (voccom_1.ktab[k - 1] / 1000 == 2) {
	    verb = voccom_1.ktab[k - 1] - 2000;
	}
    }
    i__1 = rtxsiz;
    for (k = 1; k <= i__1; ++k) {
	j = rtxsiz + 1 - k;
	if (txtcom_1.rtext[j - 1] != 0) {
	    goto L1993;
	}
/* L1994: */
    }
L1993:
    i__1 = magsiz;
    for (k = 1; k <= i__1; ++k) {
	i__ = magsiz + 1 - k;
	if (mtxcom_1.mtext[i__ - 1] != 0) {
	    goto L1991;
	}
/* L1992: */
    }
L1991:
    k = 100;
    s_wsfe(&io___135);
    i__1 = linuse - 1;
    do_fio(&c__1, (char *)&i__1, (ftnlen)sizeof(integer));
    do_fio(&c__1, (char *)&c__48251, (ftnlen)sizeof(integer));
    i__2 = liduse - 1;
    do_fio(&c__1, (char *)&i__2, (ftnlen)sizeof(integer));
    do_fio(&c__1, (char *)&c__1000, (ftnlen)sizeof(integer));
    do_fio(&c__1, (char *)&trvs, (ftnlen)sizeof(integer));
    do_fio(&c__1, (char *)&trvsiz, (ftnlen)sizeof(integer));
    do_fio(&c__1, (char *)&tabndx, (ftnlen)sizeof(integer));
    do_fio(&c__1, (char *)&voccom_1.tabsiz, (ftnlen)sizeof(integer));
    do_fio(&c__1, (char *)&kk, (ftnlen)sizeof(integer));
    do_fio(&c__1, (char *)&locsiz, (ftnlen)sizeof(integer));
    do_fio(&c__1, (char *)&obj, (ftnlen)sizeof(integer));
    do_fio(&c__1, (char *)&k, (ftnlen)sizeof(integer));
    do_fio(&c__1, (char *)&verb, (ftnlen)sizeof(integer));
    do_fio(&c__1, (char *)&vrbsiz, (ftnlen)sizeof(integer));
    do_fio(&c__1, (char *)&j, (ftnlen)sizeof(integer));
    do_fio(&c__1, (char *)&rtxsiz, (ftnlen)sizeof(integer));
    do_fio(&c__1, (char *)&clsses, (ftnlen)sizeof(integer));
    do_fio(&c__1, (char *)&clsmax, (ftnlen)sizeof(integer));
    do_fio(&c__1, (char *)&hntmax, (ftnlen)sizeof(integer));
    do_fio(&c__1, (char *)&hntsiz, (ftnlen)sizeof(integer));
    do_fio(&c__1, (char *)&i__, (ftnlen)sizeof(integer));
    do_fio(&c__1, (char *)&magsiz, (ftnlen)sizeof(integer));
    e_wsfe();
/*  FINALLY, SINCE WE'RE CLEARLY SETTING THINGS UP FOR THE FIRST TIME... */
    poof_();
/*  ADDED IN PORT--SAVE TO THE BIN FILE */
L12348:
/*  START-UP, DWARF STUFF */
L1:
    demo = start_(&c__0);
    motd_(&c_false);
    i__ = ran_(&c__1);
    hinted[2] = yes_(&c__65, &c__1, &c__0);
    newloc = 1;
    wizcom_1.setup = 3;
    limit = 330;
    if (hinted[2]) {
	limit = 1000;
    }
/*  CAN'T LEAVE CAVE ONCE IT'S CLOSING (EXCEPT BY MAIN OFFICE). */
L2:
    if (newloc >= 9 || newloc == 0 || ! closng) {
	goto L71;
    }
    rspeak_(&c__130);
    newloc = loc;
    if (! panic) {
	clock2 = 15;
    }
    panic = TRUE_;
/*  SEE IF A DWARF HAS SEEN HIM AND HAS COME FROM WHERE HE WANTS TO GO.  IF SO, */
/*  THE DWARF'S BLOCKING HIS WAY.  IF COMING FROM PLACE FORBIDDEN TO PIRATE */
/*  (DWARVES ROOTED IN PLACE) LET HIM GET OUT (AND ATTACKED). */
L71:
    /*  DRO: ADDED dwarf disable hook*/
    if (advent_disable_dwarves) {
        goto L74; /* no dwarf blocking */
    }

    if (newloc == loc || cond[loc - 1] == 2 || cond[loc - 1] / pow_ii(&c__2, &
	    c__3) % 2 != 0) {
	goto L74;
    }
    for (i__ = 1; i__ <= 5; ++i__) {
	if (odloc[i__ - 1] != newloc || ! dseen[i__ - 1]) {
	    goto L73;
	}
	newloc = loc;
	rspeak_(&c__2);
	goto L74;
L73:
	;
    }
L74:
    loc = newloc;
/*  DWARF STUFF.  SEE EARLIER COMMENTS FOR DESCRIPTION OF VARIABLES.  REMEMBER */
/*  SIXTH DWARF IS PIRATE AND IS THUS VERY DIFFERENT EXCEPT FOR MOTION RULES. */
/*  FIRST OFF, DON'T LET THE DWARVES FOLLOW HIM INTO A PIT OR A WALL.  ACTIVATE */
/*  THE WHOLE MESS THE FIRST TIME HE GETS AS FAR AS THE HALL OF MISTS (LOC 15). */
/*  IF NEWLOC IS FORBIDDEN TO PIRATE (IN PARTICULAR, IF IT'S BEYOND THE TROLL */
/*  BRIDGE), BYPASS DWARF STUFF.  THAT WAY PIRATE CAN'T STEAL RETURN TOLL, AND */
/*  DWARVES CAN'T MEET THE BEAR.  ALSO MEANS DWARVES WON'T FOLLOW HIM INTO DEAD */
/*  END IN MAZE, BUT C'EST LA VIE.  THEY'LL WAIT FOR HIM OUTSIDE THE DEAD END. */

    /*  DRO: ADDED dwarf disable hook*/
    if (advent_disable_dwarves) {
        goto L2000; /* bypass all dwarf activation/movement/attacks */
    }

    if (loc == 0 || cond[loc - 1] == 2 || cond[newloc - 1] / pow_ii(&c__2, &
	    c__3) % 2 != 0) {
	goto L2000;
    }
    if (dflag != 0) {
	goto L6000;
    }
    if (loc >= 15) {
	dflag = 1;
    }
    goto L2000;
/*  WHEN WE ENCOUNTER THE FIRST DWARF, WE KILL 0, 1, OR 2 OF THE 5 DWARVES.  IF */
/*  ANY OF THE SURVIVORS IS AT LOC, REPLACE HIM WITH THE ALTERNATE. */
L6000:
    if (dflag != 1) {
	goto L6010;
    }
    if (loc < 15 || ran_(&c__100) < c__95) {
	goto L2000;
    }
    dflag = 2;
    for (i__ = 1; i__ <= 2; ++i__) {
	j = ran_(&c__5) + 1;
/*  IF SAVED NOT = -1, HE BYPASSED THE "START" CALL. */
/* L6001: */
	if (ran_(&c__100) < c__50 && wizcom_1.saved == -1) {
	    dloc[j - 1] = 0;
	}
    }
    for (i__ = 1; i__ <= 5; ++i__) {
	if (dloc[i__ - 1] == loc) {
	    dloc[i__ - 1] = daltlc;
	}
/* L6002: */
	odloc[i__ - 1] = dloc[i__ - 1];
    }
    rspeak_(&c__3);
    drop_(&axe, &loc);
    goto L2000;
/*  THINGS ARE IN FULL SWING.  MOVE EACH DWARF AT RANDOM, EXCEPT IF HE'S SEEN US */
/*  HE STICKS WITH US.  DWARVES NEVER GO TO LOCS <15.  IF WANDERING AT RANDOM, */
/*  THEY DON'T BACK UP UNLESS THERE'S NO ALTERNATIVE.  IF THEY DON'T HAVE TO */
/*  MOVE, THEY ATTACK.  AND, OF COURSE, DEAD DWARVES DON'T DO MUCH OF ANYTHING. */
L6010:
    dtotal = 0;
    attack = 0;
    stick = 0;
    for (i__ = 1; i__ <= 6; ++i__) {
	if (dloc[i__ - 1] == 0) {
	    goto L6030;
	}
	j = 1;
	kk = dloc[i__ - 1];
	kk = key[kk - 1];
	if (kk == 0) {
	    goto L6016;
	}
L6012:
	newloc = (i__1 = travel[kk - 1], abs(i__1)) / 1000 % 1000;
	if (newloc > 300 || newloc < 15 || newloc == odloc[i__ - 1] || j > 1
		&& newloc == tk[j - 2] || j >= 20 || newloc == dloc[i__ - 1]
		|| cond[newloc - 1] == 2 || i__ == 6 && cond[newloc - 1] /
		pow_ii(&c__2, &c__3) % 2 != 0 || (i__1 = travel[kk - 1], abs(
		i__1)) / 1000000 == 100) {
	    goto L6014;
	}
	tk[j - 1] = newloc;
	++j;
L6014:
	++kk;
	if (travel[kk - 2] >= 0) {
	    goto L6012;
	}
L6016:
	tk[j - 1] = odloc[i__ - 1];
	if (j >= 2) {
	    --j;
	}
	j = ran_(&j) + 1;
	odloc[i__ - 1] = dloc[i__ - 1];
	dloc[i__ - 1] = tk[j - 1];
	dseen[i__ - 1] = dseen[i__ - 1] && loc >= 15 || (dloc[i__ - 1] == loc
		|| odloc[i__ - 1] == loc);
	if (! dseen[i__ - 1]) {
	    goto L6030;
	}
	dloc[i__ - 1] = loc;
	if (i__ != 6) {
	    goto L6027;
	}
/*  THE PIRATE'S SPOTTED HIM.  HE LEAVES HIM ALONE ONCE WE'VE FOUND CHEST. */
/*  K COUNTS IF A TREASURE IS HERE.  IF NOT, AND TALLY=TALLY2 PLUS ONE FOR */
/*  AN UNSEEN CHEST, LET THE PIRATE BE SPOTTED. */
	if (loc == chloc || prop[chest - 1] >= 0) {
	    goto L6030;
	}
	k = 0;
	i__1 = maxtrs;
	for (j = 50; j <= i__1; ++j) {
/*  PIRATE WON'T TAKE PYRAMID FROM PLOVER ROOM OR DARK ROOM (TOO EASY!). */
	    if (j == pyram && (loc == plac[pyram - 1] || loc == plac[emrald -
		    1])) {
		goto L6020;
	    }
	    idondx = j;
	    if (placom_1.place[idondx - 1] == -1) {
		goto L6022;
	    }
L6020:
	    if (placom_1.place[idondx - 1] == loc || placom_1.place[idondx -
		    1] == -1) {
		k = 1;
	    }
	}
	if (tally == tally2 + 1 && k == 0 && placom_1.place[chest - 1] == 0 &&
		 (placom_1.place[lamp - 1] == loc || placom_1.place[lamp - 1]
		== -1) && prop[lamp - 1] == 1) {
	    goto L6025;
	}
	if (odloc[5] != dloc[5] && ran_(&c__100) < c__20) {
	    rspeak_(&c__127);
	}
	goto L6030;
L6022:
	rspeak_(&c__128);
/*  DON'T STEAL CHEST BACK FROM TROLL! */
	if (placom_1.place[messag - 1] == 0) {
	    move_(&chest, &chloc);
	}
	move_(&messag, &chloc2);
	i__1 = maxtrs;
	for (j = 50; j <= i__1; ++j) {
	    if (j == pyram && (loc == plac[pyram - 1] || loc == plac[emrald -
		    1])) {
		goto L6023;
	    }
	    idondx = j;
	    if ((placom_1.place[idondx - 1] == loc || placom_1.fixed[idondx -
		    1] == loc) && placom_1.fixed[idondx - 1] == 0) {
		carry_(&idondx, &loc);
	    }
	    if (placom_1.place[idondx - 1] == -1) {
		drop_(&idondx, &chloc);
	    }
L6023:
	    ;
	}
L6024:
	dloc[5] = chloc;
	odloc[5] = chloc;
	dseen[5] = FALSE_;
	goto L6030;
L6025:
	rspeak_(&c__186);
	move_(&chest, &chloc);
	move_(&messag, &chloc2);
	goto L6024;
/*  THIS THREATENING LITTLE DWARF IS IN THE ROOM WITH HIM! */
L6027:
	++dtotal;
	if (odloc[i__ - 1] != dloc[i__ - 1]) {
	    goto L6030;
	}
	++attack;
	if (knfloc >= 0) {
	    knfloc = loc;
	}
	if (ran_(&c__1000) < (dflag - 2) * 95) {
	    ++stick;
	}
L6030:
	;
    }
/*  NOW WE KNOW WHAT'S HAPPENING.  LET'S TELL THE POOR SUCKER ABOUT IT. */
    if (dtotal == 0) {
	goto L2000;
    }
    if (dtotal == 1) {
	goto L75;
    }
    s_wsfe(&io___143);
    do_fio(&c__1, (char *)&dtotal, (ftnlen)sizeof(integer));
    e_wsfe();
    goto L77;
L75:
    rspeak_(&c__4);
L77:
    if (attack == 0) {
	goto L2000;
    }
    if (dflag == 2) {
	dflag = 3;
    }
/*  IF SAVED NOT = -1, HE BYPASSED THE "START" CALL.  DWARVES GET *VERY* MAD! */
    if (wizcom_1.saved != -1) {
	dflag = 20;
    }
    if (attack == 1) {
	goto L79;
    }
    s_wsfe(&io___144);
    do_fio(&c__1, (char *)&attack, (ftnlen)sizeof(integer));
    e_wsfe();
    k = 6;
L82:
    if (stick > 1) {
	goto L83;
    }
    i__1 = k + stick;
    rspeak_(&i__1);
    if (stick == 0) {
	goto L2000;
    }
    goto L84;
L83:
    s_wsfe(&io___145);
    do_fio(&c__1, (char *)&stick, (ftnlen)sizeof(integer));
    e_wsfe();
L84:
    oldlc2 = loc;
    goto L99;
L79:
    rspeak_(&c__5);
    k = 52;
    goto L82;
/*  DESCRIBE THE CURRENT LOCATION AND (MAYBE) GET NEXT COMMAND. */
/*  PRINT TEXT FOR CURRENT LOC. */
L2000:
    if (loc == 0) {
	goto L99;
    }
    kk = stext[loc - 1];
    if (abbcom_1.abb[loc - 1] % abbnum == 0 || kk == 0) {
	kk = ltext[loc - 1];
    }
    if (cond[loc - 1] == 2 || ! (cond[loc - 1] % 2 == 0 && (prop[lamp - 1] ==
	    0 || ! (placom_1.place[lamp - 1] == loc || placom_1.place[lamp -
	    1] == -1)))) {
	goto L2001;
    }
    if (wzdark && ran_(&c__100) < c__35) {
	goto L90;
    }
    kk = txtcom_1.rtext[15];
L2001:
    if (placom_1.place[bear - 1] == -1) {
	rspeak_(&c__141);
    }
    speak_(&kk);
    k = 1;
    if (cond[loc - 1] == 2) {
	goto L8;
    }
    if (loc == 33 && ran_(&c__100) < c__25 && ! closng) {
	rspeak_(&c__8);
    }
/*  PRINT OUT DESCRIPTIONS OF OBJECTS AT THIS LOCATION.  IF NOT CLOSING AND */
/*  PROPERTY VALUE IS NEGATIVE, TALLY OFF ANOTHER TREASURE.  RUG IS SPECIAL */
/*  CASE; ONCE SEEN, ITS PROP IS 1 (DRAGON ON IT) TILL DRAGON IS KILLED. */
/*  SIMILARLY FOR CHAIN; PROP IS INITIALLY 1 (LOCKED TO BEAR).  THESE HACKS */
/*  ARE BECAUSE PROP=0 IS NEEDED TO GET FULL SCORE. */
    if (cond[loc - 1] % 2 == 0 && (prop[lamp - 1] == 0 || ! (placom_1.place[
	    lamp - 1] == loc || placom_1.place[lamp - 1] == -1))) {
	goto L2012;
    }
    ++abbcom_1.abb[loc - 1];
    i__ = placom_1.atloc[loc - 1];
L2004:
    if (i__ == 0) {
	goto L2012;
    }
    obj = i__;
    if (obj > 100) {
	obj += -100;
    }
    if (obj == steps && placom_1.place[nugget - 1] == -1) {
	goto L2008;
    }
    if (prop[obj - 1] >= 0) {
	goto L2006;
    }
    if (closed) {
	goto L2008;
    }
    prop[obj - 1] = 0;
    if (obj == rug || obj == chain) {
	prop[obj - 1] = 1;
    }
    --tally;
/*  IF REMAINING TREASURES TOO ELUSIVE, ZAP HIS LAMP. */
    if (tally == tally2 && tally != 0) {
	limit = min(35,limit);
    }
L2006:
    kk = prop[obj - 1];
    if (obj == steps && loc == placom_1.fixed[steps - 1]) {
	kk = 1;
    }
    pspeak_(&obj, &kk);
L2008:
    i__ = placom_1.link[i__ - 1];
    goto L2004;
L2009:
    k = 54;
L2010:
    spk = k;
L2011:
    rspeak_(&spk);
L2012:
    verb = 0;
    obj = 0;
/*  CHECK IF THIS LOC IS ELIGIBLE FOR ANY HINTS.  IF BEEN HERE LONG ENOUGH, */
/*  BRANCH TO HELP SECTION (ON LATER PAGE).  HINTS ALL COME BACK HERE EVENTUALLY */
/*  TO FINISH THE LOOP.  IGNORE "HINTS" < 4 (SPECIAL STUFF, SEE DATABASE NOTES). */
L2600:
    i__1 = hntmax;
    for (hint = 4; hint <= i__1; ++hint) {
	if (hinted[hint - 1]) {
	    goto L2602;
	}
	idondx = hint;
	if (! (cond[loc - 1] / pow_ii(&c__2, &idondx) % 2 != 0)) {
	    hintlc[hint - 1] = -1;
	}
	++hintlc[hint - 1];
	if (hintlc[hint - 1] >= hints[hint - 1]) {
	    goto L40000;
	}
L2602:
	;
    }
/*  KICK THE RANDOM NUMBER GENERATOR JUST TO ADD VARIETY TO THE CHASE.  ALSO, */
/*  IF CLOSING TIME, CHECK FOR ANY OBJECTS BEING TOTED WITH PROP < 0 AND SET */
/*  THE PROP TO -1-PROP.  THIS WAY OBJECTS WON'T BE DESCRIBED UNTIL THEY'VE */
/*  BEEN PICKED UP AND PUT DOWN SEPARATE FROM THEIR RESPECTIVE PILES.  DON'T */
/*  TICK CLOCK1 UNLESS WELL INTO CAVE (AND NOT AT Y2). */
    if (! closed) {
	goto L2605;
    }
    if (prop[oyster - 1] < 0 && placom_1.place[oyster - 1] == -1) {
	pspeak_(&oyster, &c__1);
    }
    for (i__ = 1; i__ <= 100; ++i__) {
	idondx = i__;
/* L2604: */
	if (placom_1.place[idondx - 1] == -1 && prop[idondx - 1] < 0) {
	    prop[idondx - 1] = -1 - prop[idondx - 1];
	}
    }
L2605:
    wzdark = cond[loc - 1] % 2 == 0 && (prop[lamp - 1] == 0 || ! (
	    placom_1.place[lamp - 1] == loc || placom_1.place[lamp - 1] == -1)
	    );
    if (knfloc > 0 && knfloc != loc) {
	knfloc = 0;
    }
    i__ = ran_(&c__1);
    getin_(wd1, wd1x, wd2, wd2x, (ftnlen)5, (ftnlen)5, (ftnlen)5, (ftnlen)5);
/*  EVERY INPUT, CHECK "FOOBAR" FLAG.  IF ZERO, NOTHING'S GOING ON.  IF POS, */
/*  MAKE NEG.  IF NEG, HE SKIPPED A WORD, SO MAKE IT ZERO. */
L2608:
/* Computing MIN */
    i__1 = 0, i__2 = -foobar;
    foobar = min(i__1,i__2);
    if (turns == 0 && s_cmp(wd1, "MAGIC", (ftnlen)5, (ftnlen)5) == 0 && s_cmp(
	    wd2, "MODE", (ftnlen)5, (ftnlen)4) == 0) {
	maint_(&needsv);
    }
    if (needsv) {
	goto L34567;
    }
L34567:
    ++turns;
    if (demo && turns >= wizcom_1.short__) {
	goto L13000;
    }
    if (verb == say && s_cmp(wd2, blanks, (ftnlen)5, (ftnlen)5) != 0) {
	verb = 0;
    }
    if (verb == say) {
	goto L4090;
    }
    if (tally == 0 && loc >= 15 && loc != 33) {
	--clock1;
    }
    if (clock1 == 0) {
	goto L10000;
    }
    if (clock1 < 0) {
	--clock2;
    }
    if (clock2 == 0) {
	goto L11000;
    }
    if (prop[lamp - 1] == 1) {
	--limit;
    }
    if (limit <= 30 && (placom_1.place[batter - 1] == loc || placom_1.place[
	    batter - 1] == -1) && prop[batter - 1] == 0 && (placom_1.place[
	    lamp - 1] == loc || placom_1.place[lamp - 1] == -1)) {
	goto L12000;
    }
    if (limit == 0) {
	goto L12400;
    }
    if (limit < 0 && loc <= 8) {
	goto L12600;
    }
    if (limit <= 30) {
	goto L12200;
    }
L19999:
    k = 43;
    i__1 = ((cond[loc - 1] / 2 << 1) % 8 - 5) * (cond[loc - 1] / 4 % 2) + 1;
    if ((1 - i__1) * water + i__1 / 2 * (water + oil) == water) {
	k = 70;
    }
    if (s_cmp(wd1, "ENTER", (ftnlen)5, (ftnlen)5) == 0 && (s_cmp(wd2, "STREA",
	     (ftnlen)5, (ftnlen)5) == 0 || s_cmp(wd2, "WATER", (ftnlen)5, (
	    ftnlen)5) == 0)) {
	goto L2010;
    }
    if (s_cmp(wd1, "ENTER", (ftnlen)5, (ftnlen)5) == 0 && s_cmp(wd2, blanks, (
	    ftnlen)5, (ftnlen)5) != 0) {
	goto L2800;
    }
    if (s_cmp(wd1, "WATER", (ftnlen)5, (ftnlen)5) != 0 && s_cmp(wd1, "OIL", (
	    ftnlen)5, (ftnlen)3) != 0 || s_cmp(wd2, "PLANT", (ftnlen)5, (
	    ftnlen)5) != 0 && s_cmp(wd2, "DOOR", (ftnlen)5, (ftnlen)4) != 0) {
	goto L2610;
    }
    i__1 = vocab_(wd2, &c__1, (ftnlen)5);
    if (placom_1.place[i__1 - 1] == loc || placom_1.fixed[i__1 - 1] == loc) {
	s_copy(wd2, "POUR", (ftnlen)5, (ftnlen)4);
    }
L2610:
    if (s_cmp(wd1, "WEST", (ftnlen)5, (ftnlen)4) != 0) {
	goto L2630;
    }
    ++iwest;
    if (iwest == 10) {
	rspeak_(&c__17);
    }
L2630:
    i__ = vocab_(wd1, &c_n1, (ftnlen)5);
    if (i__ == -1) {
	goto L3000;
    }
    k = i__ % 1000;
    kq = i__ / 1000 + 1;
    switch (kq) {
	case 1:  goto L8;
	case 2:  goto L5000;
	case 3:  goto L4000;
	case 4:  goto L2010;
    }
    bug_(&c__22);
/*  GET SECOND WORD FOR ANALYSIS. */
L2800:
    s_copy(wd1, wd2, (ftnlen)5, (ftnlen)5);
    s_copy(wd1x, wd2x, (ftnlen)5, (ftnlen)5);
    s_copy(wd2, blanks, (ftnlen)5, (ftnlen)5);
    goto L2610;
/*  GEE, I DON'T UNDERSTAND. */
L3000:
    spk = 60;
    if (ran_(&c__100) < c__20) {
	spk = 61;
    }
    if (ran_(&c__100) < c__20) {
	spk = 13;
    }
    rspeak_(&spk);
    goto L2600;
/*  ANALYSE A VERB.  REMEMBER WHAT IT WAS, GO BACK FOR OBJECT IF SECOND WORD */
/*  UNLESS VERB IS "SAY", WHICH SNARFS ARBITRARY SECOND WORD. */
L4000:
    verb = k;
    spk = actspk[verb - 1];
    if (s_cmp(wd2, blanks, (ftnlen)5, (ftnlen)5) != 0 && verb != say) {
	goto L2800;
    }
/*  PORTING NOTE: ORIGINALLY THE ACTION WAS OBJ=WD2. AS FAR AS I CAN */
/*  TELL, THE IMPORTANT EFFECT OF THIS IS ONLY TO SET OBJ TO NONZERO IFF */
/*  A SECOND WORD WAS PROVIDED. */
    if (verb == say) {
	obj = *(unsigned char *)wd2 - ' ';
    }
    if (obj != 0) {
	goto L4090;
    }
/*  ANALYSE AN INTRANSITIVE VERB (IE, NO OBJECT GIVEN YET). */
/* L4080: */
    switch (verb) {
	case 1:  goto L8010;
	case 2:  goto L8000;
	case 3:  goto L8000;
	case 4:  goto L8040;
	case 5:  goto L2009;
	case 6:  goto L8040;
	case 7:  goto L9070;
	case 8:  goto L9080;
	case 9:  goto L8000;
	case 10:  goto L8000;
	case 11:  goto L2011;
	case 12:  goto L9120;
	case 13:  goto L9130;
	case 14:  goto L8140;
	case 15:  goto L9150;
	case 16:  goto L8000;
	case 17:  goto L8000;
	case 18:  goto L8180;
	case 19:  goto L8000;
	case 20:  goto L8200;
	case 21:  goto L8000;
	case 22:  goto L9220;
	case 23:  goto L9230;
	case 24:  goto L8240;
	case 25:  goto L8250;
	case 26:  goto L8260;
	case 27:  goto L8270;
	case 28:  goto L8000;
	case 29:  goto L8000;
	case 30:  goto L8300;
	case 31:  goto L8310;
	case 32:  goto L8320;
    }
/*          TAKE DROP  SAY OPEN NOTH LOCK   ON  OFF WAVE CALM */
/*          WALK KILL POUR  EAT DRNK  RUB TOSS QUIT FIND INVN */
/*          FEED FILL BLST SCOR  FOO  BRF READ BREK WAKE SUSP */
/*          HOUR REST */
    bug_(&c__23);
/*  ANALYSE A TRANSITIVE VERB. */
L4090:
    switch (verb) {
	case 1:  goto L9010;
	case 2:  goto L9020;
	case 3:  goto L9030;
	case 4:  goto L9040;
	case 5:  goto L2009;
	case 6:  goto L9040;
	case 7:  goto L9070;
	case 8:  goto L9080;
	case 9:  goto L9090;
	case 10:  goto L2011;
	case 11:  goto L2011;
	case 12:  goto L9120;
	case 13:  goto L9130;
	case 14:  goto L9140;
	case 15:  goto L9150;
	case 16:  goto L9160;
	case 17:  goto L9170;
	case 18:  goto L2011;
	case 19:  goto L9190;
	case 20:  goto L9190;
	case 21:  goto L9210;
	case 22:  goto L9220;
	case 23:  goto L9230;
	case 24:  goto L2011;
	case 25:  goto L2011;
	case 26:  goto L2011;
	case 27:  goto L9270;
	case 28:  goto L9280;
	case 29:  goto L9290;
	case 30:  goto L2011;
	case 31:  goto L2011;
    }
/*          TAKE DROP  SAY OPEN NOTH LOCK   ON  OFF WAVE CALM */
/*          WALK KILL POUR  EAT DRNK  RUB TOSS QUIT FIND INVN */
/*          FEED FILL BLST SCOR  FOO  BRF READ BREK WAKE SUSP */
/*          HOUR */
    bug_(&c__24);
/*  ANALYSE AN OBJECT WORD.  SEE IF THE THING IS HERE, WHETHER WE'VE GOT A VERB */
/*  YET, AND SO ON.  OBJECT MUST BE HERE UNLESS VERB IS "FIND" OR "INVENT(ORY)" */
/*  (AND NO NEW VERB YET TO BE ANALYSED).  WATER AND OIL ARE ALSO FUNNY, SINCE */
/*  THEY ARE NEVER ACTUALLY DROPPED AT ANY LOCATION, BUT MIGHT BE HERE INSIDE */
/*  THE BOTTLE OR AS A FEATURE OF THE LOCATION. */
L5000:
    obj = k;
    if (placom_1.fixed[k - 1] != loc && ! (placom_1.place[k - 1] == loc ||
	    placom_1.place[k - 1] == -1)) {
	goto L5100;
    }
L5010:
    if (s_cmp(wd2, blanks, (ftnlen)5, (ftnlen)5) != 0) {
	goto L2800;
    }
    if (verb != 0) {
	goto L4090;
    }
    a5toa1_(wd1, wd1x, "?", a1, &k, (ftnlen)5, (ftnlen)5, (ftnlen)1, (ftnlen)
	    1);
    s_wsfe(&io___156);
    i__1 = k;
    for (i__ = 1; i__ <= i__1; ++i__) {
	do_fio(&c__1, a1 + (i__ - 1), (ftnlen)1);
    }
    e_wsfe();
    goto L2600;
L5100:
    if (k != grate) {
	goto L5110;
    }
    if (loc == 1 || loc == 4 || loc == 7) {
	k = dprssn;
    }
    if (loc > 9 && loc < 15) {
	k = entrnc;
    }
    if (k != grate) {
	goto L8;
    }
L5110:
    if (k != dwarf) {
	goto L5120;
    }
    for (i__ = 1; i__ <= 5; ++i__) {
	if (dloc[i__ - 1] == loc && dflag >= 2) {
	    goto L5010;
	}
/* L5112: */
    }
L5120:
/* Computing MAX */
    i__2 = prop[bottle - 1], i__3 = -1 - prop[bottle - 1];
    i__1 = max(i__2,i__3);
    i__4 = ((cond[loc - 1] / 2 << 1) % 8 - 5) * (cond[loc - 1] / 4 % 2) + 1;
    if ((1 - i__1) * water + i__1 / 2 * (water + oil) == k && (placom_1.place[
	    bottle - 1] == loc || placom_1.place[bottle - 1] == -1) || k == (
	    1 - i__4) * water + i__4 / 2 * (water + oil)) {
	goto L5010;
    }
    if (obj != plant || ! (placom_1.place[plant2 - 1] == loc ||
	    placom_1.fixed[plant2 - 1] == loc) || prop[plant2 - 1] == 0) {
	goto L5130;
    }
    obj = plant2;
    goto L5010;
L5130:
    if (obj != knife || knfloc != loc) {
	goto L5140;
    }
    knfloc = -1;
    spk = 116;
    goto L2011;
L5140:
    if (obj != rod || ! (placom_1.place[rod2 - 1] == loc || placom_1.place[
	    rod2 - 1] == -1)) {
	goto L5190;
    }
    obj = rod2;
    goto L5010;
L5190:
    if ((verb == find || verb == invent) && s_cmp(wd2, blanks, (ftnlen)5, (
	    ftnlen)5) == 0) {
	goto L5010;
    }
    a5toa1_(wd1, wd1x, "here.", a1, &k, (ftnlen)5, (ftnlen)5, (ftnlen)5, (
	    ftnlen)1);
    s_wsfe(&io___157);
    i__1 = k;
    for (i__ = 1; i__ <= i__1; ++i__) {
	do_fio(&c__1, a1 + (i__ - 1), (ftnlen)1);
    }
    e_wsfe();
    goto L2012;
/*  FIGURE OUT THE NEW LOCATION */

/*  GIVEN THE CURRENT LOCATION IN "LOC", AND A MOTION VERB NUMBER IN "K", PUT */
/*  THE NEW LOCATION IN "NEWLOC".  THE CURRENT LOC IS SAVED IN "OLDLOC" IN CASE */
/*  HE WANTS TO RETREAT.  THE CURRENT OLDLOC IS SAVED IN OLDLC2, IN CASE HE */
/*  DIES.  (IF HE DOES, NEWLOC WILL BE LIMBO, AND OLDLOC WILL BE WHAT KILLED */
/*  HIM, SO WE NEED OLDLC2, WHICH IS THE LAST PLACE HE WAS SAFE.) */
L8:
    kk = key[loc - 1];
    newloc = loc;
    if (kk == 0) {
	bug_(&c__26);
    }
    if (k == null) {
	goto L2;
    }
    if (k == back) {
	goto L20;
    }
    if (k == look) {
	goto L30;
    }
    if (k == cave) {
	goto L40;
    }
    oldlc2 = oldloc;
    oldloc = loc;
L9:
    ll = (i__1 = travel[kk - 1], abs(i__1));
    if (ll % 1000 == 1 || ll % 1000 == k) {
	goto L10;
    }
    if (travel[kk - 1] < 0) {
	goto L50;
    }
    ++kk;
    goto L9;
L10:
    ll /= 1000;
L11:
    newloc = ll / 1000;
    k = newloc % 100;
    if (newloc <= 300) {
	goto L13;
    }
    if (prop[k - 1] != newloc / 100 - 3) {
	goto L16;
    }
L12:
    if (travel[kk - 1] < 0) {
	bug_(&c__25);
    }
    ++kk;
    newloc = (i__1 = travel[kk - 1], abs(i__1)) / 1000;
    if (newloc == ll) {
	goto L12;
    }
    ll = newloc;
    goto L11;
L13:
    if (newloc <= 100) {
	goto L14;
    }
    if (placom_1.place[k - 1] == -1 || newloc > 200 && (placom_1.place[k - 1]
	    == loc || placom_1.fixed[k - 1] == loc)) {
	goto L16;
    }
    goto L12;
L14:
    if (newloc != 0 && ! (ran_(&c__100) < newloc)) {
	goto L12;
    }
L16:
    newloc = ll % 1000;
    if (newloc <= 300) {
	goto L2;
    }
    if (newloc <= 500) {
	goto L30000;
    }
    i__1 = newloc - 500;
    rspeak_(&i__1);
    newloc = loc;
    goto L2;
/*  SPECIAL MOTIONS COME HERE.  LABELLING CONVENTION: STATEMENT NUMBERS NNNXX */
/*  (XX=00-99) ARE USED FOR SPECIAL CASE NUMBER NNN (NNN=301-500). */
L30000:
    newloc += -300;
    switch (newloc) {
	case 1:  goto L30100;
	case 2:  goto L30200;
	case 3:  goto L30300;
    }
    bug_(&c__20);
/*  TRAVEL 301.  PLOVER-ALCOVE PASSAGE.  CAN CARRY ONLY EMERALD.  NOTE: TRAVEL */
/*  TABLE MUST INCLUDE "USELESS" ENTRIES GOING THROUGH PASSAGE, WHICH CAN NEVER */
/*  BE USED FOR ACTUAL MOTION, BUT CAN BE SPOTTED BY "GO BACK". */
L30100:
    newloc = 199 - loc;
    if (placom_1.holdng == 0 || placom_1.holdng == 1 && placom_1.place[emrald
	    - 1] == -1) {
	goto L2;
    }
    newloc = loc;
    rspeak_(&c__117);
    goto L2;
/*  TRAVEL 302.  PLOVER TRANSPORT.  DROP THE EMERALD (ONLY USE SPECIAL TRAVEL IF */
/*  TOTING IT), SO HE'S FORCED TO USE THE PLOVER-PASSAGE TO GET IT OUT.  HAVING */
/*  DROPPED IT, GO BACK AND PRETEND HE WASN'T CARRYING IT AFTER ALL. */
L30200:
    drop_(&emrald, &loc);
    goto L12;
/*  TRAVEL 303.  TROLL BRIDGE.  MUST BE DONE ONLY AS SPECIAL MOTION SO THAT */
/*  DWARVES WON'T WANDER ACROSS AND ENCOUNTER THE BEAR.  (THEY WON'T FOLLOW THE */
/*  PLAYER THERE BECAUSE THAT REGION IS FORBIDDEN TO THE PIRATE.)  IF */
/*  PROP(TROLL)=1, HE'S CROSSED SINCE PAYING, SO STEP OUT AND BLOCK HIM. */
/*  (STANDARD TRAVEL ENTRIES CHECK FOR PROP(TROLL)=0.)  SPECIAL STUFF FOR BEAR. */
L30300:
    if (prop[troll - 1] != 1) {
	goto L30310;
    }
    pspeak_(&troll, &c__1);
    prop[troll - 1] = 0;
    move_(&troll2, &c__0);
    i__1 = troll2 + 100;
    move_(&i__1, &c__0);
    move_(&troll, &plac[troll - 1]);
    i__1 = troll + 100;
    move_(&i__1, &fixd[troll - 1]);
    juggle_(&chasm);
    newloc = loc;
    goto L2;
L30310:
    newloc = plac[troll - 1] + fixd[troll - 1] - loc;
    if (prop[troll - 1] == 0) {
	prop[troll - 1] = 1;
    }
    if (! (placom_1.place[bear - 1] == -1)) {
	goto L2;
    }
    rspeak_(&c__162);
    prop[chasm - 1] = 1;
    prop[troll - 1] = 2;
    drop_(&bear, &newloc);
    placom_1.fixed[bear - 1] = -1;
    prop[bear - 1] = 3;
    if (prop[spices - 1] < 0) {
	++tally2;
    }
    oldlc2 = newloc;
    goto L99;
/*  END OF SPECIALS. */
/*  HANDLE "GO BACK".  LOOK FOR VERB WHICH GOES FROM LOC TO OLDLOC, OR TO OLDLC2 */
/*  IF OLDLOC HAS FORCED-MOTION.  K2 SAVES ENTRY -> FORCED LOC -> PREVIOUS LOC. */
L20:
    k = oldloc;
    if (cond[k - 1] == 2) {
	k = oldlc2;
    }
    oldlc2 = oldloc;
    oldloc = loc;
    k2 = 0;
    if (k != loc) {
	goto L21;
    }
    rspeak_(&c__91);
    goto L2;
L21:
    ll = (i__1 = travel[kk - 1], abs(i__1)) / 1000 % 1000;
    if (ll == k) {
	goto L25;
    }
    if (ll > 300) {
	goto L22;
    }
    j = key[ll - 1];
    if (cond[ll - 1] == 2 && (i__1 = travel[j - 1], abs(i__1)) / 1000 % 1000
	    == k) {
	k2 = kk;
    }
L22:
    if (travel[kk - 1] < 0) {
	goto L23;
    }
    ++kk;
    goto L21;
L23:
    kk = k2;
    if (kk != 0) {
	goto L25;
    }
    rspeak_(&c__140);
    goto L2;
L25:
    k = (i__1 = travel[kk - 1], abs(i__1)) % 1000;
    kk = key[loc - 1];
    goto L9;
/*  LOOK.  CAN'T GIVE MORE DETAIL.  PRETEND IT WASN'T DARK (THOUGH IT MAY "NOW" */
/*  BE DARK) SO HE WON'T FALL INTO A PIT WHILE STARING INTO THE GLOOM. */
L30:
    if (detail < 3) {
	rspeak_(&c__15);
    }
    ++detail;
    wzdark = FALSE_;
    abbcom_1.abb[loc - 1] = 0;
    goto L2;
/*  CAVE.  DIFFERENT MESSAGES DEPENDING ON WHETHER ABOVE GROUND. */
L40:
    if (loc < 8) {
	rspeak_(&c__57);
    }
    if (loc >= 8) {
	rspeak_(&c__58);
    }
    goto L2;
/*  NON-APPLICABLE MOTION.  VARIOUS MESSAGES DEPENDING ON WORD GIVEN. */
L50:
    spk = 12;
    if (k >= 43 && k <= 50) {
	spk = 9;
    }
    if (k == 29 || k == 30) {
	spk = 9;
    }
    if (k == 7 || k == 36 || k == 37) {
	spk = 10;
    }
    if (k == 11 || k == 19) {
	spk = 11;
    }
    if (verb == find || verb == invent) {
	spk = 59;
    }
    if (k == 62 || k == 65) {
	spk = 42;
    }
    if (k == 17) {
	spk = 80;
    }
    rspeak_(&spk);
    goto L2;
/*  "YOU'RE DEAD, JIM." */

/*  IF THE CURRENT LOC IS ZERO, IT MEANS THE CLOWN GOT HIMSELF KILLED.  WE'LL */
/*  ALLOW THIS MAXDIE TIMES.  MAXDIE IS AUTOMATICALLY SET BASED ON THE NUMBER OF */
/*  SNIDE MESSAGES AVAILABLE.  EACH DEATH RESULTS IN A MESSAGE (81, 83, ETC.) */
/*  WHICH OFFERS REINCARNATION; IF ACCEPTED, THIS RESULTS IN MESSAGE 82, 84, */
/*  ETC.  THE LAST TIME, IF HE WANTS ANOTHER CHANCE, HE GETS A SNIDE REMARK AS */
/*  WE EXIT.  WHEN REINCARNATED, ALL OBJECTS BEING CARRIED GET DROPPED AT OLDLC2 */
/*  (PRESUMABLY THE LAST PLACE PRIOR TO BEING KILLED) WITHOUT CHANGE OF PROPS. */
/*  THE LOOP RUNS BACKWARDS TO ASSURE THAT THE BIRD IS DROPPED BEFORE THE CAGE. */
/*  (THIS KLUGE COULD BE CHANGED ONCE WE'RE SURE ALL REFERENCES TO BIRD AND CAGE */
/*  ARE DONE BY KEYWORDS.)  THE LAMP IS A SPECIAL CASE (IT WOULDN'T DO TO LEAVE */
/*  IT IN THE CAVE).  IT IS TURNED OFF AND LEFT OUTSIDE THE BUILDING (ONLY IF HE */
/*  WAS CARRYING IT, OF COURSE).  HE HIMSELF IS LEFT INSIDE THE BUILDING (AND */
/*  HEAVEN HELP HIM IF HE TRIES TO XYZZY BACK INTO THE CAVE WITHOUT THE LAMP!). */
/*  OLDLOC IS ZAPPED SO HE CAN'T JUST "RETREAT". */
/*  THE EASIEST WAY TO GET KILLED IS TO FALL INTO A PIT IN PITCH DARKNESS. */
L90:
    advent_player_dead = 1;
    rspeak_(&c__23);
    oldlc2 = loc;
/*  OKAY, HE'S DEAD.  LET'S GET ON WITH IT. */
L99:
    advent_player_dead = 1;
    if (closng) {
	goto L95;
    }
    i__1 = (numdie << 1) + 81;
    i__2 = (numdie << 1) + 82;
    yea = yes_(&i__1, &i__2, &c__54);
    ++numdie;
    if (numdie == maxdie || ! yea) {
	goto L20000;
    }
    placom_1.place[water - 1] = 0;
    placom_1.place[oil - 1] = 0;
    if (placom_1.place[lamp - 1] == -1) {
	prop[lamp - 1] = 0;
    }
    for (j = 1; j <= 100; ++j) {
	i__ = 101 - j;
	if (! (placom_1.place[i__ - 1] == -1)) {
	    goto L98;
	}
	k = oldlc2;
	if (i__ == lamp) {
	    k = 1;
	}
	drop_(&i__, &k);
L98:
	;
	}
	loc = 3;
	advent_player_dead = 0;
	oldloc = loc;
	goto L2000;
/*  HE DIED DURING CLOSING TIME.  NO RESURRECTION.  TALLY UP A DEATH AND EXIT. */
L95:
    rspeak_(&c__131);
    ++numdie;
    goto L20000;
/*  ROUTINES FOR PERFORMING THE VARIOUS ACTION VERBS */
/*  STATEMENT NUMBERS IN THIS SECTION ARE 8000 FOR INTRANSITIVE VERBS, 9000 FOR */
/*  TRANSITIVE, PLUS TEN TIMES THE VERB NUMBER.  MANY INTRANSITIVE VERBS USE THE */
/*  TRANSITIVE CODE, AND SOME VERBS USE CODE FOR OTHER VERBS, AS NOTED BELOW. */
/*  RANDOM INTRANSITIVE VERBS COME HERE.  CLEAR OBJ JUST IN CASE (SEE "ATTACK"). */
L8000:
    a5toa1_(wd1, wd1x, "What?", a1, &k, (ftnlen)5, (ftnlen)5, (ftnlen)5, (
	    ftnlen)1);
    s_wsfe(&io___162);
    i__1 = k;
    for (i__ = 1; i__ <= i__1; ++i__) {
	do_fio(&c__1, a1 + (i__ - 1), (ftnlen)1);
    }
    e_wsfe();
    obj = 0;
    goto L2600;
/*  CARRY, NO OBJECT GIVEN YET.  OK IF ONLY ONE OBJECT PRESENT. */
L8010:
    if (placom_1.atloc[loc - 1] == 0 || placom_1.link[placom_1.atloc[loc - 1]
	    - 1] != 0) {
	goto L8000;
    }
    for (i__ = 1; i__ <= 5; ++i__) {
	if (dloc[i__ - 1] == loc && dflag >= 2) {
	    goto L8000;
	}
/* L8012: */
    }
    obj = placom_1.atloc[loc - 1];
/*  CARRY AN OBJECT.  SPECIAL CASES FOR BIRD AND CAGE (IF BIRD IN CAGE, CAN'T */
/*  TAKE ONE WITHOUT THE OTHER.  LIQUIDS ALSO SPECIAL, SINCE THEY DEPEND ON */
/*  STATUS OF BOTTLE.  ALSO VARIOUS SIDE EFFECTS, ETC. */
L9010:
    if (placom_1.place[obj - 1] == -1) {
	goto L2011;
    }
    spk = 25;
    if (obj == plant && prop[plant - 1] <= 0) {
	spk = 115;
    }
    if (obj == bear && prop[bear - 1] == 1) {
	spk = 169;
    }
    if (obj == chain && prop[bear - 1] != 0) {
	spk = 170;
    }
    if (placom_1.fixed[obj - 1] != 0) {
	goto L2011;
    }
    if (obj != water && obj != oil) {
	goto L9017;
    }
/* Computing MAX */
    i__2 = prop[bottle - 1], i__3 = -1 - prop[bottle - 1];
    i__1 = max(i__2,i__3);
    if ((placom_1.place[bottle - 1] == loc || placom_1.place[bottle - 1] ==
	    -1) && (1 - i__1) * water + i__1 / 2 * (water + oil) == obj) {
	goto L9018;
    }
    obj = bottle;
    if (placom_1.place[bottle - 1] == -1 && prop[bottle - 1] == 1) {
	goto L9220;
    }
    if (prop[bottle - 1] != 1) {
	spk = 105;
    }
    if (! (placom_1.place[bottle - 1] == -1)) {
	spk = 104;
    }
    goto L2011;
L9018:
    obj = bottle;
L9017:
    if (placom_1.holdng < 7) {
	goto L9016;
    }
    rspeak_(&c__92);
    goto L2012;
L9016:
    if (obj != bird) {
	goto L9014;
    }
    if (prop[bird - 1] != 0) {
	goto L9014;
    }
    if (! (placom_1.place[rod - 1] == -1)) {
	goto L9013;
    }
    rspeak_(&c__26);
    goto L2012;
L9013:
    if (placom_1.place[cage - 1] == -1) {
	goto L9015;
    }
    rspeak_(&c__27);
    goto L2012;
L9015:
    prop[bird - 1] = 1;
L9014:
    if ((obj == bird || obj == cage) && prop[bird - 1] != 0) {
	i__1 = bird + cage - obj;
	carry_(&i__1, &loc);
    }
    carry_(&obj, &loc);
/* Computing MAX */
    i__2 = prop[bottle - 1], i__3 = -1 - prop[bottle - 1];
    i__1 = max(i__2,i__3);
    k = (1 - i__1) * water + i__1 / 2 * (water + oil);
    if (obj == bottle && k != 0) {
	placom_1.place[k - 1] = -1;
    }
    goto L2009;
/*  DISCARD OBJECT.  "THROW" ALSO COMES HERE FOR MOST OBJECTS.  SPECIAL CASES FOR */
/*  BIRD (MIGHT ATTACK SNAKE OR DRAGON) AND CAGE (MIGHT CONTAIN BIRD) AND VASE. */
/*  DROP COINS AT VENDING MACHINE FOR EXTRA BATTERIES. */
L9020:
    if (placom_1.place[rod2 - 1] == -1 && obj == rod && ! (placom_1.place[rod
	    - 1] == -1)) {
	obj = rod2;
    }
    if (! (placom_1.place[obj - 1] == -1)) {
	goto L2011;
    }
    if (obj != bird || ! (placom_1.place[snake - 1] == loc || placom_1.place[
	    snake - 1] == -1)) {
	goto L9024;
    }
    rspeak_(&c__30);
    if (closed) {
	goto L19000;
    }
    dstroy_(&snake);
/*  SET PROP FOR USE BY TRAVEL OPTIONS */
    prop[snake - 1] = 1;
L9021:
/* Computing MAX */
    i__2 = prop[bottle - 1], i__3 = -1 - prop[bottle - 1];
    i__1 = max(i__2,i__3);
    k = (1 - i__1) * water + i__1 / 2 * (water + oil);
    if (k == obj) {
	obj = bottle;
    }
    if (obj == bottle && k != 0) {
	placom_1.place[k - 1] = 0;
    }
    if (obj == cage && prop[bird - 1] != 0) {
	drop_(&bird, &loc);
    }
    if (obj == bird) {
	prop[bird - 1] = 0;
    }
    drop_(&obj, &loc);
    goto L2012;
L9024:
    if (obj != coins || ! (placom_1.place[vend - 1] == loc || placom_1.place[
	    vend - 1] == -1)) {
	goto L9025;
    }
    dstroy_(&coins);
    drop_(&batter, &loc);
    pspeak_(&batter, &c__0);
    goto L2012;
L9025:
    if (obj != bird || ! (placom_1.place[dragon - 1] == loc || placom_1.fixed[
	    dragon - 1] == loc) || prop[dragon - 1] != 0) {
	goto L9026;
    }
    rspeak_(&c__154);
    dstroy_(&bird);
    prop[bird - 1] = 0;
    if (placom_1.place[snake - 1] == plac[snake - 1]) {
	++tally2;
    }
    goto L2012;
L9026:
    if (obj != bear || ! (placom_1.place[troll - 1] == loc || placom_1.fixed[
	    troll - 1] == loc)) {
	goto L9027;
    }
    rspeak_(&c__163);
    move_(&troll, &c__0);
    i__1 = troll + 100;
    move_(&i__1, &c__0);
    move_(&troll2, &plac[troll - 1]);
    i__1 = troll2 + 100;
    move_(&i__1, &fixd[troll - 1]);
    juggle_(&chasm);
    prop[troll - 1] = 2;
    goto L9021;
L9027:
    if (obj == vase && loc != plac[pillow - 1]) {
	goto L9028;
    }
    rspeak_(&c__54);
    goto L9021;
L9028:
    prop[vase - 1] = 2;
    if (placom_1.place[pillow - 1] == loc || placom_1.fixed[pillow - 1] ==
	    loc) {
	prop[vase - 1] = 0;
    }
    i__1 = prop[vase - 1] + 1;
    pspeak_(&vase, &i__1);
    if (prop[vase - 1] != 0) {
	placom_1.fixed[vase - 1] = -1;
    }
    goto L9021;
/*  SAY.  ECHO WD2 (OR WD1 IF NO WD2 (SAY WHAT?, ETC.).)  MAGIC WORDS OVERRIDE. */
L9030:
    a5toa1_(wd2, wd2x, "\".", a1, &k, (ftnlen)5, (ftnlen)5, (ftnlen)2, (
	    ftnlen)1);
    if (s_cmp(wd2, blanks, (ftnlen)5, (ftnlen)5) == 0) {
	a5toa1_(wd1, wd1x, "\".", a1, &k, (ftnlen)5, (ftnlen)5, (ftnlen)2, (
		ftnlen)1);
    }
    if (s_cmp(wd2, blanks, (ftnlen)5, (ftnlen)5) != 0) {
	s_copy(wd1, wd2, (ftnlen)5, (ftnlen)5);
    }
    i__ = vocab_(wd1, &c_n1, (ftnlen)5);
    if (i__ == 62 || i__ == 65 || i__ == 71 || i__ == 2025) {
	goto L9035;
    }
    s_wsfe(&io___163);
    i__1 = k;
    for (i__ = 1; i__ <= i__1; ++i__) {
	do_fio(&c__1, a1 + (i__ - 1), (ftnlen)1);
    }
    e_wsfe();
    goto L2012;
L9035:
    s_copy(wd2, blanks, (ftnlen)5, (ftnlen)5);
    obj = 0;
    goto L2630;
/*  LOCK, UNLOCK, NO OBJECT GIVEN.  ASSUME VARIOUS THINGS IF PRESENT. */
L8040:
    spk = 28;
    if (placom_1.place[clam - 1] == loc || placom_1.place[clam - 1] == -1) {
	obj = clam;
    }
    if (placom_1.place[oyster - 1] == loc || placom_1.place[oyster - 1] == -1)
	     {
	obj = oyster;
    }
    if (placom_1.place[door - 1] == loc || placom_1.fixed[door - 1] == loc) {
	obj = door;
    }
    if (placom_1.place[grate - 1] == loc || placom_1.fixed[grate - 1] == loc)
	    {
	obj = grate;
    }
    if (obj != 0 && (placom_1.place[chain - 1] == loc || placom_1.place[chain
	    - 1] == -1)) {
	goto L8000;
    }
    if (placom_1.place[chain - 1] == loc || placom_1.place[chain - 1] == -1) {
	obj = chain;
    }
    if (obj == 0) {
	goto L2011;
    }
/*  LOCK, UNLOCK OBJECT.  SPECIAL STUFF FOR OPENING CLAM/OYSTER AND FOR CHAIN. */
L9040:
    if (obj == clam || obj == oyster) {
	goto L9046;
    }
    if (obj == door) {
	spk = 111;
    }
    if (obj == door && prop[door - 1] == 1) {
	spk = 54;
    }
    if (obj == cage) {
	spk = 32;
    }
    if (obj == keys) {
	spk = 55;
    }
    if (obj == grate || obj == chain) {
	spk = 31;
    }
    if (spk != 31 || ! (placom_1.place[keys - 1] == loc || placom_1.place[
	    keys - 1] == -1)) {
	goto L2011;
    }
    if (obj == chain) {
	goto L9048;
    }
    if (! closng) {
	goto L9043;
    }
    k = 130;
    if (! panic) {
	clock2 = 15;
    }
    panic = TRUE_;
    goto L2010;
L9043:
    k = prop[grate - 1] + 34;
    prop[grate - 1] = 1;
    if (verb == lock) {
	prop[grate - 1] = 0;
    }
    k += prop[grate - 1] << 1;
    goto L2010;
/*  CLAM/OYSTER. */
L9046:
    k = 0;
    if (obj == oyster) {
	k = 1;
    }
    spk = k + 124;
    if (placom_1.place[obj - 1] == -1) {
	spk = k + 120;
    }
    if (! (placom_1.place[tridnt - 1] == -1)) {
	spk = k + 122;
    }
    if (verb == lock) {
	spk = 61;
    }
    if (spk != 124) {
	goto L2011;
    }
    dstroy_(&clam);
    drop_(&oyster, &loc);
    drop_(&pearl, &c__105);
    goto L2011;
/*  CHAIN. */
L9048:
    if (verb == lock) {
	goto L9049;
    }
    spk = 171;
    if (prop[bear - 1] == 0) {
	spk = 41;
    }
    if (prop[chain - 1] == 0) {
	spk = 37;
    }
    if (spk != 171) {
	goto L2011;
    }
    prop[chain - 1] = 0;
    placom_1.fixed[chain - 1] = 0;
    if (prop[bear - 1] != 3) {
	prop[bear - 1] = 2;
    }
    placom_1.fixed[bear - 1] = 2 - prop[bear - 1];
    goto L2011;
L9049:
    spk = 172;
    if (prop[chain - 1] != 0) {
	spk = 34;
    }
    if (loc != plac[chain - 1]) {
	spk = 173;
    }
    if (spk != 172) {
	goto L2011;
    }
    prop[chain - 1] = 2;
    if (placom_1.place[chain - 1] == -1) {
	drop_(&chain, &loc);
    }
    placom_1.fixed[chain - 1] = -1;
    goto L2011;
/*  LIGHT LAMP */
L9070:
    if (! (placom_1.place[lamp - 1] == loc || placom_1.place[lamp - 1] == -1))
	     {
	goto L2011;
    }
    spk = 184;
    if (limit < 0) {
	goto L2011;
    }
    prop[lamp - 1] = 1;
    rspeak_(&c__39);
    if (wzdark) {
	goto L2000;
    }
    goto L2012;
/*  LAMP OFF */
L9080:
    if (! (placom_1.place[lamp - 1] == loc || placom_1.place[lamp - 1] == -1))
	     {
	goto L2011;
    }
    prop[lamp - 1] = 0;
    rspeak_(&c__40);
    if (cond[loc - 1] % 2 == 0 && (prop[lamp - 1] == 0 || ! (placom_1.place[
	    lamp - 1] == loc || placom_1.place[lamp - 1] == -1))) {
	rspeak_(&c__16);
    }
    goto L2012;
/*  WAVE.  NO EFFECT UNLESS WAVING ROD AT FISSURE. */
L9090:
    if (! (placom_1.place[obj - 1] == -1) && (obj != rod || ! (placom_1.place[
	    rod2 - 1] == -1))) {
	spk = 29;
    }
    if (obj != rod || ! (placom_1.place[fissur - 1] == loc || placom_1.fixed[
	    fissur - 1] == loc) || ! (placom_1.place[obj - 1] == -1) ||
	    closng) {
	goto L2011;
    }
    prop[fissur - 1] = 1 - prop[fissur - 1];
    i__1 = 2 - prop[fissur - 1];
    pspeak_(&fissur, &i__1);
    goto L2012;
/*  ATTACK.  ASSUME TARGET IF UNAMBIGUOUS.  "THROW" ALSO LINKS HERE.  ATTACKABLE */
/*  OBJECTS FALL INTO TWO CATEGORIES: ENEMIES (SNAKE, DWARF, ETC.)  AND OTHERS */
/*  (BIRD, CLAM).  AMBIGUOUS IF TWO ENEMIES, OR IF NO ENEMIES BUT TWO OTHERS. */
L9120:
    for (i__ = 1; i__ <= 5; ++i__) {
	if (dloc[i__ - 1] == loc && dflag >= 2) {
	    goto L9122;
	}
/* L9121: */
    }
    i__ = 0;
L9122:
    if (obj != 0) {
	goto L9124;
    }
    if (i__ != 0) {
	obj = dwarf;
    }
    if (placom_1.place[snake - 1] == loc || placom_1.place[snake - 1] == -1) {
	obj = obj * 100 + snake;
    }
    if ((placom_1.place[dragon - 1] == loc || placom_1.fixed[dragon - 1] ==
	    loc) && prop[dragon - 1] == 0) {
	obj = obj * 100 + dragon;
    }
    if (placom_1.place[troll - 1] == loc || placom_1.fixed[troll - 1] == loc)
	    {
	obj = obj * 100 + troll;
    }
    if ((placom_1.place[bear - 1] == loc || placom_1.place[bear - 1] == -1) &&
	     prop[bear - 1] == 0) {
	obj = obj * 100 + bear;
    }
    if (obj > 100) {
	goto L8000;
    }
    if (obj != 0) {
	goto L9124;
    }
/*  CAN'T ATTACK BIRD BY THROWING AXE. */
    if ((placom_1.place[bird - 1] == loc || placom_1.place[bird - 1] == -1) &&
	     verb != throw) {
	obj = bird;
    }
/*  CLAM AND OYSTER BOTH TREATED AS CLAM FOR INTRANSITIVE CASE; NO HARM DONE. */
    if (placom_1.place[clam - 1] == loc || placom_1.place[clam - 1] == -1 || (
	    placom_1.place[oyster - 1] == loc || placom_1.place[oyster - 1] ==
	     -1)) {
	obj = obj * 100 + clam;
    }
    if (obj > 100) {
	goto L8000;
    }
L9124:
    if (obj != bird) {
	goto L9125;
    }
    spk = 137;
    if (closed) {
	goto L2011;
    }
    dstroy_(&bird);
    prop[bird - 1] = 0;
    if (placom_1.place[snake - 1] == plac[snake - 1]) {
	++tally2;
    }
    spk = 45;
L9125:
    if (obj == 0) {
	spk = 44;
    }
    if (obj == clam || obj == oyster) {
	spk = 150;
    }
    if (obj == snake) {
	spk = 46;
    }
    if (obj == dwarf) {
	spk = 49;
    }
    if (obj == dwarf && closed) {
	goto L19000;
    }
    if (obj == dragon) {
	spk = 167;
    }
    if (obj == troll) {
	spk = 157;
    }
    if (obj == bear) {
	spk = (prop[bear - 1] + 1) / 2 + 165;
    }
    if (obj != dragon || prop[dragon - 1] != 0) {
	goto L2011;
    }
/*  FUN STUFF FOR DRAGON.  IF HE INSISTS ON ATTACKING IT, WIN!  SET PROP TO DEAD, */
/*  MOVE DRAGON TO CENTRAL LOC (STILL FIXED), MOVE RUG THERE (NOT FIXED), AND */
/*  MOVE HIM THERE, TOO.  THEN DO A NULL MOTION TO GET NEW DESCRIPTION. */
    rspeak_(&c__49);
    verb = 0;
    obj = 0;
    getin_(wd1, wd1x, wd2, wd2x, (ftnlen)5, (ftnlen)5, (ftnlen)5, (ftnlen)5);
    if (s_cmp(wd1, "Y", (ftnlen)5, (ftnlen)1) != 0 && s_cmp(wd1, "YES", (
	    ftnlen)5, (ftnlen)3) != 0) {
	goto L2608;
    }
    pspeak_(&dragon, &c__1);
    prop[dragon - 1] = 2;
    prop[rug - 1] = 0;
    k = (plac[dragon - 1] + fixd[dragon - 1]) / 2;
    i__1 = dragon + 100;
    move_(&i__1, &c_n1);
    i__1 = rug + 100;
    move_(&i__1, &c__0);
    move_(&dragon, &k);
    move_(&rug, &k);
    for (obj = 1; obj <= 100; ++obj) {
	idondx = obj;
	if (placom_1.place[idondx - 1] == plac[dragon - 1] || placom_1.place[
		idondx - 1] == fixd[dragon - 1]) {
	    move_(&idondx, &k);
	}
/* L9126: */
    }
    loc = k;
    k = null;
    goto L8;
/*  POUR.  IF NO OBJECT, OR OBJECT IS BOTTLE, ASSUME CONTENTS OF BOTTLE. */
/*  SPECIAL TESTS FOR POURING WATER OR OIL ON PLANT OR RUSTY DOOR. */
L9130:
    if (obj == bottle || obj == 0) {
/* Computing MAX */
	i__2 = prop[bottle - 1], i__3 = -1 - prop[bottle - 1];
	i__1 = max(i__2,i__3);
	obj = (1 - i__1) * water + i__1 / 2 * (water + oil);
    }
    if (obj == 0) {
	goto L8000;
    }
    if (! (placom_1.place[obj - 1] == -1)) {
	goto L2011;
    }
    spk = 78;
    if (obj != oil && obj != water) {
	goto L2011;
    }
    prop[bottle - 1] = 1;
    placom_1.place[obj - 1] = 0;
    spk = 77;
    if (! (placom_1.place[plant - 1] == loc || placom_1.fixed[plant - 1] ==
	    loc || (placom_1.place[door - 1] == loc || placom_1.fixed[door -
	    1] == loc))) {
	goto L2011;
    }
    if (placom_1.place[door - 1] == loc || placom_1.fixed[door - 1] == loc) {
	goto L9132;
    }
    spk = 112;
    if (obj != water) {
	goto L2011;
    }
    i__1 = prop[plant - 1] + 1;
    pspeak_(&plant, &i__1);
    prop[plant - 1] = (prop[plant - 1] + 2) % 6;
    prop[plant2 - 1] = prop[plant - 1] / 2;
    k = null;
    goto L8;
L9132:
    prop[door - 1] = 0;
    if (obj == oil) {
	prop[door - 1] = 1;
    }
    spk = prop[door - 1] + 113;
    goto L2011;
/*  EAT.  INTRANSITIVE: ASSUME FOOD IF PRESENT, ELSE ASK WHAT.  TRANSITIVE: FOOD */
/*  OK, SOME THINGS LOSE APPETITE, REST ARE RIDICULOUS. */
L8140:
    if (! (placom_1.place[food - 1] == loc || placom_1.place[food - 1] == -1))
	     {
	goto L8000;
    }
L8142:
    dstroy_(&food);
    spk = 72;
    goto L2011;
L9140:
    if (obj == food) {
	goto L8142;
    }
    if (obj == bird || obj == snake || obj == clam || obj == oyster || obj ==
	    dwarf || obj == dragon || obj == troll || obj == bear) {
	spk = 71;
    }
    goto L2011;
/*  DRINK.  IF NO OBJECT, ASSUME WATER AND LOOK FOR IT HERE.  IF WATER IS IN */
/*  THE BOTTLE, DRINK THAT, ELSE MUST BE AT A WATER LOC, SO DRINK STREAM. */
L9150:
    i__1 = ((cond[loc - 1] / 2 << 1) % 8 - 5) * (cond[loc - 1] / 4 % 2) + 1;
/* Computing MAX */
    i__3 = prop[bottle - 1], i__4 = -1 - prop[bottle - 1];
    i__2 = max(i__3,i__4);
    if (obj == 0 && (1 - i__1) * water + i__1 / 2 * (water + oil) != water &&
	    ((1 - i__2) * water + i__2 / 2 * (water + oil) != water || ! (
	    placom_1.place[bottle - 1] == loc || placom_1.place[bottle - 1] ==
	     -1))) {
	goto L8000;
    }
    if (obj != 0 && obj != water) {
	spk = 110;
    }
/* Computing MAX */
    i__2 = prop[bottle - 1], i__3 = -1 - prop[bottle - 1];
    i__1 = max(i__2,i__3);
    if (spk == 110 || (1 - i__1) * water + i__1 / 2 * (water + oil) != water
	    || ! (placom_1.place[bottle - 1] == loc || placom_1.place[bottle
	    - 1] == -1)) {
	goto L2011;
    }
    prop[bottle - 1] = 1;
    placom_1.place[water - 1] = 0;
    spk = 74;
    goto L2011;
/*  RUB.  YIELDS VARIOUS SNIDE REMARKS. */
L9160:
    if (obj != lamp) {
	spk = 76;
    }
    goto L2011;
/*  THROW.  SAME AS DISCARD UNLESS AXE.  THEN SAME AS ATTACK EXCEPT IGNORE BIRD, */
/*  AND IF DWARF IS PRESENT THEN ONE MIGHT BE KILLED.  (ONLY WAY TO DO SO!) */
/*  AXE ALSO SPECIAL FOR DRAGON, BEAR, AND TROLL.  TREASURES SPECIAL FOR TROLL. */
L9170:
    if (placom_1.place[rod2 - 1] == -1 && obj == rod && ! (placom_1.place[rod
	    - 1] == -1)) {
	obj = rod2;
    }
    if (! (placom_1.place[obj - 1] == -1)) {
	goto L2011;
    }
    if (obj >= 50 && obj <= maxtrs && (placom_1.place[troll - 1] == loc ||
	    placom_1.fixed[troll - 1] == loc)) {
	goto L9178;
    }
    if (obj == food && (placom_1.place[bear - 1] == loc || placom_1.place[
	    bear - 1] == -1)) {
	goto L9177;
    }
    if (obj != axe) {
	goto L9020;
    }
    for (i__ = 1; i__ <= 5; ++i__) {
/*  NEEDN'T CHECK DFLAG IF AXE IS HERE. */
	if (dloc[i__ - 1] == loc) {
	    goto L9172;
	}
/* L9171: */
    }
    spk = 152;
    if ((placom_1.place[dragon - 1] == loc || placom_1.fixed[dragon - 1] ==
	    loc) && prop[dragon - 1] == 0) {
	goto L9175;
    }
    spk = 158;
    if (placom_1.place[troll - 1] == loc || placom_1.fixed[troll - 1] == loc)
	    {
	goto L9175;
    }
    if ((placom_1.place[bear - 1] == loc || placom_1.place[bear - 1] == -1) &&
	     prop[bear - 1] == 0) {
	goto L9176;
    }
    obj = 0;
    goto L9120;
L9172:
    spk = 48;
/*  IF SAVED NOT = -1, HE BYPASSED THE "START" CALL. */
    if (ran_(&c__3) == 0 || wizcom_1.saved != -1) {
	goto L9175;
    }
    dseen[i__ - 1] = FALSE_;
    dloc[i__ - 1] = 0;
    spk = 47;
    ++dkill;
    if (dkill == 1) {
	spk = 149;
    }
L9175:
    rspeak_(&spk);
    drop_(&axe, &loc);
    k = null;
    goto L8;
/*  THIS'LL TEACH HIM TO THROW THE AXE AT THE BEAR! */
L9176:
    spk = 164;
    drop_(&axe, &loc);
    placom_1.fixed[axe - 1] = -1;
    prop[axe - 1] = 1;
    juggle_(&bear);
    goto L2011;
/*  BUT THROWING FOOD IS ANOTHER STORY. */
L9177:
    obj = bear;
    goto L9210;
L9178:
    spk = 159;
/*  SNARF A TREASURE FOR THE TROLL. */
    drop_(&obj, &c__0);
    move_(&troll, &c__0);
    i__1 = troll + 100;
    move_(&i__1, &c__0);
    drop_(&troll2, &plac[troll - 1]);
    i__1 = troll2 + 100;
    drop_(&i__1, &fixd[troll - 1]);
    juggle_(&chasm);
    goto L2011;
/*  QUIT.  INTRANSITIVE ONLY.  VERIFY INTENT AND EXIT IF THAT'S WHAT HE WANTS. */
L8180:
    gaveup = yes_(&c__22, &c__54, &c__54);
L8185:
    if (gaveup) {
	goto L20000;
    }
    goto L2012;
/*  FIND.  MIGHT BE CARRYING IT, OR IT MIGHT BE HERE.  ELSE GIVE CAVEAT. */
L9190:
/* Computing MAX */
    i__2 = prop[bottle - 1], i__3 = -1 - prop[bottle - 1];
    i__1 = max(i__2,i__3);
    i__4 = ((cond[loc - 1] / 2 << 1) % 8 - 5) * (cond[loc - 1] / 4 % 2) + 1;
    if (placom_1.place[obj - 1] == loc || placom_1.fixed[obj - 1] == loc || (
	    1 - i__1) * water + i__1 / 2 * (water + oil) == obj && (
	    placom_1.place[bottle - 1] == loc || placom_1.fixed[bottle - 1] ==
	     loc) || k == (1 - i__4) * water + i__4 / 2 * (water + oil)) {
	spk = 94;
    }
    for (i__ = 1; i__ <= 5; ++i__) {
/* L9192: */
	if (dloc[i__ - 1] == loc && dflag >= 2 && obj == dwarf) {
	    spk = 94;
	}
    }
    if (closed) {
	spk = 138;
    }
    if (placom_1.place[obj - 1] == -1) {
	spk = 24;
    }
    goto L2011;
/*  INVENTORY.  IF OBJECT, TREAT SAME AS FIND.  ELSE REPORT ON CURRENT BURDEN. */
L8200:
    spk = 98;
    for (i__ = 1; i__ <= 100; ++i__) {
	idondx = i__;
	if (idondx == bear || ! (placom_1.place[idondx - 1] == -1)) {
	    goto L8201;
	}
	if (spk == 98) {
	    rspeak_(&c__99);
	}
	blkcom_1.blklin = FALSE_;
	pspeak_(&idondx, &c_n1);
	blkcom_1.blklin = TRUE_;
	spk = 0;
L8201:
	;
    }
    if (placom_1.place[bear - 1] == -1) {
	spk = 141;
    }
    goto L2011;
/*  FEED.  IF BIRD, NO SEED.  SNAKE, DRAGON, TROLL: QUIP.  IF DWARF, MAKE HIM */
/*  MAD.  BEAR, SPECIAL. */
L9210:
    if (obj != bird) {
	goto L9212;
    }
    spk = 100;
    goto L2011;
L9212:
    if (obj != snake && obj != dragon && obj != troll) {
	goto L9213;
    }
    spk = 102;
    if (obj == dragon && prop[dragon - 1] != 0) {
	spk = 110;
    }
    if (obj == troll) {
	spk = 182;
    }
    if (obj != snake || closed || ! (placom_1.place[bird - 1] == loc ||
	    placom_1.place[bird - 1] == -1)) {
	goto L2011;
    }
    spk = 101;
    dstroy_(&bird);
    prop[bird - 1] = 0;
    ++tally2;
    goto L2011;
L9213:
    if (obj != dwarf) {
	goto L9214;
    }
    if (! (placom_1.place[food - 1] == loc || placom_1.place[food - 1] == -1))
	     {
	goto L2011;
    }
    spk = 103;
    ++dflag;
    goto L2011;
L9214:
    if (obj != bear) {
	goto L9215;
    }
    if (prop[bear - 1] == 0) {
	spk = 102;
    }
    if (prop[bear - 1] == 3) {
	spk = 110;
    }
    if (! (placom_1.place[food - 1] == loc || placom_1.place[food - 1] == -1))
	     {
	goto L2011;
    }
    dstroy_(&food);
    prop[bear - 1] = 1;
    placom_1.fixed[axe - 1] = 0;
    prop[axe - 1] = 0;
    spk = 168;
    goto L2011;
L9215:
    spk = 14;
    goto L2011;
/*  FILL.  BOTTLE MUST BE EMPTY, AND SOME LIQUID AVAILABLE.  (VASE IS NASTY.) */
L9220:
    if (obj == vase) {
	goto L9222;
    }
    if (obj != 0 && obj != bottle) {
	goto L2011;
    }
    if (obj == 0 && ! (placom_1.place[bottle - 1] == loc || placom_1.place[
	    bottle - 1] == -1)) {
	goto L8000;
    }
    spk = 107;
    i__1 = ((cond[loc - 1] / 2 << 1) % 8 - 5) * (cond[loc - 1] / 4 % 2) + 1;
    if ((1 - i__1) * water + i__1 / 2 * (water + oil) == 0) {
	spk = 106;
    }
/* Computing MAX */
    i__2 = prop[bottle - 1], i__3 = -1 - prop[bottle - 1];
    i__1 = max(i__2,i__3);
    if ((1 - i__1) * water + i__1 / 2 * (water + oil) != 0) {
	spk = 105;
    }
    if (spk != 107) {
	goto L2011;
    }
    prop[bottle - 1] = cond[loc - 1] % 4 / 2 << 1;
/* Computing MAX */
    i__2 = prop[bottle - 1], i__3 = -1 - prop[bottle - 1];
    i__1 = max(i__2,i__3);
    k = (1 - i__1) * water + i__1 / 2 * (water + oil);
    if (placom_1.place[bottle - 1] == -1) {
	placom_1.place[k - 1] = -1;
    }
    if (k == oil) {
	spk = 108;
    }
    goto L2011;
L9222:
    spk = 29;
    i__1 = ((cond[loc - 1] / 2 << 1) % 8 - 5) * (cond[loc - 1] / 4 % 2) + 1;
    if ((1 - i__1) * water + i__1 / 2 * (water + oil) == 0) {
	spk = 144;
    }
    i__1 = ((cond[loc - 1] / 2 << 1) % 8 - 5) * (cond[loc - 1] / 4 % 2) + 1;
    if ((1 - i__1) * water + i__1 / 2 * (water + oil) == 0 || ! (
	    placom_1.place[vase - 1] == -1)) {
	goto L2011;
    }
    rspeak_(&c__145);
    prop[vase - 1] = 2;
    placom_1.fixed[vase - 1] = -1;
    goto L9024;
/*  BLAST.  NO EFFECT UNLESS YOU'VE GOT DYNAMITE, WHICH IS A NEAT TRICK! */
L9230:
    if (prop[rod2 - 1] < 0 || ! closed) {
	goto L2011;
    }
    bonus = 133;
    if (loc == 115) {
	bonus = 134;
    }
    if (placom_1.place[rod2 - 1] == loc || placom_1.place[rod2 - 1] == -1) {
	bonus = 135;
    }
    rspeak_(&bonus);
    goto L20000;
/*  SCORE.  GO TO SCORING SECTION, WHICH WILL RETURN TO 8241 IF SCORNG IS TRUE. */
L8240:
    scorng = TRUE_;
    goto L20000;
L8241:
    scorng = FALSE_;
    s_wsfe(&io___164);
    do_fio(&c__1, (char *)&score, (ftnlen)sizeof(integer));
    do_fio(&c__1, (char *)&mxscor, (ftnlen)sizeof(integer));
    e_wsfe();
    gaveup = yes_(&c__143, &c__54, &c__54);
    goto L8185;
/*  FEE FIE FOE FOO (AND FUM).  ADVANCE TO NEXT STATE IF GIVEN IN PROPER ORDER. */
/*  LOOK UP WD1 IN SECTION 3 OF VOCAB TO DETERMINE WHICH WORD WE'VE GOT.  LAST */
/*  WORD ZIPS THE EGGS BACK TO THE GIANT ROOM (UNLESS ALREADY THERE). */
L8250:
    k = vocab_(wd1, &c__3, (ftnlen)5);
    spk = 42;
    if (foobar == 1 - k) {
	goto L8252;
    }
    if (foobar != 0) {
	spk = 151;
    }
    goto L2011;
L8252:
    foobar = k;
    if (k != 4) {
	goto L2009;
    }
    foobar = 0;
    if (placom_1.place[eggs - 1] == plac[eggs - 1] || placom_1.place[eggs - 1]
	     == -1 && loc == plac[eggs - 1]) {
	goto L2011;
    }
/*  BRING BACK TROLL IF WE STEAL THE EGGS BACK FROM HIM BEFORE CROSSING. */
    if (placom_1.place[eggs - 1] == 0 && placom_1.place[troll - 1] == 0 &&
	    prop[troll - 1] == 0) {
	prop[troll - 1] = 1;
    }
    k = 2;
    if (placom_1.place[eggs - 1] == loc || placom_1.place[eggs - 1] == -1) {
	k = 1;
    }
    if (loc == plac[eggs - 1]) {
	k = 0;
    }
    move_(&eggs, &plac[eggs - 1]);
    pspeak_(&eggs, &k);
    goto L2012;
/*  BRIEF.  INTRANSITIVE ONLY.  SUPPRESS LONG DESCRIPTIONS AFTER FIRST TIME. */
L8260:
    spk = 156;
    abbnum = 10000;
    detail = 3;
    goto L2011;
/*  READ.  MAGAZINES IN DWARVISH, MESSAGE WE'VE SEEN, AND . . . OYSTER? */
L8270:
    if (placom_1.place[magzin - 1] == loc || placom_1.place[magzin - 1] == -1)
	     {
	obj = magzin;
    }
    if (placom_1.place[tablet - 1] == loc || placom_1.place[tablet - 1] == -1)
	     {
	obj = obj * 100 + tablet;
    }
    if (placom_1.place[messag - 1] == loc || placom_1.place[messag - 1] == -1)
	     {
	obj = obj * 100 + messag;
    }
    if (closed && placom_1.place[oyster - 1] == -1) {
	obj = oyster;
    }
    if (obj > 100 || obj == 0 || cond[loc - 1] % 2 == 0 && (prop[lamp - 1] ==
	    0 || ! (placom_1.place[lamp - 1] == loc || placom_1.place[lamp -
	    1] == -1))) {
	goto L8000;
    }
L9270:
    if (cond[loc - 1] % 2 == 0 && (prop[lamp - 1] == 0 || ! (placom_1.place[
	    lamp - 1] == loc || placom_1.place[lamp - 1] == -1))) {
	goto L5190;
    }
    if (obj == magzin) {
	spk = 190;
    }
    if (obj == tablet) {
	spk = 196;
    }
    if (obj == messag) {
	spk = 191;
    }
    if (obj == oyster && hinted[1] && placom_1.place[oyster - 1] == -1) {
	spk = 194;
    }
    if (obj != oyster || hinted[1] || ! (placom_1.place[oyster - 1] == -1) ||
	    ! closed) {
	goto L2011;
    }
    hinted[1] = yes_(&c__192, &c__193, &c__54);
    goto L2012;
/*  BREAK.  ONLY WORKS FOR MIRROR IN REPOSITORY AND, OF COURSE, THE VASE. */
L9280:
    if (obj == mirror) {
	spk = 148;
    }
    if (obj == vase && prop[vase - 1] == 0) {
	goto L9282;
    }
    if (obj != mirror || ! closed) {
	goto L2011;
    }
    rspeak_(&c__197);
    goto L19000;
L9282:
    spk = 198;
    if (placom_1.place[vase - 1] == -1) {
	drop_(&vase, &loc);
    }
    prop[vase - 1] = 2;
    placom_1.fixed[vase - 1] = -1;
    goto L2011;
/*  WAKE.  ONLY USE IS TO DISTURB THE DWARVES. */
L9290:
    if (obj != dwarf || ! closed) {
	goto L2011;
    }
    rspeak_(&c__199);
    goto L19000;
/*  SUSPEND.  OFFER TO EXIT LEAVING THINGS RESTARTABLE, BUT REQUIRING A DELAY */
/*  BEFORE RESTARTING (SO CAN'T SAVE THE WORLD BEFORE TRYING SOMETHING RISKY). */
/*  UPON RESTARTING, SETUP=-1 CAUSES RETURN TO 8305 TO PICK UP AGAIN. */
L8300:
    s_wsfe(&io___167);
    e_wsfe();
    goto L2012;
/* (saving disabled) */
    if (demo) {
	goto L2011;
    }
    s_wsfe(&io___168);
    do_fio(&c__1, (char *)&wizcom_1.latncy, (ftnlen)sizeof(integer));
    e_wsfe();
    if (! yes_(&c__200, &c__54, &c__54)) {
	goto L2012;
    }
/*  ADDED IN PORT. PROMPT FOR PATH AND ACTUALLY SAVE */
/* L45678: */
    s_wsfe(&io___169);
    e_wsfe();
    s_rsfe(&io___170);
    do_fio(&c__1, savfil, (ftnlen)256);
    e_rsfe();
    if (s_cmp(savfil, " ", (ftnlen)256, (ftnlen)1) == 0) {
/* L45687: */
	s_wsfe(&io___172);
	e_wsfe();
	goto L2012;
    }
    ssvd = wizcom_1.saved;
    ssvt = wizcom_1.savet;
    sstu = wizcom_1.setup;
    datime_(&wizcom_1.saved, &wizcom_1.savet);
    wizcom_1.setup = -1;
    goto L45679;
L45685:
    ciao_();
L45686:
    wizcom_1.saved = ssvd;
    wizcom_1.savet = ssvt;
    wizcom_1.setup = sstu;
    goto L2012;
L8305:
    yea = start_(&c__0);
    wizcom_1.setup = 3;
    k = null;
    goto L8;
/*  HOURS.  REPORT CURRENT NON-PRIME-TIME HOURS. */
L8310:
    mspeak_(&c__6);
    hours_();
    goto L2012;
/*  RESTORE. ADDED IN PORT */
L8320:
    s_wsfe(&io___176);
    e_wsfe();
    goto L2012;
/* (loading disabled) */
/* L8321: */
    s_rsfe(&io___177);
    do_fio(&c__1, savfil, (ftnlen)256);
    e_rsfe();
    if (s_cmp(savfil, " ", (ftnlen)256, (ftnlen)1) == 0) {
/* L8322: */
	s_wsfe(&io___178);
	e_wsfe();
	goto L2012;
    }
    goto L56789;
/*  HINTS */
/*  COME HERE IF HE'S BEEN LONG ENOUGH AT REQUIRED LOC(S) FOR SOME UNUSED HINT. */
/*  HINT NUMBER IS IN VARIABLE "HINT".  BRANCH TO QUICK TEST FOR ADDITIONAL */
/*  CONDITIONS, THEN COME BACK TO DO NEAT STUFF.  GOTO 40010 IF CONDITIONS ARE */
/*  MET AND WE WANT TO OFFER THE HINT.  GOTO 40020 TO CLEAR HINTLC BACK TO ZERO, */
/*  40030 TO TAKE NO ACTION YET. */
L40000:
    switch (hint - 3) {
	case 1:  goto L40400;
	case 2:  goto L40500;
	case 3:  goto L40600;
	case 4:  goto L40700;
	case 5:  goto L40800;
	case 6:  goto L40900;
    }
/*           CAVE  BIRD  SNAKE MAZE  DARK  WITT */
    bug_(&c__27);
L40010:
    hintlc[hint - 1] = 0;
    if (! yes_(&hints[hint + 39], &c__0, &c__54)) {
	goto L2602;
    }
    s_wsfe(&io___179);
    do_fio(&c__1, (char *)&hints[hint + 19], (ftnlen)sizeof(integer));
    e_wsfe();
    hinted[hint - 1] = yes_(&c__175, &hints[hint + 59], &c__54);
    if (hinted[hint - 1] && limit > 30) {
	limit += hints[hint + 19] * 30;
    }
L40020:
    hintlc[hint - 1] = 0;
L40030:
    goto L2602;
/*  NOW FOR THE QUICK TESTS.  SEE DATABASE DESCRIPTION FOR ONE-LINE NOTES. */
L40400:
    if (prop[grate - 1] == 0 && ! (placom_1.place[keys - 1] == loc ||
	    placom_1.place[keys - 1] == -1)) {
	goto L40010;
    }
    goto L40020;
L40500:
    if ((placom_1.place[bird - 1] == loc || placom_1.place[bird - 1] == -1) &&
	     placom_1.place[rod - 1] == -1 && obj == bird) {
	goto L40010;
    }
    goto L40030;
L40600:
    if ((placom_1.place[snake - 1] == loc || placom_1.place[snake - 1] == -1)
	    && ! (placom_1.place[bird - 1] == loc || placom_1.place[bird - 1]
	    == -1)) {
	goto L40010;
    }
    goto L40020;
L40700:
    if (placom_1.atloc[loc - 1] == 0 && placom_1.atloc[oldloc - 1] == 0 &&
	    placom_1.atloc[oldlc2 - 1] == 0 && placom_1.holdng > 1) {
	goto L40010;
    }
    goto L40020;
L40800:
    if (prop[emrald - 1] != -1 && prop[pyram - 1] == -1) {
	goto L40010;
    }
    goto L40020;
L40900:
    goto L40010;
/*  CAVE CLOSING AND SCORING */
/*  THESE SECTIONS HANDLE THE CLOSING OF THE CAVE.  THE CAVE CLOSES "CLOCK1" */
/*  TURNS AFTER THE LAST TREASURE HAS BEEN LOCATED (INCLUDING THE PIRATE'S */
/*  CHEST, WHICH MAY OF COURSE NEVER SHOW UP).  NOTE THAT THE TREASURES NEED NOT */
/*  HAVE BEEN TAKEN YET, JUST LOCATED.  HENCE CLOCK1 MUST BE LARGE ENOUGH TO GET */
/*  OUT OF THE CAVE (IT ONLY TICKS WHILE INSIDE THE CAVE).  WHEN IT HITS ZERO, */
/*  WE BRANCH TO 10000 TO START CLOSING THE CAVE, AND THEN SIT BACK AND WAIT FOR */
/*  HIM TO TRY TO GET OUT.  IF HE DOESN'T WITHIN CLOCK2 TURNS, WE CLOSE THE */
/*  CAVE; IF HE DOES TRY, WE ASSUME HE PANICS, AND GIVE HIM A FEW ADDITIONAL */
/*  TURNS TO GET FRANTIC BEFORE WE CLOSE.  WHEN CLOCK2 HITS ZERO, WE BRANCH TO */
/*  11000 TO TRANSPORT HIM INTO THE FINAL PUZZLE.  NOTE THAT THE PUZZLE DEPENDS */
/*  UPON ALL SORTS OF RANDOM THINGS.  FOR INSTANCE, THERE MUST BE NO WATER OR */
/*  OIL, SINCE THERE ARE BEANSTALKS WHICH WE DON'T WANT TO BE ABLE TO WATER, */
/*  SINCE THE CODE CAN'T HANDLE IT.  ALSO, WE CAN HAVE NO KEYS, SINCE THERE IS A */
/*  GRATE (HAVING MOVED THE FIXED OBJECT!) THERE SEPARATING HIM FROM ALL THE */
/*  TREASURES.  MOST OF THESE PROBLEMS ARISE FROM THE USE OF NEGATIVE PROP */
/*  NUMBERS TO SUPPRESS THE OBJECT DESCRIPTIONS UNTIL HE'S ACTUALLY MOVED THE */
/*  OBJECTS. */
/*  WHEN THE FIRST WARNING COMES, WE LOCK THE GRATE, DESTROY THE BRIDGE, KILL */
/*  ALL THE DWARVES (AND THE PIRATE), REMOVE THE TROLL AND BEAR (UNLESS DEAD), */
/*  AND SET "CLOSNG" TO TRUE.  LEAVE THE DRAGON; TOO MUCH TROUBLE TO MOVE IT. */
/*  FROM NOW UNTIL CLOCK2 RUNS OUT, HE CANNOT UNLOCK THE GRATE, MOVE TO ANY */
/*  LOCATION OUTSIDE THE CAVE (LOC<9), OR CREATE THE BRIDGE.  NOR CAN HE BE */
/*  RESURRECTED IF HE DIES.  NOTE THAT THE SNAKE IS ALREADY GONE, SINCE HE GOT */
/*  TO THE TREASURE ACCESSIBLE ONLY VIA THE HALL OF THE MT. KING.  ALSO, HE'S */
/*  BEEN IN GIANT ROOM (TO GET EGGS), SO WE CAN REFER TO IT.  ALSO ALSO, HE'S */
/*  GOTTEN THE PEARL, SO WE KNOW THE BIVALVE IS AN OYSTER.  *AND*, THE DWARVES */
/*  MUST HAVE BEEN ACTIVATED, SINCE WE'VE FOUND CHEST. */
L10000:
    prop[grate - 1] = 0;
    prop[fissur - 1] = 0;
    for (i__ = 1; i__ <= 6; ++i__) {
	dseen[i__ - 1] = FALSE_;
/* L10010: */
	dloc[i__ - 1] = 0;
    }
    move_(&troll, &c__0);
    i__1 = troll + 100;
    move_(&i__1, &c__0);
    move_(&troll2, &plac[troll - 1]);
    i__1 = troll2 + 100;
    move_(&i__1, &fixd[troll - 1]);
    juggle_(&chasm);
    if (prop[bear - 1] != 3) {
	dstroy_(&bear);
    }
    prop[chain - 1] = 0;
    placom_1.fixed[chain - 1] = 0;
    prop[axe - 1] = 0;
    placom_1.fixed[axe - 1] = 0;
    rspeak_(&c__129);
    clock1 = -1;
    closng = TRUE_;
    goto L19999;
/*  ONCE HE'S PANICKED, AND CLOCK2 HAS RUN OUT, WE COME HERE TO SET UP THE */
/*  STORAGE ROOM.  THE ROOM HAS TWO LOCS, HARDWIRED AS 115 (NE) AND 116 (SW). */
/*  AT THE NE END, WE PLACE EMPTY BOTTLES, A NURSERY OF PLANTS, A BED OF */
/*  OYSTERS, A PILE OF LAMPS, RODS WITH STARS, SLEEPING DWARVES, AND HIM.  AND */
/*  THE SW END WE PLACE GRATE OVER TREASURES, SNAKE PIT, COVEY OF CAGED BIRDS, */
/*  MORE RODS, AND PILLOWS.  A MIRROR STRETCHES ACROSS ONE WALL.  MANY OF THE */
/*  OBJECTS COME FROM KNOWN LOCATIONS AND/OR STATES (E.G. THE SNAKE IS KNOWN TO */
/*  HAVE BEEN DESTROYED AND NEEDN'T BE CARRIED AWAY FROM ITS OLD "PLACE"), */
/*  MAKING THE VARIOUS OBJECTS BE HANDLED DIFFERENTLY.  WE ALSO DROP ALL OTHER */
/*  OBJECTS HE MIGHT BE CARRYING (LEST HE HAVE SOME WHICH COULD CAUSE TROUBLE, */
/*  SUCH AS THE KEYS).  WE DESCRIBE THE FLASH OF LIGHT AND TRUNDLE BACK. */
L11000:
    prop[bottle - 1] = put_(&bottle, &c__115, &c__1);
    prop[plant - 1] = put_(&plant, &c__115, &c__0);
    prop[oyster - 1] = put_(&oyster, &c__115, &c__0);
    prop[lamp - 1] = put_(&lamp, &c__115, &c__0);
    prop[rod - 1] = put_(&rod, &c__115, &c__0);
    prop[dwarf - 1] = put_(&dwarf, &c__115, &c__0);
    loc = 115;
    oldloc = 115;
    newloc = 115;
/*  LEAVE THE GRATE WITH NORMAL (NON-NEGATIVE PROPERTY). */
    foo = put_(&grate, &c__116, &c__0);
    prop[snake - 1] = put_(&snake, &c__116, &c__1);
    prop[bird - 1] = put_(&bird, &c__116, &c__1);
    prop[cage - 1] = put_(&cage, &c__116, &c__0);
    prop[rod2 - 1] = put_(&rod2, &c__116, &c__0);
    prop[pillow - 1] = put_(&pillow, &c__116, &c__0);
    prop[mirror - 1] = put_(&mirror, &c__115, &c__0);
    placom_1.fixed[mirror - 1] = 116;
    for (i__ = 1; i__ <= 100; ++i__) {
	idondx = i__;
/* L11010: */
	if (placom_1.place[idondx - 1] == -1) {
	    dstroy_(&idondx);
	}
    }
    rspeak_(&c__132);
    closed = TRUE_;
    goto L2;
/*  ANOTHER WAY WE CAN FORCE AN END TO THINGS IS BY HAVING THE LAMP GIVE OUT. */
/*  WHEN IT GETS CLOSE, WE COME HERE TO WARN HIM.  WE GO TO 12000 IF THE LAMP */
/*  AND FRESH BATTERIES ARE HERE, IN WHICH CASE WE REPLACE THE BATTERIES AND */
/*  CONTINUE.  12200 IS FOR OTHER CASES OF LAMP DYING.  12400 IS WHEN IT GOES */
/*  OUT, AND 12600 IS IF HE'S WANDERED OUTSIDE AND THE LAMP IS USED UP, IN WHICH */
/*  CASE WE FORCE HIM TO GIVE UP. */
L12000:
    rspeak_(&c__188);
    prop[batter - 1] = 1;
    if (placom_1.place[batter - 1] == -1) {
	drop_(&batter, &loc);
    }
    limit += 2500;
    lmwarn = FALSE_;
    goto L19999;
L12200:
    if (lmwarn || ! (placom_1.place[lamp - 1] == loc || placom_1.place[lamp -
	    1] == -1)) {
	goto L19999;
    }
    lmwarn = TRUE_;
    spk = 187;
    if (placom_1.place[batter - 1] == 0) {
	spk = 183;
    }
    if (prop[batter - 1] == 1) {
	spk = 189;
    }
    rspeak_(&spk);
    goto L19999;
L12400:
    limit = -1;
    prop[lamp - 1] = 0;
    if (placom_1.place[lamp - 1] == loc || placom_1.place[lamp - 1] == -1) {
	rspeak_(&c__184);
    }
    goto L19999;
L12600:
    rspeak_(&c__185);
    gaveup = TRUE_;
    goto L20000;
/*  AND, OF COURSE, DEMO GAMES ARE ENDED BY THE WIZARD. */
L13000:
    mspeak_(&c__1);
    goto L20000;
/*  OH DEAR, HE'S DISTURBED THE DWARVES. */
L19000:
    rspeak_(&c__136);
/*  EXIT CODE.  WILL EVENTUALLY INCLUDE SCORING.  FOR NOW, HOWEVER, ... */
/*  THE PRESENT SCORING ALGORITHM IS AS FOLLOWS: */
/*     OBJECTIVE:          POINTS:        PRESENT TOTAL POSSIBLE: */
/*  GETTING WELL INTO CAVE   25                    25 */
/*  EACH TREASURE < CHEST    12                    60 */
/*  TREASURE CHEST ITSELF    14                    14 */
/*  EACH TREASURE > CHEST    16                   144 */
/*  SURVIVING             (MAX-NUM)*10             30 */
/*  NOT QUITTING              4                     4 */
/*  REACHING "CLOSNG"        25                    25 */
/*  "CLOSED": QUIT/KILLED    10 */
/*            KLUTZED        25 */
/*            WRONG WAY      30 */
/*            SUCCESS        45                    45 */
/*  CAME TO WITT'S END        1                     1 */
/*  ROUND OUT THE TOTAL       2                     2 */
/*                                       TOTAL:   350 */
/*  (POINTS CAN ALSO BE DEDUCTED FOR USING HINTS.) */
L20000:
    score = 0;
    mxscor = 0;
/*  FIRST TALLY UP THE TREASURES.  MUST BE IN BUILDING AND NOT BROKEN. */
/*  GIVE THE POOR GUY 2 POINTS JUST FOR FINDING EACH TREASURE. */
    i__1 = maxtrs;
    for (i__ = 50; i__ <= i__1; ++i__) {
	if (ptxcom_1.ptext[i__ - 1] == 0) {
	    goto L20010;
	}
	k = 12;
	if (i__ == chest) {
	    k = 14;
	}
	if (i__ > chest) {
	    k = 16;
	}
	if (prop[i__ - 1] >= 0) {
	    score += 2;
	}
	if (placom_1.place[i__ - 1] == 3 && prop[i__ - 1] == 0) {
	    score = score + k - 2;
	}
	mxscor += k;
L20010:
	;
    }
/*  NOW LOOK AT HOW HE FINISHED AND HOW FAR HE GOT.  MAXDIE AND NUMDIE TELL US */
/*  HOW WELL HE SURVIVED.  GAVEUP SAYS WHETHER HE EXITED VIA QUIT.  DFLAG WILL */
/*  TELL US IF HE EVER GOT SUITABLY DEEP INTO THE CAVE.  CLOSNG STILL INDICATES */
/*  WHETHER HE REACHED THE ENDGAME.  AND IF HE GOT AS FAR AS "CAVE CLOSED" */
/*  (INDICATED BY "CLOSED"), THEN BONUS IS ZERO FOR MUNDANE EXITS OR 133, 134, */
/*  135 IF HE BLEW IT (SO TO SPEAK). */
    score += (maxdie - numdie) * 10;
    mxscor += maxdie * 10;
    if (! (scorng || gaveup)) {
	score += 4;
    }
    mxscor += 4;
    if (dflag != 0) {
	score += 25;
    }
    mxscor += 25;
    if (closng) {
	score += 25;
    }
    mxscor += 25;
    if (! closed) {
	goto L20020;
    }
    if (bonus == 0) {
	score += 10;
    }
    if (bonus == 135) {
	score += 25;
    }
    if (bonus == 134) {
	score += 30;
    }
    if (bonus == 133) {
	score += 45;
    }
L20020:
    mxscor += 45;
/*  DID HE COME TO WITT'S END AS HE SHOULD? */
    if (placom_1.place[magzin - 1] == 108) {
	++score;
    }
    ++mxscor;
/*  ROUND IT OFF. */
    score += 2;
    mxscor += 2;
/*  DEDUCT POINTS FOR HINTS.  HINTS < 4 ARE SPECIAL; SEE DATABASE DESCRIPTION. */
    i__1 = hntmax;
    for (i__ = 1; i__ <= i__1; ++i__) {
/* L20030: */
	if (hinted[i__ - 1]) {
	    score -= hints[i__ + 19];
	}
    }
/*  RETURN TO SCORE COMMAND IF THAT'S WHERE WE CAME FROM. */
    if (scorng) {
	goto L8241;
    }
/*  THAT SHOULD BE GOOD ENOUGH.  LET'S TELL HIM ALL ABOUT IT. */
    s_wsfe(&io___181);
    do_fio(&c__1, (char *)&score, (ftnlen)sizeof(integer));
    do_fio(&c__1, (char *)&mxscor, (ftnlen)sizeof(integer));
    do_fio(&c__1, (char *)&turns, (ftnlen)sizeof(integer));
    e_wsfe();
    i__1 = clsses;
    for (i__ = 1; i__ <= i__1; ++i__) {
	if (cval[i__ - 1] >= score) {
	    goto L20210;
	}
/* L20200: */
    }
    s_wsfe(&io___182);
    e_wsfe();
    goto L25000;
L20210:
    speak_(&ctext[i__ - 1]);
    if (i__ == clsses - 1) {
	goto L20220;
    }
    k = cval[i__ - 1] + 1 - score;
    s_copy(a5, "s.", (ftnlen)5, (ftnlen)2);
    if (k == 1) {
	s_copy(a5, ". ", (ftnlen)5, (ftnlen)2);
    }
    s_wsfe(&io___184);
    do_fio(&c__1, (char *)&k, (ftnlen)sizeof(integer));
    do_fio(&c__1, a5, (ftnlen)5);
    e_wsfe();
    goto L25000;
L20220:
    s_wsfe(&io___185);
    e_wsfe();
L25000:
    s_stop("", (ftnlen)0);
/*  PORTING NOTE: THESE SAVE AND RESTORE ROUTINES WERE ADDED HERE, AT */
/*  THE END OF THE MAIN PROGRAM, BECAUSE I DIDN'T WANT TO CONTEND WITH */
/*  OTHERWISE MAKING SURE THAT ALL SIGNIFICANT DATA WERE IN COMMON */
/*  BLOCKS AND SUCH. */
/*  SAVE THE FULL STATE. THIS IS DONE AFTER INITIALIZING FROM THE DATA */
/*  SOURCE FILE, OR AFTER WIZARDING HAS BEEN DONE. */
/* L12345: */
    o__1.oerr = 1;
    o__1.ounit = 2;
    o__1.ofnmlen = 10;
    o__1.ofnm = "advent.bin";
    o__1.orl = 0;
    o__1.osta = "REPLACE";
    o__1.oacc = "SEQUENTIAL";
    o__1.ofm = "UNFORMATTED";
    o__1.oblnk = 0;
    i__1 = f_open(&o__1);
    if (i__1 != 0) {
	goto L12346;
    }
    i__1 = s_wsue(&io___186);
    if (i__1 != 0) {
	goto L12347;
    }
    i__1 = do_uio(&c__6, (char *)&dseen[0], (ftnlen)sizeof(logical));
    if (i__1 != 0) {
	goto L12347;
    }
    i__1 = do_uio(&c__1, (char *)&blkcom_1.blklin, (ftnlen)sizeof(logical));
    if (i__1 != 0) {
	goto L12347;
    }
    i__1 = do_uio(&c__20, (char *)&hinted[0], (ftnlen)sizeof(logical));
    if (i__1 != 0) {
	goto L12347;
    }
    i__1 = do_uio(&c__1, txtcom_1.lintxt, (ftnlen)48251);
    if (i__1 != 0) {
	goto L12347;
    }
    i__1 = do_uio(&c__300, voccom_1.atab, (ftnlen)5);
    if (i__1 != 0) {
	goto L12347;
    }
    i__1 = do_uio(&c__300, (char *)&voccom_1.ktab[0], (ftnlen)sizeof(integer))
	    ;
    if (i__1 != 0) {
	goto L12347;
    }
    i__1 = do_uio(&c__1, (char *)&voccom_1.tabsiz, (ftnlen)sizeof(integer));
    if (i__1 != 0) {
	goto L12347;
    }
    i__1 = do_uio(&c__1000, (char *)&txtcom_1.linidx[0], (ftnlen)sizeof(
	    integer));
    if (i__1 != 0) {
	goto L12347;
    }
    i__1 = do_uio(&c__205, (char *)&txtcom_1.rtext[0], (ftnlen)sizeof(integer)
	    );
    if (i__1 != 0) {
	goto L12347;
    }
    i__1 = do_uio(&c__35, (char *)&actspk[0], (ftnlen)sizeof(integer));
    if (i__1 != 0) {
	goto L12347;
    }
    i__1 = do_uio(&c__150, (char *)&placom_1.atloc[0], (ftnlen)sizeof(integer)
	    );
    if (i__1 != 0) {
	goto L12347;
    }
    i__1 = do_uio(&c__200, (char *)&placom_1.link[0], (ftnlen)sizeof(integer))
	    ;
    if (i__1 != 0) {
	goto L12347;
    }
    i__1 = do_uio(&c__100, (char *)&placom_1.place[0], (ftnlen)sizeof(integer)
	    );
    if (i__1 != 0) {
	goto L12347;
    }
    i__1 = do_uio(&c__100, (char *)&fixd[0], (ftnlen)sizeof(integer));
    if (i__1 != 0) {
	goto L12347;
    }
    i__1 = do_uio(&c__100, (char *)&placom_1.fixed[0], (ftnlen)sizeof(integer)
	    );
    if (i__1 != 0) {
	goto L12347;
    }
    i__1 = do_uio(&c__1, (char *)&placom_1.holdng, (ftnlen)sizeof(integer));
    if (i__1 != 0) {
	goto L12347;
    }
    i__1 = do_uio(&c__35, (char *)&mtxcom_1.mtext[0], (ftnlen)sizeof(integer))
	    ;
    if (i__1 != 0) {
	goto L12347;
    }
    i__1 = do_uio(&c__100, (char *)&ptxcom_1.ptext[0], (ftnlen)sizeof(integer)
	    );
    if (i__1 != 0) {
	goto L12347;
    }
    i__1 = do_uio(&c__150, (char *)&abbcom_1.abb[0], (ftnlen)sizeof(integer));
    if (i__1 != 0) {
	goto L12347;
    }
    i__1 = do_uio(&c__750, (char *)&travel[0], (ftnlen)sizeof(integer));
    if (i__1 != 0) {
	goto L12347;
    }
    i__1 = do_uio(&c__150, (char *)&ltext[0], (ftnlen)sizeof(integer));
    if (i__1 != 0) {
	goto L12347;
    }
    i__1 = do_uio(&c__150, (char *)&stext[0], (ftnlen)sizeof(integer));
    if (i__1 != 0) {
	goto L12347;
    }
    i__1 = do_uio(&c__150, (char *)&key[0], (ftnlen)sizeof(integer));
    if (i__1 != 0) {
	goto L12347;
    }
    i__1 = do_uio(&c__150, (char *)&cond[0], (ftnlen)sizeof(integer));
    if (i__1 != 0) {
	goto L12347;
    }
    i__1 = do_uio(&c__100, (char *)&plac[0], (ftnlen)sizeof(integer));
    if (i__1 != 0) {
	goto L12347;
    }
    i__1 = do_uio(&c__100, (char *)&prop[0], (ftnlen)sizeof(integer));
    if (i__1 != 0) {
	goto L12347;
    }
    i__1 = do_uio(&c__1, wizcom_1.magic, (ftnlen)5);
    if (i__1 != 0) {
	goto L12347;
    }
    i__1 = do_uio(&c__4, wizcom_1.hname, (ftnlen)5);
    if (i__1 != 0) {
	goto L12347;
    }
    i__1 = do_uio(&c__100, motcom_1.msg, (ftnlen)5);
    if (i__1 != 0) {
	goto L12347;
    }
    i__1 = do_uio(&c__50, (char *)&motcom_1.msglin[0], (ftnlen)sizeof(integer)
	    );
    if (i__1 != 0) {
	goto L12347;
    }
    i__1 = do_uio(&c__1, (char *)&wizcom_1.wkday, (ftnlen)sizeof(integer));
    if (i__1 != 0) {
	goto L12347;
    }
    i__1 = do_uio(&c__1, (char *)&wizcom_1.wkend, (ftnlen)sizeof(integer));
    if (i__1 != 0) {
	goto L12347;
    }
    i__1 = do_uio(&c__1, (char *)&wizcom_1.holid, (ftnlen)sizeof(integer));
    if (i__1 != 0) {
	goto L12347;
    }
    i__1 = do_uio(&c__1, (char *)&wizcom_1.hbegin, (ftnlen)sizeof(integer));
    if (i__1 != 0) {
	goto L12347;
    }
    i__1 = do_uio(&c__1, (char *)&wizcom_1.hend, (ftnlen)sizeof(integer));
    if (i__1 != 0) {
	goto L12347;
    }
    i__1 = do_uio(&c__1, (char *)&wizcom_1.magnm, (ftnlen)sizeof(integer));
    if (i__1 != 0) {
	goto L12347;
    }
    i__1 = do_uio(&c__1, (char *)&wizcom_1.latncy, (ftnlen)sizeof(integer));
    if (i__1 != 0) {
	goto L12347;
    }
    i__1 = do_uio(&c__1, (char *)&wizcom_1.saved, (ftnlen)sizeof(integer));
    if (i__1 != 0) {
	goto L12347;
    }
    i__1 = do_uio(&c__1, (char *)&wizcom_1.setup, (ftnlen)sizeof(integer));
    if (i__1 != 0) {
	goto L12347;
    }
    i__1 = do_uio(&c__1, (char *)&wizcom_1.short__, (ftnlen)sizeof(integer));
    if (i__1 != 0) {
	goto L12347;
    }
    i__1 = do_uio(&c__12, (char *)&ctext[0], (ftnlen)sizeof(integer));
    if (i__1 != 0) {
	goto L12347;
    }
    i__1 = do_uio(&c__12, (char *)&cval[0], (ftnlen)sizeof(integer));
    if (i__1 != 0) {
	goto L12347;
    }
    i__1 = do_uio(&c__20, (char *)&hintlc[0], (ftnlen)sizeof(integer));
    if (i__1 != 0) {
	goto L12347;
    }
    i__1 = do_uio(&c__80, (char *)&hints[0], (ftnlen)sizeof(integer));
    if (i__1 != 0) {
	goto L12347;
    }
    i__1 = do_uio(&c__20, (char *)&tk[0], (ftnlen)sizeof(integer));
    if (i__1 != 0) {
	goto L12347;
    }
    i__1 = do_uio(&c__6, (char *)&dloc[0], (ftnlen)sizeof(integer));
    if (i__1 != 0) {
	goto L12347;
    }
    i__1 = do_uio(&c__6, (char *)&odloc[0], (ftnlen)sizeof(integer));
    if (i__1 != 0) {
	goto L12347;
    }
    i__1 = do_uio(&c__1, (char *)&rnlinuse, (ftnlen)sizeof(integer));
    if (i__1 != 0) {
	goto L12347;
    }
    i__1 = do_uio(&c__1, (char *)&liduse, (ftnlen)sizeof(integer));
    if (i__1 != 0) {
	goto L12347;
    }
    i__1 = do_uio(&c__1, (char *)&trvs, (ftnlen)sizeof(integer));
    if (i__1 != 0) {
	goto L12347;
    }
    i__1 = do_uio(&c__1, (char *)&trvsiz, (ftnlen)sizeof(integer));
    if (i__1 != 0) {
	goto L12347;
    }
    i__1 = do_uio(&c__1, (char *)&tabndx, (ftnlen)sizeof(integer));
    if (i__1 != 0) {
	goto L12347;
    }
    i__1 = do_uio(&c__1, (char *)&kk, (ftnlen)sizeof(integer));
    if (i__1 != 0) {
	goto L12347;
    }
    i__1 = do_uio(&c__1, (char *)&locsiz, (ftnlen)sizeof(integer));
    if (i__1 != 0) {
	goto L12347;
    }
    i__1 = do_uio(&c__1, (char *)&obj, (ftnlen)sizeof(integer));
    if (i__1 != 0) {
	goto L12347;
    }
    i__1 = do_uio(&c__1, (char *)&k, (ftnlen)sizeof(integer));
    if (i__1 != 0) {
	goto L12347;
    }
    i__1 = do_uio(&c__1, (char *)&verb, (ftnlen)sizeof(integer));
    if (i__1 != 0) {
	goto L12347;
    }
    i__1 = do_uio(&c__1, (char *)&vrbsiz, (ftnlen)sizeof(integer));
    if (i__1 != 0) {
	goto L12347;
    }
    i__1 = do_uio(&c__1, (char *)&j, (ftnlen)sizeof(integer));
    if (i__1 != 0) {
	goto L12347;
    }
    i__1 = do_uio(&c__1, (char *)&rtxsiz, (ftnlen)sizeof(integer));
    if (i__1 != 0) {
	goto L12347;
    }
    i__1 = do_uio(&c__1, (char *)&clsses, (ftnlen)sizeof(integer));
    if (i__1 != 0) {
	goto L12347;
    }
    i__1 = do_uio(&c__1, (char *)&clsmax, (ftnlen)sizeof(integer));
    if (i__1 != 0) {
	goto L12347;
    }
    i__1 = do_uio(&c__1, (char *)&hntmax, (ftnlen)sizeof(integer));
    if (i__1 != 0) {
	goto L12347;
    }
    i__1 = do_uio(&c__1, (char *)&hntsiz, (ftnlen)sizeof(integer));
    if (i__1 != 0) {
	goto L12347;
    }
    i__1 = do_uio(&c__1, (char *)&i__, (ftnlen)sizeof(integer));
    if (i__1 != 0) {
	goto L12347;
    }
    i__1 = do_uio(&c__1, (char *)&magsiz, (ftnlen)sizeof(integer));
    if (i__1 != 0) {
	goto L12347;
    }
    i__1 = do_uio(&c__1, (char *)&abbnum, (ftnlen)sizeof(integer));
    if (i__1 != 0) {
	goto L12347;
    }
    i__1 = do_uio(&c__1, (char *)&axe, (ftnlen)sizeof(integer));
    if (i__1 != 0) {
	goto L12347;
    }
    i__1 = do_uio(&c__1, (char *)&back, (ftnlen)sizeof(integer));
    if (i__1 != 0) {
	goto L12347;
    }
    i__1 = do_uio(&c__1, (char *)&batter, (ftnlen)sizeof(integer));
    if (i__1 != 0) {
	goto L12347;
    }
    i__1 = do_uio(&c__1, (char *)&bear, (ftnlen)sizeof(integer));
    if (i__1 != 0) {
	goto L12347;
    }
    i__1 = do_uio(&c__1, (char *)&bird, (ftnlen)sizeof(integer));
    if (i__1 != 0) {
	goto L12347;
    }
    i__1 = do_uio(&c__1, (char *)&bonus, (ftnlen)sizeof(integer));
    if (i__1 != 0) {
	goto L12347;
    }
    i__1 = do_uio(&c__1, (char *)&bottle, (ftnlen)sizeof(integer));
    if (i__1 != 0) {
	goto L12347;
    }
    i__1 = do_uio(&c__1, (char *)&cage, (ftnlen)sizeof(integer));
    if (i__1 != 0) {
	goto L12347;
    }
    i__1 = do_uio(&c__1, (char *)&cave, (ftnlen)sizeof(integer));
    if (i__1 != 0) {
	goto L12347;
    }
    i__1 = do_uio(&c__1, (char *)&chain, (ftnlen)sizeof(integer));
    if (i__1 != 0) {
	goto L12347;
    }
    i__1 = do_uio(&c__1, (char *)&chasm, (ftnlen)sizeof(integer));
    if (i__1 != 0) {
	goto L12347;
    }
    i__1 = do_uio(&c__1, (char *)&chest, (ftnlen)sizeof(integer));
    if (i__1 != 0) {
	goto L12347;
    }
    i__1 = do_uio(&c__1, (char *)&chloc, (ftnlen)sizeof(integer));
    if (i__1 != 0) {
	goto L12347;
    }
    i__1 = do_uio(&c__1, (char *)&chloc2, (ftnlen)sizeof(integer));
    if (i__1 != 0) {
	goto L12347;
    }
    i__1 = do_uio(&c__1, (char *)&clam, (ftnlen)sizeof(integer));
    if (i__1 != 0) {
	goto L12347;
    }
    i__1 = do_uio(&c__1, (char *)&clock1, (ftnlen)sizeof(integer));
    if (i__1 != 0) {
	goto L12347;
    }
    i__1 = do_uio(&c__1, (char *)&clock2, (ftnlen)sizeof(integer));
    if (i__1 != 0) {
	goto L12347;
    }
    i__1 = do_uio(&c__1, (char *)&closed, (ftnlen)sizeof(logical));
    if (i__1 != 0) {
	goto L12347;
    }
    i__1 = do_uio(&c__1, (char *)&closng, (ftnlen)sizeof(logical));
    if (i__1 != 0) {
	goto L12347;
    }
    i__1 = do_uio(&c__1, (char *)&coins, (ftnlen)sizeof(integer));
    if (i__1 != 0) {
	goto L12347;
    }
    i__1 = do_uio(&c__1, (char *)&daltlc, (ftnlen)sizeof(integer));
    if (i__1 != 0) {
	goto L12347;
    }
    i__1 = do_uio(&c__1, (char *)&detail, (ftnlen)sizeof(integer));
    if (i__1 != 0) {
	goto L12347;
    }
    i__1 = do_uio(&c__1, (char *)&dflag, (ftnlen)sizeof(integer));
    if (i__1 != 0) {
	goto L12347;
    }
    i__1 = do_uio(&c__1, (char *)&dkill, (ftnlen)sizeof(integer));
    if (i__1 != 0) {
	goto L12347;
    }
    i__1 = do_uio(&c__1, (char *)&door, (ftnlen)sizeof(integer));
    if (i__1 != 0) {
	goto L12347;
    }
    i__1 = do_uio(&c__1, (char *)&dprssn, (ftnlen)sizeof(integer));
    if (i__1 != 0) {
	goto L12347;
    }
    i__1 = do_uio(&c__1, (char *)&dragon, (ftnlen)sizeof(integer));
    if (i__1 != 0) {
	goto L12347;
    }
    i__1 = do_uio(&c__1, (char *)&dwarf, (ftnlen)sizeof(integer));
    if (i__1 != 0) {
	goto L12347;
    }
    i__1 = do_uio(&c__1, (char *)&eggs, (ftnlen)sizeof(integer));
    if (i__1 != 0) {
	goto L12347;
    }
    i__1 = do_uio(&c__1, (char *)&emrald, (ftnlen)sizeof(integer));
    if (i__1 != 0) {
	goto L12347;
    }
    i__1 = do_uio(&c__1, (char *)&entrnc, (ftnlen)sizeof(integer));
    if (i__1 != 0) {
	goto L12347;
    }
    i__1 = do_uio(&c__1, (char *)&find, (ftnlen)sizeof(integer));
    if (i__1 != 0) {
	goto L12347;
    }
    i__1 = do_uio(&c__1, (char *)&fissur, (ftnlen)sizeof(integer));
    if (i__1 != 0) {
	goto L12347;
    }
    i__1 = do_uio(&c__1, (char *)&foobar, (ftnlen)sizeof(integer));
    if (i__1 != 0) {
	goto L12347;
    }
    i__1 = do_uio(&c__1, (char *)&food, (ftnlen)sizeof(integer));
    if (i__1 != 0) {
	goto L12347;
    }
    i__1 = do_uio(&c__1, (char *)&gaveup, (ftnlen)sizeof(logical));
    if (i__1 != 0) {
	goto L12347;
    }
    i__1 = do_uio(&c__1, (char *)&grate, (ftnlen)sizeof(integer));
    if (i__1 != 0) {
	goto L12347;
    }
    i__1 = do_uio(&c__1, (char *)&invent, (ftnlen)sizeof(integer));
    if (i__1 != 0) {
	goto L12347;
    }
    i__1 = do_uio(&c__1, (char *)&iwest, (ftnlen)sizeof(integer));
    if (i__1 != 0) {
	goto L12347;
    }
    i__1 = do_uio(&c__1, (char *)&keys, (ftnlen)sizeof(integer));
    if (i__1 != 0) {
	goto L12347;
    }
    i__1 = do_uio(&c__1, (char *)&knfloc, (ftnlen)sizeof(integer));
    if (i__1 != 0) {
	goto L12347;
    }
    i__1 = do_uio(&c__1, (char *)&knife, (ftnlen)sizeof(integer));
    if (i__1 != 0) {
	goto L12347;
    }
    i__1 = do_uio(&c__1, (char *)&l, (ftnlen)sizeof(integer));
    if (i__1 != 0) {
	goto L12347;
    }
    i__1 = do_uio(&c__1, (char *)&lamp, (ftnlen)sizeof(integer));
    if (i__1 != 0) {
	goto L12347;
    }
    i__1 = do_uio(&c__1, (char *)&lmwarn, (ftnlen)sizeof(logical));
    if (i__1 != 0) {
	goto L12347;
    }
    i__1 = do_uio(&c__1, (char *)&loc, (ftnlen)sizeof(integer));
    if (i__1 != 0) {
	goto L12347;
    }
    i__1 = do_uio(&c__1, (char *)&lock, (ftnlen)sizeof(integer));
    if (i__1 != 0) {
	goto L12347;
    }
    i__1 = do_uio(&c__1, (char *)&look, (ftnlen)sizeof(integer));
    if (i__1 != 0) {
	goto L12347;
    }
    i__1 = do_uio(&c__1, (char *)&magzin, (ftnlen)sizeof(integer));
    if (i__1 != 0) {
	goto L12347;
    }
    i__1 = do_uio(&c__1, (char *)&maxdie, (ftnlen)sizeof(integer));
    if (i__1 != 0) {
	goto L12347;
    }
    i__1 = do_uio(&c__1, (char *)&maxtrs, (ftnlen)sizeof(integer));
    if (i__1 != 0) {
	goto L12347;
    }
    i__1 = do_uio(&c__1, (char *)&messag, (ftnlen)sizeof(integer));
    if (i__1 != 0) {
	goto L12347;
    }
    i__1 = do_uio(&c__1, (char *)&mirror, (ftnlen)sizeof(integer));
    if (i__1 != 0) {
	goto L12347;
    }
    i__1 = do_uio(&c__1, (char *)&newloc, (ftnlen)sizeof(integer));
    if (i__1 != 0) {
	goto L12347;
    }
    i__1 = do_uio(&c__1, (char *)&nugget, (ftnlen)sizeof(integer));
    if (i__1 != 0) {
	goto L12347;
    }
    i__1 = do_uio(&c__1, (char *)&null, (ftnlen)sizeof(integer));
    if (i__1 != 0) {
	goto L12347;
    }
    i__1 = do_uio(&c__1, (char *)&numdie, (ftnlen)sizeof(integer));
    if (i__1 != 0) {
	goto L12347;
    }
    i__1 = do_uio(&c__1, (char *)&oil, (ftnlen)sizeof(integer));
    if (i__1 != 0) {
	goto L12347;
    }
    i__1 = do_uio(&c__1, (char *)&oldloc, (ftnlen)sizeof(integer));
    if (i__1 != 0) {
	goto L12347;
    }
    i__1 = do_uio(&c__1, (char *)&oyster, (ftnlen)sizeof(integer));
    if (i__1 != 0) {
	goto L12347;
    }
    i__1 = do_uio(&c__1, (char *)&panic, (ftnlen)sizeof(logical));
    if (i__1 != 0) {
	goto L12347;
    }
    i__1 = do_uio(&c__1, (char *)&pearl, (ftnlen)sizeof(integer));
    if (i__1 != 0) {
	goto L12347;
    }
    i__1 = do_uio(&c__1, (char *)&pillow, (ftnlen)sizeof(integer));
    if (i__1 != 0) {
	goto L12347;
    }
    i__1 = do_uio(&c__1, (char *)&plant, (ftnlen)sizeof(integer));
    if (i__1 != 0) {
	goto L12347;
    }
    i__1 = do_uio(&c__1, (char *)&plant2, (ftnlen)sizeof(integer));
    if (i__1 != 0) {
	goto L12347;
    }
    i__1 = do_uio(&c__1, (char *)&pyram, (ftnlen)sizeof(integer));
    if (i__1 != 0) {
	goto L12347;
    }
    i__1 = do_uio(&c__1, (char *)&rod, (ftnlen)sizeof(integer));
    if (i__1 != 0) {
	goto L12347;
    }
    i__1 = do_uio(&c__1, (char *)&rod2, (ftnlen)sizeof(integer));
    if (i__1 != 0) {
	goto L12347;
    }
    i__1 = do_uio(&c__1, (char *)&rug, (ftnlen)sizeof(integer));
    if (i__1 != 0) {
	goto L12347;
    }
    i__1 = do_uio(&c__1, (char *)&say, (ftnlen)sizeof(integer));
    if (i__1 != 0) {
	goto L12347;
    }
    i__1 = do_uio(&c__1, (char *)&scorng, (ftnlen)sizeof(logical));
    if (i__1 != 0) {
	goto L12347;
    }
    i__1 = do_uio(&c__1, (char *)&sect, (ftnlen)sizeof(integer));
    if (i__1 != 0) {
	goto L12347;
    }
    i__1 = do_uio(&c__1, (char *)&snake, (ftnlen)sizeof(integer));
    if (i__1 != 0) {
	goto L12347;
    }
    i__1 = do_uio(&c__1, (char *)&steps, (ftnlen)sizeof(integer));
    if (i__1 != 0) {
	goto L12347;
    }
    i__1 = do_uio(&c__1, (char *)&tablet, (ftnlen)sizeof(integer));
    if (i__1 != 0) {
	goto L12347;
    }
    i__1 = do_uio(&c__1, (char *)&tally, (ftnlen)sizeof(integer));
    if (i__1 != 0) {
	goto L12347;
    }
    i__1 = do_uio(&c__1, (char *)&tally2, (ftnlen)sizeof(integer));
    if (i__1 != 0) {
	goto L12347;
    }
    i__1 = do_uio(&c__1, (char *)&throw, (ftnlen)sizeof(integer));
    if (i__1 != 0) {
	goto L12347;
    }
    i__1 = do_uio(&c__1, (char *)&tridnt, (ftnlen)sizeof(integer));
    if (i__1 != 0) {
	goto L12347;
    }
    i__1 = do_uio(&c__1, (char *)&troll, (ftnlen)sizeof(integer));
    if (i__1 != 0) {
	goto L12347;
    }
    i__1 = do_uio(&c__1, (char *)&troll2, (ftnlen)sizeof(integer));
    if (i__1 != 0) {
	goto L12347;
    }
    i__1 = do_uio(&c__1, (char *)&turns, (ftnlen)sizeof(integer));
    if (i__1 != 0) {
	goto L12347;
    }
    i__1 = do_uio(&c__1, (char *)&vase, (ftnlen)sizeof(integer));
    if (i__1 != 0) {
	goto L12347;
    }
    i__1 = do_uio(&c__1, (char *)&vend, (ftnlen)sizeof(integer));
    if (i__1 != 0) {
	goto L12347;
    }
    i__1 = do_uio(&c__1, (char *)&water, (ftnlen)sizeof(integer));
    if (i__1 != 0) {
	goto L12347;
    }
    i__1 = do_uio(&c__1, (char *)&wizcom_1.savet, (ftnlen)sizeof(integer));
    if (i__1 != 0) {
	goto L12347;
    }
    i__1 = do_uio(&c__1, (char *)&demo, (ftnlen)sizeof(logical));
    if (i__1 != 0) {
	goto L12347;
    }
    i__1 = e_wsue();
    if (i__1 != 0) {
	goto L12347;
    }
    cl__1.cerr = 0;
    cl__1.cunit = 2;
    cl__1.csta = 0;
    f_clos(&cl__1);
/*  SAVE AFTER MAINTENANCE MODE? */
    if (needsv) {
	s_wsle(&io___188);
	do_lio(&c__9, &c__1, "Your changes have been saved.", (ftnlen)29);
	e_wsle();
	needsv = FALSE_;
	goto L34567;
    } else {
	goto L12348;
    }
L12346:
    s_wsle(&io___189);
    do_lio(&c__9, &c__1, "ERROR: Unable to open advent.bin for writing", (
	    ftnlen)44);
    e_wsle();
    s_stop("", (ftnlen)0);
L12347:
    s_wsle(&io___190);
    do_lio(&c__9, &c__1, "ERROR: Unable to write to advent.bin", (ftnlen)36);
    e_wsle();
    s_stop("", (ftnlen)0);
/*  READ THE FULL STATE. THIS IS DONE ON EVERY STARTUP. IF IT FAILS THEN */
/*  THE GAME WILL ATTEMPT TO RE-INITIALIZE FROM SOURCE IF THAT'S */
/*  AVAILABLE, AND THEN IT WILL SAVE ITS STATE AGAIN. */
/* L23456: */
    o__1.oerr = 1;
    o__1.ounit = 2;
    o__1.ofnmlen = 10;
    o__1.ofnm = "advent.bin";
    o__1.orl = 0;
    o__1.osta = "OLD";
    o__1.oacc = "SEQUENTIAL";
    o__1.ofm = "UNFORMATTED";
    o__1.oblnk = 0;
    i__1 = f_open(&o__1);
    if (i__1 != 0) {
	goto L23457;
    }
/*  OF COURSE THIS MUST MATCH THE WRITE STATEMENT ABOVE EXACTLY */
    i__1 = s_rsue(&io___191);
    if (i__1 != 0) {
	goto L23458;
    }
    i__1 = do_uio(&c__6, (char *)&dseen[0], (ftnlen)sizeof(logical));
    if (i__1 != 0) {
	goto L23458;
    }
    i__1 = do_uio(&c__1, (char *)&blkcom_1.blklin, (ftnlen)sizeof(logical));
    if (i__1 != 0) {
	goto L23458;
    }
    i__1 = do_uio(&c__20, (char *)&hinted[0], (ftnlen)sizeof(logical));
    if (i__1 != 0) {
	goto L23458;
    }
    i__1 = do_uio(&c__1, txtcom_1.lintxt, (ftnlen)48251);
    if (i__1 != 0) {
	goto L23458;
    }
    i__1 = do_uio(&c__300, voccom_1.atab, (ftnlen)5);
    if (i__1 != 0) {
	goto L23458;
    }
    i__1 = do_uio(&c__300, (char *)&voccom_1.ktab[0], (ftnlen)sizeof(integer))
	    ;
    if (i__1 != 0) {
	goto L23458;
    }
    i__1 = do_uio(&c__1, (char *)&voccom_1.tabsiz, (ftnlen)sizeof(integer));
    if (i__1 != 0) {
	goto L23458;
    }
    i__1 = do_uio(&c__1000, (char *)&txtcom_1.linidx[0], (ftnlen)sizeof(
	    integer));
    if (i__1 != 0) {
	goto L23458;
    }
    i__1 = do_uio(&c__205, (char *)&txtcom_1.rtext[0], (ftnlen)sizeof(integer)
	    );
    if (i__1 != 0) {
	goto L23458;
    }
    i__1 = do_uio(&c__35, (char *)&actspk[0], (ftnlen)sizeof(integer));
    if (i__1 != 0) {
	goto L23458;
    }
    i__1 = do_uio(&c__150, (char *)&placom_1.atloc[0], (ftnlen)sizeof(integer)
	    );
    if (i__1 != 0) {
	goto L23458;
    }
    i__1 = do_uio(&c__200, (char *)&placom_1.link[0], (ftnlen)sizeof(integer))
	    ;
    if (i__1 != 0) {
	goto L23458;
    }
    i__1 = do_uio(&c__100, (char *)&placom_1.place[0], (ftnlen)sizeof(integer)
	    );
    if (i__1 != 0) {
	goto L23458;
    }
    i__1 = do_uio(&c__100, (char *)&fixd[0], (ftnlen)sizeof(integer));
    if (i__1 != 0) {
	goto L23458;
    }
    i__1 = do_uio(&c__100, (char *)&placom_1.fixed[0], (ftnlen)sizeof(integer)
	    );
    if (i__1 != 0) {
	goto L23458;
    }
    i__1 = do_uio(&c__1, (char *)&placom_1.holdng, (ftnlen)sizeof(integer));
    if (i__1 != 0) {
	goto L23458;
    }
    i__1 = do_uio(&c__35, (char *)&mtxcom_1.mtext[0], (ftnlen)sizeof(integer))
	    ;
    if (i__1 != 0) {
	goto L23458;
    }
    i__1 = do_uio(&c__100, (char *)&ptxcom_1.ptext[0], (ftnlen)sizeof(integer)
	    );
    if (i__1 != 0) {
	goto L23458;
    }
    i__1 = do_uio(&c__150, (char *)&abbcom_1.abb[0], (ftnlen)sizeof(integer));
    if (i__1 != 0) {
	goto L23458;
    }
    i__1 = do_uio(&c__750, (char *)&travel[0], (ftnlen)sizeof(integer));
    if (i__1 != 0) {
	goto L23458;
    }
    i__1 = do_uio(&c__150, (char *)&ltext[0], (ftnlen)sizeof(integer));
    if (i__1 != 0) {
	goto L23458;
    }
    i__1 = do_uio(&c__150, (char *)&stext[0], (ftnlen)sizeof(integer));
    if (i__1 != 0) {
	goto L23458;
    }
    i__1 = do_uio(&c__150, (char *)&key[0], (ftnlen)sizeof(integer));
    if (i__1 != 0) {
	goto L23458;
    }
    i__1 = do_uio(&c__150, (char *)&cond[0], (ftnlen)sizeof(integer));
    if (i__1 != 0) {
	goto L23458;
    }
    i__1 = do_uio(&c__100, (char *)&plac[0], (ftnlen)sizeof(integer));
    if (i__1 != 0) {
	goto L23458;
    }
    i__1 = do_uio(&c__100, (char *)&prop[0], (ftnlen)sizeof(integer));
    if (i__1 != 0) {
	goto L23458;
    }
    i__1 = do_uio(&c__1, wizcom_1.magic, (ftnlen)5);
    if (i__1 != 0) {
	goto L23458;
    }
    i__1 = do_uio(&c__4, wizcom_1.hname, (ftnlen)5);
    if (i__1 != 0) {
	goto L23458;
    }
    i__1 = do_uio(&c__100, motcom_1.msg, (ftnlen)5);
    if (i__1 != 0) {
	goto L23458;
    }
    i__1 = do_uio(&c__50, (char *)&motcom_1.msglin[0], (ftnlen)sizeof(integer)
	    );
    if (i__1 != 0) {
	goto L23458;
    }
    i__1 = do_uio(&c__1, (char *)&wizcom_1.wkday, (ftnlen)sizeof(integer));
    if (i__1 != 0) {
	goto L23458;
    }
    i__1 = do_uio(&c__1, (char *)&wizcom_1.wkend, (ftnlen)sizeof(integer));
    if (i__1 != 0) {
	goto L23458;
    }
    i__1 = do_uio(&c__1, (char *)&wizcom_1.holid, (ftnlen)sizeof(integer));
    if (i__1 != 0) {
	goto L23458;
    }
    i__1 = do_uio(&c__1, (char *)&wizcom_1.hbegin, (ftnlen)sizeof(integer));
    if (i__1 != 0) {
	goto L23458;
    }
    i__1 = do_uio(&c__1, (char *)&wizcom_1.hend, (ftnlen)sizeof(integer));
    if (i__1 != 0) {
	goto L23458;
    }
    i__1 = do_uio(&c__1, (char *)&wizcom_1.magnm, (ftnlen)sizeof(integer));
    if (i__1 != 0) {
	goto L23458;
    }
    i__1 = do_uio(&c__1, (char *)&wizcom_1.latncy, (ftnlen)sizeof(integer));
    if (i__1 != 0) {
	goto L23458;
    }
    i__1 = do_uio(&c__1, (char *)&wizcom_1.saved, (ftnlen)sizeof(integer));
    if (i__1 != 0) {
	goto L23458;
    }
    i__1 = do_uio(&c__1, (char *)&wizcom_1.setup, (ftnlen)sizeof(integer));
    if (i__1 != 0) {
	goto L23458;
    }
    i__1 = do_uio(&c__1, (char *)&wizcom_1.short__, (ftnlen)sizeof(integer));
    if (i__1 != 0) {
	goto L23458;
    }
    i__1 = do_uio(&c__12, (char *)&ctext[0], (ftnlen)sizeof(integer));
    if (i__1 != 0) {
	goto L23458;
    }
    i__1 = do_uio(&c__12, (char *)&cval[0], (ftnlen)sizeof(integer));
    if (i__1 != 0) {
	goto L23458;
    }
    i__1 = do_uio(&c__20, (char *)&hintlc[0], (ftnlen)sizeof(integer));
    if (i__1 != 0) {
	goto L23458;
    }
    i__1 = do_uio(&c__80, (char *)&hints[0], (ftnlen)sizeof(integer));
    if (i__1 != 0) {
	goto L23458;
    }
    i__1 = do_uio(&c__20, (char *)&tk[0], (ftnlen)sizeof(integer));
    if (i__1 != 0) {
	goto L23458;
    }
    i__1 = do_uio(&c__6, (char *)&dloc[0], (ftnlen)sizeof(integer));
    if (i__1 != 0) {
	goto L23458;
    }
    i__1 = do_uio(&c__6, (char *)&odloc[0], (ftnlen)sizeof(integer));
    if (i__1 != 0) {
	goto L23458;
    }
    i__1 = do_uio(&c__1, (char *)&linuse, (ftnlen)sizeof(integer));
    if (i__1 != 0) {
	goto L23458;
    }
    i__1 = do_uio(&c__1, (char *)&liduse, (ftnlen)sizeof(integer));
    if (i__1 != 0) {
	goto L23458;
    }
    i__1 = do_uio(&c__1, (char *)&trvs, (ftnlen)sizeof(integer));
    if (i__1 != 0) {
	goto L23458;
    }
    i__1 = do_uio(&c__1, (char *)&trvsiz, (ftnlen)sizeof(integer));
    if (i__1 != 0) {
	goto L23458;
    }
    i__1 = do_uio(&c__1, (char *)&tabndx, (ftnlen)sizeof(integer));
    if (i__1 != 0) {
	goto L23458;
    }
    i__1 = do_uio(&c__1, (char *)&kk, (ftnlen)sizeof(integer));
    if (i__1 != 0) {
	goto L23458;
    }
    i__1 = do_uio(&c__1, (char *)&locsiz, (ftnlen)sizeof(integer));
    if (i__1 != 0) {
	goto L23458;
    }
    i__1 = do_uio(&c__1, (char *)&obj, (ftnlen)sizeof(integer));
    if (i__1 != 0) {
	goto L23458;
    }
    i__1 = do_uio(&c__1, (char *)&k, (ftnlen)sizeof(integer));
    if (i__1 != 0) {
	goto L23458;
    }
    i__1 = do_uio(&c__1, (char *)&verb, (ftnlen)sizeof(integer));
    if (i__1 != 0) {
	goto L23458;
    }
    i__1 = do_uio(&c__1, (char *)&vrbsiz, (ftnlen)sizeof(integer));
    if (i__1 != 0) {
	goto L23458;
    }
    i__1 = do_uio(&c__1, (char *)&j, (ftnlen)sizeof(integer));
    if (i__1 != 0) {
	goto L23458;
    }
    i__1 = do_uio(&c__1, (char *)&rtxsiz, (ftnlen)sizeof(integer));
    if (i__1 != 0) {
	goto L23458;
    }
    i__1 = do_uio(&c__1, (char *)&clsses, (ftnlen)sizeof(integer));
    if (i__1 != 0) {
	goto L23458;
    }
    i__1 = do_uio(&c__1, (char *)&clsmax, (ftnlen)sizeof(integer));
    if (i__1 != 0) {
	goto L23458;
    }
    i__1 = do_uio(&c__1, (char *)&hntmax, (ftnlen)sizeof(integer));
    if (i__1 != 0) {
	goto L23458;
    }
    i__1 = do_uio(&c__1, (char *)&hntsiz, (ftnlen)sizeof(integer));
    if (i__1 != 0) {
	goto L23458;
    }
    i__1 = do_uio(&c__1, (char *)&i__, (ftnlen)sizeof(integer));
    if (i__1 != 0) {
	goto L23458;
    }
    i__1 = do_uio(&c__1, (char *)&magsiz, (ftnlen)sizeof(integer));
    if (i__1 != 0) {
	goto L23458;
    }
    i__1 = do_uio(&c__1, (char *)&abbnum, (ftnlen)sizeof(integer));
    if (i__1 != 0) {
	goto L23458;
    }
    i__1 = do_uio(&c__1, (char *)&axe, (ftnlen)sizeof(integer));
    if (i__1 != 0) {
	goto L23458;
    }
    i__1 = do_uio(&c__1, (char *)&back, (ftnlen)sizeof(integer));
    if (i__1 != 0) {
	goto L23458;
    }
    i__1 = do_uio(&c__1, (char *)&batter, (ftnlen)sizeof(integer));
    if (i__1 != 0) {
	goto L23458;
    }
    i__1 = do_uio(&c__1, (char *)&bear, (ftnlen)sizeof(integer));
    if (i__1 != 0) {
	goto L23458;
    }
    i__1 = do_uio(&c__1, (char *)&bird, (ftnlen)sizeof(integer));
    if (i__1 != 0) {
	goto L23458;
    }
    i__1 = do_uio(&c__1, (char *)&bonus, (ftnlen)sizeof(integer));
    if (i__1 != 0) {
	goto L23458;
    }
    i__1 = do_uio(&c__1, (char *)&bottle, (ftnlen)sizeof(integer));
    if (i__1 != 0) {
	goto L23458;
    }
    i__1 = do_uio(&c__1, (char *)&cage, (ftnlen)sizeof(integer));
    if (i__1 != 0) {
	goto L23458;
    }
    i__1 = do_uio(&c__1, (char *)&cave, (ftnlen)sizeof(integer));
    if (i__1 != 0) {
	goto L23458;
    }
    i__1 = do_uio(&c__1, (char *)&chain, (ftnlen)sizeof(integer));
    if (i__1 != 0) {
	goto L23458;
    }
    i__1 = do_uio(&c__1, (char *)&chasm, (ftnlen)sizeof(integer));
    if (i__1 != 0) {
	goto L23458;
    }
    i__1 = do_uio(&c__1, (char *)&chest, (ftnlen)sizeof(integer));
    if (i__1 != 0) {
	goto L23458;
    }
    i__1 = do_uio(&c__1, (char *)&chloc, (ftnlen)sizeof(integer));
    if (i__1 != 0) {
	goto L23458;
    }
    i__1 = do_uio(&c__1, (char *)&chloc2, (ftnlen)sizeof(integer));
    if (i__1 != 0) {
	goto L23458;
    }
    i__1 = do_uio(&c__1, (char *)&clam, (ftnlen)sizeof(integer));
    if (i__1 != 0) {
	goto L23458;
    }
    i__1 = do_uio(&c__1, (char *)&clock1, (ftnlen)sizeof(integer));
    if (i__1 != 0) {
	goto L23458;
    }
    i__1 = do_uio(&c__1, (char *)&clock2, (ftnlen)sizeof(integer));
    if (i__1 != 0) {
	goto L23458;
    }
    i__1 = do_uio(&c__1, (char *)&closed, (ftnlen)sizeof(logical));
    if (i__1 != 0) {
	goto L23458;
    }
    i__1 = do_uio(&c__1, (char *)&closng, (ftnlen)sizeof(logical));
    if (i__1 != 0) {
	goto L23458;
    }
    i__1 = do_uio(&c__1, (char *)&coins, (ftnlen)sizeof(integer));
    if (i__1 != 0) {
	goto L23458;
    }
    i__1 = do_uio(&c__1, (char *)&daltlc, (ftnlen)sizeof(integer));
    if (i__1 != 0) {
	goto L23458;
    }
    i__1 = do_uio(&c__1, (char *)&detail, (ftnlen)sizeof(integer));
    if (i__1 != 0) {
	goto L23458;
    }
    i__1 = do_uio(&c__1, (char *)&dflag, (ftnlen)sizeof(integer));
    if (i__1 != 0) {
	goto L23458;
    }
    i__1 = do_uio(&c__1, (char *)&dkill, (ftnlen)sizeof(integer));
    if (i__1 != 0) {
	goto L23458;
    }
    i__1 = do_uio(&c__1, (char *)&door, (ftnlen)sizeof(integer));
    if (i__1 != 0) {
	goto L23458;
    }
    i__1 = do_uio(&c__1, (char *)&dprssn, (ftnlen)sizeof(integer));
    if (i__1 != 0) {
	goto L23458;
    }
    i__1 = do_uio(&c__1, (char *)&dragon, (ftnlen)sizeof(integer));
    if (i__1 != 0) {
	goto L23458;
    }
    i__1 = do_uio(&c__1, (char *)&dwarf, (ftnlen)sizeof(integer));
    if (i__1 != 0) {
	goto L23458;
    }
    i__1 = do_uio(&c__1, (char *)&eggs, (ftnlen)sizeof(integer));
    if (i__1 != 0) {
	goto L23458;
    }
    i__1 = do_uio(&c__1, (char *)&emrald, (ftnlen)sizeof(integer));
    if (i__1 != 0) {
	goto L23458;
    }
    i__1 = do_uio(&c__1, (char *)&entrnc, (ftnlen)sizeof(integer));
    if (i__1 != 0) {
	goto L23458;
    }
    i__1 = do_uio(&c__1, (char *)&find, (ftnlen)sizeof(integer));
    if (i__1 != 0) {
	goto L23458;
    }
    i__1 = do_uio(&c__1, (char *)&fissur, (ftnlen)sizeof(integer));
    if (i__1 != 0) {
	goto L23458;
    }
    i__1 = do_uio(&c__1, (char *)&foobar, (ftnlen)sizeof(integer));
    if (i__1 != 0) {
	goto L23458;
    }
    i__1 = do_uio(&c__1, (char *)&food, (ftnlen)sizeof(integer));
    if (i__1 != 0) {
	goto L23458;
    }
    i__1 = do_uio(&c__1, (char *)&gaveup, (ftnlen)sizeof(logical));
    if (i__1 != 0) {
	goto L23458;
    }
    i__1 = do_uio(&c__1, (char *)&grate, (ftnlen)sizeof(integer));
    if (i__1 != 0) {
	goto L23458;
    }
    i__1 = do_uio(&c__1, (char *)&invent, (ftnlen)sizeof(integer));
    if (i__1 != 0) {
	goto L23458;
    }
    i__1 = do_uio(&c__1, (char *)&iwest, (ftnlen)sizeof(integer));
    if (i__1 != 0) {
	goto L23458;
    }
    i__1 = do_uio(&c__1, (char *)&keys, (ftnlen)sizeof(integer));
    if (i__1 != 0) {
	goto L23458;
    }
    i__1 = do_uio(&c__1, (char *)&knfloc, (ftnlen)sizeof(integer));
    if (i__1 != 0) {
	goto L23458;
    }
    i__1 = do_uio(&c__1, (char *)&knife, (ftnlen)sizeof(integer));
    if (i__1 != 0) {
	goto L23458;
    }
    i__1 = do_uio(&c__1, (char *)&l, (ftnlen)sizeof(integer));
    if (i__1 != 0) {
	goto L23458;
    }
    i__1 = do_uio(&c__1, (char *)&lamp, (ftnlen)sizeof(integer));
    if (i__1 != 0) {
	goto L23458;
    }
    i__1 = do_uio(&c__1, (char *)&lmwarn, (ftnlen)sizeof(logical));
    if (i__1 != 0) {
	goto L23458;
    }
    i__1 = do_uio(&c__1, (char *)&loc, (ftnlen)sizeof(integer));
    if (i__1 != 0) {
	goto L23458;
    }
    i__1 = do_uio(&c__1, (char *)&lock, (ftnlen)sizeof(integer));
    if (i__1 != 0) {
	goto L23458;
    }
    i__1 = do_uio(&c__1, (char *)&look, (ftnlen)sizeof(integer));
    if (i__1 != 0) {
	goto L23458;
    }
    i__1 = do_uio(&c__1, (char *)&magzin, (ftnlen)sizeof(integer));
    if (i__1 != 0) {
	goto L23458;
    }
    i__1 = do_uio(&c__1, (char *)&maxdie, (ftnlen)sizeof(integer));
    if (i__1 != 0) {
	goto L23458;
    }
    i__1 = do_uio(&c__1, (char *)&maxtrs, (ftnlen)sizeof(integer));
    if (i__1 != 0) {
	goto L23458;
    }
    i__1 = do_uio(&c__1, (char *)&messag, (ftnlen)sizeof(integer));
    if (i__1 != 0) {
	goto L23458;
    }
    i__1 = do_uio(&c__1, (char *)&mirror, (ftnlen)sizeof(integer));
    if (i__1 != 0) {
	goto L23458;
    }
    i__1 = do_uio(&c__1, (char *)&newloc, (ftnlen)sizeof(integer));
    if (i__1 != 0) {
	goto L23458;
    }
    i__1 = do_uio(&c__1, (char *)&nugget, (ftnlen)sizeof(integer));
    if (i__1 != 0) {
	goto L23458;
    }
    i__1 = do_uio(&c__1, (char *)&null, (ftnlen)sizeof(integer));
    if (i__1 != 0) {
	goto L23458;
    }
    i__1 = do_uio(&c__1, (char *)&numdie, (ftnlen)sizeof(integer));
    if (i__1 != 0) {
	goto L23458;
    }
    i__1 = do_uio(&c__1, (char *)&oil, (ftnlen)sizeof(integer));
    if (i__1 != 0) {
	goto L23458;
    }
    i__1 = do_uio(&c__1, (char *)&oldloc, (ftnlen)sizeof(integer));
    if (i__1 != 0) {
	goto L23458;
    }
    i__1 = do_uio(&c__1, (char *)&oyster, (ftnlen)sizeof(integer));
    if (i__1 != 0) {
	goto L23458;
    }
    i__1 = do_uio(&c__1, (char *)&panic, (ftnlen)sizeof(logical));
    if (i__1 != 0) {
	goto L23458;
    }
    i__1 = do_uio(&c__1, (char *)&pearl, (ftnlen)sizeof(integer));
    if (i__1 != 0) {
	goto L23458;
    }
    i__1 = do_uio(&c__1, (char *)&pillow, (ftnlen)sizeof(integer));
    if (i__1 != 0) {
	goto L23458;
    }
    i__1 = do_uio(&c__1, (char *)&plant, (ftnlen)sizeof(integer));
    if (i__1 != 0) {
	goto L23458;
    }
    i__1 = do_uio(&c__1, (char *)&plant2, (ftnlen)sizeof(integer));
    if (i__1 != 0) {
	goto L23458;
    }
    i__1 = do_uio(&c__1, (char *)&pyram, (ftnlen)sizeof(integer));
    if (i__1 != 0) {
	goto L23458;
    }
    i__1 = do_uio(&c__1, (char *)&rod, (ftnlen)sizeof(integer));
    if (i__1 != 0) {
	goto L23458;
    }
    i__1 = do_uio(&c__1, (char *)&rod2, (ftnlen)sizeof(integer));
    if (i__1 != 0) {
	goto L23458;
    }
    i__1 = do_uio(&c__1, (char *)&rug, (ftnlen)sizeof(integer));
    if (i__1 != 0) {
	goto L23458;
    }
    i__1 = do_uio(&c__1, (char *)&say, (ftnlen)sizeof(integer));
    if (i__1 != 0) {
	goto L23458;
    }
    i__1 = do_uio(&c__1, (char *)&scorng, (ftnlen)sizeof(logical));
    if (i__1 != 0) {
	goto L23458;
    }
    i__1 = do_uio(&c__1, (char *)&sect, (ftnlen)sizeof(integer));
    if (i__1 != 0) {
	goto L23458;
    }
    i__1 = do_uio(&c__1, (char *)&snake, (ftnlen)sizeof(integer));
    if (i__1 != 0) {
	goto L23458;
    }
    i__1 = do_uio(&c__1, (char *)&steps, (ftnlen)sizeof(integer));
    if (i__1 != 0) {
	goto L23458;
    }
    i__1 = do_uio(&c__1, (char *)&tablet, (ftnlen)sizeof(integer));
    if (i__1 != 0) {
	goto L23458;
    }
    i__1 = do_uio(&c__1, (char *)&tally, (ftnlen)sizeof(integer));
    if (i__1 != 0) {
	goto L23458;
    }
    i__1 = do_uio(&c__1, (char *)&tally2, (ftnlen)sizeof(integer));
    if (i__1 != 0) {
	goto L23458;
    }
    i__1 = do_uio(&c__1, (char *)&throw, (ftnlen)sizeof(integer));
    if (i__1 != 0) {
	goto L23458;
    }
    i__1 = do_uio(&c__1, (char *)&tridnt, (ftnlen)sizeof(integer));
    if (i__1 != 0) {
	goto L23458;
    }
    i__1 = do_uio(&c__1, (char *)&troll, (ftnlen)sizeof(integer));
    if (i__1 != 0) {
	goto L23458;
    }
    i__1 = do_uio(&c__1, (char *)&troll2, (ftnlen)sizeof(integer));
    if (i__1 != 0) {
	goto L23458;
    }
    i__1 = do_uio(&c__1, (char *)&turns, (ftnlen)sizeof(integer));
    if (i__1 != 0) {
	goto L23458;
    }
    i__1 = do_uio(&c__1, (char *)&vase, (ftnlen)sizeof(integer));
    if (i__1 != 0) {
	goto L23458;
    }
    i__1 = do_uio(&c__1, (char *)&vend, (ftnlen)sizeof(integer));
    if (i__1 != 0) {
	goto L23458;
    }
    i__1 = do_uio(&c__1, (char *)&water, (ftnlen)sizeof(integer));
    if (i__1 != 0) {
	goto L23458;
    }
    i__1 = do_uio(&c__1, (char *)&wizcom_1.savet, (ftnlen)sizeof(integer));
    if (i__1 != 0) {
	goto L23458;
    }
    i__1 = do_uio(&c__1, (char *)&demo, (ftnlen)sizeof(logical));
    if (i__1 != 0) {
	goto L23458;
    }
    i__1 = e_rsue();
    if (i__1 != 0) {
	goto L23458;
    }
    cl__1.cerr = 0;
    cl__1.cunit = 2;
    cl__1.csta = 0;
    f_clos(&cl__1);
/*  IF WE CAN'T OPEN THE FILE, WE JUST GO BACK AND COLD-INITIALIZE. */
/*  NOTE THAT WE ALSO FALL THROUGH TO HERE FROM THE READ ABOVE. */
L23457:
    goto L23459;
L23458:
    s_wsfe(&io___192);
    do_fio(&c__1, "ERROR: Unable to read advent.bin", (ftnlen)32);
    e_wsfe();
    s_stop("", (ftnlen)0);
/*  SAVE THE ADVENTURER'S GAME IN A SMALLER SAVE FILE. HERE WE ENDEAVOR */
/*  TO SAVE ONLY THE RELEVANT STATE. */
L45679:
    o__1.oerr = 1;
    o__1.ounit = 2;
    o__1.ofnmlen = 256;
    o__1.ofnm = savfil;
    o__1.orl = 0;
    o__1.osta = "REPLACE";
    o__1.oacc = "SEQUENTIAL";
    o__1.ofm = "UNFORMATTED";
    o__1.oblnk = 0;
    i__1 = f_open(&o__1);
    if (i__1 != 0) {
	goto L45680;
    }
    i__1 = s_wsue(&io___193);
    if (i__1 != 0) {
	goto L45684;
    }
    i__1 = do_uio(&c__150, (char *)&placom_1.atloc[0], (ftnlen)sizeof(integer)
	    );
    if (i__1 != 0) {
	goto L45684;
    }
    i__1 = do_uio(&c__1, (char *)&bonus, (ftnlen)sizeof(integer));
    if (i__1 != 0) {
	goto L45684;
    }
    i__1 = do_uio(&c__1, (char *)&clock1, (ftnlen)sizeof(integer));
    if (i__1 != 0) {
	goto L45684;
    }
    i__1 = do_uio(&c__1, (char *)&clock2, (ftnlen)sizeof(integer));
    if (i__1 != 0) {
	goto L45684;
    }
    i__1 = do_uio(&c__1, (char *)&closed, (ftnlen)sizeof(logical));
    if (i__1 != 0) {
	goto L45684;
    }
    i__1 = do_uio(&c__1, (char *)&closng, (ftnlen)sizeof(logical));
    if (i__1 != 0) {
	goto L45684;
    }
    i__1 = do_uio(&c__1, (char *)&demo, (ftnlen)sizeof(logical));
    if (i__1 != 0) {
	goto L45684;
    }
    i__1 = do_uio(&c__1, (char *)&detail, (ftnlen)sizeof(integer));
    if (i__1 != 0) {
	goto L45684;
    }
    i__1 = do_uio(&c__1, (char *)&dflag, (ftnlen)sizeof(integer));
    if (i__1 != 0) {
	goto L45684;
    }
    i__1 = do_uio(&c__1, (char *)&dkill, (ftnlen)sizeof(integer));
    if (i__1 != 0) {
	goto L45684;
    }
    i__1 = do_uio(&c__6, (char *)&dloc[0], (ftnlen)sizeof(integer));
    if (i__1 != 0) {
	goto L45684;
    }
    i__1 = do_uio(&c__6, (char *)&dseen[0], (ftnlen)sizeof(logical));
    if (i__1 != 0) {
	goto L45684;
    }
    i__1 = do_uio(&c__100, (char *)&placom_1.fixed[0], (ftnlen)sizeof(integer)
	    );
    if (i__1 != 0) {
	goto L45684;
    }
    i__1 = do_uio(&c__1, (char *)&foobar, (ftnlen)sizeof(integer));
    if (i__1 != 0) {
	goto L45684;
    }
    i__1 = do_uio(&c__1, (char *)&gaveup, (ftnlen)sizeof(logical));
    if (i__1 != 0) {
	goto L45684;
    }
    i__1 = do_uio(&c__20, (char *)&hinted[0], (ftnlen)sizeof(logical));
    if (i__1 != 0) {
	goto L45684;
    }
    i__1 = do_uio(&c__20, (char *)&hintlc[0], (ftnlen)sizeof(integer));
    if (i__1 != 0) {
	goto L45684;
    }
    i__1 = do_uio(&c__1, (char *)&placom_1.holdng, (ftnlen)sizeof(integer));
    if (i__1 != 0) {
	goto L45684;
    }
    i__1 = do_uio(&c__1, (char *)&iwest, (ftnlen)sizeof(integer));
    if (i__1 != 0) {
	goto L45684;
    }
    i__1 = do_uio(&c__1, (char *)&wizcom_1.latncy, (ftnlen)sizeof(integer));
    if (i__1 != 0) {
	goto L45684;
    }
    i__1 = do_uio(&c__1, (char *)&knfloc, (ftnlen)sizeof(integer));
    if (i__1 != 0) {
	goto L45684;
    }
    i__1 = do_uio(&c__1, (char *)&limit, (ftnlen)sizeof(integer));
    if (i__1 != 0) {
	goto L45684;
    }
    i__1 = do_uio(&c__200, (char *)&placom_1.link[0], (ftnlen)sizeof(integer))
	    ;
    if (i__1 != 0) {
	goto L45684;
    }
    i__1 = do_uio(&c__1, (char *)&lmwarn, (ftnlen)sizeof(logical));
    if (i__1 != 0) {
	goto L45684;
    }
    i__1 = do_uio(&c__1, (char *)&loc, (ftnlen)sizeof(integer));
    if (i__1 != 0) {
	goto L45684;
    }
    i__1 = do_uio(&c__1, (char *)&newloc, (ftnlen)sizeof(integer));
    if (i__1 != 0) {
	goto L45684;
    }
    i__1 = do_uio(&c__1, (char *)&numdie, (ftnlen)sizeof(integer));
    if (i__1 != 0) {
	goto L45684;
    }
    i__1 = do_uio(&c__6, (char *)&odloc[0], (ftnlen)sizeof(integer));
    if (i__1 != 0) {
	goto L45684;
    }
    i__1 = do_uio(&c__1, (char *)&oldloc, (ftnlen)sizeof(integer));
    if (i__1 != 0) {
	goto L45684;
    }
    i__1 = do_uio(&c__1, (char *)&panic, (ftnlen)sizeof(logical));
    if (i__1 != 0) {
	goto L45684;
    }
    i__1 = do_uio(&c__100, (char *)&placom_1.place[0], (ftnlen)sizeof(integer)
	    );
    if (i__1 != 0) {
	goto L45684;
    }
    i__1 = do_uio(&c__100, (char *)&prop[0], (ftnlen)sizeof(integer));
    if (i__1 != 0) {
	goto L45684;
    }
    i__1 = do_uio(&c__1, (char *)&wizcom_1.saved, (ftnlen)sizeof(integer));
    if (i__1 != 0) {
	goto L45684;
    }
    i__1 = do_uio(&c__1, (char *)&wizcom_1.savet, (ftnlen)sizeof(integer));
    if (i__1 != 0) {
	goto L45684;
    }
    i__1 = do_uio(&c__1, (char *)&wizcom_1.setup, (ftnlen)sizeof(integer));
    if (i__1 != 0) {
	goto L45684;
    }
    i__1 = do_uio(&c__1, (char *)&tally, (ftnlen)sizeof(integer));
    if (i__1 != 0) {
	goto L45684;
    }
    i__1 = do_uio(&c__1, (char *)&tally2, (ftnlen)sizeof(integer));
    if (i__1 != 0) {
	goto L45684;
    }
    i__1 = do_uio(&c__20, (char *)&tk[0], (ftnlen)sizeof(integer));
    if (i__1 != 0) {
	goto L45684;
    }
    i__1 = do_uio(&c__1, (char *)&turns, (ftnlen)sizeof(integer));
    if (i__1 != 0) {
	goto L45684;
    }
    i__1 = e_wsue();
    if (i__1 != 0) {
	goto L45684;
    }
    cl__1.cerr = 0;
    cl__1.cunit = 2;
    cl__1.csta = 0;
    f_clos(&cl__1);
    goto L45685;
/* L45681: */
L45680:
    for (q = 256; q >= 1; --q) {
/* L45682: */
	if (*(unsigned char *)&savfil[q - 1] != ' ') {
	    goto L45683;
	}
    }
L45683:
    s_wsfe(&io___195);
    do_fio(&c__1, savfil, q);
    e_wsfe();
    goto L45686;
L45684:
    s_wsfe(&io___196);
    do_fio(&c__1, "ERROR: Failed to write to the save file", (ftnlen)39);
    e_wsfe();
    cl__1.cerr = 0;
    cl__1.cunit = 2;
    cl__1.csta = 0;
    f_clos(&cl__1);
    goto L45686;
/*  RESUME THE ADVENTURER'S GAME FROM A SAVE FILE. AFTER READING IN THE */
/*  STATE INFORMATION, WE START OVER. */
L56789:
    o__1.oerr = 1;
    o__1.ounit = 2;
    o__1.ofnmlen = 256;
    o__1.ofnm = savfil;
    o__1.orl = 0;
    o__1.osta = "OLD";
    o__1.oacc = "SEQUENTIAL";
    o__1.ofm = "UNFORMATTED";
    o__1.oblnk = 0;
    i__1 = f_open(&o__1);
    if (i__1 != 0) {
	goto L56790;
    }
    i__1 = s_rsue(&io___197);
    if (i__1 != 0) {
	goto L56794;
    }
    i__1 = do_uio(&c__150, (char *)&placom_1.atloc[0], (ftnlen)sizeof(integer)
	    );
    if (i__1 != 0) {
	goto L56794;
    }
    i__1 = do_uio(&c__1, (char *)&bonus, (ftnlen)sizeof(integer));
    if (i__1 != 0) {
	goto L56794;
    }
    i__1 = do_uio(&c__1, (char *)&clock1, (ftnlen)sizeof(integer));
    if (i__1 != 0) {
	goto L56794;
    }
    i__1 = do_uio(&c__1, (char *)&clock2, (ftnlen)sizeof(integer));
    if (i__1 != 0) {
	goto L56794;
    }
    i__1 = do_uio(&c__1, (char *)&closed, (ftnlen)sizeof(logical));
    if (i__1 != 0) {
	goto L56794;
    }
    i__1 = do_uio(&c__1, (char *)&closng, (ftnlen)sizeof(logical));
    if (i__1 != 0) {
	goto L56794;
    }
    i__1 = do_uio(&c__1, (char *)&demo, (ftnlen)sizeof(logical));
    if (i__1 != 0) {
	goto L56794;
    }
    i__1 = do_uio(&c__1, (char *)&detail, (ftnlen)sizeof(integer));
    if (i__1 != 0) {
	goto L56794;
    }
    i__1 = do_uio(&c__1, (char *)&dflag, (ftnlen)sizeof(integer));
    if (i__1 != 0) {
	goto L56794;
    }
    i__1 = do_uio(&c__1, (char *)&dkill, (ftnlen)sizeof(integer));
    if (i__1 != 0) {
	goto L56794;
    }
    i__1 = do_uio(&c__6, (char *)&dloc[0], (ftnlen)sizeof(integer));
    if (i__1 != 0) {
	goto L56794;
    }
    i__1 = do_uio(&c__6, (char *)&dseen[0], (ftnlen)sizeof(logical));
    if (i__1 != 0) {
	goto L56794;
    }
    i__1 = do_uio(&c__100, (char *)&placom_1.fixed[0], (ftnlen)sizeof(integer)
	    );
    if (i__1 != 0) {
	goto L56794;
    }
    i__1 = do_uio(&c__1, (char *)&foobar, (ftnlen)sizeof(integer));
    if (i__1 != 0) {
	goto L56794;
    }
    i__1 = do_uio(&c__1, (char *)&gaveup, (ftnlen)sizeof(logical));
    if (i__1 != 0) {
	goto L56794;
    }
    i__1 = do_uio(&c__20, (char *)&hinted[0], (ftnlen)sizeof(logical));
    if (i__1 != 0) {
	goto L56794;
    }
    i__1 = do_uio(&c__20, (char *)&hintlc[0], (ftnlen)sizeof(integer));
    if (i__1 != 0) {
	goto L56794;
    }
    i__1 = do_uio(&c__1, (char *)&placom_1.holdng, (ftnlen)sizeof(integer));
    if (i__1 != 0) {
	goto L56794;
    }
    i__1 = do_uio(&c__1, (char *)&iwest, (ftnlen)sizeof(integer));
    if (i__1 != 0) {
	goto L56794;
    }
    i__1 = do_uio(&c__1, (char *)&wizcom_1.latncy, (ftnlen)sizeof(integer));
    if (i__1 != 0) {
	goto L56794;
    }
    i__1 = do_uio(&c__1, (char *)&knfloc, (ftnlen)sizeof(integer));
    if (i__1 != 0) {
	goto L56794;
    }
    i__1 = do_uio(&c__1, (char *)&limit, (ftnlen)sizeof(integer));
    if (i__1 != 0) {
	goto L56794;
    }
    i__1 = do_uio(&c__200, (char *)&placom_1.link[0], (ftnlen)sizeof(integer))
	    ;
    if (i__1 != 0) {
	goto L56794;
    }
    i__1 = do_uio(&c__1, (char *)&lmwarn, (ftnlen)sizeof(logical));
    if (i__1 != 0) {
	goto L56794;
    }
    i__1 = do_uio(&c__1, (char *)&loc, (ftnlen)sizeof(integer));
    if (i__1 != 0) {
	goto L56794;
    }
    i__1 = do_uio(&c__1, (char *)&newloc, (ftnlen)sizeof(integer));
    if (i__1 != 0) {
	goto L56794;
    }
    i__1 = do_uio(&c__1, (char *)&numdie, (ftnlen)sizeof(integer));
    if (i__1 != 0) {
	goto L56794;
    }
    i__1 = do_uio(&c__6, (char *)&odloc[0], (ftnlen)sizeof(integer));
    if (i__1 != 0) {
	goto L56794;
    }
    i__1 = do_uio(&c__1, (char *)&oldloc, (ftnlen)sizeof(integer));
    if (i__1 != 0) {
	goto L56794;
    }
    i__1 = do_uio(&c__1, (char *)&panic, (ftnlen)sizeof(logical));
    if (i__1 != 0) {
	goto L56794;
    }
    i__1 = do_uio(&c__100, (char *)&placom_1.place[0], (ftnlen)sizeof(integer)
	    );
    if (i__1 != 0) {
	goto L56794;
    }
    i__1 = do_uio(&c__100, (char *)&prop[0], (ftnlen)sizeof(integer));
    if (i__1 != 0) {
	goto L56794;
    }
    i__1 = do_uio(&c__1, (char *)&wizcom_1.saved, (ftnlen)sizeof(integer));
    if (i__1 != 0) {
	goto L56794;
    }
    i__1 = do_uio(&c__1, (char *)&wizcom_1.savet, (ftnlen)sizeof(integer));
    if (i__1 != 0) {
	goto L56794;
    }
    i__1 = do_uio(&c__1, (char *)&wizcom_1.setup, (ftnlen)sizeof(integer));
    if (i__1 != 0) {
	goto L56794;
    }
    i__1 = do_uio(&c__1, (char *)&tally, (ftnlen)sizeof(integer));
    if (i__1 != 0) {
	goto L56794;
    }
    i__1 = do_uio(&c__1, (char *)&tally2, (ftnlen)sizeof(integer));
    if (i__1 != 0) {
	goto L56794;
    }
    i__1 = do_uio(&c__20, (char *)&tk[0], (ftnlen)sizeof(integer));
    if (i__1 != 0) {
	goto L56794;
    }
    i__1 = do_uio(&c__1, (char *)&turns, (ftnlen)sizeof(integer));
    if (i__1 != 0) {
	goto L56794;
    }
    i__1 = e_rsue();
    if (i__1 != 0) {
	goto L56794;
    }
    cl__1.cerr = 0;
    cl__1.cunit = 2;
    cl__1.csta = 0;
    f_clos(&cl__1);
    goto L23459;
/* L56791: */
L56790:
    for (q = 256; q >= 1; --q) {
/* L56792: */
	if (*(unsigned char *)&savfil[q - 1] != ' ') {
	    goto L56793;
	}
    }
L56793:
    s_wsfe(&io___198);
    do_fio(&c__1, savfil, q);
    e_wsfe();
    goto L2012;
L56794:
    s_wsfe(&io___199);
    do_fio(&c__1, "ERROR: Failed to read from the save file", (ftnlen)40);
    e_wsfe();
    cl__1.cerr = 0;
    cl__1.cunit = 2;
    cl__1.csta = 0;
    f_clos(&cl__1);
    goto L2010;
} /* MAIN__ */

/*  I/O ROUTINES (SPEAK, PSPEAK, RSPEAK, GETIN, YES, A5TOA1) */
/* Subroutine */ int speak_(integer *n)
{
    /* Format strings */
    static char fmt_2[] = "(1x,a)";

    /* System generated locals */
    integer i__1;

    /* Builtin functions */
    integer s_cmp(char *, char *, ftnlen, ftnlen), s_wsfe(cilist *), e_wsfe(
	    void), do_fio(integer *, char *, ftnlen);

    /* Local variables */
    static integer i__, k, l;

    /* Fortran I/O blocks */
    static cilist io___201 = { 0, 6, 0, fmt_2, 0 };
    static cilist io___204 = { 0, 6, 0, fmt_2, 0 };


/*  PRINT THE MESSAGE WHICH STARTS AT LINES(N).  PRECEDE IT WITH A BLANK LINE */
/*  UNLESS BLKLIN IS FALSE. */
    if (*n == 0) {
	return 0;
    }
    k = (i__1 = txtcom_1.linidx[*n - 1], abs(i__1));
    if (s_cmp(txtcom_1.lintxt + (k - 1), ">$<", (ftnlen)3, (ftnlen)3) == 0) {
	return 0;
    }
    if (blkcom_1.blklin) {
	s_wsfe(&io___201);
	e_wsfe();
    }
    i__ = *n;
L1:
    l = (i__1 = txtcom_1.linidx[i__], abs(i__1)) - 1;
    s_wsfe(&io___204);
    do_fio(&c__1, txtcom_1.lintxt + (k - 1), l - (k - 1));
    e_wsfe();
    ++i__;
    k = l + 1;
    if (txtcom_1.linidx[i__ - 1] >= 0) {
	goto L1;
    }
    return 0;
} /* speak_ */

/* Subroutine */ int pspeak_(integer *msg, integer *skip)
{
    /* System generated locals */
    integer i__1;

    /* Local variables */
    static integer i__, m;
    extern /* Subroutine */ int speak_(integer *);

/*  FIND THE SKIP+1ST MESSAGE FROM MSG AND PRINT IT.  MSG SHOULD BE THE INDEX OF */
/*  THE INVENTORY MESSAGE FOR OBJECT.  (INVEN+N+1 MESSAGE IS PROP=N MESSAGE). */
    m = ptxcom_1.ptext[*msg - 1];
    if (*skip < 0) {
	goto L9;
    }
    i__1 = *skip;
    for (i__ = 0; i__ <= i__1; ++i__) {
L1:
	++m;
	if (txtcom_1.linidx[m - 1] >= 0) {
	    goto L1;
	}
/* L3: */
    }
L9:
    speak_(&m);
    return 0;
} /* pspeak_ */

/* Subroutine */ int rspeak_(integer *i__)
{
    extern /* Subroutine */ int speak_(integer *);

/*  PRINT THE I-TH "RANDOM" MESSAGE (SECTION 6 OF DATABASE). */
    if (*i__ != 0) {
	speak_(&txtcom_1.rtext[*i__ - 1]);
    }
    return 0;
} /* rspeak_ */

/* Subroutine */ int mspeak_(integer *i__)
{
    extern /* Subroutine */ int speak_(integer *);

/*  PRINT THE I-TH "MAGIC" MESSAGE (SECTION 12 OF DATABASE). */
    if (*i__ != 0) {
	speak_(&mtxcom_1.mtext[*i__ - 1]);
    }
    return 0;
} /* mspeak_ */

/* Subroutine */ int getin_OLD_(char *word1, char *word1x, char *word2, char *
	word2x, ftnlen word1_len, ftnlen word1x_len, ftnlen word2_len, ftnlen
	word2x_len)
{
    /* Initialized data */

    static char a[5*6] = "     " "     " "     " "     " "     " "     ";

    /* Format strings */
    static char fmt_1[] = "()";
    static char fmt_3[] = "(4a5)";

    /* System generated locals */
    integer i__1;

    /* Builtin functions */
    integer s_wsfe(cilist *), e_wsfe(void), s_rsfe(cilist *), do_fio(integer *
	    , char *, ftnlen), e_rsfe(void), s_cmp(char *, char *, ftnlen,
	    ftnlen);
    /* Subroutine */ int s_copy(char *, char *, ftnlen, ftnlen);

    /* Local variables */
    static integer i__, j, k, second;
    extern /* Subroutine */ int upcase_(char *, ftnlen);

    /* Fortran I/O blocks */
    static cilist io___208 = { 0, 6, 0, fmt_1, 0 };
    static cilist io___209 = { 0, 5, 0, fmt_3, 0 };


/*  GET A COMMAND FROM THE ADVENTURER.  SNARF OUT THE FIRST WORD, PAD IT WITH */
/*  BLANKS, AND RETURN IT IN WORD1.  CHARS 6 THRU 10 ARE RETURNED IN WORD1X, IN */
/*  CASE WE NEED TO PRINT OUT THE WHOLE WORD IN AN ERROR MESSAGE.  ANY NUMBER OF */
/*  BLANKS MAY FOLLOW THE WORD.  IF A SECOND WORD APPEARS, IT IS RETURNED IN */
/*  WORD2 (CHARS 6 THRU 10 IN WORD2X), ELSE WORD2 IS SET TO ZERO. */
    if (blkcom_1.blklin) {
	s_wsfe(&io___208);
	e_wsfe();
    }
L2:
    s_rsfe(&io___209);
    for (i__ = 1; i__ <= 4; ++i__) {
	do_fio(&c__1, a + (i__ - 1) * 5, (ftnlen)5);
    }
    e_rsfe();
    j = 0;
    for (i__ = 1; i__ <= 4; ++i__) {
	if (s_cmp(a + (i__ - 1) * 5, a + 25, (ftnlen)5, (ftnlen)5) != 0) {
	    j = 1;
	}
/* L9: */
	upcase_(a + (i__ - 1) * 5, (ftnlen)5);
    }
    if (blkcom_1.blklin && j == 0) {
	goto L2;
    }
    second = 0;
    s_copy(word1, a, (ftnlen)5, (ftnlen)5);
    s_copy(word1x, a + 5, (ftnlen)5, (ftnlen)5);
    s_copy(word2, a + 25, (ftnlen)5, (ftnlen)5);
/*  PORTING NOTE: THE ORIGINAL CODE HAD SOME VERY INTERESTING BITWISE */
/*  OPERATIONS USED TO PACK THE CHARACTERS INTO PDP-10 WORDS HERE. THESE */
/*  HAVE BEEN CONVERTED TO STRAIGHTFORWARD CHARACTER-BASED LOGIC IN AS */
/*  LITERAL A TRANSLATION AS POSSIBLE. NOTE HOWEVER THAT WE USE ALL */
/*  BLANKS AS AN INDICATOR OF AN EMPTY WORD2. */
    for (j = 1; j <= 4; ++j) {
	for (k = 1; k <= 5; ++k) {
	    if (*(unsigned char *)&a[(j - 1) * 5 + (k - 1)] == ' ') {
		goto L15;
	    }
	    if (second == 0) {
		goto L10;
	    }
	    s_copy(word2, a + ((j - 1) * 5 + (k - 1)), (ftnlen)5, 5 - (k - 1))
		    ;
	    s_copy(word2x, a + (j * 5 + (k - 1)), (ftnlen)5, 5 - (k - 1));
	    if (k != 1) {
		i__1 = 7 - k - 1;
		s_copy(word2 + i__1, a + j * 5, 5 - i__1, (ftnlen)5);
		i__1 = 7 - k - 1;
		s_copy(word2x + i__1, a + (j + 1) * 5, 5 - i__1, (ftnlen)5);
	    }
	    return 0;
L15:
	    if (second == 1) {
		goto L10;
	    }
	    second = 1;
	    if (j == 1) {
		s_copy(word1 + (k - 1), a + 25, 5 - (k - 1), (ftnlen)5);
	    }
L10:
	    ;
	}
    }
    return 0;
} /* getin_ */

logical yes_(integer *x, integer *y, integer *z__)
{
    /* System generated locals */
    logical ret_val;

    /* Local variables */
    extern logical yesx_(integer *, integer *, integer *, S_fp);
    extern /* Subroutine */ int rspeak_(integer *);

/*  CALL YESX (BELOW) WITH MESSAGES FROM SECTION 6. */
    ret_val = yesx_(x, y, z__, (S_fp)rspeak_);
    return ret_val;
} /* yes_ */

logical yesm_(integer *x, integer *y, integer *z__)
{
    /* System generated locals */
    logical ret_val;

    /* Local variables */
    extern logical yesx_(integer *, integer *, integer *, S_fp);
    extern /* Subroutine */ int mspeak_(integer *);

/*  CALL YESX (BELOW) WITH MESSAGES FROM SECTION 12. */
    ret_val = yesx_(x, y, z__, (S_fp)mspeak_);
    return ret_val;
} /* yesm_ */

logical yesx_(integer *x, integer *y, integer *z__, S_fp spk)
{
    /* Format strings */
    static char fmt_9[] = "(/\002 Please answer the question.\002)";

    /* System generated locals */
    logical ret_val;

    /* Builtin functions */
    integer s_cmp(char *, char *, ftnlen, ftnlen), s_wsfe(cilist *), e_wsfe(
	    void);

    /* Local variables */
    static char junk1[5], junk2[5], junk3[5];
    extern /* Subroutine */ int getin_(char *, char *, char *, char *, ftnlen,
	     ftnlen, ftnlen, ftnlen);
    static char reply[5];

    /* Fortran I/O blocks */
    static cilist io___218 = { 0, 6, 0, fmt_9, 0 };


/*  PRINT MESSAGE X, WAIT FOR YES/NO ANSWER.  IF YES, PRINT Y AND LEAVE YEA */
/*  TRUE; IF NO, PRINT Z AND LEAVE YEA FALSE.  SPK IS EITHER RSPEAK OR MSPEAK. */
L1:
    if (*x != 0) {
	(*spk)(x);
    }
    getin_(reply, junk1, junk2, junk3, (ftnlen)5, (ftnlen)5, (ftnlen)5, (
	    ftnlen)5);
    if (s_cmp(reply, "YES", (ftnlen)5, (ftnlen)3) == 0 || s_cmp(reply, "Y", (
	    ftnlen)5, (ftnlen)1) == 0) {
	goto L10;
    }
    if (s_cmp(reply, "NO", (ftnlen)5, (ftnlen)2) == 0 || s_cmp(reply, "N", (
	    ftnlen)5, (ftnlen)1) == 0) {
	goto L20;
    }
    s_wsfe(&io___218);
    e_wsfe();
    goto L1;
L10:
    ret_val = TRUE_;
    if (*y != 0) {
	(*spk)(y);
    }
    return ret_val;
L20:
    ret_val = FALSE_;
    if (*z__ != 0) {
	(*spk)(z__);
    }
    return ret_val;
} /* yesx_ */

/* Subroutine */ int a5toa1_(char *a, char *b, char *c__, char *chars,
	integer *leng, ftnlen a_len, ftnlen b_len, ftnlen c_len, ftnlen
	chars_len)
{
    /* Initialized data */

    static char blank[1] = " ";

    /* Builtin functions */
    /* Subroutine */ int s_copy(char *, char *, ftnlen, ftnlen);

    /* Local variables */
    static integer ch, word, posn;
    static char words[5*3];

/*  A AND B CONTAIN A 1- TO 9-CHARACTER WORD IN A5 FORMAT, C CONTAINS ANOTHER */
/*  WORD AND/OR PUNCTUATION.  THEY ARE UNPACKED TO ONE CHARACTER PER WORD IN THE */
/*  ARRAY "CHARS", WITH EXACTLY ONE BLANK BETWEEN B AND C (OR NONE, IF C >= 0). */
/*  THE INDEX OF THE LAST NON-BLANK CHAR IN CHARS IS RETURNED IN LENG. */
    /* Parameter adjustments */
    --chars;

    /* Function Body */
    s_copy(words, a, (ftnlen)5, (ftnlen)5);
    s_copy(words + 5, b, (ftnlen)5, (ftnlen)5);
    s_copy(words + 10, c__, (ftnlen)5, c_len);
    posn = 1;
    for (word = 1; word <= 3; ++word) {
	if (word == 2 && posn != 6) {
	    goto L1;
	}
	if (word == 3 && *(unsigned char *)c__ >= '@') {
	    ++posn;
	}
	for (ch = 1; ch <= 5; ++ch) {
	    *(unsigned char *)&chars[posn] = *(unsigned char *)&words[(word -
		    1) * 5 + (ch - 1)];
	    if (*(unsigned char *)&chars[posn] == *(unsigned char *)&blank[0])
		     {
		goto L1;
	    }
	    *leng = posn;
/* L2: */
	    ++posn;
	}
L1:
	;
    }
    return 0;
} /* a5toa1_ */

/*  DATA STRUCTURE ROUTINES (VOCAB, DSTROY, JUGGLE, MOVE, PUT, CARRY, DROP) */
integer vocab_(char *id, integer *init, ftnlen id_len)
{
    /* System generated locals */
    integer ret_val, i__1;
    char ch__1[5];

    /* Builtin functions */
    /* Subroutine */ int s_copy(char *, char *, ftnlen, ftnlen);
    integer s_cmp(char *, char *, ftnlen, ftnlen);

    /* Local variables */
    static integer i__;
    static char id2[5];
    extern /* Subroutine */ int bug_(integer *);
    static char hash[5];
    extern /* Character */ VOID phrog_(char *, ftnlen, char *, ftnlen);

/*  LOOK UP ID IN THE VOCABULARY (ATAB) AND RETURN ITS "DEFINITION" (KTAB), OR */
/*  -1 IF NOT FOUND.  IF INIT IS POSITIVE, THIS IS AN INITIALISATION CALL SETTING */
/*  UP A KEYWORD VARIABLE, AND NOT FINDING IT CONSTITUTES A BUG.  IT ALSO MEANS */
/*  THAT ONLY KTAB VALUES WHICH TAKEN OVER 1000 EQUAL INIT MAY BE CONSIDERED. */
/*  (THUS "STEPS", WHICH IS A MOTION VERB AS WELL AS AN OBJECT, MAY BE LOCATED */
/*  AS AN OBJECT.)  AND IT ALSO MEANS THE KTAB VALUE IS TAKEN MOD 1000. */
    s_copy(id2, id, (ftnlen)5, id_len);
    phrog_(ch__1, (ftnlen)5, id2, (ftnlen)5);
    s_copy(hash, ch__1, (ftnlen)5, (ftnlen)5);
    i__1 = voccom_1.tabsiz;
    for (i__ = 1; i__ <= i__1; ++i__) {
	if (voccom_1.ktab[i__ - 1] == -1) {
	    goto L2;
	}
	if (*init >= 0 && voccom_1.ktab[i__ - 1] / 1000 != *init) {
	    goto L1;
	}
	if (s_cmp(voccom_1.atab + (i__ - 1) * 5, hash, (ftnlen)5, (ftnlen)5)
		== 0) {
	    goto L3;
	}
L1:
	;
    }
    bug_(&c__21);
L2:
    ret_val = -1;
    if (*init < 0) {
	return ret_val;
    }
    bug_(&c__5);
L3:
    ret_val = voccom_1.ktab[i__ - 1];
    if (*init >= 0) {
	ret_val %= 1000;
    }
    return ret_val;
} /* vocab_ */

/* Subroutine */ int dstroy_(integer *object)
{
    extern /* Subroutine */ int move_(integer *, integer *);

/*  PERMANENTLY ELIMINATE "OBJECT" BY MOVING TO A NON-EXISTENT LOCATION. */
    move_(object, &c__0);
    return 0;
} /* dstroy_ */

/* Subroutine */ int juggle_(integer *object)
{
    /* System generated locals */
    integer i__1;

    /* Local variables */
    static integer i__, j;
    extern /* Subroutine */ int move_(integer *, integer *);

/*  JUGGLE AN OBJECT BY PICKING IT UP AND PUTTING IT DOWN AGAIN, THE PURPOSE */
/*  BEING TO GET THE OBJECT TO THE FRONT OF THE CHAIN OF THINGS AT ITS LOC. */
    i__ = placom_1.place[*object - 1];
    j = placom_1.fixed[*object - 1];
    move_(object, &i__);
    i__1 = *object + 100;
    move_(&i__1, &j);
    return 0;
} /* juggle_ */

/* Subroutine */ int move_(integer *object, integer *where)
{
    static integer from;
    extern /* Subroutine */ int drop_(integer *, integer *), carry_(integer *,
	     integer *);

/*  PLACE ANY OBJECT ANYWHERE BY PICKING IT UP AND DROPPING IT.  MAY ALREADY BE */
/*  TOTING, IN WHICH CASE THE CARRY IS A NO-OP.  MUSTN'T PICK UP OBJECTS WHICH */
/*  ARE NOT AT ANY LOC, SINCE CARRY WANTS TO REMOVE OBJECTS FROM ATLOC CHAINS. */
    if (*object > 100) {
	goto L1;
    }
    from = placom_1.place[*object - 1];
    goto L2;
L1:
    from = placom_1.fixed[*object - 101];
L2:
    if (from > 0 && from <= 300) {
	carry_(object, &from);
    }
    drop_(object, where);
    return 0;
} /* move_ */

integer put_(integer *object, integer *where, integer *pval)
{
    /* System generated locals */
    integer ret_val;

    /* Local variables */
    extern /* Subroutine */ int move_(integer *, integer *);

/*  PUT IS THE SAME AS MOVE, EXCEPT IT RETURNS A VALUE USED TO SET UP THE */
/*  NEGATED PROP VALUES FOR THE REPOSITORY OBJECTS. */
    move_(object, where);
    ret_val = -1 - *pval;
    return ret_val;
} /* put_ */

/* Subroutine */ int carry_(integer *object, integer *where)
{
    static integer temp;

/*  START TOTING AN OBJECT, REMOVING IT FROM THE LIST OF THINGS AT ITS FORMER */
/*  LOCATION.  INCR HOLDNG UNLESS IT WAS ALREADY BEING TOTED.  IF OBJECT>100 */
/*  (MOVING "FIXED" SECOND LOC), DON'T CHANGE PLACE OR HOLDNG. */
    if (*object > 100) {
	goto L5;
    }
    if (placom_1.place[*object - 1] == -1) {
	return 0;
    }
    placom_1.place[*object - 1] = -1;
    ++placom_1.holdng;
L5:
    if (placom_1.atloc[*where - 1] != *object) {
	goto L6;
    }
    placom_1.atloc[*where - 1] = placom_1.link[*object - 1];
    return 0;
L6:
    temp = placom_1.atloc[*where - 1];
L7:
    if (placom_1.link[temp - 1] == *object) {
	goto L8;
    }
    temp = placom_1.link[temp - 1];
    goto L7;
L8:
    placom_1.link[temp - 1] = placom_1.link[*object - 1];
    return 0;
} /* carry_ */

/* Subroutine */ int drop_(integer *object, integer *where)
{
/*  PLACE AN OBJECT AT A GIVEN LOC, PREFIXING IT ONTO THE ATLOC LIST.  DECR */
/*  HOLDNG IF THE OBJECT WAS BEING TOTED. */
    if (*object > 100) {
	goto L1;
    }
    if (placom_1.place[*object - 1] == -1) {
	--placom_1.holdng;
    }
    placom_1.place[*object - 1] = *where;
    goto L2;
L1:
    placom_1.fixed[*object - 101] = *where;
L2:
    if (*where <= 0) {
	return 0;
    }
    placom_1.link[*object - 1] = placom_1.atloc[*where - 1];
    placom_1.atloc[*where - 1] = *object;
    return 0;
} /* drop_ */

/*  WIZARDRY ROUTINES (START, MAINT, WIZARD, HOURS(X), NEWHRS(X), MOTD, POOF) */
logical start_(integer *dummy)
{
    /* System generated locals */
    logical ret_val;

/*  Patched: disable time/wizard gating; always run full game, not demo. */
    ret_val = FALSE_;
    return ret_val;
} /* start_ */

/* Subroutine */ int maint_(logical *needsv)
{
    /* Format strings */
    static char fmt_2[] = "(4a5)";
    static char fmt_12[] = "(\002 Length of short game (null to leave at\002"
	    ",i3,\002):\002)";
    static char fmt_16[] = "(\002 Latency for restart (null to leave at\002,"
	    "i3,\002):\002)";

    /* Builtin functions */
    integer s_rsle(cilist *), do_lio(integer *, integer *, char *, ftnlen),
	    e_rsle(void), s_rsfe(cilist *), do_fio(integer *, char *, ftnlen),
	     e_rsfe(void), s_wsfe(cilist *), e_wsfe(void), s_cmp(char *, char
	    *, ftnlen, ftnlen);
    /* Subroutine */ int s_copy(char *, char *, ftnlen, ftnlen);

    /* Local variables */
    static integer d__, t, x;
    static char wd[5], scr[5];
    extern /* Subroutine */ int motd_(logical *);
    extern logical yesm_(integer *, integer *, integer *);
    extern /* Subroutine */ int getin_(char *, char *, char *, char *, ftnlen,
	     ftnlen, ftnlen, ftnlen), hours_(void), datime_(integer *,
	    integer *), mspeak_(integer *);
    extern logical wizard_(integer *);
    extern /* Subroutine */ int newhrs_(void);

    /* Fortran I/O blocks */
    static cilist io___231 = { 0, 5, 0, 0, 0 };
    static cilist io___232 = { 0, 5, 0, 0, 0 };
    static cilist io___235 = { 0, 5, 0, fmt_2, 0 };
    static cilist io___236 = { 0, 6, 0, fmt_12, 0 };
    static cilist io___237 = { 0, 5, 0, "(I8)", 0 };
    static cilist io___241 = { 0, 5, 0, "(I8)", 0 };
    static cilist io___242 = { 0, 6, 0, fmt_16, 0 };
    static cilist io___243 = { 0, 5, 0, "(I8)", 0 };


/*  SOMEONE SAID THE MAGIC WORD TO INVOKE MAINTENANCE MODE.  MAKE SURE HE'S A */
/*  WIZARD.  IF SO, LET HIM TWEAK ALL SORTS OF RANDOM THINGS, THEN EXIT SO CAN */
/*  SAVE TWEAKED VERSION.  SINCE MAGIC WORD MUST BE FIRST COMMAND GIVEN, ONLY */
/*  THING WHICH NEEDS TO BE FIXED UP IS ABB(1). */
    *needsv = FALSE_;
    if (! wizard_(&c__0)) {
	return 0;
    }
    blkcom_1.blklin = FALSE_;
    if (yesm_(&c__10, &c__0, &c__0)) {
	hours_();
    }
    if (yesm_(&c__11, &c__0, &c__0)) {
	newhrs_();
    }
    if (! yesm_(&c__26, &c__0, &c__0)) {
	goto L10;
    }
    mspeak_(&c__27);
    s_rsle(&io___231);
    do_lio(&c__3, &c__1, (char *)&wizcom_1.hbegin, (ftnlen)sizeof(integer));
    e_rsle();
    mspeak_(&c__28);
    s_rsle(&io___232);
    do_lio(&c__3, &c__1, (char *)&wizcom_1.hend, (ftnlen)sizeof(integer));
    e_rsle();
    datime_(&d__, &t);
    wizcom_1.hbegin += d__;
    wizcom_1.hend = wizcom_1.hbegin + wizcom_1.hend - 1;
    mspeak_(&c__29);
    s_rsfe(&io___235);
    do_fio(&c__4, wizcom_1.hname, (ftnlen)5);
    e_rsfe();
L10:
    s_wsfe(&io___236);
    do_fio(&c__1, (char *)&wizcom_1.short__, (ftnlen)sizeof(integer));
    e_wsfe();
    s_rsfe(&io___237);
    do_fio(&c__1, (char *)&x, (ftnlen)sizeof(integer));
    e_rsfe();
    if (x > 0) {
	wizcom_1.short__ = x;
    }
    mspeak_(&c__12);
    getin_(wd, scr, scr, scr, (ftnlen)5, (ftnlen)5, (ftnlen)5, (ftnlen)5);
    if (s_cmp(wd, " ", (ftnlen)5, (ftnlen)1) != 0) {
	s_copy(wizcom_1.magic, wd, (ftnlen)5, (ftnlen)5);
    }
    mspeak_(&c__13);
    s_rsfe(&io___241);
    do_fio(&c__1, (char *)&x, (ftnlen)sizeof(integer));
    e_rsfe();
    if (x > 0) {
	wizcom_1.magnm = x;
    }
    s_wsfe(&io___242);
    do_fio(&c__1, (char *)&wizcom_1.latncy, (ftnlen)sizeof(integer));
    e_wsfe();
    s_rsfe(&io___243);
    do_fio(&c__1, (char *)&x, (ftnlen)sizeof(integer));
    e_rsfe();
    if (x > 0 && x < 45) {
	mspeak_(&c__30);
    }
    if (x > 0) {
	wizcom_1.latncy = max(45,x);
    }
    if (yesm_(&c__14, &c__0, &c__0)) {
	motd_(&c_true);
    }
    wizcom_1.saved = 0;
    wizcom_1.setup = 2;
    abbcom_1.abb[0] = 0;
/*     CALL MSPEAK(15) */
    blkcom_1.blklin = TRUE_;
    *needsv = TRUE_;
    return 0;
} /* maint_ */

logical wizard_(integer *dummy)
{
    /* Format strings */
    static char fmt_18[] = "(/1x,a5)";

    /* System generated locals */
    integer i__1;
    logical ret_val;

    /* Builtin functions */
    integer s_cmp(char *, char *, ftnlen, ftnlen);
    /* Subroutine */ int s_copy(char *, char *, ftnlen, ftnlen);
    integer s_wsfe(cilist *), do_fio(integer *, char *, ftnlen), e_wsfe(void);

    /* Local variables */
    static integer d__, t, x, y, z__, val[5];
    static char scr1[5], scr2[5], scr3[5], word[5];
    extern logical yesm_(integer *, integer *, integer *);
    extern /* Subroutine */ int getin_(char *, char *, char *, char *, ftnlen,
	     ftnlen, ftnlen, ftnlen), datime_(integer *, integer *), mspeak_(
	    integer *);

    /* Fortran I/O blocks */
    static cilist io___254 = { 0, 6, 0, fmt_18, 0 };


/*  ASK IF HE'S A WIZARD.  IF HE SAYS YES, MAKE HIM PROVE IT.  RETURN TRUE IF HE */
/*  REALLY IS A WIZARD. */
    ret_val = yesm_(&c__16, &c__0, &c__7);
    if (! ret_val) {
	return ret_val;
    }
/*  HE SAYS HE IS.  FIRST STEP: DOES HE KNOW ANYTHING MAGICAL? */
    mspeak_(&c__17);
    getin_(word, scr1, scr2, scr3, (ftnlen)5, (ftnlen)5, (ftnlen)5, (ftnlen)5)
	    ;
    if (s_cmp(word, wizcom_1.magic, (ftnlen)5, (ftnlen)5) != 0) {
	goto L99;
    }
/*  HE DOES.  GIVE HIM A RANDOM CHALLENGE AND CHECK HIS REPLY. */
    datime_(&d__, &t);
    t = (t << 1) + 1;
    s_copy(word, "@@@@@", (ftnlen)5, (ftnlen)5);
    for (y = 1; y <= 5; ++y) {
	x = d__ % 5 + 79;
	d__ /= 5;
	i__1 = x;
	for (z__ = 1; z__ <= i__1; ++z__) {
/* L12: */
	    t = t * 1027 % 1048576;
	}
	val[y - 1] = t * 26 / 1048576 + 1;
/* L15: */
	*(unsigned char *)&word[y - 1] = (char) (*(unsigned char *)&word[y -
		1] + val[y - 1]);
    }
    if (yesm_(&c__18, &c__0, &c__0)) {
	goto L99;
    }
    s_wsfe(&io___254);
    do_fio(&c__1, word, (ftnlen)5);
    e_wsfe();
    getin_(word, scr1, scr2, scr3, (ftnlen)5, (ftnlen)5, (ftnlen)5, (ftnlen)5)
	    ;
    datime_(&d__, &t);
    t = t / 60 * 40 + t / 10 * 10;
    d__ = wizcom_1.magnm;
    for (y = 1; y <= 5; ++y) {
	z__ = y % 5 + 1;
	x = ((i__1 = val[y - 1] - val[z__ - 1], abs(i__1)) * (d__ % 10) + t %
		10) % 26 + 1;
	t /= 10;
	d__ /= 10;
/* L19: */
	*(unsigned char *)&word[y - 1] = (char) (*(unsigned char *)&word[y -
		1] - x);
    }
    if (s_cmp(word, "@@@@@", (ftnlen)5, (ftnlen)5) != 0) {
	goto L99;
    }
/*  BY GEORGE, HE REALLY *IS* A WIZARD! */
    mspeak_(&c__19);
    return ret_val;
/*  AHA!  AN IMPOSTOR! */
L99:
    mspeak_(&c__20);
    ret_val = FALSE_;
    return ret_val;
} /* wizard_ */

/* Subroutine */ int hours_(void)
{
    /* Format strings */
    static char fmt_1[] = "()";
    static char fmt_5[] = "(/\002 Today is a holiday, namely \002,4a5)";
    static char fmt_15[] = "(/\002 The next holiday will be in\002,i3,\002"
	    " \002,a5,\002 namely \002,4a5)";

    /* Builtin functions */
    integer s_wsfe(cilist *), e_wsfe(void), do_fio(integer *, char *, ftnlen);
    /* Subroutine */ int s_copy(char *, char *, ftnlen, ftnlen);

    /* Local variables */
    static integer d__, t;
    static char txt[5];
    extern /* Subroutine */ int datime_(integer *, integer *), hoursx_(
	    integer *, char *, char *, ftnlen, ftnlen);

    /* Fortran I/O blocks */
    static cilist io___255 = { 0, 6, 0, fmt_1, 0 };
    static cilist io___258 = { 0, 6, 0, fmt_5, 0 };
    static cilist io___260 = { 0, 6, 0, fmt_15, 0 };


/*  ANNOUNCE THE CURRENT HOURS WHEN THE CAVE IS OPEN FOR ADVENTURING.  THIS INFO */
/*  IS STORED IN WKDAY, WKEND, AND HOLID, WHERE BIT SHIFT(1,N) IS ON IFF THE */
/*  HOUR FROM N:00 TO N:59 IS "PRIME TIME" (CAVE CLOSED).  WKDAY IS FOR */
/*  WEEKDAYS, WKEND FOR WEEKENDS, HOLID FOR HOLIDAYS.  NEXT HOLIDAY IS FROM */
/*  HBEGIN TO HEND. */
    s_wsfe(&io___255);
    e_wsfe();
    hoursx_(&wizcom_1.wkday, "Mon -", " Fri:", (ftnlen)5, (ftnlen)5);
    hoursx_(&wizcom_1.wkend, "Sat -", " Sun:", (ftnlen)5, (ftnlen)5);
    hoursx_(&wizcom_1.holid, "Holid", "ays: ", (ftnlen)5, (ftnlen)5);
    datime_(&d__, &t);
    if (wizcom_1.hend < d__ || wizcom_1.hend < wizcom_1.hbegin) {
	return 0;
    }
    if (wizcom_1.hbegin > d__) {
	goto L10;
    }
    s_wsfe(&io___258);
    do_fio(&c__4, wizcom_1.hname, (ftnlen)5);
    e_wsfe();
    return 0;
L10:
    d__ = wizcom_1.hbegin - d__;
    s_copy(txt, "Days,", (ftnlen)5, (ftnlen)5);
    if (d__ == 1) {
	s_copy(txt, "Day, ", (ftnlen)5, (ftnlen)5);
    }
    s_wsfe(&io___260);
    do_fio(&c__1, (char *)&d__, (ftnlen)sizeof(integer));
    do_fio(&c__1, txt, (ftnlen)5);
    do_fio(&c__4, wizcom_1.hname, (ftnlen)5);
    e_wsfe();
    return 0;
} /* hours_ */

/* Subroutine */ int hoursx_(integer *h__, char *day1, char *day2, ftnlen
	day1_len, ftnlen day2_len)
{
    /* Format strings */
    static char fmt_2[] = "(10x,2a5,\002  Open all day\002)";
    static char fmt_16[] = "(10x,2a5,i4,\002:00 to\002,i3,\002:00\002)";
    static char fmt_18[] = "(20x,i4,\002:00 to\002,i3,\002:00\002)";
    static char fmt_22[] = "(10x,2a5,\002  Closed all day\002)";

    /* Builtin functions */
    integer s_wsfe(cilist *), do_fio(integer *, char *, ftnlen), e_wsfe(void),
	     pow_ii(integer *, integer *);

    /* Local variables */
    static integer from, till;
    static logical first;

    /* Fortran I/O blocks */
    static cilist io___263 = { 0, 6, 0, fmt_2, 0 };
    static cilist io___265 = { 0, 6, 0, fmt_16, 0 };
    static cilist io___266 = { 0, 6, 0, fmt_18, 0 };
    static cilist io___267 = { 0, 6, 0, fmt_22, 0 };


/*  USED BY HOURS (ABOVE) TO PRINT HOURS FOR EITHER WEEKDAYS OR WEEKENDS. */
    first = TRUE_;
    from = -1;
    if (*h__ != 0) {
	goto L10;
    }
    s_wsfe(&io___263);
    do_fio(&c__1, day1, (ftnlen)5);
    do_fio(&c__1, day2, (ftnlen)5);
    e_wsfe();
    return 0;
L10:
    ++from;
    if (*h__ / pow_ii(&c__2, &from) % 2 != 0) {
	goto L10;
    }
    if (from >= 24) {
	goto L20;
    }
    till = from;
L14:
    ++till;
    if (! (*h__ / pow_ii(&c__2, &till) % 2 != 0) && till != 24) {
	goto L14;
    }
    if (first) {
	s_wsfe(&io___265);
	do_fio(&c__1, day1, (ftnlen)5);
	do_fio(&c__1, day2, (ftnlen)5);
	do_fio(&c__1, (char *)&from, (ftnlen)sizeof(integer));
	do_fio(&c__1, (char *)&till, (ftnlen)sizeof(integer));
	e_wsfe();
    }
    if (! first) {
	s_wsfe(&io___266);
	do_fio(&c__1, (char *)&from, (ftnlen)sizeof(integer));
	do_fio(&c__1, (char *)&till, (ftnlen)sizeof(integer));
	e_wsfe();
    }
    first = FALSE_;
    from = till;
    goto L10;
L20:
    if (first) {
	s_wsfe(&io___267);
	do_fio(&c__1, day1, (ftnlen)5);
	do_fio(&c__1, day2, (ftnlen)5);
	e_wsfe();
    }
    return 0;
} /* hoursx_ */

/* Subroutine */ int newhrs_(void)
{
    extern /* Subroutine */ int hours_(void), mspeak_(integer *);
    extern integer newhrx_(char *, char *, ftnlen, ftnlen);

/*  SET UP NEW HOURS FOR THE CAVE.  SPECIFIED AS INVERSE--I.E., WHEN IS IT */
/*  CLOSED DUE TO PRIME TIME?  SEE HOURS (ABOVE) FOR DESC OF VARIABLES. */
    mspeak_(&c__21);
    wizcom_1.wkday = newhrx_("Weekd", "ays:", (ftnlen)5, (ftnlen)4);
    wizcom_1.wkend = newhrx_("Weeke", "nds:", (ftnlen)5, (ftnlen)4);
    wizcom_1.holid = newhrx_("Holid", "ays:", (ftnlen)5, (ftnlen)4);
    mspeak_(&c__22);
    hours_();
    return 0;
} /* newhrs_ */

integer newhrx_(char *day1, char *day2, ftnlen day1_len, ftnlen day2_len)
{
    /* Format strings */
    static char fmt_1[] = "(\002 Prime time on \002,2a5)";
    static char fmt_2[] = "(\002 from:\002)";
    static char fmt_4[] = "(\002 till:\002)";

    /* System generated locals */
    integer ret_val, i__1;

    /* Builtin functions */
    /* Subroutine */ int s_copy(char *, char *, ftnlen, ftnlen);
    integer s_wsfe(cilist *), do_fio(integer *, char *, ftnlen), e_wsfe(void),
	     s_rsle(cilist *), do_lio(integer *, integer *, char *, ftnlen),
	    e_rsle(void);

    /* Local variables */
    static integer i__;
    static char dy2[5];
    static integer from, till, idondx;
    extern /* Subroutine */ int setbit_(integer *, integer *);

    /* Fortran I/O blocks */
    static cilist io___269 = { 0, 6, 0, fmt_1, 0 };
    static cilist io___270 = { 0, 6, 0, fmt_2, 0 };
    static cilist io___271 = { 0, 5, 0, 0, 0 };
    static cilist io___273 = { 0, 6, 0, fmt_4, 0 };
    static cilist io___274 = { 0, 5, 0, 0, 0 };


/*  INPUT PRIME TIME SPECS AND SET UP A WORD OF INTERNAL FORMAT. */
    s_copy(dy2, day2, (ftnlen)5, day2_len);
    ret_val = 0;
    s_wsfe(&io___269);
    do_fio(&c__1, day1, (ftnlen)5);
    do_fio(&c__1, dy2, (ftnlen)5);
    e_wsfe();
L10:
    s_wsfe(&io___270);
    e_wsfe();
    s_rsle(&io___271);
    do_lio(&c__3, &c__1, (char *)&from, (ftnlen)sizeof(integer));
    e_rsle();
    if (from < 0 || from >= 24) {
	return ret_val;
    }
    s_wsfe(&io___273);
    e_wsfe();
    s_rsle(&io___274);
    do_lio(&c__3, &c__1, (char *)&till, (ftnlen)sizeof(integer));
    e_rsle();
    --till;
    if (till < from || till >= 24) {
	return ret_val;
    }
    i__1 = till;
    for (i__ = from; i__ <= i__1; ++i__) {
	idondx = i__;
/* L5: */
	setbit_(&ret_val, &idondx);
    }
    goto L10;
} /* newhrx_ */

/* Subroutine */ int motd_(logical *alter)
{
    /* Format strings */
    static char fmt_20[] = "(\002 \002,15a5)";
    static char fmt_56[] = "(16a5)";

    /* System generated locals */
    integer i__1;

    /* Builtin functions */
    integer s_wsfe(cilist *), do_fio(integer *, char *, ftnlen), e_wsfe(void),
	     s_rsfe(cilist *), e_rsfe(void), s_cmp(char *, char *, ftnlen,
	    ftnlen);

    /* Local variables */
    static integer i__;
    static char k[1];
    static integer m, ln;
    extern /* Subroutine */ int mspeak_(integer *);

    /* Fortran I/O blocks */
    static cilist io___279 = { 0, 6, 0, fmt_20, 0 };
    static cilist io___282 = { 0, 5, 0, fmt_56, 0 };


/*  HANDLES MESSAGE OF THE DAY.  IF ALTER IS TRUE, READ A NEW MESSAGE FROM THE */
/*  WIZARD.  ELSE PRINT THE CURRENT ONE.  MESSAGE IS INITIALLY NULL. */
/*  PORTING NOTE: MSG HAS BEEN CHANGED IN A SIMILAR WAY TO LINES. */
/*  SEE THE COMMENTS AT THE TOP OF THIS FILE. ALSO, WE'VE MADE THE STATE */
/*  HERE COMMON TO THE MAIN PROGRAM TO FACILITATE SAVING IT. */
    ln = 1;
    if (*alter) {
	goto L50;
    }
L10:
    if (motcom_1.msglin[ln] < 0) {
	return 0;
    }
    s_wsfe(&io___279);
    i__1 = motcom_1.msglin[ln] - 1;
    for (i__ = motcom_1.msglin[ln - 1]; i__ <= i__1; ++i__) {
	do_fio(&c__1, motcom_1.msg + (i__ - 1) * 5, (ftnlen)5);
    }
    e_wsfe();
    ++ln;
    goto L10;
L50:
    m = 1;
    motcom_1.msglin[0] = 1;
    mspeak_(&c__23);
L55:
    s_rsfe(&io___282);
    i__1 = m + 14;
    for (i__ = m; i__ <= i__1; ++i__) {
	do_fio(&c__1, motcom_1.msg + (i__ - 1) * 5, (ftnlen)5);
    }
    do_fio(&c__1, k, (ftnlen)1);
    e_rsfe();
    if (*(unsigned char *)k == ' ') {
	goto L60;
    }
    mspeak_(&c__24);
    goto L55;
L60:
    for (i__ = 14; i__ >= 0; --i__) {
	if (s_cmp(motcom_1.msg + (m + i__ - 1) * 5, " ", (ftnlen)5, (ftnlen)1)
		 != 0) {
	    goto L65;
	}
/* L62: */
    }
    goto L90;
L65:
    motcom_1.msglin[ln] = m + i__ + 1;
    m = m + i__ + 1;
    ++ln;
    if (m + 14 < 100 && ln < 49) {
	goto L55;
    }
    mspeak_(&c__25);
L90:
    motcom_1.msglin[ln] = -1;
    return 0;
} /* motd_ */

/* Subroutine */ int poof_(void)
{
    /* Builtin functions */
    /* Subroutine */ int s_copy(char *, char *, ftnlen, ftnlen);

/*  AS PART OF DATABASE INITIALISATION, WE CALL POOF TO SET UP SOME DUMMY */
/*  PRIME-TIME SPECS, MAGIC WORDS, ETC. */
/*  WKDAY="00777400 */
    wizcom_1.wkday = 261888;
    wizcom_1.wkend = 0;
    wizcom_1.holid = 0;
    wizcom_1.hbegin = 0;
    wizcom_1.hend = -1;
    wizcom_1.short__ = 30;
    s_copy(wizcom_1.magic, "DWARF", (ftnlen)5, (ftnlen)5);
    wizcom_1.magnm = 11111;
    wizcom_1.latncy = 90;
    return 0;
} /* poof_ */

/*  UTILITY ROUTINES (SHIFT, RAN, DATIME, CIAO, BUG) */
integer shift_(integer *val, integer *dist)
{
    /* System generated locals */
    integer ret_val, i__1;

    /* Builtin functions */
    integer pow_ii(integer *, integer *);

/*  RETURN VAL LEFT-SHIFTED (LOGICALLY) DIST BITS (RIGHT-SHIFT IF DIST<0). */
/*  PORTING NOTE: THIS WAS ORIGINALLY A ROTATE OPERATION, BUT IT TURNED */
/*  OUT THAT A SHIFT WAS SUFFICIENT IN ALL CASES WHERE THIS WAS USED IN */
/*  THE CODE. IT HAD TO BE CHANGED BECAUSE THE ORIGINAL USED NON- */
/*  STANDARD BITWISE OPERATORS. NOTE ALSO THAT THE LOGIC IN THIS */
/*  FUNCTION WILL NOT WORK IF THE GIVEN VALUE IS NEGATIVE (HIGHEST BIT */
/*  SET). IN PRACTICE THIS DOES NOT AFFECT USES OF THE FUNCTION. */
    ret_val = *val;
    if (*dist < 0) {
	goto L10;
    } else if (*dist == 0) {
	goto L20;
    } else {
	goto L30;
    }
L10:
    i__1 = -(*dist);
    ret_val = *val / pow_ii(&c__2, &i__1);
L20:
    return ret_val;
L30:
    ret_val = *val * pow_ii(&c__2, dist);
    return ret_val;
} /* shift_ */

integer ran_(integer *range)
{
    /* Initialized data */

    static integer r__ = 0;

    /* System generated locals */
    integer ret_val, i__1;

    /* Local variables */
    static integer d__, t;
    extern /* Subroutine */ int datime_(integer *, integer *);

/*  SINCE THE RAN FUNCTION IN LIB40 SEEMS TO BE A REAL LOSE, WE'LL USE ONE OF */
/*  OUR OWN.  IT'S BEEN RUN THROUGH MANY OF THE TESTS IN KNUTH VOL. 2 AND */
/*  SEEMS TO BE QUITE RELIABLE.  RAN RETURNS A VALUE UNIFORMLY SELECTED */
/*  BETWEEN 0 AND RANGE-1.  NOTE RESEMBLANCE TO ALG USED IN WIZARD. */
    d__ = 1;
    if (r__ != 0) {
	goto L1;
    }
    datime_(&d__, &t);
    r__ = t * 18 + 5;
    d__ = d__ % 1000 + 1000;
L1:
    i__1 = d__;
    for (t = 1; t <= i__1; ++t) {
/* L2: */
	r__ = r__ * 1021 % 1048576;
    }
    ret_val = *range * r__ / 1048576;
    return ret_val;
} /* ran_ */

/* Subroutine */ int datime_(integer *d__, integer *t)
{
    /* System generated locals */
    integer i__1;

    /* Local variables */
    static integer y, tar[9], leap;
    extern integer time_(void);
    extern /* Subroutine */ int ltime_(integer *, integer *);

/*  RETURN THE DATE AND TIME IN D AND T.  D IS NUMBER OF DAYS SINCE 01-JAN-77, */
/*  T IS MINUTES PAST MIDNIGHT.  THIS IS HARDER THAN IT SOUNDS, BECAUSE THE */
/*  FINAGLED DEC FUNCTIONS RETURN THE VALUES ONLY AS ASCII STRINGS! */
/*  PORTING NOTE: THE CODE HERE IS A COMPLETE REWRITE OF THE ORIGINAL. */
/*  THERE IS NO PORTABLE WAY TO DO THIS, BUT THIS WAY WORKS FOR G77 AND */
/*  WILL PROBABLY WORK ON OTHER UNIX FORTRAN IMPLEMENTATIONS. */
    i__1 = time_();
    ltime_(&i__1, tar);
    y = tar[5] - 77;
    leap = y / 4;
    *d__ = y * 365 + leap + tar[7];
    *t = tar[2] * 60 + tar[1];
    return 0;
} /* datime_ */

/* Subroutine */ int ciao_(void)
{
    /* Initialized data */

    static integer k = 32;

    /* Builtin functions */
    /* Subroutine */ int s_stop(char *, ftnlen);

    /* Local variables */
    static char a[5], b[5], c__[5], d__[5];
    extern /* Subroutine */ int getin_(char *, char *, char *, char *, ftnlen,
	     ftnlen, ftnlen, ftnlen), mspeak_(integer *);

/*  EXITS, AFTER ISSUING REMINDER TO SAVE NEW CORE IMAGE.  USED WHEN SUSPENDING */
/*  AND WHEN CREATING NEW VERSION VIA MAGIC MODE.  ON SOME SYSTEMS, THE CORE */
/*  IMAGE IS LOST ONCE THE PROGRAM EXITS.  IF SO, SET K=31 INSTEAD OF 32. */
    mspeak_(&k);
    if (k == 31) {
	getin_(a, b, c__, d__, (ftnlen)5, (ftnlen)5, (ftnlen)5, (ftnlen)5);
    }
    s_stop("", (ftnlen)0);
    return 0;
} /* ciao_ */

/* Subroutine */ int bug_(integer *num)
{
    /* Format strings */
    static char fmt_1[] = "(\002 Fatal error, see source code for interpreta"
	    "tion.\002/\002 Probable cause: erroneous info in database.\002"
	    "/\002 Error code =\002,i2/)";

    /* Builtin functions */
    integer s_wsfe(cilist *), do_fio(integer *, char *, ftnlen), e_wsfe(void);
    /* Subroutine */ int s_stop(char *, ftnlen);

    /* Fortran I/O blocks */
    static cilist io___295 = { 0, 6, 0, fmt_1, 0 };


/*  THE FOLLOWING CONDITIONS ARE CURRENTLY CONSIDERED FATAL BUGS.  NUMBERS < 20 */
/*  ARE DETECTED WHILE READING THE DATABASE; THE OTHERS OCCUR AT "RUN TIME". */
/*     0  MESSAGE LINE > 70 CHARACTERS */
/*     1  NULL LINE IN MESSAGE */
/*     2  TOO MANY WORDS OF MESSAGES */
/*     3  TOO MANY TRAVEL OPTIONS */
/*     4  TOO MANY VOCABULARY WORDS */
/*     5  REQUIRED VOCABULARY WORD NOT FOUND */
/*     6  TOO MANY RTEXT OR MTEXT MESSAGES */
/*     7  TOO MANY HINTS */
/*     8  LOCATION HAS COND BIT BEING SET TWICE */
/*     9  INVALID SECTION NUMBER IN DATABASE */
/*     20 SPECIAL TRAVEL (500>L>300) EXCEEDS GOTO LIST */
/*     21 RAN OFF END OF VOCABULARY TABLE */
/*     22 VOCABULARY TYPE (N/1000) NOT BETWEEN 0 AND 3 */
/*     23 INTRANSITIVE ACTION VERB EXCEEDS GOTO LIST */
/*     24 TRANSITIVE ACTION VERB EXCEEDS GOTO LIST */
/*     25 CONDITIONAL TRAVEL ENTRY WITH NO ALTERNATIVE */
/*     26 LOCATION HAS NO TRAVEL ENTRIES */
/*     27 HINT NUMBER EXCEEDS GOTO LIST */
/*     28 INVALID MONTH RETURNED BY DATE FUNCTION */
    s_wsfe(&io___295);
    do_fio(&c__1, (char *)&(*num), (ftnlen)sizeof(integer));
    e_wsfe();
    s_stop("", (ftnlen)0);
    return 0;
} /* bug_ */

/*  ADDED IN THE PORT: OBF/UNOBF A VOCAB WORD. THIS TAKES THE PLACE OF */
/*  WHAT WAS A USE OF THE NONPORTABLE .XOR. OPERATOR. THE NAME OF THE */
/*  FUNCTION COMES FROM THE OBFUSCATION KEY USED. */
/* Character */ VOID phrog_(char *ret_val, ftnlen ret_val_len, char *w, ftnlen w_len)
{
    /* Copy w -> ret_val, padded with spaces to ret_val_len. */
    int n = (ret_val_len < w_len ? (int)ret_val_len : (int)w_len);
    int i;
    for (i = 0; i < n; ++i) ret_val[i] = w[i];
    for (; i < (int)ret_val_len; ++i) ret_val[i] = ' ';
}/* phrog_ */

/*  ADDED IN THE PORT: UPPERCASE A 5-CHARACTER ARRAY */
/* Subroutine */ int upcase_(char *s, ftnlen s_len)
{
    static integer i__;

    for (i__ = 1; i__ <= 5; ++i__) {
/*     old single-line IF overflowed col 72: */
/*     IF (S(I:I).GE.'a' .AND. S(I:I).LE.'z') S(I:I)=CHAR(ICHAR(S(I:I))-32) */
	if (*(unsigned char *)&s[i__ - 1] >= 'a' && *(unsigned char *)&s[i__
		- 1] <= 'z') {
	    *(unsigned char *)&s[i__ - 1] = (char) (*(unsigned char *)&s[i__
		    - 1] - 32);
	}
/* L10: */
    }
    return 0;
} /* upcase_ */

/*  ADDED IN THE PORT: SET BIT N OF AN INTEGER VALUE */
/* Subroutine */ int setbit_(integer *val, integer *n)
{
    /* Builtin functions */
    integer pow_ii(integer *, integer *);

/*     Statement function to test bit N */
    if (! (*val / pow_ii(&c__2, n) % 2 != 0)) {
	*val += pow_ii(&c__2, n);
    }
/* === No-save stub for SAVE/SUSPEND (inserted by build script) === */
/* === No-load stub for RESUME (inserted by build script) === */
    return 0;
} /* setbit_ */

/* Main program alias */ int advent_ () { MAIN__ (); return 0; }
