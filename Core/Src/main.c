#include "main.h"
#include "cmsis_os.h"
#include <string.h>

uint8_t buffer[15];
uint8_t frameCopy[15];

osEventFlagsId_t uartEventFlag;

volatile uint8_t Frame_Length = 0; // store full frame length

UART_HandleTypeDef huart1;

osMessageQueueId_t uartQueueHandle;

osMutexId_t frameMutex;

uint8_t rxByte;

const osMessageQueueAttr_t uartQueue_attributes = {
  .name = "uartQueue"
};

osThreadId_t defaultTaskHandle;
const osThreadAttr_t defaultTask_attributes = {
  .name = "defaultTask",
  .stack_size = 128 * 4,
  .priority = (osPriority_t) osPriorityNormal,
};

const osThreadAttr_t uartEchoTask_attributes = {
  .name = "uartEchoTask",
  .stack_size = 128 * 4,
  .priority = osPriorityNormal,
};


osThreadId_t buttonTask;
const osThreadAttr_t buttonTask_attributes = {
  .name = "buttonTask",
  .stack_size = 128 * 4,
  .priority = osPriorityNormal,
};

void SystemClock_Config(void);
static void MPU_Config(void);
static void MX_GPIO_Init(void);
static void MX_USART1_UART_Init(void);
void StartDefaultTask(void *argument);
void SbuttonTask(void *argument);


void ProcessFrame(uint8_t *frame, uint8_t len)
{
        uint8_t led_green = frame[2];
        uint8_t led_yellow = frame[3];
        uint8_t led_red = frame[4];

        if (led_green == 1) BSP_LED_On(LED_GREEN);
        else BSP_LED_Off(LED_GREEN);

        if (led_yellow == 1) BSP_LED_On(LED_YELLOW);
        else BSP_LED_Off(LED_YELLOW);

        if (led_red == 1) BSP_LED_On(LED_RED);
        else BSP_LED_Off(LED_RED);

}


void StartUARTEchoTask(void *argument)
{

  for(;;)
  {

	  osEventFlagsWait(uartEventFlag, 0x01, osFlagsWaitAny, osWaitForever);	//-- now commented since mutex is handling in above line

	  osMutexAcquire(frameMutex, osWaitForever);	// the echo didnt worked when i put this line on first line in this for loop, causing deadlock and misplacing of mutex

      HAL_UART_Transmit(&huart1, frameCopy, Frame_Length, HAL_MAX_DELAY);	// echo at the same time regarding the data recieved

      osMutexRelease(frameMutex);



  }
}

int main(void)
{
  MPU_Config();
  HAL_Init();
  SystemClock_Config();
  MX_GPIO_Init();
  MX_USART1_UART_Init();

  BSP_LED_Init(LED_GREEN);
  BSP_LED_Init(LED_YELLOW);
  BSP_LED_Init(LED_RED);
  BSP_PB_Init(BUTTON_USER, BUTTON_MODE_EXTI);

  HAL_UART_Receive_IT(&huart1, &rxByte, 1);

  osKernelInitialize();

  defaultTaskHandle = osThreadNew(StartDefaultTask, NULL, &defaultTask_attributes);
  buttonTask = osThreadNew(SbuttonTask, NULL, &buttonTask_attributes);

  uartQueueHandle = osMessageQueueNew(9, sizeof(uint8_t), &uartQueue_attributes);	// used queue to pass uart rx data without interrupt blocking the process	-- 28-08-2025

  osThreadNew(StartUARTEchoTask, NULL, &uartEchoTask_attributes);

  uartEventFlag = osEventFlagsNew(NULL);	// Used OS event flag to signal amother thread.	-- 30-08-2025

  frameMutex = osMutexNew(NULL);	// now using mutex to lock a specific piece of code execution and not let share it with another thread until released. -- 01-09-2025

  osKernelStart();

  while (1)
  {
  }
}

void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
  if (huart->Instance == USART1)
  {
    osMessageQueuePut(uartQueueHandle, &rxByte, 0, 0);
    HAL_UART_Receive_IT(&huart1, &rxByte, 1);
  }
}

void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  HAL_PWREx_ConfigSupply(PWR_LDO_SUPPLY);
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE3);
  while(!__HAL_PWR_GET_FLAG(PWR_FLAG_VOSRDY)) {}

  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_DIV1;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_NONE;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2
                              |RCC_CLOCKTYPE_D3PCLK1|RCC_CLOCKTYPE_D1PCLK1;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_HSI;
  RCC_ClkInitStruct.SYSCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_HCLK_DIV1;
  RCC_ClkInitStruct.APB3CLKDivider = RCC_APB3_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_APB1_DIV1;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_APB2_DIV1;
  RCC_ClkInitStruct.APB4CLKDivider = RCC_APB4_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_1) != HAL_OK)
  {
    Error_Handler();
  }
}

