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

/**
 * Modificaciones del proyecto:
 * se extendió el repositorio SQLite para almacenar y consultar reportes NR
 * adicionales utilizados por los LM de monitoreo y aprendizaje por refuerzo.
 * Los reportes agregados incluyen estado de buffer, CQI, PDU transmitidas y
 * paquetes descartados en transmisión.
 */

#ifndef ORAN_NR_DATA_REPOSITORY_SQLITE_H
#define ORAN_NR_DATA_REPOSITORY_SQLITE_H

#include "oran-nr-data-repository.h"

#include "ns3/traced-callback.h"

#include <sqlite3.h>
#include <sstream>

namespace ns3
{

/**
 * @ingroup oran
 *
 * A Data Repository implementation that uses an SQLite database as
 * the storage backend.
 *
 * The database used as the backend may be created new, or it may be an existing
 * database. This class does not provide methods for deleting existing database
 * files; if this is required, the user must take care of that in the scenario.
 *
 * The methods defined in the OranNrDataRepository API build SQL prepared
 * statements to access the database, validating the return code after each
 * database query.
 */
class OranNrDataRepositorySqlite : public OranNrDataRepository
{
  public:
    /**
     * Gets the TypeId of the OranNrDataRepositorySqlite class.
     *
     * @return The TypeId.
     */
    static TypeId GetTypeId();
    /**
     * Creates an instance of the OranNrDataRepositorySqlite class.
     */
    OranNrDataRepositorySqlite();
    /**
     * The destructor of the OranNrDataRepositorySqlite class.
     */
    ~OranNrDataRepositorySqlite() override;
    /**
     * Activate the data storage. If the database is not open,
     * this method will call OpenDb.
     */
    void Activate() override;
    /**
     * Deactivate the data storage. If the database is open,
     * this method will call CloseDb.
     */
    void Deactivate() override;

    /* Data Storage API */
    bool IsNodeRegistered(uint64_t e2NodeId) override;

    uint64_t RegisterNode(OranNrNearRtRic::NodeType type, uint64_t id) override;
    uint64_t RegisterNodeNrUe(uint64_t id, uint64_t imsi) override;
    uint64_t RegisterNodeNrGnb(uint64_t id, uint16_t cellId) override;
    uint64_t DeregisterNode(uint64_t e2NodeId) override;
    void SavePosition(uint64_t e2NodeId, Vector pos, Time t) override;
    void SaveNrUeCellInfo(uint64_t e2NodeId, uint16_t cellId, uint16_t rnti, Time t) override;
    void SaveAppLoss(uint64_t e2NodeId, double appLoss, Time t) override;
    void SaveNrUeRsrpRsrq(uint64_t e2NodeId,
                           Time t,
                           uint16_t rnti,
                           uint16_t cellId,
                           double rsrp,
                           double rsrq,
                           bool isServingCell,
                           uint8_t componentCarrierId) override;

    /**
     * Modificación del proyecto:
     * almacena reportes de estado de buffer de transmisión de los UE.
     */
    void SaveNrUeBufferStatus(uint64_t e2NodeId, const NrUeBufferStatus& status) override;

    /**
     * Modificación del proyecto:
     * almacena reportes de CQI, MCS y RI de las UE.
     */
    void SaveNrUeCqi(uint64_t e2NodeId, const NrUeCqi& cqi) override;

    /**
     * Modificación del proyecto:
     * almacena reportes de PDU transmitidas por las UE.
     */
    void SaveNrUeTxPdu(uint64_t e2NodeId, const NrUeTxPdu& pdu) override;

    /**
     * Modificación del proyecto:
     * almacena reportes de paquetes descartados en transmisión por las UE.
     */
    void SaveNrUeTxDrop(uint64_t e2NodeId, const NrUeTxDrop& drop) override;    


