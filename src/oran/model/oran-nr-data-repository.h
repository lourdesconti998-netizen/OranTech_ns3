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

#ifndef ORAN_NR_DATA_REPOSITORY_H
#define ORAN_NR_DATA_REPOSITORY_H

#include "oran-command.h"  // necesario?
#include "oran-nr-near-rt-ric.h"// necesario?
#include "oran-report.h" // necesario?

#include "ns3/nstime.h"
#include "ns3/object.h"
#include "ns3/vector.h"

#include <map> // necesario?
// #include <vector> ??
#include <tuple>

namespace ns3
{

/**
 * @ingroup oran
 *
 * Base class for the Data Repository module.
 * This class defines the API for accessing the data stored by the RIC,
 * regardless of the actual implementation. This allows other models to access
 * the data without having to know the details of the storage backend.
 *
 * When the Data Repository is deactivated all the requests to store data will
 * be ignored, and all queries for stored data will return empty sets.
 *
 */
class OranNrDataRepository : public Object
{
  public:
    /**
     * Uplink buffer status sample for an NR UE.
     */
    struct NrUeBufferStatus
    {
        Time time;                //!< Time at which the sample was captured.
        uint16_t rnti;            //!< UE RNTI.
        uint8_t lcid;             //!< Logical channel identifier.
        uint32_t txQueueSize;     //!< Transmission queue size in bytes.
        uint16_t txQueueHolDelay; //!< Transmission queue HOL delay.
        uint32_t retxQueueSize;   //!< Retransmission queue size in bytes.
        uint16_t retxQueueHolDelay; //!< Retransmission queue HOL delay.
        uint16_t statusPduSize;   //!< Pending STATUS PDU size in bytes.
    };
        /**
     * CQI feedback sample for an NR UE.
     */
    struct NrUeCqi
    {
        Time time;     //!< Time at which the sample was captured.
        uint16_t rnti; //!< UE RNTI.
        uint8_t cqi;   //!< Wideband CQI.
        uint8_t mcs;   //!< MCS derived from the CQI.
        uint8_t ri;    //!< Rank indicator.
    };
     /**
     * Transmitted PDU sample for an NR UE.
     */
    struct NrUeTxPdu
    {
        Time time;     //!< Time at which the sample was captured.
        uint16_t rnti; //!< UE RNTI.
        uint8_t lcid;  //!< Logical channel identifier.
        uint64_t txPdu; //!< Transmitted PDU bytes.
    };
    /**
     * Dropped PDU sample for an NR UE.
     */
    struct NrUeTxDrop
    {
        Time time;     //!< Time at which the sample was captured.
        uint16_t rnti; //!< UE RNTI.
        uint8_t lcid;  //!< Logical channel identifier.
        uint64_t txDrop; //!< Dropped PDU bytes.
    };
  
    /**
     * Gets the TypeId of the OranNrDataRepository class.
     *
     * @return The TypeId.
     */
    static TypeId GetTypeId();
    /**
     * Creates an instance of the OranNrDataRepository class.
     */
    OranNrDataRepository();
    /**
     * The destructor of the OranNrDataRepository class.
     */
    ~OranNrDataRepository() override;
    /**
     * Activate the data storage.
     */
    virtual void Activate();
    /**
     * Deactivate the data storage.
     */
    virtual void Deactivate();
    /**
     * Check if the data storage is active.
     *
     * @return True, if the data storage is active; otherwise, false.
     */
    virtual bool IsActive() const;

