#include "oran-nr-monitoring-helper.h"

#include "ns3/oran-lm-nr-2-nr-moni.h"
#include "ns3/nr-ue-mac.h"
#include "ns3/nr-ue-net-device.h"
#include "ns3/nr-ue-phy.h"
#include "ns3/nr-ue-rrc.h"
#include "ns3/oran-e2-node-terminator-nr-gnb.h"
#include "ns3/oran-e2-node-terminator-nr-ue.h"
#include "ns3/oran-nr-near-rt-ric.h"
#include "ns3/oran-reporter-apploss.h"
#include "ns3/oran-reporter-buffer-status.h"
#include "ns3/oran-reporter-location.h"
#include "ns3/oran-reporter-nr-ue-cell-info.h"
#include "ns3/oran-reporter-nr-ue-cqi.h"
#include "ns3/oran-reporter-nr-ue-rsrp-rsrq.h"
#include "ns3/oran-reporter-nr-ue-tx-pdu.h"
#include "ns3/oran-reporter-nr-ue-tx-drop.h"
#include "ns3/config.h"
#include "ns3/packet.h"
#include "ns3/simulator.h"
#include "ns3/nstime.h"
#include "ns3/pointer.h"
#include "ns3/string.h"
#include "ns3/boolean.h"
#include "ns3/log.h"
#include <cstdio>
#include <sstream>

namespace ns3
{

NS_LOG_COMPONENT_DEFINE("OranNrMonitoringHelper");

NS_OBJECT_ENSURE_REGISTERED(OranNrMonitoringHelper);


 // Callback intermedio para asociar a un evento TxDrop el contexto (RNTI y LCID)
 // que no siempre viene directamente en la traza.
 
 // Se utiliza al conectar dinámicamente las trazas RLC luego de la creación
 // del DRB correspondiente.
 
static void
ForwardTxDrop(Ptr<OranReporterNrUeTxDrop> rep,
              uint16_t rnti,
              uint8_t lcid,
              Ptr<const Packet> p)
{
    rep->AddTxDrop(p, rnti, lcid);
}


 // Conecta las trazas RLC de un bearer de datos con los reporters de TxPDU y TxDrop.
 
 // Esta función no se ejecuta al inicio de la simulación, sino cuando se crea
 // efectivamente un DRB (Data Radio Bearer). Esto es importante porque las
 // trazas RLC asociadas a un LCID específico no existen hasta ese momento.
 
