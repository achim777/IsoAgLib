/***************************************************************************
                 tracmove_c.cpp - working on Base Data Msg Type 1;
                                  stores, updates  and delivers all base
                                  data informations from CANCustomer_c
                                  derived for CAN sending and receiving
                                  interaction;
                                  from ElementBase_c derived for
                                  interaction with other IsoAgLib objects
                             -------------------
    begin                 Fri Apr 07 2000
    copyright            : (C) 2000 - 2004 by Dipl.-Inform. Achim Spangler
    email                : a.spangler@osb-ag:de
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 * This file is part of the "IsoAgLib", an object oriented program library *
 * to serve as a software layer between application specific program and   *
 * communication protocol details. By providing simple function calls for  *
 * jobs like starting a measuring program for a process data value on a    *
 * remote ECU, the main program has not to deal with single CAN telegram   *
 * formatting. This way communication problems between ECU's which use     *
 * this library should be prevented.                                       *
 * Everybody and every company is invited to use this library to make a    *
 * working plug and play standard out of the printed protocol standard.    *
 *                                                                         *
 * Copyright (C) 2000 - 2004 Dipl.-Inform. Achim Spangler                  *
 *                                                                         *
 * The IsoAgLib is free software; you can redistribute it and/or modify it *
 * under the terms of the GNU General Public License as published          *
 * by the Free Software Foundation; either version 2 of the License, or    *
 * (at your option) any later version.                                     *
 *                                                                         *
 * This library is distributed in the hope that it will be useful, but     *
 * WITHOUT ANY WARRANTY; without even the implied warranty of              *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU       *
 * General Public License for more details.                                *
 *                                                                         *
 * You should have received a copy of the GNU General Public License       *
 * along with IsoAgLib; if not, write to the Free Software Foundation,     *
 * Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA           *
 *                                                                         *
 * As a special exception, if other files instantiate templates or use     *
 * macros or inline functions from this file, or you compile this file and *
 * link it with other works to produce a work based on this file, this file*
 * does not by itself cause the resulting work to be covered by the GNU    *
 * General Public License. However the source code for this file must still*
 * be made available in accordance with section (3) of the                 *
 * GNU General Public License.                                             *
 *                                                                         *
 * This exception does not invalidate any other reasons why a work based on*
 * this file might be covered by the GNU General Public License.           *
 *                                                                         *
 * Alternative licenses for IsoAgLib may be arranged by contacting         *
 * the main author Achim Spangler by a.spangler@osb-ag:de                  *
 ***************************************************************************/

 /**************************************************************************
 *                                                                         *
 *     ###    !!!    ---    ===    IMPORTANT    ===    ---    !!!    ###   *
 * Each software module, which accesses directly elements of this file,    *
 * is considered to be an extension of IsoAgLib and is thus covered by the *
 * GPL license. Applications must use only the interface definition out-   *
 * side :impl: subdirectories. Never access direct elements of __IsoAgLib  *
 * and __HAL namespaces from applications which shouldnt be affected by    *
 * the license. Only access their interface counterparts in the IsoAgLib   *
 * and HAL namespaces. Contact a.spangler@osb-ag:de in case your applicat- *
 * ion really needs access to a part of an internal namespace, so that the *
 * interface might be extended if your request is accepted.                *
 *                                                                         *
 * Definition of direct access:                                            *
 * - Instantiation of a variable with a datatype from internal namespace   *
 * - Call of a (member-) function                                          *
 * Allowed is:                                                             *
 * - Instatiation of a variable with a datatype from interface namespace,  *
 *   even if this is derived from a base class inside an internal namespace*
 * - Call of member functions which are defined in the interface class     *
 *   definition ( header )                                                 *
 *                                                                         *
 * Pairing of internal and interface classes:                              *
 * - Internal implementation in an :impl: subdirectory                     *
 * - Interface in the parent directory of the corresponding internal class *
 * - Interface class name IsoAgLib::iFoo_c maps to the internal class      *
 *   __IsoAgLib::Foo_c                                                     *
 *                                                                         *
 * AS A RULE: Use only classes with names beginning with small letter :i:  *
 ***************************************************************************/
