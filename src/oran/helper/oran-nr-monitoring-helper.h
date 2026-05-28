#ifndef ORAN_NR_MONITORING_HELPER_H
#define ORAN_NR_MONITORING_HELPER_H

#include "ns3/application-container.h"
#include "ns3/node-container.h"
#include "ns3/nstime.h"
#include "ns3/object.h"
#include "ns3/ptr.h"
#include <vector>

#include <string>

namespace ns3
{

class OranNrNearRtRic;
class OranE2NodeTerminatorNrUe;
class OranE2NodeTerminatorNrGnb;
class OranLm; 


 // Helper para configurar e instalar la infraestructura de monitoreo O-RAN en escenarios NR

class OranNrMonitoringHelper : public Object
{
  public:
   
     //Obtiene el TypeId de la clase OranNrMonitoringHelper
     
    static TypeId GetTypeId();

    //Crea una instancia de la clase OranNrMonitoringHelper
     
    OranNrMonitoringHelper();

   
     //Destructor de la clase OranNrMonitoringHelper
   
    ~OranNrMonitoringHelper() override;

   
     // Establece el Near-RT RIC creado o administrado por el ejemplo (no por este helper)
     
    void SetNearRtRic(Ptr<OranNrNearRtRic> ric);

   
     //Instala la infraestructura de monitoreo O-RAN para los nodos UE y gNB indicados, así como para las aplicaciones asociadas.
     
      //ueNodes Nodos UE sobre los cuales se instalarán los terminators y reporters
     // gnbNodes Nodos gNB sobre los cuales se instalarán los terminators y reporters
     // clientApps Aplicaciones cliente (utilizadas para el monitoreo de AppLoss)
     // serverApps Aplicaciones servidor (utilizadas para el monitoreo de AppLoss)
     
    void Install(const NodeContainer& ueNodes,
                 const NodeContainer& gnbNodes,
                 const ApplicationContainer& clientApps,
                 const ApplicationContainer& serverApps);

   
     // Accede al Near-RT RIC asociado a este helper.
   
    Ptr<OranNrNearRtRic> GetNearRtRic() const;


  private:
    // Configuración (atributos)
    bool m_verbose;
    bool m_enableLocationReport;
    bool m_enableCellInfoReport;
    bool m_enableRsrpRsrqReport;
    bool m_enableAppLossReport;
    bool m_enableCqiReport;
    bool m_enableTxBufferReport;
    bool m_enableTxDropReport; 
    bool m_enableTxPduReport;
    
    // Mantiene vivos los terminators (creados por este helper como parte del monitoreo)
    std::vector<Ptr<OranE2NodeTerminatorNrUe>> m_ueTerms;
    std::vector<Ptr<OranE2NodeTerminatorNrGnb>> m_gnbTerms;
    
    // Objetos principales del sistema
    Ptr<OranNrNearRtRic> m_ric;
    Ptr<OranLm> m_moniLm;

};

} // namespace ns3

#endif // ORAN_NR_MONITORING_HELPER_H