 // Parámetros como imsi y cellId llegan desde la señal DrbCreated, aunque en
 //esta implementación no se utilizan directamente.
 
static void
ConnectUeRlcTraces(uint32_t nodeId,
                   uint32_t deviceIndex,
                   Ptr<OranReporterNrUeTxPdu> txPduReporter,
                   Ptr<OranReporterNrUeTxDrop> txDropReporter,
                   uint64_t imsi,
                   uint16_t cellId,
                   uint16_t rnti,
                   uint8_t lcid)
{
    // Si no hay ningún reporter RLC habilitado, no se realiza ninguna conexión.
    if (!txPduReporter && !txDropReporter)
    {
        return;
    }

    // Construcción del path de configuración hacia la instancia RLC asociada
    // al LCID creado dentro del UE.
    std::ostringstream basePath;
    basePath << "/NodeList/" << nodeId << "/DeviceList/" << deviceIndex
             << "/$ns3::NrUeNetDevice/NrUeRrc/DataRadioBearerMap/" << +lcid << "/NrRlc/";

    if (txPduReporter)
    {
        std::string txPduPath = basePath.str() + "TxPDU";

        // Conecta la traza TxPDU del RLC con el método AddTxPdu() del reporter.
        Config::ConnectWithoutContextFailSafe(
            txPduPath,
            MakeCallback(static_cast<void (OranReporterNrUeTxPdu::*)(uint16_t, uint8_t, uint32_t)>(
                             &OranReporterNrUeTxPdu::AddTxPdu),
                         txPduReporter));
    }

    if (txDropReporter)
    {
        std::string txDropPath = basePath.str() + "TxDrop";

        // Conecta la traza TxDrop del RLC y le "inyecta" el RNTI y LCID
        // mediante un callback enlazado.
        Config::ConnectWithoutContextFailSafe(
            txDropPath,
            MakeBoundCallback(&ForwardTxDrop, txDropReporter, rnti, lcid));
    }
}

TypeId
OranNrMonitoringHelper::GetTypeId()
{
    static TypeId tid =
        TypeId("ns3::OranNrMonitoringHelper")
            .SetParent<Object>()
            .AddConstructor<OranNrMonitoringHelper>()
            .AddAttribute("Verbose",
                          "Habilita comportamiento detallado del helper y de la LM de monitoreo.",
                          BooleanValue(false),
                          MakeBooleanAccessor(&OranNrMonitoringHelper::m_verbose),
                          MakeBooleanChecker())
            .AddAttribute("EnableLocationReport",
                          "Habilita reportes de localización.",
                          BooleanValue(true),
                          MakeBooleanAccessor(&OranNrMonitoringHelper::m_enableLocationReport),
                          MakeBooleanChecker())
            .AddAttribute("EnableCellInfoReport",
                          "Habilita reportes de información de celda del UE NR.",
                          BooleanValue(true),
                          MakeBooleanAccessor(&OranNrMonitoringHelper::m_enableCellInfoReport),
                          MakeBooleanChecker())
            .AddAttribute("EnableRsrpRsrqReport",
                          "Habilita reportes de RSRP y RSRQ.",
                          BooleanValue(true),
                          MakeBooleanAccessor(&OranNrMonitoringHelper::m_enableRsrpRsrqReport),
                          MakeBooleanChecker())
            .AddAttribute("EnableAppLossReport",
                          "Habilita reportes de pérdidas a nivel de aplicación.",
                          BooleanValue(true),
                          MakeBooleanAccessor(&OranNrMonitoringHelper::m_enableAppLossReport),
                          MakeBooleanChecker())
            .AddAttribute("EnableCqiReport",
                          "Habilita reportes de CQI.",
                          BooleanValue(true),
                          MakeBooleanAccessor(&OranNrMonitoringHelper::m_enableCqiReport),
                          MakeBooleanChecker())
            .AddAttribute("EnableTxBufferReport",
                          "Habilita reportes del estado del buffer de transmisión del UE.",
                          BooleanValue(true),
                          MakeBooleanAccessor(&OranNrMonitoringHelper::m_enableTxBufferReport),
                          MakeBooleanChecker())
            .AddAttribute("EnableTxPduReport",
                          "Habilita reportes de PDUs transmitidas en RLC.",
                          BooleanValue(true),
                          MakeBooleanAccessor(&OranNrMonitoringHelper::m_enableTxPduReport),
                          MakeBooleanChecker())
            .AddAttribute("EnableTxDropReport",
                          "Habilita reportes de datos descartados en RLC.",
                          BooleanValue(true),
                          MakeBooleanAccessor(&OranNrMonitoringHelper::m_enableTxDropReport),
                          MakeBooleanChecker());

    return tid;
}

OranNrMonitoringHelper::OranNrMonitoringHelper()
  : m_verbose(false),
    m_enableLocationReport(true),
    m_enableCellInfoReport(true),
    m_enableRsrpRsrqReport(true),
    m_enableAppLossReport(true),
    m_enableCqiReport(true),
    m_enableTxBufferReport(true),
    m_enableTxDropReport(true),
    m_enableTxPduReport(true),
    m_ric(nullptr),
    m_moniLm(nullptr)
{
    NS_LOG_FUNCTION(this);
}

OranNrMonitoringHelper::~OranNrMonitoringHelper()
{
    NS_LOG_FUNCTION(this);
}

void
OranNrMonitoringHelper::SetNearRtRic(Ptr<OranNrNearRtRic> ric)
{
    NS_LOG_FUNCTION(this << ric);
    m_ric = ric;
}


 // Instala la infraestructura completa de monitoreo O-RAN sobre los nodos
 // UE y gNB provistos por el escenario.
 