#include <IsoAgLib/driver/system/impl/system_c.h>
#include <IsoAgLib/driver/can/impl/canio_c.h>
#include <IsoAgLib/comm/Scheduler/impl/scheduler_c.h>
#include <IsoAgLib/comm/SystemMgmt/impl/systemmgmt_c.h>
#include <IsoAgLib/comm/SystemMgmt/impl/istate_c.h>
#ifdef USE_DIN_9684
  #include <IsoAgLib/comm/SystemMgmt/DIN9684/impl/dinmonitor_c.h>
#endif
#ifdef USE_ISO_11783
  #include <IsoAgLib/comm/SystemMgmt/ISO11783/impl/isomonitor_c.h>
#endif
#include <IsoAgLib/util/config.h>
#include <IsoAgLib/util/iutil_funcs.h>
#include <IsoAgLib/comm/Base/itracgeneral_c.h>
#include "tracmove_c.h"

using namespace std;

namespace __IsoAgLib { // Begin Namespace __IsoAglib

  #if defined(PRT_INSTANCE_CNT) && (PRT_INSTANCE_CNT > 1)
  /** C-style function, to get access to the unique TracMove_c singleton instance
    * if more than one CAN BUS is used for IsoAgLib, an index must be given to select the wanted BUS
    */
  TracMove_c& getTracMoveInstance( uint8_t rui8_instance )
  { // if > 1 singleton instance is used, no static reference can be used
    return TracMove_c::instance( rui8_instance );
  };
  #else
  /** C-style function, to get access to the unique Base_c singleton instance */
  TracMove_c& getTracMoveInstance( void )
  {
    static TracMove_c& c_lbsTracMove = TracMove_c::instance();
    return c_lbsTracMove;
  };
  #endif

  void TracMove_c::singletonInit()
  {
    setAlreadyClosed();
    init( NULL, IsoAgLib::BaseDataNothing );
  }

  /** initialise element which can't be done during construct;
      above all create the needed FilterBox_c instances, to receive
      the needed CAN msg with base msg type 1,2 and calendar
      possible errors:
        * dependant error in CANIO_c problems during insertion of new FilterBox_c entries for IsoAgLibBase
      @param rpc_devKey optional pointer to the DEV_KEY variable of the ersponsible member instance (pointer enables automatic value update if var val is changed)
      @param rt_mySendSelection optional Bitmask of base data to send ( default send nothing )
    */
  void TracMove_c::init(const DevKey_c* rpc_devKey, IsoAgLib::BaseDataGroup_t rt_mySendSelection)
  {
    // first register in Scheduler_c
    getSchedulerInstance4Comm().registerClient( this );
    c_data.setSingletonKey( getSingletonVecKey() );

    // set the member base msg value vars to NO_VAL codes
      i16_speedReal = i16_speedTheor = NO_VAL_16S;

    // set distance value to 0
    i16_lastDistReal = i16_lastDistTheor = 0;
    i32_distReal = i32_distTheor = 0;

    // set the timestamps to 0
    i32_lastBase1 = 0;
    #ifdef USE_DIN_9684
    if (checkAlreadyClosed())
    {
      b_dinFilterCreated = false;
    }
    #endif

    #ifdef USE_ISO_11783
    if (checkAlreadyClosed())
    {
      b_isoFilterCreated = false;
    }
    #endif

    // set configure values with call for config
    config(rpc_devKey, rt_mySendSelection);

    // clear state of b_alreadyClosed, so that close() is called one time
    clearAlreadyClosed();
  };

  /** every subsystem of IsoAgLib has explicit function for controlled shutdown */
  void TracMove_c::close(void) {
    if (! checkAlreadyClosed()) {
      // avoid another call
      setAlreadyClosed();
      // unregister from timeEvent() call by Scheduler_c
      getSchedulerInstance4Comm().unregisterClient(this);
    }
  };

  /** deliver reference to data pkg as reference to CANPkgExt_c
      to implement the base virtual function correct
    */
  __IsoAgLib::CANPkgExt_c& TracMove_c::dataBase()
  {
    return c_data;
  }

