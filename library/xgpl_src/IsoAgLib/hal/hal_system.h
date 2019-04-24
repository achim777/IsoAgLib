/*
  hal_system.h: include dependent on used target (defined in
    IsoAgLib/isoaglib_config.h) the suitable HAL specific header for
    central system functions

  (C) Copyright 2009 - 2019 by OSB AG

  See the repository-log for details on the authors and file-history.
  (Repository information can be found at <http://isoaglib.com/download>)

  Usage under Commercial License:
  Licensees with a valid commercial license may use this file
  according to their commercial license agreement. (To obtain a
  commercial license contact OSB AG via <http://isoaglib.com/en/contact>)

  Usage under GNU General Public License with exceptions for ISOAgLib:
  Alternatively (if not holding a valid commercial license)
  use, modification and distribution are subject to the GNU General
  Public License with exceptions for ISOAgLib. (See accompanying
  file LICENSE.txt or copy at <http://isoaglib.com/download/license>)
*/

/* ************************************************************ */
/** \file IsoAgLib/hal/hal_system.h
  * include dependent on used target (defined in
	  IsoAgLib/isoaglib_config.h) the suitable HAL
		specific header for central system functions.
*/
/* ************************************************************ */
#ifndef _HAL_INDEPENDENT_SYSTEM_H_
#define _HAL_INDEPENDENT_SYSTEM_H_

#include <IsoAgLib/isoaglib_config.h>
#include "hal_config.h"

//! NOTE: The HAL functions are not neccessarily thread-safe!!!
//        Consider looking at the exact implementation,
//        or use the Scheduler-Mutex to ensure thread-safety!

namespace HAL
{
  void openSystem();
  void closeSystem();

  bool isSystemOpened();

  void initWatchdog( void* config );
  void closeWatchdog();

  void triggerWatchdog();

  ecutime_t getTime(); // in [ms]

  int16_t getSnr(uint8_t *snrDat);
  int32_t getSerialNr(int16_t* pi16_errCode = NULL);

  /**
    check if D+/OnOffSwitch is active
    @return true if D+ is switched on
  */
  bool getOn_offSwitch();

  /** 
    check if it's safe to power off
    @return true -> it's safe to power off (all settings have been saved)  
  */
  bool safeToPowerOff();

  /**
    get the main power voltage
    @return voltage of power [mV]
  */
  int16_t getAdcUbat();

  /**
    get the voltage of the external reference 8.5Volt for work of external sensors
    @return voltage at external reference [mV]
  */
  int16_t getAdc_u85();

  /**
    get the voltage of the external reference 5Volt for work of external sensors
    @return voltage at external reference [mV]
  */
  int16_t getAdc_u5();

  /** allow the HAL to power down the system (or not) */
  void powerHold( bool ab_on );

  /**
    Sleep and yield CPU for other tasks.
    Especially important for Multitasking environments
    as Windows/Linux running with CAN-Server for example.
    NOTE: The sleep can also be shorter depending on the
          operating system, even no-sleep on
          single-tasking systems!
   */    
  void sleep_max_ms( uint32_t ms );
}


/// INCLUDE CONCRETE SYSTEM-HAL-IMPLEMENTATION
/// ==========================================

#ifdef HAL_PATH_ISOAGLIB_SYSTEM
#  define _hal_system_header_ <HAL_PATH_ISOAGLIB_SYSTEM/system.h>
#  include _hal_system_header_
#else
#  error "No HAL_PATH_ISOAGLIB_SYSTEM set. This is a mandatory module!"
#endif


#endif
