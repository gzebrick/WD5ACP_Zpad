Guy Zebrick / WD5ACP

<img width="600" height="972" alt="zpad case small " src="https://github.com/user-attachments/assets/b1d4a9ca-8aa3-453b-a101-c6a296c40542" />

This program is written for a low cost color touch screen display known as a "CYD" (Cheap Yellow Device).  
Here the board I used, Includes the case too!
https://a.co/d/00XEqkp3
No other hardware needed! Your radio must be connected to the same LAN as the WiFi connection for the ZPAD, and since the ZPAD logs in via the network, 
you can't use it and any other network program at the same time (but it's easy to log off / log back in with the ZPAD!

There are three display screens, selectable by touching the header's left or right side:  

Configuration Buttons
Radio Command Buttons
Status display

The background color of the display may be changed by repeatedly the "-COLOR-" section on the display header. 
3 colors are available that mimic the Kenwood radio display.
The CYD tri-color LED is also used to indicate status:

WHITE = Startup
MAGENTA = WiFi Configuration access portal open
CYAN = Configured and Connected to local WiFi network
GREEN = Connected to local WiFi network and also connected to HOST
YELLOW = COnfigured but NOT Connected to local WiFi network

The ZPAD lets the user configure up to 10 touch screen buttons to issue a string of commands to a Kenwood TS-890 Amateur Radio Transciever.
The commands are based on the published PC COMMAND document available from Kenwood.
The ZPAD connects to the radio using the LAN/WiFi network. The Radios' IP address, Port, and the administrator User Name and Password must be correct
Once connected, the ZPAD polls the radio every few seconds to maintain a connection.

The WiFi network configuration of the ZPAD uses the built-in access portal and local WiFi network to first discover and select the correct WiFi SSID desired.
Once selected and configured, the ZPAD will reboot and connect to the local network. Once connected, a configuration screen is available from the ZPAD using the 
ZPAD's IP address as shown on the top line of the display, or using the name followed by .local. For example: http://zpad.local

<img width="710" height="355" alt="image" src="https://github.com/user-attachments/assets/ee346d2d-6cef-4f2e-927e-31dad16d9463" />


<img width="710" height="355" alt="ZpadWebPage" src="https://github.com/user-attachments/assets/c9c277b7-6adb-4518-9a45-8bdb30a24bae" />

The Radio Host configuration as well as the 10 button label and commands are configured using the ZPAD configuration screen and are saved in non-volatile memory.
NOTE: The button labels support a maximum of 12 characters AND the radio PC commands MUST include the special end-of-line ';" after each command.
Multiple commands may be included on one line, but NO SPACES ALLOWED!

Additional settings and functions are available on the ZPAD's configuration menu:
LOG OUT / LOG IN for manual host connections

MAN HOST / AUTO HOST for automatic login to host on bootup, or manual connection required. If Auto, the command buttons screen will be displayed. 
Select AUTO only after Host configuration is complete and working!
If Auto, the command buttons screen will be displayed on startup.  Setting retained in non-volatile memory

DIMMWE/BRIGHTER steps through the available brightness levels. Setting retained in non-volatile memory.

DIS WIFI/ENA WIFI connects and disconnects to the local WiFi SSID network

CLR WIFI  This button, when held down for at least 2 seconds then tap,  will ERASE ALL WiFi configuration and will require connection to the ZPAD's temporary access portal.

RECAL This button, when held down for at least 2 seconds then tap, re-runs the screen calibration, and then reboots same as the device's reboot button.

--------------------------

BASICS getting started below... see youtube for detailed how-too videos on using CYD and Visual Studio...

Purchase a 2.8" CYD and plug it into a USB port on your computer. A pre-loaded demo program may start, that's okay... Select the CODE button to download the entire folder for this application from github onto your local PC, Put it somewhere you can find it.

1, Download and install free Visual Studio programming tool...
https://visualstudio.microsoft.com/
2. Open Visual Studio - go to Views, select Extensions, install platformIO extension
3. Select the new platform IO extension, and select PIO Quick Access > Home > Open
4. In the Welcome to PlatformIO screen, select Open Project and select the folder you download from github
5. The download app folder and subfolders will appear in the Explorer file tree. Select src (source code) then main.cpp (c-plus-plus)... that's the program!
6. At the bottom of the screen you should see an check box (compile), and a right arrow (compile + download). Also a COM port should be indicated. 
The COM port needs point to your CYD that's plugged in. click on the right-arrow to compile the code and download it into the CYD. If there are no errors, the program will start! 

7. First time running the app you may need to calibrate the touch points (4 touches) and you'll need to find the WiFi access point since it doesn't know your WiFi network yet. connect to it, and go to 192.168.4.1
8. Pick your WiFi network for the ZPAD (should be on the same network as your host radio). Let it reboot.
9. After the reboot, you can now connect to the configuration screen at zpad.local - be sure to setup your host radio's IP, port (60000), user name and password BEFORE you go to auto-connect
10 Program some buttons, save, and you're rollin! Enjoy!




