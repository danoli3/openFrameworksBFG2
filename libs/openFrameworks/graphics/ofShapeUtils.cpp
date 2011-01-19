#include "ofShapeUtils.h"

// helper functions for accessing points in polylines without worrying about wraparound indexing
inline int loopMod(int i, int n) {
	i = i % n;
	return i < 0 ? i + n : i;
}

template <class T>
inline T& loopGet(vector<T>& vec, int i) {
	return vec[loopMod(i, vec.size())];
}

void ofSmoothPolyline(ofPolyline& polyline, int smoothingSize, float smoothingAmount) {
	ofPolyline original = polyline;
	
	// precompute weights and normalization
	vector<float> weights;
	float weightSum = 0;
	weights.push_back(1); // center weight
	// side weights
	for(int i = 1; i <= smoothingSize; i++) {
		float curWeight = ofMap(i, 0, smoothingSize, 1, smoothingAmount);
		weights.push_back(curWeight);
		weightSum += curWeight;
	}
	float weightNormalization = 1 / (1 + 2 * weightSum);
	
	// use weights to make weighted averages of neighbors
	int n = polyline.size();
	for(int i = 0; i < n; i++) {
		polyline[i].set(0, 0);
		for(int j = 1; j <= smoothingSize; j++) {
			int leftPosition = (n + i - j) % n;
			int rightPosition = (i + j) % n;
			const ofPoint& left = original[leftPosition];
			const ofPoint& right = original[rightPosition];
			polyline[i] += (left + right) * weights[j];
		}
		polyline[i] += original[i];
		polyline[i] *= weightNormalization;
	}
}

void ofResamplePolyline(ofPolyline& polyline, float spacing) {
	ofPolyline original = polyline;
	polyline.clear();
	
	float totalLength = 0;
	int curStep = 0;
	for(int i = 0; i < (int) original.size() - 1; i++) {
		const ofPoint& cur = original[i];
		const ofPoint& next = original[i + 1];
		ofPoint diff = next - cur;
		
		float curSegmentLength = diff.length();
		totalLength += curSegmentLength;
		
		while(curStep * spacing <= totalLength) {
			float curSample = curStep * spacing;
			float curLength = curSample - (totalLength - curSegmentLength);
			float relativeSample = curLength / curSegmentLength;
			polyline.addVertex(cur.getInterpolated(next, relativeSample));
			curStep++;
		}
	}
}

/*

ofPoint ofGetClosestPoint(const ofPoint& p1, const ofPoint& p2, const ofPoint& p3, float* uptr) {
	float u = (p3.x - p1.x) * (p2.x - p1.x);
	u += (p3.y - p1.y) * (p2.y - p1.y);
	u /= (p2 - p1).length();
	// clamp u
	if(u > 1) {
		u = 1;
	} else if(u < 0) {
		u = 0;
	}
	if(uptr != NULL) {
		*uptr = u;
	}
	return p1.interpolated(p2, u);
}

ofPoint ofGetClosestPoint(ofPolyline& polyline, const ofPoint& target, int& nearest) {
	vector<ofPoint>& pts = polyline.pts;
	
	if(pts.size() == 0) {
		return target;
	}
	
	float distance = 0;
	for(int i = 0; i < pts.size(); i++) {
		float curDistance = target.distance(pts[i]);
		if(i == 0 || curDistance < distance) {
			distance = curDistance;
			nearest = i;
		}
	}
	
	ofPoint left = loopGet(pts, nearest - 1);
	ofPoint center = pts[nearest];
	ofPoint right = loopGet(pts, nearest + 1);
	
	ofPoint leftClosest = closestPoint(left, center, target);
	ofPoint rightClosest = closestPoint(center, right, target);
	
	if(leftClosest.distance(target) < rightClosest.distance(target)) {
		return leftClosest;
	} else {
		return rightClosest;
	}
}

ofPoint ofGetClosestPoint(vector<ofPolyline>& polylines, const ofPoint& target, ofPolyline*& matchedPolyline, int& matchedIndex) {
	matchedPolyline = NULL;
	ofPoint closest;
	float distance;
	bool hasPoints = false;
	for(int i = 0; i < polylines.size(); i++) {
		ofPolyline& cur = polylines[i];
		if(cur.pts.size() > 0) {
			hasPoints = true;
			int curIndex;
			ofPoint curClosest = closestPoint(cur, target, curIndex);
			float curDistance = curClosest.distance(target);
			if(i == 0 || curDistance < distance) {
				distance = curDistance;
				closest = curClosest;
				matchedPolyline = &cur;
				matchedIndex = curIndex;
			}
		}
	}
	if(hasPoints)
		return closest;
	else
		return target;
}

// if possible, these two conditions should be combined into one big case
// so the code is more elegant
void ofGetOffsetPoint(ofPolyline& polyline, int start, float radius, int& lastIndex, ofPoint& point) {
	vector<ofPoint>& pts = polyline.pts;
	if(radius > 0) {
		float curRadius = 0;
		lastIndex = start;
		// find the first index after the radius
		do {
			ofPoint& lastPoint = loopGet(pts, lastIndex);
			lastIndex++;
			ofPoint& curPoint = loopGet(pts, lastIndex);
			float curLength = ofDist(lastPoint.x, lastPoint.y, curPoint.x, curPoint.y);
			curRadius += curLength;			 
		} while(curRadius < radius);
		// overshoots once, bring back
		lastIndex--;
		ofPoint& lastPoint = loopGet(pts, lastIndex);
		ofPoint& pastPoint = loopGet(pts, lastIndex + 1);
		float lastLength = ofDist(lastPoint.x, lastPoint.y, pastPoint.x, pastPoint.y);
		curRadius -= lastLength;
		// then interpolate
		float remainingLength = radius - curRadius;
		point = lastPoint;
		point.interpolate(pastPoint, remainingLength / lastLength);
		lastIndex = loopMod(lastIndex, pts.size()); // clean lastIndex before returning it
	} else {
		float curRadius = 0;
		lastIndex = start;
		// find the first index before the radius
		do {
			ofPoint& lastPoint = loopGet(pts, lastIndex);
			lastIndex--;
			ofPoint& curPoint = loopGet(pts, lastIndex);
			float curLength = ofDist(lastPoint.x, lastPoint.y, curPoint.x, curPoint.y);
			curRadius -= curLength;
		} while(curRadius > radius);
		// overshoots once, bring back
		lastIndex++;
		ofPoint& lastPoint = loopGet(pts, lastIndex);
		ofPoint& pastPoint = loopGet(pts, lastIndex - 1);
		float lastLength = ofDist(lastPoint.x, lastPoint.y, pastPoint.x, pastPoint.y);
		curRadius += lastLength;
		// then interpolate
		float remainingLength = curRadius - radius;
		point = lastPoint;
		point.interpolate(pastPoint, remainingLength / lastLength);
		lastIndex = loopMod(lastIndex, pts.size()); // clean lastIndex before returning it
	}
}
*/
