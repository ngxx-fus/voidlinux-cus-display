/* user and group to drop privileges to */
static const char *user  = "fus";
static const char *group = "fus";

/*
###############################################
#											  #
#											  #
#				status message				  # index=1, lineCount=4
#				date and time				  # index=2, lineCount=4
#				info message				  # index=3, lineCount=4
#				blank line					  # index=0, lineCount=4
#											  #
###############################################
You can add more infos by incr the number of lines (lineCount)
*/

static const char *colorname[NUMCOLS] = {
	[INIT] =   "black",     /* after initialization */
	[INPUT] =  "#005577",   /* during input */
	[FAILED] = "#cc3333",   /* wrong password */
	[INPUT_PW] = "#7D8D86",   /* during input with show password*/
};

/* text color */
static const char * text_color = "#ffffff";
static const float 	alpha = 0.5;		// lock screen opacity
static const int    lineHeight 	= 0;	// height of line
static const int    lineCount 	= 4;	// number of lines
static const int 	failOnClear = 0;	// treat a cleared input like a wrong password (color)
static const char * defaultMsg  = "Enter password to continue!";
static const char * wrongPWMsg 	= "Wrong password!";
static const char * infoMsg 	= "Nguyen Thanh Phu - 0845939722 - msnp@outlook.com.vn";
static const char * blankMsg 	= " "; 
static const char * text_size 	= "10x20";// text size (must be a valid size)

#define CtrlL_as_ShowHidePW 	1	/// Default
#define CtrlR_as_ShowHidePW 	0
#define AltL_as_ShowHidePW 		0
#define AltR_as_ShowHidePW 		0

/*This is the hidden pw pattern*/
/*DO NOT CHANGE!*/
static const char stars[] =
"###################################"
"################################################################"
"################################################################"
"################################################################"
"############################";
