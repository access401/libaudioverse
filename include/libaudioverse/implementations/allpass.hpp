/**Copyright (C) Austin Hicks, 2014-2016
This file is part of Libaudioverse, a library for realtime audio applications.
This code is dual-licensed.  It is released under the terms of the Mozilla Public License version 2.0 or the Gnu General Public License version 3 or later.
You may use this code under the terms of either license at your option.
A copy of both licenses may be found in license.gpl and license.mpl at the root of this repository.
If these files are unavailable to you, see either http://www.gnu.org/licenses/ (GPL V3 or later) or https://www.mozilla.org/en-US/MPL/2.0/ (MPL 2.0).*/
#pragma once

namespace libaudioverse_implementation {

/**This is an allpass filter in direct form II and using a specified delay line type.

The line can be accessed directly via .line, and all delay setting should be done through the line directly.
This filter knows only how to tick the line to build an allpass.
Via using lineRead and tick, it is possible to nest these allpasses.

Transfer function: \frac{coefficient+z^{-delay}}{1+coefficient*z^{-delay}}.*/
template<typename delay_type>
class AllpassFilter {
	public:
	template<typename... args>
	AllpassFilter(args... delay_args): line(delay_args...) {}
	void setCoefficient(float c);
	float tick(float input);
	//These are for nesting.  Call the first one, feed it through the nested filter, call the second one.
	//This returns the internal line value.
	float beginNestedTick();
	float endNestedTick(float input, float lineValue);
	void reset();
	AllpassFilter<delay_type>* getSlave();
	void setSlave(AllpassFilter<delay_type>* slave);
	float coefficient = 1.0;
	delay_type line;
	private:
	AllpassFilter<delay_type> *slave = nullptr;
};

template<typename delay_type>
void AllpassFilter<delay_type>::setCoefficient(float c) {
	coefficient = c;
	if(slave) slave->setCoefficient(c);
}

template<typename delay_type>
float AllpassFilter<delay_type>::tick(float input) {
	//No nesting.
	return endNestedTick(input, beginNestedTick());
}

template<typename delay_type>
float AllpassFilter<delay_type>::beginNestedTick() {
	return line.computeSample();
}

template<typename delay_type>
float AllpassFilter<delay_type>::endNestedTick(float input, float lineValue) {
	float rec = input-coefficient*lineValue;
	float out = coefficient*rec+lineValue;
	line.advance(rec);
	return out;
}

template<typename delay_type>
void AllpassFilter<delay_type>::reset() {
	line.reset();
}

template<typename delay_type>
AllpassFilter<delay_type>* AllpassFilter<delay_type>::getSlave() {
	return slave;
}

template<typename delay_type>
void AllpassFilter<delay_type>::setSlave(AllpassFilter<delay_type>* s) {
	slave = s;
	//Hook up the line, too.
	if(s) line.setSlave(&(s->line));
	else line.setSlave(nullptr);
}

}