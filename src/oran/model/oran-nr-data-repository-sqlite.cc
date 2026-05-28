/**
 * NIST-developed software is provided by NIST as a public service. You may
 * use, copy and distribute copies of the software in any medium, provided that
 * you keep intact this entire notice. You may improve, modify and create
 * derivative works of the software or any portion of the software, and you may
 * copy and distribute such modifications or works. Modified works should carry
 * a notice stating that you changed the software and should note the date and
 * nature of any such change. Please explicitly acknowledge the National
 * Institute of Standards and Technology as the source of the software.
 *
 * NIST-developed software is expressly provided "AS IS." NIST MAKES NO
 * WARRANTY OF ANY KIND, EXPRESS, IMPLIED, IN FACT OR ARISING BY OPERATION OF
 * LAW, INCLUDING, WITHOUT LIMITATION, THE IMPLIED WARRANTY OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE, NON-INFRINGEMENT AND DATA ACCURACY. NIST
 * NEITHER REPRESENTS NOR WARRANTS THAT THE OPERATION OF THE SOFTWARE WILL BE
 * UNINTERRUPTED OR ERROR-FREE, OR THAT ANY DEFECTS WILL BE CORRECTED. NIST
 * DOES NOT WARRANT OR MAKE ANY REPRESENTATIONS REGARDING THE USE OF THE
 * SOFTWARE OR THE RESULTS THEREOF, INCLUDING BUT NOT LIMITED TO THE
 * CORRECTNESS, ACCURACY, RELIABILITY, OR USEFULNESS OF THE SOFTWARE.
 *
 * You are solely responsible for determining the appropriateness of using and
 * distributing the software and you assume all risks associated with its use,
 * including but not limited to the risks and costs of program errors,
 * compliance with applicable laws, damage to or loss of data, programs or
 * equipment, and the unavailability or interruption of operation. This
 * software is not intended to be used in any situation where a failure could
 * cause risk of injury or damage to property. The software developed by NIST
 * employees is not subject to copyright protection within the United States.
 */

#include "oran-nr-data-repository-sqlite.h"

#include "ns3/abort.h"
#include "ns3/log.h"
#include "ns3/simulator.h"
#include "ns3/string.h"

