GPS Tracker with SIM800 and NEO-6M

This repository contains the code for a GPS tracker system built using an Arduino, SIM800 module for GSM communication, and NEO-6M module for GPS data. The system allows tracking and sending location information via SMS messages.

Features

Real-time GPS tracking.
Send location information via SMS.
Store and manage phone numbers for communication.
Password-protected access to system functionalities.
EEPROM storage for persistent data.
Requirements

Arduino board (e.g., Arduino Uno).
SIM800 module for GSM communication.
NEO-6M module for GPS data.
SoftwareSerial library for serial communication.
TinyGPS++ library for parsing GPS data.
AltSoftSerial library for additional serial communication.
Installation

Connect the SIM800 module to the Arduino board.
Connect the NEO-6M module to the Arduino board.
Install required libraries: SoftwareSerial, TinyGPS++, AltSoftSerial.
Upload the provided code to the Arduino board.
Usage

Configure the SIM800 module and insert a valid SIM card.
Power up the system and wait for GPS fix.
Send SMS commands to interact with the system:
PASSWORD: Authenticate and perform actions.
loc: Request current location.
numbers: Retrieve stored phone numbers.
delete [number]: Delete a stored phone number.
deleteall: Delete all stored phone numbers.
whoami: Get information about the GPS module.
Contributing

Contributions to improve this project are welcome! Feel free to fork the repository and submit pull requests.

License

This project is licensed under the MIT License.

Acknowledgements

Thanks to contributors of the libraries used in this project.
Special thanks to Morteza Abdollahi for the initial idea and inspiration.
Contact

For questions or support, please contact Morteza abdollahi.
