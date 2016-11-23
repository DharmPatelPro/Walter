/*
 * TrajectoryExecController.cpp
 *
 *  Created on: 29.08.2016
 *      Author: JochenAlt
 */

#include "TrajectoryExecution.h"
#include "ActuatorCtrlInterface.h"

TrajectoryExecution::TrajectoryExecution() {
	lastLoopInvocation = 0;
}

TrajectoryExecution& TrajectoryExecution::getInstance() {
	static TrajectoryExecution instance;
	return instance;
}


bool TrajectoryExecution::setup() {
	bool ok = ActuatorCtrlInterface::getInstance().setupCommunication();
	if (!ok)
    	LOG(ERROR) << "uC not present";

	TrajectoryPlayer::setup();

	return ok;
}

// send a direct command to uC
void TrajectoryExecution::directAccess(string cmd, string& response, bool &okOrNOk) {
	ActuatorCtrlInterface::getInstance().directAccess(cmd, response, okOrNOk);
}

void TrajectoryExecution::loguCToConsole() {
	ActuatorCtrlInterface::getInstance().loguCToConsole();
}

string TrajectoryExecution::currentTrajectoryNodeToString() {
	TrajectoryNode node = getCurrentTrajectoryNode();
	return node.toString();
}

void TrajectoryExecution::runTrajectory(const string& trajectoryStr) {
	Trajectory& traj = getTrajectory();
	int idx = 0;
	bool ok = traj.fromString(trajectoryStr, idx);
	if (!ok)
		LOG(ERROR) << "parse error trajectory";
	traj.compile();

	playTrajectory();
}

void TrajectoryExecution::setPose(const string& poseStr) {
	Pose pose;
	int idx = 0;
	bool ok = pose.fromString(poseStr, idx);
	if (!ok)
		LOG(ERROR) << "parse error trajectory";
	TrajectoryPlayer::setPose(pose);
}


void TrajectoryExecution::setAnglesAsString(string anglesAsString) {
	JointAngles angles;
	int idx = 0;
	bool ok = angles.fromString(anglesAsString, idx);
	if (!ok)
		LOG(ERROR) << "parse error angles";
	TrajectoryPlayer::setAngles(angles);
}

void TrajectoryExecution::loop() {
	// take current time, compute IK and store pose and angles every TrajectorySampleRate.
	// When a new pose is computed, notifyNewPose is called
	TrajectoryPlayer::loop();
}


// is called by TrajectoryPlayer whenever a new pose is set
void TrajectoryExecution::notifyNewPose(const Pose& pPose) {
	// ensure that we are not called more often then TrajectorySampleRate
	uint32_t now = millis();
	if ((lastLoopInvocation>0) && (now<lastLoopInvocation+TrajectorySampleRate)) {
		// we are called too early, wait
		delay(lastLoopInvocation+TrajectorySampleRate-now);
		LOG(ERROR) << "TrajectoryExecution:notifyNewPose called too early: now=" << now << " lastcall=" << lastLoopInvocation;
	}

	// move the bot to the passed position within the next TrajectorySampleRate ms.
	ActuatorCtrlInterface::getInstance().move(pPose.angles, TrajectorySampleRate);
}

