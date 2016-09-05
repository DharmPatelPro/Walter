/*
 * uiconfig.h
 *
 *  Created on: 22.08.2016
 *      Author: JochenAlt
 */

#ifndef UI_UICONFIG_H_
#define UI_UICONFIG_H_

// constants used in the UI
const float ViewEyeDistance 		= 1500.0f;	// distance of the eye to the bot
const float ViewBotHeight 			= 800.0f;	// height of the bot to be viewed
const int pearlChainDistance_ms		= 50;		// trajectories are display with pearls in a timing distance


// window size
const int WindowGap=10;							// gap between subwindows
const int InteractiveWindowWidth=390;			// initial width of the interactive window

// used colors
static const GLfloat glMainWindowColor[]         = {1.0,1.0,1.0};
static const GLfloat glBotArmColor[] 			= { 0.9f, 0.3f, 0.2f };
static const GLfloat glBotaccentColor[] 		= { 0.45f, 0.4f, 0.4f };
static const GLfloat glBlackColor[] 			= { 0.0f, 0.0f, 0.0f };
static const GLfloat glWhiteColor[] 			= { 1.0f, 1.0f, 1.0f };

static const GLfloat glBotJointColor[] 			= { 0.5f, 0.6f, 0.6f };
static const GLfloat glCoordSystemColor3v[] 	= { 0.40f, 0.40f, 0.6f };
static const GLfloat glRasterColor3v[] 			= { .90f, .97f, 0.97f };
static const GLfloat glSubWindowColor[] 		= { 0.97,0.97,0.97};
static const GLfloat glWindowTitleColor[] 		= { 1.0f, 1.0f, 1.0f };
static const GLfloat glTCPColor3v[] 			= { 0.23f, 0.62f, 0.94f };
static const GLfloat startPearlColor[] 			= { 0.23f, 1.0f, 0.24f };
static const GLfloat endPearlColor[] 			= { 0.90f, 0.2f, 0.2f };
static const GLfloat midPearlColor[] 			= { 1.0f, 0.8f, 0.0f };
static const GLfloat limitExceededPearlColor[]	= { 1.0f, 0.0f, 0.0f };

#endif /* UI_UICONFIG_H_ */