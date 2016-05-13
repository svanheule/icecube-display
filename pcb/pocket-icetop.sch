EESchema Schematic File Version 2
LIBS:power
LIBS:device
LIBS:transistors
LIBS:conn
LIBS:linear
LIBS:regul
LIBS:74xx
LIBS:cmos4000
LIBS:adc-dac
LIBS:memory
LIBS:xilinx
LIBS:microcontrollers
LIBS:dsp
LIBS:microchip
LIBS:analog_switches
LIBS:motorola
LIBS:texas
LIBS:intel
LIBS:audio
LIBS:interface
LIBS:digital-audio
LIBS:philips
LIBS:display
LIBS:cypress
LIBS:siliconi
LIBS:opto
LIBS:atmel
LIBS:contrib
LIBS:valves
LIBS:pocket-icetop-cache
EELAYER 25 0
EELAYER END
$Descr A4 11693 8268
encoding utf-8
Sheet 1 1
Title "IceTop event display controller"
Date "2016-05-13"
Rev "2"
Comp "Universiteit Gent"
Comment1 "Vakgroep Fysica en Sterrenkunde"
Comment2 "Sander Vanheule"
Comment3 ""
Comment4 ""
$EndDescr
$Comp
L ATMEGA32U4-A U1
U 1 1 56914079
P 4050 4150
F 0 "U1" H 3100 5850 50  0000 C CNN
F 1 "ATMEGA32U4-AU" H 4750 2650 50  0000 C CNN
F 2 "Housings_QFP:TQFP-44_10x10mm_Pitch0.8mm" H 4050 4150 50  0001 C CIN
F 3 "" H 5150 5250 50  0000 C CNN
F 4 "http://www.conrad.be/ce/nl/product/1266866" H 4050 4150 60  0001 C CNN "URL"
	1    4050 4150
	1    0    0    -1  
$EndComp
$Comp
L Crystal Y1
U 1 1 569141C7
P 2600 3100
F 0 "Y1" H 2600 3250 50  0000 C CNN
F 1 "Crystal" H 2600 2950 50  0000 C CNN
F 2 "Crystals:Crystal_HC49-U_Vertical" H 2600 3100 50  0001 C CNN
F 3 "" H 2600 3100 50  0000 C CNN
F 4 "18pF" H 2600 3100 60  0001 C CNN "Load capacitance"
F 5 "http://www.conrad.be/ce/nl/product/155145" H 2600 3100 60  0001 C CNN "URL"
	1    2600 3100
	0    1    1    0   
$EndComp
$Comp
L R R1
U 1 1 569143D9
P 2200 3950
F 0 "R1" V 2280 3950 50  0000 C CNN
F 1 "22" V 2200 3950 50  0000 C CNN
F 2 "Resistors_SMD:R_0603_HandSoldering" V 2130 3950 50  0001 C CNN
F 3 "" H 2200 3950 50  0000 C CNN
F 4 "https://www.conrad.be/ce/nl/product/1208485" V 2200 3950 60  0001 C CNN "URL"
	1    2200 3950
	0    1    1    0   
$EndComp
$Comp
L R R2
U 1 1 56914466
P 2500 4050
F 0 "R2" V 2580 4050 50  0000 C CNN
F 1 "22" V 2500 4050 50  0000 C CNN
F 2 "Resistors_SMD:R_0603_HandSoldering" V 2430 4050 50  0001 C CNN
F 3 "" H 2500 4050 50  0000 C CNN
F 4 "https://www.conrad.be/ce/nl/product/1208485" V 2500 4050 60  0001 C CNN "URL"
	1    2500 4050
	0    1    1    0   
$EndComp
$Comp
L BARREL_JACK CON1
U 1 1 56914803
P 2750 950
F 0 "CON1" H 2750 1200 50  0000 C CNN
F 1 "5V_DC" H 2750 750 50  0000 C CNN
F 2 "Connect:BARREL_JACK" H 2750 950 50  0001 C CNN
F 3 "" H 2750 950 50  0000 C CNN
	1    2750 950 
	1    0    0    -1  
$EndComp
$Comp
L LED D2
U 1 1 56914DF1
P 5700 2600
F 0 "D2" H 5700 2700 50  0000 C CNN
F 1 "USB_ACT" H 5700 2500 50  0000 C CNN
F 2 "LEDs:LED-3MM" H 5700 2600 50  0001 C CNN
F 3 "" H 5700 2600 50  0000 C CNN
F 4 "Low current (10mA) orange LED" H 5700 2600 60  0001 C CNN "Comment"
F 5 "http://www.conrad.be/ce/nl/product/180652" H 5700 2600 60  0001 C CNN "URL"
F 6 "http://www.conrad.be/ce/nl/product/181922" H 5700 2600 60  0001 C CNN "LED spacer"
	1    5700 2600
	1    0    0    -1  
$EndComp
$Comp
L LED D1
U 1 1 5691521A
P 1000 2450
F 0 "D1" H 1000 2550 50  0000 C CNN
F 1 "POWER" H 1000 2350 50  0000 C CNN
F 2 "LEDs:LED-3MM" H 1000 2450 50  0001 C CNN
F 3 "" H 1000 2450 50  0000 C CNN
F 4 "Low current (10mA) green LED" H 1000 2450 60  0001 C CNN "Comment"
F 5 "http://www.conrad.be/ce/nl/product/180612" H 1000 2450 60  0001 C CNN "URL"
F 6 "http://www.conrad.be/ce/nl/product/181922" H 1000 2450 60  0001 C CNN "LED spacer"
	1    1000 2450
	0    -1   -1   0   
$EndComp
$Comp
L SW_PUSH SW1
U 1 1 56915346
P 2400 2600
F 0 "SW1" H 2550 2710 50  0000 C CNN
F 1 "RESET" H 2400 2520 50  0000 C CNN
F 2 "pocket-icetop:SW_Tactile_SPST_Angled" H 2400 2600 50  0001 C CNN
F 3 "" H 2400 2600 50  0000 C CNN
F 4 "http://www.conrad.be/ce/nl/product/700019" H 2400 2600 60  0001 C CNN "URL"
F 5 "3.85mm" H 2400 2600 60  0001 C CNN "Stem length"
	1    2400 2600
	1    0    0    -1  
