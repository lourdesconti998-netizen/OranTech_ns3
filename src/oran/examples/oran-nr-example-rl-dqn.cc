/////////////////////////////////////////////////////////////////////////////
///    Ejemplo creado por estudiantes de Facultad de Ingeniería UdelaR    ///
///    tomando como base los ejemplos ya desarrollados en el módulo.      ///
///                                                                       ///
///    Consta de un gNb y dos UE fijos con tráfico UDP.                   ///
/////////////////////////////////////////////////////////////////////////////

#include "ns3/oran-nr-monitoring-helper.h"
#include "ns3/applications-module.h"
#include "ns3/core-module.h"
#include "ns3/internet-module.h"
#include "ns3/ipv4-static-routing-helper.h"
#include "ns3/ipv4-static-routing.h"
#include "ns3/nr-module.h"
#include "ns3/mobility-module.h"
#include "ns3/network-module.h"
#include "ns3/oran-module.h"
#include "ns3/oran-lm-nr-rl-dqn.h"
#include "ns3/point-to-point-module.h"
#include "ns3/oran-e2-node-terminator-nr-ue.h"
#include "ns3/oran-e2-node-terminator-nr-gnb.h"
#include "ns3/oran-reporter-apploss.h"
#include "ns3/oran-reporter-buffer-status.h"
#include "ns3/oran-reporter-nr-ue-cqi.h"
#include "ns3/nr-mac-scheduler-ofdma-ul-target.h"
#include "ns3/nr-ue-rrc.h"
#include "ns3/nr-gnb-rrc.h"

#include <stdio.h>

using namespace ns3;

NS_LOG_COMPONENT_DEFINE("OranNrExampleRlDqn");

void
ReverseVelocity(NodeContainer nodes, Time interval)
{
    for (uint32_t idx = 0; idx < nodes.GetN(); idx++)
    {
        Ptr<ConstantVelocityMobilityModel> mob =
            nodes.Get(idx)->GetObject<ConstantVelocityMobilityModel>();
        if (!mob)
        {
            continue;
        }
        mob->SetVelocity(Vector(mob->GetVelocity().x * -1, 0, 0));
    }
    Simulator::Schedule(interval, &ReverseVelocity, nodes, interval);
}

void
QueryRcSink(std::string query, std::string args, int rc)
{
    std::cout << Simulator::Now().GetSeconds() << " Query "
              << ((rc == SQLITE_OK || rc == SQLITE_DONE) ? "OK" : "ERROR") << "(" << rc << "): \""
              << query << "\"";

    if (!args.empty())
    {
        std::cout << " (" << args << ")";
    }
    std::cout << std::endl;
}

