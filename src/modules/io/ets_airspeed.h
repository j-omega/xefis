/* vim:ts=4
 *
 * Copyleft 2012…2016  Michał Gawron
 * Marduk Unix Labs, http://mulabs.org/
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Visit http://www.gnu.org/licenses/gpl-3.0.html for more information on licensing.
 *
 * This module was based on Paparazzi AirspeedETS module.
 */

#ifndef XEFIS__MODULES__IO__ETAIRSPEED_H__INCLUDED
#define XEFIS__MODULES__IO__ETAIRSPEED_H__INCLUDED

// Standard:
#include <cstddef>

// Xefis:
#include <xefis/config/all.h>
#include <xefis/core/module.h>
#include <xefis/core/property.h>
#include <xefis/support/bus/i2c.h>
#include <xefis/utility/smoother.h>


/**
 * Warning: this module uses I2C I/O in main thread, which may block.
 *
 * Handles EagleTree Airspeed V3 sensor.
 * The sensor must be in default mode, not in 3-rd party mode.
 */
class ETSAirspeed:
	public QObject,
	public xf::Module
{
	Q_OBJECT

	static constexpr uint8_t		ValueRegister				= 0xea;
	static constexpr float			ValueScale					= 1.8f;
	static constexpr Time			InitializationDelay			= 0.2_s;
	static constexpr unsigned int	OffsetCalculationSamples	= 100;
	static constexpr uint16_t		RawValueMinimum				= 1450;
	static constexpr uint16_t		RawValueMaximum				= 1750;

	enum class Stage {
		Calibrating,
		Running,
	};

  public:
	// Ctor:
	ETSAirspeed (xf::ModuleManager*, QDomElement const& config);

  private slots:
	/**
	 * Starts module calibration.
	 */
	void
	initialize();

	/**
	 * Reinitialize module after failure.
	 * Don't recalibrate.
	 */
	void
	reinitialize();

	/**
	 * Read data from the sensor and update properties.
	 */
	void
	read();

  private:
	/**
	 * Called when enough initial samples are collected to get
	 * offset value.
	 */
	void
	offset_collected();

	/**
	 * Guard and reinitialize on I2C error.
	 */
	void
	guard (std::function<void()> guarded_code);

  private:
	xf::PropertyBoolean		_serviceable;
	xf::PropertySpeed		_airspeed;
	xf::PropertySpeed		_airspeed_minimum;
	xf::PropertySpeed		_airspeed_maximum;
	Time					_airspeed_read_interval		= 100_ms;
	Time					_airspeed_smoothing_time	= 100_ms;
	xf::i2c::Device			_i2c_device;
	Stage					_stage						= Stage::Calibrating;
	QTimer*					_initialization_timer;
	QTimer*					_periodic_read_timer;
	std::vector<uint16_t>	_calibration_data;
	uint16_t				_offset						= 0;
	xf::Smoother<double>	_airspeed_smoother			= Time (100_ms);
};

#endif