 // El flujo general es:
 // 1. Verificar que exista un Near-RT RIC.
 // 2. Crear y registrar la LM de monitoreo si aún no fue creada.
 // 3. Para cada UE:
 //    - crear terminator UE,
 //   - crear reporters habilitados,
 //   - conectarlos a sus trazas correspondientes,
 //   - adjuntar el terminator al nodo,
 //   - programar su activación.
 // 4. Para cada gNB:
 //   - crear terminator gNB,
 //   - instalar reporters habilitados (en esta versión, localización),
 //   - adjuntar el terminator,
 //   - programar su activación.
 
void
OranNrMonitoringHelper::Install(const NodeContainer& ueNodes,
                                const NodeContainer& gnbNodes,
                                const ApplicationContainer& clientApps,
                                const ApplicationContainer& serverApps)
{
    // El helper depende de un Near-RT RIC externo: no lo crea, sino que lo recibe.
    NS_ABORT_MSG_IF(m_ric == nullptr,
                    "OranNrMonitoringHelper: Near-RT RIC is null. "
                    "Call SetNearRtRic() in the example before Install().");

    // Crear la Logic Module de monitoreo solo una vez.
    // Esto evita registrar múltiples LM duplicadas si Install() se invoca más de una vez.
    if (m_moniLm == nullptr)
    {
        m_moniLm = CreateObject<OranLmNr2NrMoni>();

        // La LM necesita conocer qué RIC consultar para acceder al repositorio.
        m_moniLm->SetAttribute("NearRtRic", PointerValue(m_ric));

        // Configuración opcional: verbosidad y retardo interno de procesamiento.
        m_moniLm->SetAttribute("Verbose", BooleanValue(m_verbose));
        m_moniLm->SetAttribute("ProcessingDelayRv",
                               StringValue("ns3::ConstantRandomVariable[Constant=0]"));

        // Registrar la LM dentro del Near-RT RIC.
        m_ric->AddLogicModule(m_moniLm);
    }

    NS_LOG_FUNCTION(this << ueNodes.GetN() << gnbNodes.GetN());

    // Validación para AppLoss:
    // si faltan aplicaciones cliente/servidor para algunos UEs, el monitoreo
    // de AppLoss quedará incompleto, pero el resto del monitoreo puede seguir funcionando.
    if (m_enableAppLossReport &&
        (clientApps.GetN() < ueNodes.GetN() || serverApps.GetN() < ueNodes.GetN()))
    {
        NS_LOG_WARN("AppLoss reports enabled but client/server app counts are smaller than UE "
                    "nodes; some reports will not be connected.");
    }

   
    // Instalación del monitoreo en los UEs

    for (uint32_t idx = 0; idx < ueNodes.GetN(); ++idx)
    {
        // Cada UE recibe su propio terminator E2.
        Ptr<OranE2NodeTerminatorNrUe> nrUeTerminator = CreateObject<OranE2NodeTerminatorNrUe>();
        nrUeTerminator->SetAttribute("NearRtRic", PointerValue(m_ric));
        nrUeTerminator->SetAttribute("RegistrationIntervalRv",
                                     StringValue("ns3::ConstantRandomVariable[Constant=1]"));
        nrUeTerminator->SetAttribute("SendIntervalRv",
                                     StringValue("ns3::ConstantRandomVariable[Constant=0.001]"));
        nrUeTerminator->SetAttribute("TransmissionDelayRv",
                                     StringValue("ns3::ConstantRandomVariable[Constant=0.001]"));

        // Punteros a reporters potenciales. Se crearán solo si están habilitados.
        Ptr<OranReporterAppLoss> appLossReporter;
        Ptr<OranReporterNrUeCqi> cqiReporter;
        Ptr<OranReporterUeTxBuffer> txBufferReporter;
        Ptr<OranReporterNrUeRsrpRsrq> rsrpRsrqReporter;
        Ptr<OranReporterNrUeTxPdu> txPduReporter;
        Ptr<OranReporterNrUeTxDrop> txDropReporter;

        if (m_enableLocationReport)
        {
            Ptr<OranReporterLocation> locationReporter = CreateObject<OranReporterLocation>();
            locationReporter->SetAttribute("Terminator", PointerValue(nrUeTerminator));
            locationReporter->SetAttribute("Trigger",
                                           StringValue("ns3::OranReportTriggerPeriodic"));
            nrUeTerminator->AddReporter(locationReporter);
        }

        if (m_enableCellInfoReport)
        {
            Ptr<OranReporterNrUeCellInfo> nrUeCellInfoReporter =
                CreateObject<OranReporterNrUeCellInfo>();
            nrUeCellInfoReporter->SetAttribute("Terminator", PointerValue(nrUeTerminator));

            // Este reporter usa un trigger asociado a handover y además
            // fuerza un reporte inicial al comenzar.
            nrUeCellInfoReporter->SetAttribute(
                "Trigger",
                StringValue("ns3::OranReportTriggerNrUeHandover[InitialReport=true]"));
            nrUeTerminator->AddReporter(nrUeCellInfoReporter);
        }

        if (m_enableRsrpRsrqReport)
        {
            rsrpRsrqReporter = CreateObject<OranReporterNrUeRsrpRsrq>();
            rsrpRsrqReporter->SetAttribute("Terminator", PointerValue(nrUeTerminator));
            nrUeTerminator->AddReporter(rsrpRsrqReporter);
        }

        if (m_enableAppLossReport)
        {
            appLossReporter = CreateObject<OranReporterAppLoss>();
            appLossReporter->SetAttribute("Terminator", PointerValue(nrUeTerminator));
            nrUeTerminator->AddReporter(appLossReporter);

            // Para AppLoss se conectan eventos de capa de aplicación:
            // Tx en cliente y Rx en servidor.
            if (clientApps.GetN() > idx)
            {
                clientApps.Get(idx)->TraceConnectWithoutContext(
                    "Tx",
                    MakeCallback(&ns3::OranReporterAppLoss::AddTx, appLossReporter));
            }
            if (serverApps.GetN() > idx)
            {
                serverApps.Get(idx)->TraceConnectWithoutContext(
                    "Rx",
                    MakeCallback(&ns3::OranReporterAppLoss::AddRx, appLossReporter));
            }
        }

        if (m_enableCqiReport)
        {
            cqiReporter = CreateObject<OranReporterNrUeCqi>();
            cqiReporter->SetAttribute("Terminator", PointerValue(nrUeTerminator));
            cqiReporter->SetAttribute("Trigger",
                                      StringValue("ns3::OranReportTriggerPeriodic"));
            nrUeTerminator->AddReporter(cqiReporter);
        }

        if (m_enableTxBufferReport)
        {
            txBufferReporter = CreateObject<OranReporterUeTxBuffer>();
            txBufferReporter->SetAttribute("Terminator", PointerValue(nrUeTerminator));
            txBufferReporter->SetAttribute("Trigger",
                                           StringValue("ns3::OranReportTriggerPeriodic"));
            nrUeTerminator->AddReporter(txBufferReporter);
        }

        if (m_enableTxPduReport)
        {
            txPduReporter = CreateObject<OranReporterNrUeTxPdu>();
            txPduReporter->SetAttribute("Terminator", PointerValue(nrUeTerminator));
            txPduReporter->SetAttribute("Trigger",
                                        StringValue("ns3::OranReportTriggerPeriodic"));
            nrUeTerminator->AddReporter(txPduReporter);
        }

        if (m_enableTxDropReport)
        {
            txDropReporter = CreateObject<OranReporterNrUeTxDrop>();
            txDropReporter->SetAttribute("Terminator", PointerValue(nrUeTerminator));
            txDropReporter->SetAttribute("Trigger",
                                         StringValue("ns3::OranReportTriggerPeriodic"));
            nrUeTerminator->AddReporter(txDropReporter);
        }

        // Conexión de reporters con las trazas reales del stack NR del UE

        for (uint32_t netDevIdx = 0; netDevIdx < ueNodes.Get(idx)->GetNDevices(); ++netDevIdx)
        {
            Ptr<NrUeNetDevice> nrUeDevice =
                ueNodes.Get(idx)->GetDevice(netDevIdx)->GetObject<NrUeNetDevice>();
            if (!nrUeDevice)
            {
                continue;
            }

            // En esta implementación se trabaja sobre el BWP 0.
            uint8_t bwpId = 0;
            Ptr<NrUePhy> uePhy = nrUeDevice->GetPhy(bwpId);

            if (m_enableRsrpRsrqReport && rsrpRsrqReporter)
            {
                uePhy->TraceConnectWithoutContext(
                    "ReportUeMeasurements",
                    MakeCallback(&ns3::OranReporterNrUeRsrpRsrq::ReportRsrpRsrq,
                                 rsrpRsrqReporter));
            }

            if (m_enableCqiReport && cqiReporter)
            {
                uePhy->TraceConnectWithoutContext(
                    "CqiFeedbackTrace",
                    MakeCallback(&ns3::OranReporterNrUeCqi::ReportCqi, cqiReporter));
            }

            if (m_enableTxBufferReport && txBufferReporter)
            {
                Ptr<NrUeMac> ueMac = nrUeDevice->GetMac(bwpId);

                ueMac->TraceConnectWithoutContext(
                    "ReportBufferStatus",
                    MakeCallback(&OranReporterUeTxBuffer::AddSample, txBufferReporter));
            }

            // Las trazas RLC no se conectan directamente aquí, sino cuando se crea el DRB.
            if ((m_enableTxPduReport && txPduReporter) || (m_enableTxDropReport && txDropReporter))
            {
                Ptr<NrUeRrc> ueRrc = nrUeDevice->GetRrc();
                if (ueRrc)
                {
                    ueRrc->TraceConnectWithoutContext(
                        "DrbCreated",
                        MakeBoundCallback(&ConnectUeRlcTraces,
                                          ueNodes.Get(idx)->GetId(),
                                          netDevIdx,
                                          txPduReporter,
                                          txDropReporter));
                }
            }
        }

        // Asociar el terminator al nodo UE.
        nrUeTerminator->Attach(ueNodes.Get(idx));

        // Guardar el terminator para mantenerlo vivo durante toda la simulación.
        m_ueTerms.push_back(nrUeTerminator);

        // Activación diferida del terminator UE.
        // Se hace a los 2 s para permitir que el resto de la infraestructura
        // esté ya inicializada cuando el monitoreo comience.
        Simulator::Schedule(Seconds(2),
                            &OranE2NodeTerminatorNrUe::Activate,
                            nrUeTerminator);
    }


    // Instalación del monitoreo en los gNBs

    for (uint32_t idx = 0; idx < gnbNodes.GetN(); ++idx)
    {
        Ptr<OranE2NodeTerminatorNrGnb> nrGnbTerminator =
            CreateObject<OranE2NodeTerminatorNrGnb>();

        nrGnbTerminator->SetAttribute("NearRtRic", PointerValue(m_ric));
        nrGnbTerminator->SetAttribute("RegistrationIntervalRv",
                                      StringValue("ns3::ConstantRandomVariable[Constant=1]"));
        nrGnbTerminator->SetAttribute("SendIntervalRv",
                                      StringValue("ns3::ConstantRandomVariable[Constant=0.001]"));
        nrGnbTerminator->SetAttribute("TransmissionDelayRv",
                                      StringValue("ns3::ConstantRandomVariable[Constant=0.001]"));

        // En esta versión del helper, el monitoreo sobre gNB se limita
        // al reporter de localización.
        if (m_enableLocationReport)
        {
            Ptr<OranReporterLocation> locationReporter = CreateObject<OranReporterLocation>();
            locationReporter->SetAttribute("Terminator", PointerValue(nrGnbTerminator));
            locationReporter->SetAttribute("Trigger",
                                           StringValue("ns3::OranReportTriggerPeriodic"));
            nrGnbTerminator->AddReporter(locationReporter);
        }

        nrGnbTerminator->Attach(gnbNodes.Get(idx));
        m_gnbTerms.push_back(nrGnbTerminator);

        // El terminator gNB se activa antes que los de UE.
        // Esto ayuda a que la infraestructura del lado de la red ya esté lista
        // cuando empiecen a activarse los UEs.
        Simulator::Schedule(Seconds(1.5),
                            &OranE2NodeTerminatorNrGnb::Activate,
                            nrGnbTerminator);
    }
}

Ptr<OranNrNearRtRic>
OranNrMonitoringHelper::GetNearRtRic() const
{
    return m_ric;
}

} // namespace ns3
