//Modify this file to change what commands output to your statusbar, and recompile using the make command.
static const Block blocks[] = {
    /* Icon   */    /* Command */                                                                       /* Interval */    /* Signal */
    { " [",         "", 0, 0},
    { " ",         "top -bn1 | awk '/^%Cpu/ { printf \"%.1f%%\\n\", 100 - $8 }'",                      1,              0 },
    { " ",         "free -h | awk '/^Mem/ { print $3 }' | sed 's/i//g'",                               1,              0 },
    { " ",         "sensors | grep \"Package id 0\" | awk '{print $4}'",                               1,              0 },
    { "| ",         "/home/fus/.fus/get_wifi.sh",                                                       1,              0 },
    { "| 󰃠 ",       "/home/fus/.fus/brightness_control.sh --get-value",                                 1,              17 },
    { "|  ",       "/home/fus/.fus/volume_control.sh --get-volume",                                    1,              15 },
    { "| ",         "/home/fus/.fus/get_battery_level.sh",                                              1,              16 },
    { "| 󰔟 ",       "date '+%a %H:%M:%S %d/%m/%Y'",                                                        1,              0 },
    { "]   ",      "", 0, 0},
};

//sets delimiter between status commands. NULL character ('\0') means no delimiter.
static char delim[] = " ";
static unsigned int delimLen = 5;
