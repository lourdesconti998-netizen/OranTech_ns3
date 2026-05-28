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

#include "oran-e2-node-terminator-nr-gnb.h"

#include "oran-command-nr-2-nr-handover.h"
#include "oran-command-nr-ul-rbg-targets.h"

#include "ns3/abort.h"
#include "ns3/log.h"
#include "ns3/nr-gnb-net-device.h"
#include "ns3/nr-gnb-rrc.h"
#include "ns3/node.h"
#include "ns3/pointer.h"
#include "ns3/string.h"

// Modificación incorporada en el marco del proyecto:
// se agrega soporte para comandos que transportan targets de RBGs uplink
// por UE desde el Logic Module hacia el scheduler del gNB.
#include "ns3/nr-mac-scheduler-ofdma-ul-target.h"


namespace ns3
{

NS_LOG_COMPONENT_DEFINE("OranE2NodeTerminatorNrGnb");

NS_OBJECT_ENSURE_REGISTERED(OranE2NodeTerminatorNrGnb);

TypeId
OranE2NodeTerminatorNrGnb::GetTypeId()
{
    static TypeId tid = TypeId("ns3::OranE2NodeTerminatorNrGnb")
                            .SetParent<OranNrE2NodeTerminator>()
                            .AddConstructor<OranE2NodeTerminatorNrGnb>();

    return tid;
}

OranE2NodeTerminatorNrGnb::OranE2NodeTerminatorNrGnb()
    : OranNrE2NodeTerminator()
{
    NS_LOG_FUNCTION(this);
}

OranE2NodeTerminatorNrGnb::~OranE2NodeTerminatorNrGnb()
{
    NS_LOG_FUNCTION(this);
}

OranNrNearRtRic::NodeType
OranE2NodeTerminatorNrGnb::GetNodeType() const
{
    NS_LOG_FUNCTION(this);

    return OranNrNearRtRic::NodeType::NRGNB;
}

// Esta función pertenece al flujo original de procesamiento de comandos del
// E2 Node Terminator del gNB. En este proyecto se extendió para soportar,
// además de comandos de handover, comandos de asignación de targets UL.
void
OranE2NodeTerminatorNrGnb::ReceiveCommand(Ptr<OranCommand> command)
{
    NS_LOG_FUNCTION(this << command);

    if (m_active)
    {
        if (command->GetInstanceTypeId() == OranCommandNr2NrHandover::GetTypeId())
        {
            Ptr<OranCommandNr2NrHandover> handoverCommand =
                command->GetObject<OranCommandNr2NrHandover>();

            Ptr<NrGnbNetDevice> nrGnbNetDev = GetNrNetDevice()->GetObject<NrGnbNetDevice>();
            NS_ABORT_MSG_IF(nrGnbNetDev == nullptr, "NetDevice no es un gNB");

            Ptr<NrGnbRrc> nrGnbRrc = nrGnbNetDev->GetRrc();
            nrGnbRrc->SendHandoverRequest(handoverCommand->GetTargetRnti(),
                                           handoverCommand->GetTargetCellId());
        }
        else if (command->GetInstanceTypeId() == OranCommandNrUlRbgTargets::GetTypeId())
        {
            
            // Modificación incorporada en el marco del proyecto:
            // se procesa un nuevo tipo de comando ORAN que permite actualizar,
            // desde el Near-RT RIC, los targets     de RBGs uplink por UE en el scheduler.
            Ptr<OranCommandNrUlRbgTargets> targetsCommand =
                command->GetObject<OranCommandNrUlRbgTargets>();

            Ptr<NrGnbNetDevice> nrGnbNetDev = GetNrNetDevice()->GetObject<NrGnbNetDevice>();
            NS_ABORT_MSG_IF(nrGnbNetDev == nullptr, "NetDevice no es un gNB");

            Ptr<NrMacScheduler> scheduler = nrGnbNetDev->GetScheduler(0);
            Ptr<NrMacSchedulerOfdmaUlTarget> ulTargetScheduler =
                DynamicCast<NrMacSchedulerOfdmaUlTarget>(scheduler);
            if (ulTargetScheduler == nullptr)
            {
                NS_LOG_WARN("UL target scheduler not installed on gNB");
                return;
            }

            ulTargetScheduler->SetUlTargets(targetsCommand->GetUlTargets());
        }
    }
}

Ptr<NetDevice>
OranE2NodeTerminatorNrGnb::GetNrNetDevice() const
{
    NS_LOG_FUNCTION(this);
    Ptr<NetDevice> netDev = GetNode()->GetDevice(GetNetDeviceIndex());
    NS_ABORT_MSG_IF(netDev == nullptr, "Unable to find NetDevice");
    NS_ABORT_MSG_IF(netDev->GetObject<NrGnbNetDevice>() == nullptr,
                    "NetDevice is not an NrGnbNetDevice");
    return netDev;
}

} // namespace ns3
