/* vim:ts=4
 *
 * Copyleft 2012…2015  Michał Gawron
 * Marduk Unix Labs, http://mulabs.org/
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Visit http://www.gnu.org/licenses/gpl-3.0.html for more information on licensing.
 */

#ifndef XEFIS__MODULES__SYSTEMS__ENGINE_TORQUE_H__INCLUDED
#define XEFIS__MODULES__SYSTEMS__ENGINE_TORQUE_H__INCLUDED

// Standard:
#include <cstddef>

// Xefis:
#include <xefis/config/all.h>
#include <xefis/core/module.h>
#include <xefis/core/property.h>
#include <xefis/utility/temporal.h>


/**
 * Compute engine torque based on RPM.
 *
 * Assumes that the bigger RPM, the bigger the friction a propeller feels,
 * and a greater torque needs to be generated by an engine.
 *
 * Also counts in RPM change over time.
 *
 * So: torque = k * d(rpm)/dt + c * rpm
 *     k and c should be obtained experimentally.
 */
class EngineTorque: public xf::Module
{
  public:
	// Ctor
	EngineTorque (xf::ModuleManager*, QDomElement const& config);

  private:
	// Module API
	void
	data_updated() override;

  private:
	// Settings:
	double								_linear_coefficient		= 0.0;
	double								_derivative_coefficient	= 0.0;
	double								_total_coefficient		= 1.0;
	// Input:
	xf::PropertyFrequency				_input_engine_rpm;
	// Output:
	xf::PropertyTorque					_output_engine_torque;
	// Other:
	Optional<xf::Temporal<Frequency>>	_previous_engine_spd;
};

#endif
