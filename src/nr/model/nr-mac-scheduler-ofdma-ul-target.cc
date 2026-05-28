////////////////////////////////////////////////////////////////////
//// Copyright (c) 2019 Centre Tecnologic de Telecomunicacions  ////
//// de Catalunya (CTTC).                                      ////
////                                                            ////
//// Este archivo fue desarrollado a partir del scheduler        ////
//// nr-mac-scheduler-ofdma-rr, perteneciente al modulo NR LENA, ///
//// realizado por el CTTC                                      ////
//// Esta versiós, modificada por estudiantes de la udelar      ////
/// mantiene la lógica base del scheduler                       ////
//// Round Robin, pero se modifica la asignación uplink para     ////
//// permitir asignaciones de RBGs por UE definidos externamente.   ////
////                                                            ////
//// SPDX-License-Identifier: GPL-2.0-only                       ////
////////////////////////////////////////////////////////////////////
#include "nr-mac-scheduler-ofdma-ul-target.h"

#include "ns3/log.h"

#include <algorithm>
#include <functional>
#include <set>

namespace ns3
{

NS_LOG_COMPONENT_DEFINE("NrMacSchedulerOfdmaUlTarget");

NS_OBJECT_ENSURE_REGISTERED(NrMacSchedulerOfdmaUlTarget);

TypeId
NrMacSchedulerOfdmaUlTarget::GetTypeId()
{
    static TypeId tid = TypeId("ns3::NrMacSchedulerOfdmaUlTarget")
                            // Se define este scheduler como una clase derivada del Round Robin original
                            .SetParent<NrMacSchedulerOfdmaRR>()
                            .AddConstructor<NrMacSchedulerOfdmaUlTarget>();
    return tid;
}

NrMacSchedulerOfdmaUlTarget::NrMacSchedulerOfdmaUlTarget(): NrMacSchedulerOfdmaRR()
{}
// Guarda los objetivos de asignación uplink definidos externamente.
// Cada entrada asocia un RNTI con la cantidad de RBGs objetivo para ese UE.

void
NrMacSchedulerOfdmaUlTarget::SetUlTargets(const std::unordered_map<uint16_t, uint32_t>& targets)
{
    NS_LOG_FUNCTION(this << targets.size());
    m_ulTargets = targets;
}
// Verifica si un UE ya alcanzó su objetivo de RBGs uplink.
// Si el UE no tiene un objetivo definido, se mantiene el comportamiento base
// y se permite que continúe recibiendo recursos.
bool
NrMacSchedulerOfdmaUlTarget::HasReachedUlTarget(const UePtrAndBufferReq& ue,
                                                uint32_t beamSym) const
{
    if (beamSym == 0)
    {
        return true;
    }

    const auto& ueInfo = ue.first;
    auto it = m_ulTargets.find(ueInfo->m_rnti);
    if (it == m_ulTargets.end())
    {
        return false;
    }

    uint32_t assignedLogicalRbg = ueInfo->m_ulRBG.size() / beamSym;
    return assignedLogicalRbg >= it->second;
}
// Asigna los RBGs uplink manteniendo la lógica base del scheduler RR,
// pero incorporando los objetivos de RBGs definidos externamente para cada UE.
NrMacSchedulerNs3::BeamSymbolMap
NrMacSchedulerOfdmaUlTarget::AssignULRBG(uint32_t symAvail, const ActiveUeMap& activeUl) const
{
    NS_LOG_FUNCTION(this);

    NS_LOG_DEBUG("# beams active flows: " << activeUl.size() << ", # sym: " << symAvail);

    //      Cuento los rbg
        static bool printed = false;
    if (!printed)
    {
        uint32_t B = 0;
        const auto& bm = GetUlBitmask();
        for (bool b : bm)
        {
            if (b) B++;
        }
        NS_LOG_UNCOND("[UL-SCHED] Logical UL RBG available (B) = " << B
                      << "  (bitmask size=" << bm.size() << ")");
        printed = true;
    }
    //

    GetFirst GetBeamId;
    GetSecond GetUeVector;
    BeamSymbolMap symPerBeam = GetSymPerBeam(symAvail, activeUl);

    // Itero sobre los diferentes beams
    for (const auto& el : activeUl)
    {
        uint32_t beamSym = symPerBeam.at(GetBeamId(el));
        std::vector<UePtrAndBufferReq> ueVector;
        FTResources assigned(0, 0);

        const std::vector<bool> availableRbgs = GetUlBitmask();
        std::set<uint32_t> remainingRbgSet;
        for (size_t i = 0; i < availableRbgs.size(); i++)
        {
            if (availableRbgs.at(i))
            {
                remainingRbgSet.emplace(i);
            }
        }

        NS_ASSERT(!remainingRbgSet.empty());

        for (const auto& ue : GetUeVector(el))
        {
            ueVector.emplace_back(ue);
        }

        for (auto& ue : ueVector)
        {
            //en el UL original esto es así 
            BeforeUlSched(ue, FTResources(beamSym * beamSym, beamSym));
        }

        while (!remainingRbgSet.empty())
        {
            if (m_activeUlAi)
            {
                CallNotifyUlFn(ueVector);
            }

            GetFirst GetUe;
            SortUeVector(&ueVector,
                         std::bind(&NrMacSchedulerOfdmaUlTarget::GetUeCompareUlFn, this));

            auto schedInfoIt = ueVector.begin();

            // Buscar el próximo UE elegible
            while (schedInfoIt != ueVector.end())
            {
                auto uePtr = GetUe(*schedInfoIt);
                uint32_t bufQueueSize = schedInfoIt->second;

                //  si ya cubrió buffer, skip
                if (uePtr->m_ulTbSize >= std::max(bufQueueSize, 12U))
                {
                    std::advance(schedInfoIt, 1);
                    continue;
                }

                // target UL alcanzado, skip
                if (HasReachedUlTarget(*schedInfoIt, beamSym))
                {
                    auto itT = m_ulTargets.find(uePtr->m_rnti);
                    uint32_t target = (itT == m_ulTargets.end()) ? 0 : itT->second;
                    uint32_t logicalAssigned = (beamSym == 0) ? 0 : static_cast<uint32_t>(uePtr->m_ulRBG.size() / beamSym);

                    double t = Simulator::Now().GetSeconds();

                    std::advance(schedInfoIt, 1);
                    continue;
                }  

                break; // elegible
            }

            // Si todos están cubiertos o alcanzaron target, no hay más que asignar en este beam
            if (schedInfoIt == ueVector.end())
            {
                break;
            }

            // Asignar 1 RBG (idéntico al UL original)
            auto assignedRbg = remainingRbgSet.begin();

            auto& assignedRbgs = GetUe(*schedInfoIt)->m_ulRBG;
            auto existingRbgs = assignedRbgs.size();
            assignedRbgs.resize(assignedRbgs.size() + beamSym);
            std::fill(assignedRbgs.begin() + existingRbgs, assignedRbgs.end(), *assignedRbg);
            assigned.m_rbg++;

            auto& assignedSymbols = GetUe(*schedInfoIt)->m_ulSym;
            auto existingSymbols = assignedSymbols.size();
            assignedSymbols.resize(assignedSymbols.size() + beamSym);
            std::iota(assignedSymbols.begin() + existingSymbols, assignedSymbols.end(), 0);
            assigned.m_sym = beamSym;

            remainingRbgSet.erase(assignedRbg);

            {
            GetFirst GetUe;
            auto uePtr = GetUe(*schedInfoIt);
            auto itT = m_ulTargets.find(uePtr->m_rnti);
            bool hasTarget = (itT != m_ulTargets.end());
            uint32_t target = hasTarget ? itT->second : 999999;
            uint32_t logicalAssigned = (beamSym == 0) ? 0 : static_cast<uint32_t>(uePtr->m_ulRBG.size() / beamSym);
            double t = Simulator::Now().GetSeconds();

            NS_LOG_DEBUG("Assigned " << assigned.m_rbg << " UL RBG, spanned over " << beamSym
                                     << " SYM, to UE " << GetUe(*schedInfoIt)->m_rnti);
            AssignedUlResources(*schedInfoIt, FTResources(beamSym, beamSym), assigned);
            }

            // Update metrics for UEs no asignados (igual que original)
            for (auto& ue : ueVector)
            {
                if (GetUe(ue)->m_rnti != GetUe(*schedInfoIt)->m_rnti)
                {
                    NotAssignedUlResources(ue, FTResources(beamSym, beamSym), assigned);
                }
            }
        }
    }

    return symPerBeam;
}

} // namespace ns3
