/**
 * NIST-developed software is provided by NIST as a public service. You may
 * use, copy and distribute copies of the software in any medium, provided that
 * you keep intact this entire notice. You may improve, modify and create
 * derivative works of the software or any portion of the software, and you may
 * copy and distribute such modifications or works. Modified works should carry
 * a notice stating that you changed the software and should note the date and
 * nature of any such change. Please explicitly acknowledge the National
 * Institute of Standards and Technology as the source of the software.
 *
 * NIST-developed software is expressly provided "AS IS." NIST MAKES NO
 * WARRANTY OF ANY KIND, EXPRESS, IMPLIED, IN FACT OR ARISING BY OPERATION OF
 * LAW, INCLUDING, WITHOUT LIMITATION, THE IMPLIED WARRANTY OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE, NON-INFRINGEMENT AND DATA ACCURACY. NIST
 * NEITHER REPRESENTS NOR WARRANTS THAT THE OPERATION OF THE SOFTWARE WILL BE
 * UNINTERRUPTED OR ERROR-FREE, OR THAT ANY DEFECTS WILL BE CORRECTED. NIST
 * DOES NOT WARRANT OR MAKE ANY REPRESENTATIONS REGARDING THE USE OF THE
 * SOFTWARE OR THE RESULTS THEREOF, INCLUDING BUT NOT LIMITED TO THE
 * CORRECTNESS, ACCURACY, RELIABILITY, OR USEFULNESS OF THE SOFTWARE.
 *
 * You are solely responsible for determining the appropriateness of using and
 * distributing the software and you assume all risks associated with its use,
 * including but not limited to the risks and costs of program errors,
 * compliance with applicable laws, damage to or loss of data, programs or
 * equipment, and the unavailability or interruption of operation. This
 * software is not intended to be used in any situation where a failure could
 * cause risk of injury or damage to property. The software developed by NIST
 * employees is not subject to copyright protection within the United States.
 */

#include "oran-nr-near-rt-ric-e2terminator.h"
#include "oran-e2-node-terminator.h" // nuevo

#include "oran-command.h"
#include "oran-nr-data-repository.h" 
#include "oran-nr-near-rt-ric.h"
// Modificación del proyecto:
// Se incorporan terminadores específicos para nodos NR UE y NR gNB.
#include "oran-e2-node-terminator-nr-gnb.h"
#include "oran-e2-node-terminator-nr-ue.h"

#include "oran-report-apploss.h"
#include "oran-report-location.h"
#include "oran-report-nr-ue-cell-info.h"
#include "oran-report-nr-ue-rsrp-rsrq.h"

// Modificación del proyecto:
// Se agregan reportes NR utilizados para monitoreo.
#include "oran-report-nr-ue-cqi.h"
#include "oran-report-buffer-status.h"
#include "oran-report-nr-ue-tx-drop.h"
#include "oran-report-nr-ue-tx-pdu.h"

#include "oran-report.h"

#include "ns3/abort.h"
#include "ns3/log.h"
#include "ns3/nr-gnb-net-device.h"
#include "ns3/nr-ue-net-device.h"
#include "ns3/nr-ue-rrc.h" 
#include "ns3/nr-gnb-rrc.h" 

#include "ns3/pointer.h"
#include "ns3/simulator.h"
#include "ns3/string.h"

namespace ns3
{

NS_LOG_COMPONENT_DEFINE("OranNrNearRtRicE2Terminator");
NS_OBJECT_ENSURE_REGISTERED(OranNrNearRtRicE2Terminator);
TypeId
OranNrNearRtRicE2Terminator::GetTypeId()
{
    static TypeId tid =
        TypeId("ns3::OranNrNearRtRicE2Terminator")
            .SetParent<Object>()
            .AddConstructor<OranNrNearRtRicE2Terminator>()

            .AddAttribute("NearRtRic",
                          "Pointer to the Near-RT RIC",
                          PointerValue(nullptr),
                          MakePointerAccessor(&OranNrNearRtRicE2Terminator::m_nearRtRic),
                          MakePointerChecker<OranNrNearRtRic>())
            .AddAttribute("DataRepository",
                          "Pointer to the Data Repository",
                          PointerValue(nullptr),
                          MakePointerAccessor(&OranNrNearRtRicE2Terminator::m_data),
                          MakePointerChecker<OranNrDataRepository>())
            .AddAttribute("TransmissionDelayRv",
                          "The random variable used (in seconds) to calculate the transmission "
                          "delay for a command.",
                          StringValue("ns3::ConstantRandomVariable[Constant=0]"),
                          MakePointerAccessor(&OranNrNearRtRicE2Terminator::m_transmissionDelayRv),
                          MakePointerChecker<RandomVariableStream>());

    return tid;
}

OranNrNearRtRicE2Terminator::OranNrNearRtRicE2Terminator()

