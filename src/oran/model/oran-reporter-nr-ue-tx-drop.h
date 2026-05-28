///////////////////////////////////////////////////////////////////////////////
/// Reporter desarrollado en el marco del proyecto.                         ///
///                                                                         ///
/// Este componente captura eventos de descarte en transmisión de un UE     ///
/// y acumula la cantidad de bytes descartados por par RNTI-LCID. Luego     ///
/// transforma esas muestras en objetos OranReportNrUeTxDrop para enviarlos ///
/// al Near-RT RIC.                                                         ///
///////////////////////////////////////////////////////////////////////////////


#ifndef ORAN_REPORTER_NR_UE_TX_DROP_H
#define ORAN_REPORTER_NR_UE_TX_DROP_H

#include "oran-reporter.h"

#include "ns3/nstime.h"
#include "ns3/ptr.h"

#include <map>
#include <vector>

namespace ns3
{

class Packet;

class OranReporterNrUeTxDrop : public OranReporter
{
  public:

    // Obtiene el TypeId asociado a la clase.
    static TypeId GetTypeId();

    // Constructor.
    OranReporterNrUeTxDrop();

    // Destructor.
    ~OranReporterNrUeTxDrop() override;

    // Agrega un evento de descarte cuando la traza no entrega RNTI y LCID.
    void AddTxDrop(Ptr<const Packet> p);

    // Agrega un evento de descarte con identificadores explícitos.
    void AddTxDrop(Ptr<const Packet> p, uint16_t rnti, uint8_t lcid);

    // Genera reportes ORAN a partir de los descartes acumulados.
    std::vector<Ptr<OranReport>> GenerateReports() override;

  private:

    // Muestra acumulada de descartes para un par RNTI-LCID.
    struct Sample
    {
        Time time;            // Instante de la última actualización de la muestra.
        uint16_t rnti{0};     // RNTI de la UE.
        uint8_t lcid{0};      // Identificador del canal lógico.
        uint64_t bytes{0};    // Bytes descartados acumulados.
    };

    // Bytes descartados acumulados por par RNTI-LCID.
    std::map<std::pair<uint16_t, uint8_t>, Sample> m_samples;
    
    // RNTI utilizado cuando la traza no entrega identificador explícito.
    uint16_t m_defaultRnti{0};

    // LCID utilizado cuando la traza no entrega identificador explícito.
    uint8_t m_defaultLcid{0};
}; // class OranReporterNrUeTxDrop

} // namespace ns3

#endif // ORAN_REPORTER_NR_UE_TX_DROP_H
