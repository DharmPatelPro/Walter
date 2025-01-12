/*
 * TrajectoryMgr.h
 *
 * Class that moves Walter by playing the trajectory and calling the cortex interpolated move commands
 *
 * Author: JochenAlt
 */

#ifndef TRAJECTORYMGR_H_
#define TRAJECTORYMGR_H_

#include "TrajectoryPlayer.h"

class TrajectoryExecution : public TrajectoryPlayer {
public:
	TrajectoryExecution();
	static TrajectoryExecution& getInstance();

	// call this upfront before doing anything.
	bool setup(int pSampleDuration /* [ms] */);

	// call as often as possible. Runs the trajectory by computing a support point every TrajectorySampleRate
	// and call notifyNewPose where communication with uC happens
	void loop();

	// send a direct command to uC. Used in console only
	void directAccess(string cmd, string& response, bool &okOrNOk);

	// log everything from uC to cout. Used when directly access the uC
	void loguCToConsole();

	// get the current trajectory node of the robot as string. Used to display it in the UI
	string currentTrajectoryNodeToString(int &indent);

	// set the current angles in stringified form
	bool setAnglesAsString(string angles);

	// set the current trajectory to be player
	void runTrajectory(const string& trajectory);

	// set the current pose to the bot
	void setPose(const string& pose);

	// is called by TrajectoryPlayer whenever a new pose is computed
	void notifyNewPose(const Pose& pPose);

	// switch on power and move bot into default position
	bool startupBot();

	// move the bot to its null position
	bool moveToNullPosition();

	// move into default position and power down
	bool teardownBot();

	// move into default position and power down
	bool emergencyStopBot();

	// check if we can control the bot or receive commands
	bool isBotUpAndReady();

	bool heartBeatSendOp();

private:
	uint32_t lastLoopInvocation = 0;
	bool botIsUpAndRunning = false;
	bool heartbeatSend = false;

};



#endif /* TRAJECTORYMGR_H_ */