  /** config the TracMove_c object after init -> set pointer to devKey and
      config send/receive of different base msg types
      @param rpc_devKey pointer to the DEV_KEY variable of the ersponsible member instance (pointer enables automatic value update if var val is changed)
      @param rt_mySendSelection optional Bitmask of base data to send ( default send nothing )
    */
  void TracMove_c::config(const DevKey_c* rpc_devKey, IsoAgLib::BaseDataGroup_t rt_mySendSelection)
  {
    // set configure values
    pc_devKey = rpc_devKey;
    t_mySendSelection = rt_mySendSelection;

    // set ui8_sendDevKey to the pointed value, if pointer is valid
    if ((rpc_devKey != NULL) && ((t_mySendSelection & IsoAgLib::BaseDataGroup1) != 0)) c_sendBase1DevKey = *rpc_devKey;
    else c_sendBase1DevKey.setUnspecified();
  };

  /** Retrieve the last update time of the specified information type
    @param rt_mySendSelection optional Bitmask of base data to send ( default send nothing )
    */
  int32_t TracMove_c::lastedTimeSinceUpdate(IsoAgLib::BaseDataGroup_t rt_mySendSelection) const
  {
    const int32_t ci32_now = System_c::getTime();
    if (rt_mySendSelection == IsoAgLib::BaseDataGroup1) return (ci32_now - i32_lastBase1);
    else return 0x7FFFFFFF;
  }
  /** Retrieve the time of last update */
  int32_t TracMove_c::lastUpdateTime( IsoAgLib::BaseDataGroup_t rt_mySendSelection ) const
  {
    if (rt_mySendSelection == IsoAgLib::BaseDataGroup1) return i32_lastBase1;
    else return 0x7FFFFFFF;
  }

  /** process received base msg and store updated value for later reading access;
      called by FilterBox_c::processMsg after receiving a msg
      possible errors:
        * LibErr_c::LbsBaseSenderConflict base msg recevied from different member than before
      @see FilterBox_c::processMsg
      @see CANIO_c::processMsg
      @return true -> message was processed; else the received CAN message will be served to other matching CANCustomer_c
    */
  bool TracMove_c::processMsg()
  {
    #if defined(USE_ISO_11783) && defined(USE_DIN_9684)
    if (c_data.identType() == Ident_c::ExtendedIdent)
    #endif // USE_DIN_9684 && USE_ISO_11783
    #ifdef USE_ISO_11783
    { // an ISO11783 base information msg received
      return isoProcessMsg();
    }
    #endif // USE_ISO_11783
    #if defined(USE_ISO_11783) && defined(USE_DIN_9684)
    else
    #endif
    #ifdef USE_DIN_9684
    return dinProcessMsg();
    #endif
    return false;
  };

  /** functions with actions, which must be performed periodically
      -> called periodically by Scheduler_c
      ==> sends base msg if configured in the needed rates
      possible errors:
        * dependant error in CANIO_c on CAN send problems
      @see CANPkg_c::getData
      @see CANPkgExt_c::getData
      @see CANIO_c::operator<<
      @return true -> all planned activities performed in allowed time
    */
  bool TracMove_c::timeEvent(void)
  {
    if (Scheduler_c::getAvailableExecTime() == 0) return false;

    checkCreateReceiveFilter();
    if (Scheduler_c::getAvailableExecTime() == 0) return false;

    const int32_t ci32_now = Scheduler_c::getLastTimeEventTrigger();

    // check for different base data types whether the previously
    // sending node stopped sending -> other nodes can now step in
    if (((t_mySendSelection & IsoAgLib::BaseDataGroup1) == 0)
      && ((ci32_now - i32_lastBase1) >= 3000)
      && (c_sendBase1DevKey.isSpecified()))
    { // the previously sending node didn't send the information for 3 seconds -> give other items a chance
      c_sendBase1DevKey.setUnspecified();
    }
    if ((pc_devKey != NULL) && (t_mySendSelection != IsoAgLib::BaseDataNothing))
    { // there is at least something configured to send
      #ifdef USE_ISO_11783
      if (getIsoMonitorInstance4Comm().existIsoMemberDevKey(*pc_devKey, true))
      { // stored base information sending ISO member has claimed address
        isoTimeEvent();
      }
      #endif
      #if defined(USE_ISO_11783) && defined(USE_DIN_9684)
      if (Scheduler_c::getAvailableExecTime() == 0) return false;
      #endif
      #ifdef USE_DIN_9684
      if (getDinMonitorInstance4Comm().existDinMemberDevKey(*pc_devKey, true))
      { // stored base information sending DIN member has claimed address
        dinTimeEvent();
      }
      #endif
    }
    return true;
  }