$EndComp
$Comp
L GND #PWR30
U 1 1 56915CE3
P 6500 4800
F 0 "#PWR30" H 6500 4550 50  0001 C CNN
F 1 "GND" H 6500 4650 50  0000 C CNN
F 2 "" H 6500 4800 50  0000 C CNN
F 3 "" H 6500 4800 50  0000 C CNN
	1    6500 4800
	1    0    0    -1  
$EndComp
Text GLabel 5400 4150 2    60   Output ~ 0
LED_dat
Text GLabel 5400 4300 2    60   Output ~ 0
LED_clk
$Comp
L C_Small C1
U 1 1 569283D7
P 2200 2950
F 0 "C1" H 2210 3020 50  0000 L CNN
F 1 "6.8p" H 2210 2870 50  0000 L CNN
F 2 "Capacitors_SMD:C_0603_HandSoldering" H 2200 2950 50  0001 C CNN
F 3 "" H 2200 2950 50  0000 C CNN
F 4 "See crystal datasheet for required total capacitance" H 2200 2950 60  0001 C CNN "Comment"
F 5 "https://www.conrad.be/ce/nl/product/1279779" H 2200 2950 60  0001 C CNN "URL"
	1    2200 2950
	0    1    1    0   
$EndComp
$Comp
L C_Small C2
U 1 1 56928454
P 2200 3250
F 0 "C2" H 2210 3320 50  0000 L CNN
F 1 "6.8p" H 2210 3170 50  0000 L CNN
F 2 "Capacitors_SMD:C_0603_HandSoldering" H 2200 3250 50  0001 C CNN
F 3 "" H 2200 3250 50  0000 C CNN
F 4 "See crystal datasheet for required total capacitance" H 2200 3250 60  0001 C CNN "Comment"
F 5 "https://www.conrad.be/ce/nl/product/1279779" H 2200 3250 60  0001 C CNN "URL"
	1    2200 3250
	0    1    1    0   
$EndComp
$Comp
L GND #PWR8
U 1 1 56928DEF
P 1950 2700
F 0 "#PWR8" H 1950 2450 50  0001 C CNN
F 1 "GND" H 1950 2550 50  0000 C CNN
F 2 "" H 1950 2700 50  0000 C CNN
F 3 "" H 1950 2700 50  0000 C CNN
	1    1950 2700
	1    0    0    -1  
$EndComp
$Comp
L VCC #PWR11
U 1 1 56929032
P 2800 1850
F 0 "#PWR11" H 2800 1700 50  0001 C CNN
F 1 "VCC" H 2800 2000 50  0000 C CNN
F 2 "" H 2800 1850 50  0000 C CNN
F 3 "" H 2800 1850 50  0000 C CNN
	1    2800 1850
	1    0    0    -1  
$EndComp
$Comp
L VCC #PWR34
U 1 1 5692C264
P 8050 800
F 0 "#PWR34" H 8050 650 50  0001 C CNN
F 1 "VCC" H 8050 950 50  0000 C CNN
F 2 "" H 8050 800 50  0000 C CNN
F 3 "" H 8050 800 50  0000 C CNN
	1    8050 800 
	1    0    0    -1  
$EndComp
$Comp
L GND #PWR35
U 1 1 5692C2C0
P 8050 1700
F 0 "#PWR35" H 8050 1450 50  0001 C CNN
F 1 "GND" H 8050 1550 50  0000 C CNN
F 2 "" H 8050 1700 50  0000 C CNN
F 3 "" H 8050 1700 50  0000 C CNN
	1    8050 1700
	1    0    0    -1  
$EndComp
$Comp
L R_Small R4
U 1 1 5692FA0E
P 1000 2150
F 0 "R4" H 1030 2170 50  0000 L CNN
F 1 "1k" H 1030 2110 50  0000 L CNN
F 2 "Resistors_SMD:R_0603_HandSoldering" H 1000 2150 50  0001 C CNN
F 3 "" H 1000 2150 50  0000 C CNN
F 4 "https://www.conrad.be/ce/nl/product/1374968" H 1000 2150 60  0001 C CNN "URL"
	1    1000 2150
	1    0    0    -1  
$EndComp
$Comp
L R_Small R5
U 1 1 5692FA69
P 6000 2600
F 0 "R5" H 6030 2620 50  0000 L CNN
F 1 "1k" H 6030 2560 50  0000 L CNN
F 2 "Resistors_SMD:R_0603_HandSoldering" H 6000 2600 50  0001 C CNN
F 3 "" H 6000 2600 50  0000 C CNN
F 4 "https://www.conrad.be/ce/nl/product/1374968" H 6000 2600 60  0001 C CNN "URL"
	1    6000 2600
	0    1    1    0   
$EndComp
$Comp
L VCC #PWR29
U 1 1 5692FF43
P 6250 2350
F 0 "#PWR29" H 6250 2200 50  0001 C CNN
F 1 "VCC" H 6250 2500 50  0000 C CNN
F 2 "" H 6250 2350 50  0000 C CNN
F 3 "" H 6250 2350 50  0000 C CNN
	1    6250 2350
	1    0    0    -1  
$EndComp
$Comp
L CONN_01X04 P4
U 1 1 56930B0C
P 8300 4800
F 0 "P4" H 8300 5050 50  0000 C CNN
F 1 "LED_STRIP" V 8400 4800 50  0000 C CNN
F 2 "Pin_Headers:Pin_Header_Straight_1x04" H 8300 4800 50  0001 C CNN
F 3 "" H 8300 4800 50  0000 C CNN
	1    8300 4800
	0    -1   -1   0   
$EndComp
$Comp
L GND #PWR38
U 1 1 56930FEB
P 9050 5200
F 0 "#PWR38" H 9050 4950 50  0001 C CNN
F 1 "GND" H 9050 5050 50  0000 C CNN
F 2 "" H 9050 5200 50  0000 C CNN
F 3 "" H 9050 5200 50  0000 C CNN
	1    9050 5200
	1    0    0    -1  
