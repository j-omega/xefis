/* vim:ts=4
 *
 * Copyleft 2008…2012  Michał Gawron
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
#include <utility>
#include <cmath>

// Qt:
#include <QtCore/QTimer>
#include <QtGui/QApplication>
#include <QtGui/QPainter>

// Xefis:
#include <xefis/config/all.h>
#include <xefis/utility/numeric.h>
#include <xefis/utility/text_painter.h>

// Local:
#include "efis.h"


const char*	EFIS::AP = "A/P";
const char	EFIS::DIGITS[] = { '0', '1', '2', '3', '4', '5', '6', '7', '8', '9' };
const char*	EFIS::MINUS_SIGN = "−";


EFIS::AltitudeLadder::AltitudeLadder (EFIS& efis, QPainter& painter, float altitude):
	_efis (efis),
	_painter (painter),
	_text_painter (_painter, &_efis._text_painter_cache),
	_altitude (bound (altitude, -9999.f, +99999.f)),
	_extent (825.f),
	_sgn (_altitude < 0.f ? -1.f : 1.f),
	_min_shown (_altitude - _extent / 2.f),
	_max_shown (_altitude + _extent / 2.f),
	_rounded_altitude (static_cast<int> (_altitude + _sgn * 10.f) / 20 * 20),
	_ladder_rect (-0.0675f * _efis.wh(), -0.375 * _efis.wh(), 0.135 * _efis.wh(), 0.75f * _efis.wh()),
	_ladder_pen (_efis._ladder_color, _efis.pen_width (0.5f), Qt::SolidLine, Qt::SquareCap, Qt::MiterJoin),
	_white_pen (QColor (255, 255, 255), _efis.pen_width(), Qt::SolidLine, Qt::SquareCap, Qt::MiterJoin),
	_bold_white_pen (QColor (255, 255, 255), _efis.pen_width (3.f), Qt::SolidLine, Qt::SquareCap, Qt::MiterJoin),
	_negative_altitude_pen (QColor (255, 128, 128), _efis.pen_width(), Qt::SolidLine, Qt::SquareCap, Qt::MiterJoin)
{ }


void
EFIS::AltitudeLadder::paint()
{
	float const x = _ladder_rect.width() / 4.0f;

	_painter.save();

	_painter.setPen (_ladder_pen);
	_painter.setBrush (_efis._ladder_color);
	_painter.drawRect (_ladder_rect);

	paint_black_box (x, true);
	paint_ladder_scale (x);
	paint_bugs (x);
	paint_black_box (x);

	_painter.restore();
}


void
EFIS::AltitudeLadder::paint_black_box (float x, bool only_compute_black_box_rect)
{
	QFont b_font = _efis._font_20_bold;
	float const b_digit_width = _efis._font_20_digit_width;
	float const b_digit_height = _efis._font_20_digit_height;

	QFont s_font = _efis._font_16_bold;
	float const s_digit_width = _efis._font_16_digit_width;
	float const s_digit_height = _efis._font_16_digit_height;

	int const b_digits = 2;
	int const s_digits = 3;
	float const margin = 0.2f * b_digit_width;

	QRectF b_digits_box (0.f, 0.f, b_digits * b_digit_width + margin, 2.f * b_digit_height);
	QRectF s_digits_box (0.f, 0.f, s_digits * s_digit_width + margin, 2.f * b_digit_height);
	_black_box_rect = QRectF (0.f, -0.5f * b_digits_box.height(),
							  b_digits_box.width() + s_digits_box.width(), b_digits_box.height());

	if (only_compute_black_box_rect)
		return;

	b_digits_box.translate (0.f, -0.5f * b_digits_box.height());
	s_digits_box.translate (b_digits_box.width(), -0.5f * s_digits_box.height());

	_painter.save();
	_painter.translate (-0.75f * x, 0.f);

	_painter.setPen (_white_pen);
	_painter.setBrush (QBrush (QColor (0, 0, 0)));
	_painter.drawPolygon (QPolygonF()
		<< QPointF (-0.5f * x, 0.f)
		<< QPointF (0.f, -0.5f * x)
		<< _black_box_rect.topLeft()
		<< _black_box_rect.topRight()
		<< _black_box_rect.bottomRight()
		<< _black_box_rect.bottomLeft()
		<< QPointF (0.f, +0.5f * x));

	_painter.setFont (b_font);
	if (_sgn < 0.f)
		_painter.setPen (_negative_altitude_pen);

	// 11000 part of the altitude:
	QRectF box_11000 = b_digits_box.adjusted (margin, margin, 0.f, -margin);
	QString minus_sign_s = _sgn < 0.f ? MINUS_SIGN : "";
	_text_painter.drawText (box_11000, Qt::AlignVCenter | Qt::AlignRight,
							minus_sign_s + QString::number (std::abs (_rounded_altitude / 1000)));

	_painter.setFont (s_font);

	// 00100 part of the altitude:
	QRectF box_00100 = s_digits_box.adjusted (0.f, margin, -margin, -margin);
	_text_painter.drawText (box_00100, Qt::AlignVCenter | Qt::AlignLeft, QString::number (std::abs ((_rounded_altitude / 100) % 10)));

	// 00011 part of the altitude:
	QRectF box_00011 = box_00100.adjusted (s_digit_width, 0.f, 0.f, 0.f);
	QRectF box_00011_p10 = box_00011.translated (0.f, -s_digit_height);
	QRectF box_00011_m10 = box_00011.translated (0.f, +s_digit_height);
	_painter.setClipRect (box_00011);
	_painter.translate (0.f, -s_digit_height * (_rounded_altitude - _altitude) / 20.f);
	_text_painter.drawText (box_00011_p10, Qt::AlignVCenter | Qt::AlignLeft,
							QString::number (std::abs (std::fmod (_rounded_altitude / 10.f + 2.f, 10.f))) + "0");
	_text_painter.drawText (box_00011, Qt::AlignVCenter | Qt::AlignLeft,
							QString::number (std::abs (std::fmod (_rounded_altitude / 10.f, 10.f))) + "0");
	_text_painter.drawText (box_00011_m10, Qt::AlignVCenter | Qt::AlignLeft,
							QString::number (std::abs (std::fmod (_rounded_altitude / 10.f - 2.f, 10.f))) + "0");

	_painter.restore();
}


void
EFIS::AltitudeLadder::paint_ladder_scale (float x)
{
	int const line_every = 100;
	int const num_every = 200;
	int const bold_every = 500;

	QFont b_ladder_font = _efis._font_13_bold;
	float const b_ladder_digit_width = _efis._font_13_digit_width;
	float const b_ladder_digit_height = _efis._font_13_digit_height;

	QFont s_ladder_font = _efis._font_10_bold;
	float const s_ladder_digit_width = _efis._font_10_digit_width;
	float const s_ladder_digit_height = _efis._font_10_digit_height;

	// Special clipping that leaves some margin around black indicator:
	QPainterPath clip_path_m;
	clip_path_m.addRect (_black_box_rect.translated (-x, 0.f).adjusted (0.f, -0.2f * x, 0.f, +0.2f * x));
	QPainterPath clip_path;
	clip_path.addRect (_ladder_rect);
	clip_path -= clip_path_m;

	_painter.save();
	_painter.setClipPath (clip_path);
	_painter.translate (-2.f * x, 0.f);

	// -+line_every is to have drawn also numbers that barely fit the scale.
	for (int ft = (static_cast<int> (_min_shown) / line_every) * line_every - line_every;
		 ft <= _max_shown + line_every;
		 ft += line_every)
	{
		float posy = ft_to_px (ft);

		_painter.setPen (ft % bold_every == 0 ? _bold_white_pen : _white_pen);
		_painter.drawLine (QPointF (0.f, posy), QPointF (0.8f * x, posy));

		if (ft % num_every == 0)
		{
			QRectF big_text_box (1.1f * x, -0.5f * b_ladder_digit_height + posy,
								 2.f * b_ladder_digit_width, b_ladder_digit_height);
			if (std::abs (ft) / 1000 > 0)
			{
				QString big_text = QString::number (ft / 1000);
				_painter.setFont (b_ladder_font);
				_text_painter.drawText (big_text_box, Qt::AlignVCenter | Qt::AlignRight, big_text);
			}

			QString small_text = QString ("%1").arg (QString::number (ft % 1000), 3, '0');
			if (ft == 0)
				small_text = "0";
			_painter.setFont (s_ladder_font);
			QRectF small_text_box (1.1f * x + 2.1f * b_ladder_digit_width, -0.5f * s_ladder_digit_height + posy,
								   3.f * s_ladder_digit_width, s_ladder_digit_height);
			_text_painter.drawText (small_text_box, Qt::AlignVCenter | Qt::AlignRight, small_text);
			// Minus sign?
			if (ft < 0)
			{
				if (ft > -1000)
					_text_painter.drawText (small_text_box.adjusted (-s_ladder_digit_width, 0.f, 0.f, 0.f),
											Qt::AlignVCenter | Qt::AlignLeft, MINUS_SIGN);
			}
		}
	}

	_painter.restore();
}


void
EFIS::AltitudeLadder::paint_bugs (float x)
{
	_painter.save();

	// TODO regular bugs

	// AP bug:
	auto ap_bug = _efis._altitude_bugs.find (AP);
	if (ap_bug != _efis._altitude_bugs.end())
	{
		float posy = bound (ft_to_px (ap_bug->second),
							static_cast<float> (-_ladder_rect.height() / 2), static_cast<float> (_ladder_rect.height() / 2));
		QPolygonF bug_shape = QPolygonF()
			<< QPointF (0.f, 0.f)
			<< QPointF (-0.5f * x, -0.5f * x)
			<< QPointF (-0.5f * x, _black_box_rect.top())
			<< QPointF (+1.4f * x, _black_box_rect.top())
			<< QPointF (+1.4f * x, _black_box_rect.bottom())
			<< QPointF (-0.5f * x, _black_box_rect.bottom())
			<< QPointF (-0.5f * x, +0.5f * x);
		_painter.setClipRect (_ladder_rect.translated (-x, 0.f));
		_painter.translate (-2.f * x, posy);
		_painter.setBrush (Qt::NoBrush);
		_painter.setPen (QPen (_efis._autopilot_color.darker (400), _efis.pen_width (2.5f), Qt::SolidLine, Qt::SquareCap, Qt::MiterJoin));
		_painter.drawPolygon (bug_shape);
		_painter.setPen (QPen (_efis._autopilot_color, _efis.pen_width (1.5f), Qt::SolidLine, Qt::SquareCap, Qt::MiterJoin));
		_painter.drawPolygon (bug_shape);
	}

	_painter.restore();
}


EFIS::SpeedLadder::SpeedLadder (EFIS& efis, QPainter& painter, float speed):
	_efis (efis),
	_painter (painter),
	_text_painter (_painter, &_efis._text_painter_cache),
	_speed (bound (speed, 0.f, 9999.9f)),
	_extent (124.f),
	_min_shown (_speed - _extent / 2.f),
	_max_shown (_speed + _extent / 2.f),
	_rounded_speed (static_cast<int> (speed + 0.5f)),
	_ladder_rect (-0.0675f * _efis.wh(), -0.375 * _efis.wh(), 0.135 * _efis.wh(), 0.75f * _efis.wh()),
	_ladder_pen (_efis._ladder_color, _efis.pen_width (0.5f), Qt::SolidLine, Qt::SquareCap, Qt::MiterJoin),
	_white_pen (QColor (255, 255, 255), _efis.pen_width(), Qt::SolidLine, Qt::SquareCap, Qt::MiterJoin),
	_speed_bug_pen (QColor (0, 255, 0), _efis.pen_width (1.5f), Qt::SolidLine, Qt::SquareCap, Qt::MiterJoin)
{ }


void
EFIS::SpeedLadder::paint()
{
	float const x = _ladder_rect.width() / 4.0f;

	_painter.save();

	_painter.setPen (_ladder_pen);
	_painter.setBrush (_efis._ladder_color);
	_painter.drawRect (_ladder_rect);

	paint_black_box (x, true);
	paint_ladder_scale (x);
	paint_bugs (x);
	paint_black_box (x);

	_painter.restore();
}


void
EFIS::SpeedLadder::paint_black_box (float x, bool only_compute_black_box_rect)
{
	QFont actual_speed_font = _efis._font_20_bold;
	float const digit_width = _efis._font_20_digit_width;
	float const digit_height = _efis._font_20_digit_height;

	int digits = 3;
	if (_speed >= 1000.0f - 0.5f)
		digits = 4;
	float const margin = 0.2f * digit_width;

	_black_box_rect = QRectF (-digits * digit_width - 2.f * margin, -digit_height,
							  +digits * digit_width + 2.f * margin, 2.f * digit_height);

	if (only_compute_black_box_rect)
		return;

	_painter.save();
	_painter.translate (+0.75f * x, 0.f);

	_painter.setPen (_white_pen);
	_painter.setBrush (QBrush (QColor (0, 0, 0)));
	_painter.drawPolygon (QPolygonF()
		<< QPointF (+0.5f * x, 0.f)
		<< QPointF (0.f, -0.5f * x)
		<< _black_box_rect.topRight()
		<< _black_box_rect.topLeft()
		<< _black_box_rect.bottomLeft()
		<< _black_box_rect.bottomRight()
		<< QPointF (0.f, +0.5f * x));

	// 110 part of the speed:
	_painter.setFont (actual_speed_font);
	QRectF box_10 = _black_box_rect.adjusted (margin, margin, -margin - digit_width, -margin);
	_text_painter.drawText (box_10, Qt::AlignVCenter | Qt::AlignRight, QString::number (static_cast<int> (_speed + 0.5f) / 10));

	// 001 part of the speed:
	QRectF box_01 (box_10.right(), box_10.top(), digit_width, box_10.height());
	QRectF box_01_p1 = box_01.translated (0.f, -digit_height);
	QRectF box_01_m1 = box_01.translated (0.f, +digit_height);
	_painter.setClipRect (box_01);
	_painter.translate (0.f, -digit_height * (_rounded_speed - _speed));
	_text_painter.drawText (box_01_p1, Qt::AlignVCenter | Qt::AlignLeft,
							QString::number (static_cast<int> (floored_mod (1.f * _rounded_speed + 1.f, 10.f))));
	_text_painter.drawText (box_01, Qt::AlignVCenter | Qt::AlignLeft,
							QString::number (static_cast<int> (floored_mod (1.f * _rounded_speed, 10.f))));
	// Don't draw negative values:
	if (_speed > 0.5f)
		_text_painter.drawText (box_01_m1, Qt::AlignVCenter | Qt::AlignLeft,
								QString::number (static_cast<int> (floored_mod (1.f * _rounded_speed - 1.f, 10.f))));

	_painter.restore();
}


void
EFIS::SpeedLadder::paint_ladder_scale (float x)
{
	QFont ladder_font = _efis._font_13_bold;
	float const ladder_digit_width = _efis._font_13_digit_width;
	float const ladder_digit_height = _efis._font_13_digit_height;

	_painter.setFont (ladder_font);

	int const line_every = 10;
	int const num_every = 20;

	if (_min_shown < 0.f)
		_min_shown = 0.f;

	// Special clipping that leaves some margin around black indicator:
	QPainterPath clip_path_m;
	clip_path_m.addRect (_black_box_rect.translated (x, 0.f).adjusted (0.f, -0.2f * x, 0.f, +0.2f * x));
	QPainterPath clip_path;
	clip_path.addRect (_ladder_rect);
	clip_path -= clip_path_m;

	_painter.save();
	_painter.setClipPath (clip_path);
	_painter.translate (2.f * x, 0.f);

	_painter.setPen (_white_pen);
	// -+line_every is to have drawn also numbers that barely fit the scale.
	for (int kt = (static_cast<int> (_min_shown) / line_every) * line_every - line_every;
		 kt <= _max_shown + line_every;
		 kt += line_every)
	{
		if (kt < 0)
			continue;
		float posy = kt_to_px (kt);
		_painter.drawLine (QPointF (-0.8f * x, posy), QPointF (0.f, posy));

		if (kt % num_every == 0)
			_text_painter.drawText (QRectF (-4.f * ladder_digit_width - 1.25f * x, -0.5f * ladder_digit_height + posy,
											+4.f * ladder_digit_width, ladder_digit_height),
									Qt::AlignVCenter | Qt::AlignRight, QString::number (kt));
	}

	_painter.restore();
}


void
EFIS::SpeedLadder::paint_bugs (float x)
{
	QFont speed_bug_font = _efis._font_10_bold;
	float const speed_bug_digit_height = _efis._font_10_digit_height;

	_painter.save();
	_painter.setFont (speed_bug_font);

	for (auto& bug: _efis._speed_bugs)
	{
		// AP bug should be drawn last, to be on top:
		if (bug.first == AP)
			continue;

		if (bug.second > _min_shown && bug.second < _max_shown)
		{
			float posy = kt_to_px (bug.second);
			_painter.setPen (_speed_bug_pen);
			_painter.setClipRect (_ladder_rect.translated (x, 0.f));
			_painter.drawLine (QPointF (1.5f * x, posy), QPointF (2.25f * x, posy));
			_painter.setClipping (false);
			_text_painter.drawText (QRectF (2.5f * x, posy - 0.5f * speed_bug_digit_height,
											2.f * x, speed_bug_digit_height),
									Qt::AlignVCenter | Qt::AlignLeft, bug.first);
		}
	}

	// AP bug:
	auto ap_bug = _efis._speed_bugs.find (AP);
	if (ap_bug != _efis._speed_bugs.end())
	{
		float posy = bound (kt_to_px (ap_bug->second),
							static_cast<float> (-_ladder_rect.height() / 2.f), static_cast<float> (_ladder_rect.height() / 2.f));
		QPolygonF bug_shape = QPolygonF()
			<< QPointF (0.f, 0.f)
			<< QPointF (+0.5f * x, -0.5f * x)
			<< QPointF (2.f * x, -0.5f * x)
			<< QPointF (2.f * x, +0.5f * x)
			<< QPointF (+0.5f * x, +0.5f * x);
		_painter.setClipRect (_ladder_rect.translated (2.5f * x, 0.f));
		_painter.translate (1.25f * x, posy);
		_painter.setBrush (Qt::NoBrush);
		_painter.setPen (QPen (_efis._autopilot_color.darker (400), _efis.pen_width (2.5f), Qt::SolidLine, Qt::SquareCap, Qt::MiterJoin));
		_painter.drawPolygon (bug_shape);
		_painter.setPen (QPen (_efis._autopilot_color, _efis.pen_width (1.5f), Qt::SolidLine, Qt::SquareCap, Qt::MiterJoin));
		_painter.drawPolygon (bug_shape);
	}

	_painter.restore();
}


EFIS::EFIS (QWidget* parent):
	QWidget (parent, nullptr)
{
	setAttribute (Qt::WA_NoBackground);
	_sky_color.setHsv (213, 217, 255);
	_ground_color.setHsv (34, 233, 127);
	_ladder_color = QColor (16, 0, 67, 0x60);
	_autopilot_color = QColor (250, 140, 255);
	_navigation_color = QColor (0, 255, 0);
	_ladder_border_color = QColor (0, 0, 0, 0x70);
	_font = QApplication::font();

	_input = new QUdpSocket (this);
	_input->bind (QHostAddress::LocalHost, 9000, QUdpSocket::ShareAddress);
	QObject::connect (_input, SIGNAL (readyRead()), this, SLOT (read_input()));

	_input_alert_timer = new QTimer (this);
	_input_alert_timer->setSingleShot (true);
	QObject::connect (_input_alert_timer, SIGNAL (timeout()), this, SLOT (input_timeout()));

	_input_alert_hide_timer = new QTimer (this);
	_input_alert_hide_timer->setSingleShot (true);
	QObject::connect (_input_alert_hide_timer, SIGNAL (timeout()), this, SLOT (input_ok()));

	// Default alert timeout:
	set_input_alert_timeout (0.15f);
	update_fonts();
}


void
EFIS::set_input_alert_timeout (Seconds timeout)
{
	_input_alert_timeout = timeout;

	if (_input_alert_timeout > 0.f)
		_input_alert_timer->start (_input_alert_timeout * 1000.f);
	else
	{
		_show_input_alert = false;
		_input_alert_timer->stop();
		_input_alert_hide_timer->stop();
		update();
	}
}


void
EFIS::read_input()
{
	while (_input->hasPendingDatagrams())
	{
		QByteArray datagram;
		datagram.resize (_input->pendingDatagramSize());
		QHostAddress sender_host;
		uint16_t sender_port;

		_input->readDatagram (datagram.data(), datagram.size(), &sender_host, &sender_port);

		QString line (datagram);
		for (QString pair: QString (datagram).split (',', QString::SkipEmptyParts))
		{
			bool no_control = false;

			QStringList split_pair = pair.split ('=');
			if (split_pair.size() != 2)
				continue;
			QString var = split_pair[0];
			QString value = split_pair[1];

			if (var == "ias")
				set_ias (value.toFloat());
			else if (var == "heading")
				set_heading (value.toFloat());
			else if (var == "altitude")
				set_altitude (value.toFloat());
			else if (var == "cbr")
				set_climb_rate (value.toFloat());
			else if (var == "pitch")
				set_pitch (value.toFloat());
			else if (var == "roll")
				set_roll (value.toFloat());
			else
				no_control = true;
//			else if (var == "latitude")
//				set_latitude (value.toFloat());
//			else if (var == "longitude")
//				set_longitude (value.toFloat());
//			else if (var == "time")
//				set_time (value.toInt());

			if (no_control)
			{
				if (_input_alert_timeout > 0.f)
					_input_alert_timer->start (_input_alert_timeout * 1000.f);
			}
			else if (_show_input_alert && !_input_alert_hide_timer->isActive())
				_input_alert_hide_timer->start (350);
		}
	}
}


void
EFIS::input_timeout()
{
	_input_alert_hide_timer->stop();
	_show_input_alert = true;
	update();
}


void
EFIS::input_ok()
{
	_show_input_alert = false;
	update();
}


void
EFIS::paintEvent (QPaintEvent* paint_event)
{
	float const w = width();
	float const h = height();

	float p = floored_mod (pitch() + 180.f, 360.f) - 180.f;
	float r = floored_mod (roll() + 180.f, 360.f) - 180.f;
	float hdg = floored_mod (heading(), 360.f);

	// Mirroring, eg. -180° pitch is the same
	// as 0° pitch with roll inverted:
	if (p < -90.f)
	{
		p = -180.f - p;
		r = +180.f - r;
	}
	else if (p > 90.f)
	{
		p = +180.f - p;
		r = +180.f - r;
	}

	// Prepare useful transforms:
	_center_transform.reset();
	_center_transform.translate (w / 2.f, h / 2.f);

	_pitch_transform.reset();
	_pitch_transform.translate (0.f, -pitch_to_px (p));

	_roll_transform.reset();
	_roll_transform.rotate (-r);

	_heading_transform.reset();
	_heading_transform.translate (-heading_to_px (hdg), 0.f);

	// Total transform of horizon (heading is not really necessary here):
	_horizon_transform = _pitch_transform * _roll_transform;

	// Draw on buffer:
	QPixmap buffer (w, h);
	QPainter painter (&buffer);
	painter.setRenderHint (QPainter::Antialiasing, true);
	painter.setRenderHint (QPainter::TextAntialiasing, true);
	painter.setRenderHint (QPainter::SmoothPixmapTransform, true);
	painter.setRenderHint (QPainter::NonCosmeticDefaultPen, true);

	paint_horizon (painter);
	paint_pitch_scale (painter);
	paint_heading (painter);
	paint_roll (painter);
	paint_center_cross (painter);

	painter.save();
	SpeedLadder sl (*this, painter, _ias);
	painter.setTransform (_center_transform);
	painter.translate (-0.4f * wh(), 0.f);
	sl.paint();
	painter.restore();

	painter.save();
	AltitudeLadder al (*this, painter, _altitude);
	painter.setTransform (_center_transform);
	painter.translate (+0.4f * wh(), 0.f);
	al.paint();
	painter.restore();

	paint_climb_rate (painter);
	paint_pressure (painter);

	if (_show_input_alert)
		paint_input_alert (painter);

	// Copy buffer to screen:
	QPainter (this).drawPixmap (paint_event->rect().topLeft(), buffer, paint_event->rect());
}


void
EFIS::resizeEvent (QResizeEvent*)
{
	update_fonts();
}


void
EFIS::paint_horizon (QPainter& painter)
{
	painter.save();
	painter.setTransform (_horizon_transform * _center_transform);

	float const max = std::max (width(), height());
	float const w_max = 2.f * max;
	float const h_max = 10.f * max;
	// Sky and ground:
	painter.fillRect (-w_max, -h_max, 2.f * w_max, h_max + 1.f, QBrush (_sky_color, Qt::SolidPattern));
	painter.fillRect (-w_max, 0.f, 2.f * w_max, h_max, QBrush (_ground_color, Qt::SolidPattern));

	painter.restore();
}


void
EFIS::paint_pitch_scale (QPainter& painter)
{
	// TODO don't draw invisible lines/numbers

	QFont font = _font;
	font.setPixelSize (font_size (10.f));
	font.setBold (true);

	float const w = std::min (width(), height()) * 2.f / 9.f;
	float const z = 0.5f * w;
	float const fpxs = font.pixelSize();

	painter.save();

	// Clip rectangle before and after rotation:
	painter.setTransform (_center_transform);
	painter.setClipPath (get_pitch_scale_clipping_path());
	painter.setTransform (_roll_transform * _center_transform);
	painter.setClipRect (QRectF (-w, -0.9f * w, 2.f * w, 2.2f * w), Qt::IntersectClip);
	painter.setTransform (_horizon_transform * _center_transform);
	painter.setFont (font);

	TextPainter text_painter (painter, &_text_painter_cache);

	painter.setPen (QPen (QColor (255, 255, 255), pen_width(), Qt::SolidLine));
	// 10° lines, exclude +/-90°:
	for (int deg = -180; deg < 180; deg += 10)
	{
		if (deg == -90 || deg == 0 || deg == 90)
			continue;
		float d = pitch_to_px (deg);
		painter.drawLine (QPointF (-z, d), QPointF (z, d));
		// Degs number:
		int abs_deg = std::abs (deg);
		QString deg_t = QString::number (abs_deg > 90 ? 180 - abs_deg : abs_deg);
		// Text:
		QRectF lbox = QRectF (-z - 4.25f * fpxs, d - 0.5f * fpxs, 4.f * fpxs, fpxs);
		QRectF rbox = QRectF (+z + 0.25f * fpxs, d - 0.5f * fpxs, 4.f * fpxs, fpxs);
		text_painter.drawText (lbox, Qt::AlignVCenter | Qt::AlignRight, deg_t);
		text_painter.drawText (rbox, Qt::AlignVCenter | Qt::AlignLeft, deg_t);
	}
	// 5° lines:
	for (int deg = -180; deg < 180; deg += 5)
	{
		if (deg % 10 == 0)
			continue;
		float d = pitch_to_px (deg);
		painter.drawLine (QPointF (-z / 2.f, d), QPointF (z / 2.f, d));
	}
	// 2.5° lines:
	for (int deg = -1800; deg < 1800; deg += 25)
	{
		if (deg % 50 == 0)
			continue;
		float d = pitch_to_px (deg / 10.f);
		painter.drawLine (QPointF (-z / 4.f, d), QPointF (z / 4.f, d));
	}

	painter.setPen (QPen (QColor (255, 255, 255), pen_width (1.75f), Qt::SolidLine));
	// -90°, 90° lines:
	for (float deg: { -90.f, 90.f })
	{
		float d = pitch_to_px (deg);
		painter.drawLine (QPointF (-z * 1.5f, d), QPointF (z * 1.5f, d));
		QRectF lbox = QRectF (-1.5f * z - 4.25f * fpxs, d - 0.5f * fpxs, 4.f * fpxs, fpxs);
		QRectF rbox = QRectF (+1.5f * z + 0.25f * fpxs, d - 0.5f * fpxs, 4.f * fpxs, fpxs);
		text_painter.drawText (lbox, Qt::AlignVCenter | Qt::AlignRight, "90");
		text_painter.drawText (rbox, Qt::AlignVCenter | Qt::AlignLeft, "90");
	}

	painter.restore();
}


void
EFIS::paint_heading (QPainter& painter)
{
	float const w = std::min (width(), height()) * 2.25f / 9.f;
	float const fpxs = _font_10_bold.pixelSize();

	painter.save();
	// Clip rectangle before and after rotation:
	painter.setTransform (_center_transform);
	painter.setClipPath (get_pitch_scale_clipping_path());
	painter.setTransform (_roll_transform * _center_transform);
	painter.setClipRect (QRectF (-1.1f * w, -0.8f * w, 2.2f * w, 1.9f * w), Qt::IntersectClip);
	painter.setTransform (_horizon_transform * _center_transform);
	painter.setFont (_font_10_bold);

	TextPainter text_painter (painter, &_text_painter_cache);

	painter.setTransform (_horizon_transform * _center_transform);
	painter.setPen (QPen (QColor (255, 255, 255), pen_width (1.25f), Qt::SolidLine));
	painter.drawLine (QPointF (-1.25 * w, 0.f), QPointF (1.25f * w, 0.f));
	painter.setPen (QPen (QColor (255, 255, 255), pen_width(), Qt::SolidLine));

	painter.setTransform (_heading_transform * _horizon_transform * _center_transform);
	for (int deg = -360; deg < 450; deg += 10)
	{
		float d10 = heading_to_px (deg);
		float d05 = heading_to_px (deg + 5);
		QString text = QString::number (floored_mod (1.f * deg, 360.f) / 10);
		if (text == "0")
			text = "N";
		else if (text == "9")
			text = "E";
		else if (text == "18")
			text = "S";
		else if (text == "27")
			text = "W";
		// 10° lines:
		painter.drawLine (QPointF (d10, -w / 18.f), QPointF (d10, 0.f));
		text_painter.drawText (QRectF (d10 - 2.f * fpxs, 0.05f * fpxs, 4.f * fpxs, fpxs),
							   Qt::AlignVCenter | Qt::AlignHCenter, text);
		// 5° lines:
		painter.drawLine (QPointF (d05, -w / 36.f), QPointF (d05, 0.f));
	}

	painter.restore();
}


void
EFIS::paint_roll (QPainter& painter)
{
	float const w = std::min (width(), height()) * 3.f / 9.f;

	painter.save();

	QPen pen (QColor (255, 255, 255), pen_width(), Qt::SolidLine, Qt::SquareCap, Qt::MiterJoin);
	painter.setPen (pen);
	painter.setBrush (QBrush (QColor (255, 255, 255)));

	painter.setTransform (_center_transform);
	painter.setClipRect (QRectF (-w, -w, 2.f * w, 2.25f * w));
	for (float deg: { -60.f, -45.f, -30.f, -20.f, -10.f, 0.f, +10.f, +20.f, +30.f, +45.f, +60.f })
	{
		painter.setTransform (_center_transform);
		painter.rotate (1.f * deg);
		painter.translate (0.f, -0.795f * w);

		if (deg == 0.f)
		{
			// Triangle:
			QPointF p0 (0.f, 0.f);
			QPointF px (0.025f * w, 0.f);
			QPointF py (0.f, 0.05f * w);
			painter.drawPolygon (QPolygonF() << p0 << p0 - px - py << p0 + px - py);
		}
		else
		{
			float length = -0.05f * w;
			if (std::abs (std::fmod (deg, 30.f)) < 1.f)
				length *= 2.f;
			painter.drawLine (QPointF (0.f, 0.f), QPointF (0.f, length));
		}
	}

	float const bold_width = pen_width (3.f);
	QPointF a (0, 0.01f * w); // Miter
	QPointF b (-0.052f * w, 0.1f * w);
	QPointF c (+0.052f * w, 0.1f * w);
	QPointF x0 (0.001f * w, 0.f);
	QPointF y0 (0.f, 0.005f * w);
	QPointF x1 (0.001f * w, 0.f);
	QPointF y1 (0.f, 1.f * bold_width);

	painter.setTransform (_roll_transform * _center_transform);
	painter.translate (0.f, -0.79f * w);
	painter.setBrush (QBrush (QColor (255, 255, 255)));
	painter.drawPolyline (QPolygonF() << b << a << c);
	painter.drawPolygon (QPolygonF() << b - x0 + y0 << b + x1 + y1 << c - x1 + y1 << c + x0 + y0);

	painter.restore();
}


void
EFIS::paint_center_cross (QPainter& painter)
{
	float const w = std::min (width(), height()) * 3.f / 9.f;

	QPen white_pen (QColor (255, 255, 255), pen_width (1.5f), Qt::SolidLine, Qt::SquareCap, Qt::MiterJoin);

	painter.save();

	painter.setTransform (_center_transform);
	painter.setPen (white_pen);
	painter.setBrush (QBrush (QColor (0, 0, 0)));

	QPointF x (0.025f * w, 0.f);
	QPointF y (0.f, 0.025f * w);
	QPolygonF a = QPolygonF()
		<< -x - y
		<<  x - y
		<<  x + y
		<< -x + y;
	QPolygonF b = QPolygonF()
		<< -25.f * x - y
		<< -11.f * x - y
		<< -11.f * x + 4.f * y
		<< -13.f * x + 4.f * y
		<< -13.f * x + y
		<< -25.f * x + y;

	painter.drawPolygon (a);
	painter.drawPolygon (b);
	painter.scale (-1.f, 1.f);
	painter.drawPolygon (b);

	painter.restore();
}


void
EFIS::paint_climb_rate (QPainter& painter)
{
	float const w = std::min (width(), height()) * 3.5f / 9.f;
	float const box_w = 0.35f * w;

	QPen bold_white_pen = get_pen (QColor (255, 255, 255), 1.25f);
	QPen thin_white_pen = get_pen (QColor (255, 255, 255), 0.50f);
	QPen ladder_pen = get_pen (_ladder_color, 1.f);
	QBrush ladder_brush (_ladder_color);

	painter.save();

	float const x = w / 18.f;
	float const y = x * 4.f;

	painter.setTransform (_center_transform);
	painter.translate (0.9f * w + box_w + 1.5 * x, 0.f);

	painter.setPen (ladder_pen);
	painter.setBrush (ladder_brush);
	painter.drawPolygon (QPolygonF()
		<< QPointF (0.0f, -0.9 * y)
		<< QPointF (-1.5f * x, -0.9 * y - x)
		<< QPointF (-1.5f * x, -2.85f * y - x)
		<< QPointF (+0.5f * x, -2.85f * y - x)
		<< QPointF (2.5f * x, -1.5f * y - x)
		<< QPointF (2.5f * x, +1.5f * y + x)
		<< QPointF (+0.5f * x, +2.85f * y + x)
		<< QPointF (-1.5f * x, +2.85f * y + x)
		<< QPointF (-1.5f * x, +0.9 * y + x)
		<< QPointF (0.0f, +0.9 * y));

	TextPainter text_painter (painter, &_text_painter_cache);

	float const line_w = 0.35 * x;

	painter.setFont (_font_10_bold);
	painter.setPen (bold_white_pen);
	painter.drawLine (QPointF (0.f, 0.f), QPointF (x, 0.f));
	for (float kfpm: { -6.f, -2.f, -1.f, +1.f, +2.f, +6.f })
	{
		float posy = -2.75f * y * scale_cbr (kfpm * 1000.f);
		QRectF num_rect (-1.5f * x, posy - x, 1.3f * x, 2.f * x);
		painter.drawLine (QPointF (0.f, posy), QPointF (line_w, posy));
		text_painter.drawText (num_rect, Qt::AlignVCenter | Qt::AlignRight, QString::number (std::abs (static_cast<int> (kfpm))));
	}
	painter.setPen (thin_white_pen);
	for (float kfpm: { -4.f, -1.5f, -0.5f, +0.5f, +1.5f, +4.f })
	{
		float posy = -2.75f * y * scale_cbr (kfpm * 1000.f);
		painter.drawLine (QPointF (0.f, posy), QPointF (line_w, posy));
	}

	painter.setClipRect (QRectF (0.5f * x, -2.75f * y - x,
								 2.0f * x, 5.5f * y + 2.f * x));
	painter.setPen (bold_white_pen);
	painter.drawLine (QPointF (5.f * x, 0.f), QPointF (line_w, -2.75f * y * scale_cbr (_cbr)));

	painter.restore();
}


void
EFIS::paint_pressure (QPainter& painter)
{
}


void
EFIS::paint_input_alert (QPainter& painter)
{
	painter.save();

	QFont font = _font;
	font.setPixelSize (font_size (30.f));
	font.setBold (true);

	QString alert = "NO INPUT";

	QFontMetrics font_metrics (font);
	int width = font_metrics.width (alert);

	QPen pen (QColor (255, 255, 255), pen_width (2.f), Qt::SolidLine, Qt::SquareCap, Qt::MiterJoin);

	painter.setTransform (_center_transform);
	painter.setPen (pen);
	painter.setBrush (QBrush (QColor (0xdd, 0, 0, 0xdd)));
	painter.setFont (font);

	QRectF rect (-0.6f * width, 0.5f * height() - 1.4f * font_metrics.height(), 1.2f * width, 1.2f * font_metrics.height());

	painter.drawRect (rect);
	painter.drawText (rect, Qt::AlignVCenter | Qt::AlignHCenter, alert);

	painter.restore();
}


QPainterPath
EFIS::get_pitch_scale_clipping_path() const
{
	float const w = std::min (width(), height()) * 2.f / 9.f;

	QPainterPath clip_path;
	clip_path.setFillRule (Qt::WindingFill);
	clip_path.addEllipse (QRectF (-1.15f * w, -1.175f * w, 2.30f * w, 2.35f * w));
	clip_path.addRect (QRectF (-1.15f * w, 0.f, 2.30f * w, 1.375f * w));

	return clip_path;
}


float
EFIS::scale_cbr (Feet climb_rate) const
{
	Feet cbr = std::abs (climb_rate);

	if (cbr < 1000.f)
		cbr = cbr / 1000.f * 0.46f;
	else if (cbr < 2000)
		cbr = 0.46f + 0.32f * (cbr - 1000.f) / 1000.f;
	else if (cbr < 6000)
		cbr = 0.78f + 0.22f * (cbr - 2000.f) / 4000.f;
	else
		cbr = 1.f;

	if (climb_rate < 0.f)
		cbr *= -1.f;

	return cbr;
}


int
EFIS::get_digit_width (QFont& font) const
{
	QFontMetrics font_metrics (font);
	int digit_width = 0;
	for (char c: DIGITS)
		digit_width = std::max (digit_width, font_metrics.width (c));
	return digit_width;
}


void
EFIS::update_fonts()
{
	float const height_scale_factor = 0.7f;

	_font_10_bold = _font;
	_font_10_bold.setPixelSize (font_size (10.f));
	_font_10_bold.setBold (true);
	_font_10_digit_width = get_digit_width (_font_10_bold);
	_font_10_digit_height = height_scale_factor * QFontMetrics (_font_10_bold).height();

	_font_13_bold = _font;
	_font_13_bold.setPixelSize (font_size (13.f));
	_font_13_bold.setBold (true);
	_font_13_digit_width = get_digit_width (_font_13_bold);
	_font_13_digit_height = QFontMetrics (_font_13_bold).height();
	_font_13_digit_height = height_scale_factor * QFontMetrics (_font_13_bold).height();

	_font_16_bold = _font;
	_font_16_bold.setPixelSize (font_size (16.f));
	_font_16_bold.setBold (true);
	_font_16_digit_width = get_digit_width (_font_16_bold);
	_font_16_digit_height = QFontMetrics (_font_16_bold).height();
	_font_16_digit_height = height_scale_factor * QFontMetrics (_font_16_bold).height();

	_font_20_bold = _font;
	_font_20_bold.setPixelSize (font_size (20.f));
	_font_20_bold.setBold (true);
	_font_20_digit_width = get_digit_width (_font_20_bold);
	_font_20_digit_height = QFontMetrics (_font_20_bold).height();
	_font_20_digit_height = height_scale_factor * QFontMetrics (_font_20_bold).height();
}