  /** check if filter boxes shall be created - create only ISO or DIN filters based
      on active local idents which has already claimed an address
      --> avoid to much Filter Boxes
    */
  void TracMove_c::checkCreateReceiveFilter(void)
  {
    SystemMgmt_c& c_systemMgmt = getSystemMgmtInstance4Comm();
    CANIO_c &c_can = getCanInstance4Comm();
    #ifdef USE_DIN_9684
    if ( ( !b_dinFilterCreated ) && (c_systemMgmt.existActiveLocalDinMember() ) )
    { // check if needed receive filters for DIN are active
      b_dinFilterCreated = true;
      // filter for base data 1
      c_can.insertFilter(*this, (0x7F << 4),(0x14 << 4), false);
    }
    #endif
    #ifdef USE_ISO_11783
    if ( ( ! b_isoFilterCreated ) && ( c_systemMgmt.existActiveLocalIsoMember() ) )
    { // check if needed receive filters for ISO are active
      b_isoFilterCreated = true;
      // create FilterBox_c for PGN GROUND_BASED_SPEED_DIST_PGN, PF 254 - mask for DP, PF and PS
      // mask: (0x1FFFF << 8) filter: (GROUND_BASED_SPEED_DIST_PGN << 8)
      c_can.insertFilter(*this, (static_cast<MASK_TYPE>(0x1FFFF) << 8),
                        (static_cast<MASK_TYPE>(GROUND_BASED_SPEED_DIST_PGN) << 8), false, Ident_c::ExtendedIdent);
      // create FilterBox_c for PGN WHEEL_BASED_SPEED_DIST_PGN, PF 254 - mask for DP, PF and PS
      // mask: (0x1FFFF << 8) filter: (WHEEL_BASED_SPEED_DIST_PGN << 8)
      c_can.insertFilter(*this, (static_cast<MASK_TYPE>(0x1FFFF) << 8),
                        (static_cast<MASK_TYPE>(WHEEL_BASED_SPEED_DIST_PGN) << 8), true, Ident_c::ExtendedIdent);

    }
    #endif
  }

  #ifdef USE_DIN_9684
  /** send a DIN9684 base information PGN
    * this is only called when sending ident is configured and it has already claimed an address
    */
  bool TracMove_c::dinTimeEvent(void)
  {
    CANIO_c& c_can = getCanInstance4Comm();
    const int32_t ci32_now = Scheduler_c::getLastTimeEventTrigger();
    // retreive the actual dynamic sender no of the member with the registered devKey
    uint8_t b_send = getDinMonitorInstance4Comm().dinMemberDevKey(*pc_devKey, true).nr();

    if (((ci32_now - i32_lastBase1) >= 100)
      && ((t_mySendSelection & IsoAgLib::BaseDataGroup1) != 0))
    { // send actual base1 data
      c_sendBase1DevKey = *pc_devKey;
      data().setBabo(4);
      data().setSend(b_send);
      data().setVal12(i16_speedReal);
      data().setVal34(i16_speedTheor);
      data().setVal56(long2int(i32_distReal));
      data().setVal78(long2int(i32_distTheor));

      // CANIO_c::operator<< retreives the information with the help of CANPkg_c::getData
      // then it sends the data
      c_can << data();

      // update time
      i32_lastBase1 = ci32_now;
    }
    if (Scheduler_c::getAvailableExecTime() == 0) return false;

    return true;
  }

  /** process a DIN9684 base information PGN */
  bool TracMove_c::dinProcessMsg()
  { // a DIN9684 base information msg received
    bool b_result = false;
    // store the devKey of the sender of base data
    const int32_t ci32_now = Scheduler_c::getLastTimeEventTrigger();
    if (! getDinMonitorInstance4Comm().existDinMemberNr(data().send()))
    { // the sender is not known -> ignore and block interpretation by other classes
      return true;
    }
    // the corresponding sender entry exist in the monitor list
    DevKey_c c_tempDevKey = getDinMonitorInstance4Comm().dinMemberNr(data().send()).devKey();

    // interprete data accordingto BABO
    switch (data().babo())
    {
      case 4: // base data 1: speed, dist
        // only take values, if i am not the regular sender
        // and if actual sender isn't in conflict to previous sender
        if ( checkParseReceived( c_tempDevKey, c_sendBase1DevKey, IsoAgLib::BaseDataGroup1 ) )
        { // sender is allowed to send
          // real speed
          setSpeedReal(data().val12());
          // theor speed
          setSpeedTheor(data().val34());
          // real dist -> react on 16bit int16_t overflow
          setOverflowSecure(i32_distReal, i16_lastDistReal, data().val56());
          // theor dist -> react on 16bit int16_t overflow
          setOverflowSecure(i32_distTheor, i16_lastDistTheor, data().val78());

          // set last time
          i32_lastBase1 = ci32_now;
          c_sendBase1DevKey = c_tempDevKey;
        }
        else
        { // there is a sender conflict
          getLbsErrInstance().registerError( LibErr_c::LbsBaseSenderConflict, LibErr_c::LbsBase );
        }
        b_result = true;
        break;
    } // end switch
    return b_result;
  }

