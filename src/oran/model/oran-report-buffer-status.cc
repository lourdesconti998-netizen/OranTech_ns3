/////////////////////////////////////////////////////////////////////////////////
/// Report desarrollado en el marco del proyecto.                             ///
///                                                                           ///
/// Este archivo define un reporte ORAN para transportar el estado del buffer ///
/// de transmisión de un UE. La estructura sigue el estilo de los reportes    ///
/// existentes del módulo ORAN.                                               ///
/////////////////////////////////////////////////////////////////////////////////

#include "oran-report-buffer-status.h"

#include "ns3/log.h"
#include "ns3/uinteger.h"

#include <sstream>

namespace ns3
{

NS_LOG_COMPONENT_DEFINE("OranReportUeTxBuffer");
NS_OBJECT_ENSURE_REGISTERED(OranReportUeTxBuffer);

TypeId
OranReportUeTxBuffer::GetTypeId()
{
    static TypeId tid =
        TypeId("ns3::OranReportUeTxBuffer")
            .SetParent<OranReport>()
            .AddConstructor<OranReportUeTxBuffer>()

            // Se registran como atributos las métricas que componen el reporte de estado de buffer de la UE.
            .AddAttribute("Rnti",
                          "The UE RNTI associated with the buffer status.",
                          UintegerValue(),
                          MakeUintegerAccessor(&OranReportUeTxBuffer::m_rnti),
                          MakeUintegerChecker<uint16_t>())
            .AddAttribute("Lcid",
                          "The logical channel identifier associated with the buffer status.",
                          UintegerValue(),
                          MakeUintegerAccessor(&OranReportUeTxBuffer::m_lcid),
                          MakeUintegerChecker<uint8_t>())
            .AddAttribute("TxQueueSize",
                          "The RLC transmission queue size in bytes.",
                          UintegerValue(),
                          MakeUintegerAccessor(&OranReportUeTxBuffer::m_txQueueSize),
                          MakeUintegerChecker<uint32_t>())
            .AddAttribute("TxQueueHolDelay",
                          "The head-of-line delay of the transmission queue in milliseconds.",
                          UintegerValue(),
                          MakeUintegerAccessor(&OranReportUeTxBuffer::m_txQueueHolDelay),
                          MakeUintegerChecker<uint16_t>())
            .AddAttribute("RetxQueueSize",
                          "The RLC retransmission queue size in bytes.",
                          UintegerValue(),
                          MakeUintegerAccessor(&OranReportUeTxBuffer::m_retxQueueSize),
                          MakeUintegerChecker<uint32_t>())
            .AddAttribute("RetxQueueHolDelay",
                          "The head-of-line delay of the retransmission queue in milliseconds.",
                          UintegerValue(),
                          MakeUintegerAccessor(&OranReportUeTxBuffer::m_retxQueueHolDelay),
                          MakeUintegerChecker<uint16_t>())
            .AddAttribute("StatusPduSize",
                          "The pending STATUS PDU size in bytes.",
                          UintegerValue(),
                          MakeUintegerAccessor(&OranReportUeTxBuffer::m_statusPduSize),
                          MakeUintegerChecker<uint16_t>());

    return tid;
}

OranReportUeTxBuffer::OranReportUeTxBuffer()
{
    NS_LOG_FUNCTION(this);
}

OranReportUeTxBuffer::~OranReportUeTxBuffer()
{
    NS_LOG_FUNCTION(this);
}

// Genera una representación en texto del reporte, útil para logs y depuración.
std::string
OranReportUeTxBuffer::ToString() const
{
    NS_LOG_FUNCTION(this);

    std::stringstream ss;
    Time time = GetTime();

    ss << "OranReportUeTxBuffer(E2NodeId=" << GetReporterE2NodeId()
       << ";Time=" << time.As(Time::S) << ";RNTI=" << +m_rnti
       << ";LCID=" << +m_lcid << ";TxQueueSize=" << m_txQueueSize
       << ";TxQueueHolDelay=" << m_txQueueHolDelay
       << ";RetxQueueSize=" << m_retxQueueSize
       << ";RetxQueueHolDelay=" << m_retxQueueHolDelay
       << ";StatusPduSize=" << m_statusPduSize << ")";

    return ss.str();
}

uint16_t
OranReportUeTxBuffer::GetRnti() const
{
    return m_rnti;
}

uint8_t
OranReportUeTxBuffer::GetLcid() const
{
    return m_lcid;
}

uint32_t
OranReportUeTxBuffer::GetTxQueueSize() const
{
    return m_txQueueSize;
}

uint16_t
OranReportUeTxBuffer::GetTxQueueHolDelay() const
{
    return m_txQueueHolDelay;
}

uint32_t
OranReportUeTxBuffer::GetRetxQueueSize() const
{
    return m_retxQueueSize;
}

uint16_t
OranReportUeTxBuffer::GetRetxQueueHolDelay() const
{
    return m_retxQueueHolDelay;
}

uint16_t
OranReportUeTxBuffer::GetStatusPduSize() const
{
    return m_statusPduSize;
}

} // namespace ns3
