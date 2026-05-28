///////////////////////////////////////////////////////////////////////////////////
/// Reporter desarrollado en el marco del proyecto.                             ///
///                                                                             ///
/// Este componente captura eventos de transmisión de PDU de un UE y acumula    ///
/// la cantidad de bytes transmitidos por LCID. Luego transforma  esas          ///
/// muestras en objetos OranReportNrUeTxPdu para enviarlos al Near-RT RIC.      ///
///////////////////////////////////////////////////////////////////////////////////


#include "oran-reporter-nr-ue-tx-pdu.h"

#include "oran-report-nr-ue-tx-pdu.h"

#include "ns3/log.h"
#include "ns3/simulator.h"
#include "ns3/uinteger.h"

namespace ns3
{

NS_LOG_COMPONENT_DEFINE("OranReporterNrUeTxPdu");
NS_OBJECT_ENSURE_REGISTERED(OranReporterNrUeTxPdu);

TypeId
OranReporterNrUeTxPdu::GetTypeId()
{
    static TypeId tid =
        TypeId("ns3::OranReporterNrUeTxPdu")
            .SetParent<OranReporter>()
            .AddConstructor<OranReporterNrUeTxPdu>();

    return tid;
}

OranReporterNrUeTxPdu::OranReporterNrUeTxPdu()
{
    NS_LOG_FUNCTION(this);
}

OranReporterNrUeTxPdu::~OranReporterNrUeTxPdu()
{
    NS_LOG_FUNCTION(this);
}

void
OranReporterNrUeTxPdu::AddTxPdu(uint16_t rnti, uint8_t lcid, uint32_t bytes)
{
    NS_LOG_FUNCTION(this << rnti << +lcid << bytes);

    // Se acumulan los bytes de PDU transmitidas para cada LCID.
    // Si ya existe una muestra para ese LCID, se actualiza el tiempo y se suma la cantidad de bytes transmitidos.
    auto& sample = m_samples[lcid];
    sample.time = Simulator::Now();
    sample.rnti = rnti;
    sample.lcid = lcid;
    sample.bytes += bytes;

}

std::vector<Ptr<OranReport>>
OranReporterNrUeTxPdu::GenerateReports()
{
    NS_LOG_FUNCTION(this);

    std::vector<Ptr<OranReport>> reports;

    if (m_active)
    {
        NS_ABORT_MSG_IF(m_terminator == nullptr,
                        "Attempting to generate reports in reporter with NULL E2 Terminator");

        // Se genera un reporte ORAN por cada LCID con bytes transmitidos acumulados.    
        for (const auto& [lcid, sample] : m_samples)
        {
            Ptr<OranReportNrUeTxPdu> report = CreateObject<OranReportNrUeTxPdu>();
            report->SetAttribute("ReporterE2NodeId", UintegerValue(m_terminator->GetE2NodeId()));
            report->SetAttribute("Time", TimeValue(sample.time));
            report->SetAttribute("Rnti", UintegerValue(sample.rnti));
            report->SetAttribute("Lcid", UintegerValue(sample.lcid));
            report->SetAttribute("Bytes", UintegerValue(sample.bytes));


            reports.push_back(report);
        }

        // Una vez generados los reportes, se limpian las muestras acumuladas.
        m_samples.clear();
    }

    return reports;
 }

} // namespace ns3