  #endif

  /** check if a received message should be parsed */
  bool TracMove_c::checkParseReceived(const DevKey_c& rrefc_currentSender, const DevKey_c& rrefc_activeSender, IsoAgLib::BaseDataGroup_t rt_selfSend ) const
  {
    return ( ( ( t_mySendSelection & rt_selfSend   ) == 0 ) // I'm not the sender
          && ( // one of the following conditions must be true
              (rrefc_activeSender == rrefc_currentSender) // actual sender equivalent to last
            || (rrefc_activeSender.isUnspecified() ) // last sender has not correctly claimed address member
            || ((i32_distReal == -1)&&(i32_distTheor == -1)&&(i16_speedTheor == -1)) // current received data is not valid
            )
          )?true:false;
  };

  #ifdef USE_ISO_11783
  /**
    process a ISO11783 base information PGN
  */
  bool TracMove_c::isoProcessMsg()
  {
    bool b_result = false;
    #if defined(USE_BASE) || defined(USE_TRACTOR_GENERAL)
    IsoAgLib::iTracGeneral_c& c_tracgeneral = IsoAgLib::getITracGeneralInstance();
    #endif
    DevKey_c c_tempDevKey( DevKey_c::DevKeyUnspecified );
    const int32_t ci32_now = Scheduler_c::getLastTimeEventTrigger();
    // store the devKey of the sender of base data
    if (getIsoMonitorInstance4Comm().existIsoMemberNr(data().isoSa()))
    { // the corresponding sender entry exist in the monitor list
      c_tempDevKey = getIsoMonitorInstance4Comm().isoMemberNr(data().isoSa()).devKey();
    }

    switch (data().isoPgn() & 0x1FFFF)
    {
      case GROUND_BASED_SPEED_DIST_PGN:
      case WHEEL_BASED_SPEED_DIST_PGN:
        // only take values, if i am not the regular sender
        // and if actual sender isn't in conflict to previous sender
        if ( checkParseReceived( c_tempDevKey, c_sendBase1DevKey, IsoAgLib::BaseDataGroup1 ) )
        { // sender is allowed to send
          int16_t i16_tempSpeed = data().val12();
          switch (data().val8() & 3 ) {
          case 0: // driving reverse
            if ( (i16_tempSpeed != ERROR_VAL_16S)
              && (i16_tempSpeed != NO_VAL_16S) ) i16_tempSpeed *= -1;
            break;
          case 1: // driving forward
            break;
          case 2: // ERROR
            i16_tempSpeed = ERROR_VAL_16S;
            break;
          case 3: // not available
            i16_tempSpeed = NO_VAL_16S;
            break;
          }

          if (data().isoPgn() == GROUND_BASED_SPEED_DIST_PGN)
          { // real speed
            setSpeedReal(i16_tempSpeed);
            // real dist
            setDistReal(static_cast<int32_t>(data().val36()));
          }
          else
          { // wheel based speed
            setSpeedTheor(i16_tempSpeed);
            // wheel based dist
            setDistTheor(static_cast<int32_t>(data().val36()));
            #if defined(USE_BASE) || defined(USE_TRACTOR_GENERAL)
            // additionally scan for key switch and maximum power time
            c_tracgeneral.setKeySwitch(IsoAgLib::IsoActiveFlag_t( ( data().val8() >> 2 ) & 3 ));
            c_tracgeneral.setMaxPowerTime(data().val7());
            #endif
          }
          // set last time
          i32_lastBase1 = ci32_now;
          c_sendBase1DevKey = c_tempDevKey;
        }
        else
        { // there is a sender conflict
          getLbsErrInstance().registerError( LibErr_c::LbsBaseSenderConflict, LibErr_c::LbsBase );
        }
        b_result = true;
        break;
    }
    return b_result;
  }

