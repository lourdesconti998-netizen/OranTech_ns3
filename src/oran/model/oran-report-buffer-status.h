/////////////////////////////////////////////////////////////////////////////////
/// Report desarrollado en el marco del proyecto.                             ///
///                                                                           ///
/// Este archivo define un reporte ORAN para transportar el estado del buffer ///
/// de transmisión de un UE. La estructura sigue el estilo de los reportes    ///
/// existentes del módulo ORAN.                                               ///
/////////////////////////////////////////////////////////////////////////////////

#ifndef ORAN_REPORT_BUFFER_STATUS_H
#define ORAN_REPORT_BUFFER_STATUS_H

#include "oran-report.h"

#include <string>

namespace ns3
{

class OranReportUeTxBuffer : public OranReport
{
  public:

    // Obtiene el TypeId de la clase OranReportUeTxBuffer. 
    static TypeId GetTypeId();
    
    // Constructor.    
    OranReportUeTxBuffer();
  
     // Destructor.
    ~OranReportUeTxBuffer() override;

    std::string ToString() const override;

    // Obtiene el RNTI asociado al estado del buffer.
    uint16_t GetRnti() const;

    // Obtiene el identificador del canal lógico asociado al buffer.
    uint8_t GetLcid() const;

    // Obtiene el tamaño de la cola de transmisión.
    uint32_t GetTxQueueSize() const;

    // Obtiene retardo HOL de la cola de transmisión.
    uint16_t GetTxQueueHolDelay() const;

    // Obtiene tamaño de la cola de retransmisión en bytes.
    uint32_t GetRetxQueueSize() const;

    // Obtiene retardo HOL de la cola de retransmisión.
    uint16_t GetRetxQueueHolDelay() const;

    // Obtiene el tamaño pendiente de STATUS PDU en bytes.
    uint16_t GetStatusPduSize() const;

  private:
    uint16_t m_rnti;              // RNTI de la UE.
    uint8_t m_lcid;               // Identificador del canal lógico.
    uint32_t m_txQueueSize;       // Tamaño de la cola de transmisión en bytes.
    uint16_t m_txQueueHolDelay;   // Retardo HOL de la cola de transmisión.
    uint32_t m_retxQueueSize;     // Tamaño de la cola de retransmisión en bytes.
    uint16_t m_retxQueueHolDelay; // Retardo HOL de la cola de retransmisión.
    uint16_t m_statusPduSize;     // Tamaño pendiente de STATUS PDU en bytes.
};

} // namespace ns3

#endif // ORAN_REPORT_BUFFER_STATUS_H