$EndComp
$Comp
L VCC #PWR32
U 1 1 56931029
P 7900 4250
F 0 "#PWR32" H 7900 4100 50  0001 C CNN
F 1 "VCC" H 7900 4400 50  0000 C CNN
F 2 "" H 7900 4250 50  0000 C CNN
F 3 "" H 7900 4250 50  0000 C CNN
	1    7900 4250
	1    0    0    -1  
$EndComp
Text GLabel 8100 5200 0    60   Input ~ 0
LED_dat
Text GLabel 8100 5350 0    60   Input ~ 0
LED_clk
$Comp
L C_Small C4
U 1 1 569381C3
P 2600 3550
F 0 "C4" H 2610 3620 50  0000 L CNN
F 1 "10u" H 2610 3470 50  0000 L CNN
F 2 "Capacitors_SMD:C_0603_HandSoldering" H 2600 3550 50  0001 C CNN
F 3 "" H 2600 3550 50  0000 C CNN
F 4 "https://www.conrad.be/ce/nl/product/450781" H 2600 3550 60  0001 C CNN "URL"
	1    2600 3550
	0    -1   -1   0   
$EndComp
$Comp
L JUMPER JP1
U 1 1 56938C14
P 5750 4700
F 0 "JP1" H 5750 4850 50  0000 C CNN
F 1 "JUMPER" H 5750 4620 50  0000 C CNN
F 2 "Pin_Headers:Pin_Header_Straight_1x02" H 5750 4700 50  0001 C CNN
F 3 "" H 5750 4700 50  0000 C CNN
F 4 "Use pin header and jumper" H 5750 4700 60  0001 C CNN "Comment"
	1    5750 4700
	1    0    0    -1  
$EndComp
$Comp
L SW_PUSH SW3
U 1 1 5697DB1C
P 8700 3300
F 0 "SW3" H 8850 3410 50  0000 C CNN
F 1 "SW_DEMO2" H 8700 3220 50  0000 C CNN
F 2 "pocket-icetop:SW_Tactile_SPST_Angled" H 8700 3300 50  0001 C CNN
F 3 "" H 8700 3300 50  0000 C CNN
F 4 "http://www.conrad.be/ce/nl/product/705066" H 8700 3300 60  0001 C CNN "URL"
F 5 "5.85mm" H 8700 3300 60  0001 C CNN "Stem length"
	1    8700 3300
	-1   0    0    -1  
$EndComp
$Comp
L R_Small R6
U 1 1 5697DC21
P 8350 2850
F 0 "R6" H 8380 2870 50  0000 L CNN
F 1 "10k" H 8380 2810 50  0000 L CNN
F 2 "Resistors_SMD:R_0603_HandSoldering" H 8350 2850 50  0001 C CNN
F 3 "" H 8350 2850 50  0000 C CNN
F 4 "https://www.conrad.be/ce/nl/product/1375015" H 8350 2850 60  0001 C CNN "URL"
	1    8350 2850
	1    0    0    1   
$EndComp
$Comp
L GND #PWR39
U 1 1 5697DFDC
P 9100 3400
F 0 "#PWR39" H 9100 3150 50  0001 C CNN
F 1 "GND" H 9100 3250 50  0000 C CNN
F 2 "" H 9100 3400 50  0000 C CNN
F 3 "" H 9100 3400 50  0000 C CNN
	1    9100 3400
	1    0    0    -1  
$EndComp
$Comp
L R_Small R3
U 1 1 5697FF75
P 2800 2100
F 0 "R3" H 2830 2120 50  0000 L CNN
F 1 "10k" H 2830 2060 50  0000 L CNN
F 2 "Resistors_SMD:R_0603_HandSoldering" H 2800 2100 50  0001 C CNN
F 3 "" H 2800 2100 50  0000 C CNN
F 4 "https://www.conrad.be/ce/nl/product/1375015" H 2800 2100 60  0001 C CNN "URL"
	1    2800 2100
	1    0    0    1   
$EndComp
$Comp
L GND #PWR3
U 1 1 5698169E
P 1000 2800
F 0 "#PWR3" H 1000 2550 50  0001 C CNN
F 1 "GND" H 1000 2650 50  0000 C CNN
F 2 "" H 1000 2800 50  0000 C CNN
F 3 "" H 1000 2800 50  0000 C CNN
	1    1000 2800
	1    0    0    -1  
$EndComp
$Comp
L VCC #PWR2
U 1 1 5698287C
P 1000 1900
F 0 "#PWR2" H 1000 1750 50  0001 C CNN
F 1 "VCC" H 1000 2050 50  0000 C CNN
F 2 "" H 1000 1900 50  0000 C CNN
F 3 "" H 1000 1900 50  0000 C CNN
	1    1000 1900
	1    0    0    -1  
$EndComp
$Comp
L R_Small R7
U 1 1 56984776
P 6300 4700
F 0 "R7" H 6330 4720 50  0000 L CNN
F 1 "10k" H 6330 4660 50  0000 L CNN
F 2 "Resistors_SMD:R_0603_HandSoldering" H 6300 4700 50  0001 C CNN
F 3 "" H 6300 4700 50  0000 C CNN
F 4 "https://www.conrad.be/ce/nl/product/1375015" H 6300 4700 60  0001 C CNN "URL"
	1    6300 4700
	0    1    1    0   
$EndComp
$Comp
L C_Small C3
U 1 1 56985395
P 2800 4350
F 0 "C3" H 2810 4420 50  0000 L CNN
F 1 "1u" H 2810 4270 50  0000 L CNN
F 2 "Capacitors_SMD:C_0603_HandSoldering" H 2800 4350 50  0001 C CNN
F 3 "" H 2800 4350 50  0000 C CNN
F 4 "http://www.conrad.be/ce/nl/product/450696" H 2800 4350 60  0001 C CNN "URL"
	1    2800 4350
	1    0    0    -1  