  /** send a ISO11783 base information PGN.
  * this is only called when sending ident is configured and it has already claimed an address
  */
  bool TracMove_c::isoTimeEvent( void )
  {
    const int32_t ci32_now = Scheduler_c::getLastTimeEventTrigger();

    // retreive the actual dynamic sender no of the member with the registered devKey
    uint8_t b_sa = getIsoMonitorInstance4Comm().isoMemberDevKey(*pc_devKey, true).nr();
    data().setIdentType(Ident_c::ExtendedIdent);
    data().setIsoPri(3);
    data().setIsoSa(b_sa);

    if ( ( ( ci32_now - i32_lastBase1 ) >= 100                   )
      && ( ( t_mySendSelection & IsoAgLib::BaseDataGroup1 ) != 0 ) )
    { // it's time to send Base1
      isoSendBase1Update();
    }
    if ( Scheduler_c::getAvailableExecTime() == 0 ) return false;

    return true;
  }

  /** send Base1 Group data with ground&theor speed&dist */
  void TracMove_c::isoSendBase1Update( void )
  { // send actual base1 data: ground/wheel based speed/dist
    CANIO_c& c_can = getCanInstance4Comm();
    c_sendBase1DevKey = *pc_devKey;
    data().setIsoPgn(GROUND_BASED_SPEED_DIST_PGN);
  #ifdef SYSTEM_PC_VC
    data().setVal12(abs(i16_speedReal));
  #else
    data().setVal12(CNAMESPACE::abs(i16_speedReal));
  #endif
    data().setVal36(i32_distReal);
    switch (i16_speedReal) {
      case ERROR_VAL_16S:
        data().setVal8(IsoAgLib::IsoError);
        break;
      case NO_VAL_16S:
        data().setVal8(IsoAgLib::IsoNotAvailable);
        break;
      default:
        if (i16_speedReal < 0) data().setVal8(0);
        else data().setVal8(1);
        break;
    }
      // CANIO_c::operator<< retreives the information with the help of CANPkg_c::getData
      // then it sends the data
    c_can << data();

    data().setIsoPgn(WHEEL_BASED_SPEED_DIST_PGN);
  #ifdef SYSTEM_PC_VC
    data().setVal12(abs(i16_speedTheor));
  #else
    data().setVal12(CNAMESPACE::abs(i16_speedTheor));
  #endif
    data().setVal36(i32_distTheor);

    uint8_t b_val8 = IsoAgLib::IsoInactive;
    switch (i16_speedTheor) {
      case ERROR_VAL_16S:
        b_val8 |= IsoAgLib::IsoError;
        break;
      case NO_VAL_16S:
        b_val8 |= IsoAgLib::IsoNotAvailable;
        break;
      default:
        if (i16_speedTheor >= 0) b_val8 |= IsoAgLib::IsoActive;
        break;
    }
    // CANIO_c::operator<< retreives the information with the help of CANPkg_c::getData
    // then it sends the data
    c_can << data();

    // update time
    i32_lastBase1 = Scheduler_c::getLastTimeEventTrigger();
  }
  #endif