bool TrajectoryExecution::startupBot() {
	LOG(INFO) << "initiating startup procedure";

	// if the bot is in zombie state, disable it properly
	bool enabled, setuped, powered;
	bool ok = ActuatorCtrlInterface::getInstance().info(powered, setuped, enabled);
	if (ok && !powered && enabled) {
		ActuatorCtrlInterface::getInstance().disableBot();
		ActuatorCtrlInterface::getInstance().info(powered, setuped, enabled);
		if (enabled)
			LOG(ERROR) << "startupBot: disable did not work";
	}

	// initialize all actuator controller, idempotent. Enables reading angle sensors
	ok = ActuatorCtrlInterface::getInstance().setupBot();
	if (!ok) {
		LOG(ERROR) << "startupBot: setup did not work";
		return false;
	}

	// read all angles and check if ok
	ActuatorStateType initialActuatorState[NumberOfActuators];
	ok = ActuatorCtrlInterface::getInstance().getAngles(initialActuatorState);
	if (!ok) {
		LOG(ERROR) << "startupBot: getAngles did not work";
		return false;
	}

	// before powering up, get the status of all actuators
	if (!ok && powered)
		ok = ActuatorCtrlInterface::getInstance().power(true);
	if (!ok) {
		LOG(ERROR) << "startupBot: powerUp did not work";
		return false;
	}


	ok = ActuatorCtrlInterface::getInstance().enableBot();	// enable every actuator (now reacting to commands)
	if (!ok) {
		ok = ActuatorCtrlInterface::getInstance().power(false);
		LOG(ERROR) << "startupBot: enable did not work";
		return false;
	}

	// move to default position, but compute necessary time required with slow movement
	rational maxAngleDiff = 0;
	for (int i = 0;i<NumberOfActuators;i++) {
		rational angleDiff = fabs(JointAngles::getDefaultPosition()[i]-initialActuatorState[i].currentAngle);
		maxAngleDiff = min(maxAngleDiff, angleDiff);
	}
	rational speed_deg_per_s = 10; // degrees per second
	rational duration_ms = 0; // duration for movement


	// if we are next to default position already, move every angle by 5�
	// in order to check that all sensors are working properly
	if (degrees(maxAngleDiff) < 1.0) {
		JointAngles angles = JointAngles::getDefaultPosition();
		for (int i = 0;i<NumberOfActuators;i++)
			angles[i] += radians(5.0);

		duration_ms = 5.0/speed_deg_per_s*1000;
		ok = ActuatorCtrlInterface::getInstance().move(angles, duration_ms);
		if (!ok) {
			ok = ActuatorCtrlInterface::getInstance().power(false);
			LOG(ERROR) << "startupBot: move to check position failed";
			return false;
		}
		// wait until we are there
		delay(duration_ms+200);

		ok = ActuatorCtrlInterface::getInstance().getAngles(initialActuatorState);
		if (!ok) {
			ok = ActuatorCtrlInterface::getInstance().power(false);
			LOG(ERROR) << "startupBot: sensing angles after check position failed ";
			return false;
		}
	}

	// move to default position
	duration_ms = degrees(maxAngleDiff)/speed_deg_per_s*1000;
	ok = ActuatorCtrlInterface::getInstance().move(JointAngles::getDefaultPosition(), duration_ms);
	if (!ok) {
		ok = ActuatorCtrlInterface::getInstance().power(false);
		LOG(ERROR) << "startupBot: move to default position did not work";
		return false;
	}

	// wait until we are there
	delay(duration_ms+200);

	// fetch current angles, now from reset position
	ActuatorStateType resetActuatorState[NumberOfActuators];
	ok = ActuatorCtrlInterface::getInstance().getAngles(resetActuatorState);
	if (!ok) {
		ok = ActuatorCtrlInterface::getInstance().power(false);
		LOG(ERROR) << "startupBot: fetching reset position failed";
		return false;
	}

	// check that we really are in default position
	for (int i = 0;i<NumberOfActuators;i++) {
		rational angleDiff = fabs(JointAngles::getDefaultPosition()[i]-resetActuatorState[i].currentAngle);
		maxAngleDiff = min(maxAngleDiff, angleDiff);
	}
	if (maxAngleDiff > 1.0) {
		ok = ActuatorCtrlInterface::getInstance().power(false);
		LOG(ERROR) << "startupBot: checking reset position failed";
		return false;

	}

	LOG(INFO) << "startup procedure completed";

	return true;
}

bool TrajectoryExecution::teardownBot() {
	bool enabled, setuped, powered;
	bool ok = ActuatorCtrlInterface::getInstance().info(powered, setuped, enabled);
	if (!ok) {
		ActuatorCtrlInterface::getInstance().power(false); 	// delays are done internally
		LOG(ERROR) << "teardownBotBot: info failed";
		return false;
	}

	if (powered && enabled && setuped) {
		ActuatorStateType currentActuatorState[NumberOfActuators];
		ok = ActuatorCtrlInterface::getInstance().getAngles(currentActuatorState);
		if (!ok) {
			ActuatorCtrlInterface::getInstance().power(false); 	// delays are done internally
			LOG(ERROR) << "teardownBotBot: getAngles failed";
			return false;
		}

		// move to default position
		rational maxAngleDiff = 0;
		for (int i = 0;i<NumberOfActuators;i++) {
			rational angleDiff = fabs(JointAngles::getDefaultPosition()[i]-currentActuatorState[i].currentAngle);
			maxAngleDiff = min(maxAngleDiff, angleDiff);
		}
		rational speed_deg_per_s = 10; // degrees per second
		rational duration_ms = degrees(maxAngleDiff) / speed_deg_per_s*1000.0; // duration for movement

		ActuatorCtrlInterface::getInstance().move(JointAngles::getDefaultPosition(), duration_ms);
		delay(duration_ms+200);
	}

	ok = ActuatorCtrlInterface::getInstance().disableBot();
	if (!ok) {
		ActuatorCtrlInterface::getInstance().power(false);
		LOG(ERROR) << "teardownBot: disable failed";
		return false;
	}

	ok = ActuatorCtrlInterface::getInstance().power(false);
	if (!ok) {
		ActuatorCtrlInterface::getInstance().power(false);
		LOG(ERROR) << "teardownBot: power off failed";
		return false;
	}

	return true;
}