$EndComp
$Comp
L USB_B P1
U 1 1 5698838C
P 1500 3500
F 0 "P1" H 1700 3300 50  0000 C CNN
F 1 "USB_B" H 1450 3700 50  0000 C CNN
F 2 "pocket-icetop:USB_B" V 1450 3400 50  0001 C CNN
F 3 "" V 1450 3400 50  0000 C CNN
F 4 "http://www.conrad.be/ce/nl/product/738897" H 1500 3500 60  0001 C CNN "URL"
	1    1500 3500
	-1   0    0    -1  
$EndComp
NoConn ~ 5150 3000
NoConn ~ 5150 3100
NoConn ~ 5150 3500
NoConn ~ 5150 3600
NoConn ~ 5150 4800
NoConn ~ 5150 5000
NoConn ~ 5150 5100
NoConn ~ 5150 5200
NoConn ~ 5150 5300
NoConn ~ 5150 5400
NoConn ~ 5150 5500
NoConn ~ 5150 4500
NoConn ~ 5150 4400
NoConn ~ 5150 4200
NoConn ~ 2900 5000
NoConn ~ 5150 4000
NoConn ~ 5150 3200
Text Label 5150 2700 0    60   ~ 0
SCK
Text Label 5150 2800 0    60   ~ 0
MOSI
Text Label 5150 2900 0    60   ~ 0
MISO
Text Label 2800 2550 1    60   ~ 0
RESET
Text Label 3600 2350 1    60   ~ 0
UVCC
Text Label 3850 2350 1    60   ~ 0
VCC1
Text Label 3950 2350 1    60   ~ 0
VCC2
Text Label 4200 2350 1    60   ~ 0
AVCC1
Text Label 4300 2350 1    60   ~ 0
AVCC2
$Comp
L C_Small C5
U 1 1 569D0912
P 4950 1250
F 0 "C5" H 4960 1320 50  0000 L CNN
F 1 "100n" H 4960 1170 50  0000 L CNN
F 2 "Capacitors_SMD:C_0603_HandSoldering" H 4950 1250 50  0001 C CNN
F 3 "" H 4950 1250 50  0000 C CNN
F 4 "http://www.conrad.be/ce/nl/product/1279681" H 4950 1250 60  0001 C CNN "URL"
	1    4950 1250
	1    0    0    -1  
$EndComp
$Comp
L C_Small C6
U 1 1 569D0A30
P 5200 1250
F 0 "C6" H 5210 1320 50  0000 L CNN
F 1 "100n" H 5210 1170 50  0000 L CNN
F 2 "Capacitors_SMD:C_0603_HandSoldering" H 5200 1250 50  0001 C CNN
F 3 "" H 5200 1250 50  0000 C CNN
F 4 "http://www.conrad.be/ce/nl/product/1279681" H 5200 1250 60  0001 C CNN "URL"
	1    5200 1250
	1    0    0    -1  
$EndComp
$Comp
L C_Small C7
U 1 1 569D0AA8
P 5450 1250
F 0 "C7" H 5460 1320 50  0000 L CNN
F 1 "100n" H 5460 1170 50  0000 L CNN
F 2 "Capacitors_SMD:C_0603_HandSoldering" H 5450 1250 50  0001 C CNN
F 3 "" H 5450 1250 50  0000 C CNN
F 4 "http://www.conrad.be/ce/nl/product/1279681" H 5450 1250 60  0001 C CNN "URL"
	1    5450 1250
	1    0    0    -1  
$EndComp
$Comp
L C_Small C8
U 1 1 569D0B09
P 5700 1250
F 0 "C8" H 5710 1320 50  0000 L CNN
F 1 "100n" H 5710 1170 50  0000 L CNN
F 2 "Capacitors_SMD:C_0603_HandSoldering" H 5700 1250 50  0001 C CNN
F 3 "" H 5700 1250 50  0000 C CNN
F 4 "http://www.conrad.be/ce/nl/product/1279681" H 5700 1250 60  0001 C CNN "URL"
	1    5700 1250
	1    0    0    -1  
$EndComp
$Comp
L C_Small C9
U 1 1 569D0B7F
P 5950 1250
F 0 "C9" H 5960 1320 50  0000 L CNN
F 1 "100n" H 5960 1170 50  0000 L CNN
F 2 "Capacitors_SMD:C_0603_HandSoldering" H 5950 1250 50  0001 C CNN
F 3 "" H 5950 1250 50  0000 C CNN
F 4 "http://www.conrad.be/ce/nl/product/1279681" H 5950 1250 60  0001 C CNN "URL"
	1    5950 1250
	1    0    0    -1  
$EndComp
Text Label 4950 1150 1    60   ~ 0
UVCC
Text Label 5200 1150 1    60   ~ 0
VCC1
Text Label 5450 1150 1    60   ~ 0
VCC2
Text Label 5700 1150 1    60   ~ 0
AVCC1
Text Label 5950 1150 1    60   ~ 0
AVCC2
$Comp
L CONN_02X03 P3
U 1 1 569D387D
P 9950 3200
F 0 "P3" H 9950 3400 50  0000 C CNN
F 1 "ICSP" H 9950 3000 50  0000 C CNN
F 2 "Pin_Headers:Pin_Header_Straight_2x03" H 9950 2000 50  0001 C CNN
F 3 "" H 9950 2000 50  0000 C CNN
F 4 "Use pin header" H 9950 3200 60  0001 C CNN "Comment"
	1    9950 3200
	1    0    0    -1  
$EndComp
$Comp
L VCC #PWR40
U 1 1 569D3A3B
P 10300 3000
F 0 "#PWR40" H 10300 2850 50  0001 C CNN
F 1 "VCC" H 10300 3150 50  0000 C CNN
F 2 "" H 10300 3000 50  0000 C CNN
F 3 "" H 10300 3000 50  0000 C CNN
	1    10300 3000
	1    0    0    -1  
$EndComp
$Comp
L GND #PWR41
U 1 1 569D3ADB
P 10300 3400
F 0 "#PWR41" H 10300 3150 50  0001 C CNN
F 1 "GND" H 10300 3250 50  0000 C CNN
F 2 "" H 10300 3400 50  0000 C CNN
F 3 "" H 10300 3400 50  0000 C CNN
	1    10300 3400
	1    0    0    -1  
