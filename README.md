# CAN simulator for Ford Focus mk3 IPC
This tool developed to simulating CAN exchange with Focus instrument cluster via 
cheap elm327 compatible adapter.

## Connection scheme
```

+-----------+----+                     +------------+-------+
|           | 3  |---------------------| 2 MS-CAN + |       |
|  OBDII    +----+                     +------------+       |
|           | 11 |---------------------| 1 MS-CAN - |       | <OPTIONAL>
|  elm\els  +----+                     +------------+  IPC  +------------+
|  adapter  |         +----------------|   10 GND   |       |   6 GND    |------------+
|           |         |                +------------+       +------------+            |
+-----------+         |         +------|    3 PWR   |       | 8 MFD BTNs |--+         |
                      |         |      +------------+-------+------------+  |         |
                      |         |                                           |         |
                      |         |                                           |         |
                    +-|-+     +-|-+                                       +---+     +---+
                    | - |     | + |                                       | 1 |     | 2 |
                    +---+-----+---+                                       +---+-----+---+
                    |             |                                       |  STEERING   |
                    |  PSU/BATT   |                                       |    WHEEL    |
                    |     12V     |                                       | BUTTONS(OK) |
                    +-------------+                                       +-------------+
            
```

## It allows emulating:
- Speed
- Engine RPM and temperature
- Fuel level (not perfectly yet)
- Outside temperature
- Lights and turns
- Doors status
- Cruise and speed limit (include ACC)
- Park brake status
