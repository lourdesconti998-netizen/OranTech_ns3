build_lib(
  LIBNAME oran
  SOURCE_FILES
    helper/oran-helper.cc
    helper/oran-nr-monitoring-helper.cc
    model/oran-cmm.cc
    model/oran-cmm-noop.cc
    model/oran-nr-data-repository-sqlite.cc
    model/oran-command.cc
    model/oran-command-nr-2-nr-handover.cc
    model/oran-command-nr-ul-rbg-targets.cc
    model/oran-e2-node-terminator.cc
    model/oran-e2-node-terminator-container.cc
    model/oran-e2-node-terminator-wired.cc
    model/oran-e2-node-terminator-nr-gnb.cc
    model/oran-e2-node-terminator-nr-ue.cc
    model/oran-lm.cc
    model/oran-lm-noop.cc
    model/oran-nr-data-repository.cc
    model/oran-nr-near-rt-ric.cc
    model/oran-nr-near-rt-ric-e2terminator.cc
    model/oran-query-trigger.cc
    model/oran-report.cc
    model/oran-report-apploss.cc
    model/oran-report-location.cc
    model/oran-report-nr-ue-cell-info.cc
    model/oran-report-nr-ue-cqi.cc
    model/oran-report-nr-ue-rsrp-rsrq.cc
    model/oran-report-nr-ue-tx-drop.cc
    model/oran-report-nr-ue-tx-pdu.cc
    model/oran-report-trigger.cc
    model/oran-report-buffer-status.cc
    model/oran-reporter-nr-ue-cell-info.cc
    model/oran-reporter-nr-ue-cqi.cc
    model/oran-reporter-nr-ue-rsrp-rsrq.cc
    model/oran-reporter-nr-ue-tx-drop.cc
    model/oran-reporter-nr-ue-tx-pdu.cc
    model/oran-report-trigger-periodic.cc
    model/oran-reporter.cc
    model/oran-reporter-location.cc
    model/oran-reporter-buffer-status.cc
    model/oran-lm-nr-2-nr-moni.cc
    model/oran-lm-nr-rl-qlearn.cc
    model/oran-lm-nr-rl-dqn.cc
    # gRPC generated sources (desde src/grpc/)
    ../grpc/rl_data_4_qtable.pb.cc
    ../grpc/rl_data_4_qtable.grpc.pb.cc
    model/oran-reporter-apploss.cc
    model/oran-report-trigger-nr-ue-handover.cc
  HEADER_FILES
    helper/oran-helper.h
    helper/oran-nr-monitoring-helper.h
    model/oran-cmm.h
    model/oran-cmm-noop.h
    model/oran-nr-data-repository-sqlite.h
    model/oran-command.h
    model/oran-command-nr-2-nr-handover.h
    model/oran-command-nr-ul-rbg-targets.h
    model/oran-e2-node-terminator.h
    model/oran-e2-node-terminator-container.h
    model/oran-e2-node-terminator-wired.h
    model/oran-e2-node-terminator-nr-gnb.h
    model/oran-e2-node-terminator-nr-ue.h
    model/oran-lm.h
    model/oran-lm-noop.h
    model/oran-nr-data-repository.h
    model/oran-nr-near-rt-ric.h
    model/oran-nr-near-rt-ric-e2terminator.h
    model/oran-query-trigger.h
    model/oran-report.h
    model/oran-report-apploss.h
    model/oran-report-location.h
    model/oran-report-nr-ue-cell-info.h
    model/oran-report-nr-ue-cqi.h
    model/oran-report-nr-ue-rsrp-rsrq.h
    model/oran-report-trigger.h
    model/oran-report-nr-ue-tx-drop.h
    model/oran-report-nr-ue-tx-pdu.h
    model/oran-report-buffer-status.h
    model/oran-reporter-nr-ue-cell-info.h
    model/oran-reporter-nr-ue-cqi.h
    model/oran-reporter-nr-ue-rsrp-rsrq.h
    model/oran-reporter-nr-ue-tx-drop.h
    model/oran-reporter-nr-ue-tx-pdu.h
    model/oran-report-trigger-periodic.h
    model/oran-reporter.h
    model/oran-reporter-location.h
    model/oran-lm-nr-2-nr-moni.h    
    model/oran-lm-nr-rl-qlearn.h
    model/oran-lm-nr-rl-dqn.h
    ../grpc/rl_data_4_qtable.pb.h
    ../grpc/rl_data_4_qtable.grpc.pb.h
    model/oran-reporter-apploss.h
    model/oran-reporter-buffer-status.h
    model/oran-report-trigger-nr-ue-handover.h
    model/oran-report-buffer-status.h
  LIBRARIES_TO_LINK
    ${libcore}
    ${libnetwork}
    ${libmobility}
    ${libinternet}
    ${libnr}         # lbreria NR-LENA
)

# gRPC / Protobuf (C++) para el módulo oran
find_package(PkgConfig REQUIRED)

# Crea targets CMake “limpios” (evita flags raros dentro de build_lib)
pkg_check_modules(GRPCPP REQUIRED IMPORTED_TARGET grpc++)
pkg_check_modules(PROTOBUF REQUIRED IMPORTED_TARGET protobuf)

# Para encontrar test.grpc.pb.h (está en src/grpc)
target_include_directories(${liboran} PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}/../grpc
)

# Link contra gRPC++ y protobuf (incluye absl y deps donde corresponda)
target_link_libraries(${liboran} PRIVATE
  PkgConfig::GRPCPP
  PkgConfig::PROTOBUF
)
