/////////////////////////////////////////////////////////////////////////////////
/// Report desarrollado en el marco del proyecto.                             ///
///                                                                           ///
/// Implementa un reporte ORAN para transportar métricas de calidad de canal  ///
/// reportadas por un UE. En particular, incluye CQI, MCS y RI. Estas         ///
/// métricas son recibidas por el Near-RT RIC y almacenadas en el repositorio ///
/// de datos.                                                                 ///
/////////////////////////////////////////////////////////////////////////////////


#include "oran-report-nr-ue-cqi.h"

#include "ns3/log.h"
#include "ns3/uinteger.h"

#include <sstream>

namespace ns3
{

NS_LOG_COMPONENT_DEFINE("OranReportNrUeCqi");
NS_OBJECT_ENSURE_REGISTERED(OranReportNrUeCqi);

TypeId
OranReportNrUeCqi::GetTypeId()
{
    static TypeId tid =
        TypeId("ns3::OranReportNrUeCqi")
            .SetParent<OranReport>()
            .AddConstructor<OranReportNrUeCqi>()
            
            // Se registran como atributos las métricas de calidad de canal reportadas por la UE.
            .AddAttribute("Rnti",
                          "The RNTI of the UE.",
                          UintegerValue(0),
                          MakeUintegerAccessor(&OranReportNrUeCqi::m_rnti),
                          MakeUintegerChecker<uint16_t>())
            .AddAttribute("Cqi",
                          "The wideband CQI reported by the UE.",
                          UintegerValue(0),
                          MakeUintegerAccessor(&OranReportNrUeCqi::m_cqi),
                          MakeUintegerChecker<uint8_t>())
            .AddAttribute("Mcs",
                          "The MCS corresponding to the CQI.",
                          UintegerValue(0),
                          MakeUintegerAccessor(&OranReportNrUeCqi::m_mcs),
                          MakeUintegerChecker<uint8_t>())
            .AddAttribute("Ri",
                          "The rank indicator reported by the UE.",
                          UintegerValue(0),
                          MakeUintegerAccessor(&OranReportNrUeCqi::m_ri),
                          MakeUintegerChecker<uint8_t>());

    return tid;
}

OranReportNrUeCqi::OranReportNrUeCqi()
    : OranReport()
{
    NS_LOG_FUNCTION(this);
}

OranReportNrUeCqi::~OranReportNrUeCqi()
{
    NS_LOG_FUNCTION(this);
}

std::string
OranReportNrUeCqi::ToString() const
{
    std::stringstream ss;
    Time time = GetTime();

    ss << "OranReportNrUeCqi(";
    ss << "E2NodeId=" << GetReporterE2NodeId() << ";Time=" << time.As(Time::S)
       << ";Rnti=" << m_rnti << ";Cqi=" << +m_cqi << ";Mcs=" << +m_mcs << ";Ri=" << +m_ri;
    ss << ")";

    return ss.str();
}

uint16_t
OranReportNrUeCqi::GetRnti() const
{
    NS_LOG_FUNCTION(this);

    return m_rnti;
}

uint8_t
OranReportNrUeCqi::GetCqi() const
{
    NS_LOG_FUNCTION(this);

    return m_cqi;
}

uint8_t
OranReportNrUeCqi::GetMcs() const
{
    NS_LOG_FUNCTION(this);

    return m_mcs;
}

uint8_t
OranReportNrUeCqi::GetRi() const
{
    NS_LOG_FUNCTION(this);

    return m_ri;
}

} // namespace ns3
