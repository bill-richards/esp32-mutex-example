#include <Arduino.h>
#include <random>
#include <functional>

/********
 * Forward declarations of functions
 *********************************************************************************/
int IncrementBy(int step);
int MultiplyBy(int step);
int GetAndClearCounterValue();
int GetCounterValue();

int GetRandomNumber();

void CounterIncrementTask(void * pvParameter);
void CounterMultiplyTask(void * pvParameter);
void CounterResetTask(void * pvParameter);
void DisplayTask(void * pvParameter);
/*********************************************************************************/

enum
{
  TASK_PRIO_LOWEST  = 0, // Idle task, should not be used
  TASK_PRIO_LOW     = 1, // Loop
  TASK_PRIO_NORMAL  = 2, // Timer Task, Callback Task
  TASK_PRIO_HIGH    = 3, 
  TASK_PRIO_HIGHEST = 4,
};


/********
 * Global variables
 *********************************************************************************/
SemaphoreHandle_t _xCounterSemaphore = xSemaphoreCreateMutex();
SemaphoreHandle_t _xRandomSemaphore = xSemaphoreCreateMutex();
 
QueueHandle_t _xMessageQueue = xQueueCreate(512, 64); // the Queue can hold 512 entries at a size of 64 bytes per entry

volatile int _voatileCounter = 0;

std::default_random_engine generator;
std::uniform_int_distribution<int> distribution(10, 1000);
auto random_number = std::bind (distribution, generator);
/*********************************************************************************/

// As it says, initializes the hardware serial port
void InitializeSerialPort()
{
  Serial.begin(115200);
  time_t timeout = millis();
  while (!Serial)
  {
    if ((millis() - timeout) < 5000)
      delay(100);
    else
      break;
  }
}

void setup() 
{
  InitializeSerialPort();
  
  // Create the tasks
  xTaskCreate(DisplayTask, "display-task", 1024, (void*)&_xMessageQueue, TASK_PRIO_NORMAL, NULL);
  xTaskCreate(CounterResetTask, "reset-task", 1024, (void*)&_xMessageQueue, TASK_PRIO_NORMAL, NULL);
  xTaskCreate(CounterIncrementTask, "increment-task", 1024, (void*)&_xMessageQueue, TASK_PRIO_HIGH, NULL);
  xTaskCreate(CounterMultiplyTask, "multiply-task", 1024, (void*)&_xMessageQueue, TASK_PRIO_HIGH, NULL);
}

void loop() { }

void CounterMultiplyTask(void * pvParameter)
{
  const QueueHandle_t message_queue = *(QueueHandle_t*)pvParameter;
  TickType_t last_wake_time = xTaskGetTickCount();
  
  std::default_random_engine localGenerator;
  std::uniform_int_distribution<int> localDistribution(2, 6);
  auto randomNumberBetween2And6 = std::bind(localDistribution, localGenerator);
  
  while(true)
  {
    char message[64] = {0};
    auto sleep_period = pdMS_TO_TICKS(GetRandomNumber());
    vTaskDelayUntil(&last_wake_time, sleep_period);

    auto step = randomNumberBetween2And6();
    int value = MultiplyBy(step);
    snprintf(message, 64, "Multiplied by %d to ... %d\n", step, value);
    xQueueSendToBack(message_queue, &message, 0);
  }
}

void CounterIncrementTask(void * pvParameter)
{
  const QueueHandle_t message_queue = *(QueueHandle_t*)pvParameter;
  TickType_t last_wake_time = xTaskGetTickCount();
  
  std::default_random_engine localGenerator;
  std::uniform_int_distribution<int> localDistribution(1, 5);
  auto randomNumberBetween1And5 = std::bind(localDistribution, localGenerator);

  while(true)
  {
    vTaskDelayUntil(&last_wake_time, pdMS_TO_TICKS(GetRandomNumber()));

    char message[64] = {0};
    int step = randomNumberBetween1And5();
    int value = IncrementBy(step);
    snprintf(message, 64, "Incremented by %d to ... %d\n", step, value);
    xQueueSendToBack(message_queue, &message, 0);
  }
}

void CounterResetTask(void * pvParameter)
{
  const QueueHandle_t message_queue = *(QueueHandle_t*)pvParameter;
  TickType_t last_wake_time = xTaskGetTickCount();

  std::default_random_engine localGenerator;
  std::uniform_int_distribution<int> localDistribution(5, 15);
  auto randomNumberBetween5And15 = std::bind(localDistribution, localGenerator);

  while(true)
  {
    vTaskDelayUntil(&last_wake_time, pdMS_TO_TICKS(randomNumberBetween5And15()*1000));

    char message[64] = {0};
    snprintf(message, 64, "Resetting from %d to ... %d\n", GetAndClearCounterValue(), GetCounterValue());
    xQueueSendToBack(message_queue, &message, 0);
  }
}

void DisplayTask(void * pvParameter)
{
  const QueueHandle_t message_queue = *(QueueHandle_t*)pvParameter;
  TickType_t wake_time = xTaskGetTickCount();
  char message[64] = {0};

  while(true)
  {
    while(xQueueReceive(message_queue, &message, 0) == pdPASS)
      Serial.print(message);

    Serial.println();
    vTaskDelayUntil(&wake_time, pdMS_TO_TICKS(GetRandomNumber()));
  }
}

int GetCounterValue()
{
  int returnValue = 0;

  if( xSemaphoreTake( _xCounterSemaphore, ( TickType_t ) 100 ) == pdTRUE )
  {
    returnValue = _voatileCounter;
    xSemaphoreGive( _xCounterSemaphore );
  }

  return returnValue;
}

int MultiplyBy(int step)
{
  int returnValue = 0; 

  if( xSemaphoreTake( _xCounterSemaphore, ( TickType_t ) 100 ) == pdTRUE )
  {
    _voatileCounter *= step;
    returnValue = _voatileCounter;
    xSemaphoreGive( _xCounterSemaphore );
  }

  return returnValue;
}

int IncrementBy(int step)
{
  int returnValue = 0; 

  if( xSemaphoreTake( _xCounterSemaphore, ( TickType_t ) 100 ) == pdTRUE )
  {
    _voatileCounter += step;
    returnValue = _voatileCounter;
    xSemaphoreGive( _xCounterSemaphore );
  }

  return returnValue;
}

int GetAndClearCounterValue()
{
  int returnValue = 0;

  if( xSemaphoreTake( _xCounterSemaphore, ( TickType_t ) 100 ) == pdTRUE )
  {
    returnValue = _voatileCounter;
    _voatileCounter = 0;
    xSemaphoreGive( _xCounterSemaphore );
  }

  return returnValue;
}

int GetRandomNumber()
{
  int returnValue = 100;

  if( xSemaphoreTake( _xRandomSemaphore, ( TickType_t ) 100 ) == pdTRUE )
  {
    returnValue = random_number();
    xSemaphoreGive( _xRandomSemaphore );
  }

  return returnValue;
}
