////////////////////////////////////////////////////////////////////
// Copyright (c) 2019 Centre Tecnologic de Telecomunicacions      //
// de Catalunya (CTTC).                                          //
//                                                                //
// Este archivo fue desarrollado a partir del scheduler           //
// nr-mac-scheduler-ofdma-rr, perteneciente al módulo 5G-LENA     //
// de ns-3.                                                       //
//                                                                //
// Esta versión, modificada en el marco del proyecto, mantiene    //
// la lógica base del scheduler OFDMA Round Robin, pero extiende  //
// su comportamiento en uplink para permitir definir un objetivo  //
// máximo de RBGs por UE en cada slot.                            //
//                                                                //
//                                                                //
// SPDX-License-Identifier: GPL-2.0-only                         //
////////////////////////////////////////////////////////////////////
#pragma once

#include "nr-mac-scheduler-ofdma-rr.h"

#include <unordered_map>
#include <fstream>

namespace ns3
{


class NrMacSchedulerOfdmaUlTarget : public NrMacSchedulerOfdmaRR
{
  public:
    /**
     * @brief GetTypeId
     * @return The TypeId of the class
     */
    static TypeId GetTypeId();

    /**
     * @brief Constructor
     */
    NrMacSchedulerOfdmaUlTarget();

    /**
     * @brief Destructor
     */
    ~NrMacSchedulerOfdmaUlTarget() override = default;


    void SetUlTargets(const std::unordered_map<uint16_t, uint32_t>& targets);

  protected:
    BeamSymbolMap AssignULRBG(uint32_t symAvail, const ActiveUeMap& activeUl) const override;

  private:
    bool HasReachedUlTarget(const UePtrAndBufferReq& ue, uint32_t beamSym) const;

    std::unordered_map<uint16_t, uint32_t> m_ulTargets;
};

} // namespace ns3
