Divide 16MB of flash memory: One partition for the application, one for Over-the-Air (OTA) updates, and a large one for the SPIFFS filesyste to store the face data.

partitions.csv

```
# ESP-IDF Partition Table
# Name,   Type, SubType, Offset,  Size,   Flags
nvs,      data, nvs,     ,        24K,
otadata,  data, ota,     ,        8K,
app0,     app,  factory, ,        3M,
app1,     app,  ota_0,   ,        3M,
storage,  data, spiffs,  ,        9M,
```

Defines partitions for Non-Volatile Storage (nvs), OTA data, and two app slots (app0, app1).
Defines a storage partition with the subtype spiffs. By leaving the size field blank, all remaining flash memory (~10MB) is used, enough for big face database.
DONT FORGET: 

`idf.py menuconfig` Partition Table ---> Partition Table (Custom partition table CSV)--->(X) Custom partition table CSV -> Save (Y).

`idf.py menuconfig` Serial Flasher Config ---> Flash Size (16MB) or the size detected in flash_id -> Save (Y).

Enable the websocket server
`idf.py menuconfig` --> HTTP Server --> WebSocket server support (*)

`storage_manager.c/h` hold all the file system necessary functions.
`app_diagnostics.c/h` can be enabled/disabled in config.h

The websocket server (wesocket_server.c) has been updated to the latest version provided ESP-IDF v5.4.1. Be careful with the shared libraries versions.

IMPORTANT: Every time you play with libraries, components, CMakeLists.txt etc., it is a good practice to check the `idf.py menuconfig` if the settings are still there. For example, the websocket parameter is needed to be re-initialized...



ERRORS: 
../components/who_task/who_task_state.cpp:66:37: error: 'TaskStatus_t' {aka 'struct xTASK_STATUS'} has no member named 'xCoreID'
   66 |                task_status_array[i].xCoreID,
      |                                     
	  Commented //task_status_array[i].xCoreID,
	  
components/who_task/who_task_state.cpp:63:71: error: format '%llu' expects a matching 'long long unsigned int' argument [-Werror=format=]
   63 |         printf("%-15s | %-8x | %-9s | %-8u | %-11lu | %-11llu | %-12llu |\n",
      |                                                                 ~~~~~~^
      |                                                                       |
      |                                                                       long long unsigned int
cc1plus.exe: some warnings being treated as errors
[1155/1256] Building CXX ob...r/human_face_detect.cpp.obj
ninja: build stopped: subcommand failed.
HINT: The issue is better to resolve by replacing format specifiers to 'PRI'-family macros (include <inttypes.h> header file).	  
added #include <inttypes.h>
changed the printf, look for // George

