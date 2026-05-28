/////////////////////////////////////////////////////////////////////////////////
/// Report desarrollado en el marco del proyecto.                             ///
///                                                                           ///
/// Implementa un reporte ORAN para transportar información de paquetes o     ///
/// bytes descartados en transmisión por un UE. Esta métrica es recibida      ///
/// por el Near-RT RIC y almacenada en el repositorio de datos.               ///
/////////////////////////////////////////////////////////////////////////////////


#ifndef ORAN_REPORT_NR_UE_TX_DROP_H
#define ORAN_REPORT_NR_UE_TX_DROP_H

#include "oran-report.h"

#include <string>

namespace ns3
{

class OranReportNrUeTxDrop : public OranReport
{
  public:

    // Obtiene el TypeId asociado a la clase.
    static TypeId GetTypeId();

    // Constructor.
    OranReportNrUeTxDrop();

    // Destructor.
    ~OranReportNrUeTxDrop() override;

    // Devuelve una representación en texto del reporte.
    std::string ToString() const override;

    // Obtiene la cantidad reportada de bytes descartados.
    uint64_t GetDrops() const;

    // Obtiene el RNTI asociado a la transmisión.
    uint16_t GetRnti() const;

    // Obtiene el identificador del canal lógico asociado a la transmisión.
    uint8_t GetLcid() const;

  private:
    uint16_t m_rnti{0}; // RNTI del UE.
    uint8_t m_lcid{0};  // Identificador del canal lógico.
    uint64_t m_drops{0}; // Bytes descartados en transmisión.
}; // class OranReportNrUeTxDrop

} // namespace ns3

#endif // ORAN_REPORT_NR_UE_TX_DROP_H
