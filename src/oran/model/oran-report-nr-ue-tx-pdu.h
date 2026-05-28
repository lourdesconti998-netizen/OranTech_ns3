/////////////////////////////////////////////////////////////////////////////////
/// Report desarrollado en el marco del proyecto.                             ///
///                                                                           ///
/// Implementa un reporte ORAN para transportar información de bytes de PDU   ///
/// transmitidos por un UE. Esta métrica es recibida por el Near-RT RIC       ///
/// y almacenada en el repositorio de datos.                                  ///
/////////////////////////////////////////////////////////////////////////////////    


#ifndef ORAN_REPORT_NR_UE_TX_PDU_H
#define ORAN_REPORT_NR_UE_TX_PDU_H

#include "oran-report.h"

#include <string>

namespace ns3
{

class OranReportNrUeTxPdu : public OranReport
{
  public:

    // Obtiene el TypeId asociado a la clase.
    static TypeId GetTypeId();

    // Constructor.
    OranReportNrUeTxPdu();

    // Destructor.
    ~OranReportNrUeTxPdu() override;

    // Devuelve una representación en texto del reporte.
    std::string ToString() const override;

    // Obtiene la cantidad reportada de bytes de PDU transmitidas.
    uint64_t GetBytes() const;

    // Obtiene el RNTI asociado a la transmisión.
    uint16_t GetRnti() const;

    // Obtiene el identificador del canal lógico asociado a la transmisión.
    uint8_t GetLcid() const;

  private:
    uint16_t m_rnti{0};     // RNTI de la UE.
    uint8_t m_lcid{0};      // Identificador del canal lógico.
    uint64_t m_bytes{0};    // Bytes de PDU transmitidas.
}; // class OranReportNrUeTxPdu

} // namespace ns3

#endif // ORAN_REPORT_NR_UE_TX_PDU_H
