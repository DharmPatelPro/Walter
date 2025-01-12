#include <iostream>
#include <string>
#include "setup.h"
#include "Util.h"
#include "BezierCurve.h"
#include "Kinematics.h"
#include "Trajectory.h"
#include "logger.h"


// the support points in our cubic bezier curves are at one third of the length of the interpolated distance
#define BEZIER_CURVE_SUPPORT_POINT_SCALE (1.0/3.0)

bool useDynamicBezierSupportPoint = false; // if true, support points are calculated considering the speed at end points.

BezierCurve::BezierCurve() {
};

BezierCurve::BezierCurve(const BezierCurve& par) {
	a = par.a;
	supportA = par.supportA;
	b = par.b;
	supportB = par.supportB;
};
void BezierCurve::operator=(const BezierCurve& par) {
	a = par.a;
	supportA = par.supportA;
	b = par.b;
	supportB = par.supportB;
};

void BezierCurve::reset() {
	a.null();
	b.null();
	supportA.null();
	supportB.null();
}

TrajectoryNode& BezierCurve::getStart() {
	return a;
};

TrajectoryNode& BezierCurve::getEnd() {
	return b;
};


float BezierCurve::computeBezier(InterpolationType ipType, float a, float supportA,  float b, float supportB, float t) {
	if ((t>1.0) || (t<0.0)) {
		LOG(ERROR) << "BUG t!=[0..1]:" << t;
	}

	// we interpolate linear for both interpolation types, pose and joints
	if ((ipType == POSE_LINEAR) || (ipType == JOINT_LINEAR))
		// linear interpolatation
		return (1-t)*a + t*b;
	else
		// formula of cubic bezier curve (wikipedia)
		return (1-t)*(1-t)*(1-t)*a + 3*t*(1-t)*(1-t)*supportA + 3*t*t*(1-t)*supportB + t*t*t*b;
}


// interpolate a bezier curve between a and b by use of passeds support points
Pose BezierCurve::computeBezier(InterpolationType ipType, const Pose& a, const Pose& supportA,  const Pose& b, const Pose& supportB, float t) {
	Pose result;
	if ((ipType == JOINT_LINEAR)) {
		for (int i = 0;i<NumberOfActuators;i++)
			result.angles[i] = computeBezier(ipType,a.angles[i], supportA.angles[i], b.angles[i], supportB.angles[i],t);
		Kinematics::getInstance().computeForwardKinematics(result);
	} else {
		for (int i = 0;i<3;i++)
			result.position[i] = computeBezier(ipType,a.position[i], supportA.position[i], b.position[i], supportB.position[i],t);

		for (int i = 0;i<3;i++)
			result.orientation[i] = computeBezier(ipType,a.orientation[i], supportA.orientation[i], b.orientation[i], supportB.orientation[i],t);

		result.gripperDistance = computeBezier(ipType,a.gripperDistance, supportA.gripperDistance, b.gripperDistance, supportB.gripperDistance,t);
	}

	return result;
}

TrajectoryNode BezierCurve::computeBezier(InterpolationType ipType, const TrajectoryNode& a, const TrajectoryNode& supportA,  const TrajectoryNode& b, const TrajectoryNode& supportB, float t) {
	TrajectoryNode result(a); // take over all other attributes from the start node like average speed or duration
	result.pose = computeBezier(ipType, a.pose, supportA.pose, b.pose, supportB.pose, t);
	result.time= a.time + t*(b.time-a.time);
	return result;
}

// define a bezier curve by start and end point and two support points
void BezierCurve::set(TrajectoryNode& pPrev, TrajectoryNode& pA, TrajectoryNode& pB, TrajectoryNode& pNext) {
	a = pA;
	b = pB;
	supportB = pB.pose;
	supportA = pA.pose;

	if ((pA.interpolationTypeDef != JOINT_LINEAR)) {
		if (!pNext.isNull()) {
			supportB =  getSupportPoint(pA.interpolationTypeDef, pA,pB,pNext);
		}
		if (!pPrev.isNull()) {
			supportA =  getSupportPoint(pA.interpolationTypeDef, pB,pA,pPrev);
		}
	}
}

