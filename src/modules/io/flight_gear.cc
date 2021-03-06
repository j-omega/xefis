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
 */

// Standard:
#include <cstddef>

// Xefis:
#include <xefis/config/all.h>
#include <xefis/core/services.h>
#include <xefis/core/xefis.h>
#include <xefis/utility/numeric.h>
#include <xefis/utility/painter.h>
#include <xefis/utility/qdom.h>

// Local:
#include "flight_gear.h"


XEFIS_REGISTER_MODULE_CLASS ("io/flightgear", FlightGearIO)


typedef float		FGFloat;
typedef double		FGDouble;
typedef uint8_t		FGBool;
typedef uint32_t	FGInt;


BEGIN_PACKED_STRUCT
struct FGInputData
{
	FGDouble	rotation_x_degps;					// rox
	FGDouble	rotation_y_degps;					// roy
	FGDouble	rotation_z_degps;					// roz
	FGDouble	acceleration_x_fps2;				// acx
	FGDouble	acceleration_y_fps2;				// acy
	FGDouble	acceleration_z_fps2;				// acz
	FGDouble	aoa_alpha_maximum_rad;				// ama
	FGDouble	aoa_alpha_minimum_rad;				// ami
	FGDouble	aoa_alpha_rad;						// aoa
	FGDouble	cmd_alt_setting_ft;					// apa
	FGDouble	cmd_cbr_setting_fpm;				// apc
	FGDouble	cmd_speed_setting_kt;				// ats
	FGDouble	cmd_heading_setting_deg;			// aph
	FGDouble	flight_director_pitch_deg;			// fdp
	FGDouble	flight_director_roll_deg;			// fdr
	FGDouble	ias_kt;								// ias
	FGDouble	tas_kt;								// tas
	FGDouble	gs_kt;								// gs
	FGDouble	mach;								// ma
	FGDouble	ias_lookahead_kt;					// iasl
	FGDouble	maximum_ias_kt;						// iasma
	FGDouble	minimum_ias_kt;						// iasmi
	FGBool		standard_pressure;					// std
	FGDouble	altitude_ft;						// al
	FGDouble	radar_altimeter_altitude_agl_ft;	// alr
	FGDouble	pressure_inHg;						// als
	FGDouble	cbr_fpm;							// cbr
	FGDouble	gps_latitude_deg;					// lt
	FGDouble	gps_longitude_deg;					// ln
	FGDouble	gps_amsl_ft;						// alg
	FGDouble	ahrs_pitch_deg;						// p
	FGDouble	ahrs_roll_deg;						// r
	FGDouble	ahrs_magnetic_heading_deg;			// h
	FGDouble	ahrs_true_heading_deg;				// th
	FGDouble	fpm_alpha_deg;						// fpa
	FGDouble	fpm_beta_deg;						// fpb
	FGDouble	magnetic_track_deg;					// tr
	FGBool		navigation_needles_visible;			// nav
	FGBool		vertical_deviation_ok;				// ngso
	FGDouble	vertical_deviation_deg;				// ngs
	FGBool		lateral_deviation_ok;				// nhdo
	FGDouble	lateral_deviation_deg;				// nhd
	FGBool		navigation_dme_ok;					// dok
	FGDouble	dme_distance_nmi;					// dme
	FGDouble	slip_skid_g;						// ss
	FGDouble	total_air_temperature_degc;			// tat
	FGDouble	engine_throttle_pct;				// thr
	FGDouble	engine_1_thrust_lb;					// thrust1
	FGDouble	engine_1_rpm_rpm;					// rpm1
	FGDouble	engine_1_pitch_deg;					// pitch1
	FGDouble	engine_1_epr;						// epr1
	FGDouble	engine_1_n1_pct;					// n1-1
	FGDouble	engine_1_n2_pct;					// n2-1
	FGDouble	engine_1_egt_degf;					// egt1
	FGDouble	engine_2_thrust_lb;					// thrust2
	FGDouble	engine_2_rpm_rpm;					// rpm2
	FGDouble	engine_2_pitch_deg;					// pitch2
	FGDouble	engine_2_epr;						// epr2
	FGDouble	engine_2_n1_pct;					// n1-2
	FGDouble	engine_2_n2_pct;					// n2-2
	FGDouble	engine_2_egt_degf;					// egt2
	FGDouble	wind_from_magnetic_heading_deg;		// wfh
	FGDouble	wind_tas_kt;						// ws
	FGBool		gear_setting_down;					// gd
	FGDouble	gear_nose_position;					// gdn
	FGDouble	gear_left_position;					// gdl
	FGDouble	gear_right_position;				// gdr
}
END_PACKED_STRUCT


