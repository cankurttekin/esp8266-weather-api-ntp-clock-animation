# esp8266-weather-api-ntp-clock-animation

It has menu that you can navigate with ir remote to display clock and weather, set alarm and generate random number(if you are someone who can never decide).

Stores wifi credentials in EEPROM. 
If connection fails, it generates access point and simple web server. You can connect to new wifi from that web server instead of hard coding wifi pass and flashing to esp everytime.
Web server shows surrounding wifi networks and rssi values and textbox to enter password.

NTP Clock - i did not have rtc laying around, i used ntp instead.

Weather - By giving latitude and longtitude i get weather info from "api.openweathermap.org"