    std::map<Time, Vector> GetNodePositions(uint64_t e2NodeId,
                                            Time fromTime,
                                            Time toTime,
                                            uint64_t maxEntries = 1) override;
    std::tuple<bool, uint16_t, uint16_t> GetNrUeCellInfo(uint64_t e2NodeId) override;
    std::vector<uint64_t> GetNrUeE2NodeIds() override;
    uint64_t GetNrUeE2NodeIdFromCellInfo(uint16_t cellId, uint16_t rnti) override;
    std::tuple<bool, uint16_t> GetNrGnbCellInfo(uint64_t e2NodeId) override;
    std::vector<uint64_t> GetNrGnbE2NodeIds() override;
    std::vector<std::tuple<uint64_t, Time>> GetLastRegistrationRequests() override;
    double GetAppLoss(uint64_t e2NodeId) override;
    std::vector<std::tuple<uint16_t, uint16_t, double, double, bool, uint8_t>> GetNrUeRsrpRsrq(
        uint64_t e2NodeId) override;

    /**
     * Modificación del proyecto:
     * consulta los reportes de estado de buffer almacenados para una UE.
     */
    std::vector<NrUeBufferStatus> GetNrUeBufferStatus(uint64_t e2NodeId) override;

    /**
     * Modificación del proyecto:
     * consulta los reportes de CQI, MCS y RI almacenados para una UE.
     */
    std::vector<NrUeCqi> GetNrUeCqi(uint64_t e2NodeId) override;

    /**
     * Modificación del proyecto:
     * consulta los reportes de PDU transmitidas almacenados para una UE.
     */
    std::vector<NrUeTxPdu> GetNrUeTxPdu(uint64_t e2NodeId) override;

    /**
     * Modificación del proyecto:
     * consulta los reportes de paquetes descartados en transmisión almacenados
     * para una UE.
     */
    std::vector<NrUeTxDrop> GetNrUeTxDrop(uint64_t e2NodeId) override;


    void LogCommandE2Terminator(Ptr<OranCommand> cmd) override;
    void LogCommandLm(std::string lm, Ptr<OranCommand> cmd) override;
    void LogActionLm(std::string lm, std::string logstr) override;
    void LogActionCmm(std::string cmm, std::string logstr) override;

    /**
     * TracedCallback signature for SQL Queries. Traces the queries and the result code
     * (does not trace the returned records).
     *
     * @param [in] query The SQL prepared statement
     * @param [in] args The bound arguments (if any)
     * @param [in] rc The return code
     */
    typedef void (*QueryRcTracedCallback)(std::string query, std::string args, int rc);

  protected:
    /**
     * Enumeration with the type of SQL statement.
     * To be used as key for the map with the statement's strings
     */
    enum StatementType
    {
        CHECK_NODE_REGISTERED = 0,         //!< Query if a node is registered
        GET_ALL_LAST_REGISTRATION_TIMES,   //!< Get node registation times
        GET_NR_ALL_GNB_E2NODEIDS,         //!< Get all NR gNB E2 IDs
        GET_NR_ALL_UE_E2NODEIDS,          //!< Get all NR UE E2 IDs
        GET_NR_CELLID_FROM_E2NODEID,      //!< Get the cell ID of an NR gNB from its E2 Node ID
        GET_NR_UE_CELLINFO,               //!< Get the cell information associated with NR UE
        GET_NR_UE_E2NODEID_FROM_CELLINFO, //!< Get the E2 ID of a UE from the cell information

        // Modificación del proyecto:
        // consultas agregadas para recuperar reportes NR usados por los LM.
        GET_NR_UE_CQI,                    //!< Get the UE CQI samples
        GET_NR_UE_TX_BUFFER_STATUS,       //!< Get the UE buffer status samples
        GET_NR_UE_TX_PDU,                 //!< Get the UE transmitted PDU samples
        GET_NR_UE_TX_DROP,                //!< Get the UE dropped PDU samples       

