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
LIBS:modular_connectors
LIBS:string_adapter-cache
EELAYER 25 0
EELAYER END
$Descr A4 11693 8268
encoding utf-8
Sheet 1 1
Title "LED strip adapter"
Date ""
Rev "1"
Comp "Universiteit Gent"
Comment1 "Vakgroep fysica en sterrenkunde"
Comment2 "Sander Vanheule"
Comment3 ""
Comment4 ""
$EndDescr
$Comp
L 4P4C P1
U 1 1 56E69467
P 3450 2800
F 0 "P1" H 3700 3300 60  0000 C CNN
F 1 "4P4C" H 3300 3300 60  0000 C CNN
F 2 "modular_connectors:4P4C_horizontal" V 3050 2800 60  0000 C CNN
F 3 "" H 3450 2800 60  0000 C CNN
	1    3450 2800
	1    0    0    -1  
$EndComp
$Comp
L 6P6C P2
U 1 1 56E694BE
P 4600 2800
F 0 "P2" H 4850 3300 60  0000 C CNN
F 1 "6P6C" H 4450 3300 60  0000 C CNN
F 2 "modular_connectors:6P6C_vertical" V 5000 2800 60  0000 C CNN
F 3 "" H 4600 2800 60  0000 C CNN
	1    4600 2800
	-1   0    0    -1  
$EndComp
$Comp
L CONN_01X02 P3
U 1 1 56E69505
P 4800 4200
F 0 "P3" H 4800 4350 50  0000 C CNN
F 1 "CONN_01X02" V 4900 4200 50  0000 C CNN
F 2 "modular_connectors:TerminalBlock_3.5mm_2pol" V 5000 4200 50  0000 C CNN
F 3 "" H 4800 4200 50  0000 C CNN
	1    4800 4200
	0    1    1    0   
$EndComp
NoConn ~ 3600 3400
NoConn ~ 3400 3400
Wire Wire Line
	4850 3450 4850 3400
Wire Wire Line
	4750 3400 4750 4000
Wire Wire Line
	4750 3450 4850 3450
Wire Wire Line
	3500 3400 3500 3600
Wire Wire Line
	3500 3600 4650 3600
Wire Wire Line
	4650 3600 4650 3400
Wire Wire Line
	4550 3400 4550 3800
Wire Wire Line
	4450 3400 4450 3450
Wire Wire Line
	3300 3400 3300 3700
Text Label 3750 3600 2    60   ~ 0
DATA
Connection ~ 4750 3450
Wire Wire Line
	4850 4000 4850 3800
Text Label 4850 3800 0    60   ~ 0
5V
Text Label 4750 3950 0    60   ~ 0
GND
Wire Wire Line
	4850 3800 4550 3800
Wire Wire Line
	4450 3450 4550 3450
Connection ~ 4550 3450
Wire Wire Line
	3300 3700 4750 3700
Connection ~ 4750 3700
$EndSCHEMATC
