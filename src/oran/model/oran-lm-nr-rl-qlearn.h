/////////////////////////////////////////////////////////////////////////////
///    LM creado por estudiantes de Facultad de Ingeniería UdelaR         ///
///    tomando como base los LM ya desarrollados en el módulo.            ///
///                                                                       ///
///    Logic Module para control RL basado en MDP.                        ///
///                                                                       ///
///    Este LM obtiene métricas de los UE desde el repositorio de datos,  ///
///    construye una observación y la envía a un agente externo QLearn    ///
///    mediante gRPC. La respuesta del agente se transforma en comandos   ///
///    de asignación de RBGs uplink por UE.                               ///
/////////////////////////////////////////////////////////////////////////////

#ifndef ORAN_LM_NR_RL_QLEARN_H
#define ORAN_LM_NR_RL_QLEARN_H

#include "oran-lm.h"
#include <map> 
#include "ns3/nstime.h"
#include <unordered_map>
#include <iostream>
#include <memory>
#include <grpcpp/grpcpp.h>
#include "../../grpc/rl_data_4_qtable.grpc.pb.h"

namespace ns3
{

class OranNrDataRepository;

class OranLmNrRlQlearn : public OranLm
{
  public:
    static TypeId GetTypeId();

    OranLmNrRlQlearn();
    ~OranLmNrRlQlearn() override;

    std::vector<Ptr<OranCommand>> Run() override;

  private:
    // Último instante de ejecución del LM.
    Time m_lastLmTime{Seconds(0)};

    // Cliente gRPC utilizado por el LM para comunicarse con el agente RL externo.
    struct GrpcClient
    {
      std::shared_ptr<grpc::Channel> channel;
      std::unique_ptr<rl_data_4_qtable::RlData4QTable::Stub> stub;
    
      // Se crea el canal de comunicación y el stub del servicio definido en el proto.
      explicit GrpcClient(const std::string& addr)
      {
        channel = grpc::CreateChannel(addr, grpc::InsecureChannelCredentials());
        stub = rl_data_4_qtable::RlData4QTable::NewStub(channel);
      }
    };
    std::unique_ptr<GrpcClient> m_grpc;

    // Últimas proporciones de recursos asignadas por RNTI.
    std::map<uint32_t, double> m_lastShares;
    
    // Cada cuántas llamadas promedia muestras.
    uint32_t m_N = 1;
    
    // Contadores de llamadas por UE para cada métrica.
    std::unordered_map<uint64_t, uint32_t> m_bufferCounter;
    std::unordered_map<uint64_t, uint32_t> m_txDropCounter;
    std::unordered_map<uint64_t, uint32_t> m_txPduCounter;

    // Último valor calculado por UE para cada métrica.
    std::unordered_map<uint64_t, uint64_t> m_lastBufferValue;
    std::unordered_map<uint64_t, uint64_t> m_lastTxDropValue;
    std::unordered_map<uint64_t, uint64_t> m_lastTxPduValue;

    // Último tiempo procesado por UE para cada métrica.
    std::unordered_map<uint64_t, Time> m_lastBufferTime;
    std::unordered_map<uint64_t, Time> m_lastTxDropTime;
    std::unordered_map<uint64_t, Time> m_lastTxPduTime;

    // Obtiene el valor de buffer de transmisión para una UE.
    //     data: Repositorio de datos del Near-RT RIC.
    //     ueId: Identificador del nodo UE.
    //     lcid: Identificador del canal lógico.
    //     return: Valor de buffer calculado para la UE.
    uint64_t GetBufferSize(Ptr<OranNrDataRepository> data, uint64_t ueId, uint8_t lcid);

    // Obtiene la cantidad de paquetes descartados en transmisión para una UE.
    //     data: Repositorio de datos del Near-RT RIC.
    //     ueId: Identificador del nodo UE.
    //     lcid: Identificador del canal lógico.
    //     return: Valor de paquetes descartados calculado para la UE.
    uint64_t GetTxDrop(Ptr<OranNrDataRepository> data, uint64_t ueId, uint8_t lcid);

    // Obtiene la cantidad de PDU transmitidas para una UE.
    //     data: Repositorio de datos del Near-RT RIC.
    //     ueId: Identificador del nodo UE.
    //     lcid: Identificador del canal lógico.
    //     return: Valor de PDU transmitidas calculado para la UE.
    uint64_t GetTxPdu(Ptr<OranNrDataRepository> data, uint64_t ueId, uint8_t lcid);

};

} // namespace ns3

#endif /* ORAN_LM_NR_RL_QLEARN_H */
