#ifndef CONFIG_H
#define CONFIG_H

/* Global configuration, SERIAL OUTPUT ON/OFF */
#define ENABLE_SERIAL_OUTPUT true 

// Macro to control serial output 
#if ENABLE_SERIAL_OUTPUT 
  #define SERIAL_PRINT(x) Serial.print(x) 
  #define SERIAL_PRINTLN(x) Serial.println(x) 
#else 
  #define SERIAL_PRINT(x) 
  #define SERIAL_PRINTLN(x)
#endif

#endif // CONFIG_H