        GET_NR_UE_RSRP_RSRQ,              //!< Get the UE RSRP and RSRQ measurements
        GET_NODE_ALL_POSITIONS,            //!< The location of all nodes E2 nodes
        INSERT_NR_GNB_NODE,               //!< Add an NR gNB E2 node
        INSERT_NR_UE_CELL,                //!< Add NR UE cell information for an E2 node
        INSERT_NR_UE_NODE,                //!< Add an NR UE E2 node
        INSERT_NODE_ADD,                   //!< Add an E2 node
        INSERT_NODE_UPDATE,                //!< Update an E2 node's information
        INSERT_NODE_LOCATION,              //!< Add an E2 node's location
        INSERT_NODE_REGISTRATION,          //!< Add an E2 node registration request
        INSERT_NR_UE_RSRP_RSRQ,           //!< Add NR UE RSRP and RSRQ

        // Modificación del proyecto:
        // sentencias agregadas para insertar reportes NR usados por los LM.
        INSERT_NR_UE_CQI,                 //!< Add NR UE CQI sample
        INSERT_NR_UE_TX_BUFFER_STATUS,    //!< Add NR UE buffer status sample
        INSERT_NR_UE_TX_PDU,              //!< Add NR UE transmitted PDU sample
        INSERT_NR_UE_TX_DROP,             //!< Add NR UE dropped PDU sample

        LOG_CMM_ACTION,                    //!< Log a CM module action
        LOG_E2TERMINATOR_COMMAND,          //!< Log an E2 terminator command from the RIC
        LOG_LM_ACTION,                     //!< Log an LM action
        LOG_LM_COMMAND                     //!< Log an LM command
    };

    /**
     * Enumeration with the type of SQL CREATE TABLE statements
     * To be used as key for the map with the CREATE TABLE statements' strings
     */
    enum CreateStatementType
    {
        INDEX_NR_GNB_CELLID = 0, //!< Index for the table with NR gNB based on Cell IDs
        INDEX_NR_GNB_NODEID,     //!< Index for the table with NR gNB based on E2 Node IDs
        INDEX_NR_UE_CELL_CELLID, //!< Index for the table with NR UE Cell Information based on
                                  //!< Cell IDs
        INDEX_NR_UE_CELL_NODEID, //!< Index for the table with NR UE Cell Information based on E2
                                  //!< Node IDs
        INDEX_NR_UE_IMSI,        //!< Index for the table with NR UE based on IMSI
        INDEX_NR_UE_NODEID,      //!< Index for the table with NR UE based on E2 Node ID
        INDEX_NODE,               //!< Index for the table with E2 Node Information
        INDEX_NODE_LOCATION,      //!< Index for the table with Node Locations
        INDEX_NODE_REGISTRATION,  //!< Index for the table with Node Registrations
        TABLE_CMM_ACTION,         //!< Table with logs of CMM actions
        TABLE_LM_ACTION,          //!< Table with logs of LM actions
        TABLE_LM_COMMAND,         //!< Table with logs of LM commamds
        TABLE_NR_GNB,            //!< Table with NR gNB information
        TABLE_NR_UE,             //!< Table with NR UE information
        TABLE_NR_UE_CELL,        //!< Table with NR UE Cell Information

        // Modificación del proyecto:
        // tablas agregadas para almacenar reportes NR usados por los LM.    
        TABLE_NR_UE_CQI,         //!< Table with NR UE CQI information
        TABLE_NR_UE_TX_BUFFER,   //!< Table with NR UE buffer status information
        TABLE_NR_UE_TX_PDU,      //!< Table with NR UE transmitted PDU information
        TABLE_NR_UE_TX_DROP,     //!< Table with NR UE dropped PDU information