namespace ns3
{

NS_LOG_COMPONENT_DEFINE("OranNrDataRepositorySqlite");

NS_OBJECT_ENSURE_REGISTERED(OranNrDataRepositorySqlite);

TypeId
OranNrDataRepositorySqlite::GetTypeId()
{
    static TypeId tid =
        TypeId("ns3::OranNrDataRepositorySqlite")
            .SetParent<OranNrDataRepository>()
            .AddConstructor<OranNrDataRepositorySqlite>()
            .AddAttribute("DatabaseFile",
                          "The database file path.",
                          StringValue("oran-repository.db"),
                          MakeStringAccessor(&OranNrDataRepositorySqlite::m_dbPath),
                          MakeStringChecker())
            .AddTraceSource("QueryRc",
                            "Return code for SQL queries",
                            MakeTraceSourceAccessor(&OranNrDataRepositorySqlite::m_queryRc),
                            "ns3::OranNrDataRepositorySqlite::QueryTracedCallback")

        ;

    return tid;
}

OranNrDataRepositorySqlite::OranNrDataRepositorySqlite()
    : OranNrDataRepository(),
      m_db(nullptr)
{
    NS_LOG_FUNCTION(this);

    InitStatements();
}

OranNrDataRepositorySqlite::~OranNrDataRepositorySqlite()
{
    NS_LOG_FUNCTION(this);
}

void
OranNrDataRepositorySqlite::Activate()
{
    NS_LOG_FUNCTION(this);

    OranNrDataRepository::Activate();

    if (!IsDbOpen())
    {
        OpenDb();
    }
}

void
OranNrDataRepositorySqlite::Deactivate()
{
    NS_LOG_FUNCTION(this);

    if (IsDbOpen())
    {
        CloseDb();
    }

    OranNrDataRepository::Deactivate();
}

bool
OranNrDataRepositorySqlite::IsNodeRegistered(uint64_t e2NodeId)
{
    NS_LOG_FUNCTION(this);

    bool registered = false;
    if (m_active)
    {
        int rc;
        sqlite3_stmt* stmt = nullptr;

        sqlite3_prepare_v2(m_db, m_queryStmtsStrings[CHECK_NODE_REGISTERED].c_str(), -1, &stmt, 0);
        sqlite3_bind_int64(stmt, 1, e2NodeId);

        while ((rc = sqlite3_step(stmt)) == SQLITE_ROW)
        {
            registered = sqlite3_column_int(stmt, 0);
        }

        CheckQueryReturnCode(stmt, rc, FormatBoundArgsList(e2NodeId));
        sqlite3_finalize(stmt);
    }
    return registered;
}

uint64_t
OranNrDataRepositorySqlite::RegisterNode(OranNrNearRtRic::NodeType type, uint64_t id)
{
    NS_LOG_FUNCTION(this);

    uint64_t e2NodeId = 0;

    if (m_active)
    {
        int rc;
        sqlite3_stmt* stmt = nullptr;

        if (id == 0)
        {
            // Insert or update the node information
            sqlite3_prepare_v2(m_db, m_queryStmtsStrings[INSERT_NODE_ADD].c_str(), -1, &stmt, 0);

            sqlite3_bind_int(stmt, 1, type);

            rc = sqlite3_step(stmt);

            CheckQueryReturnCode(stmt, rc, FormatBoundArgsList(type));

            e2NodeId = sqlite3_last_insert_rowid(m_db);
        }
        else
        {
            sqlite3_prepare_v2(m_db, m_queryStmtsStrings[INSERT_NODE_UPDATE].c_str(), -1, &stmt, 0);

            sqlite3_bind_int(stmt, 1, id);
            sqlite3_bind_int(stmt, 2, type);

            rc = sqlite3_step(stmt);

            CheckQueryReturnCode(stmt, rc, FormatBoundArgsList(type));

            e2NodeId = sqlite3_last_insert_rowid(m_db);
        }

        sqlite3_finalize(stmt);

        // Insert the registration information
        sqlite3_prepare_v2(m_db,
                           m_queryStmtsStrings[INSERT_NODE_REGISTRATION].c_str(),
                           -1,
                           &stmt,
                           0);

        sqlite3_bind_int64(stmt, 1, e2NodeId);
        sqlite3_bind_int(stmt, 2, 1);
        sqlite3_bind_int64(stmt, 3, Simulator::Now().GetTimeStep());

        rc = sqlite3_step(stmt);

        CheckQueryReturnCode(stmt,
                             rc,
                             FormatBoundArgsList(e2NodeId, true, Simulator::Now().GetTimeStep()));

        sqlite3_finalize(stmt);
    }

    return e2NodeId;
}

uint64_t
OranNrDataRepositorySqlite::RegisterNodeNrUe(uint64_t id, uint64_t imsi)
{
    NS_LOG_FUNCTION(this);
    uint64_t e2NodeId = 0;

    if (m_active)
    {
        int rc;
        sqlite3_stmt* stmt = nullptr;
        e2NodeId = RegisterNode(OranNrNearRtRic::NodeType::NRUE, id);

        sqlite3_prepare_v2(m_db, m_queryStmtsStrings[INSERT_NR_UE_NODE].c_str(), -1, &stmt, 0);

        sqlite3_bind_int64(stmt, 1, id);
        sqlite3_bind_int64(stmt, 2, imsi);

        rc = sqlite3_step(stmt);
        CheckQueryReturnCode(stmt, rc, FormatBoundArgsList(id, imsi));
        sqlite3_finalize(stmt);
    }
    return e2NodeId;
}

uint64_t
OranNrDataRepositorySqlite::RegisterNodeNrGnb(uint64_t id, uint16_t cellId)
{
    NS_LOG_FUNCTION(this << id << cellId);

    uint64_t e2NodeId = 0;

    if (m_active)
    {
        int rc;
        sqlite3_stmt* stmt = nullptr;
        e2NodeId = RegisterNode(OranNrNearRtRic::NodeType::NRGNB, id);

        sqlite3_prepare_v2(m_db, m_queryStmtsStrings[INSERT_NR_GNB_NODE].c_str(), -1, &stmt, 0);

        sqlite3_bind_int64(stmt, 1, id);
        sqlite3_bind_int(stmt, 2, cellId);

        rc = sqlite3_step(stmt);
        CheckQueryReturnCode(stmt, rc, FormatBoundArgsList(id, cellId));
        sqlite3_finalize(stmt);
    }
    return e2NodeId;
}

uint64_t
OranNrDataRepositorySqlite::DeregisterNode(uint64_t e2NodeId)
{
    NS_LOG_FUNCTION(this << e2NodeId);

    uint64_t retVal = 0;
    if (m_active)
    {
        int rc;
        sqlite3_stmt* stmt = nullptr;

        retVal = e2NodeId;

        sqlite3_prepare_v2(m_db,
                           m_queryStmtsStrings[INSERT_NODE_REGISTRATION].c_str(),
                           -1,
                           &stmt,
                           0);

        sqlite3_bind_int64(stmt, 1, e2NodeId);
        sqlite3_bind_int(stmt, 2, false);
        sqlite3_bind_int64(stmt, 3, Simulator::Now().GetTimeStep());

        rc = sqlite3_step(stmt);
        CheckQueryReturnCode(stmt,
                             rc,
                             FormatBoundArgsList(e2NodeId, false, Simulator::Now().GetTimeStep()));
        sqlite3_finalize(stmt);
    }
    return retVal;
}

void
OranNrDataRepositorySqlite::SavePosition(uint64_t e2NodeId, Vector pos, Time t)
{
    NS_LOG_FUNCTION(this << e2NodeId << pos << t);

    if (m_active)
    {
        if (IsNodeRegistered(e2NodeId))
        {
            int rc;
            sqlite3_stmt* stmt = nullptr;

            sqlite3_prepare_v2(m_db,
                               m_queryStmtsStrings[INSERT_NODE_LOCATION].c_str(),
                               -1,
                               &stmt,
                               0);

            sqlite3_bind_int64(stmt, 1, e2NodeId);
            sqlite3_bind_double(stmt, 2, pos.x);
            sqlite3_bind_double(stmt, 3, pos.y);
            sqlite3_bind_double(stmt, 4, pos.z);
            sqlite3_bind_int64(stmt, 5, t.GetTimeStep());

            rc = sqlite3_step(stmt);
            CheckQueryReturnCode(
                stmt,
                rc,
                FormatBoundArgsList(e2NodeId, pos.x, pos.y, pos.z, t.GetTimeStep()));
            sqlite3_finalize(stmt);
        }
    }
}

void
OranNrDataRepositorySqlite::SaveNrUeCellInfo(uint64_t e2NodeId,
                                            uint16_t cellId,
                                            uint16_t rnti,
                                            Time t)
{
    NS_LOG_FUNCTION(this << e2NodeId << (uint32_t)cellId << (uint32_t)rnti << t);

    if (m_active)
    {
        if (IsNodeRegistered(e2NodeId))
        {
            int rc;
            sqlite3_stmt* stmt = nullptr;

            sqlite3_prepare_v2(m_db, m_queryStmtsStrings[INSERT_NR_UE_CELL].c_str(), -1, &stmt, 0);

            sqlite3_bind_int64(stmt, 1, e2NodeId);
            sqlite3_bind_int(stmt, 2, cellId);
            sqlite3_bind_int(stmt, 3, rnti);
            sqlite3_bind_int64(stmt, 4, t.GetTimeStep());

            rc = sqlite3_step(stmt);
            CheckQueryReturnCode(stmt,
                                 rc,
                                 FormatBoundArgsList(e2NodeId, cellId, rnti, t.GetTimeStep()));
            sqlite3_finalize(stmt);
        }
    }
}

// Modificación del proyecto:
// Función agregada para almacenar reportes de estado de buffer de transmisión de los UE. 
void
OranNrDataRepositorySqlite::SaveNrUeBufferStatus(uint64_t e2NodeId,
                                                const NrUeBufferStatus& status)
{
    NS_LOG_FUNCTION(this << e2NodeId << status.time << +status.lcid);

    if (m_active)
    {
        if (IsNodeRegistered(e2NodeId))
        {
            // Además de guardar la muestra en SQLite, se conserva una copia en memoria
            // para permitir el acceso a muestras recientes durante la simulación.
            m_bufferStatus[e2NodeId].push_back(status);

            // Se limita la cantidad de muestras almacenadas en memoria para evitar
            // crecimiento indefinido durante simulaciones largas.
            if (m_bufferStatus[e2NodeId].size() > 1000)
            {
                m_bufferStatus[e2NodeId].erase(m_bufferStatus[e2NodeId].begin());
            }
            
            int rc;
            sqlite3_stmt* stmt = nullptr;

            sqlite3_prepare_v2(m_db,
                               m_queryStmtsStrings[INSERT_NR_UE_TX_BUFFER_STATUS].c_str(),
                               -1,
                               &stmt,
                               0);

            sqlite3_bind_int64(stmt, 1, e2NodeId);
            sqlite3_bind_int64(stmt, 2, status.time.GetTimeStep());
            sqlite3_bind_int(stmt, 3, status.rnti);
            sqlite3_bind_int(stmt, 4, status.lcid);
            sqlite3_bind_int64(stmt, 5, status.txQueueSize);
            sqlite3_bind_int(stmt, 6, status.txQueueHolDelay);
            sqlite3_bind_int64(stmt, 7, status.retxQueueSize);
            sqlite3_bind_int(stmt, 8, status.retxQueueHolDelay);
            sqlite3_bind_int(stmt, 9, status.statusPduSize);

            rc = sqlite3_step(stmt);

            CheckQueryReturnCode(stmt,
                                 rc,
                                 FormatBoundArgsList(e2NodeId,
                                                     status.time.GetTimeStep(),
                                                     status.rnti,
                                                     +status.lcid,
                                                     status.txQueueSize,
                                                     status.txQueueHolDelay,
                                                     status.retxQueueSize,
                                                     status.retxQueueHolDelay,
                                                     status.statusPduSize));
            sqlite3_finalize(stmt);
        }
    }
}

// Modificación del proyecto:
// Función agregada para almacenar reportes de CQI, MCS y RI de los UE.
// Estas métricas permiten monitorear la calidad del canal radio.
void
OranNrDataRepositorySqlite::SaveNrUeCqi(uint64_t e2NodeId, const NrUeCqi& cqi)
{
    NS_LOG_FUNCTION(this << e2NodeId << cqi.time << +cqi.cqi);

    if (m_active)
    {
        if (IsNodeRegistered(e2NodeId))
        {
            // Se conserva una copia en memoria para consultar muestras recientes
            // sin depender únicamente de la base SQLite.
            m_cqiSamples[e2NodeId].push_back(cqi);
            if (m_cqiSamples[e2NodeId].size() > 1000)
            {
                m_cqiSamples[e2NodeId].erase(m_cqiSamples[e2NodeId].begin());
            }

            int rc;
            sqlite3_stmt* stmt = nullptr;

            sqlite3_prepare_v2(m_db,
                               m_queryStmtsStrings[INSERT_NR_UE_CQI].c_str(),
                               -1,
                               &stmt,
                               0);

            sqlite3_bind_int64(stmt, 1, e2NodeId);
            sqlite3_bind_int64(stmt, 2, cqi.time.GetTimeStep());
            sqlite3_bind_int(stmt, 3, cqi.rnti);
            sqlite3_bind_int(stmt, 4, cqi.cqi);
            sqlite3_bind_int(stmt, 5, cqi.mcs);
            sqlite3_bind_int(stmt, 6, cqi.ri);

            rc = sqlite3_step(stmt);
            CheckQueryReturnCode(stmt,
                                 rc,
                                 FormatBoundArgsList(e2NodeId,
                                                     cqi.time.GetTimeStep(),
                                                     cqi.rnti,
                                                     +cqi.cqi,
                                                     +cqi.mcs,
                                                     +cqi.ri));
            sqlite3_finalize(stmt);
        }
    }
}

// Modificación del proyecto:
// Función agregada para almacenar reportes de PDU transmitidas por los UE.
void
OranNrDataRepositorySqlite::SaveNrUeTxPdu(uint64_t e2NodeId, const NrUeTxPdu& pdu)
{
    NS_LOG_FUNCTION(this << e2NodeId << pdu.time << +pdu.lcid << pdu.txPdu);

    if (m_active)
    {
        if (IsNodeRegistered(e2NodeId))
        {
            // Se conserva una copia en memoria para disponer de muestras recientes
            // durante la ejecución de la simulación.
            m_txPduSamples[e2NodeId].push_back(pdu);
            if (m_txPduSamples[e2NodeId].size() > 1000)
            {
                m_txPduSamples[e2NodeId].erase(m_txPduSamples[e2NodeId].begin());
            }

            int rc;
            sqlite3_stmt* stmt = nullptr;

            sqlite3_prepare_v2(m_db,
                               m_queryStmtsStrings[INSERT_NR_UE_TX_PDU].c_str(),
                               -1,
                               &stmt,
                               0);

            sqlite3_bind_int64(stmt, 1, e2NodeId);
            sqlite3_bind_int64(stmt, 2, pdu.time.GetTimeStep());
            sqlite3_bind_int(stmt, 3, pdu.rnti);
            sqlite3_bind_int(stmt, 4, pdu.lcid);
            sqlite3_bind_int64(stmt, 5, pdu.txPdu);

            rc = sqlite3_step(stmt);
            CheckQueryReturnCode(stmt,
                                 rc,
                                 FormatBoundArgsList(e2NodeId,
                                                     pdu.time.GetTimeStep(),
                                                     pdu.rnti,
                                                     +pdu.lcid,
                                                     pdu.txPdu));
            sqlite3_finalize(stmt);
        }
    }
}

// Modificación del proyecto:
// Función agregada para almacenar reportes de paquetes descartados por los UE en transmisión.
void
OranNrDataRepositorySqlite::SaveNrUeTxDrop(uint64_t e2NodeId, const NrUeTxDrop& drop)
{
    NS_LOG_FUNCTION(this << e2NodeId << drop.time << +drop.lcid << drop.txDrop);

    if (m_active)
    {
        if (IsNodeRegistered(e2NodeId))
        {
            // Se conserva una copia en memoria para disponer de muestras recientes
            // durante la ejecución de la simulación.
            m_txDropSamples[e2NodeId].push_back(drop);
            if (m_txDropSamples[e2NodeId].size() > 1000)
            {
                m_txDropSamples[e2NodeId].erase(m_txDropSamples[e2NodeId].begin());
            }

            int rc;
            sqlite3_stmt* stmt = nullptr;

            sqlite3_prepare_v2(m_db,
                               m_queryStmtsStrings[INSERT_NR_UE_TX_DROP].c_str(),
                               -1,
                               &stmt,
                               0);

            sqlite3_bind_int64(stmt, 1, e2NodeId);
            sqlite3_bind_int64(stmt, 2, drop.time.GetTimeStep());
            sqlite3_bind_int(stmt, 3, drop.rnti);
            sqlite3_bind_int(stmt, 4, drop.lcid);
            sqlite3_bind_int64(stmt, 5, drop.txDrop);

            rc = sqlite3_step(stmt);
            CheckQueryReturnCode(stmt,
                                 rc,
                                 FormatBoundArgsList(e2NodeId,
                                                     drop.time.GetTimeStep(),
                                                     drop.rnti,
                                                     +drop.lcid,
                                                     drop.txDrop));
            sqlite3_finalize(stmt);
        }
    }
}


void
OranNrDataRepositorySqlite::SaveAppLoss(uint64_t e2NodeId, double appLoss, Time t)
{
    NS_LOG_FUNCTION(this << e2NodeId << appLoss << t);

    if (m_active)
    {
        if (IsNodeRegistered(e2NodeId))
        {
            int rc;
            std::string query;
            sqlite3_stmt* stmt = nullptr;

            query = "INSERT INTO nodeapploss (nodeid, loss, simulationtime)"
                    " VALUES (?, ?, ?)"
                    ";";

            sqlite3_prepare_v2(m_db, query.c_str(), -1, &stmt, 0);

            sqlite3_bind_int64(stmt, 1, e2NodeId);
            sqlite3_bind_double(stmt, 2, appLoss);
            sqlite3_bind_int64(stmt, 3, t.GetTimeStep());

            rc = sqlite3_step(stmt);

            CheckQueryReturnCode(stmt, rc, FormatBoundArgsList(e2NodeId, appLoss, t.GetTimeStep()));
            sqlite3_finalize(stmt);
        }
    }
}

void
OranNrDataRepositorySqlite::SaveNrUeRsrpRsrq(uint64_t e2NodeId,
                                            Time t,
                                            uint16_t rnti,
                                            uint16_t cellId,
                                            double rsrp,
                                            double rsrq,
                                            bool isServing,
                                            uint8_t componentCarrierId)
{
    NS_LOG_FUNCTION(this << e2NodeId << t << +rnti << +cellId << rsrp << rsrq << isServing
                         << +componentCarrierId);

    if (m_active)
    {
        if (IsNodeRegistered(e2NodeId))
        {
            int rc;
            std::string query;
            sqlite3_stmt* stmt = nullptr;

            sqlite3_prepare_v2(m_db,
                               m_queryStmtsStrings[INSERT_NR_UE_RSRP_RSRQ].c_str(),
                               -1,
                               &stmt,
                               0);

            sqlite3_bind_int64(stmt, 1, e2NodeId);
            sqlite3_bind_int64(stmt, 2, t.GetTimeStep());
            sqlite3_bind_int(stmt, 3, rnti);
            sqlite3_bind_int(stmt, 4, cellId);
            sqlite3_bind_double(stmt, 5, rsrp);
            sqlite3_bind_double(stmt, 6, rsrq);
            sqlite3_bind_int(stmt, 7, isServing);
            sqlite3_bind_int(stmt, 8, componentCarrierId);

            rc = sqlite3_step(stmt);

            CheckQueryReturnCode(stmt,
                                 rc,
                                 FormatBoundArgsList(e2NodeId,
                                                     t.GetTimeStep(),
                                                     rnti,
                                                     cellId,
                                                     rsrp,
                                                     rsrq,
                                                     isServing,
                                                     componentCarrierId));
            sqlite3_finalize(stmt);
        }
    }
}

std::map<Time, Vector>
OranNrDataRepositorySqlite::GetNodePositions(uint64_t e2NodeId,
                                           Time fromTime,
                                           Time toTime,
                                           uint64_t maxEntries)
{
    NS_LOG_FUNCTION(this << e2NodeId << fromTime << toTime << maxEntries);

    std::map<Time, Vector> nodePositions;

    if (m_active)
    {
        if (IsNodeRegistered(e2NodeId))
        {
            int rc;
            sqlite3_stmt* stmt = nullptr;

            sqlite3_prepare_v2(m_db,
                               m_queryStmtsStrings[GET_NODE_ALL_POSITIONS].c_str(),
                               -1,
                               &stmt,
                               0);

            sqlite3_bind_int64(stmt, 1, e2NodeId);
            sqlite3_bind_int64(stmt, 2, fromTime.GetTimeStep());
            sqlite3_bind_int64(stmt, 3, toTime.GetTimeStep());
            sqlite3_bind_int64(stmt, 4, maxEntries);

            while ((rc = sqlite3_step(stmt)) == SQLITE_ROW)
            {
                uint64_t timeStep = sqlite3_column_int64(stmt, 0);
                double x = sqlite3_column_double(stmt, 1);
                double y = sqlite3_column_double(stmt, 2);
                double z = sqlite3_column_double(stmt, 3);

                Time t = Time(timeStep);
                Vector pos = Vector(x, y, z);

                nodePositions[t] = pos;
            }

            CheckQueryReturnCode(
                stmt,
                rc,
                FormatBoundArgsList(e2NodeId, fromTime.GetTimeStep(), toTime.GetTimeStep()));
            sqlite3_finalize(stmt);
        }
    }
    return nodePositions;
}

std::tuple<bool, uint16_t, uint16_t>
OranNrDataRepositorySqlite::GetNrUeCellInfo(uint64_t e2NodeId)
{
    NS_LOG_FUNCTION(this << e2NodeId);

    auto retVal = std::make_tuple(false, 0, 0);
    if (m_active)
    {
        if (IsNodeRegistered(e2NodeId))
        {
            int rc;
            sqlite3_stmt* stmt = nullptr;

            sqlite3_prepare_v2(m_db,
                               m_queryStmtsStrings[GET_NR_UE_CELLINFO].c_str(),
                               -1,
                               &stmt,
                               0);
            sqlite3_bind_int64(stmt, 1, e2NodeId);

            while ((rc = sqlite3_step(stmt)) == SQLITE_ROW)
            {
                uint16_t cellId = sqlite3_column_int(stmt, 0);
                uint16_t rnti = sqlite3_column_int(stmt, 1);
                retVal = std::make_tuple(true, cellId, rnti);
            }

            CheckQueryReturnCode(stmt, rc, FormatBoundArgsList(e2NodeId));
            sqlite3_finalize(stmt);
        }
    }
    return retVal;
}

std::vector<uint64_t>
OranNrDataRepositorySqlite::GetNrUeE2NodeIds()
{
    NS_LOG_FUNCTION(this);

    std::vector<uint64_t> e2NodeIds;

    if (m_active)
    {
        int rc;
        sqlite3_stmt* stmt = nullptr;

        if (sqlite3_prepare_v2(m_db,
                               m_queryStmtsStrings[GET_NR_ALL_UE_E2NODEIDS].c_str(),
                               -1,
                               &stmt,
                               0) != SQLITE_OK)
        {
            std::cerr << "SQL Error: " << sqlite3_errmsg(m_db) << std::endl;
        }

        while ((rc = sqlite3_step(stmt)) == SQLITE_ROW)
        {
            e2NodeIds.push_back(sqlite3_column_int64(stmt, 0));
        }

        CheckQueryReturnCode(stmt, rc);
        sqlite3_finalize(stmt);
    }
    return e2NodeIds;
}

double
OranNrDataRepositorySqlite::GetAppLoss(uint64_t e2NodeId)
{
    NS_LOG_FUNCTION(this << e2NodeId);

    double loss = 0;

    if (m_active)
    {
        if (IsNodeRegistered(e2NodeId))
        {
            int rc;
            std::string query;
            sqlite3_stmt* stmt = nullptr;

            query = "SELECT loss"
                    " FROM nodeapploss"
                    " WHERE nodeid = ?"
                    " ORDER BY entryid DESC LIMIT 1"
                    ";";

            sqlite3_prepare_v2(m_db, query.c_str(), -1, &stmt, 0);

            sqlite3_bind_int64(stmt, 1, e2NodeId);

            while ((rc = sqlite3_step(stmt)) == SQLITE_ROW)
            {
                loss = sqlite3_column_double(stmt, 0);
            }

            CheckQueryReturnCode(stmt, rc, FormatBoundArgsList(e2NodeId));
            sqlite3_finalize(stmt);
        }
    }
    return loss;
}

uint64_t
OranNrDataRepositorySqlite::GetNrUeE2NodeIdFromCellInfo(uint16_t cellId, uint16_t rnti)
{
    NS_LOG_FUNCTION(this << cellId << rnti);

    uint64_t id = 0;
    if (m_active)
    {
        int rc;
        sqlite3_stmt* stmt = nullptr;

        sqlite3_prepare_v2(m_db,
                           m_queryStmtsStrings[GET_NR_UE_E2NODEID_FROM_CELLINFO].c_str(),
                           -1,
                           &stmt,
                           0);
        sqlite3_bind_int(stmt, 1, cellId);
        sqlite3_bind_int(stmt, 2, rnti);

        while ((rc = sqlite3_step(stmt)) == SQLITE_ROW)
        {
            id = sqlite3_column_int64(stmt, 0);
        }

        CheckQueryReturnCode(stmt, rc, FormatBoundArgsList(cellId, rnti));
        sqlite3_finalize(stmt);
    }
    return id;
}

std::tuple<bool, uint16_t>
OranNrDataRepositorySqlite::GetNrGnbCellInfo(uint64_t e2NodeId)
{
    NS_LOG_FUNCTION(this << e2NodeId);

    auto retVal = std::make_tuple(false, 0);
    if (m_active)
    {
        if (IsNodeRegistered(e2NodeId))
        {
            int rc;
            sqlite3_stmt* stmt = nullptr;

            sqlite3_prepare_v2(m_db,
                               m_queryStmtsStrings[GET_NR_CELLID_FROM_E2NODEID].c_str(),
                               -1,
                               &stmt,
                               0);
            sqlite3_bind_int64(stmt, 1, e2NodeId);

            while ((rc = sqlite3_step(stmt)) == SQLITE_ROW)
            {
                uint16_t cellId = sqlite3_column_int(stmt, 0);
                retVal = std::make_tuple(true, cellId);
            }

            CheckQueryReturnCode(stmt, rc, FormatBoundArgsList(e2NodeId));
            sqlite3_finalize(stmt);
        }
    }
    return retVal;
}

std::vector<uint64_t>
OranNrDataRepositorySqlite::GetNrGnbE2NodeIds()
{
    NS_LOG_FUNCTION(this);

    std::vector<uint64_t> e2NodeIds;

    if (m_active)
    {
        int rc;
        sqlite3_stmt* stmt = nullptr;

        sqlite3_prepare_v2(m_db,
                           m_queryStmtsStrings[GET_NR_ALL_GNB_E2NODEIDS].c_str(),
                           -1,
                           &stmt,
                           0);

        while ((rc = sqlite3_step(stmt)) == SQLITE_ROW)
        {
            e2NodeIds.push_back(sqlite3_column_int64(stmt, 0));
        }

        CheckQueryReturnCode(stmt, rc);
        sqlite3_finalize(stmt);
    }
    return e2NodeIds;
}

std::vector<std::tuple<uint64_t, Time>>
OranNrDataRepositorySqlite::GetLastRegistrationRequests()
{
    NS_LOG_FUNCTION(this);

    std::vector<std::tuple<uint64_t, Time>> requests;
    if (m_active)
    {
        int rc;
        sqlite3_stmt* stmt = nullptr;

        sqlite3_prepare_v2(m_db,
                           m_queryStmtsStrings[GET_ALL_LAST_REGISTRATION_TIMES].c_str(),
                           -1,
                           &stmt,
                           0);

        while ((rc = sqlite3_step(stmt)) == SQLITE_ROW)
        {
            uint64_t e2NodeId = sqlite3_column_int64(stmt, 0);
            Time t = Time(sqlite3_column_int64(stmt, 1));

            requests.push_back(std::make_tuple(e2NodeId, t));
        }

        CheckQueryReturnCode(stmt, rc);
        sqlite3_finalize(stmt);
    }

    return requests;
}

std::vector<std::tuple<uint16_t, uint16_t, double, double, bool, uint8_t>>
OranNrDataRepositorySqlite::GetNrUeRsrpRsrq(uint64_t e2NodeId)
{
    NS_LOG_FUNCTION(this << e2NodeId);

    std::vector<std::tuple<uint16_t, uint16_t, double, double, bool, uint8_t>> retVal;

    if (m_active)
    {
        if (IsNodeRegistered(e2NodeId))
        {
            int rc;
            sqlite3_stmt* stmt = nullptr;

            sqlite3_prepare_v2(m_db,
                               m_queryStmtsStrings[GET_NR_UE_RSRP_RSRQ].c_str(),
                               -1,
                               &stmt,
                               0);
            sqlite3_bind_int64(stmt, 1, e2NodeId);
            sqlite3_bind_int64(stmt, 2, e2NodeId);

            while ((rc = sqlite3_step(stmt)) == SQLITE_ROW)
            {
                uint16_t rnti = sqlite3_column_int(stmt, 0);
                uint16_t cellId = sqlite3_column_int(stmt, 1);
                double rsrp = sqlite3_column_double(stmt, 2);
                double rsrq = sqlite3_column_double(stmt, 3);
                bool isServing = sqlite3_column_int(stmt, 4);
                uint8_t componentCarrierId = sqlite3_column_int(stmt, 5);

                retVal.push_back(
                    std::make_tuple(rnti, cellId, rsrp, rsrq, isServing, componentCarrierId));
            }

            CheckQueryReturnCode(stmt, rc, FormatBoundArgsList(e2NodeId, e2NodeId));
            sqlite3_finalize(stmt);
        }
    }
    return retVal;
}

// Modificación del proyecto:
// Función agregada para consultar los reportes de estado de buffer almacenados para un UE.
std::vector<OranNrDataRepository::NrUeBufferStatus>
OranNrDataRepositorySqlite::GetNrUeBufferStatus(uint64_t e2NodeId)
{
    NS_LOG_FUNCTION(this << e2NodeId);

    std::vector<OranNrDataRepository::NrUeBufferStatus> retVal;

    if (m_active && IsNodeRegistered(e2NodeId))
    {
        int rc;
        sqlite3_stmt* stmt = nullptr;

        sqlite3_prepare_v2(m_db,
                           m_queryStmtsStrings[GET_NR_UE_TX_BUFFER_STATUS].c_str(),
                           -1,
                           &stmt,
                           0);

        sqlite3_bind_int64(stmt, 1, e2NodeId);
        sqlite3_bind_int64(stmt, 2, e2NodeId);

        while ((rc = sqlite3_step(stmt)) == SQLITE_ROW)
        {
            OranNrDataRepository::NrUeBufferStatus sample;

            sample.time = NanoSeconds(sqlite3_column_int64(stmt, 0));
            sample.rnti = static_cast<uint16_t>(sqlite3_column_int(stmt, 1));
            sample.lcid = static_cast<uint8_t>(sqlite3_column_int(stmt, 2));
            sample.txQueueSize = static_cast<uint32_t>(sqlite3_column_int(stmt, 3));
            sample.txQueueHolDelay = static_cast<uint16_t>(sqlite3_column_int(stmt, 4));
            sample.retxQueueSize = static_cast<uint32_t>(sqlite3_column_int(stmt, 5));
            sample.retxQueueHolDelay = static_cast<uint16_t>(sqlite3_column_int(stmt, 6));
            sample.statusPduSize = static_cast<uint16_t>(sqlite3_column_int(stmt, 7));

            retVal.push_back(sample);
        }

        CheckQueryReturnCode(stmt, rc, FormatBoundArgsList(e2NodeId, e2NodeId));
        sqlite3_finalize(stmt);
    }

    return retVal;
}

// Modificación del proyecto:
// Función agregada para consultar las muestras recientes de CQI, MCS y RI almacenadas para un UE.
std::vector<OranNrDataRepository::NrUeCqi>
OranNrDataRepositorySqlite::GetNrUeCqi(uint64_t e2NodeId)
{
    NS_LOG_FUNCTION(this << e2NodeId);

    if (m_cqiSamples.count(e2NodeId) > 0)
    {
        return m_cqiSamples[e2NodeId];
    }

    return {};
}

// Modificación del proyecto:
// Función agregada para consultar los reportes de PDU transmitidas almacenados para un UE.
std::vector<OranNrDataRepository::NrUeTxPdu>
OranNrDataRepositorySqlite::GetNrUeTxPdu(uint64_t e2NodeId)
{
    NS_LOG_FUNCTION(this << e2NodeId);

    std::vector<OranNrDataRepository::NrUeTxPdu> retVal;

    if (m_active && IsNodeRegistered(e2NodeId))
    {
        int rc;
        sqlite3_stmt* stmt = nullptr;

        sqlite3_prepare_v2(m_db,
                           m_queryStmtsStrings[GET_NR_UE_TX_PDU].c_str(),
                           -1,
                           &stmt,
                           0);

        sqlite3_bind_int64(stmt, 1, e2NodeId);
        sqlite3_bind_int64(stmt, 2, e2NodeId);

        while ((rc = sqlite3_step(stmt)) == SQLITE_ROW)
        {
            OranNrDataRepository::NrUeTxPdu sample;

            sample.time = NanoSeconds(sqlite3_column_int64(stmt, 0));
            sample.rnti = static_cast<uint16_t>(sqlite3_column_int(stmt, 1));
            sample.lcid = static_cast<uint8_t>(sqlite3_column_int(stmt, 2));
            sample.txPdu = static_cast<uint32_t>(sqlite3_column_int(stmt, 3));

            retVal.push_back(sample);
        }

        CheckQueryReturnCode(stmt, rc, FormatBoundArgsList(e2NodeId, e2NodeId));
        sqlite3_finalize(stmt);
    }

    return retVal;
}

// Modificación del proyecto:
// Función agregada para consultar los reportes de paquetes descartados en transmisión almacenados para un UE.
std::vector<OranNrDataRepository::NrUeTxDrop>
OranNrDataRepositorySqlite::GetNrUeTxDrop(uint64_t e2NodeId)
{
    NS_LOG_FUNCTION(this << e2NodeId);

    std::vector<OranNrDataRepository::NrUeTxDrop> retVal;

    if (m_active && IsNodeRegistered(e2NodeId))
    {
        int rc;
        sqlite3_stmt* stmt = nullptr;

        sqlite3_prepare_v2(m_db,
                           m_queryStmtsStrings[GET_NR_UE_TX_DROP].c_str(),
                           -1,
                           &stmt,
                           0);

        sqlite3_bind_int64(stmt, 1, e2NodeId);
        sqlite3_bind_int64(stmt, 2, e2NodeId);

        while ((rc = sqlite3_step(stmt)) == SQLITE_ROW)
        {
            OranNrDataRepository::NrUeTxDrop sample;

            sample.time = NanoSeconds(sqlite3_column_int64(stmt, 0));
            sample.rnti = static_cast<uint16_t>(sqlite3_column_int(stmt, 1));
            sample.lcid = static_cast<uint8_t>(sqlite3_column_int(stmt, 2));
            sample.txDrop = static_cast<uint32_t>(sqlite3_column_int(stmt, 3));

            retVal.push_back(sample);
        }

        CheckQueryReturnCode(stmt, rc, FormatBoundArgsList(e2NodeId, e2NodeId));
        sqlite3_finalize(stmt);
    }

    return retVal;
}


void
OranNrDataRepositorySqlite::LogCommandE2Terminator(Ptr<OranCommand> cmd)
{
    NS_LOG_FUNCTION(this);

    if (m_active)
    {
        if (IsNodeRegistered(cmd->GetTargetE2NodeId()))
        {
            int rc;
            sqlite3_stmt* stmt = nullptr;

            sqlite3_prepare_v2(m_db,
                               m_queryStmtsStrings[LOG_E2TERMINATOR_COMMAND].c_str(),
                               -1,
                               &stmt,
                               0);

            sqlite3_bind_int64(stmt, 1, cmd->GetTargetE2NodeId());
            sqlite3_bind_int64(stmt, 2, Simulator::Now().GetTimeStep());
            sqlite3_bind_text(stmt, 3, cmd->ToString().c_str(), -1, 0);

            rc = sqlite3_step(stmt);
            CheckQueryReturnCode(stmt,
                                 rc,
                                 FormatBoundArgsList(cmd->GetTargetE2NodeId(),
                                                     Simulator::Now().GetTimeStep(),
                                                     cmd->ToString()));
            sqlite3_finalize(stmt);
        }
    }
}

void
OranNrDataRepositorySqlite::LogCommandLm(std::string lm, Ptr<OranCommand> cmd)
{
    NS_LOG_FUNCTION(this);

    if (m_active)
    {
        int rc;
        sqlite3_stmt* stmt = nullptr;

        sqlite3_prepare_v2(m_db, m_queryStmtsStrings[LOG_LM_COMMAND].c_str(), -1, &stmt, 0);

        sqlite3_bind_text(stmt, 1, lm.c_str(), -1, 0);
        sqlite3_bind_int64(stmt, 2, Simulator::Now().GetTimeStep());
        sqlite3_bind_text(stmt, 3, cmd->ToString().c_str(), -1, 0);

        rc = sqlite3_step(stmt);
        CheckQueryReturnCode(
            stmt,
            rc,
            FormatBoundArgsList(lm, Simulator::Now().GetTimeStep(), cmd->ToString()));
        sqlite3_finalize(stmt);
    }
}

void
OranNrDataRepositorySqlite::LogActionLm(std::string lm, std::string logStr)
{
    NS_LOG_FUNCTION(this << lm << logStr);

    if (m_active)
    {
        int rc;
        sqlite3_stmt* stmt = nullptr;

        sqlite3_prepare_v2(m_db, m_queryStmtsStrings[LOG_LM_ACTION].c_str(), -1, &stmt, 0);

        sqlite3_bind_text(stmt, 1, lm.c_str(), -1, 0);
        sqlite3_bind_int64(stmt, 2, Simulator::Now().GetTimeStep());
        sqlite3_bind_text(stmt, 3, logStr.c_str(), -1, 0);

        rc = sqlite3_step(stmt);

        CheckQueryReturnCode(stmt,
                             rc,
                             FormatBoundArgsList(lm, Simulator::Now().GetTimeStep(), logStr));
        sqlite3_finalize(stmt);
    }
}

void
OranNrDataRepositorySqlite::LogActionCmm(std::string cmm, std::string logStr)
{
    NS_LOG_FUNCTION(this << cmm << logStr);

    if (m_active)
    {
        int rc;
        sqlite3_stmt* stmt = nullptr;

        sqlite3_prepare_v2(m_db, m_queryStmtsStrings[LOG_CMM_ACTION].c_str(), -1, &stmt, 0);

        sqlite3_bind_text(stmt, 1, cmm.c_str(), -1, 0);
        sqlite3_bind_int64(stmt, 2, Simulator::Now().GetTimeStep());
        sqlite3_bind_text(stmt, 3, logStr.c_str(), -1, 0);

        rc = sqlite3_step(stmt);

        CheckQueryReturnCode(stmt,
                             rc,
                             FormatBoundArgsList(cmm, Simulator::Now().GetTimeStep(), logStr));
        sqlite3_finalize(stmt);
    }
}

void
OranNrDataRepositorySqlite::CheckQueryReturnCode(sqlite3_stmt* stmt,
                                               int rc,
                                               std::string boundParmsStr) const
{
    NS_LOG_FUNCTION(this << stmt << rc);

    // Get the formated string of the prepared statement
    std::string stmtStr = sqlite3_sql(stmt);

    // Trace the result of the query
    m_queryRc(stmtStr, boundParmsStr, rc);

    if (rc == SQLITE_OK || rc == SQLITE_DONE)
    {
        NS_LOG_INFO("Query SUCCESSFUL: \"" << stmtStr << "\"; " << boundParmsStr);
    }
    else
    {
        NS_ABORT_MSG("Query FAILED: \"" << stmtStr << "\"; (" << boundParmsStr << "); RC = " << rc);
    }
}

void
OranNrDataRepositorySqlite::CloseDb()
{
    NS_LOG_FUNCTION(this);

    sqlite3_close(m_db);
    m_db = nullptr;
}

void
OranNrDataRepositorySqlite::DoDispose()
{
    NS_LOG_FUNCTION(this);

    if (IsDbOpen())
    {
        CloseDb();
    }

    OranNrDataRepository::DoDispose();
}

bool
OranNrDataRepositorySqlite::IsDbOpen() const
{
    NS_LOG_FUNCTION(this);

    return m_db != nullptr;
}

void
OranNrDataRepositorySqlite::OpenDb()
{
    NS_LOG_FUNCTION(this);

    // Check for special file names and print a warning if we find them
    if (m_dbPath == ":memory:")
    {
        NS_LOG_WARN("Using in-memory DB for the ORAN Storage. DB will not be saved to disk.");
        std::cerr
            << "WARNING: Using in-memory DB for the ORAN Storage. DB will not be saved to disk."
            << std::endl;
    }
    else
    {
        // Check for DB names that are URIs. We do not support those
        if (m_dbPath.find(":") != std::string::npos)
        {
            NS_ABORT_MSG("File name for the ORAN Storage DB ("
                         << m_dbPath << ") is an URI. URI-named DBs are not supported");
        }
    }

    if (m_dbPath.empty())
    {
        NS_LOG_WARN("Using a randomly named temporary file as DB for the ORAN Storage. DB will not "
                    "be available after simulation ends.");
        std::cerr << "WARNING: Using a randomly named temporary file as DB for the ORAN Storage. "
                     "DB will not be available after simulation ends."
                  << std::endl;
    }

    int error = sqlite3_open(m_dbPath.c_str(), &m_db);
    if (error != 0)
    {
        NS_ABORT_MSG("Could not open database: " << sqlite3_errmsg(m_db));
    }
    else
    {
        NS_LOG_INFO("Oran repository \"" << m_dbPath << "\" connected to successfully!");
        ;
    }

    InitDb();
}

void
OranNrDataRepositorySqlite::InitDb()
{
    NS_LOG_FUNCTION(this);

    // E2 Node Table
    RunCreateStatement(m_createStmtsStrings[TABLE_NODE]);
    RunCreateStatement(m_createStmtsStrings[INDEX_NODE]);

    // E2 Node Registration
    RunCreateStatement(m_createStmtsStrings[TABLE_NODE_REGISTRATION]);
    RunCreateStatement(m_createStmtsStrings[INDEX_NODE_REGISTRATION]);

    // E2 Node Location
    RunCreateStatement(m_createStmtsStrings[TABLE_NODE_LOCATION]);
    RunCreateStatement(m_createStmtsStrings[INDEX_NODE_LOCATION]);

    // NR gNB
    RunCreateStatement(m_createStmtsStrings[TABLE_NR_GNB]);
    RunCreateStatement(m_createStmtsStrings[INDEX_NR_GNB_NODEID]);
    RunCreateStatement(m_createStmtsStrings[INDEX_NR_GNB_CELLID]);

    // NR UE
    RunCreateStatement(m_createStmtsStrings[TABLE_NR_UE]);
    RunCreateStatement(m_createStmtsStrings[INDEX_NR_UE_NODEID]);
    RunCreateStatement(m_createStmtsStrings[INDEX_NR_UE_IMSI]);

    // NR UE Cell Information
    RunCreateStatement(m_createStmtsStrings[TABLE_NR_UE_CELL]);
    RunCreateStatement(m_createStmtsStrings[TABLE_NR_UE_RSRP_RSRQ]);
    RunCreateStatement(m_createStmtsStrings[INDEX_NR_UE_CELL_NODEID]);
    RunCreateStatement(m_createStmtsStrings[INDEX_NR_UE_CELL_CELLID]);
    RunCreateStatement(m_createStmtsStrings[TABLE_APPLOSS_COMMAND]);

    // Modificación del proyecto:
    // Creación de tablas adicionales para almacenar reportes NR.
    RunCreateStatement(m_createStmtsStrings[TABLE_NR_UE_TX_BUFFER]);
    RunCreateStatement(m_createStmtsStrings[TABLE_NR_UE_TX_PDU]);
    RunCreateStatement(m_createStmtsStrings[TABLE_NR_UE_TX_DROP]);    
    RunCreateStatement(m_createStmtsStrings[TABLE_NR_UE_CQI]);

    // E2 Terminator Commands
    RunCreateStatement(m_createStmtsStrings[TABLE_TERMINATOR_COMMAND]);

    // LM Commands
    RunCreateStatement(m_createStmtsStrings[TABLE_LM_COMMAND]);

    // LM Actions (Internal Log)
    RunCreateStatement(m_createStmtsStrings[TABLE_LM_ACTION]);

    // CMM Actions (Internal Log)
    RunCreateStatement(m_createStmtsStrings[TABLE_CMM_ACTION]);
}

void
OranNrDataRepositorySqlite::InitStatements()
{
    NS_LOG_FUNCTION(this);

    // Initialize the create statements
    m_createStmtsStrings[INDEX_NR_GNB_CELLID] = "CREATE INDEX IF NOT EXISTS "
                                                 "idx_nrgnb_cellid ON nrgnb(cellid);";

    m_createStmtsStrings[INDEX_NR_GNB_NODEID] = "CREATE INDEX IF NOT EXISTS "
                                                 "idx_nrgnb_nodeid ON nrgnb(nodeid);";

    m_createStmtsStrings[INDEX_NR_UE_CELL_CELLID] = "CREATE INDEX IF NOT EXISTS "
                                                     "idx_nruecell_cellid ON nruecell(cellid);";

    m_createStmtsStrings[INDEX_NR_UE_CELL_NODEID] = "CREATE INDEX IF NOT EXISTS "
                                                     "idx_nruecell_nodeid ON nruecell(nodeid);";

    m_createStmtsStrings[INDEX_NR_UE_IMSI] = "CREATE INDEX IF NOT EXISTS "
                                              "idx_nrue_imsi ON nrue(imsi);";

    m_createStmtsStrings[INDEX_NR_UE_NODEID] = "CREATE INDEX IF NOT EXISTS "
                                                "idx_nrue_nodeid ON nrue(nodeid);";

    m_createStmtsStrings[INDEX_NODE] = "CREATE INDEX IF NOT EXISTS "
                                       "idx_node_nodeid ON node (nodeid);";

    m_createStmtsStrings[INDEX_NODE_LOCATION] = "CREATE INDEX IF NOT EXISTS "
                                                "idx_nodelocation_nodeid ON nodelocation(nodeid);";

    m_createStmtsStrings[INDEX_NODE_REGISTRATION] =
        "CREATE INDEX IF NOT EXISTS "
        "idx_noderegistration_nodeid ON noderegistration(nodeid);";

    m_createStmtsStrings[TABLE_CMM_ACTION] =
        "CREATE TABLE IF NOT EXISTS cmmaction ("
        "entryid        INTEGER PRIMARY KEY AUTOINCREMENT NOT NULL, "
        "cmmname        TEXT                              NOT NULL, "
        "simulationtime INTEGER                           NOT NULL, "
        "description    TEXT                              NOT NULL);";

    m_createStmtsStrings[TABLE_LM_ACTION] =
        "CREATE TABLE IF NOT EXISTS lmaction ("
        "entryid        INTEGER PRIMARY KEY AUTOINCREMENT NOT NULL, "
        "lmname         TEXT                              NOT NULL, "
        "simulationtime INTEGER                           NOT NULL, "
        "description    TEXT                              NOT NULL);";

    m_createStmtsStrings[TABLE_LM_COMMAND] =
        "CREATE TABLE IF NOT EXISTS lmcommand ("
        "entryid        INTEGER PRIMARY KEY AUTOINCREMENT NOT NULL, "
        "lmname         TEXT                              NOT NULL, "
        "simulationtime INTEGER                           NOT NULL, "
        "cmdname        TEXT                              NOT NULL);";

    m_createStmtsStrings[TABLE_NR_GNB] = "CREATE TABLE IF NOT EXISTS nrgnb ("
                                          "nodeid INTEGER PRIMARY KEY NOT NULL, "
                                          "cellid INTEGER             NOT NULL, "
                                          "FOREIGN KEY(nodeid) REFERENCES node(nodeid));";

    m_createStmtsStrings[TABLE_NR_UE] = "CREATE TABLE IF NOT EXISTS nrue ("
                                         "nodeid INTEGER PRIMARY KEY NOT NULL, "
                                         "imsi   INTEGER UNIQUE      NOT NULL, "
                                         "FOREIGN KEY(nodeid) REFERENCES node(nodeid));";

    m_createStmtsStrings[TABLE_NR_UE_CELL] =
        "CREATE TABLE IF NOT EXISTS nruecell ("
        "entryid        INTEGER PRIMARY KEY AUTOINCREMENT NOT NULL, "
        "nodeid         INTEGER                           NOT NULL, "
        "cellid         INTEGER                           NOT NULL, "
        "rnti           INTEGER                           NOT NULL, "
        "simulationtime INTEGER                           NOT NULL, "
        "FOREIGN KEY(cellid) REFERENCES nrgnb(cellid), "
        "FOREIGN KEY(nodeid) REFERENCES nrue(nodeid));";

    // Modificación del proyecto:
    // Tabla agregada para almacenar reportes de estado de buffer de los UE.
    m_createStmtsStrings[TABLE_NR_UE_TX_BUFFER] =
        "CREATE TABLE IF NOT EXISTS nruetxbuffer ("
        "entryid             INTEGER PRIMARY KEY AUTOINCREMENT NOT NULL, "
        "nodeid              INTEGER                           NOT NULL, "
        "simulationtime      INTEGER                           NOT NULL, "
        "rnti                INTEGER                           NOT NULL, "
        "lcid                INTEGER                           NOT NULL, "
        "txqueuesize         INTEGER                           NOT NULL, "
        "txqueueholdelay     INTEGER                           NOT NULL, "
        "retxqueuesize       INTEGER                           NOT NULL, "
        "retxqueueholdelay   INTEGER                           NOT NULL, "
        "statuspdusize       INTEGER                           NOT NULL, "
        "FOREIGN KEY(nodeid) REFERENCES nrue(nodeid));";
       
    // Modificación del proyecto:
    // Tabla agregada para almacenar reportes de PDU transmitidas por los UE.
    m_createStmtsStrings[TABLE_NR_UE_TX_PDU] =
        "CREATE TABLE IF NOT EXISTS nruetxpdu ("
        "entryid        INTEGER PRIMARY KEY AUTOINCREMENT NOT NULL, "
        "nodeid         INTEGER                           NOT NULL, "
        "simulationtime INTEGER                           NOT NULL, "
        "rnti           INTEGER                           NOT NULL, "
        "lcid           INTEGER                           NOT NULL, "
        "txpdu          INTEGER                           NOT NULL, "
        "FOREIGN KEY(nodeid) REFERENCES nrue(nodeid));";

    // Modificación del proyecto:
    // Tabla agregada para almacenar reportes de paquetes descartados por los UE en transmisión.
    m_createStmtsStrings[TABLE_NR_UE_TX_DROP] =
        "CREATE TABLE IF NOT EXISTS nruetxdrop ("
        "entryid        INTEGER PRIMARY KEY AUTOINCREMENT NOT NULL, "
        "nodeid         INTEGER                           NOT NULL, "
        "simulationtime INTEGER                           NOT NULL, "
        "rnti           INTEGER                           NOT NULL, "
        "lcid           INTEGER                           NOT NULL, "
        "txdrop         INTEGER                           NOT NULL, "
        "FOREIGN KEY(nodeid) REFERENCES nrue(nodeid));";
    
    // Modificación del proyecto:
    // Tabla agregada para almacenar reportes de CQI, MCS y RI de los UE.
    m_createStmtsStrings[TABLE_NR_UE_CQI] =
        "CREATE TABLE IF NOT EXISTS nruecqi ("
        "entryid        INTEGER PRIMARY KEY AUTOINCREMENT NOT NULL, "
        "nodeid         INTEGER                           NOT NULL, "
        "simulationtime INTEGER                           NOT NULL, "
        "rnti           INTEGER                           NOT NULL, "
        "cqi            INTEGER                           NOT NULL, "
        "mcs            INTEGER                           NOT NULL, "
        "ri             INTEGER                           NOT NULL, "
        "FOREIGN KEY(nodeid) REFERENCES nrue(nodeid));";

    m_createStmtsStrings[TABLE_NR_UE_RSRP_RSRQ] =
        "CREATE TABLE IF NOT EXISTS nruersrprsrq ("
        "entryid        INTEGER PRIMARY KEY AUTOINCREMENT NOT NULL, "
        "nodeid         INTEGER                           NOT NULL, "
        "simulationtime INTEGER                           NOT NULL, "
        "rnti           INTEGER                           NOT NULL, "
        "cellid         INTEGER                           NOT NULL, "
        "rsrp           REAL                              NOT NULL, "
        "rsrq           REAL                              NOT NULL, "
        "serving        BOOLEAN                           NOT NULL, "
        "ccid           BOOLEAN                           NOT NULL, "
        "FOREIGN KEY(cellid) REFERENCES nrgnb(cellid), "
        "FOREIGN KEY(nodeid) REFERENCES nrue(nodeid));";

    m_createStmtsStrings[TABLE_NODE] =
        "CREATE TABLE IF NOT EXISTS node ("
        "nodeid         INTEGER PRIMARY KEY AUTOINCREMENT NOT NULL, "
        "nodetype       INTEGER                           NOT NULL);";

    m_createStmtsStrings[TABLE_NODE_LOCATION] =
        "CREATE TABLE IF NOT EXISTS nodelocation ("
        "entryid        INTEGER PRIMARY KEY AUTOINCREMENT NOT NULL, "
        "nodeid         INTEGER                           NOT NULL, "
        "x              REAL                              NOT NULL, "
        "y              REAL                              NOT NULL, "
        "z              REAL                              NOT NULL, "
        "simulationtime INTEGER                           NOT NULL, "
        "FOREIGN KEY(nodeid) REFERENCES node(nodeid));";

    m_createStmtsStrings[TABLE_NODE_REGISTRATION] =
        "CREATE TABLE IF NOT EXISTS noderegistration ("
        "entryid        INTEGER PRIMARY KEY AUTOINCREMENT NOT NULL, "
        "nodeid         INTEGER                           NOT NULL, "
        "registered     BOOLEAN                           NOT NULL, "
        "simulationtime INTEGER                           NOT NULL, "
        "FOREIGN KEY(nodeid) REFERENCES node(nodeid));";

    m_createStmtsStrings[TABLE_TERMINATOR_COMMAND] =
        "CREATE TABLE IF NOT EXISTS terminatorcommand ("
        "entryid        INTEGER PRIMARY KEY AUTOINCREMENT NOT NULL, "
        "targetid       INTEGER                           NOT NULL, "
        "simulationtime INTEGER                           NOT NULL, "
        "cmdname        TEXT                              NOT NULL, "
        "FOREIGN KEY(targetid) REFERENCES node(nodeid));";

    m_createStmtsStrings[TABLE_APPLOSS_COMMAND] =
        "CREATE TABLE IF NOT EXISTS nodeapploss ("
        "entryid        INTEGER PRIMARY KEY AUTOINCREMENT NOT NULL, "
        "nodeid         INTEGER                           NOT NULL, "
        "loss           REAL                              NOT NULL, "
        "simulationtime INTEGER                           NOT NULL, "
        "FOREIGN KEY(nodeid) REFERENCES node(nodeid)              );";

    // Query Statements
    m_queryStmtsStrings[CHECK_NODE_REGISTERED] = "SELECT registered "
                                                 "FROM noderegistration "
                                                 "WHERE nodeid = ? "
                                                 "ORDER BY simulationtime DESC, entryid DESC "
                                                 "LIMIT 1;";

    m_queryStmtsStrings[GET_ALL_LAST_REGISTRATION_TIMES] = "SELECT nodeid, MAX(simulationtime) "
                                                           "FROM noderegistration "
                                                           "GROUP BY nodeid "
                                                           "HAVING registered = 1 "
                                                           "ORDER BY nodeid;";

    m_queryStmtsStrings[GET_NR_ALL_GNB_E2NODEIDS] =
        "SELECT nr.nodeid, MAX(nr.simulationtime) "
        "FROM noderegistration AS nr "
        "INNER JOIN nrgnb ON nrgnb.nodeid = nr.nodeid "
        "GROUP BY nr.nodeid "
        "HAVING nr.registered = 1 "
        "ORDER BY nr.nodeid;";

    m_queryStmtsStrings[GET_NR_ALL_UE_E2NODEIDS] = "SELECT nr.nodeid, MAX(nr.simulationtime) "
                                                    "FROM noderegistration AS nr "
                                                    "INNER JOIN nrue ON nrue.nodeid = nr.nodeid "
                                                    "GROUP BY nr.nodeid "
                                                    "HAVING nr.registered = 1 "
                                                    "ORDER BY nr.nodeid;";

    m_queryStmtsStrings[GET_NR_CELLID_FROM_E2NODEID] = "SELECT cellid "
                                                        "FROM nrgnb "
                                                        "WHERE nodeid = ?;";

    m_queryStmtsStrings[GET_NR_UE_CELLINFO] = "SELECT cellid, rnti "
                                               "FROM nruecell "
                                               "WHERE nodeid = ? "
                                               "ORDER BY simulationtime DESC, entryid DESC "
                                               "LIMIT 1;";

    m_queryStmtsStrings[GET_NR_UE_TX_BUFFER_STATUS] =
        "SELECT simulationtime, rnti, lcid, txqueuesize, txqueueholdelay, retxqueuesize, "
        "retxqueueholdelay, statuspdusize "
        "FROM nruetxbuffer "
        "WHERE nodeid = ? "
        "ORDER BY simulationtime ASC, entryid ASC;";                                               


    m_queryStmtsStrings[GET_NR_UE_TX_PDU] =
        "SELECT simulationtime, rnti, lcid, txpdu "
        "FROM nruetxpdu "
        "WHERE nodeid = ? "
        "ORDER BY simulationtime DESC, entryid DESC;";

    m_queryStmtsStrings[GET_NR_UE_TX_DROP] =
        "SELECT simulationtime, rnti, lcid, txdrop "
        "FROM nruetxdrop "
        "WHERE nodeid = ? "
        "ORDER BY simulationtime DESC, entryid DESC;";


    m_queryStmtsStrings[GET_NR_UE_CQI] =
        "SELECT simulationtime, rnti, cqi, mcs, ri "
        "FROM nruecqi "
        "WHERE nodeid = ? "
        "ORDER BY simulationtime DESC, entryid DESC;";

    m_queryStmtsStrings[GET_NR_UE_E2NODEID_FROM_CELLINFO] = "SELECT nodeid "
                                                             "FROM nruecell "
                                                             "WHERE cellid = ? AND rnti = ? "
                                                             "ORDER BY entryid DESC LIMIT 1;";

    m_queryStmtsStrings[GET_NODE_ALL_POSITIONS] =
        "SELECT simulationtime, x, y, z "
        "FROM nodelocation "
        "WHERE nodeid = ? AND simulationtime >= ? AND simulationtime <= ? "
        "ORDER BY simulationtime DESC, entryid DESC LIMIT ? ;";

    m_queryStmtsStrings[GET_NR_UE_RSRP_RSRQ] = "SELECT rnti, cellid, rsrp, rsrq, serving, ccid "
                                                "FROM nruersrprsrq "
                                                "WHERE nodeid = ? "
                                                "AND simulationtime IN ("
                                                "SELECT simulationtime "
                                                "FROM nruersrprsrq "
                                                "WHERE nodeid = ? "
                                                "ORDER BY simulationtime DESC LIMIT 1"
                                                ");";

    m_queryStmtsStrings[INSERT_NR_GNB_NODE] = "INSERT OR REPLACE INTO nrgnb "
                                               "(nodeid, cellid) VALUES (?, ?);";

    m_queryStmtsStrings[INSERT_NR_UE_CELL] =
        "INSERT INTO nruecell "
        "(nodeid, cellid, rnti, simulationtime) VALUES (?, ?, ?, ?);";

    m_queryStmtsStrings[INSERT_NR_UE_TX_BUFFER_STATUS] =
        "INSERT INTO nruetxbuffer "
        "(nodeid, simulationtime, rnti, lcid, txqueuesize, txqueueholdelay, retxqueuesize, "
        "retxqueueholdelay, statuspdusize) VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?);";

    m_queryStmtsStrings[INSERT_NR_UE_TX_PDU] =
        "INSERT INTO nruetxpdu "
        "(nodeid, simulationtime, rnti, lcid, txpdu) VALUES (?, ?, ?, ?, ?);";

    m_queryStmtsStrings[INSERT_NR_UE_TX_DROP] =
        "INSERT INTO nruetxdrop "
        "(nodeid, simulationtime, rnti, lcid, txdrop) VALUES (?, ?, ?, ?, ?);";

    m_queryStmtsStrings[INSERT_NR_UE_CQI] = "INSERT INTO nruecqi "
                                           "(nodeid, simulationtime, rnti, cqi, mcs, ri) "
                                           "VALUES (?, ?, ?, ?, ?, ?);";

    m_queryStmtsStrings[INSERT_NR_UE_NODE] = "INSERT OR REPLACE INTO nrue "
                                              "(nodeid, imsi) VALUES (?, ?);";

    m_queryStmtsStrings[INSERT_NODE_ADD] = "INSERT INTO node "
                                           "(nodetype) VALUES (?);";

    m_queryStmtsStrings[INSERT_NODE_UPDATE] = "INSERT OR REPLACE INTO node "
                                              "(nodeid, nodetype) VALUES (?, ?);";

    m_queryStmtsStrings[INSERT_NODE_LOCATION] =
        "INSERT INTO nodelocation "
        "(nodeid, x, y, z, simulationtime) VALUES (?, ?, ?, ?, ?);";

    m_queryStmtsStrings[INSERT_NODE_REGISTRATION] =
        "INSERT INTO noderegistration "
        "(nodeid, registered, simulationtime) VALUES (?, ?, ?);";

    m_queryStmtsStrings[INSERT_NR_UE_RSRP_RSRQ] =
        "INSERT INTO nruersrprsrq "
        "(nodeid, simulationtime, rnti, cellid, rsrp, rsrq, serving, ccid) VALUES (?, ?, ?, ?, ?, "
        "?, ?, ?);";

    m_queryStmtsStrings[LOG_CMM_ACTION] =
        "INSERT INTO cmmaction "
        "(cmmname, simulationtime, description) VALUES (?, ?, ?);";

    m_queryStmtsStrings[LOG_E2TERMINATOR_COMMAND] =
        "INSERT INTO terminatorcommand "
        "(targetid, simulationtime, cmdname) VALUES (?, ?, ?);";

    m_queryStmtsStrings[LOG_LM_ACTION] = "INSERT INTO lmaction "
                                         "(lmname, simulationtime, description) VALUES (?, ?, ?);";

    m_queryStmtsStrings[LOG_LM_COMMAND] = "INSERT INTO lmcommand "
                                          "(lmname, simulationtime, cmdname) VALUES (?, ?, ?);";
}

void
OranNrDataRepositorySqlite::RunCreateStatement(std::string statement)
{
    NS_LOG_FUNCTION(this << statement);

    // Keep the return code in a separate variable to make it easier to debug
    // Otherwise, we could just run the sqlite3_step as the 2nd argument to the
    // CheckQueryReturnCode call
    int rc;
    sqlite3_stmt* stmt;

    sqlite3_prepare_v2(m_db, statement.c_str(), -1, &stmt, 0);
    rc = sqlite3_step(stmt);
    CheckQueryReturnCode(stmt, rc);
    sqlite3_finalize(stmt);
}

} // namespace ns3
