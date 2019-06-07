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
NEXA_AGC1_PULSE = 0.00033
NEXA_AGC2_PULSE = 0.0028
NEXA_RADIO_SILENCE = 0.01068

NEXA_PULSE_SHORT = 0.00024
NEXA_PULSE_LONG = 0.00137

NEXA_COMMAND_BIT_ARRAY_SIZE = 32


# ------------------------------------------------------------------
def sendNexaCommand(sender, group, on_off, recipient):

    if len(str(sender)) is not 26:
        print "Your (invalid) sender was", len(str(sender)), "bits long."
        print
        printUsage()
    if len(str(group)) is not 1:
        print "Your (invalid) group was", len(str(group)), "bits long."
        print
        printUsage()
    if len(str(on_off)) is not 1:
        print "Your (invalid) on_off was", len(str(on_off)), "bits long."
        print
        printUsage()
    if len(str(recipient)) is not 4:
        print "Your (invalid) recipient was", len(str(recipient)), "bits long."
        print
        printUsage()

    # Let's form and transmit the full command:
    full_command = sender + group + on_off + recipient

    # Prepare:
    GPIO.setmode(GPIO.BOARD)
    GPIO.setup(TRANSMIT_PIN, GPIO.OUT)

    # Send command:
    for t in range(REPEAT_COMMAND):
        doNexaManchesterSend(full_command)

    # Disable output to transmitter and clean up:
    exitProgram()
# ------------------------------------------------------------------


# ------------------------------------------------------------------
def doNexaManchesterSend(command):

    # AGC bits:
    transmitHigh(NEXA_AGC1_PULSE)  # AGC 1
    transmitLow(NEXA_AGC2_PULSE)  # AGC 2

    for i in command:

        if i == '0':  # Wire bits 01
            transmitNexaWireBit(0)
            transmitNexaWireBit(1)

        elif i == '1':  # Wire bits 10
            transmitNexaWireBit(1)
            transmitNexaWireBit(0)

        else:
            print "Invalid character", i, "in command! Exiting..."
            exitProgram()

    # Radio silence:
    transmitHigh(NEXA_PULSE_SHORT)  # HIGH space to close the last wire bit
    transmitLow(NEXA_RADIO_SILENCE)
# ------------------------------------------------------------------


# ------------------------------------------------------------------
def transmitNexaWireBit(wire_bit):
    transmitHigh(NEXA_PULSE_SHORT)  # HIGH space

    if wire_bit == 0: transmitLow(NEXA_PULSE_SHORT)  # Wire bit 0
    if wire_bit == 1: transmitLow(NEXA_PULSE_LONG)  # Wire bit 1
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
    print "Usage:"
    print os.path.basename(sys.argv[0]), "[sender] [group] [on_off] [recipient]"
    print
    print "Correct parameter lengths are:"
    print "sender = 26 bits"
    print "group = 1 bit"
    print "on_off = 1 bit"
    print "recipient = 4 bits"
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
