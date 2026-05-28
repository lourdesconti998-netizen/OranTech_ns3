/////////////////////////////////////////////////////////////////////////////
///    LM creado por estudiantes de Facultad de Ingeniería UdelaR         ///
///    tomando como base los LM ya desarrollados en el módulo.            ///
///                                                                       ///
///    Este LM consulta métricas de monitoreo del repositorio de datos,   ///
///    los procesa y los imprime en terminal.                             ///
///                                                                       ///
///    Entre las métricas monitoreadas se incluyen posición, RSRP/RSRQ,   ///
///    pérdida de aplicación, estado del buffer, CQI, paquetes            ///
///    transmitidos y paquetes descartados.                               ///
/////////////////////////////////////////////////////////////////////////////

#include "oran-lm-nr-2-nr-moni.h"
#include "oran-nr-data-repository.h"

#include "ns3/abort.h"
#include "ns3/log.h"
#include "ns3/simulator.h"
#include "ns3/uinteger.h"

#include <cfloat>
#include <cmath>
#include <tuple>
#include <algorithm>

namespace ns3
{

NS_LOG_COMPONENT_DEFINE("OranLmNr2NrMoni");

NS_OBJECT_ENSURE_REGISTERED(OranLmNr2NrMoni);

TypeId
OranLmNr2NrMoni::GetTypeId()
{
    static TypeId tid = TypeId("ns3::OranLmNr2NrMoni")
                            .SetParent<OranLm>()
                            .AddConstructor<OranLmNr2NrMoni>();

    return tid;
}

OranLmNr2NrMoni::OranLmNr2NrMoni()
    : OranLm()
{
    LogComponentEnable("OranLmNr2NrMoni", LOG_LEVEL_INFO);
    NS_LOG_FUNCTION(this);

    m_name = "OranLmNr2NrMoni";
}

OranLmNr2NrMoni::~OranLmNr2NrMoni()
{
    NS_LOG_FUNCTION(this);
}

std::vector<Ptr<OranCommand>>
OranLmNr2NrMoni::Run()
{
    double tick = Simulator::Now().GetSeconds();
    NS_LOG_UNCOND("\n t=" << tick << "s - running LM: " << m_name);

    NS_LOG_FUNCTION(this);

    std::vector<Ptr<OranCommand>> commands;

    if (m_active)
    {
        NS_ABORT_MSG_IF(m_nearRtRic == nullptr,
                        "Attempting to run LM (" + m_name + ") with NULL Near-RT RIC");

        Ptr<OranNrDataRepository> data = m_nearRtRic->Data();

        // Se obtienen los UE y los gNB registrados en el repositorio del Near-RT RIC.
        std::vector<UeInfo> ueInfos = GetUeInfos(data);
        std::vector<GnbInfo> gnbInfos = GetGnbInfos(data);

        // Para cada UE detectado, se consultan las métricas disponibles en el repositorio.
        for (const auto& ueInfo : ueInfos)
        {
            // Posición.
            NS_LOG_UNCOND("[POS] UE NodeId=" << ueInfo.nodeId
                           << " CellId=" << ueInfo.cellId
                           << " Pos=(" << ueInfo.position.x
                           << ", " << ueInfo.position.y
                           << ", " << ueInfo.position.z << ")");

            // Calidad de señal.
            GetRsrpInfos(data, ueInfo.nodeId);

            // Pérdida de paquetes a nivel de aplicación.
            GetAppLossInfos(data, ueInfo.nodeId);

            // Estado del buffer de transmisión del UE.
            GetBufferInfos(data, ueInfo.nodeId);

            // Calidad de canal.
            GetCqiInfos(data, ueInfo.nodeId);
            
            // Envío de paquetes del UE.
            GetTxPduInfos(data, ueInfo.nodeId);
            
            // Descarte de paquetes del UE.
	        GetTxDropInfos(data, ueInfo.nodeId);
        }
    }

    return commands;
}

// Obtiene información básica de los UE registrados en el repositorio,
// incluyendo NodeId, CellId, RNTI y última posición conocida.
std::vector<OranLmNr2NrMoni::UeInfo>
OranLmNr2NrMoni::GetUeInfos(Ptr<OranNrDataRepository> data) const
{
    NS_LOG_FUNCTION(this << data);

    std::vector<UeInfo> ueInfos;

    for (uint64_t ueId : data->GetNrUeE2NodeIds())
    {
        UeInfo ueInfo;
        ueInfo.nodeId = ueId;

        bool found = false;
        std::tie(found, ueInfo.cellId, ueInfo.rnti) = data->GetNrUeCellInfo(ueInfo.nodeId);

        if (!found)
        {
            NS_LOG_INFO("Could not find NR UE cell info for E2 Node ID = " << ueInfo.nodeId);
            continue;
        }

        std::map<Time, Vector> nodePositions =
            data->GetNodePositions(ueInfo.nodeId, Seconds(0), Simulator::Now());

        if (nodePositions.empty())
        {
            NS_LOG_INFO("Could not find NR UE location for E2 Node ID = " << ueInfo.nodeId);
            continue;
        }

        ueInfo.position = nodePositions.rbegin()->second;
        ueInfos.push_back(ueInfo);

    }

    return ueInfos;
}

// Obtiene información básica de los gNB registrados en el repositorio,
// incluyendo NodeId, CellId y última posición conocida.
std::vector<OranLmNr2NrMoni::GnbInfo>
OranLmNr2NrMoni::GetGnbInfos(Ptr<OranNrDataRepository> data) const
{
    NS_LOG_FUNCTION(this << data);

    std::vector<GnbInfo> gnbInfos;

    for (uint64_t gnbId : data->GetNrGnbE2NodeIds())
    {
        GnbInfo gnbInfo;
        gnbInfo.nodeId = gnbId;

        bool found = false;
        std::tie(found, gnbInfo.cellId) = data->GetNrGnbCellInfo(gnbInfo.nodeId);

        if (!found)
        {
            NS_LOG_INFO("Could not find NR gNB cell info for E2 Node ID = " << gnbInfo.nodeId);
            continue;
        }

        std::map<Time, Vector> nodePositions =
            data->GetNodePositions(gnbInfo.nodeId, Seconds(0), Simulator::Now());

        if (nodePositions.empty())
        {
            NS_LOG_INFO("Could not find NR gNB location for E2 Node ID = " << gnbInfo.nodeId);
            continue;
        }

        gnbInfo.position = nodePositions.rbegin()->second;
        gnbInfos.push_back(gnbInfo);
    }

    return gnbInfos;
}

// Consulta las mediciones RSRP/RSRQ asociadas a una UE.
std::vector<OranLmNr2NrMoni::RsrpInfo>
OranLmNr2NrMoni::GetRsrpInfos(Ptr<OranNrDataRepository> data, uint64_t ueNodeId) const
{
    NS_LOG_FUNCTION(this << data << ueNodeId);

    std::vector<RsrpInfo> rsrpInfos;
    auto measurements = data->GetNrUeRsrpRsrq(ueNodeId);

    for (const auto& measurement : measurements)
    {
        RsrpInfo rsrpInfo{};
        rsrpInfo.nodeId = ueNodeId;
        std::tie(rsrpInfo.rnti,
                 rsrpInfo.cellId,
                 rsrpInfo.rsrp,
                 rsrpInfo.rsrq,
                 rsrpInfo.isServingCell,
                 rsrpInfo.componentCarrierId) = measurement;
        rsrpInfos.push_back(rsrpInfo);

        NS_LOG_INFO("[RSRP] UE NodeId=" << rsrpInfo.nodeId << " RNTI=" << rsrpInfo.rnti
                                                << " CellId=" << rsrpInfo.cellId
                                                << " RSRP=" << rsrpInfo.rsrp
                                                << " RSRQ=" << rsrpInfo.rsrq
                                                << " Serving=" << rsrpInfo.isServingCell);
    }

    if (rsrpInfos.empty())
    {
        NS_LOG_INFO("Could not find NR UE RSRP/RSRQ info for E2 Node ID = " << ueNodeId);
    }

    return rsrpInfos;
}

// Consulta la pérdida de paquetes a nivel de aplicación para una UE.
std::vector<OranLmNr2NrMoni::AppLossInfo>
OranLmNr2NrMoni::GetAppLossInfos(Ptr<OranNrDataRepository> data, uint64_t ueNodeId) const
{
    NS_LOG_FUNCTION(this << data << ueNodeId);

    std::vector<AppLossInfo> infos;

    // Obtener el valor de pérdida desde el repositorio.
    double loss = data->GetAppLoss(ueNodeId);

    AppLossInfo info;
    info.nodeId = ueNodeId;
    info.lossRatio = loss;          // Valor normalizado entre 0 y 1.

    infos.push_back(info);

    NS_LOG_UNCOND("[APPLOSS] UE NodeId=" << ueNodeId
               << " Loss=" << info.lossRatio * 100 << "%");

    return infos;
}

// Consulta el estado del buffer de transmisión de una UE.
std::vector<OranLmNr2NrMoni::BufferStatusInfo>
OranLmNr2NrMoni::GetBufferInfos(Ptr<OranNrDataRepository> data, uint64_t ueNodeId) const
{
    NS_LOG_FUNCTION(this << data << ueNodeId);

    std::vector<BufferStatusInfo> bufferInfos;
    auto bufList = data->GetNrUeBufferStatus(ueNodeId);

    // Ordena ascendente por timestamp.
    std::sort(bufList.begin(), bufList.end(),
              [](const auto& a, const auto& b) {
                  return a.time < b.time;
              });

    BufferStatusInfo result{};
    result.nodeId = ueNodeId;
    result.lcid = 3;                    // El LCID de datos
    result.txQueueSize = 0;
    result.retxQueueSize = 0;

    bool found = false;

    for (const auto& status : bufList)
    {
        if (status.lcid == 3)
        {
            result.rnti = status.rnti;
            result.time = status.time;
            result.txQueueSize = status.txQueueSize;
            result.retxQueueSize = status.retxQueueSize;
            found = true;
        }
    }

    if (found)
    {
        NS_LOG_UNCOND("[BUFFER_STATUS] UE " << ueNodeId
                       << " LCID=3 TxQueue=" << result.txQueueSize
                       << " Retx=" << result.retxQueueSize);
    }
    else
    {
        NS_LOG_UNCOND("[BUFFER_STATUS] UE " << ueNodeId
                       << " LCID=3 TxQueue=0 (no data yet)");
    }

    bufferInfos.push_back(result);
    return bufferInfos;
}

// Consulta el último valor disponible de CQI, MCS y RI para una UE.
std::vector<OranLmNr2NrMoni::CqiInfo>
OranLmNr2NrMoni::GetCqiInfos(Ptr<OranNrDataRepository> data, uint64_t ueNodeId) const
{
    NS_LOG_FUNCTION(this << data << ueNodeId);

    std::vector<CqiInfo> infos;

    
    auto cqiList = data->GetNrUeCqi(ueNodeId);

    if (cqiList.empty())
    {
        NS_LOG_UNCOND("[CQI] UE NodeId=" << ueNodeId << " (no data yet)");
        return infos;
    }

    const auto& last = cqiList.back();

    CqiInfo info;
    info.nodeId = ueNodeId;
    info.time = last.time;
    info.rnti = last.rnti;
    info.cqi = last.cqi;
    info.mcs = last.mcs;
    info.ri  = last.ri;

    infos.push_back(info);

    NS_LOG_UNCOND("[CQI] UE NodeId=" << ueNodeId
                   << " RNTI=" << info.rnti
                   << " CQI=" << +info.cqi
                   << " MCS=" << +info.mcs
                   << " RI="  << +info.ri);

    return infos;
}

// Consulta los paquetes descartados en transmisión para un UE.
std::vector<OranLmNr2NrMoni::TxDropInfo>
OranLmNr2NrMoni::GetTxDropInfos(Ptr<OranNrDataRepository> data, uint64_t ueNodeId) const
{
    NS_LOG_FUNCTION(this << data << ueNodeId);

    std::vector<TxDropInfo> infos;

    auto list = data->GetNrUeTxDrop(ueNodeId); 
    if (list.empty())
    {
        NS_LOG_UNCOND("[TXDROP] UE NodeId=" << ueNodeId << "no data");
        return infos;
    }
    // Se ordenan las muestras por tiempo para identificar la más reciente.
    std::sort(list.begin(), list.end(),
              [](const auto& a, const auto& b) { return a.time < b.time; }); 
    
    bool found = false;
    uint16_t cellId = 0;
    uint16_t rnti = 0;
    // Se obtiene el RNTI actual de la UE para filtrar las muestras correspondientes.
    std::tie(found, cellId, rnti) = data->GetNrUeCellInfo(ueNodeId);
    if (!found)
    {
        NS_LOG_UNCOND("[TXDROP] UE NodeId=" << ueNodeId << " no cell info");
        return infos;
    }
    uint64_t totalTxDrop = 0;
    Time newest = Seconds(0);
    uint8_t newestLcid = 0;
    bool hasSample = false;
    // Se acumulan los descartes de transmisión asociados al RNTI de la UE.
    for (const auto& sample : list)
    {
        if (sample.rnti != rnti)
        {
            continue;
        }

        hasSample = true;
        totalTxDrop += sample.txDrop;
        if (sample.time > newest)
        {
            newest = sample.time;
            newestLcid = sample.lcid;
        }
    }

    if (!hasSample)
    {
        NS_LOG_UNCOND("[TXDROP] UE NodeId=" << ueNodeId << " no samples for RNTI=" << rnti);
        return infos;
    }
    // Se almacena el total acumulado y el tiempo de la muestra más reciente.
    TxDropInfo info;
    info.nodeId = ueNodeId;
    info.time   = newest;
    info.rnti   = rnti;
    info.lcid   = newestLcid;
    info.txDrop = totalTxDrop;
    

    infos.push_back(info);

    NS_LOG_UNCOND("[TXDROP] UE NodeId=" << ueNodeId
               << " RNTI=" << info.rnti
               << " LCID=" << +info.lcid
               << " TxDrop=" << info.txDrop);

    return infos;
}

// Consulta los paquetes transmitidos por el UE.
std::vector<OranLmNr2NrMoni::TxPduInfo>
OranLmNr2NrMoni::GetTxPduInfos(Ptr<OranNrDataRepository> data, uint64_t ueNodeId) const
{
    NS_LOG_FUNCTION(this << data << ueNodeId);

    std::vector<TxPduInfo> infos;

    auto list = data->GetNrUeTxPdu(ueNodeId); 

    if (list.empty())
    {
        NS_LOG_UNCOND("[TXPDU] UE NodeId=" << ueNodeId << "no data");
        return infos;
    }

    std::sort(list.begin(), list.end(),
              [](const auto& a, const auto& b) { return a.time < b.time; }); 

    const auto& last = list.back();

    TxPduInfo info;
    info.nodeId = ueNodeId;
    info.time   = last.time;    
    info.rnti   = last.rnti;      
    info.lcid   = last.lcid;      
    info.txPdu  = last.txPdu;   
    infos.push_back(info);

    NS_LOG_UNCOND("[TXPDU] UE NodeId=" << ueNodeId       
                   << " TxPdu=" << info.txPdu);

    return infos;
}
} // namespace ns3
