///////////////////////////////////////////////////////////////////////////////
///    Comando creado por estudiantes de Facultad de Ingeniería UdelaR      ///
///    tomando como base los comandos ya desarrollados en el módulo.        ///
///                                                                         ///
///    Este comando es generado por el Logic Module y utilizado por el      ///
///    scheduler del gNB para limitar o definir la cantidad de RBGs         ///
///    uplink asignados a cada UE. El mapa interno utiliza como clave       ///
///    el RNTI de la UE y como valor la cantidad de RBGs lógicos por slot.  ///
///////////////////////////////////////////////////////////////////////////////


#ifndef ORAN_COMMAND_NR_UL_RBG_TARGETS_H
#define ORAN_COMMAND_NR_UL_RBG_TARGETS_H

#include "oran-command.h"

#include <unordered_map>

namespace ns3
{

class OranCommandNrUlRbgTargets : public OranCommand
{
  public:
    
    // Obtiene el TypeId asociado a la clase.
    static TypeId GetTypeId();

    // Constructor.
    OranCommandNrUlRbgTargets();

    // Destructor.
    ~OranCommandNrUlRbgTargets() override;

    // Devuelve una representación en texto del comando.
    std::string ToString() const override;

    // Define los RBGs uplink por UE.
    void SetUlTargets(const std::unordered_map<uint16_t, uint32_t>& targets);

    // Obtiene los RBGs uplink por UE.
    const std::unordered_map<uint16_t, uint32_t>& GetUlTargets() const;

  private:

    // Mapa RNTI -> cantidad de RBGs lógicos uplink por slot.
    std::unordered_map<uint16_t, uint32_t> m_ulTargets;
}; // class OranCommandNrUlRbgTargets

} // namespace ns3

#endif /* ORAN_COMMAND_NR_UL_RBG_TARGETS_H */