static void MX_USART1_UART_Init(void)
{
  huart1.Instance = USART1;
  huart1.Init.BaudRate = 115200;
  huart1.Init.WordLength = UART_WORDLENGTH_8B;
  huart1.Init.StopBits = UART_STOPBITS_1;
  huart1.Init.Parity = UART_PARITY_NONE;
  huart1.Init.Mode = UART_MODE_TX_RX;
  huart1.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart1.Init.OverSampling = UART_OVERSAMPLING_16;
  huart1.Init.OneBitSampling = UART_ONE_BIT_SAMPLE_DISABLE;
  huart1.Init.ClockPrescaler = UART_PRESCALER_DIV1;
  huart1.AdvancedInit.AdvFeatureInit = UART_ADVFEATURE_NO_INIT;
  if (HAL_UART_Init(&huart1) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_UARTEx_SetTxFifoThreshold(&huart1, UART_TXFIFO_THRESHOLD_1_8) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_UARTEx_SetRxFifoThreshold(&huart1, UART_RXFIFO_THRESHOLD_1_8) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_UARTEx_DisableFifoMode(&huart1) != HAL_OK)
  {
    Error_Handler();
  }
}

static void MX_GPIO_Init(void)
{
  __HAL_RCC_GPIOC_CLK_ENABLE();
  __HAL_RCC_GPIOH_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
}



void StartDefaultTask(void *argument)
{

	    uint8_t index = 0;
	    uint8_t length = 0;
	    uint8_t byte;

	    for(;;)
	    {
	        if (osMessageQueueGet(uartQueueHandle, &byte, NULL, osWaitForever) == osOK)
	        {

	            buffer[index++] = byte;

	            if (index == 1 && buffer[0] != 0x2B) {	// invalid start byte
	                index = 0;
	            }

	            if (index == 2) {
	                length = buffer[1]; // store length
	                if (length + 3 > 15) {
	                    index = 0; // invalid frame, reset
	                }
	            }

	            // Check if full frame is received
	            if (index >= 2 && index == length)
	            {
	                    ProcessFrame(&buffer[2], length);

	                    osMutexAcquire(frameMutex, osWaitForever);
	                    Frame_Length = index;
	                    memcpy(frameCopy, buffer, index);

	                    osMutexRelease(frameMutex);

	                    osEventFlagsSet(uartEventFlag, 0x01); // -- commented this since above line uses mutex now to not share the execution steps

	                    index = 0; // Reset for next frame
	            }
	        }
	    }


}

void SbuttonTask(void *argument)
{

	    for(;;)
	    {
	    	 if(BSP_PB_GetState(0) == 1){

	    		  BSP_LED_Toggle(LED_RED);
	    	 }
	    }

}

void MPU_Config(void)
{
  MPU_Region_InitTypeDef MPU_InitStruct = {0};

  HAL_MPU_Disable();

  MPU_InitStruct.Enable = MPU_REGION_ENABLE;
  MPU_InitStruct.Number = MPU_REGION_NUMBER0;
  MPU_InitStruct.BaseAddress = 0x0;
  MPU_InitStruct.Size = MPU_REGION_SIZE_4GB;
  MPU_InitStruct.SubRegionDisable = 0x87;
  MPU_InitStruct.TypeExtField = MPU_TEX_LEVEL0;
  MPU_InitStruct.AccessPermission = MPU_REGION_NO_ACCESS;
  MPU_InitStruct.DisableExec = MPU_INSTRUCTION_ACCESS_DISABLE;
  MPU_InitStruct.IsShareable = MPU_ACCESS_SHAREABLE;
  MPU_InitStruct.IsCacheable = MPU_ACCESS_NOT_CACHEABLE;
  MPU_InitStruct.IsBufferable = MPU_ACCESS_NOT_BUFFERABLE;

  HAL_MPU_ConfigRegion(&MPU_InitStruct);
  HAL_MPU_Enable(MPU_PRIVILEGED_DEFAULT);
}

void Error_Handler(void)
{
  __disable_irq();
  while (1)
  {
  }
}

#ifdef  USE_FULL_ASSERT
void assert_failed(uint8_t *file, uint32_t line)
{
}
#endif
