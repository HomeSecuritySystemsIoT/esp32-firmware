In this repo you will have the code for the ESP32, windows app and for the latest,the compiled application ready to use.



First flash your esp32-s3-wroom n16r8 ,then launch the app ,after that you can power your esp32 and get your pc to connect to the esp32's wifi.


The esp32's wifi credentials are:
SSID:myssid
Paswword:Mypassword

If the application doesn't catch any signal from the esp32,relaunch the esp32 and make sure that you are connected to it's wifi.


If everything is succesful,you will able to type to the standard input a message whose size is smaller than 1024 bytes,this message will be sent from the pc to the esp32 by UDP,you also have these following commands:
/tcp/{message} :   Changes the protocol to tcp. The computer sends first "/tcp/" to the esp32 by UDP and sends the message by TCP ,the esp32 intercepts this message and starts to intercept the message that was sent using TCP.
/led/{colour}  :   Changes/Sets the colour of the led the colour,the colour must be in hexadecimal form (handles both uppercase and lowercase letters)
/camera/       :   UDP camera ,doesn't work
/camerb/       :   TCP camera,works