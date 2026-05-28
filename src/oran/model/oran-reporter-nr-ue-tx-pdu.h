///////////////////////////////////////////////////////////////////////////////////
/// Reporter desarrollado en el marco del proyecto.                             ///
///                                                                             ///
/// Este componente captura eventos de transmisión de PDU de un UE y acumula    ///
/// la cantidad de bytes transmitidos por LCID. Luego transforma  esas          ///
/// muestras en objetos OranReportNrUeTxPdu para enviarlos al Near-RT RIC.      ///
///////////////////////////////////////////////////////////////////////////////////


#ifndef ORAN_REPORTER_NR_UE_TX_PDU_H
#define ORAN_REPORTER_NR_UE_TX_PDU_H

#include "oran-reporter.h"

#include "ns3/ptr.h"
#include "ns3/nstime.h"

#include <map>
#include <vector>

namespace ns3
{

class Packet;

class OranReporterNrUeTxPdu : public OranReporter
{
  public:

    // Obtiene el TypeId asociado a la clase.
    static TypeId GetTypeId();

    // Constructor.
    OranReporterNrUeTxPdu();

    // Destructor.
    ~OranReporterNrUeTxPdu() override;

    // Agrega una muestra de PDU transmitida.
    void AddTxPdu(uint16_t rnti, uint8_t lcid, uint32_t bytes);

    // Genera reportes ORAN a partir de las PDU transmitidas acumuladas.
    std::vector<Ptr<OranReport>> GenerateReports() override;

  private:

    // Muestra acumulada de PDU transmitidas para un LCID.
    struct Sample
    {
        Time time;            // Instante de la última actualización de la muestra.
        uint16_t rnti;        // RNTI de la UE.
        uint8_t lcid;         // Identificador del canal lógico.
        uint64_t bytes;       // Bytes de PDU transmitidas acumulados.
    };

    // Bytes de PDU transmitidas acumulados por LCID.
    std::map<uint8_t, Sample> m_samples;
}; // class OranReporterNrUeTxPdu

} // namespace ns3

#endif // ORAN_REPORTER_NR_UE_TX_PDU_H
