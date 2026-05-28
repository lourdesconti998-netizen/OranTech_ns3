///////////////////////////////////////////////////////////////////////////////
/// Reporter desarrollado en el marco del proyecto.                         ///
///                                                                         ///
/// Este componente captura reportes de estado de buffer generados por el   ///
/// UE y los transforma en objetos OranReportUeTxBuffer para enviarlos al   ///
/// Near-RT RIC.                                                            ///
///////////////////////////////////////////////////////////////////////////////

#include "oran-reporter-buffer-status.h"
#include "oran-report-buffer-status.h"

#include "ns3/log.h"
#include "ns3/simulator.h"
#include "ns3/uinteger.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE("OranReporterUeTxBuffer");
NS_OBJECT_ENSURE_REGISTERED(OranReporterUeTxBuffer);

TypeId
OranReporterUeTxBuffer::GetTypeId()
{
    static TypeId tid =
        TypeId("ns3::OranReporterUeTxBuffer")
            .SetParent<OranReporter>()
            .AddConstructor<OranReporterUeTxBuffer>();
    return tid;
}

OranReporterUeTxBuffer::OranReporterUeTxBuffer()
{
    NS_LOG_FUNCTION(this);
}

OranReporterUeTxBuffer::~OranReporterUeTxBuffer()
{
    NS_LOG_FUNCTION(this);
}

void
OranReporterUeTxBuffer::AddSample(NrMacSapProvider::BufferStatusReportParameters params)
{
    // Se almacena la muestra de buffer recibida desde la pila NR. Para cada LCID se conserva la última muestra disponible.
    Sample s;
    s.time = Simulator::Now();
    s.data = params;
    m_samples[params.lcid] = s;
}

std::vector<Ptr<OranReport>>
OranReporterUeTxBuffer::GenerateReports()
{
    std::vector<Ptr<OranReport>> reports;

    if (m_active && !m_samples.empty())
    {
        NS_ABORT_MSG_IF(m_terminator == nullptr,
                        "Attempting to generate reports in reporter with NULL E2 Terminator");

        // Se genera un reporte ORAN por cada LCID con muestra disponible.
        for (const auto& [lcid, sample] : m_samples)
        {
            Ptr<OranReportUeTxBuffer> rep = CreateObject<OranReportUeTxBuffer>();

            rep->SetAttribute("ReporterE2NodeId",
                              UintegerValue(m_terminator->GetE2NodeId()));
            rep->SetAttribute("Time", TimeValue(sample.time));
            rep->SetAttribute("Rnti", UintegerValue(sample.data.rnti));
            rep->SetAttribute("Lcid", UintegerValue(sample.data.lcid));
            rep->SetAttribute("TxQueueSize",
                              UintegerValue(sample.data.txQueueSize));
            rep->SetAttribute("TxQueueHolDelay",
                              UintegerValue(sample.data.txQueueHolDelay));
            rep->SetAttribute("RetxQueueSize",
                              UintegerValue(sample.data.retxQueueSize));
            rep->SetAttribute("RetxQueueHolDelay",
                              UintegerValue(sample.data.retxQueueHolDelay));
            rep->SetAttribute("StatusPduSize",
                              UintegerValue(sample.data.statusPduSize));

            reports.push_back(rep);
        }

        // Una vez generados los reportes, se limpian las muestras ya enviadas.
        m_samples.clear();
    }

    return reports;
}

} // namespace ns3
