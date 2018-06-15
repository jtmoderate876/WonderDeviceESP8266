# WonderDeviceESP8266


The device can be reset using #   DoubleResetDetect by Jens-Christian Skibakk

You can then configure via wifimanager by connecting to the ESPXXXXXX device and setting your SSID/PWD/ETC.

Raspberry pi's set up for Wonderware ITME "IoTView" per the following:

```
#install apache on pi:
sudo apt-get install apache2
#sudo systemctl disable apache2
#sudo systemctl stop apache2
#sudo systemctl restart apache2
sudo systemctl start apache2
sudo systemctl enable apache2
#skip the old service way which still works:
#sudo service apache2 start
#sudo service apache2 restart
#enable cgi
sudo a2enmod cgi
sudo cp /etc/apache2/mods-available/cgi.load /etc/apache2/mods-enabled/
```

Now from a windows pc that has Wonderware ITME installed copy the remote agent files from PC to removable drive:
copy "C:\Program Files (x86)\Wonderware\InTouch Machine Edition v8.1\Redist\IoTView\Linux\arm-gnueabihf-2.13-6.0.17\*.*"
"d:\iotview\*.*"

```
# now on raspberry create a directory and copy those:

md /home/pi/iotview
cd /home/pi/iotview
cp /media/pi/LexarNTFS/iotview /home/pi/iotview
#or sudo pcmanfm
sudo ./install.sh -a -i

#copy iotview cgi to apache cgi-bin
sudo cp webaddon/CGI/WebCGIProc /usr/lib/cgi-bin/WebCGIProc
sudo systemctl disable remote-agent
sudo service remote-agent start
  144  ls
  145  cd iotview
  146  ls
  147  sudo cp webaddon/CGI/WebCGIProc /usr/lib/cgi-bin/WebCGIProc
  148  sudo chmod a+x /usr/lib/cgi-bin/WebCGIProc
  149  sudo ln -s MA /var/www/MA
  150  service apache2 restart
  151  nano /var/www/MA/sma/config.js
  152  sudo nano /var/www/MA/sma/config.js

```

If you already have a raspberry pi (2, 3 or maybe even a zero - I haven't tried IoTView with the slower processor in pizerow) then you can skip buying the pi stuff otherwise you may look at parts such as these:
Raspberry Pi Complete:
http://a.co/gGVRZKB

You can run any MQTT Broker (mosquitto, node-red, or perhaps others)

On a PC install the Wonderware ITME Development environment (demo mode available from your Wonderware guy or downloadable from
wdn.wonderware.com with a support account).
We use it to create the HMI and download it to the pi.

To skip the pi and use just the PC you can install an mqtt broker on your PC (again, either mosquitto or several others are available for PC) and use it with Wonderware InTouch or ITME or System Platform.

The WonderDevicESP8266 Sensor:
![Wiring up WonderDeviceESP8266.](https://github.com/jtmoderate876/WonderDeviceESP8266/blob/master/WonderDeviceESP8266.PNG)


We went with an ESP8266 12E microcontroller (also known as NodeMCU), 2 temperature probes and an ultrasonic distance sensor.

These are programmed with the arduino IDE - which can be installed on either a PC or your Raspberry PI.
If you use Arduino  on pi, don't use the standard installation (sudo apt-get install arudino) because that is an old version that doesn't support other boards like the ESP8266. Instead install via:

```
# on pi in Chromium browser navigate to https://www.arduino.cc/en/Main/Software and download for linux arm
# extract in place by right clicking using:
pcmanfm
cd /home/pi/Downloads/arduino-1.8.5
sudo mv /home/pi/Downloads/arduino-1.8.5 /opt/arduino-1.8.5
cd /opt/arduino-1.8.5
./install.sh

# now you need to add esp8266/nodemcu support to arduino ide
# run arduino (Start>Programming>Arduino) and menu: edit > preferences ...paste the following in "Additional Boards Manager":
http://arduino.esp8266.com/stable/package_esp8266com_index.json
# restart arduino ide and Tools > Board > Boards Manager and "Install" esp8266
# >Sketch>Include Libraries>Manage Libraries ... add:
#   PubSubClient from Nick O'Leary
#   OneWire by  Jim Studt, etc.
#   DallasTemperature by Miles Burton, etc.
#   WifiManager by Tzapu
#   ArduinoJson by Benoit Blanchon
#   DoubleResetDetect by Jens-Christian Skibakk
#   
```

The untrasonic sensor is called HC-SR04 BUT...
Rather than having to create / wire a voltage divider to know the 5V down to 3.3v it is much better to find an HC-SR04 that works with 3V.
Our first were the common, 5VDC type - which required us to create a voltage divider using several additional resistors - which was a pain.

Get 3v-5v version (a little harder to find - sometimes called  "wide voltage range" or "Upgraded voltage") which match the inputs on the ESP8266 microcontroller and doesn't require the extra two resistors and wiring.

Unfortunately they are all called HC-SR04 so you need to check that it is 3V tolerant.


I've created a list of materials on Amazon
WonderDeviceESP8266 example BOM: http://a.co/bolUVeg


Anything you have already laying around (like a MicroUSB cable, USB Power, wires, etc.) you wouldn't need to buy.
If you are patient with shipping then you can find this stuff cheaper (sometimes MUCH CHEAPER) at https://www.aliexpress.com/

A wiring diagram (the PNG file) can be found here too:
  https://github.com/jtmoderate876/WonderDeviceESP8266

Misc. supplies such as screw driver, wire cutter, hot glue gun, soldering iron and solder (the temperature probes have flimsy wires that can't be pressed into the breadboard so we cut some of those Dupont wires and soldered them to it), band-aids :), etc.

For the time being I have a raspberry pi image with a sample application on the desktop for v8.1+ Wonderware InTouch Machine Edition IoTView / InduSoft WebStudio IoTView (pi/raspberry):
https://1drv.ms/f/s!AvJzcfwnK1a3g61JaO3n66KUUUTJbw