    /* Data Storage API */
    /**
     * Check if a node is registered.
     *
     * @param e2NodeId The E2 Node ID.
     *
     * @return True, if the node is registered; otherwise, false.
     */
    virtual bool IsNodeRegistered(uint64_t e2NodeId) = 0;
    /**
     * Register a new node and return the E2 Node ID for that node.
     *
     * Nodes are identified by the type of node they are, and a unique ID
     * within the type (e.g. IMSI for Nr UEs).
     *
     * If the node is already registered, return the currently assigned E2 Node ID.
     * If the node is not registered, generate and store a new E2 node ID for this node.
     *
     * @param type The Node Type.
     * @param id The unique ID for this node in the simulation.
     *
     * @return The E2 Node ID for this node.
     */
    virtual uint64_t RegisterNode(OranNrNearRtRic::NodeType type, uint64_t id) = 0;
    /**
     * Register a new Nr UE node and return the E2 Node ID.
     *
     * Nr UEs are uniquely identified by their IMSI.
     *
     * If the UE is already registered, return the currently assigned E2 Node ID.
     * If the UE is not registered, generate and store a new E2 node ID for this UE.
     *
     * @param id The unique ID for this node in the simulation.
     * @param imsi The IMSI of the Nr UE.
     *
     * @return The E2 Node ID for this node.
     */
    virtual uint64_t RegisterNodeNrUe(uint64_t id, uint64_t imsi) = 0;
    /**
     * Register a new Nr gNB node and return the E2 Node ID.
     *
     * Nr gNBs are uniquely identified by their Cell ID.
     *
     * If the gNB is already registered, return the currently assigned E2 Node ID.
     * If the gNB is not registered, generate and store a new E2 node ID for this gNB.
     *
     * @param id The unique ID for this node in the simulation.
     * @param cellId The cell ID of the Nr gNB.
     *
     * @return The E2 Node ID for this node.
     */
    virtual uint64_t RegisterNodeNrGnb(uint64_t id, uint16_t cellId) = 0;
    /**
     * Deregister an E2 Node.
     *
     * If the node is not registered, do nothing.
     *
     * @param e2NodeId The E2 Node ID to deregister.
     *
     * @return The E2 Node ID that was deregistered.
     */
    virtual uint64_t DeregisterNode(uint64_t e2NodeId) = 0;
    /**
     * Store the position of a node at a given time.
     *
     * @param e2NodeId The E2 Node ID of the node.
     * @param pos The position.
     * @param t The time at which this position was reported for the node.
     */
    virtual void SavePosition(uint64_t e2NodeId, Vector pos, Time t) = 0;
    /**
     * Store the UE's connected cell information at the given time.
     *
     * @param e2NodeId The E2 Node ID of the node.
     * @param cellId The cell ID of the connected cell.
     * @param rnti The RNTI assigned to the UE by the cell.
     * @param t The time at which this cell information was reported by the node.
     */
    virtual void SaveNrUeCellInfo(uint64_t e2NodeId, uint16_t cellId, uint16_t rnti, Time t) = 0;
    /**
     * Store the UE's application packet loss.
     *
     * @param e2NodeId The E2 Node ID of the node.
     * @param appLoss The application packet loss.
     * @param t The time at which this cell information was reported by the node.
     */
    virtual void SaveAppLoss(uint64_t e2NodeId, double appLoss, Time t) = 0;
    /**
     * Store the UE's RSRP and RSRQ.
     *
     * @param e2NodeId The E2 Node ID of the node.
     * @param t The time at which this cell information was reported by the node.
     * @param rnti The RNTI assigned to the UE by the cell.
     * @param cellId The cell ID of the connected cell.
     * @param rsrp The RSRP value.
     * @param rsrq The RSRQ value.
     * @param bool isServingCell A flag that indicates if this is the serving cell.
     * @param componentCarrierId The component carrier ID.
     */
    virtual void SaveNrUeRsrpRsrq(uint64_t e2NodeId,
                                   Time t,
                                   uint16_t rnti,
                                   uint16_t cellId,
                                   double rsrp,
                                   double rsrq,
                                   bool isServingCell,
                                   uint8_t componentCarrierId) = 0;

    /* Data Access API */
    /**
     * Get all the recoreded positions of a node between two times.
     *
     * @param e2NodeId The E2 Node ID of the node.
     * @param fromTime Starting time of the interval to report.
     * @param toTime End time of the interval to report.
     * @param maxEntries Maximum number of entries to return
     *
     * @return A map with the positions of the node during the interval, indexed by time
     */
    virtual std::map<Time, Vector> GetNodePositions(uint64_t e2NodeId,
                                                    Time fromTime,
                                                    Time toTime,
                                                    uint64_t maxEntries = 1) = 0;
    /**
     * Gets the the cell information for a UE.
     *
     * @param e2NodeId The E2 Node ID.
     *
     * @return A tuple with a boolean indicating if the cell info for the UE was found, the gNB cell
     * ID, and the UE RNTI.
     */
    virtual std::tuple<bool, uint16_t, uint16_t> GetNrUeCellInfo(uint64_t e2NodeId) = 0;
    /**
     * Gets the E2 Node ID of all registered NR UEs.
     *
     * @return The collection of E2 Node IDs.
     */
    virtual std::vector<uint64_t> GetNrUeE2NodeIds() = 0;
    /**
     * Get the E2 Node ID for an Nr UE given the cell ID and RNTI of the UE in the cell.
     *
     * @param cellId The Cell ID .
     * @param rnti The RNTI of the UE.
     * @return The E2 Node ID of the Nr UE.
     */
    virtual uint64_t GetNrUeE2NodeIdFromCellInfo(uint16_t cellId, uint16_t rnti) = 0;
    /**
     * Gets the the cell information for an gNB.
     *
     * @param e2NodeId The E2 Node ID.
     *
     * @return A tuple with a boolean indicating if the cell info for the gNB was found, and the gNB
     * cell ID.
     */
    virtual std::tuple<bool, uint16_t> GetNrGnbCellInfo(uint64_t e2NodeId) = 0;
    /**
     * Gets the E2 Node ID of all registered NR gNBs.
     *
     * @return The collection of E2 Node IDs.
     */
    virtual std::vector<uint64_t> GetNrGnbE2NodeIds() = 0;
    /**
     * Gets the last time that a registration was received for all registered nodes.
     *
     * @return The collection of last registration times.
     */
    virtual std::vector<std::tuple<uint64_t, Time>> GetLastRegistrationRequests() = 0;
    /**
     * Gets the last reported application loss for a node.
     *
     * @param e2NodeId The E2 Node ID.
     * @return The application packet loss.
     */
    virtual double GetAppLoss(uint64_t e2NodeId) = 0;
    /**
     * Gets the last reported RSRP and RSRQ values.
     *
     * @param e2NodeId The E2 Node ID.
     * @return A collection of RNTI, cell ID, RSRP, RSRQ, is serving, and component carrier ID
     * tuples.
     */
    virtual std::vector<std::tuple<uint16_t, uint16_t, double, double, bool, uint8_t>>
    GetNrUeRsrpRsrq(uint64_t e2NodeId) = 0;

