############################################################################
###                                                                      ###
###    Archivo creado por estudiantes de Facultad de Ingeniería UdelaR   ###
###                                                                      ###
############################################################################
import os
import sys


# Se importan utils

THIS_DIR = os.path.dirname(os.path.abspath(__file__))
GRPC_DIR = os.path.abspath(os.path.join(THIS_DIR, "..", "grpc"))
UTILS_DIR = os.path.join(THIS_DIR, "utils")
sys.path.insert(0, GRPC_DIR)
if UTILS_DIR not in sys.path:
    sys.path.insert(0,UTILS_DIR)

import logging

# Se importan qTable y logica de estados
from utils_qlearn import QTable, load_qtable
from utils_state import QlearnSpace


# Se importan clases generadas por gRPC
import rl_data_4_qtable_pb2_grpc as pb2_grpc
from utils_grpc import (QLearnAggregator, TtiTotals, build_action_response, serve_forever, split_rntis_in_groups,)


# Se importan clases generadas para csv

from utils_csv import TtiCsvLogger

# Puerto de escucha del servidor
GRPC_ADDR = "0.0.0.0:50051"


# Modo de ejecución
EVAL_MODE = False  # True = evaluación pura | False = entrenamiento


# Modo de prueba: acción fija
# None => comportamiento normal
# 0,1,2 => fuerza siempre esa acción
FORCED_ACTION_ID = None


# Tag para nombre de archivo .npz 
CONFIG_TAG = "perue3bins_buf20k_60k_drop2k_6k"


# Learning rate.
ALPHA = 0.0001

# Discount factor.
GAMMA = 0.1


# Valores para epsilon-greedy.
EPS_START = 1.0
EPS_END = 0.05
EPS_DECAY = 0.9997     #10k iteraciones
#EPS_DECAY = 0.99985   #20k iteraciones
#EPS_DECAY = 0.99995   #60k iteraciones

###################################
### Clase principal del servidor ##
###################################
class RlData4QTableQLearnServicer(pb2_grpc.RlData4QTableServicer):

    def __init__(self):

        # Procesa las observaciones recibidas de el LM por gRPC
        self.aggregator = QLearnAggregator()
        self.prev_state_id = None
        self.prev_action_id = None
        self.prev_tti_counter = None

        # Se definen acciones posibles.
        actions = [(0.5, 0.5), (0.9, 0.1), (0.1, 0.9)]


        # Se definen espacio de estados y acciones
        self.space = QlearnSpace(n_ues=2, buffer_bin_edges_t0=20_000,buffer_bin_edges_t1=60_000,
            drop_bin_edges_t0=2_000,drop_bin_edges_t1=6_000,actions=actions,)

        # Se crea QTable
        self.qtable = QTable(n_states=self.space.n_states,n_actions=self.space.n_actions,
            alpha=ALPHA,gamma=GAMMA,eps_start=EPS_START,eps_end=EPS_END,eps_decay=EPS_DECAY,)

        # Se crea directorio y archivo de chechpoint
        ckpt_dir = os.path.join(THIS_DIR, "checkpoints")
        os.makedirs(ckpt_dir, exist_ok=True)

        self.qtable_path = os.path.join(ckpt_dir, f"qtable_{CONFIG_TAG}.npz")

        # Verificacion de archivo existente
        if os.path.exists(self.qtable_path):
            try:
                loaded = load_qtable(self.qtable_path)
                if (
                    loaded.n_states == self.qtable.n_states
                    and loaded.n_actions == self.qtable.n_actions
                ):
                    self.qtable = loaded
                    logging.info("Q-table cargada desde %s", self.qtable_path)
                else:
                    logging.warning(
                        "Checkpoint incompatible en shape: esperado=(%d,%d), encontrado=(%d,%d). Se ignora.",
                        self.qtable.n_states,self.qtable.n_actions,
                        loaded.n_states,loaded.n_actions,)
            except Exception:
                logging.exception("Error cargando checkpoint desde %s", self.qtable_path)
        else:
            logging.info("No hay checkpoint previo para esta configuración")

        # Verifica si es modo evaluación o fuerzo una acción.
        if EVAL_MODE:
            self.qtable.eps = 0.0
            logging.info("Modo evaluación activado: epsilon forzado a 0.0")
        else:
            logging.info("Modo entrenamiento activado: eps=%.6f, eps_end=%.6f, decay=%.6f",
                        self.qtable.eps,self.qtable.eps_end,self.qtable.eps_decay,)


        # Máximos para normalización
        self.max_buffer = 100000
        self.max_pdu = 30000
        self.max_drop = 7200


        # Genero un historico de TxBuffer previo
        self.prev_txbuffer_by_ue = {}
        self.have_prev_buffer = False

        # Guardado de métricas en .csv
        self.csv_logger = TtiCsvLogger(THIS_DIR)
        logging.info("QLearn log file: %s", self.csv_logger.path)


        # Guarda estado actual y los datos traidos por gRPC
        self.curr_totals = None
        self.curr_state_id = None

        # Parametros de guardado de QTable
        self._save_every = 500
        self._updates = 0

    def PushObservation(self, request, context):
        # gRPC llama a esta funcion cada vez que le llega una observación nueva. 
        # en este punto se genera cada iteración.

        # Transfomo la observación en una clase TtiTotals
        totals = self.aggregator.aggregate(request)

        # Calculo el vector de estado a partir de la nueva observación.
        state_id = self.compute_state_id(totals)

        # Muestro las metricas del TTI
        self.log_totals(totals)

        # Como es una nueva observación vuelvo a tener las variables de interes en None
        reward = None
        td_error = None

        # Verifico si ya hubo una transición.
        hubo_paso_anterior = (self.prev_state_id is not None and self.prev_action_id is not None and self.prev_tti_counter is not None)

        if hubo_paso_anterior and totals.tti_counter > self.prev_tti_counter:
            # Calculo el reward
            reward = self.reward(totals)
            logging.info("Reward=%f", reward)
            # Calculo td_error
            td_error = self.on_transition(self.prev_state_id,self.prev_action_id,
                reward,state_id,)

        #Elijo una acción y a partir de esa acción los shares
        action_id = self.select_action(state_id)
        ue_shares = self.shares_from_action(totals, action_id)

        # Guardo las metricas en un csv
        self.csv_logger.append(totals=totals,state_id=state_id,
             action_id=action_id,ue_shares=ue_shares,reward=reward,td_error=td_error,)

        # Acutualizo los datos con la nueva observación. 
        self.after_observation(totals)
        self.prev_state_id = state_id
        self.prev_action_id = action_id
        self.prev_tti_counter = totals.tti_counter

        # Muestro en consola el resumen de los datos obtenidos de esta iteración.
        self.log_step_summary(totals, state_id, action_id, td_error)
        return build_action_response(ue_shares, action_id)

    ############################
    ### Funciones auxiliares ###
    ############################
    def log_totals(self, totals):
        logging.info("Iteración %s | n_ues=%d | txbuffer_by_ue=%s | txdrop_by_ue=%s | txpdu_by_ue=%s",
            totals.tti_counter,len(set(totals.txbuffer_by_ue)| set(totals.txdrop_by_ue)| set(totals.txpdu_by_ue)),
            totals.txbuffer_by_ue,totals.txdrop_by_ue,totals.txpdu_by_ue,)

    def log_step_summary(self, totals, state_id, action_id, td_error):
        if td_error is None:
            td_error_value = "NaN"
        else:
            td_error_value = f"{td_error:.6f}"

