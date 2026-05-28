import logging
import time

from concurrent import futures

import grpc

import rl_data_4_qtable_pb2 as pb2
import rl_data_4_qtable_pb2_grpc as pb2_grpc


#Clase para guardar datos extraidos de gRPC
class TtiTotals:
    def __init__(self, tti_counter,txbuffer_by_ue=None, txdrop_by_ue=None, txpdu_by_ue=None,total_txbuffer=0,total_txdrop=0):
        # Inicializo variables.
        self.tti_counter =tti_counter
        
        if txdrop_by_ue is None:
            self.txdrop_by_ue = {}
        else:
            self.txdrop_by_ue = txdrop_by_ue 

        if txpdu_by_ue is None:
            self.txpdu_by_ue = {}
        else:
            self.txpdu_by_ue = txpdu_by_ue
        if txbuffer_by_ue is None:
            self.txbuffer_by_ue = {}
        else:
            self.txbuffer_by_ue = txbuffer_by_ue
        
        
        self.total_txbuffer = total_txbuffer
        self.total_txdrop = total_txdrop


class QLearnAggregator:

    def __init__(self):
        # Guardo los datos por iteración.
        self.by_tti= {}

    # La funcion agregate junta las observaciones crudas (request) y las garda
    def aggregate(self, request):
        
        tti_counter = int(request.tti_counter)

        # Genero diccionarios para acumular metricas de cada UE
        txdrop_by_ue ={}
        txpdu_by_ue={}
        txbuffer_by_ue ={}

        # Recorro las UEs y acumulo las metricas recibidas
        for ue in request.ues:
            rnti = int(ue.rnti)
            txdrop = int(ue.txdrop)
            txbuffer = int(ue.txbuffer)
            txpdu = int(ue.txpdu)

            #Si el rnti no esta en los diccionarios arranco la suma en cero
            if rnti not in txdrop_by_ue:
                txdrop_by_ue[rnti] = 0
            if rnti not in txpdu_by_ue:
                txpdu_by_ue[rnti] = 0
            if rnti not in txbuffer_by_ue:
                txbuffer_by_ue[rnti] = 0

            txdrop_by_ue[rnti] += txdrop
            txpdu_by_ue[rnti] += txpdu
            txbuffer_by_ue[rnti] += txbuffer
        # Genero totals para agrupar las metricas de la iteración.
        totals = TtiTotals(tti_counter=tti_counter,
            txdrop_by_ue=dict(txdrop_by_ue),txpdu_by_ue=dict(txpdu_by_ue),
            txbuffer_by_ue=dict(txbuffer_by_ue),total_txbuffer=sum(txbuffer_by_ue.values()),
            total_txdrop=sum(txdrop_by_ue.values()),)
        # Guardo los datos de la iteración
        self.by_tti[totals.tti_counter] = totals
        return totals


def split_rntis_in_groups(rntis, n_groups=2):
    # Separo los Ues en dos grupos.
    lim = len(rntis) // 2
    group_0 = rntis[:lim] # Grupo de UEs con mas buffer
    group_1 = rntis[lim:]

    return [group_0, group_1]

def build_action_response(ue_shares, action_id):
    # Genero la respuesta del agente a la LM mediante gRPC.
    resp = pb2.ActionResponse()
    lista = sorted(ue_shares.items())
    # Devuelvo por ue su rnti, la cantidad de shares y el action id
    for rnti, share in lista:
        item = resp.ue_shares.add()
        item.rnti = int(rnti)
        item.share = float(share)
    resp.action_id = int(action_id)
    return resp


def rntis_from_totals(totals):
    #Saco los rntis que aparecen en las metricas de interes.
    return set(totals.txdrop_by_ue) | set(totals.txpdu_by_ue) | set(totals.txbuffer_by_ue)

    
# funciones del servidor grpc
def make_server(servicer,grpc_addr, max_workers= 4,):
    # Creo el servidor gRPC
    server = grpc.server(futures.ThreadPoolExecutor(max_workers=max_workers))
    pb2_grpc.add_RlData4QTableServicer_to_server(servicer, server)
    server.add_insecure_port(grpc_addr) #Abro el servidor en la direccion que escucha grpc.
    return server


def serve_forever(servicer, grpc_addr, sleep_s=1.0, max_workers=4):
    # Creo el servidor.
    server = make_server(servicer, grpc_addr, max_workers=max_workers)
    # Inicio el servidor.
    server.start()
    logging.info("gRPC server listening on %s", grpc_addr)

    # El servidor queda corriendo hasta que el usuario lo pare
    try:
        while True:
            time.sleep(sleep_s)
    except KeyboardInterrupt:
        logging.info("Stopping gRPC server")
        server.stop(0)