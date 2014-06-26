/**Copyright (C) Austin Hicks, 2014
This file is part of Libaudioverse, a library for 3D and environmental audio simulation, and is released under the terms of the Gnu General Public License Version 3 or (at your option) any later version.
A copy of the GPL, as well as other important copyright and licensing information, may be found in the file 'LICENSE' in the root of the Libaudioverse repository.  Should this file be missing or unavailable to you, see <http://www.gnu.org/licenses/>.*/

#include <libaudioverse/private_sources.hpp>
#include <libaudioverse/private_sourcemanager.hpp>
#include <libaudioverse/private_properties.hpp>
#include <libaudioverse/private_macros.hpp>
#include <libaudioverse/private_constants.hpp>
#include <libaudioverse/private_devices.hpp>
#include <libaudioverse/private_creators.hpp>
#include <libaudioverse/libaudioverse.h>
#include <libaudioverse/libaudioverse_properties.h>
#include <libaudioverse/libaudioverse3d.h>
#include <libaudioverse/private_errors.hpp>
#include <math.h>
#include <stdlib.h>
#include <glm/glm.hpp>
#include <limits>

LavSource::LavSource(LavDevice* device, LavSourceManager* manager, LavObject* sourceNode): LavPassthroughObject(device, device->getChannels()) {
	if(sourceNode->getOutputCount() > 1) throw LavErrorException(Lav_ERROR_SHAPE);
	source_object = sourceNode;
	attenuator_object = createAttenuatorObject(device, 1);
	panner_object = manager->createPannerObject();
	attenuator_object->setParent(0, source_object, 0);
	panner_object->setParent(0, attenuator_object, 0);
	for(unsigned int i = 0; i <num_inputs; i++) {
		setParent(i, panner_object, i);
	}
	float defaultPos[] = {0.0f, 0.0f, 0.0f};
	properties[Lav_3D_POSITION] = createFloat3Property("position", defaultPos);
	properties[Lav_SOURCE_DISTANCE_MODEL] = createIntProperty("distance_model", Lav_DISTANCE_MODEL_LINEAR, Lav_DISTANCE_MODEL_MIN, Lav_DISTANCE_MODEL_MAX);
	properties[Lav_SOURCE_MAX_DISTANCE] = createFloatProperty("max_distance", 50.0f, 0.0f, std::numeric_limits<float>::infinity());
	this->manager = manager;
	manager->associateSource(this);
}

LavObject* createSource(LavDevice* device, LavSourceManager* manager, LavObject* sourceNode) {
	return new LavSource(device, manager, sourceNode);
}

void LavSource::update(LavEnvironment env) {
	environment = env;
}

//helper function: calculates gains given distance models.
float calculateGainForDistanceModel(int model, float distance, float maxDistance) {
	float retval = 1.0f;
	switch(model) {
		case Lav_DISTANCE_MODEL_LINEAR: retval = 1.0f-(distance/maxDistance);
	}

	//safety clamping.  Some of the equations above will go negative after max_distance.
	if(retval < 0.0f) retval = 0.0f;
	return retval;
}

void LavSource::willProcessParents() {
	//first, extract the vector of our position.
	const float* pos = properties[Lav_3D_POSITION]->getFloat3Value();
	glm::vec4 npos = environment.world_to_listener_transform*glm::vec4(pos[0], pos[1], pos[2], 1.0f);
	//npos is now easy to work with.
	float distance = glm::length(npos);
	float xz = sqrtf(npos.x*npos.x+npos.z*npos.z);
	//elevation and azimuth, in degrees.
	float elevation = atan2f(npos.y, xz)/PI*180.0f;
	float azimuth = atan2(npos.x, -npos.z)/PI*180.0f;
	if(elevation > 90.0f) elevation = 90.0f;
	if(elevation < -90.0f) elevation = -90.0f;

	int distanceModel = properties[Lav_SOURCE_DISTANCE_MODEL]->getIntValue();
	float maxDistance = properties[Lav_SOURCE_MAX_DISTANCE]->getFloatValue();
	float gain = calculateGainForDistanceModel(distanceModel, distance, maxDistance);

	//set the panner and attenuator.
	panner_object->getProperty(Lav_HRTF_AZIMUTH)->setFloatValue(azimuth);
	panner_object->getProperty(Lav_HRTF_ELEVATION)->setFloatValue(elevation);
	attenuator_object ->getProperty(Lav_ATTENUATOR_MULTIPLIER)->setFloatValue(gain);
}

Lav_PUBLIC_FUNCTION LavError Lav_createSource(LavDevice* device, LavObject* environment, LavObject* node, LavObject** destination) {
	PUB_BEGIN
	LavObject* retval = createSource(device, (LavSourceManager*)environment, node);
	*destination = retval;
	PUB_END
}