  /** update distance values ; react on int16_t overflow
      @param reflVal to be updated value as int32_t variable
      @param refiVal to be updated value as 16bit int16_t variable
      @param rrefiNewVal new value given as reference to 16bit int
    */
  void TracMove_c::setOverflowSecure(int32_t& reflVal, int16_t& refiVal, const int16_t& rrefiNewVal)
  {
    // definition area [0..i32_maxVal] with overflow
    // 10000 of Fendt; 32767 of DIN
    const int32_t i32_maxDefFendt = 10000,
                  i32_maxDefDin = 32767;
    int32_t i32_newValFendt = reflVal,
            i32_newValDin = reflVal;
    int16_t i16_diff = rrefiNewVal - refiVal;

    // check if there was an overflow = diff is greater than half of def area (per sign side)
    #ifdef SYSTEM_PC_VC
    if ((abs(i16_diff) > i32_maxDefFendt/2) || (abs(i16_diff) > i32_maxDefDin/2))
    { // one of the overflow checks triggers
      if (abs(i16_diff) > i32_maxDefFendt/2)
    #else
    if ((CNAMESPACE::abs(i16_diff) > i32_maxDefFendt/2) || (CNAMESPACE::abs(i16_diff) > i32_maxDefDin/2))
    { // one of the overflow checks triggers
      if (CNAMESPACE::abs(i16_diff) > i32_maxDefFendt/2)
    #endif
      { // the old wrong fendt limit triggers
        if (rrefiNewVal > refiVal)
        { // dist decreased lower than 0 -> lower underflow
          i32_newValFendt -= refiVal; // max reducable before underflow
          i32_newValFendt -= (i32_maxDefFendt - rrefiNewVal); // decreased after underflow (jump to int16 max pos val)
        }
        else
        { // dist increased upper than 0x7FFF -> overflow
          i32_newValFendt += (i32_maxDefFendt - refiVal); // max decrease before overflow
          i32_newValFendt += rrefiNewVal; // decreased after overflow
        }
      }
      #ifdef SYSTEM_PC_VC
      if (abs(i16_diff) > i32_maxDefDin/2)
      #else
      if (CNAMESPACE::abs(i16_diff) > i32_maxDefDin/2)
      #endif
    { // the correct DIN limit triggers
        if (rrefiNewVal > refiVal)
        { // dist decreased lower than 0 -> lower underflow
          i32_newValDin -= refiVal; // max reducable before underflow
          i32_newValDin -= (i32_maxDefDin - rrefiNewVal); // decreased after underflow (jump to int16 max pos val)
        }
        else
        { // dist increased upper than 0x7FFF -> overflow
          i32_newValDin += (i32_maxDefDin - refiVal); // max decrease before overflow
          i32_newValDin += rrefiNewVal; // decreased after overflow
        }
      }
      // check, which new value should be used -> take the value which mean less distance to old val
      if (CNAMESPACE::labs(i32_newValFendt - reflVal) < CNAMESPACE::labs(i32_newValDin - reflVal))
      {
        reflVal = i32_newValFendt;
      }
      else
      {
        reflVal = i32_newValDin;
      }
    }
    else
    { // there's no overflow -> easy increment reflVal
      reflVal += i16_diff;
    }
    // store the given int16 val as last in16 val in refiVal
    refiVal = rrefiNewVal;
  };

  /** deliver the devKey of the sender of the base data
      possible errors:
          * Err_c::range rui8_typeNr doesn't match valid base msg type number
      @param rt_typeGrp base msg type no of interest: BaseDataGroup1 | BaseDataGroup2 | BaseDataCalendar
      @return DEV_KEY code of member who is sending the intereested base msg type
    */
  const DevKey_c& TracMove_c::senderDevKey(IsoAgLib::BaseDataGroup_t rt_typeGrp) const {
    // simply answer first matching result if more than one type is selected
    if ( ( rt_typeGrp & IsoAgLib::BaseDataGroup1   ) != 0 ) return c_sendBase1DevKey;
    else return DevKey_c::DevKeyUnspecified;
  }

  /** set the real (radar measured) driven distance with int32_t val
      @param rreflVal value to store as real radar measured distance
    */
  void TracMove_c::setDistReal(const int32_t& rreflVal)
  {
    i32_distReal = rreflVal;
    // set the int16_t value regarding the overflow counting
    i16_lastDistReal = long2int(rreflVal);
  };

  /** set the theoretical (gear calculated) driven distance with int32_t val
      @param rreflVal value to store as theoretical (gear calculated) driven distance
    */
  void TracMove_c::setDistTheor(const int32_t& rreflVal)
  {
    i32_distTheor = rreflVal;
    // set the int16_t value regarding the overflow counting
    i16_lastDistTheor = long2int(rreflVal);
  };

  /** get int16_t overflowed val from long
      @param rreflVal value as int32_t (32bit) variable
      @return 16bit int16_t calculated with counting overflow from 32767 to (-32766)
    */
  int16_t TracMove_c::long2int(const int32_t& rreflVal)
  {
    // Version mit [0..10000] und Uberlauf a la Fendt Vario
    //int32_t i32_mod =  rreflVal % 10000;
    // Version mit [0..32767] und Uberlauf a la Scheduler_c DIN 9684
    int32_t i32_mod =  rreflVal % 32767;
    return i32_mod;
  };

} // End Namespace __IsoAglib