    : Object(),
      m_active(false),
      m_nodeTerminators(std::map<uint64_t, Ptr<OranNrE2NodeTerminator>>())
{
    NS_LOG_FUNCTION(this);
}

OranNrNearRtRicE2Terminator::~OranNrNearRtRicE2Terminator()


{
    NS_LOG_FUNCTION(this);
}

void
OranNrNearRtRicE2Terminator::Activate()
{
    NS_LOG_FUNCTION(this);

    m_active = true;
}

void
OranNrNearRtRicE2Terminator::Deactivate()
{
    NS_LOG_FUNCTION(this);

    m_active = false;
}

bool
OranNrNearRtRicE2Terminator::IsActive() const
{
    NS_LOG_FUNCTION(this);

    return m_active;
}

void
OranNrNearRtRicE2Terminator::ReceiveRegistrationRequest(OranNrNearRtRic::NodeType type,
                                                      uint64_t id,
                                                      Ptr<OranNrE2NodeTerminator> terminator)
{
    NS_LOG_FUNCTION(this << type << id << terminator);

    if (m_active)
    {
        NS_ABORT_MSG_IF(
            m_data == nullptr,
            "Attempting to use a null data repository in the Near-RT RIC E2 Terminator");
        NS_ABORT_MSG_IF(terminator == nullptr, "Attempting to register a NULL Node E2 Terminator");

        uint64_t e2NodeId;
        switch (type)
        {
        case OranNrNearRtRic::NodeType::NRUE:
        {
            Ptr<OranE2NodeTerminatorNrUe> ueTerminator =
                terminator->GetObject<OranE2NodeTerminatorNrUe>();
            if (ueTerminator)
            {
               Ptr<NrUeNetDevice> nrUeNetDev = DynamicCast<NrUeNetDevice>(ueTerminator->GetNrNetDevice());
               NS_ABORT_MSG_IF(nrUeNetDev == nullptr, "Unable to cast to NrUeNetDevice");
               e2NodeId = m_data->RegisterNodeNrUe(id, 
                                                nrUeNetDev
                                                ->GetRrc()
                                                ->GetImsi());   

            }

            break;
        }
        case OranNrNearRtRic::NodeType::NRGNB:
        { 
             Ptr<OranE2NodeTerminatorNrGnb> gnbTerminator =
              terminator->GetObject<OranE2NodeTerminatorNrGnb>();
              Ptr<NrGnbNetDevice> nrGnbNetDev = DynamicCast<NrGnbNetDevice>(gnbTerminator->GetNrNetDevice());
              NS_ABORT_MSG_IF(nrGnbNetDev == nullptr, "Unable to cast to NrGnbNetDevice");
              e2NodeId = m_data->RegisterNodeNrGnb(id, 
                                            nrGnbNetDev->GetCellId());
            break;
              }
        default:
        {
            e2NodeId = m_data->RegisterNode(type, id);
            break;
        }
        }
        m_nodeTerminators[e2NodeId] = terminator;

        Simulator::Schedule(Seconds(m_transmissionDelayRv->GetValue()),
                            &OranNrE2NodeTerminator::ReceiveRegistrationResponse,
                            terminator,
                            e2NodeId);
    }
}

void
OranNrNearRtRicE2Terminator::ReceiveDeregistrationRequest(uint64_t e2NodeId)
{
    NS_LOG_FUNCTION(this << e2NodeId);

    if (m_active)
    {
        NS_ABORT_MSG_IF(
            m_data == nullptr,
            "Attempting to use a null data repository in the Near-RT RIC E2 Terminator");

        uint64_t deregisteredE2NodeId = m_data->DeregisterNode(e2NodeId);

        Simulator::Schedule(Seconds(m_transmissionDelayRv->GetValue()),
                            &OranNrE2NodeTerminator::ReceiveDeregistrationResponse,
                            m_nodeTerminators[e2NodeId],
                            deregisteredE2NodeId);
    }
}

void
OranNrNearRtRicE2Terminator::ReceiveReport(Ptr<OranReport> report)
{
    NS_LOG_FUNCTION(this << report->ToString());

    if (m_active)
    {
        NS_ABORT_MSG_IF(
            m_data == nullptr,
            "Attempting to use a null data repository in the Near-RT RIC E2 Terminator");

        if (report->GetInstanceTypeId() == TypeId::LookupByName("ns3::OranReportLocation"))
        {
            Ptr<OranReportLocation> posRpt = report->GetObject<OranReportLocation>();
            m_data->SavePosition(posRpt->GetReporterE2NodeId(),
                                 posRpt->GetLocation(),
                                 posRpt->GetTime());
        }
        else if (report->GetInstanceTypeId() ==
                 TypeId::LookupByName("ns3::OranReportNrUeCellInfo"))
        {
            Ptr<OranReportNrUeCellInfo> NRUECellInfoRpt =
                report->GetObject<OranReportNrUeCellInfo>();
            m_data->SaveNrUeCellInfo(NRUECellInfoRpt->GetReporterE2NodeId(),
                                      NRUECellInfoRpt->GetCellId(),
                                      NRUECellInfoRpt->GetRnti(),
                                      NRUECellInfoRpt->GetTime());
        }
        else if (report->GetInstanceTypeId() == TypeId::LookupByName("ns3::OranReportAppLoss"))
        {
            Ptr<OranReportAppLoss> appLossRpt = report->GetObject<OranReportAppLoss>();
            m_data->SaveAppLoss(appLossRpt->GetReporterE2NodeId(),
                                appLossRpt->GetLoss(),
                                appLossRpt->GetTime());
        }
        else if (report->GetInstanceTypeId() ==
                 TypeId::LookupByName("ns3::OranReportNrUeRsrpRsrq"))
        {
            Ptr<OranReportNrUeRsrpRsrq> rsrpRsrqRpt = report->GetObject<OranReportNrUeRsrpRsrq>();
            m_data->SaveNrUeRsrpRsrq(rsrpRsrqRpt->GetReporterE2NodeId(),
                                      rsrpRsrqRpt->GetTime(),
                                      rsrpRsrqRpt->GetRnti(),
                                      rsrpRsrqRpt->GetCellId(),
                                      rsrpRsrqRpt->GetRsrp(),
                                      rsrpRsrqRpt->GetRsrq(),
                                      rsrpRsrqRpt->GetIsServingCell(),
                                      rsrpRsrqRpt->GetComponentCarrierId());
        }
            
        // Modificación del proyecto:
        // Procesamiento de reportes de CQI, MCS y RI del UE.
        // La información se convierte al formato interno del repositorio.
        else if (report->GetInstanceTypeId() ==
                 TypeId::LookupByName("ns3::OranReportNrUeCqi"))
        {
            Ptr<OranReportNrUeCqi> cqiRpt = report->GetObject<OranReportNrUeCqi>();
            OranNrDataRepository::NrUeCqi cqiSample;
            cqiSample.time = cqiRpt->GetTime();
            cqiSample.rnti = cqiRpt->GetRnti();
            cqiSample.cqi = cqiRpt->GetCqi();
            cqiSample.mcs = cqiRpt->GetMcs();
            cqiSample.ri = cqiRpt->GetRi();
            m_data->SaveNrUeCqi(cqiRpt->GetReporterE2NodeId(), cqiSample);
        }

        // Modificación del proyecto:
        // Procesamiento de reportes de estado de buffer de transmisión del UE.
        // Estas métricas permiten observar la ocupación de cola por LCID.
        else if (report->GetInstanceTypeId() ==
                 TypeId::LookupByName("ns3::OranReportUeTxBuffer"))
        {
            Ptr<OranReportUeTxBuffer> txBufferRpt = report->GetObject<OranReportUeTxBuffer>();
            OranNrDataRepository::NrUeBufferStatus status;
            status.time = txBufferRpt->GetTime();
            status.rnti = txBufferRpt->GetRnti();
            status.lcid = txBufferRpt->GetLcid();
            status.txQueueSize = txBufferRpt->GetTxQueueSize();
            status.txQueueHolDelay = txBufferRpt->GetTxQueueHolDelay();
            status.retxQueueSize = txBufferRpt->GetRetxQueueSize();
            status.retxQueueHolDelay = txBufferRpt->GetRetxQueueHolDelay();
            status.statusPduSize = txBufferRpt->GetStatusPduSize();
            m_data->SaveNrUeBufferStatus(txBufferRpt->GetReporterE2NodeId(), status);
        }

        // Modificación del proyecto:
        // Procesamiento de reportes de PDU transmitidas por el UE.
        // Esta métrica se usa para estimar el tráfico efectivamente transmitido.
        else if (report->GetInstanceTypeId() ==
                 TypeId::LookupByName("ns3::OranReportNrUeTxPdu"))
        {
            Ptr<OranReportNrUeTxPdu> txPduRpt = report->GetObject<OranReportNrUeTxPdu>();
            OranNrDataRepository::NrUeTxPdu pdu;
            pdu.time = txPduRpt->GetTime();
            pdu.rnti = txPduRpt->GetRnti();
            pdu.lcid = txPduRpt->GetLcid();
            pdu.txPdu = txPduRpt->GetBytes();
            m_data->SaveNrUeTxPdu(txPduRpt->GetReporterE2NodeId(), pdu);
        }

        // Modificación del proyecto:
        // Procesamiento de reportes de paquetes descartados en transmisión.
        // Esta métrica permite registrar pérdidas a nivel de transmisión.
        else if (report->GetInstanceTypeId() ==
                 TypeId::LookupByName("ns3::OranReportNrUeTxDrop"))
        {
            Ptr<OranReportNrUeTxDrop> txDropRpt = report->GetObject<OranReportNrUeTxDrop>();
            OranNrDataRepository::NrUeTxDrop drop;
            drop.time = txDropRpt->GetTime();
            drop.rnti = txDropRpt->GetRnti();
            drop.lcid = txDropRpt->GetLcid();
            drop.txDrop = txDropRpt->GetDrops();
            m_data->SaveNrUeTxDrop(txDropRpt->GetReporterE2NodeId(), drop);
        }


        m_nearRtRic->NotifyReportReceived(report);
    }
}

void
OranNrNearRtRicE2Terminator::SendCommand(Ptr<OranCommand> command)
{
    NS_LOG_FUNCTION(this << command->ToString());

    if (m_active)
    {
        NS_ABORT_MSG_IF(
            m_data == nullptr,
            "Attempting to use a null data repository in the Near-RT RIC E2 Terminator");

        m_data->LogCommandE2Terminator(command);

        Simulator::Schedule(Seconds(m_transmissionDelayRv->GetValue()),
                            &OranNrE2NodeTerminator::ReceiveCommand,
                            m_nodeTerminators[command->GetTargetE2NodeId()],
                            command);
    }
}

void
OranNrNearRtRicE2Terminator::ProcessCommands(std::vector<Ptr<OranCommand>> commands)
{
    NS_LOG_FUNCTION(this);

    if (m_active)
    {
        for (const auto& cmd : commands)
        {
            SendCommand(cmd);
        }
    }
}

void
OranNrNearRtRicE2Terminator::DoDispose()
{
    NS_LOG_FUNCTION(this);

    m_nearRtRic = nullptr;
    m_data = nullptr;
    m_nodeTerminators.clear();
    m_transmissionDelayRv = nullptr;

    Object::DoDispose();
}

} // namespace ns3