BEGIN_PACKED_STRUCT
struct FGOutputData
{
	FGFloat		ailerons;						// a
	FGFloat		elevator;						// e
	FGFloat		rudder;							// r
	FGFloat		throttle_1;						// t1
	FGFloat		throttle_2;						// t2
	FGFloat		flaps;							// f
}
END_PACKED_STRUCT


FlightGearIO::FlightGearIO (xf::ModuleManager* module_manager, QDomElement const& config):
	Module (module_manager, config)
{
	for (QDomElement& e: config)
	{
		if (e == "input")
		{
			_input_enabled = e.attribute ("disabled") != "true";

			for (QDomElement& e2: e)
			{
				if (e2 == "host")
					_input_host = e2.text();
				else if (e2 == "port")
					_input_port = e2.text().toInt();
				else if (e2 == "properties")
				{
					parse_properties (e2, {
						{ "rotation.x", _rotation_x, false },
						{ "rotation.y", _rotation_y, false },
						{ "rotation.z", _rotation_z, false },
						{ "acceleration.x", _acceleration_x, false },
						{ "acceleration.y", _acceleration_y, false },
						{ "acceleration.z", _acceleration_z, false },
						{ "aoa.alpha.maximum", _aoa_alpha_maximum, false },
						{ "aoa.alpha.minimum", _aoa_alpha_minimum, false },
						{ "aoa.alpha", _aoa_alpha, false },
						{ "ias", _ias, false },
						{ "ias-lookahead", _ias_lookahead, false },
						{ "ias-minimum", _minimum_ias, false },
						{ "ias-maximum", _maximum_ias, false },
						{ "ias.serviceable", _ias_serviceable, false },
						{ "gs", _gs, false },
						{ "tas", _tas, false },
						{ "mach", _mach, false },
						{ "ahrs.pitch", _ahrs_pitch, false },
						{ "ahrs.roll", _ahrs_roll, false },
						{ "ahrs.heading.magnetic", _ahrs_magnetic_heading, false },
						{ "ahrs.true-heading", _ahrs_true_heading, false },
						{ "ahrs.serviceable", _ahrs_serviceable, false },
						{ "slip-skid", _slip_skid_g, false },
						{ "flight-path-marker-alpha", _fpm_alpha, false },
						{ "flight-path-marker-beta", _fpm_beta, false },
						{ "track.magnetic", _magnetic_track, false },
						{ "standard-pressure", _standard_pressure, false },
						{ "altitude", _altitude, false },
						{ "radar-altimeter.altitude.agl", _radar_altimeter_altitude_agl, false },
						{ "radar-altimeter.serviceable", _radar_altimeter_serviceable, false },
						{ "cbr", _cbr, false },
						{ "pressure", _pressure, false },
						{ "pressure.serviceable", _pressure_serviceable, false },
						{ "cmd-setting-altitude", _cmd_alt_setting, false },
						{ "cmd-setting-ias", _cmd_speed_setting, false },
						{ "cmd-setting-heading", _cmd_heading_setting, false },
						{ "cmd-setting-cbr", _cmd_cbr_setting, false },
						{ "flight-director-pitch", _flight_director_pitch, false },
						{ "flight-director-roll", _flight_director_roll, false },
						{ "navigation-needles-visible", _navigation_needles_visible, false },
						{ "lateral-deviation", _lateral_deviation, false },
						{ "vertical-deviation", _vertical_deviation, false },
						{ "dme-distance", _dme_distance, false },
						{ "total-air-temperature", _total_air_temperature, false },
						{ "engine-throttle-pct", _engine_throttle_pct, false },
						{ "engine.1.thrust", _engine_1_thrust, false },
						{ "engine.1.rpm", _engine_1_rpm, false },
						{ "engine.1.pitch", _engine_1_pitch, false },
						{ "engine.1.epr", _engine_1_epr, false },
						{ "engine.1.n1", _engine_1_n1_pct, false },
						{ "engine.1.n2", _engine_1_n2_pct, false },
						{ "engine.1.egt", _engine_1_egt, false },
						{ "engine.2.thrust", _engine_2_thrust, false },
						{ "engine.2.rpm", _engine_2_rpm, false },
						{ "engine.2.pitch", _engine_2_pitch, false },
						{ "engine.2.epr", _engine_2_epr, false },
						{ "engine.2.n1", _engine_2_n1_pct, false },
						{ "engine.2.n2", _engine_2_n2_pct, false },
						{ "engine.2.egt", _engine_2_egt, false },
						{ "gps.latitude", _gps_latitude, false },
						{ "gps.longitude", _gps_longitude, false },
						{ "gps.amsl", _gps_amsl, false },
						{ "gps.lateral.stddev", _gps_lateral_stddev, false },
						{ "gps.vertical.stddev", _gps_vertical_stddev, false },
						{ "gps.source", _gps_source, false },
						{ "gps.serviceable", _gps_serviceable, false },
						{ "wind-from-mag-heading", _wind_from_magnetic_heading, false },
						{ "wind-tas", _wind_tas, false },
						{ "gear.setting.down", _gear_setting_down, false },
						{ "gear.nose.up", _gear_nose_up, false },
						{ "gear.nose.down", _gear_nose_down, false },
						{ "gear.left.up", _gear_left_up, false },
						{ "gear.left.down", _gear_left_down, false },
						{ "gear.right.up", _gear_right_up, false },
						{ "gear.right.down", _gear_right_down, false },
					});
				}
			}
		}
		else if (e == "output")
		{
			_output_enabled = e.attribute ("disabled") != "true";

			for (QDomElement& e2: e)
			{
				if (e2 == "host")
					_output_host = e2.text();
				else if (e2 == "port")
					_output_port = e2.text().toInt();
				else if (e2 == "properties")
				{
					parse_properties (e2, {
						{ "ailerons", _ailerons, false },
						{ "elevator", _elevator, false },
						{ "rudder", _rudder, false },
						{ "throttle.1", _throttle_1, false },
						{ "throttle.2", _throttle_2, false },
						{ "flaps", _flaps, false },
					});
				}
			}
		}
	}

	_serviceable_flags = {
		&_ahrs_serviceable,
		&_ias_serviceable,
		&_radar_altimeter_serviceable,
		&_pressure_serviceable,
		&_gps_serviceable,
	};

	_output_properties = {
		&_rotation_x,
		&_rotation_y,
		&_rotation_z,
		&_acceleration_x,
		&_acceleration_y,
		&_acceleration_z,
		&_aoa_alpha_maximum,
		&_aoa_alpha_minimum,
		&_aoa_alpha,
		&_ias,
		&_ias_lookahead,
		&_minimum_ias,
		&_maximum_ias,
		&_gs,
		&_tas,
		&_mach,
		&_ahrs_pitch,
		&_ahrs_roll,
		&_ahrs_magnetic_heading,
		&_ahrs_true_heading,
		&_slip_skid_g,
		&_fpm_alpha,
		&_fpm_beta,
		&_magnetic_track,
		&_standard_pressure,
		&_altitude,
		&_radar_altimeter_altitude_agl,
		&_cbr,
		&_pressure,
		&_cmd_alt_setting,
		&_cmd_speed_setting,
		&_cmd_heading_setting,
		&_cmd_cbr_setting,
		&_flight_director_pitch,
		&_flight_director_roll,
		&_navigation_needles_visible,
		&_lateral_deviation,
		&_vertical_deviation,
		&_dme_distance,
		&_total_air_temperature,
		&_engine_throttle_pct,
		&_engine_1_thrust,
		&_engine_1_rpm,
		&_engine_1_pitch,
		&_engine_1_epr,
		&_engine_1_n1_pct,
		&_engine_1_n2_pct,
		&_engine_1_egt,
		&_engine_2_thrust,
		&_engine_2_rpm,
		&_engine_2_pitch,
		&_engine_2_epr,
		&_engine_2_n1_pct,
		&_engine_2_n2_pct,
		&_engine_2_egt,
		&_gps_latitude,
		&_gps_longitude,
		&_gps_amsl,
		&_gps_lateral_stddev,
		&_gps_vertical_stddev,
		&_wind_from_magnetic_heading,
		&_wind_tas,
		&_gear_setting_down,
		&_gear_nose_up,
		&_gear_nose_down,
		&_gear_left_up,
		&_gear_left_down,
		&_gear_right_up,
		&_gear_right_down,
	};

	_timeout_timer = std::make_unique<QTimer>();
	_timeout_timer->setSingleShot (true);
	_timeout_timer->setInterval (200);
	QObject::connect (_timeout_timer.get(), SIGNAL (timeout()), this, SLOT (invalidate_all()));

	_input = std::make_unique<QUdpSocket>();
	_input->bind (QHostAddress (_input_host), _input_port, QUdpSocket::ShareAddress);
	QObject::connect (_input.get(), SIGNAL (readyRead()), this, SLOT (got_packet()));

	_output = std::make_unique<QUdpSocket>();

	invalidate_all();
}


