/////////////////////////////////////////////////////////////////////////////
///    LM creado por estudiantes de Facultad de Ingeniería UdelaR         ///
///    tomando como base los LM ya desarrollados en el módulo.            ///
///                                                                       ///
///    Logic Module para control RL basado en MDP.                        ///
///                                                                       ///
///    Este LM obtiene métricas de los UE desde el repositorio de datos,  ///
///    construye una observación y la envía a un agente externo de RL     ///
///    mediante gRPC. La respuesta del agente se transforma en comandos   ///
///    de asignación de RBGs uplink por UE.                               ///
/////////////////////////////////////////////////////////////////////////////


#include "oran-lm-nr-rl-qlearn.h"
#include "oran-command-nr-ul-rbg-targets.h"
#include "oran-nr-data-repository.h"
#include "oran-nr-data-repository-sqlite.h"

#include "ns3/log.h"
#include "ns3/simulator.h"
#include "ns3/uinteger.h"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cfloat>
#include <tuple>
#include <unordered_map>
#include <vector>

using grpc::Channel;
using grpc::ClientContext;
using grpc::Status;

using rl_data_4_qtable::ActionResponse;
using rl_data_4_qtable::ObservationRequest;
using rl_data_4_qtable::RlData4QTable;

namespace ns3
{

NS_LOG_COMPONENT_DEFINE("OranLmNrRlQlearn");
NS_OBJECT_ENSURE_REGISTERED(OranLmNrRlQlearn);

TypeId
OranLmNrRlQlearn::GetTypeId()
{
  static TypeId tid = TypeId("ns3::OranLmNrRlQlearn").SetParent<OranLm>().AddConstructor<OranLmNrRlQlearn>();
  return tid;
}

OranLmNrRlQlearn::OranLmNrRlQlearn()
{
  m_name = "OranLmNrRlQlearn";
  m_lastLmTime = Simulator::Now();
  // Inicializa el cliente gRPC apuntando al servidor local del agente RL.
  m_grpc = std::make_unique<GrpcClient>("127.0.0.1:50051");
}

OranLmNrRlQlearn::~OranLmNrRlQlearn() = default;

std::vector<Ptr<OranCommand>>
OranLmNrRlQlearn::Run()
{    
  // Vector de comandos que serán enviados al Near-RT RIC como resultado del LM.
  std::vector<Ptr<OranCommand>> commands;
    
  // Si el LM no está activo, no se generan comandos.
  if (!m_active)
  {
    return commands;
  }

  NS_ABORT_MSG_IF(m_nearRtRic == nullptr, "MDP RL LM running without a Near-RT RIC");

  // Se obtiene el repositorio de datos del Near-RT RIC y los nodos E2 disponibles.
  Ptr<OranNrDataRepository> data = m_nearRtRic->Data();
  std::vector<uint64_t> ueIds = data->GetNrUeE2NodeIds();
  std::vector<uint64_t> gnbIds = data->GetNrGnbE2NodeIds();

////////////////////////////////////////
///    Comunicación con agente RL    ///
////////////////////////////////////////
if (m_grpc)
{
  // Se construye la observación que será enviada al agente RL.
  ObservationRequest request;

  // Contador lógico utilizado para identificar cada observación enviada.
  static uint64_t ttiCounter = 0;
  request.set_tti_counter(ttiCounter++);

  // Para cada UE se agregan al mensaje las métricas usadas por el agente RL.
  for (auto ueId : ueIds)
  {
    bool found = false;
    uint16_t cellId = 0;
    uint16_t rnti = 0;
    std::tie(found, cellId, rnti) = data->GetNrUeCellInfo(ueId);

    // Se consultan las métricas de buffer, paquetes descartados y PDU transmitidas.
    uint64_t buf  = GetBufferSize(data, ueId, 3);                     
    uint64_t drop = GetTxDrop(data, ueId, 1);                       
    uint64_t pdu  = GetTxPdu(data, ueId, 3);                          

    auto* ue = request.add_ues();
    ue->set_rnti(static_cast<uint32_t>(rnti));
    ue->set_txdrop(drop);
    ue->set_txbuffer(buf);
    ue->set_txpdu(pdu);
  }

  // Se envía la observación al agente RL y se espera una acción como respuesta.
  ActionResponse response;
  grpc::ClientContext context;

  // Se define un tiempo máximo de espera para evitar bloquear la simulación.
  context.set_deadline(std::chrono::system_clock::now() + std::chrono::milliseconds(200));

  grpc::Status status = m_grpc->stub->PushObservation(&context, request, &response);

  if (status.ok())
  {
    // Si la respuesta es válida, se transforma la acción del agente en targets UL por UE.
    if (!gnbIds.empty() && response.ue_shares_size() > 0)
    {
      // Cantidad total de RBGs lógicos disponibles para repartir entre las UE.
      constexpr uint32_t B_FIXED = 533;

      struct Item { uint16_t rnti; double share; };
      std::vector<Item> items;
      items.reserve(response.ue_shares_size());

      double sumShare = 0.0;
      for (int i = 0; i < response.ue_shares_size(); ++i)
      {
        const auto& sh = response.ue_shares(i);
        uint16_t rnti = static_cast<uint16_t>(sh.rnti());
        double share = std::max(0.0, static_cast<double>(sh.share()));
        items.push_back({rnti, share});
        sumShare += share;
      }

      // Se normalizan las proporciones recibidas para asegurar que sumen 1.
      if (sumShare <= 0.0)
      {
        // Si todas las proporciones son cero, se reparte el recurso de forma equitativa.
        double eq = 1.0 / static_cast<double>(items.size());
        for (auto& it : items) it.share = eq;
      }
      else
      {
        for (auto& it : items) it.share /= sumShare;
      }
      // Se convierten las proporciones en targets enteros de RBGs por UE.
      std::unordered_map<uint16_t, uint32_t> ulTargets;
      ulTargets.reserve(items.size());
        
      uint32_t sumT = 0;
      for (const auto& it : items)
      {
        uint32_t t = static_cast<uint32_t>(std::llround(it.share * static_cast<double>(B_FIXED)));
        // Se asegura al menos un RBG lógico por UE.
        if (t < 1) t = 1;
        ulTargets[it.rnti] = t;
        sumT += t;
      }

      // Se corrige el efecto del redondeo para que la suma total sea igual a B_FIXED.
      while (sumT > B_FIXED)
      {
        // Si la suma excede el presupuesto, se reduce el mayor target posible.
        auto best = ulTargets.end();
        for (auto it = ulTargets.begin(); it != ulTargets.end(); ++it)
          if (it->second > 1 && (best == ulTargets.end() || it->second > best->second))
            best = it;

        if (best == ulTargets.end()) break;
        best->second -= 1;
        sumT -= 1;
      }

      while (sumT < B_FIXED)
      {
        // Si quedan RBGs sin asignar, se agregan a la UE con mayor proporción.
        uint16_t bestRnti = items.front().rnti;
        double bestShare = items.front().share;
        for (const auto& it : items)
        { 
          if (it.share > bestShare) 
          { 
              bestShare = it.share; 
              bestRnti = it.rnti; 
          }
        }
        ulTargets[bestRnti] += 1;
        sumT += 1;
      }

      // Se crea el comando que actualiza los target UL en el scheduler del gNB.
      Ptr<OranCommandNrUlRbgTargets> cmd = CreateObject<OranCommandNrUlRbgTargets>();
      cmd->SetAttribute("TargetE2NodeId", UintegerValue(gnbIds.front()));
      cmd->SetUlTargets(ulTargets);
      commands.push_back(cmd);
    }
  }
  else
  {
    NS_LOG_UNCOND("[LM gRPC] ERROR: " << status.error_message());
  }
}
else
{
  NS_LOG_UNCOND("[LM gRPC] ERROR: gRPC client not initialized");
}
  return commands;
}

// Obtiene el promedio de las nuevas muestras de buffer para una UE.
// Para reducir fluctuaciones, el valor se actualiza cada m_N llamadas y
// entre actualizaciones se devuelve el último valor calculado.
uint64_t
OranLmNrRlQlearn::GetBufferSize(Ptr<OranNrDataRepository> data, uint64_t ueId, uint8_t lcid)
{
    // Se incrementa contador de llamadas.
    m_bufferCounter[ueId]++;

    // Si no llega a N devuelve último valor calculado.
    if (m_bufferCounter[ueId] < m_N)
    {
        return m_lastBufferValue[ueId];
    }

    // Cuendo llega a N reiniciar contador.
    m_bufferCounter[ueId] = 0;

    // Se obtienen las muestras desde el repositorio de datos.
    auto samples = data->GetNrUeBufferStatus(ueId);

    // Si no hay nuevas muestras devuelve 0.
    if (samples.empty())
    {
        return 0;
    }
    
    // Inicializa el tiempo de ultima muestra procesada.
    Time lastTime = Seconds(0);

    // Verifica si existen tiempos de muestras para ese ueId.
    if (m_lastBufferTime.count(ueId))
    {
        lastTime = m_lastBufferTime[ueId];
    }

    uint64_t sum = 0;
    uint64_t count = 0;

    // Actualiza timpo de ultima muestra procesada.
    Time newestTime = lastTime;

    for (const auto& s : samples)
    {
        if (s.time > lastTime)
        {
            sum += s.txQueueSize;
            count++;

            if (s.time > newestTime)
            {
                newestTime = s.time;
            }
        }
    }

    if (count == 0) { 
      m_lastBufferTime[ueId] = newestTime;
      m_lastBufferValue[ueId] = 0;
      return 0; 
    }

    uint64_t avg = sum / count;

    // Solo actualiza tiempo y valor si hubo muestras nuevas.
    m_lastBufferTime[ueId] = newestTime;
    m_lastBufferValue[ueId] = avg;

    return avg;
}

// Obtiene el promedio de las nuevas muestras de paquetes descartados para una UE.
// El valor se actualiza cada m_N llamadas y se mantiene el último valor entre consultas.
uint64_t
OranLmNrRlQlearn::GetTxDrop(Ptr<OranNrDataRepository> data, uint64_t ueId, uint8_t lcid)
{
 // Se incrementa contador de llamadas.
    m_txDropCounter[ueId]++;

    // Si no llega a N devuelve último valor calculado.
    if (m_txDropCounter[ueId] < m_N)
    {
        return m_lastTxDropValue[ueId];
    }

    // Cuendo llega a N reiniciar contador.
    m_txDropCounter[ueId] = 0;

    // Se obtienen las muestras desde el repositorio de datos.
    auto samples = data->GetNrUeTxDrop(ueId);

    // Si no hay nuevas muestras devuelve 0.
    if (samples.empty())
    {
        return 0;
    }
    
    // Inicializa tiempo de ultima muestra procesada.
    Time lastTime = Seconds(0);

    // Verifica si existen tiempos de muestras para ese ueId.
    if (m_lastTxDropTime.count(ueId))
    {
        lastTime = m_lastTxDropTime[ueId];
    }

    uint64_t sum = 0;
    uint64_t count = 0;

    // Actualiza timpo de ultima muestra procesada.
    Time newestTime = lastTime;

    for (const auto& s : samples)
    {
        if (s.time > lastTime)
        {
            sum += s.txDrop;
            count++;

            if (s.time > newestTime)
            {
                newestTime = s.time;
            }
        }
    }

    if (count == 0) { 
      m_lastTxDropTime[ueId] = newestTime;
      m_lastTxDropValue[ueId] = 0;
      return 0; 
    }

    uint64_t avg = sum / count;

    // Solo actualiza tiempo y valor si hubo muestras nuevas.
    m_lastTxDropTime[ueId] = newestTime;
    m_lastTxDropValue[ueId] = avg;

    return avg;
}

// Obtiene el promedio de las nuevas muestras de PDU transmitidas para una UE.
// El valor se actualiza cada m_N llamadas y se mantiene el último valor entre consultas.
uint64_t
OranLmNrRlQlearn::GetTxPdu(Ptr<OranNrDataRepository> data, uint64_t ueId, uint8_t lcid)
{
 // Se incrementa contador de llamadas.
    m_txPduCounter[ueId]++;

    // Si no llega a N devuelve último valor calculado.
    if (m_txPduCounter[ueId] < m_N)
    {
        return m_lastTxPduValue[ueId];
    }

    // Cuendo llega a N reiniciar contador.
    m_txPduCounter[ueId] = 0;

    // Se obtienen las muestras desde el repositorio de datos.
    auto samples = data->GetNrUeTxPdu(ueId);

    // Si no hay nuevas muestras devuelve 0.
    if (samples.empty())
    {
        return 0;
    }
    
    // Inicializa tiempo de ultima muestra procesada.
    Time lastTime = Seconds(0);

    // Verifica si existen tiempos de muestras para ese ueId.
    if (m_lastTxPduTime.count(ueId))
    {
        lastTime = m_lastTxPduTime[ueId];
    }

    uint64_t sum = 0;
    uint64_t count = 0;

    // Actualiza timpo de ultima muestra procesada.
    Time newestTime = lastTime;

    for (const auto& s : samples)
    {
        if (s.time > lastTime)
        {
            sum += s.txPdu;
            count++;

            if (s.time > newestTime)
            {
                newestTime = s.time;
            }
        }
    }

    if (count == 0) { 
      m_lastTxPduTime[ueId] = newestTime;
      m_lastTxPduValue[ueId] = 0;
      return 0; 
    }

    uint64_t avg = sum / count;

    // Solo actualiza tiempo y valor de si hubo muestras nuevas.
    m_lastTxPduTime[ueId] = newestTime;
    m_lastTxPduValue[ueId] = avg;

    return avg;
}

} // namespace ns3
