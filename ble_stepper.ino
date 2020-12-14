/* BLE controlled stepper motor */
#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEServer.h>

/* I've laid out the pin connections so that they "walk up" the 
 *  side of the ESP32.  Can't go completly contiguous on both as the 
 *  Big-easy board has VCC, GND, RESET, and SLEEP in the middle.
 */
#define MS1_PIN        2
#define MS2_PIN        0
#define MS3_PIN        4
#define ENABLE_PIN    15
#define STEP_PIN      17
#define DIR_PIN       16

typedef enum
{
  DIR_FORWARD,
  DIR_BACKWARD
} dir_type;

typedef enum
{
  STATE_STOPPED,
  STATE_MOVING
} state_type;

state_type current_state;

uint32_t step_delay_ms = 1;
uint32_t next_step_ms = 0;
uint32_t step_delay = 2;

uint32_t step_count = 0;
uint32_t max_steps = 0;

/*==================================================
 * BLE Characteristic Definitions
 *================================================*/
#define SERVICE_UUID             "23613f70-7648-4d5a-af18-4e0133f655a3"
#define CHAR_STEP_SIZE_UUID      "3c81bce1-144d-4b49-87ae-53850dc1c3d7"
#define CHAR_STEP_DELAY_MS_UUID  "2e475f5e-e4f7-4518-ab72-fcc6f2176746"
#define CHAR_DIR_UUID            "30103b8b-a5ec-4e78-bfc5-eb4d2a522b1b"
#define CHAR_MOTION_CTL_UUID     "2798ff83-9107-452b-90e3-bf9a79f9505f"
#define CHAR_NUM_STEPS_UUID      "17ac194e-8d53-4590-a55e-381aff319b82"


/*==================================================
 * set_step_size
 * 
 * Sets the stepper step size via the MS PINS
 * 
 * step_size must be one of 1,2,4,8, or 16.  Note the actual step size
 *   is 1/step_size
 *   
 * returns non-zero if we have an error. 
 *=================================================*/
int set_step_size(int step_size )
{

  Serial.print("Setting step size = ");
  Serial.println(step_size);
  
  switch (step_size)
  {
    case 1:
      digitalWrite(MS1_PIN, LOW);
      digitalWrite(MS2_PIN, LOW);
      digitalWrite(MS3_PIN, LOW);
    break;

    case 2: 
      digitalWrite(MS1_PIN, HIGH);
      digitalWrite(MS2_PIN, LOW);
      digitalWrite(MS3_PIN, LOW);
    break;

    case 4:
      digitalWrite(MS1_PIN, LOW);
      digitalWrite(MS2_PIN, HIGH);
      digitalWrite(MS3_PIN, LOW);
    break;

    case 8:
      digitalWrite(MS1_PIN, HIGH);
      digitalWrite(MS2_PIN, HIGH);
      digitalWrite(MS3_PIN, LOW);
    break;

    case 16:
      digitalWrite(MS1_PIN, HIGH);
      digitalWrite(MS2_PIN, HIGH);
      digitalWrite(MS3_PIN, HIGH);
    break;

    default:
      Serial.print("Invalid step size: ");
      Serial.println(step_size);
      return(-1);
  }

  return 0;
  
}

/*==================================================
 * set_step_delay
 * 
 * Sets the delay between steps
 * 
 * delay is the number of ms between steps.
 *   
 * returns non-zero if we have an error. 
 *=================================================*/
int set_step_delay(uint32_t delay_ms )
{
  if (delay_ms >=2 )
  {
    step_delay_ms = delay_ms;
    return 0;
  }
  else
  {
    Serial.print("Invalid step delay: ");
    Serial.println(delay_ms);
    return -1;
  }
}

/*==================================================
 * set_num_steps
 * 
 * Sets the number of steps we'll take.   
 * 
 * returns non-zero if we have an error. 
 *=================================================*/
int set_num_steps(uint32_t num_steps )
{
  max_steps = num_steps;
}