        TABLE_NR_UE_RSRP_RSRQ,   //!< Table with NR UE RSRP and RSRQ Information
        TABLE_NODE,               //!< Table with E2 Node Information
        TABLE_NODE_LOCATION,      //!< Table with Node Locations
        TABLE_NODE_REGISTRATION,  //!< Table with Node Registrations
        TABLE_TERMINATOR_COMMAND, //!< Table with logs of E2 Terminator Commands
        TABLE_APPLOSS_COMMAND     //!< Table with logs of application loss Commands
    };

    /**
     * Checks that a query was executed successfully. This method checks the return codeof a query,
     * and if there was an error, the simulation is aborted.
     *
     * @param stmt The query that was executed.
     * @param rc The return code.
     * @param boundParmsStr String with the bound parameters (if any). Defaults to empty string.
     */
    virtual void CheckQueryReturnCode(sqlite3_stmt* stmt,
                                      int rc,
                                      std::string boundParmsStr = "") const;

    /**
     * Converts the bound arguments of a prepared statement into a formatted string.
     * Newer versions of sqlite3 can print prepared statements directly, but using
     * this approach ensures support for older versions.
     *
     * @param arg1 The first argument
     * @param args The remaining list of arguments.
     *
     * @return A string with the bound arguments separated with commas
     */
    template <typename T, typename... BoundArgs>
    std::string FormatBoundArgsList(T arg1, BoundArgs... args) const
    {
        std::stringstream ss;
        ss << arg1 << ", " << FormatBoundArgsList(args...);
        return ss.str();
    }

    /**
     * Template for the end case of the recursions used to convert bound arguments into a string.
     *
     * @param arg1 The last argument in the list.
     *
     * @return The provided argument as a string
     */
    template <typename T>
    std::string FormatBoundArgsList(T arg1) const
    {
        std::stringstream ss;
        ss << arg1;
        return ss.str();
    }

    /**
     * Closes the connection to the database.
     */
    virtual void CloseDb();

    void DoDispose() override;
    /**
     * Indicates if the database connection has been established.
     *
     * @return True, if the database connection is open; otherwise, false.
     */
    virtual bool IsDbOpen() const;
    /**
     * Opens the database file stores the handler. This method
     * calls InitDb to ensure that the required tables and indexes are available.
     */
    virtual void OpenDb();
    /**
     * Used to report the return code of SQL queries.
     */
    TracedCallback<std::string, std::string, int> m_queryRc;

  private:
    /**
     * Ready the database schema. This method creates the required tables and indexes.
     * If the schema already exists, no change is made, allowing for reusing existing
     * database files and extending databases created with previous simulations.
     */

    /**
     * Modificación del proyecto:
     * almacenamiento en memoria de reportes NR recientes para acceso rápido
     * desde los LM durante la simulación.
     */
    std::map<uint64_t, std::vector<NrUeBufferStatus>> m_bufferStatus;
    std::map<uint64_t, std::vector<NrUeCqi>> m_cqiSamples;
    std::map<uint64_t, std::vector<NrUeTxPdu>> m_txPduSamples;
    std::map<uint64_t, std::vector<NrUeTxDrop>> m_txDropSamples;


    void InitDb();

    /**
     * Initialize the maps with the prepared statements' strings
     */
    void InitStatements();

    /**
     * The database.
     */
    sqlite3* m_db;
    /**
     * The file path of the database.
     */
    std::string m_dbPath;
    /**
     * Map with the prepared statements' strings
     */
    std::map<StatementType, std::string> m_queryStmtsStrings;
    /**
     * Map with the table creation prepared statements' strings
     */
    std::map<CreateStatementType, std::string> m_createStmtsStrings;

    /**
     * Wrapper for the code needed to run the CREATE statements
     * It's only purpose is reduce the repeated code in the initialization
     * of the DB
     *
     * @param string The string with the SQL CREATE statement to run
     */
    void RunCreateStatement(std::string string);

}; // class OranNrDataRepositorySqlite

} // namespace ns3

#endif /* ORAN_NR_DATA_REPOSITORY_SQLITE_H */