#########################################
###  Hooks (meétodos principales)     ###
#########################################

    # Función que determina estado según métricas.
    def compute_state_id(self, totals):
        s_id = self.space.encode_state_from_totals(totals)
        self.curr_totals = totals
        self.curr_state_id = s_id
        return s_id

    # Función que calcula reward
    def reward(self, totals):
        # Variables para el calculo del reward
        a = 0.8
        b = 0.2

        if not self.have_prev_buffer:
            return 0.0

        #Obtengo las rntis de las UEs para las cuales tengo metricas
        rntis = (set(totals.txdrop_by_ue)| set(totals.txpdu_by_ue)| set(totals.txbuffer_by_ue))

        if not rntis:
            return 0.0

        costs = []

        for r in rntis:
            drop = float(totals.txdrop_by_ue.get(r, 0))
            pdu = float(totals.txpdu_by_ue.get(r, 0))
            buf = float(self.prev_txbuffer_by_ue.get(r, 0))

            # Normalizo.
            drop_n = min(drop / self.max_drop, 1.0)
            pdu_n = min(pdu / self.max_pdu, 1.0)
            buf_n = min(buf / self.max_buffer, 1.0)

            # Calculo de costo por UE.
            c_i = a * drop_n + b * (buf_n - pdu_n)
            costs.append(c_i)

        #Calculo el reward promedio.
        reward = -(sum(costs) / len(costs))
        return reward

    # Funcion que actualiza TxBufer para usarlo en el próximo TTI
    def after_observation(self, totals):
        self.prev_txbuffer_by_ue = dict(totals.txbuffer_by_ue)
        self.have_prev_buffer = True

    # Función para transición de estado y actualiza QTable.
    def on_transition(self, prev_s, prev_a, reward, s2):
        # En evaluación no se entrena
        if EVAL_MODE:
            return None

        td = self.qtable.update(s_id=prev_s, a_id=prev_a, reward=reward, s2_id=s2,)

        self._updates += 1
        if self._updates % self._save_every == 0:
            self.save_checkpoint()

        return td


    # Función que guarda QTable
    def save_checkpoint(self):
        # En evaluación no se guarda
        if EVAL_MODE:
            return

        try:
            self.qtable.save(self.qtable_path)
            logging.info("Q-table guardada en %s", self.qtable_path)
        except Exception:
            logging.exception("No se pudo guardar la Q-table en %s", self.qtable_path)


    # Función para seleccionar que acción tomar
    def select_action(self, state_id):
        # En evaluación: greedy puro
        if FORCED_ACTION_ID is not None:
            return int(FORCED_ACTION_ID)
        if EVAL_MODE:
            return self.qtable.best_action(state_id)

        # En entrenamiento: epsilon-greedy con decay
        return int(self.qtable.select_action(state_id, do_decay=True))

    # Función que transforma acción en shares
    def shares_from_action(self, totals: TtiTotals, action_id):
        ue_rntis = sorted(totals.txbuffer_by_ue.keys())

        if not ue_rntis:
            return {}

        group_shares = self.space.actions[action_id]

        ue_shares = {}

        for rnti, share in zip(ue_rntis, group_shares):
            ue_shares[rnti] = float(share)

        # Normalizar por seguridad
        s = float(sum(ue_shares.values()))
        if s > 0.0:
            for rnti in list(ue_shares.keys()):
                ue_shares[rnti] /= s

        return ue_shares


# Arranque del servidor
def main():
    # Creo el servicer para que la LM se conecte con el agente por gRPC
    servicer: pb2_grpc.RlData4QTableServicer = RlData4QTableQLearnServicer()
    # Incio el servidor y se queda escuchando requests
    serve_forever(servicer, GRPC_ADDR, max_workers=4)


if __name__ == "__main__":
    # Configuro Logs
    logging.basicConfig(level=logging.INFO,format="%(message)s",)
    # Inicio servidor 
    main()