/*==================================================
 * set_direction
 * 
 *=================================================*/
int set_direction(dir_type dir )
{
  if (dir == DIR_FORWARD)
  {
    digitalWrite(DIR_PIN, HIGH);
    return 0;  
  }
  else if (dir == DIR_BACKWARD)
  {
    digitalWrite(DIR_PIN, LOW);
    return 0;
  }
  else
  {
    Serial.println("BAD DIRECTION!!!");
    return -1;
  }
  
}

void stop_motion(bool engage_stepper)
{
  current_state = STATE_STOPPED;
  if (!engage_stepper)
  {
    digitalWrite(ENABLE_PIN, HIGH);
  }
}

/*==================================================
 * check_for_command
 * 
 * This processes any commands over all interfaces
 * and performs the appropriate action
 *=================================================*/
void check_for_command( void )
{
  /*  nothing yet */
  
}

/*==================================================
 * start_motion
 * 
 * enable the stepper and set our state to "moving".
 * Assumes step size, num steps, and step delay have already
 *   been set properly.
 * Note that the "check_for_step function (called from loop) 
 *   will actually step our stepper when the time is right.
 *=================================================*/
void start_motion( void )
{
  Serial.print("Starting motion: ");
  Serial.print(max_steps);
  Serial.println(" steps to take");

  step_count = 0;

  next_step_ms = millis();
  
  current_state = STATE_MOVING;
  digitalWrite(ENABLE_PIN, LOW);
}

/*==================================================
 * check_for_step
 * 
 * If we're moving, see if it's time to take our next step.
 *=================================================*/
void check_for_step( void )
{
  uint32_t current_ms;

  current_ms = millis();

  if ((current_state == STATE_MOVING) && (current_ms >= next_step_ms))
  {
    Serial.print("step # ");
    Serial.println(step_count);
    
    digitalWrite(STEP_PIN, LOW);
    delay(1);  // bleah!  This means step_delay_ms should always be at least 2 ms.
    digitalWrite(STEP_PIN, HIGH);

    step_count++;
    if (step_count > max_steps)
    {
      Serial.println("Max steps...stopping");
      current_state = STATE_STOPPED;
    }
    
    next_step_ms += step_delay_ms;
  }
  
}

class MyServerCallbacks: public BLEServerCallbacks 
{
    void onConnect(BLEServer* pServer) 
    {
      Serial.println("Device Connected");
    };

    void onDisconnect(BLEServer* pServer) 
    {
      Serial.println("Device Disconnected");
    }
};

class StepSizeCB: public BLECharacteristicCallbacks 
{
    void onWrite(BLECharacteristic *pCharacteristic) 
    {
      std::string value = pCharacteristic->getValue();
      
      int step_size;
      
      
      step_size = atoi((char*) value.c_str());
      
      Serial.print("step_size set by BLE: ");
      Serial.println(step_size);

      set_step_size(step_size);
      
    }
};

class StepDelayCB: public BLECharacteristicCallbacks 
{
    void onWrite(BLECharacteristic *pCharacteristic) 
    {
      std::string value = pCharacteristic->getValue();
      
      uint32_t step_delay;
      
      
      step_delay = atol((char*) value.c_str());
      
      Serial.print("Step_delay set by BLE: ");
      Serial.println(step_delay);

      set_step_delay(step_delay);
      
    }
};

class DirCB: public BLECharacteristicCallbacks 
{
    void onWrite(BLECharacteristic *pCharacteristic) 
    {
      std::string value = pCharacteristic->getValue();

      if (value[0] == '0')
      {
        Serial.println("Setting Direction to FORWARD");
        set_direction(DIR_FORWARD);
      }
      else if (value[0] == '1')
      {
        Serial.println("Setting Direction to BACKWARD");
        set_direction(DIR_BACKWARD);
      }
      else
      {
        Serial.print("unknown direction: ");
        Serial.println(value.c_str());
      }
    }
};

