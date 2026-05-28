///////////////////////////////////////////////////////////////////////////////
///    Comando creado por estudiantes de Facultad de Ingeniería UdelaR      ///
///    tomando como base los comandos ya desarrollados en el módulo.        ///
///                                                                         ///
///    Este comando es generado por el Logic Module y utilizado por el      ///
///    scheduler del gNB para limitar o definir la cantidad de RBGs         ///
///    uplink asignados a cada UE. El mapa interno utiliza como clave       ///
///    el RNTI de la UE y como valor la cantidad de RBGs lógicos por slot.  ///
///////////////////////////////////////////////////////////////////////////////


#include "oran-command-nr-ul-rbg-targets.h"

#include "ns3/log.h"

#include <sstream>

namespace ns3
{

NS_LOG_COMPONENT_DEFINE("OranCommandNrUlRbgTargets");

NS_OBJECT_ENSURE_REGISTERED(OranCommandNrUlRbgTargets);

TypeId
OranCommandNrUlRbgTargets::GetTypeId()
{
    static TypeId tid = TypeId("ns3::OranCommandNrUlRbgTargets")
                            .SetParent<OranCommand>()
                            .AddConstructor<OranCommandNrUlRbgTargets>();
    return tid;
}

OranCommandNrUlRbgTargets::OranCommandNrUlRbgTargets()
    : OranCommand()
{
    NS_LOG_FUNCTION(this);
}

OranCommandNrUlRbgTargets::~OranCommandNrUlRbgTargets()
{
    NS_LOG_FUNCTION(this);
}

std::string
OranCommandNrUlRbgTargets::ToString() const
{
    std::ostringstream oss;
    oss << "OranCommandNrUlRbgTargets{targets=" << m_ulTargets.size() << "}";
    return oss.str();
}

void
OranCommandNrUlRbgTargets::SetUlTargets(const std::unordered_map<uint16_t, uint32_t>& targets)
{
    NS_LOG_FUNCTION(this << targets.size());
    
    // Almacena los objetivos de RBGs uplink por UE.
    // La clave del mapa es el RNTI de la UE y el valor es el target de RBGs.
    m_ulTargets = targets;
}

const std::unordered_map<uint16_t, uint32_t>&
OranCommandNrUlRbgTargets::GetUlTargets() const
{
    // Devuelve los target UL que serán interpretados por el scheduler.
    return m_ulTargets;
}

} // namespace ns3
