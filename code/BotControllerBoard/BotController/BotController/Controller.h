/* 
* Motors.h
*
* Created: 22.04.2016 19:26:44
* Author: JochenAlt
*/


#ifndef __MOTORS_H__
#define __MOTORS_H__


#include "Arduino.h"
#include "Actuator.h"
#include "HerkulexServoDrive.h"
#include "GearedStepperDrive.h"
#include "RotaryEncoder.h"

#include "TimePassedBy.h"

#define ADJUST_MOTOR_MANUALLY 1
#define ADJUST_MOTOR_BY_KNOB_WITHOUT_FEEDBACK 2
#define ADJUST_MOTOR_BY_KNOB_WITH_FEEDBACK 3

class Controller {
	public:
		Controller();

		void printMenuHelp();

		bool setup();
		bool setuped();

		bool checkEncoders();
		void printConfiguration();
		void loop();
		void stepperLoop();
		void printAngles();
		Actuator* getActuator(uint8_t number);
		bool setupIsDone() { return setupDone;};
		void enable();
		void disable();
		bool selectActuator(uint8_t number);
		Actuator* getCurrentActuator();

		void adjustMotor(int adjustmentType);	
		void changeAngle(float incr, int duration_ms);

		bool isEnabled() { return enabled;}
		void switchActuatorPowerSupply(bool on);
	private:
		HerkulexServoDrive	servos[MAX_SERVOS];
		GearedStepperDrive	steppers[MAX_STEPPERS];
		RotaryEncoder		encoders[MAX_ENCODERS];
		Actuator			actuators[MAX_ACTUATORS];

		uint8_t numberOfActuators;
		uint8_t numberOfEncoders;
		uint8_t numberOfSteppers;
		uint8_t numberOfServos;

		Actuator* currentMotor;				// currently set motor used for interaction
		TimePassedBy motorKnobTimer;		// used for measuring sample rate of motor knob
		bool setupDone;
		bool enabled;
}; //Motors

#endif //__MOTORS_H__
