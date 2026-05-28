///////////////////////////////////////////////////////////////////////////////
/// Reporter desarrollado en el marco del proyecto.                         ///
///                                                                         ///
/// Este componente captura métricas de calidad de canal reportadas por una ///
/// UE y las transforma en objetos OranReportNrUeCqi para enviarlas al      ///
/// Near-RT RIC. Las métricas reportadas incluyen CQI, MCS y RI, y pueden   ///
/// ser utilizadas para monitoreo y aprendizaje por refuerzo.               ///
///////////////////////////////////////////////////////////////////////////////


#ifndef ORAN_REPORTER_NR_UE_CQI_H
#define ORAN_REPORTER_NR_UE_CQI_H

#include "oran-reporter.h"

#include <vector>

namespace ns3
{

class OranReporterNrUeCqi : public OranReporter
{
  public:

    // Obtiene el TypeId asociado a la clase.
    static TypeId GetTypeId();

    // Constructor.
    OranReporterNrUeCqi();

    // Destructor.
    ~OranReporterNrUeCqi() override;

    // Agrega una muestra de calidad de canal reportada por una UE NR.
    void ReportCqi(uint16_t rnti, uint8_t cqi, uint8_t mcs, uint8_t ri);

    // Genera los reportes ORAN acumulados.
    std::vector<Ptr<OranReport>> GenerateReports() override;

  private:

    // Reportes CQI acumulados hasta la siguiente llamada a GenerateReports().
    std::vector<Ptr<OranReport>> m_reports;
}; // class OranReporterNrUeCqi

} // namespace ns3

#endif // ORAN_REPORTER_NR_UE_CQI_H