$EndComp
Text Label 10200 3200 0    60   ~ 0
MOSI
Text Label 9700 3100 2    60   ~ 0
MISO
Text Label 9700 3200 2    60   ~ 0
SCK
Text Label 9700 3300 2    60   ~ 0
RESET
$Comp
L PWR_FLAG #FLG1
U 1 1 569D75B7
P 750 800
F 0 "#FLG1" H 750 895 50  0001 C CNN
F 1 "PWR_FLAG" H 750 980 50  0000 C CNN
F 2 "" H 750 800 50  0000 C CNN
F 3 "" H 750 800 50  0000 C CNN
	1    750  800 
	1    0    0    -1  
$EndComp
$Comp
L PWR_FLAG #FLG2
U 1 1 569D7639
P 1150 800
F 0 "#FLG2" H 1150 895 50  0001 C CNN
F 1 "PWR_FLAG" H 1150 980 50  0000 C CNN
F 2 "" H 1150 800 50  0000 C CNN
F 3 "" H 1150 800 50  0000 C CNN
	1    1150 800 
	1    0    0    -1  
$EndComp
$Comp
L GND #PWR5
U 1 1 569D76D7
P 1150 950
F 0 "#PWR5" H 1150 700 50  0001 C CNN
F 1 "GND" H 1150 800 50  0000 C CNN
F 2 "" H 1150 950 50  0000 C CNN
F 3 "" H 1150 950 50  0000 C CNN
	1    1150 950 
	1    0    0    -1  
$EndComp
$Comp
L VCC #PWR1
U 1 1 569D7723
P 750 950
F 0 "#PWR1" H 750 800 50  0001 C CNN
F 1 "VCC" H 750 1100 50  0000 C CNN
F 2 "" H 750 950 50  0000 C CNN
F 3 "" H 750 950 50  0000 C CNN
	1    750  950 
	-1   0    0    1   
$EndComp
$Comp
L CONN_02X08 P2
U 1 1 569DD22A
P 8400 1250
F 0 "P2" H 8400 1700 50  0000 C CNN
F 1 "LED_POWER" V 8400 1250 50  0000 C CNN
F 2 "pocket-icetop:IDC_socket_2x08" H 8400 50  50  0001 C CNN
F 3 "" H 8400 50  50  0000 C CNN
F 4 "https://www.conrad.be/ce/nl/product/1088165" H 8400 1250 60  0001 C CNN "URL"
F 5 "https://www.conrad.be/ce/nl/product/1088171" H 8400 1250 60  0001 C CNN "Connector"
F 6 "https://www.conrad.be/ce/nl/product/1088166" H 8400 1250 60  0001 C CNN "Alternative"
	1    8400 1250
	0    1    1    0   
$EndComp
Text Label 8350 3300 1    60   ~ 0
DEMO2
Text Label 5150 3800 0    60   ~ 0
DEMO1
$Comp
L VCC #PWR37
U 1 1 569E2AD9
P 8350 2600
F 0 "#PWR37" H 8350 2450 50  0001 C CNN
F 1 "VCC" H 8350 2750 50  0000 C CNN
F 2 "" H 8350 2600 50  0000 C CNN
F 3 "" H 8350 2600 50  0000 C CNN
	1    8350 2600
	1    0    0    -1  
$EndComp
$Comp
L SW_PUSH SW2
U 1 1 569E3877
P 7600 3300
F 0 "SW2" H 7750 3410 50  0000 C CNN
F 1 "SW_DEMO1" H 7600 3220 50  0000 C CNN
F 2 "pocket-icetop:SW_Tactile_SPST_Angled" H 7600 3300 50  0001 C CNN
F 3 "" H 7600 3300 50  0000 C CNN
F 4 "http://www.conrad.be/ce/nl/product/705066" H 7600 3300 60  0001 C CNN "URL"
F 5 "5.85mm" H 7600 3300 60  0001 C CNN "Stem length"
	1    7600 3300
	-1   0    0    -1  
$EndComp
$Comp
L R_Small R8
U 1 1 569E387D
P 7250 2850
F 0 "R8" H 7280 2870 50  0000 L CNN
F 1 "10k" H 7280 2810 50  0000 L CNN
F 2 "Resistors_SMD:R_0603_HandSoldering" H 7250 2850 50  0001 C CNN
F 3 "" H 7250 2850 50  0000 C CNN
F 4 "https://www.conrad.be/ce/nl/product/1375015" H 7250 2850 60  0001 C CNN "URL"
	1    7250 2850
	1    0    0    1   
$EndComp
$Comp
L GND #PWR33
U 1 1 569E3883
P 8000 3400
F 0 "#PWR33" H 8000 3150 50  0001 C CNN
F 1 "GND" H 8000 3250 50  0000 C CNN
F 2 "" H 8000 3400 50  0000 C CNN
F 3 "" H 8000 3400 50  0000 C CNN
	1    8000 3400
	1    0    0    -1  
$EndComp
Text Label 7250 3300 1    60   ~ 0
DEMO1
$Comp
L VCC #PWR31
U 1 1 569E388E
P 7250 2600
F 0 "#PWR31" H 7250 2450 50  0001 C CNN
F 1 "VCC" H 7250 2750 50  0000 C CNN
F 2 "" H 7250 2600 50  0000 C CNN
F 3 "" H 7250 2600 50  0000 C CNN
	1    7250 2600
	1    0    0    -1  
$EndComp
Text Label 5150 3900 0    60   ~ 0
DEMO2
NoConn ~ 5150 3300
Text Label 2700 3950 0    60   ~ 0
D+
Text Label 2700 4050 0    60   ~ 0
D-
Text Label 1700 3950 0    60   ~ 0
CONN+
Text Label 1700 4050 0    60   ~ 0
CONN-
Text Label 2600 2950 0    60   ~ 0
XTAL+
Text Label 2600 3250 0    60   ~ 0
XTAL-
$Comp
L C C10
U 1 1 56A1E834
P 4450 1150
F 0 "C10" H 4475 1250 50  0000 L CNN
F 1 "1u" H 4475 1050 50  0000 L CNN
F 2 "Capacitors_SMD:C_0805_HandSoldering" H 4488 1000 50  0001 C CNN
F 3 "" H 4450 1150 50  0000 C CNN
F 4 "http://www.conrad.be/ce/nl/product/445210" H 4450 1150 60  0001 C CNN "URL"
	1    4450 1150
	1    0    0    -1  
