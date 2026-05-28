/////////////////////////////////////////////////////////////////////////////////
/// Report desarrollado en el marco del proyecto.                             ///
///                                                                           ///
/// Implementa un reporte ORAN para transportar información de paquetes o     ///
/// bytes descartados en transmisión por un UE. Esta métrica es recibida      ///
/// por el Near-RT RIC y almacenada en el repositorio de datos.               ///
/////////////////////////////////////////////////////////////////////////////////


#include "oran-report-nr-ue-tx-drop.h"

#include "oran-report.h"

#include "ns3/log.h"
#include "ns3/uinteger.h"

namespace ns3
{

NS_LOG_COMPONENT_DEFINE("OranReportNrUeTxDrop");
NS_OBJECT_ENSURE_REGISTERED(OranReportNrUeTxDrop);

TypeId
OranReportNrUeTxDrop::GetTypeId()
{
    static TypeId tid =
        TypeId("ns3::OranReportNrUeTxDrop")
            .SetParent<OranReport>()
            .AddConstructor<OranReportNrUeTxDrop>()

            // Se registran como atributos los datos asociados al descarte en transmisión reportado por el UE.
            .AddAttribute("Rnti",
                          "The RNTI associated with the transmission.",
                          UintegerValue(),
                          MakeUintegerAccessor(&OranReportNrUeTxDrop::m_rnti),
                          MakeUintegerChecker<uint16_t>())
            .AddAttribute("Lcid",
                          "The logical channel ID associated with the transmission.",
                          UintegerValue(),
                          MakeUintegerAccessor(&OranReportNrUeTxDrop::m_lcid),
                          MakeUintegerChecker<uint8_t>())
            .AddAttribute("Drops",
                          "Dropped PDU bytes",
                          UintegerValue(),
                          MakeUintegerAccessor(&OranReportNrUeTxDrop::m_drops),
                          MakeUintegerChecker<uint64_t>());

    return tid;
}

OranReportNrUeTxDrop::OranReportNrUeTxDrop()
{
    NS_LOG_FUNCTION(this);
}

OranReportNrUeTxDrop::~OranReportNrUeTxDrop()
{
    NS_LOG_FUNCTION(this);
}

std::string
OranReportNrUeTxDrop::ToString() const
{
    NS_LOG_FUNCTION(this);

    std::stringstream ss;
    Time time = GetTime();

    ss << "OranReportNrUeTxDrop(";
    ss << "E2NodeId=" << GetReporterE2NodeId();
    ss << ";Time=" << time.As(Time::S);
    ss << ";Rnti=" << m_rnti;
    ss << ";Lcid=" << +m_lcid;
    ss << ";Drops=" << m_drops;
    ss << ")";

    return ss.str();
}

uint64_t
OranReportNrUeTxDrop::GetDrops() const
{
    NS_LOG_FUNCTION(this);

    return m_drops;
}

uint16_t
OranReportNrUeTxDrop::GetRnti() const
{
    NS_LOG_FUNCTION(this);

    return m_rnti;
}

uint8_t
OranReportNrUeTxDrop::GetLcid() const
{
    NS_LOG_FUNCTION(this);

    return m_lcid;
}

} // namespace ns3
