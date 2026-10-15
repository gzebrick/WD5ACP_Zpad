Guy Zebrick / WD5ACP

This program is written for a low cost color touch screen display known as a "CYD" (Cheap Yellow Device).
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

The Radio Host configuration as well as the 10 button label and commands are configured using the ZPAD configuration screen and are saved in non-volatile memory.
NOTE: The button labels support a maximum of 10 characters AND the radio PC commands MUST include the special end-of-line ';" after each command.
Multiple commands may be included on one line, but NO SPACES ALLOWED!

Additional settings and functions are available on the ZPAD's configuration menu:
LOG OUT / LOG IN for manual host connections
MAN HOST / AUTO HOST for automatic login to host on bootup, or manual connection required. If Auto, the command buttons screen will be displayed. Setting retained in non-volatile memory
DIMMWE/BRIGHTER steps through the available brightness levels. Setting retained in non-volatile memory.
DIS WIFI/ENA WIFI connects and disconnects to the local WiFi SSID network
CLR WIFI  This button, when held down for at least 2 seconds, will ERASE ALL WiFi configuration and will require connection to the ZPAD's temporary access portal
REBOOT Same as the device's reboot button.