$EndComp
$Comp
L GND #PWR18
U 1 1 56A1E918
P 4450 1400
F 0 "#PWR18" H 4450 1150 50  0001 C CNN
F 1 "GND" H 4450 1250 50  0000 C CNN
F 2 "" H 4450 1400 50  0000 C CNN
F 3 "" H 4450 1400 50  0000 C CNN
	1    4450 1400
	1    0    0    -1  
$EndComp
$Comp
L VCC #PWR17
U 1 1 56A1E980
P 4450 900
F 0 "#PWR17" H 4450 750 50  0001 C CNN
F 1 "VCC" H 4450 1050 50  0000 C CNN
F 2 "" H 4450 900 50  0000 C CNN
F 3 "" H 4450 900 50  0000 C CNN
	1    4450 900 
	1    0    0    -1  
$EndComp
$Comp
L L_Small L1
U 1 1 56A1F8E0
P 3550 850
F 0 "L1" H 3580 890 50  0000 L CNN
F 1 "ferrite" H 3580 810 50  0000 L CNN
F 2 "pocket-icetop:L_0603_HandSoldering" H 3550 850 50  0001 C CNN
F 3 "" H 3550 850 50  0000 C CNN
F 4 "https://www.conrad.be/ce/nl/product/1171712" H 3550 850 60  0001 C CNN "URL"
	1    3550 850 
	0    -1   -1   0   
$EndComp
Wire Wire Line
	2900 3000 2800 3000
Wire Wire Line
	2800 3000 2800 2950
Wire Wire Line
	2800 2950 2300 2950
Wire Wire Line
	2300 3250 2800 3250
Wire Wire Line
	2800 3250 2800 3200
Wire Wire Line
	2800 3200 2900 3200
Wire Wire Line
	2700 2600 2900 2600
Wire Wire Line
	2800 2200 2800 2600
Connection ~ 2800 2600
Wire Wire Line
	5150 4700 5450 4700
Wire Wire Line
	5150 4100 5250 4100
Wire Wire Line
	5250 4100 5250 4150
Wire Wire Line
	5250 4150 5400 4150
Wire Wire Line
	5150 4300 5400 4300
Wire Wire Line
	2900 4200 2800 4200
Wire Wire Line
	1700 3800 2900 3800
Wire Wire Line
	2350 3950 2900 3950
Wire Wire Line
	2650 4050 2900 4050
Wire Wire Line
	2050 3950 1500 3950
Wire Wire Line
	1500 3950 1500 3800
Wire Wire Line
	1600 3800 1600 4050
Wire Wire Line
	1600 4050 2350 4050
Connection ~ 2600 2950
Connection ~ 2600 3250
Wire Wire Line
	2100 2950 1950 2950
Wire Wire Line
	1950 2950 1950 3250
Wire Wire Line
	1950 3250 2100 3250
Wire Wire Line
	1100 3400 1200 3400
Wire Wire Line
	1950 2700 1950 2600
Wire Wire Line
	1950 2600 2100 2600
Wire Wire Line
	2800 1850 2800 2000
Wire Wire Line
	3050 950  3050 1050
Wire Wire Line
	3050 850  3450 850 
Wire Wire Line
	1400 3800 1400 5900
Wire Wire Line
	1100 3400 1100 3950
Wire Wire Line
	4200 5750 4200 6100
Wire Wire Line
	3900 5750 3900 5950
Wire Wire Line
	3900 5950 4200 5950
Connection ~ 4200 5950
Wire Wire Line
	4000 5750 4000 5950
Connection ~ 4000 5950
Wire Wire Line
	4100 5750 4100 5950
Connection ~ 4100 5950
Wire Wire Line
	8050 1500 8050 1700
Wire Wire Line
	8050 800  8050 1000
Connection ~ 8050 900 
Connection ~ 8050 1600
Wire Wire Line
	8150 1500 8150 1600
Connection ~ 8150 1600
Wire Wire Line
	8250 1600 8250 1500
Connection ~ 8250 1600
Wire Wire Line
	8350 1600 8350 1500
Connection ~ 8350 1600
Wire Wire Line
	8450 1600 8450 1500
Connection ~ 8450 1600
Wire Wire Line
	8550 1600 8550 1500
Connection ~ 8550 1600
Wire Wire Line
	8650 1600 8650 1500
Connection ~ 8650 1600
Wire Wire Line
	8750 1600 8750 1500
Connection ~ 8750 1600
Wire Wire Line
	8150 1000 8150 900 
Connection ~ 8150 900 
Wire Wire Line
	8250 900  8250 1000
Connection ~ 8250 900 
Wire Wire Line
	8350 900  8350 1000
Connection ~ 8350 900 
Wire Wire Line
	8450 900  8450 1000
Connection ~ 8450 900 
Wire Wire Line
	8550 900  8550 1000
Connection ~ 8550 900 
Wire Wire Line
	8650 900  8650 1000
Connection ~ 8650 900 
Wire Wire Line
	8750 900  8750 1000
Connection ~ 8750 900 
Wire Wire Line
	8150 5000 7900 5000
Wire Wire Line
	7900 5000 7900 4250
Wire Wire Line
	8450 5000 9050 5000
Wire Wire Line
	9050 5000 9050 5200
Wire Wire Line
	2700 3550 2800 3550
Wire Wire Line
	2800 3550 2800 3800
Connection ~ 2800 3800
Wire Wire Line
	2500 3550 2350 3550
Wire Wire Line
	2350 3550 2350 3600
Wire Wire Line
	6250 2600 6100 2600
Wire Wire Line
	9000 3300 9100 3300
Wire Wire Line
	9100 3300 9100 3400
Wire Wire Line
	8350 2950 8350 3300
Wire Wire Line
	8350 3300 8400 3300
Wire Wire Line
	1000 1900 1000 2050