void
FlightGearIO::got_packet()
{
	read_input();
	write_output();
}


void
FlightGearIO::invalidate_all()
{
	for (auto property: _output_properties)
		if (property->configured())
			property->set_nil();

	for (auto flag: _serviceable_flags)
		if (flag->configured())
			flag->write (false);
}


void
FlightGearIO::read_input()
{
	while (_input->hasPendingDatagrams())
	{
		int datagram_size = _input->pendingDatagramSize();
		if (_input_datagram.size() < datagram_size)
			_input_datagram.resize (datagram_size);

		_input->readDatagram (_input_datagram.data(), datagram_size, nullptr, nullptr);

		if (!_input_enabled)
			continue;

		FGInputData* fg_data = reinterpret_cast<FGInputData*> (_input_datagram.data());

#define ASSIGN(unit, x) \
		if (_##x.configured()) \
			_##x.write (1_##unit * fg_data->x##_##unit);

#define ASSIGN_UNITLESS(x) \
		if (_##x.configured()) \
			_##x.write (fg_data->x);

		ASSIGN (ft,   cmd_alt_setting);
		ASSIGN (fpm,  cmd_cbr_setting);
		ASSIGN (kt,   cmd_speed_setting);
		ASSIGN (deg,  cmd_heading_setting);
		ASSIGN (deg,  flight_director_pitch);
		ASSIGN (deg,  flight_director_roll);
		ASSIGN (rad,  aoa_alpha_maximum);
		ASSIGN (rad,  aoa_alpha_minimum);
		ASSIGN (rad,  aoa_alpha);
		ASSIGN (kt,   ias);
		ASSIGN (kt,   tas);
		ASSIGN (kt,   gs);
		ASSIGN_UNITLESS (mach);
		ASSIGN (kt,   ias_lookahead);
		ASSIGN (kt,   maximum_ias);
		ASSIGN (kt,   minimum_ias);
		ASSIGN_UNITLESS (standard_pressure);
		ASSIGN (ft,   altitude);
		ASSIGN (ft,   radar_altimeter_altitude_agl);
		ASSIGN (inHg, pressure);
		ASSIGN (fpm,  cbr);
		ASSIGN (deg,  gps_latitude);
		ASSIGN (deg,  gps_longitude);
		ASSIGN (ft,   gps_amsl);
		ASSIGN (deg,  ahrs_pitch);
		ASSIGN (deg,  ahrs_roll);
		ASSIGN (deg,  ahrs_magnetic_heading);
		ASSIGN (deg,  ahrs_true_heading);
		ASSIGN (deg,  fpm_alpha);
		ASSIGN (deg,  fpm_beta);
		ASSIGN (deg,  magnetic_track);
		ASSIGN_UNITLESS (navigation_needles_visible);
		ASSIGN (nmi,  dme_distance);
		ASSIGN_UNITLESS (slip_skid_g);
		ASSIGN_UNITLESS (engine_throttle_pct);
		ASSIGN (rpm,  engine_1_rpm);
		ASSIGN (deg,  engine_1_pitch);
		ASSIGN_UNITLESS (engine_1_epr);
		ASSIGN_UNITLESS (engine_1_n1_pct);
		ASSIGN_UNITLESS (engine_1_n2_pct);
		ASSIGN (rpm,  engine_2_rpm);
		ASSIGN (deg,  engine_2_pitch);
		ASSIGN_UNITLESS (engine_2_epr);
		ASSIGN_UNITLESS (engine_2_n1_pct);
		ASSIGN_UNITLESS (engine_2_n2_pct);
		ASSIGN (deg,  wind_from_magnetic_heading);
		ASSIGN (kt,   wind_tas);
		ASSIGN_UNITLESS (gear_setting_down);

#undef ASSIGN_UNITLESS
#undef ASSIGN

		_rotation_x = 1_deg * fg_data->rotation_x_degps / 1_s;
		_rotation_y = 1_deg * fg_data->rotation_y_degps / 1_s;
		_rotation_z = 1_deg * fg_data->rotation_z_degps / 1_s;

		_acceleration_x = 1_ft * fg_data->acceleration_x_fps2 / 1_s / 1_s;
		_acceleration_y = 1_ft * fg_data->acceleration_y_fps2 / 1_s / 1_s;
		_acceleration_z = -1_ft * fg_data->acceleration_z_fps2 / 1_s / 1_s;

		if (_vertical_deviation.configured())
			_vertical_deviation.write (2_deg * fg_data->vertical_deviation_deg);
		if (_lateral_deviation.configured())
			_lateral_deviation.write (2_deg * fg_data->lateral_deviation_deg);

		if (!fg_data->vertical_deviation_ok && _vertical_deviation.configured())
			_vertical_deviation.set_nil();
		if (!fg_data->lateral_deviation_ok && _lateral_deviation.configured())
			_lateral_deviation.set_nil();
		if (!fg_data->navigation_dme_ok && _dme_distance.configured())
			_dme_distance.set_nil();

		_gear_nose_down.write (fg_data->gear_nose_position > 0.999);
		_gear_left_down.write (fg_data->gear_left_position > 0.999);
		_gear_right_down.write (fg_data->gear_right_position > 0.999);

		_gear_nose_up.write (fg_data->gear_nose_position < 0.001);
		_gear_left_up.write (fg_data->gear_left_position < 0.001);
		_gear_right_up.write (fg_data->gear_right_position < 0.001);

		// TAT
		if (_total_air_temperature.configured())
			_total_air_temperature.write (Quantity<Celsius> (fg_data->total_air_temperature_degc));

		// Convert EGT from °F to Kelvins:
		if (_engine_1_egt.configured())
			_engine_1_egt.write (Quantity<Fahrenheit> (fg_data->engine_1_egt_degf));
		if (_engine_2_egt.configured())
			_engine_2_egt.write (Quantity<Fahrenheit> (fg_data->engine_2_egt_degf));

		// Engine thrust:
		_engine_1_thrust = 1_lb * fg_data->engine_1_thrust_lb * 1_g;
		_engine_2_thrust = 1_lb * fg_data->engine_2_thrust_lb * 1_g;
	}

	if (_maximum_ias.valid() && *_maximum_ias < 1_kt)
		_maximum_ias.set_nil();
	if (_minimum_ias.valid() && *_minimum_ias < 1_kt)
		_minimum_ias.set_nil();
	if (_radar_altimeter_altitude_agl.valid() && *_radar_altimeter_altitude_agl > 2500_ft)
		_radar_altimeter_altitude_agl.set_nil();

	for (auto flag: _serviceable_flags)
		if (flag->configured())
			flag->write (true);

	_gps_lateral_stddev.write (1_m);
	_gps_vertical_stddev.write (1_m);
	_gps_source.write ("GPS");

	_timeout_timer->start();
}


void
FlightGearIO::write_output()
{
	if (!_output_enabled)
		return;

	FGOutputData fg_data;

#define ASSIGN(x, def) \
		fg_data.x = _##x.read (def);

	ASSIGN (ailerons, 0.0);
	ASSIGN (elevator, 0.0);
	ASSIGN (rudder, 0.0);
	ASSIGN (throttle_1, 0.0);
	ASSIGN (throttle_2, 0.0);
	ASSIGN (flaps, 0.0);

#undef ASSIGN

	_output->writeDatagram (reinterpret_cast<const char*> (&fg_data), sizeof (fg_data), QHostAddress (_output_host), _output_port);
}