class MotionCtlCB: public BLECharacteristicCallbacks 
{
    void onWrite(BLECharacteristic *pCharacteristic) 
    {
      std::string value = pCharacteristic->getValue();

      switch (value[0])
      {
        case 't':
        case 'T':
          Serial.println("Start tracking");
          start_motion();
        break;

        case 'x':
        case 'X':
          Serial.println("Stop tracking, disengage stepper");
          stop_motion(false);
        break;

        case 'e':
        case 'E':
          Serial.println("Stop tracking, keep stepper engaged");
          stop_motion(true);
        break;

        default:
          Serial.print("Unknown motion command: ");
          Serial.println(value.c_str());
      }
    }
};

class NumStepsCB: public BLECharacteristicCallbacks 
{
    void onWrite(BLECharacteristic *pCharacteristic) 
    {
      std::string value = pCharacteristic->getValue();
      
      uint32_t num_steps;
      
      
      num_steps = atol((char*) value.c_str());
      
      Serial.print("Num Steps set by BLE: ");
      Serial.println(num_steps);

      set_num_steps(num_steps);
    }
};

void init_BLE( void )
{
  uint8_t init_value[6];
  
  BLEDevice::init("Stepper Controller");
  BLEServer *pServer = BLEDevice::createServer();
  pServer->setCallbacks(new MyServerCallbacks());

  BLEService *pService = pServer->createService(SERVICE_UUID);

  BLECharacteristic *pStepSizeChar = pService->createCharacteristic(
                                         CHAR_STEP_SIZE_UUID,
                                         BLECharacteristic::PROPERTY_READ |
                                         BLECharacteristic::PROPERTY_WRITE
                                       );
                                       
  pStepSizeChar->setCallbacks(new StepSizeCB());  

  BLECharacteristic *pStepDelayChar = pService->createCharacteristic(
                                         CHAR_STEP_DELAY_MS_UUID,
                                         BLECharacteristic::PROPERTY_READ |
                                         BLECharacteristic::PROPERTY_WRITE
                                       );
                                       
  pStepDelayChar->setCallbacks(new StepDelayCB());
  
  BLECharacteristic *pDirChar = pService->createCharacteristic(
                                         CHAR_DIR_UUID,
                                         BLECharacteristic::PROPERTY_READ |
                                         BLECharacteristic::PROPERTY_WRITE
                                       );
                                       
  pDirChar->setCallbacks(new DirCB());
  
  BLECharacteristic *pMotionCtlChar = pService->createCharacteristic(
                                         CHAR_MOTION_CTL_UUID,
                                         BLECharacteristic::PROPERTY_READ |
                                         BLECharacteristic::PROPERTY_WRITE
                                       );
                                       
  pMotionCtlChar->setCallbacks(new MotionCtlCB());

  BLECharacteristic *pNumStepsChar = pService->createCharacteristic(
                                         CHAR_NUM_STEPS_UUID,
                                         BLECharacteristic::PROPERTY_READ |
                                         BLECharacteristic::PROPERTY_WRITE
                                     );
                                       
  pNumStepsChar->setCallbacks(new NumStepsCB());

  pService->start();
  BLEAdvertising *pAdvertising = pServer->getAdvertising();
  pAdvertising->start();
}

/*==================================================
 * setup
 *=================================================*/
void setup( void )
{
  Serial.begin(9600);

  pinMode(STEP_PIN, OUTPUT);
  pinMode(DIR_PIN, OUTPUT);
  pinMode(MS1_PIN, OUTPUT);
  pinMode(MS2_PIN, OUTPUT);
  pinMode(MS3_PIN, OUTPUT);
  pinMode(ENABLE_PIN, OUTPUT);

  digitalWrite(ENABLE_PIN, HIGH);  // Start disabled.  
  digitalWrite(STEP_PIN, HIGH);    

  init_BLE();

  Serial.println("inited");
  
}

/*==================================================
 * loop
 *=================================================*/
void loop( void )
{
  check_for_command();
  check_for_step();
}