Wire Wire Line
	1000 2800 1000 2650
Wire Wire Line
	6050 4700 6200 4700
Wire Wire Line
	6400 4700 6500 4700
Wire Wire Line
	6500 4700 6500 4800
Wire Wire Line
	2800 4200 2800 4250
Wire Wire Line
	2800 4450 2800 4500
Wire Wire Line
	5150 2600 5500 2600
Wire Wire Line
	1400 5750 3650 5750
Wire Wire Line
	8350 5000 8350 5200
Wire Wire Line
	3950 2350 3950 2000
Wire Wire Line
	4300 2350 4300 2000
Wire Wire Line
	4950 850  4950 1150
Wire Wire Line
	5200 850  5200 1150
Wire Wire Line
	5450 850  5450 1150
Wire Wire Line
	5700 850  5700 1150
Wire Wire Line
	5950 850  5950 1150
Wire Wire Line
	4950 1350 4950 1450
Wire Wire Line
	5200 1350 5200 1450
Wire Wire Line
	5450 1350 5450 1450
Wire Wire Line
	5700 1350 5700 1450
Wire Wire Line
	5950 1350 5950 1450
Wire Wire Line
	10200 3300 10300 3300
Wire Wire Line
	10300 3300 10300 3400
Wire Wire Line
	10300 3000 10300 3100
Wire Wire Line
	10300 3100 10200 3100
Wire Wire Line
	750  950  750  800 
Wire Wire Line
	1150 800  1150 950 
Wire Wire Line
	4200 2350 4200 2000
Wire Wire Line
	3850 2350 3850 2000
Wire Wire Line
	3600 2350 3600 2000
Wire Wire Line
	8050 900  8750 900 
Wire Wire Line
	8050 1600 8750 1600
Wire Wire Line
	8350 2600 8350 2750
Wire Wire Line
	7900 3300 8000 3300
Wire Wire Line
	8000 3300 8000 3400
Wire Wire Line
	7250 2950 7250 3300
Wire Wire Line
	7250 3300 7300 3300
Wire Wire Line
	7250 2600 7250 2750
Wire Wire Line
	6250 2350 6250 2600
Wire Wire Line
	4450 900  4450 1000
Wire Wire Line
	4450 1300 4450 1400
Connection ~ 3350 850 
Wire Wire Line
	3650 850  3800 850 
$Comp
L GND #PWR14
U 1 1 569292A2
P 3350 1050
F 0 "#PWR14" H 3350 800 50  0001 C CNN
F 1 "GND" H 3350 900 50  0000 C CNN
F 2 "" H 3350 1050 50  0000 C CNN
F 3 "" H 3350 1050 50  0000 C CNN
	1    3350 1050
	1    0    0    -1  
$EndComp
$Comp
L VCC #PWR13
U 1 1 5692926A
P 3350 850
F 0 "#PWR13" H 3350 700 50  0001 C CNN
F 1 "VCC" H 3350 1000 50  0000 C CNN
F 2 "" H 3350 850 50  0000 C CNN
F 3 "" H 3350 850 50  0000 C CNN
	1    3350 850 
	1    0    0    -1  
$EndComp
$Comp
L +5VD #PWR15
U 1 1 56A2015D
P 3800 850
F 0 "#PWR15" H 3800 700 50  0001 C CNN
F 1 "+5VD" H 3800 990 50  0000 C CNN
F 2 "" H 3800 850 50  0000 C CNN
F 3 "" H 3800 850 50  0000 C CNN
	1    3800 850 
	1    0    0    -1  
$EndComp
$Comp
L +5VD #PWR19
U 1 1 56A205E4
P 4950 850
F 0 "#PWR19" H 4950 700 50  0001 C CNN
F 1 "+5VD" H 4950 990 50  0000 C CNN
F 2 "" H 4950 850 50  0000 C CNN
F 3 "" H 4950 850 50  0000 C CNN
	1    4950 850 
	1    0    0    -1  
$EndComp
$Comp
L +5VD #PWR21
U 1 1 56A20723
P 5200 850
F 0 "#PWR21" H 5200 700 50  0001 C CNN
F 1 "+5VD" H 5200 990 50  0000 C CNN
F 2 "" H 5200 850 50  0000 C CNN
F 3 "" H 5200 850 50  0000 C CNN
	1    5200 850 
	1    0    0    -1  
$EndComp
$Comp
L +5VD #PWR23
U 1 1 56A20791
P 5450 850
F 0 "#PWR23" H 5450 700 50  0001 C CNN
F 1 "+5VD" H 5450 990 50  0000 C CNN
F 2 "" H 5450 850 50  0000 C CNN
F 3 "" H 5450 850 50  0000 C CNN
	1    5450 850 
	1    0    0    -1  
$EndComp
$Comp
L +5VD #PWR25
U 1 1 56A207FF
P 5700 850
F 0 "#PWR25" H 5700 700 50  0001 C CNN
F 1 "+5VD" H 5700 990 50  0000 C CNN
F 2 "" H 5700 850 50  0000 C CNN
F 3 "" H 5700 850 50  0000 C CNN
	1    5700 850 
	1    0    0    -1  
$EndComp
$Comp
L +5VD #PWR27
U 1 1 56A2086D
P 5950 850
F 0 "#PWR27" H 5950 700 50  0001 C CNN
F 1 "+5VD" H 5950 990 50  0000 C CNN
F 2 "" H 5950 850 50  0000 C CNN
F 3 "" H 5950 850 50  0000 C CNN
	1    5950 850 
	1    0    0    -1  
$EndComp
$Comp
L PWR_FLAG #FLG3
U 1 1 56A2183F
P 1550 800
F 0 "#FLG3" H 1550 895 50  0001 C CNN
F 1 "PWR_FLAG" H 1550 980 50  0000 C CNN
F 2 "" H 1550 800 50  0000 C CNN
F 3 "" H 1550 800 50  0000 C CNN
	1    1550 800 
	1    0    0    -1  
$EndComp
Wire Wire Line
	1550 950  1550 800 
