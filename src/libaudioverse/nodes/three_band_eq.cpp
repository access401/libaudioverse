/* Copyright 2016 Libaudioverse Developers. See the COPYRIGHT
file at the top-level directory of this distribution.

Licensed under the mozilla Public License, version 2.0 <LICENSE.MPL2 or
https://www.mozilla.org/en-US/MPL/2.0/> or the Gbnu General Public License, V3 or later
<LICENSE.GPL3 or http://www.gnu.org/licenses/>, at your option. All files in the project
carrying such notice may not be copied, modified, or distributed except according to those terms. */
#include <libaudioverse/libaudioverse.h>
#include <libaudioverse/libaudioverse_properties.h>
#include <libaudioverse/nodes/three_band_eq.hpp>
#include <libaudioverse/private/node.hpp>
#include <libaudioverse/private/server.hpp>
#include <libaudioverse/private/properties.hpp>
#include <libaudioverse/implementations/biquad.hpp>
#include <libaudioverse/private/macros.hpp>
#include <libaudioverse/private/memory.hpp>
#include <libaudioverse/private/kernels.hpp>
#include <libaudioverse/private/dspmath.hpp>
#include <libaudioverse/private/multichannel_filter_bank.hpp>
#include <algorithm>

namespace libaudioverse_implementation {

ThreeBandEqNode::ThreeBandEqNode(std::shared_ptr<Server> server, int channels): Node(Lav_OBJTYPE_THREE_BAND_EQ_NODE, server, channels, channels),
midband_peaks(server->getSr()),
highband_shelves(server->getSr()) {
	if(channels <= 0) ERROR(Lav_ERROR_RANGE, "Channels must be greater 0.");
	appendInputConnection(0, channels);
	appendOutputConnection(0, channels);
	midband_peaks.setChannelCount(channels);
	highband_shelves.setChannelCount(channels);
	//Set ranges of the nyqiuist properties.
	getProperty(Lav_THREE_BAND_EQ_HIGHBAND_FREQUENCY).setFloatRange(0.0, server->getSr()/2.0);
	getProperty(Lav_THREE_BAND_EQ_LOWBAND_FREQUENCY).setFloatRange(0.0, server->getSr()/2.0);
	recompute();
	setShouldZeroOutputBuffers(false);
}

std::shared_ptr<Node> createThreeBandEqNode(std::shared_ptr<Server> server, int channels) {
	return standardNodeCreation<ThreeBandEqNode>(server, channels);
}

void ThreeBandEqNode::recompute() {
	double lowbandFreq=getProperty(Lav_THREE_BAND_EQ_LOWBAND_FREQUENCY).getFloatValue();
	double lowbandDb=getProperty(Lav_THREE_BAND_EQ_LOWBAND_DBGAIN).getFloatValue();
	double midbandDb=getProperty(Lav_THREE_BAND_EQ_MIDBAND_DBGAIN).getFloatValue();
	double highbandFreq = getProperty(Lav_THREE_BAND_EQ_HIGHBAND_FREQUENCY).getFloatValue();
	double highbandDb= getProperty(Lav_THREE_BAND_EQ_HIGHBAND_DBGAIN).getFloatValue();
	double midbandFreq = lowbandFreq+(highbandFreq-lowbandFreq)/2.0;
	//low band's gain is the simplest.
	lowband_gain=dbToScalar(lowbandDb, 1.0);
	//The peaking filter for the middle band needs to go from lowbandDb to midbandDb, i.e.:
	double peakingDbgain =midbandDb-lowbandDb;
	//And the highband needs to go from the middle band to the high.
	double highshelfDbgain=highbandDb-midbandDb;
	//Compute q from bw and s, using an arbetrary biquad filter.
	//The biquad filters only care about sr, so we can just pick one.
	double peakingQ = midband_peaks->qFromBw(midbandFreq, (highbandFreq-midbandFreq)*2);
	double highshelfQ = highband_shelves->qFromS(highbandFreq, 1.0);
	midband_peaks->configure(Lav_BIQUAD_TYPE_PEAKING, midbandFreq, peakingDbgain, peakingQ);
	highband_shelves->configure(Lav_BIQUAD_TYPE_HIGHSHELF, highbandFreq, highshelfDbgain, highshelfQ);
}

void ThreeBandEqNode::process() {
	if(werePropertiesModified(this,
	Lav_THREE_BAND_EQ_LOWBAND_DBGAIN,
	Lav_THREE_BAND_EQ_LOWBAND_FREQUENCY,
	Lav_THREE_BAND_EQ_MIDBAND_DBGAIN,
	Lav_THREE_BAND_EQ_HIGHBAND_DBGAIN,
	Lav_THREE_BAND_EQ_HIGHBAND_FREQUENCY
	)) recompute();
	for(int channel=0; channel < midband_peaks.getChannelCount(); channel++) scalarMultiplicationKernel(block_size, lowband_gain, input_buffers[channel], output_buffers[channel]);
	midband_peaks.process(block_size, &output_buffers[0], &output_buffers[0]);
	highband_shelves.process(block_size, &output_buffers[0], &output_buffers[0]);
}

void ThreeBandEqNode::reset() {
	midband_peaks.reset();
	highband_shelves.reset();
}

//begin public api

Lav_PUBLIC_FUNCTION LavError Lav_createThreeBandEqNode(LavHandle serverHandle, int channels, LavHandle* destination) {
	PUB_BEGIN
	auto server = incomingObject<Server>(serverHandle);
	LOCK(*server);
	auto retval = createThreeBandEqNode(server, channels);
	*destination = outgoingObject<Node>(retval);
	PUB_END
}

}