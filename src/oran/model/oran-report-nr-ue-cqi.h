/////////////////////////////////////////////////////////////////////////////////
/// Report desarrollado en el marco del proyecto.                             ///
///                                                                           ///
/// Implementa un reporte ORAN para transportar métricas de calidad de canal  ///
/// reportadas por un UE. En particular, incluye CQI, MCS y RI. Estas         ///
/// métricas son recibidas por el Near-RT RIC y almacenadas en el repositorio ///
/// de datos.                                                                 ///
/////////////////////////////////////////////////////////////////////////////////


#ifndef ORAN_REPORT_NR_UE_CQI_H
#define ORAN_REPORT_NR_UE_CQI_H

#include "oran-report.h"

#include "ns3/nstime.h"

namespace ns3
{

class OranReportNrUeCqi : public OranReport
{
  public:

    // Obtiene el TypeId asociado a la clase.
    static TypeId GetTypeId();

    // Constructor.
    OranReportNrUeCqi();

    // Destructor.
    ~OranReportNrUeCqi() override;

    // Devuelve una representación en texto del reporte.
    std::string ToString() const override;

    // Obtiene el RNTI del UE.    
    uint16_t GetRnti() const;

    // Obtiene el valor de CQI reportado por el UE.
    uint8_t GetCqi() const;

    // Obtiene el valor de MCS asociado al CQI.
    uint8_t GetMcs() const;

    // Obtiene el indicador de rango reportado por la UE.
    uint8_t GetRi() const;

  private:
    uint16_t m_rnti{0};
    uint8_t m_cqi{0};
    uint8_t m_mcs{0};
    uint8_t m_ri{0};
}; // class OranReportNrUeCqi

} // namespace ns3

#endif // ORAN_REPORT_NR_UE_CQI_H
