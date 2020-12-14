/* BLE controlled stepper motor */

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
  switch (step_size)
  {
    case 1:

    break;
      digitalWrite(MS1_PIN, LOW);
      digitalWrite(MS2_PIN, LOW);
      digitalWrite(MS3_PIN, LOW);
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
  step_delay_ms = delay_ms;
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
    digitalWrite(STEP_PIN, LOW);
    delay(1);  // bleah!  This means step_delay_ms should always be at least 2 ms.
    digitalWrite(STEP_PIN, HIGH);

    step_count++;
    if (step_count > max_steps)
    {
      current_state = STATE_STOPPED;
    }
    
    next_step_ms += step_delay_ms;
  }
  
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

  Serial.println("inited");

  /* quick motion tests */
  delay(1000);
  set_step_size(16);
  max_steps = 50;
  set_step_delay(100);
  current_state = STATE_MOVING;
  digitalWrite(ENABLE_PIN, LOW);
  
}

/*==================================================
 * loop
 *=================================================*/
void loop( void )
{
  check_for_command();
  check_for_step();
}
