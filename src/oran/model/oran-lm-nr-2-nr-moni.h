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

#ifndef ORAN_LM_NR_2_NR_MONI_H
#define ORAN_LM_NR_2_NR_MONI_H

#include "oran-nr-data-repository.h"
#include "oran-lm.h"
#include "ns3/nstime.h"

#include "ns3/vector.h"

#include <vector>

namespace ns3
{

class OranLmNr2NrMoni : public OranLm
{
  protected:
    // Información asociada a un UE.
    struct UeInfo
    {
        uint64_t nodeId; // Identificador del nodo UE.
        uint16_t cellId; // Identificador de celda.
        uint16_t rnti;   // RNTI del UE.
        Vector position; // Última posicion conocida.
    };

    // Información asociada a un gNb.
    struct GnbInfo
    {
        uint64_t nodeId; // Identificador del nodo gNb.
        uint16_t cellId; // Identificador de celda.
        Vector position; // Última posicion conocida.
    };

    // Medida de RSRP/RSRQ reportada por el UE.
    struct RsrpInfo
    {
        uint64_t nodeId;             // Identificador del nodo UE.
        uint16_t rnti;               // RNTI del UE..
        uint16_t cellId;             // Identificador de celda.
        double rsrp;                 // Valor RSRP en dBm.
        double rsrq;                 // Valor RSRQ en dB.
        bool isServingCell;          // True si la medida viene de la celda que sirve al UE.
        uint16_t componentCarrierId; // Identificador de componente portadora.
    };

    // Pérdida de paquetes a nivel de aplicación.
    struct AppLossInfo
    {
      uint64_t nodeId;      // ID del nodo UE.
      uint64_t txPackets;   // Cantidad de paquetes transmitidos.
      uint64_t rxPackets;   // Cantidad de paquetes recibidos.
      double lossRatio;     // Porcentaje de pérdida.
    };

    // Buffer status reportado por el UE.
    struct BufferStatusInfo
    {
        uint64_t nodeId;             // Identificador del nodo UE.
        uint16_t rnti;               // RNTI del UE.
        uint8_t lcid;                // LCID: Logical channel identifier.
        uint32_t txQueueSize;        // Tamaño en bytes de información en buffer del UE .
        uint16_t txQueueHolDelay;    // Transmission queue HOL delay en ms.
        uint32_t retxQueueSize;      // Retransmission queue size en bytes.
        uint16_t retxQueueHolDelay;  // Retransmission queue HOL delay en ms.
        uint16_t statusPduSize;      // Pending STATUS PDU en bytes.
        Time time;                   // Tiempo de captura de la muestra.
    };

    // Consulta el último valor disponible de CQI, MCS y RI para una UE.
    struct CqiInfo
    {
        uint64_t nodeId;
        Time time;
        uint16_t rnti;
        uint8_t cqi;
        uint8_t mcs;
        uint8_t ri;
    };

    // Consulta los paquetes transmitidos por el UE.
    struct TxPduInfo
	{
	  uint64_t nodeId;
	  Time time;
	  uint16_t rnti;
	  uint32_t lcid;     
	  uint64_t txPdu;
	};

    // Consulta los paquetes descartados en transmisión para un UE.
    struct TxDropInfo
	{
	  uint64_t nodeId;
	  Time time;
	  uint16_t rnti;
	  uint32_t lcid;     
	  uint64_t txDrop;
	};	

  std::vector<AppLossInfo> GetAppLossInfos(Ptr<OranNrDataRepository> data, uint64_t ueNodeId) const;

  public:
    static TypeId GetTypeId();

    OranLmNr2NrMoni();
    ~OranLmNr2NrMoni() override;

    std::vector<Ptr<OranCommand>> Run() override;

  private:
    std::vector<OranLmNr2NrMoni::UeInfo> GetUeInfos(Ptr<OranNrDataRepository> data) const;
    std::vector<OranLmNr2NrMoni::GnbInfo> GetGnbInfos(Ptr<OranNrDataRepository> data) const;
    std::vector<OranLmNr2NrMoni::RsrpInfo> GetRsrpInfos(Ptr<OranNrDataRepository> data, uint64_t ueNodeId) const;
    std::vector<OranLmNr2NrMoni::BufferStatusInfo> GetBufferInfos(Ptr<OranNrDataRepository> data, uint64_t ueNodeId) const;
    std::vector<CqiInfo> GetCqiInfos(Ptr<OranNrDataRepository> data, uint64_t ueNodeId) const;
    std::vector<TxPduInfo>  GetTxPduInfos (Ptr<OranNrDataRepository> data, uint64_t ueNodeId) const;
    std::vector<TxDropInfo> GetTxDropInfos(Ptr<OranNrDataRepository> data, uint64_t ueNodeId) const;
    std::vector<Ptr<OranCommand>> GetHandoverCommands(Ptr<OranNrDataRepository> data, std::vector<OranLmNr2NrMoni::UeInfo> ueInfos, std::vector<OranLmNr2NrMoni::GnbInfo> gnbInfos) const;
};

} // namespace ns3

#endif /* ORAN_LM_NR_2_NR_MONI_H */
