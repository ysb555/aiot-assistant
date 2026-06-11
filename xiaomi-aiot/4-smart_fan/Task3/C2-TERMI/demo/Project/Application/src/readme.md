cmd

{"turn", "Turn the fan on or off", cmd_turn},
	{"set", "Set the temperature threshold", cmd_set},
	{"dis", "display the temperature or humidity", cmd_display},
	{"check", "Check the temperature thresholds", cmd_check},
	{"test", "Test the System", cmd_test},
	{"nfc", "NFC control", cmd_nfc},

    turn on/off
    dis t/h/off         temperature or humidity or nop
    set t1/t2 [data]    set temperature thresholds
    check t1/t2         show temperature thresholds
    set developer/normal/read       nfc mode
    nfc write [speed1, speed2, threshold1, threshold2]  set data of nfc
    nfc priv    [priv]              0x08,0x04,0x02