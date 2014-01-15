/* vim:ts=4
 *
 * Copyleft 2012…2013  Michał Gawron
 * Marduk Unix Labs, http://mulabs.org/
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Visit http://www.gnu.org/licenses/gpl-3.0.html for more information on licensing.
 */

#ifndef XEFIS__WIDGETS__GROUP_BOX_H__INCLUDED
#define XEFIS__WIDGETS__GROUP_BOX_H__INCLUDED

// Standard:
#include <cstddef>
#include <array>

// Qt:
#include <QtWidgets/QWidget>

// Xefis:
#include <xefis/config/all.h>
#include <xefis/core/instrument.h>
#include <xefis/core/instrument_aids.h>


namespace Xefis {

class GroupBox:
	public QWidget,
	public Xefis::InstrumentAids
{
  public:
	// Ctor
	GroupBox (QString const& label, QWidget* parent);

	/**
	 * Set padding around contents.
	 */
	void
	set_padding (std::array<int, 4> const& padding);

  protected:
	// QWidget API
	void
	resizeEvent (QResizeEvent*) override;

	// QWidget API
	void
	paintEvent (QPaintEvent*) override;

  private:
	QString				_label;
	QColor				_label_color	= { 0x00, 0xdd, 0xff };
	QColor				_frame_color	= { 0x00, 0xb0, 0xcc };
	std::array<int, 4>	_padding		= { { 0, 0, 0, 0 } };
};

} // namespace Xefis

#endif
