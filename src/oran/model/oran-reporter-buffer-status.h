///////////////////////////////////////////////////////////////////////////////
/// Reporter desarrollado en el marco del proyecto.                         ///
///                                                                         ///
/// Este componente captura reportes de estado de buffer generados por el   ///
/// UE y los transforma en objetos OranReportUeTxBuffer para enviarlos al   ///
/// Near-RT RIC.                                                            ///
///////////////////////////////////////////////////////////////////////////////

#ifndef ORAN_REPORTER_BUFFER_STATUS_H
#define ORAN_REPORTER_BUFFER_STATUS_H

#include "oran-reporter.h"
#include "ns3/nr-mac-sap.h"
#include "ns3/nstime.h"

#include <map>

namespace ns3 {

class OranReporterUeTxBuffer : public OranReporter
{
public:

    // Obtiene el TypeId asociado a la clase.
    static TypeId GetTypeId();

    // Constructor.
    OranReporterUeTxBuffer();

    // Destructor.
    ~OranReporterUeTxBuffer() override;

    // Agrega una muestra de estado de buffer obtenida desde la pila NR.
    void AddSample(NrMacSapProvider::BufferStatusReportParameters params);

    // Genera reportes ORAN a partir de las muestras almacenadas.    
    std::vector<Ptr<OranReport>> GenerateReports() override;

private:

    // Muestra de buffer almacenada por el reporter.
    struct Sample
    {
        Time time;                                                // Instante en que se capturó la muestra.
        NrMacSapProvider::BufferStatusReportParameters data;      // Datos del reporte de buffer.
    };

    // Última muestra de buffer almacenada por LCID.
    std::map<uint8_t, Sample> m_samples;
};

} // namespace ns3

#endif // ORAN_REPORTER_BUFFER_STATUS_H
