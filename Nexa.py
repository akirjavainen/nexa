#!/usr/bin/python

"""
* Nexa 433.92MHz appliances
*
* Code by Antti Kirjavainen (antti.kirjavainen [_at_] gmail.com)
*
* This is a Python implementation of the Nexa protocol, for
* the Raspberry Pi. Plug your transmitter to BOARD PIN 16 (BCM/GPIO23).
*
* HOW TO USE
* ./Nexa.py [sender] [group] [on_off] [recipient]
*
* More info on the protocol in Nexa.ino here:
* https://github.com/akirjavainen/nexa
*
"""

import time
import sys
import os
import RPi.GPIO as GPIO


TRANSMIT_PIN = 16  # BCM PIN 23 (GPIO23, BOARD PIN 16)
REPEAT_COMMAND = 8


# Microseconds (us) converted to seconds for time.sleep() function:
NEXA_PULSE_HIGH_SPACE = 0.00031
NEXA_PULSE_WIRE_0 = 0.0002
NEXA_PULSE_WIRE_1 = 0.001195

NEXA_PULSE_AGC1 = NEXA_PULSE_HIGH_SPACE
NEXA_PULSE_AGC2 = 0.00244
NEXA_RADIO_SILENCE = 0.008844

NEXA_COMMAND_BIT_ARRAY_SIZE = 32


# ------------------------------------------------------------------
def sendNexaCommand(sender, group, on_off, recipient):

    if len(str(sender)) is not 26:
        print("Your (invalid) sender was", len(str(sender)), "bits long.")
        print("Do you need to add leading zeroes?")
        print
        printUsage()
    if len(str(group)) is not 1:
        print("Your (invalid) group was", len(str(group)), "bits long.")
        print
        printUsage()
    if len(str(on_off)) is not 1:
        print("Your (invalid) on_off was", len(str(on_off)), "bits long.")
        print
        printUsage()
    if len(str(recipient)) is not 4:
        print("Your (invalid) recipient was", len(str(recipient)), "bits long.")
        print
        printUsage()

    # Let's form and transmit the full command:
    full_command = sender + group + on_off + recipient

    # Prepare:
    GPIO.setmode(GPIO.BOARD)
    GPIO.setup(TRANSMIT_PIN, GPIO.OUT)

    # Transmit radio silence before the first command in sequence:
    transmitLow(NEXA_RADIO_SILENCE)

    # Send command:
    for t in range(REPEAT_COMMAND):
        doNexaManchesterSend(full_command)

    # Disable output to transmitter and clean up:
    exitProgram()
# ------------------------------------------------------------------


# ------------------------------------------------------------------
def doNexaManchesterSend(command):

    # AGC bits:
    transmitHigh(NEXA_PULSE_AGC1)  # AGC 1
    transmitLow(NEXA_PULSE_AGC2)  # AGC 2

    for i in command:

        if i == '0':  # Wire bits 01
            transmitNexaWireBit(0)
            transmitNexaWireBit(1)

        elif i == '1':  # Wire bits 10
            transmitNexaWireBit(1)
            transmitNexaWireBit(0)

        else:
            print("Invalid character", i, "in command! Exiting...")
            exitProgram()

    # Radio silence:
    transmitHigh(NEXA_PULSE_HIGH_SPACE)  # HIGH space to close the last wire bit
    transmitLow(NEXA_RADIO_SILENCE)
# ------------------------------------------------------------------


# ------------------------------------------------------------------
def transmitNexaWireBit(wire_bit):
    transmitHigh(NEXA_PULSE_HIGH_SPACE)  # HIGH space

    if wire_bit == 0: transmitLow(NEXA_PULSE_WIRE_0)  # Wire bit 0
    if wire_bit == 1: transmitLow(NEXA_PULSE_WIRE_1)  # Wire bit 1
# ------------------------------------------------------------------


# ------------------------------------------------------------------
def transmitHigh(delay):
    GPIO.output(TRANSMIT_PIN, GPIO.HIGH)
    time.sleep(delay)
# ------------------------------------------------------------------


# ------------------------------------------------------------------
def transmitLow(delay):
    GPIO.output(TRANSMIT_PIN, GPIO.LOW)
    time.sleep(delay)
# ------------------------------------------------------------------


# ------------------------------------------------------------------
def printUsage():
    print("Usage:")
    print(os.path.basename(sys.argv[0]), "[sender] [group] [on_off] [recipient]")
    print
    print("Correct parameter lengths are:")
    print("sender = 26 bits (device ID)")
    print("group = 1 bit (all ON/OFF or single button?)")
    print("on_off = 1 bit")
    print("recipient = 4 bits (example: button ID on a remote)")
    print
    exit()
# ------------------------------------------------------------------


# ------------------------------------------------------------------
def exitProgram():
    # Disable output to transmitter and clean up:
    GPIO.output(TRANSMIT_PIN, GPIO.LOW)
    GPIO.cleanup()
    exit()
# ------------------------------------------------------------------


# ------------------------------------------------------------------
# Main program:
# ------------------------------------------------------------------
if len(sys.argv) < 5:
    printUsage()

sendNexaCommand(sys.argv[1], sys.argv[2], sys.argv[3], sys.argv[4])
