# Controlling Nexa 433.92MHz appliances
This code is command-compatible with the HomeEasy library, so you can capture your remote either with that or use an oscillator. You can capture your remotes and other Nexa devices with this code (convert decimal to binary as needed):

https://playground.arduino.cc/Code/HomeEasy?action=sourceblock&num=3


# How to use
Capture the binary commands from your Nexa devices with HomeEasy and copy paste binary bits to the sendNexaCommand() function as follows:

sender (string) = first 26 bits
group (int) = 1 bit
on_off (int) = 1 bit
recipient (string) = last 4 bits

= a total of 32 bits (64 wire bits)
