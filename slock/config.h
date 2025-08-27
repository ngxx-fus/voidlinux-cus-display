/* user and group to drop privileges to */
static const char *user  = "fus";
static const char *group = "fus";

/*Font settings for the time text*/
static const float textsize=64.0;
static const char* textfamily="serif";
static const double textcolorred=255;
static const double textcolorgreen=255;
static const double textcolorblue=255;
 

static const char *colorname[NUMCOLS] = {
	[INIT] =   "black",     /* after initialization */
	[INPUT] =  "#005577",   /* during input */
	[FAILED] = "#CC3333",   /* wrong password */
};

/* treat a cleared input like a wrong password (color) */
static const int failonclear = 1;

/* Background image path, should be available to the user above */
static const char *background_image = "/home/fus/.fus/.BG/IMG_4263.JPG";
