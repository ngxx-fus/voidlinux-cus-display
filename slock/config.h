/* user and group to drop privileges to */
static const char *user  = "fus";
static const char *group = "fus";

static const char *colorname[NUMCOLS] = {
	[INIT] =   "black",     /* after initialization */
	[INPUT] =  "#005577",   /* during input */
	[FAILED] = "#cc3333",   /* wrong password */
};

/* lock screen opacity */
static const float alpha = 0.5;
static const int    lineHeight = 15;

/* treat a cleared input like a wrong password (color) */
static const int failonclear = 1;

/* default message */
static const char * message  = "Enter password to continue!";
static const char * wrongPW  = "Wrong password!";

/* text color */
static const char * text_color = "#ffffff";

/* text size (must be a valid size) */
static const char * text_size = "10x20";
