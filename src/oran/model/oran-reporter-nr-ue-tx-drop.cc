///////////////////////////////////////////////////////////////////////////////
/// Reporter desarrollado en el marco del proyecto.                         ///
///                                                                         ///
/// Este componente captura eventos de descarte en transmisión de un UE     ///
/// y acumula la cantidad de bytes descartados por par RNTI-LCID. Luego     ///
/// transforma esas muestras en objetos OranReportNrUeTxDrop para enviarlos ///
/// al Near-RT RIC.                                                         ///
///////////////////////////////////////////////////////////////////////////////


#include "oran-reporter-nr-ue-tx-drop.h"

#include "oran-report-nr-ue-tx-drop.h"

#include "ns3/log.h"
#include "ns3/packet.h"
#include "ns3/simulator.h"
#include "ns3/uinteger.h"

namespace ns3
{

NS_LOG_COMPONENT_DEFINE("OranReporterNrUeTxDrop");
NS_OBJECT_ENSURE_REGISTERED(OranReporterNrUeTxDrop);

TypeId
OranReporterNrUeTxDrop::GetTypeId()
{
    static TypeId tid =
        TypeId("ns3::OranReporterNrUeTxDrop")
            .SetParent<OranReporter>()
            .AddConstructor<OranReporterNrUeTxDrop>()
            .AddAttribute("Rnti",
                          "Default RNTI to use when drop traces do not provide one.",
                          UintegerValue(),
                          MakeUintegerAccessor(&OranReporterNrUeTxDrop::m_defaultRnti),
                          MakeUintegerChecker<uint16_t>())
            .AddAttribute("Lcid",
                          "Default logical channel ID to use when drop traces do not provide one.",
                          UintegerValue(),
                          MakeUintegerAccessor(&OranReporterNrUeTxDrop::m_defaultLcid),
                          MakeUintegerChecker<uint8_t>());

    return tid;
}

OranReporterNrUeTxDrop::OranReporterNrUeTxDrop()
{
    NS_LOG_FUNCTION(this);
}

OranReporterNrUeTxDrop::~OranReporterNrUeTxDrop()
{
    NS_LOG_FUNCTION(this);
}

void
OranReporterNrUeTxDrop::AddTxDrop(Ptr<const Packet> p)
{
    NS_LOG_FUNCTION(this << p);

    // Versión usada cuando la traza no entrega explícitamente RNTI y LCID.
    // En ese caso se utilizan los valores por defecto configurados como atributos.
    AddTxDrop(p, m_defaultRnti, m_defaultLcid);
}

void
OranReporterNrUeTxDrop::AddTxDrop(Ptr<const Packet> p, uint16_t rnti, uint8_t lcid)
{
    NS_LOG_FUNCTION(this << p << rnti << +lcid);

    // Se acumulan los bytes descartados para cada par RNTI-LCID.
    // Si ya existe una muestra para ese par, se actualiza el tiempo y se suma el tamaño del paquete descartado.
    auto& sample = m_samples[{rnti, lcid}];
    sample.time = Simulator::Now();
    sample.rnti = rnti;
    sample.lcid = lcid;
    sample.bytes += p->GetSize();
}

std::vector<Ptr<OranReport>>
OranReporterNrUeTxDrop::GenerateReports()
{
    NS_LOG_FUNCTION(this);

    std::vector<Ptr<OranReport>> reports;

    if (m_active)
    {
        NS_ABORT_MSG_IF(m_terminator == nullptr,
                        "Attempting to generate reports in reporter with NULL E2 Terminator");

        // Se genera un reporte ORAN por cada par RNTI-LCID con descartes acumulados.
        for (const auto& [key, sample] : m_samples)
        {
            Ptr<OranReportNrUeTxDrop> report = CreateObject<OranReportNrUeTxDrop>();
            report->SetAttribute("ReporterE2NodeId", UintegerValue(m_terminator->GetE2NodeId()));
            report->SetAttribute("Time", TimeValue(sample.time));
            report->SetAttribute("Rnti", UintegerValue(sample.rnti));
            report->SetAttribute("Lcid", UintegerValue(sample.lcid));
            report->SetAttribute("Drops", UintegerValue(sample.bytes));

            reports.push_back(report);
        }

        // Una vez generados los reportes, se limpian las muestras acumuladas.
        m_samples.clear();
    }

    return reports;
}

} // namespace ns3