$Comp
L +5VD #PWR7
U 1 1 56A21FBC
P 1550 950
F 0 "#PWR7" H 1550 800 50  0001 C CNN
F 1 "+5VD" H 1550 1090 50  0000 C CNN
F 2 "" H 1550 950 50  0000 C CNN
F 3 "" H 1550 950 50  0000 C CNN
	1    1550 950 
	-1   0    0    1   
$EndComp
Connection ~ 1400 5750
$Comp
L GND #PWR4
U 1 1 56A23B1C
P 1100 3950
F 0 "#PWR4" H 1100 3700 50  0001 C CNN
F 1 "GND" H 1100 3800 50  0000 C CNN
F 2 "" H 1100 3950 50  0000 C CNN
F 3 "" H 1100 3950 50  0000 C CNN
	1    1100 3950
	1    0    0    -1  
$EndComp
Wire Wire Line
	3050 1050 3350 1050
$Comp
L GND #PWR10
U 1 1 56A22B5D
P 2350 3600
F 0 "#PWR10" H 2350 3350 50  0001 C CNN
F 1 "GND" H 2350 3450 50  0000 C CNN
F 2 "" H 2350 3600 50  0000 C CNN
F 3 "" H 2350 3600 50  0000 C CNN
	1    2350 3600
	1    0    0    -1  
$EndComp
$Comp
L GND #PWR6
U 1 1 56A22BC8
P 1400 5900
F 0 "#PWR6" H 1400 5650 50  0001 C CNN
F 1 "GND" H 1400 5750 50  0000 C CNN
F 2 "" H 1400 5900 50  0000 C CNN
F 3 "" H 1400 5900 50  0000 C CNN
	1    1400 5900
	1    0    0    -1  
$EndComp
$Comp
L GND #PWR16
U 1 1 56A23033
P 4200 6100
F 0 "#PWR16" H 4200 5850 50  0001 C CNN
F 1 "GND" H 4200 5950 50  0000 C CNN
F 2 "" H 4200 6100 50  0000 C CNN
F 3 "" H 4200 6100 50  0000 C CNN
	1    4200 6100
	1    0    0    -1  
$EndComp
$Comp
L GND #PWR20
U 1 1 56A2501E
P 4950 1450
F 0 "#PWR20" H 4950 1200 50  0001 C CNN
F 1 "GND" H 4950 1300 50  0000 C CNN
F 2 "" H 4950 1450 50  0000 C CNN
F 3 "" H 4950 1450 50  0000 C CNN
	1    4950 1450
	1    0    0    -1  
$EndComp
$Comp
L GND #PWR22
U 1 1 56A25089
P 5200 1450
F 0 "#PWR22" H 5200 1200 50  0001 C CNN
F 1 "GND" H 5200 1300 50  0000 C CNN
F 2 "" H 5200 1450 50  0000 C CNN
F 3 "" H 5200 1450 50  0000 C CNN
	1    5200 1450
	1    0    0    -1  
$EndComp
$Comp
L GND #PWR24
U 1 1 56A25134
P 5450 1450
F 0 "#PWR24" H 5450 1200 50  0001 C CNN
F 1 "GND" H 5450 1300 50  0000 C CNN
F 2 "" H 5450 1450 50  0000 C CNN
F 3 "" H 5450 1450 50  0000 C CNN
	1    5450 1450
	1    0    0    -1  
$EndComp
$Comp
L GND #PWR26
U 1 1 56A2519F
P 5700 1450
F 0 "#PWR26" H 5700 1200 50  0001 C CNN
F 1 "GND" H 5700 1300 50  0000 C CNN
F 2 "" H 5700 1450 50  0000 C CNN
F 3 "" H 5700 1450 50  0000 C CNN
	1    5700 1450
	1    0    0    -1  
$EndComp
$Comp
L GND #PWR28
U 1 1 56A2520A
P 5950 1450
F 0 "#PWR28" H 5950 1200 50  0001 C CNN
F 1 "GND" H 5950 1300 50  0000 C CNN
F 2 "" H 5950 1450 50  0000 C CNN
F 3 "" H 5950 1450 50  0000 C CNN
	1    5950 1450
	1    0    0    -1  
$EndComp
$Comp
L GND #PWR12
U 1 1 56A25DDC
P 2800 4500
F 0 "#PWR12" H 2800 4250 50  0001 C CNN
F 1 "GND" H 2800 4350 50  0000 C CNN
F 2 "" H 2800 4500 50  0000 C CNN
F 3 "" H 2800 4500 50  0000 C CNN
	1    2800 4500
	1    0    0    -1  
$EndComp
$Comp
L GND #PWR9
U 1 1 56A26245
P 1950 3250
F 0 "#PWR9" H 1950 3000 50  0001 C CNN
F 1 "GND" H 1950 3100 50  0000 C CNN
F 2 "" H 1950 3250 50  0000 C CNN
F 3 "" H 1950 3250 50  0000 C CNN
	1    1950 3250
	1    0    0    -1  
$EndComp
Wire Wire Line
	8250 5000 8250 5450
Wire Wire Line
	8250 5350 8100 5350
$Comp
L R_Small R9
U 1 1 57358B64
P 8250 5550
F 0 "R9" H 8280 5570 50  0000 L CNN
F 1 "10k" H 8280 5510 50  0000 L CNN
F 2 "" H 8250 5550 50  0000 C CNN
F 3 "" H 8250 5550 50  0000 C CNN
	1    8250 5550
	1    0    0    -1  
$EndComp
Wire Wire Line
	8350 5200 8100 5200
Connection ~ 8250 5350
$Comp
L GND #PWR36
U 1 1 57358E99
P 8250 5750
F 0 "#PWR36" H 8250 5500 50  0001 C CNN
F 1 "GND" H 8250 5600 50  0000 C CNN
F 2 "" H 8250 5750 50  0000 C CNN
F 3 "" H 8250 5750 50  0000 C CNN
	1    8250 5750
	1    0    0    -1  
$EndComp
Wire Wire Line
	8250 5650 8250 5750
Text Notes 850  6800 0    60   ~ 0
Rev 2: Add pull-down on LED_clk to suppres glitches on power connect
$EndSCHEMATC
