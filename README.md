## 📱 Smart Home System 🏩

🧑‍💻 Smart home system by using two AVR atmega32 MCU (master and slave) communicate with each other
With SPI Protocol, Master MCU takes user Order by using UART through 📶Bluetooth module.

💾 And store system setting in EEPROM, So when the system starts operating in the last setting.

💡 The system has two mode RGB mode and smart home control mode, User can change to any mode easily, user can also
reset the system to the default setting at any time.

🤳🏻 The smart home mode has many features that can be controlled by UART MSG through a Bluetooth module.

## Smart Home features:‎
### Master

    ⭐ RGB lighting Control has two mode sets the color mode and showroom ‎mode.‎
    ⭐ Control rooms lighting turn (on/off) LEDs, and color LEDs (on/off).‎
    ⭐ Turn (on/off) two TVs using relays.‎
    ⭐ Write String on LCD.‎

### Slave

    ⭐ Control the Windows position using a servo motor.‎
    ⭐ Open/Close the Garage door by using a DC motor and two proximity ‎sensors to determine door status.‎
    ⭐ Open/Close the Curtain using a DC motor and two proximity sensors.‎
    ⭐ Control Fan (on/off) and its Speed (10 - 100)%.‎
    ⭐ Control Air condition degree (18 - 28) C and print Temperature At LCD.‎
    ⭐ Open/Close Outdoor LEDs.‎
    ⭐ Temperature control mode controls the fan and air conditioner by using ‎temperature sensors to read home temperature and control the operation of ‎the fan and air conditioner without user intervention.‎
    ⭐ LDR control mode control (on/off) of Outdoor LEDs Automatically.‎
    ⭐ The user should turn auto control mode to have control over the instrument ‎again.‎


  



##  👷Implementations:‎
‎✅ ‎‎Implement MCAL layer (a timer, ADC, SPI, DIO, EEPROM, EX interrupt, ‎UART …)‎

✅ ‎Implement HAL layer Drivers (Motor, Servo, Servo, Ultrasonic, LCD, ‎Sensors,…..)‎

‎✅ ‎Implement a service layer for timer, UART, and SPI.‎

‎✅ Service of SPI Sends and Receives Frames through SPI Between master and ‎slave using Interrupts  (send and receive data using Queue).‎

‎✅ Service of UART Sends and Receives string and frame through UART (using ‎Queues and Stacks).‎

‎✅ ‎Implement a Temperature filter for the temp sensor.‎

✅ ‎Control Motor Speed by PWM.‎

‎✅ Make a Schedule for Tasks.‎

‎✅ ‎Implement Application for master and slave.‎ ‎