    /**
     * Store the NR UE uplink buffer status.
     *
     * @param e2NodeId The E2 Node ID.
     * @param status The NR UE buffer status sample.
     */
    virtual void SaveNrUeBufferStatus(uint64_t e2NodeId,
                                      const NrUeBufferStatus& status) = 0;
    /**
     * Store the NR UE CQI feedback sample.
     *
     * @param e2NodeId The E2 Node ID.
     * @param cqi The NR UE CQI sample.
     */
    virtual void SaveNrUeCqi(uint64_t e2NodeId, const NrUeCqi& cqi) = 0;
     /**
     * Store the NR UE transmitted PDU bytes sample.
     *
     * @param e2NodeId The E2 Node ID of the node.
     * @param pdu The NR UE PDU sample.
     */
    virtual void SaveNrUeTxPdu(uint64_t e2NodeId, const NrUeTxPdu& pdu) = 0;
    /**
     * Store the NR UE dropped PDU bytes sample.
     *
     * @param e2NodeId The E2 Node ID of the node.
     * @param drop The NR UE drop sample.
     */
    virtual void SaveNrUeTxDrop(uint64_t e2NodeId, const NrUeTxDrop& drop) = 0;
  
    /**
     * Get all reported NR UE buffer status samples.
     *
     * @param e2NodeId The E2 Node ID.
     * @return A vector of buffer status samples.
     */
    virtual std::vector<NrUeBufferStatus>
    GetNrUeBufferStatus(uint64_t e2NodeId) = 0;
    
    /**
     * Get all reported NR UE CQI samples.
     *
     * @param e2NodeId The E2 Node ID.
     * @return A vector of CQI samples.
     */
    virtual std::vector<NrUeCqi>
    GetNrUeCqi(uint64_t e2NodeId) = 0;
    /**
     * Get all reported NR UE transmitted PDU samples.
     *
     * @param e2NodeId The E2 Node ID of the node.
     * @return A vector of PDU samples.
     */
    virtual std::vector<NrUeTxPdu>
    GetNrUeTxPdu(uint64_t e2NodeId) = 0;
    /**
     * Get all reported NR UE dropped PDU samples.
     *
     * @param e2NodeId The E2 Node ID of the node.
     * @return A vector of drop samples.
     */
    virtual std::vector<NrUeTxDrop>
    GetNrUeTxDrop(uint64_t e2NodeId) = 0;

    /* Logging API */
    /**
     * Log a Command when it is issued by the E2 Terminator.
     *
     * @param cmd The Command.
     */
    virtual void LogCommandE2Terminator(Ptr<OranCommand> cmd) = 0;
    /**
     * Log a Command issued by a Logic Module.
     *
     * @param lm The Logic Module's name.
     * @param cmd The Command.
     */
    virtual void LogCommandLm(std::string lm, Ptr<OranCommand> cmd) = 0;
    /**
     * Log a Logic Module action.
     *
     * @param lm The Logic Module name.
     * @param logstr An string with the action to be logged.
     */
    virtual void LogActionLm(std::string lm, std::string logstr) = 0;
    /**
     * Log a Conflict Mitigation Module action.
     *
     * @param cmm The Conflict Mitigation Module name.
     * @param logstr An string with the action to be logged.
     */
    virtual void LogActionCmm(std::string cmm, std::string logstr) = 0;

  protected:
    /**
     * Disposes of the object.
     */
    void DoDispose() override;

    /**
     * Flag to keep track of the active status.
     */
    bool m_active;
}; // class OranNrDataRepository

} // namespace ns3

#endif /* ORAN_NR_DATA_REPOSITORY_H */
