
/*
 * Ekiga -- A VoIP and Video-Conferencing application
 * Copyright (C) 2000-2009 Damien Sandras <dsandras@seconix.com>

 * This program is free software; you can  redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or (at
 * your option) any later version. This program is distributed in the hope
 * that it will be useful, but WITHOUT ANY WARRANTY; without even the
 * implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 * Ekiga is licensed under the GPL license and as a special exception, you
 * have permission to link or otherwise combine this program with the
 * programs OPAL, OpenH323 and PWLIB, and distribute the combination, without
 * applying the requirements of the GNU GPL to the OPAL, OpenH323 and PWLIB
 * programs, as long as you do follow the requirements of the GNU GPL for all
 * the rest of the software thus combined.
 */


/*
 *                         runtime.h  -  description
 *                         ------------------------------------------
 *   begin                : written in 2007 by Julien Puydt
 *   copyright            : (c) 2007 by Julien Puydt
 *                          (c) 2007 by Damien Sandras
 *   description          : Threading helper functions
 *
 */

#include <boost/signals2.hpp>
#include <boost/bind.hpp>

#ifndef __RUNTIME_H__
#define __RUNTIME_H__

#include "services.h"

namespace Ekiga
{

  /**
   * @addtogroup services
   * @{
   */

  namespace Runtime
  {
    void init (); // depends on the implementation

    void run (); // depends on the implementation

    void quit (); // depends on the implementation

    void run_in_main (boost::function0<void> action,
		      unsigned int seconds = 0); // depends on the implementation
  };

  /**
   * @}
   */

};

#endif
