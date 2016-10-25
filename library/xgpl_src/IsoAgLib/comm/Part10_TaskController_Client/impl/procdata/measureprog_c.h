/*
  measureprog_c.h: object for managing connected local measure programs

  (C) Copyright 2009 - 2016 by OSB AG and developing partners

  See the repository-log for details on the authors and file-history.
  (Repository information can be found at <http://isoaglib.com/download>)

  Use, modification and distribution are subject to the GNU General
  Public License with exceptions for ISOAgLib. (See accompanying
  file LICENSE.txt or copy at <http://isoaglib.com/download/license>)
*/
#ifndef MEASUREPROG_C_H
#define MEASUREPROG_C_H

#include <IsoAgLib/isoaglib_config.h>
#include <IsoAgLib/comm/Part10_TaskController_Client/iprocdata.h>
#include <IsoAgLib/comm/Part10_TaskController_Client/impl/procdata/connectedpd_c.h>
#include <IsoAgLib/comm/Part10_TaskController_Client/impl/procdata/pdlocal_c.h>

namespace __IsoAgLib {

  class PdConnection_c;
  class MeasureOnChange_c;
  class MeasureTimeProp_c;
  class MeasureDistProp_c;


  class MeasureProg_c : public ConnectedPd_c
  {
    public:
      MeasureProg_c( PdConnection_c&, PdLocal_c& );
      virtual ~MeasureProg_c();

      virtual void handleRequest() { sendValue(); }
      virtual void handleIncoming( int32_t, bool wasBroadcast );
      virtual bool startMeasurement( IsoAgLib::ProcData::MeasurementCommand_t, int32_t ai32_increment );

      void valueUpdated();
      void sendValue();
      void stopAllMeasurements();

      bool minMaxLimitsPassed() const;

      const PdLocal_c &pdLocal() const { return static_cast<const PdLocal_c &>( m_pdBase ); }
      PdLocal_c &pdLocal() { return static_cast<PdLocal_c &>( m_pdBase ); }

    private:
      int32_t getValue() const;

    private:
      MeasureOnChange_c *m_subProgOnChange;
      MeasureTimeProp_c *m_subProgTimeProp;
      MeasureDistProp_c *m_subProgDistProp;

      int32_t m_minThreshold;
      int32_t m_maxThreshold;
  };

}

#endif
