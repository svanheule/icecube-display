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
EELAYER 25 0
EELAYER END
$Descr A4 11693 8268
encoding utf-8
Sheet 1 1
Title "OctoWS2811 splitter board"
Date ""
Rev "1"
Comp "Universiteit Gent"
Comment1 "Vakgroep fysica en sterrenkunde"
Comment2 "Sander Vanheule"
Comment3 ""
Comment4 ""
$EndDescr
$Comp
L 4P4C P4
U 1 1 56E2F5E9
P 3250 4400
F 0 "P4" H 3500 4900 60  0000 C CNN
F 1 "4P4C" H 3100 4900 60  0000 C CNN
F 2 "modular_connectors:4P4C_horizontal" H 3250 4400 60  0001 C CNN
F 3 "" H 3250 4400 60  0000 C CNN
	1    3250 4400
	0    1    1    0   
$EndComp
NoConn ~ 2650 4550
NoConn ~ 2650 5300
$Comp
L 8P8C P1
U 1 1 56E304AA
P 1350 3950
F 0 "P1" H 1650 4450 60  0000 C CNN
F 1 "8P8C" H 1175 4450 60  0000 C CNN
F 2 "modular_connectors:8P8C_horizontal" H 1325 3950 60  0001 C CNN
F 3 "" H 1325 3950 60  0000 C CNN
	1    1350 3950
	0    -1   1    0   
$EndComp
Wire Wire Line
	1950 4050 2600 4050
Wire Wire Line
	2600 4050 2600 4250
Wire Wire Line
	2600 4250 2650 4250
Wire Wire Line
	1950 4350 1950 5200
Wire Wire Line
	1950 5200 2650 5200
Wire Wire Line
	2050 4250 1950 4250
Wire Wire Line
	2050 5000 2650 5000
NoConn ~ 2650 5100
$Comp
L 4P4C P5
U 1 1 56E30AEF
P 3250 5150
F 0 "P5" H 3500 5650 60  0000 C CNN
F 1 "4P4C" H 3100 5650 60  0000 C CNN
F 2 "modular_connectors:4P4C_horizontal" H 3250 5150 60  0001 C CNN
F 3 "" H 3250 5150 60  0000 C CNN
	1    3250 5150
	0    1    1    0   
$EndComp
NoConn ~ 2650 4350
$Comp
L 4P4C P3
U 1 1 56E30B96
P 3250 3650
F 0 "P3" H 3500 4150 60  0000 C CNN
F 1 "4P4C" H 3100 4150 60  0000 C CNN
F 2 "modular_connectors:4P4C_horizontal" H 3250 3650 60  0001 C CNN
F 3 "" H 3250 3650 60  0000 C CNN
	1    3250 3650
	0    1    1    0   
$EndComp
$Comp
L 4P4C P2
U 1 1 56E30BD8
P 3250 2900
F 0 "P2" H 3500 3400 60  0000 C CNN
F 1 "4P4C" H 3100 3400 60  0000 C CNN
F 2 "modular_connectors:4P4C_horizontal" H 3250 2900 60  0001 C CNN
F 3 "" H 3250 2900 60  0000 C CNN
	1    3250 2900
	0    1    1    0   
$EndComp
NoConn ~ 2650 3800
NoConn ~ 2650 3600
NoConn ~ 2650 3050
NoConn ~ 2650 2850
Wire Wire Line
	2650 3500 2200 3500
Wire Wire Line
	2200 3500 2200 3850
Wire Wire Line
	2200 3850 1950 3850
Wire Wire Line
	2650 2750 1950 2750
Wire Wire Line
	1950 2750 1950 3650
Wire Wire Line
	1950 3750 2050 3750
Wire Wire Line
	2050 3750 2050 2950
Wire Wire Line
	2050 2950 2650 2950
Wire Wire Line
	2050 4250 2050 5000
Wire Wire Line
	1950 4150 2300 4150
Wire Wire Line
	2300 4150 2300 3700
Wire Wire Line
	2300 3700 2650 3700
Wire Wire Line
	1950 3950 2500 3950
Wire Wire Line
	2500 3950 2500 4450
Wire Wire Line
	2500 4450 2650 4450
Text Label 2500 2950 0    60   ~ 0
D1
Text Label 2500 3700 0    60   ~ 0
D2
Text Label 2500 4450 0    60   ~ 0
D3
Text Label 2450 5200 0    60   ~ 0
D4
$EndSCHEMATC
