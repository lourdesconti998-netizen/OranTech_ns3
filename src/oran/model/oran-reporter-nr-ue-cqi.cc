///////////////////////////////////////////////////////////////////////////////
/// Reporter desarrollado en el marco del proyecto.                         ///
///                                                                         ///
/// Este componente captura métricas de calidad de canal reportadas por una ///
/// UE y las transforma en objetos OranReportNrUeCqi para enviarlas al      ///
/// Near-RT RIC. Las métricas reportadas incluyen CQI, MCS y RI, y pueden   ///
/// ser utilizadas para monitoreo y aprendizaje por refuerzo.               ///
///////////////////////////////////////////////////////////////////////////////


#include "oran-reporter-nr-ue-cqi.h"

#include "oran-report-nr-ue-cqi.h"

#include "ns3/abort.h"
#include "ns3/log.h"
#include "ns3/simulator.h"
#include "ns3/uinteger.h"

namespace ns3
{

NS_LOG_COMPONENT_DEFINE("OranReporterNrUeCqi");
NS_OBJECT_ENSURE_REGISTERED(OranReporterNrUeCqi);

TypeId
OranReporterNrUeCqi::GetTypeId()
{
    static TypeId tid = TypeId("ns3::OranReporterNrUeCqi")
                            .SetParent<OranReporter>()
                            .AddConstructor<OranReporterNrUeCqi>();

    return tid;
}

OranReporterNrUeCqi::OranReporterNrUeCqi()
{
    NS_LOG_FUNCTION(this);
}

OranReporterNrUeCqi::~OranReporterNrUeCqi()
{
    NS_LOG_FUNCTION(this);
}

void
OranReporterNrUeCqi::ReportCqi(uint16_t rnti, uint8_t cqi, uint8_t mcs, uint8_t ri)
{
    NS_LOG_FUNCTION(this << +rnti << +cqi << +mcs << +ri);

    if (m_active)
    {
        NS_ABORT_MSG_IF(m_terminator == nullptr,
                        "Attempting to generate reports in reporter with NULL E2 Terminator");

        // Se crea un reporte ORAN con las métricas de calidad de canal recibidas desde el UE.
        Ptr<OranReportNrUeCqi> report = CreateObject<OranReportNrUeCqi>();
        
        report->SetAttribute("ReporterE2NodeId", UintegerValue(m_terminator->GetE2NodeId()));
        report->SetAttribute("Time", TimeValue(Simulator::Now()));
        report->SetAttribute("Rnti", UintegerValue(rnti));
        report->SetAttribute("Cqi", UintegerValue(cqi));
        report->SetAttribute("Mcs", UintegerValue(mcs));
        report->SetAttribute("Ri", UintegerValue(ri));

        // El reporte se almacena temporalmente hasta que el E2 Terminator lo solicite.
        m_reports.push_back(report);
    }
}

std::vector<Ptr<OranReport>>
OranReporterNrUeCqi::GenerateReports()
{
    NS_LOG_FUNCTION(this);

    std::vector<Ptr<OranReport>> reports;

    if (m_active)
    {
        // Se devuelven los reportes acumulados y luego se limpia el buffer local.
        reports = m_reports;
        m_reports.clear();
    }

    return reports;
}

} // namespace ns3
