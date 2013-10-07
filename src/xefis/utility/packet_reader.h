/* vim:ts=4
 *
 * Copyleft 2008…2013  Michał Gawron
 * Marduk Unix Labs, http://mulabs.org/
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Visit http://www.gnu.org/licenses/gpl-3.0.html for more information on licensing.
 */

#ifndef XEFIS__UTILITY__PACKET_READER_H__INCLUDED
#define XEFIS__UTILITY__PACKET_READER_H__INCLUDED

// Standard:
#include <cstddef>
#include <string>
#include <functional>

// Xefis:
#include <xefis/config/all.h>
#include <xefis/utility/noncopyable.h>


namespace Xefis {

class PacketReader: private Noncopyable
{
  public:
	/**
	 * Callback should return number of parsed bytes.
	 * This number of bytes will be removed from the beginning
	 * of input buffer. If returns 0, it indicates that there
	 * was not enough data.
	 */
	typedef std::function<std::size_t()> ParseCallback;

  public:
	/**
	 * Ctor
	 * @callback will get called, whenever there's data in buffer with
	 * @magic value and when its size > minimum packet size.
	 */
	PacketReader (std::string const& magic, ParseCallback callback);

	/**
	 * Set minimum packet size in bytes. If data in the input buffer
	 * is smaller than this, parse callback will not be called.
	 * Packet size includes magic value size.
	 */
	void
	set_minimum_packet_size (std::size_t bytes) noexcept;

	/**
	 * Set maximum buffer size. If 0, buffer size will not be limited.
	 */
	void
	set_buffer_capacity (std::size_t bytes) noexcept;

	/**
	 * Feed synchronizer with input data. It will search for magic values
	 * and asks if synchronization is possible.
	 */
	void
	feed (std::string const& data);

	/**
	 * Access input buffer.
	 */
	std::string&
	buffer() noexcept;

  private:
	std::string		_magic;
	std::size_t		_minimum_packet_size	= 0;
	std::size_t		_capacity				= 0;
	std::string		_buffer;
	ParseCallback	_parse;
};


inline std::string&
PacketReader::buffer() noexcept
{
	return _buffer;
}

} // namespace Xefis

#endif
