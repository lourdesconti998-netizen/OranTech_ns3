/////////////////////////////////////////////////////////////////////////////////
/// Report desarrollado en el marco del proyecto.                             ///
///                                                                           ///
/// Implementa un reporte ORAN para transportar información de bytes de PDU   ///
/// transmitidos por un UE. Esta métrica es recibida por el Near-RT RIC       ///
/// y almacenada en el repositorio de datos.                                  ///
/////////////////////////////////////////////////////////////////////////////////


#include "oran-report-nr-ue-tx-pdu.h"

#include "oran-report.h"

#include "ns3/log.h"
#include "ns3/uinteger.h"

namespace ns3
{

NS_LOG_COMPONENT_DEFINE("OranReportNrUeTxPdu");
NS_OBJECT_ENSURE_REGISTERED(OranReportNrUeTxPdu);

TypeId
OranReportNrUeTxPdu::GetTypeId()
{
    static TypeId tid =
        TypeId("ns3::OranReportNrUeTxPdu")
            .SetParent<OranReport>()
            .AddConstructor<OranReportNrUeTxPdu>()

            // Se registran como atributos los datos asociados a las PDU transmitidas por el UE.
            .AddAttribute("Rnti",
                          "The RNTI associated with the transmission.",
                          UintegerValue(),
                          MakeUintegerAccessor(&OranReportNrUeTxPdu::m_rnti),
                          MakeUintegerChecker<uint16_t>())
            .AddAttribute("Lcid",
                          "The logical channel ID associated with the transmission.",
                          UintegerValue(),
                          MakeUintegerAccessor(&OranReportNrUeTxPdu::m_lcid),
                          MakeUintegerChecker<uint8_t>())
            .AddAttribute("Bytes",
                          "Transmitted PDU bytes",
                          UintegerValue(),
                          MakeUintegerAccessor(&OranReportNrUeTxPdu::m_bytes),
                          MakeUintegerChecker<uint64_t>());

    return tid;
}

OranReportNrUeTxPdu::OranReportNrUeTxPdu()
{
    NS_LOG_FUNCTION(this);
}

OranReportNrUeTxPdu::~OranReportNrUeTxPdu()
{
    NS_LOG_FUNCTION(this);
}

std::string
OranReportNrUeTxPdu::ToString() const
{
    NS_LOG_FUNCTION(this);

    std::stringstream ss;
    Time time = GetTime();

    ss << "OranReportNrUeTxPdu(";
    ss << "E2NodeId=" << GetReporterE2NodeId();
    ss << ";Time=" << time.As(Time::S);
    ss << ";Rnti=" << m_rnti;
    ss << ";Lcid=" << +m_lcid;    
    ss << ";Bytes=" << m_bytes;
    ss << ")";

    return ss.str();
}

uint64_t
OranReportNrUeTxPdu::GetBytes() const
{
    NS_LOG_FUNCTION(this);

    return m_bytes;
}

uint16_t
OranReportNrUeTxPdu::GetRnti() const
{
    NS_LOG_FUNCTION(this);

    return m_rnti;
}

uint8_t
OranReportNrUeTxPdu::GetLcid() const
{
    NS_LOG_FUNCTION(this);

    return m_lcid;
}

} // namespace ns3