// compute b's support point
Pose  BezierCurve::getSupportPoint(InterpolationType interpType, const TrajectoryNode& a, const TrajectoryNode& b, const TrajectoryNode& c) {
	// support point for bezier curve is computed by
	// BC' = mirror BC at B with length(BC') = length(AB)

	// mirror C at B = mirrorC
	Point mirroredC(c.pose.position);
	mirroredC.mirrorAt(b.pose.position);

	// compute length of BmirrorC and AB and translate point along BmirrorC such that its length equals len(AB)
	float lenBmC = mirroredC.distance(b.pose.position);
	float lenAB = a.pose.position.distance(b.pose.position);
	Point mirroredNormedC (b.pose.position);
	Point t  = b.pose.position - mirroredC;

	if (lenBmC > 1 ) { // [mm]
		t *=lenAB/lenBmC;
		mirroredNormedC  -= t;
	}// otherwise mirroredNormedC equals b (at least it is very close)
	// compute the middle point of A and mirrored C
	Point midOfA_mC = (a.pose.position + mirroredNormedC) * 0.5;

	// consider the speed change in the support point. Compute speed of
	// arriving and leaving this support point and adapt the support point
	// accordingly in order to smooth the acceleration
	rational speedAB = 0, speedBC = 0;
	if (b.time != a.time)
		speedAB = lenAB/(b.time-a.time);
	rational lenBC = b.pose.distance(c.pose);
	if (c.time != b.time)
		speedBC = lenBC/(c.time-b.time);

	rational ratioBCcomparedToAB = 1.0;
	if (useDynamicBezierSupportPoint) {
		ratioBCcomparedToAB = speedAB/speedBC;
		ratioBCcomparedToAB = constrain(ratioBCcomparedToAB,0.3,1/0.3);
	}

	/*
	if (interpType == POSE_SLIGHTLY_ROUNDED) {
		ratioBCcomparedToAB = ratioBCcomparedToAB*0.2;
	}
	*/
	// now move the point towards B such that its length is like BEZIER_CURVE_SUPPORT_POINT_SCALE
	t = b.pose.position - midOfA_mC;
	rational lent = midOfA_mC.distance(b.pose.position);
	if (lent > 1)
		t *= BEZIER_CURVE_SUPPORT_POINT_SCALE*ratioBCcomparedToAB*lenAB/lent;
	else {
		t.null();
	}
	Pose supportB;
	supportB.position = b.pose.position - t;

	// all this is done for the position only,
	// the rotation becomes the point itself (dont have a good model yet for beziercurves of orientations)
	supportB.orientation = b.pose.orientation;
	supportB.gripperDistance = b.pose.gripperDistance;


	return supportB;
}

TrajectoryNode BezierCurve::getCurrent(float t) {
	InterpolationType interpolType = a.interpolationTypeDef;
	TrajectoryNode sA;
	sA.pose = supportA;
	TrajectoryNode sB;
	sB.pose = supportB;

	TrajectoryNode result = computeBezier(interpolType,a,sA,b, sB, t);

	return result;
}

float BezierCurve::distance(float dT1, float dT2) {
	TrajectoryNode last = getCurrent(dT1);
	TrajectoryNode prev = getCurrent(dT2);
	return last.pose.distance(prev.pose);
}

// return a number 0..1 representing the position of t between a and b
float intervalRatio(unsigned long a, unsigned long t, unsigned long b) {
	return ((float)t-(float)a)/((float)b-(float)a);
}

TrajectoryNode BezierCurve::getPointOfLine(unsigned long time) {
	float t = intervalRatio(a.time,time, b.time);
	TrajectoryNode result = getCurrent(t);
	return result;
}


void BezierCurve::amend(float t, TrajectoryNode& pNewB, TrajectoryNode& pNext) {

	// compute current and next curve point. This is used later on to compute
	// the current bezier support point which has the same derivation, assuming
	// that we start from the current position
	static float dT = 0.01;										// arbitrary value, needed to compute the current support points
	TrajectoryNode current = getCurrent(t);						// compute curve point for t(which is now)
	TrajectoryNode currentPoint_plus_dT = getCurrent(t + dT);	// and for t+dT (which is an arbitrary point in time)

	// since we already passed a small piece of t, the next interval is now smaller (actually 1-t)
	// Later on the need dT that has the same distance but relatively to the (shorter) remaining piece
	static float dTNew = dT/(1.0-t);	// compute new dt assuming that we start from current position

	// now we set the new bezier curve. The current position becomes the new starting point
	// and the end point and end support point remains the same. But we need a new support point,
	// which we get out of the current tangent of the current and next, enhanced to the new time frame dTNew
	//
	// the following equation is derived out of the condition
	// 		Bezier (current, current-support,end,end-support, dT) = currentPoint-for-dT
	//
	// out of that, we separate the Bezier terms
	// 		Bezier(0,1,0,0,dT)*supportA = currentPoint-for-dT - Bezier(current,0,end,end-support)
	//
	// and end up in the equation
	// 		supportA = (currentPoint-for-dTNew - Bezier(current,0,end,end-support, dTNew) / Bezier(0,1,0,0,dTNew)
	float supportABezierTermRezi = 1.0/(computeBezier(POSE_CUBIC_BEZIER, 0, 1, 0,0,dTNew)); // take the bezier term of support a only

	Pose newSupportA =
			(currentPoint_plus_dT.pose - computeBezier(POSE_CUBIC_BEZIER, current.pose, Pose(), b.pose, supportB, dTNew))*supportABezierTermRezi;

	// set the new curve
	Pose newSupportPointB;
	if (pNext.isNull())
		newSupportPointB = pNewB.pose;
	else
		newSupportPointB =  getSupportPoint(current.interpolationTypeDef, current,pNewB,pNext);

	a = current; // this sets current time as new point in time as well
	a.interpolationTypeDef = pNewB.interpolationTypeDef;
	b = pNewB;
	b.interpolationTypeDef = pNewB.interpolationTypeDef;

	supportB = newSupportPointB;
	supportA = newSupportA;
}


float BezierCurve::curveLength() {
	float distance = 0.0;
	TrajectoryNode curr = getCurrent(0);

	// BTW: computing the length of a bezier curve in maths style is really complicated, so do it numerically
	float t = 0.01; // at least 0.01mm, in order to not divide by 0 later on
	while (t<1.0) {
		TrajectoryNode next = getCurrent(t);
		distance += curr.pose.distance(next.pose);
		curr = next;
		t += 0.05;
	}

	// last node
	distance += curr.pose.distance(b.pose);
	return distance;
}


