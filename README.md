# STM32 FreeRTOS UART Frame Handler
A FreeRTOS-based UART communication system on NUCLEO-STM32H743ZI2 Board with frame parsing, mutex, queues, and event flags.

# Interrupt-based UART RX
# FreeRTOS tasks (frame parser, echo task, button task)
# Mutex protection for shared buffers
# Event flags for task synchronization
# LED control via UART frame

-----------------------------------------------------------------------------------------------------------------------------------------------------------------------

/* Hardware Requirements */
i> NUCLEO-STM32H743ZI2 Board
ii> USB-UART Bridge TTL( I have CP210x based TTL ).



/* Software Requirements */
i> STM32CubeIDE
ii> FreeRTOS (CMSIS-RTOS v2 API)



/* Build & Flash Instructions */
Clone the repo
Open project in STM32CubeIDE
Build & flash with ST-Link
Open serial terminal at 115200 baud



Frame Format : 
Byte 0: Start byte (0x2B)
Byte 1: Length (N)
Byte 2: Green LED state
Byte 3: Yellow LED state
Byte 4: Red LED state
Bytes 5..N: Payload