int
main(int argc, char* argv[])
{
    // Parámetros del escenario
    uint16_t numberOfUes = 2;
    uint16_t numberOfGnbs = 1;
    Time simTime = Seconds(300);
    double distance = 50;
    Time interval = Seconds(15);
    double speed = 1.0;
    double centralFrequency = 3.5e9;
    double bandwidth = 100e6;

    std::string dbFileName = "oran-repository.db";
    bool oranVerbose = true;
    Time lmQueryInterval = Seconds(0.01);

    // Flags dpara activar reportes
    bool enLoc = true;
    bool enCell = true;
    bool enRsrp = true;
    bool enAppLoss = true;
    bool enCqi = true;
    bool enTxBuf = true;
    bool enTxPdu = true;
    bool enTxDrop = true;

    CommandLine cmd(__FILE__);
    cmd.AddValue("db", "SQLite DB file name for O-RAN repository", dbFileName);
    cmd.AddValue("oranVerbose", "Enable O-RAN verbose behavior", oranVerbose);
    cmd.AddValue("lmQi", "LM query interval (seconds)", lmQueryInterval);
    cmd.AddValue("enLoc", "Enable Location report", enLoc);
    cmd.AddValue("enCell", "Enable UE CellInfo report", enCell);
    cmd.AddValue("enRsrp", "Enable RSRP/RSRQ report", enRsrp);
    cmd.AddValue("enAppLoss", "Enable AppLoss report", enAppLoss);
    cmd.AddValue("enCqi", "Enable CQI report", enCqi);
    cmd.AddValue("enTxBuf", "Enable TxBuffer report", enTxBuf);
    cmd.AddValue("enTxPdu", "Enable Tx PDU report", enTxPdu);
    cmd.AddValue("enTxDrop", "Enable Tx Drop report", enTxDrop);
    cmd.Parse(argc, argv);

    Config::SetDefault("ns3::NrHelper::UseIdealRrc", BooleanValue(true));

    Ptr<NrHelper> nrHelper = CreateObject<NrHelper>();
    Ptr<NrPointToPointEpcHelper> epcHelper = CreateObject<NrPointToPointEpcHelper>();
    nrHelper->SetEpcHelper(epcHelper);
    nrHelper->SetSchedulerTypeId(ns3::NrMacSchedulerOfdmaUlTarget::GetTypeId()); // scheduler elegido para el ejemplo, si se quiere modificar el scheduler, cambiar esta línea.

    Ptr<Node> pgw = epcHelper->GetPgwNode();
    
    // // BWP (Bandwidth Part) para una portadora (CC Component Carrier) - Configuración de banda y canal NR
    CcBwpCreator ccBwpCreator;
    const uint8_t numCcPerBand = 1;
    CcBwpCreator::SimpleOperationBandConf bandConf(centralFrequency, bandwidth, numCcPerBand);
    OperationBandInfo band = ccBwpCreator.CreateOperationBandContiguousCc(bandConf);

    Ptr<NrChannelHelper> channelHelper = CreateObject<NrChannelHelper>();
    channelHelper->ConfigureFactories("UMa");
    channelHelper->AssignChannelsToBands({band});
    BandwidthPartInfoPtrVector allBwps = CcBwpCreator::GetAllBwps({band});

    // Crea un host remoto para generar trafico hacia los UEs.
    NodeContainer remoteHostContainer;
    remoteHostContainer.Create(1);
    Ptr<Node> remoteHost = remoteHostContainer.Get(0);

    NodeContainer ueNodes;
    NodeContainer gnbNodes;
    gnbNodes.Create(numberOfGnbs);
    ueNodes.Create(numberOfUes);

    ///////////////////////////////////////////////
    //                Mobilidad                 ///
    ///////////////////////////////////////////////
    Ptr<ListPositionAllocator> positionAlloc = CreateObject<ListPositionAllocator>();

    // gNBs fijo
    for (uint16_t i = 0; i < numberOfGnbs; i++)
    {
        positionAlloc->Add(Vector(distance * i, 0, 20));
    }

    // UEs fijos en la misma posición que el gNb
    for (uint16_t i = 0; i < numberOfUes; i++)
    {
        positionAlloc->Add(Vector(0, 0, 1.5));
    }


    MobilityHelper mobility;

    // gNBs quietos
    mobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");
    mobility.SetPositionAllocator(positionAlloc);
    mobility.Install(gnbNodes);

    // UEs quietos
    mobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");
    mobility.Install(ueNodes);

    Simulator::Schedule(interval, &ReverseVelocity, ueNodes, interval);

    // Instala dispositivos NR en gNB y UEs
    NetDeviceContainer gnbNrDevs = nrHelper->InstallGnbDevice(gnbNodes, allBwps);
    NetDeviceContainer ueNrDevs = nrHelper->InstallUeDevice(ueNodes, allBwps);

    // Instala el stack IP en los UEs y el host remoto
    InternetStackHelper internet;
    internet.Install(ueNodes);
    internet.Install(remoteHostContainer);
    Ipv4InterfaceContainer ueIpIfaces = epcHelper->AssignUeIpv4Address(NetDeviceContainer(ueNrDevs));

    // Crea la conxión a internet entre el PGW y el host remoto
    PointToPointHelper p2ph;
    p2ph.SetDeviceAttribute("DataRate", StringValue("1Gbps"));
    p2ph.SetChannelAttribute("Delay", StringValue("2ms"));
    NetDeviceContainer internetDevices = p2ph.Install(pgw, remoteHost);

    Ipv4AddressHelper ipv4h;
    ipv4h.SetBase("1.0.0.0", "255.0.0.0");
    Ipv4InterfaceContainer internetIpIfaces = ipv4h.Assign(internetDevices);

    // Configura una ruta estatica para que el host remoto pueda alcanzar la subred del UE
    Ipv4StaticRoutingHelper ipv4RoutingHelper;
    Ptr<Ipv4StaticRouting> remoteHostStaticRouting =
        ipv4RoutingHelper.GetStaticRouting(remoteHost->GetObject<Ipv4>());
    remoteHostStaticRouting->AddNetworkRouteTo(
        Ipv4Address("7.0.0.0"), Ipv4Mask("255.0.0.0"), internetIpIfaces.GetAddress(0), 1);

    // Default gateway para los UEs
    Ipv4StaticRoutingHelper ueRoutingHelper;
    for (uint32_t i = 0; i < ueNodes.GetN(); ++i)
    {
        Ptr<Ipv4StaticRouting> ueStaticRouting =
            ueRoutingHelper.GetStaticRouting(ueNodes.Get(i)->GetObject<Ipv4>());
        ueStaticRouting->SetDefaultRoute(epcHelper->GetUeDefaultGatewayAddress(), 1);
    }

    // Registra todos los UE al gNb
    for (uint16_t i = 0; i < numberOfUes; i++)
    {
        nrHelper->AttachToGnb(ueNrDevs.Get(i), gnbNrDevs.Get(0));
    }

    // Agrega la interfZ X2
    nrHelper->AddX2Interface(gnbNodes);

    // Configura el trafico UDP entre el host remoto y la UE.
    uint16_t dlPort = 1234;
    uint16_t ulPort = 2000;
    ApplicationContainer serverApps;
    ApplicationContainer clientApps;
    ApplicationContainer ulServerApps;
    ApplicationContainer ulClientApps;

    for (uint16_t i = 0; i < numberOfUes; ++i)
    {
        // Receptor de tráfico downlink en el UE
        PacketSinkHelper packetSinkHelper(
            "ns3::UdpSocketFactory",
            InetSocketAddress(ueIpIfaces.GetAddress(i), dlPort + i));
        serverApps.Add(packetSinkHelper.Install(ueNodes.Get(i)));

        // DL OnOff al host remoto
        OnOffHelper onOffHelper(
            "ns3::UdpSocketFactory",
            InetSocketAddress(ueIpIfaces.GetAddress(i), dlPort + i));
        onOffHelper.SetAttribute("DataRate", DataRateValue(DataRate("4kbps")));
        onOffHelper.SetAttribute("PacketSize", UintegerValue(10));
        onOffHelper.SetAttribute("OnTime", StringValue("ns3::ConstantRandomVariable[Constant=1]"));
        onOffHelper.SetAttribute("OffTime", StringValue("ns3::ConstantRandomVariable[Constant=0]"));
        clientApps.Add(onOffHelper.Install(remoteHost));

        // Receptor de tráfico downlink en el host remoto
        PacketSinkHelper ulPacketSinkHelper(
            "ns3::UdpSocketFactory",
            InetSocketAddress(internetIpIfaces.GetAddress(1), ulPort + i));
        ulServerApps.Add(ulPacketSinkHelper.Install(remoteHost));

        // UL OnOff al host remoto
        OnOffHelper ulOnOffHelper(
            "ns3::UdpSocketFactory",
            InetSocketAddress(internetIpIfaces.GetAddress(1), ulPort + i));
        ulOnOffHelper.SetAttribute("DataRate", DataRateValue(DataRate("80Mbps")));
        ulOnOffHelper.SetAttribute("PacketSize", UintegerValue(500));

        // Ciclo de 1 s
        ulOnOffHelper.SetAttribute("OnTime",StringValue("ns3::ConstantRandomVariable[Constant=0.50]"));
        ulOnOffHelper.SetAttribute("OffTime", StringValue("ns3::ConstantRandomVariable[Constant=0.50]"));
        // Las UE's quedan 20 milisegundos en estado ON y 10 milisegundo en estado off
        ApplicationContainer ulApp = ulOnOffHelper.Install(ueNodes.Get(i));

        // Defasaje entre UEs
        double ulOffset = 0.0;
        if (i == 0)
        {
            ulOffset = 0.0;   // UE1 arranca en t=0.00
        }
        else if (i == 1)
        {
            ulOffset = 0.25;   // UE2 arranca en t=0.25
        }
       
        ulApp.Start(Seconds(ulOffset));
        ulClientApps.Add(ulApp);

        // bearer
        NrEpsBearer bearer(NrEpsBearer::NGBR_LOW_LAT_EMBB);
        Ptr<NrEpcTft> tft = Create<NrEpcTft>();
        NrEpcTft::PacketFilter dlpf;
        dlpf.localPortStart = dlPort + i;
        dlpf.localPortEnd = dlPort + i;
        tft->Add(dlpf);
        nrHelper->ActivateDedicatedEpsBearer(ueNrDevs.Get(i), bearer, tft);
    }

    serverApps.Start(Seconds(1));
    serverApps.Stop(simTime);
    clientApps.Start(Seconds(2));
    clientApps.Stop(simTime - Seconds(1));
    ulServerApps.Start(Seconds(1));
    ulServerApps.Stop(simTime);
    // UL apps se arrancan/terminan por UE dentro del loop para respetar offsets.

    ///////////////////////////////////
    ///           O-RAN Core        ///
    ///////////////////////////////////
    if (!dbFileName.empty())
    {
        std::remove(dbFileName.c_str());
    }

    // Repositorio
    Ptr<OranNrDataRepository> dataRepository = CreateObject<OranNrDataRepositorySqlite>();
    dataRepository->SetAttribute("DatabaseFile", StringValue(dbFileName));

    // LM + CMM
    Ptr<OranLm> defaultLm = CreateObject<OranLmNrRlDqn>();
    Ptr<OranCmm> cmm = CreateObject<OranCmmNoop>();

    // Near-RT RIC + E2 Terminator del RIC
    Ptr<OranNrNearRtRic> nearRtRic = CreateObject<OranNrNearRtRic>();
    Ptr<OranNrNearRtRicE2Terminator> nearRtRicE2Terminator =
        CreateObject<OranNrNearRtRicE2Terminator>();

    defaultLm->SetAttribute("Verbose", BooleanValue(oranVerbose));
    defaultLm->SetAttribute("NearRtRic", PointerValue(nearRtRic));
    defaultLm->SetAttribute("ProcessingDelayRv", StringValue("ns3::ConstantRandomVariable[Constant=0]"));

    cmm->SetAttribute("NearRtRic", PointerValue(nearRtRic));
    cmm->SetAttribute("Verbose", BooleanValue(oranVerbose));

    nearRtRicE2Terminator->SetAttribute("NearRtRic", PointerValue(nearRtRic));
    nearRtRicE2Terminator->SetAttribute("DataRepository", PointerValue(dataRepository));
    nearRtRicE2Terminator->SetAttribute("TransmissionDelayRv", StringValue("ns3::ConstantRandomVariable[Constant=0.001]"));

    nearRtRic->SetAttribute("DefaultLogicModule", PointerValue(defaultLm));
    nearRtRic->SetAttribute("E2Terminator", PointerValue(nearRtRicE2Terminator));
    nearRtRic->SetAttribute("DataRepository", PointerValue(dataRepository));
    nearRtRic->SetAttribute("ConflictMitigationModule", PointerValue(cmm));
    
    nearRtRic->SetAttribute("LmQueryInterval", TimeValue(lmQueryInterval));
    nearRtRic->SetAttribute("E2NodeInactivityThreshold", TimeValue(Seconds(2)));
    nearRtRic->SetAttribute("E2NodeInactivityIntervalRv", StringValue("ns3::ConstantRandomVariable[Constant=2]"));
    nearRtRic->SetAttribute("LmQueryMaxWaitTime", TimeValue(Seconds(0)));
    nearRtRic->SetAttribute("LmQueryLateCommandPolicy", EnumValue(OranNrNearRtRic::DROP));

    // Arrancar el RIC
    Simulator::Schedule(Seconds(1), &OranNrNearRtRic::Start, nearRtRic);

    //////////////////////////////////////
    ///     O-RAN Monitoring Helper    ///
    //////////////////////////////////////
    Ptr<OranNrMonitoringHelper> moni = CreateObject<OranNrMonitoringHelper>();
    moni->SetAttribute("Verbose", BooleanValue(oranVerbose));
    moni->SetAttribute("EnableLocationReport", BooleanValue(enLoc));
    moni->SetAttribute("EnableCellInfoReport", BooleanValue(enCell));
    moni->SetAttribute("EnableRsrpRsrqReport", BooleanValue(enRsrp));
    moni->SetAttribute("EnableAppLossReport", BooleanValue(enAppLoss));
    moni->SetAttribute("EnableCqiReport", BooleanValue(enCqi));
    moni->SetAttribute("EnableTxBufferReport", BooleanValue(enTxBuf));
    moni->SetAttribute("EnableTxPduReport", BooleanValue(enTxPdu));
    moni->SetAttribute("EnableTxDropReport", BooleanValue(enTxDrop));

    // Se pasa el RIC que creó el ejemplo
    moni->SetNearRtRic(nearRtRic);

    // Instalar terminators+reporters
    moni->Install(ueNodes, gnbNodes, clientApps, serverApps);

    ////////////////////////////////////////
    ///                Run               ///
    ////////////////////////////////////////
    Simulator::Stop(simTime);
    Simulator::Run();
    Simulator::Destroy();
    return 0;
}
