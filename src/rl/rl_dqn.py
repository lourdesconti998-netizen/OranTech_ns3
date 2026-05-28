############################################################################
###    Archivo creado por estudiantes de Facultad de Ingeniería UdelaR   ###
###    tomando como base otro archivo de los mismos autores              ###
###        (rl_qlearn.py)                                                ###
############################################################################
import os
import sys

# Rutas
THIS_DIR = os.path.dirname(os.path.abspath(__file__))
# Ruta a la carpeta de utils.
UTILS_DIR = os.path.join(THIS_DIR, "utils")
if UTILS_DIR not in sys.path:
    sys.path.append(UTILS_DIR)

# Ruta a la carpeta de grpc
GRPC_DIR = os.path.abspath(os.path.join(THIS_DIR, ".." ,"grpc"))
if GRPC_DIR not in sys.path:
    sys.path.append(GRPC_DIR)


import logging
import math

import glob
import numpy as np

import rl_data_4_qtable_pb2_grpc as pb2_grpc

from utils_dqn import DQNAgent, load_dqn_agent
from utils_state import DQNSpace
from utils_grpc import (QLearnAggregator,TtiTotals, build_action_response,rntis_from_totals, serve_forever,split_rntis_in_groups,)
from utils_csv import DqnCsvLogger

# Parámetros GRPC
GRPC_ADDR = "0.0.0.0:50051"

# Parámetros para DQN.
GAMMA = 0.1
LR = 1e-4 #alpha


# Cada cuantos pasos se actualiza la red target.
TARGET_UPDATE_EVERY = 500

BATCH_SIZE = 256
REPLAY_CAPACITY = 10000

# Tamaño minimo del replay buffer para empezar a entrenar.
MIN_REPLAY_SIZE_FOR_TRAINING = 1000

# Parámetros para epsilon-greedy
EPS_START = 1.0
EPS_END = 0.05
EPS_DECAY = 0.9997 #9984 interaciones para llegar al minimo

# Parámetros para el costo
const_a = 0.2
const_b = 0.3
const_y = 1
const_x = 1

# Tamaño de la ventana para calcular metricas promediadas
METRICS_WINDOW = 1000

# Cada cuantos TTIs se muestra en consola las metricas
LOG_EVERY_TTI = 10000

#Directorio para guardar las redes neuronales entrenadas
CHECKPOINT_DIR = os.path.join(THIS_DIR, "checkpoints_nn")


def get_latest_checkpoint(dir_path = CHECKPOINT_DIR, pattern= "dqn_*.pt"):
    # Busco si existe una red entrenada previamente, si la tengo la uso.
    files = glob.glob(os.path.join(dir_path, pattern))
    if not files:
        return None
    #return max(files, key=os.path.getmtime) por ahora no voy a entrenar sobre un modelo entrenado igual le faltan guardar datos al .pt
    return None

# Servicer de grpc, para conectar las observaciones dadas por la LM con el agente
class RlData4QTableDqnServicer(pb2_grpc.RlData4QTableServicer):
    def __init__(self,const_a = const_a,const_b = const_b, const_x = const_x, const_y = const_y,METRICS_WINDOW = 200):
        # 
        self.aggregator = QLearnAggregator()

        # Las variables de estados previos las inicializo en None
        self.prev_state_id = None
        self.prev_action_id = None
        self.prev_tti_counter = None
      
        # Creo el espacio de estados y acciones
        self.build_state_space()

        # busco si hay una red ya entrenada si no inicializo una
        checkpoint_path = get_latest_checkpoint()
        if checkpoint_path:
            self.agent = load_dqn_agent(checkpoint_path)
        else:
            self.agent = DQNAgent(state_dim=self.space.state_dim,
            n_actions=self.space.n_actions,gamma=GAMMA,lr=LR,
            eps_start=EPS_START,eps_end=EPS_END,eps_decay=EPS_DECAY,
            buffer_capacity=REPLAY_CAPACITY, batch_size=BATCH_SIZE,
            target_update_every=TARGET_UPDATE_EVERY,checkpoint_dir=CHECKPOINT_DIR,)

        # Guardo constantes para calcular el reward.
        self.init_reward_parameters(const_a, const_b, const_x, const_y)

        self.prev_txbuffer_by_ue = {}
        self.have_prev_buffer = False

        # inicializo logger y variables para seguimiento de metricas
        self.init_loggers()
        self.init_tracking(METRICS_WINDOW)

    def PushObservation(self, request, context):
        # gRPC llama a esta funcion cada vez que le llega una observación nueva. 
        # en este punto se genera cada iteración.

        # Transfomo la observación en una clase TtiTotals
        totals = self.aggregator.aggregate(request)

        # Calculo el vector de estado a partir de la nueva observación.
        state_id = self.compute_state_id(totals)

        # Como es una nueva observación vuelvo a tener las variables de interes en None
        reward = None
        td_error = None

        # Me fijo que no sea la primer iteración
        hubo_paso_anterior = (self.prev_state_id is not None and self.prev_action_id is not None and self.prev_tti_counter is not None)

        if hubo_paso_anterior and totals.tti_counter > self.prev_tti_counter:

            #Calculo el reward
            reward = self.reward(totals)
            logging.info("Reward=%f", reward)
            # Entreno la red y obtengo td error
            td_error = self.on_transition(self.prev_state_id,self.prev_action_id,reward, state_id)

        # Selecciono la accion y los recursos a asignar
        action_id = self.select_action(state_id)
        ue_shares = self.shares_from_action(totals, action_id)

        self.after_observation(totals)

        # Guardo el estado actual, asi me queda como estado previo para la siguiente iteración
        self.prev_state_id = state_id
        self.prev_action_id = action_id
        self.prev_tti_counter = totals.tti_counter

        return build_action_response(ue_shares, action_id)

    def build_state_space(self):
        # Defino las acciones posibles que puede tomar el agente
        #las acciones son porcentajes de RBG's a asignar
        actions = [(0.5, 0.5), (0.9, 0.1),(0.1, 0.9),]
        
        # Defino los limites de los Bins para buffer y drops
        buffer_bin_edges = [0, 20000,60000, float("inf"),]
        drop_bin_edges = [0,2000,6000, float("inf"),]

        # Defino el espacio de estados
        self.space = DQNSpace(actions=actions,buffer_bin_edges=buffer_bin_edges,drop_bin_edges=drop_bin_edges, buf_norm=100000.0, drop_norm=7200.0, pdu_norm=30000.0,)

    def get_active_rntis(self,totals):
        # Obtengo las rntis de las UEs activas.
        rntis= rntis_from_totals(totals)
        return list(rntis)

    def init_reward_parameters(self, const_a, const_b, const_x, const_y):
        # Inicializo las constantes del reward
        self.const_a = const_a
        self.const_b = const_b
        self.const_x = const_x
        self.const_y = const_y

        #constantes de normalización
        self.max_buffer = 100000
        self.max_pdu = 30000
        self.max_drop = 7200


    def init_loggers(self):
        #Inicializo la clase logger la cual guarda las metricas en un .csv
        self.dqn_csv_logger = DqnCsvLogger(THIS_DIR)
        logging.info("DQN log file:" + " " + self.dqn_csv_logger.path)
    def init_tracking(self, METRICS_WINDOW):
        # Inicializo variables de tiempos anteriores  y actuales en None
        self.curr_totals = None
        self.curr_state_vec = None

        self.last_reward = None
        self.last_td_error = None
        self.last_loss = None
        self.last_action = None
        self.last_ue_shares = {}


    # Genero el vector de estado
    def compute_state_id(self, totals):
        state_vec = self.space.encode_state_from_totals(totals)

        #Guardo el vector y los datos actuales traidos de grpc.
        self.curr_totals = totals
        self.curr_state_vec = state_vec

        return state_vec

    # Calculo Reward
    def reward(self, totals):

        if not self.have_prev_buffer:
            self.last_reward = 0.0
            return 0.0
        
        rntis = self.get_active_rntis(totals)

        n_rntis = len(rntis)
        if n_rntis == 0:
            self.last_reward = 0.0
            return 0.0

        # Calculo el costo promedio de las UEs activas.
        cost_avg = self.compute_average_cost(totals,rntis)

        reward = -cost_avg

        #Guardo el reward
        self.last_reward = reward

        return reward

    def compute_average_cost(self, totals,rntis):
        # Calculo el costo promedio de las UEs activas.
        suma_costos = 0.0
        for rnti in rntis:
            costo = self.get_ue_cost(totals,rnti)
            suma_costos += costo
        avg = suma_costos /len(rntis)
        return  avg
    def get_cost_variables(self, totals, rnti):
        # Obtengo las features actuales usadas en el costo.
        drop_actual = float(totals.txdrop_by_ue.get(rnti, 0))
        pdu_actual = float(totals.txpdu_by_ue.get(rnti, 0))
        buffer_actual = float(totals.txbuffer_by_ue.get(rnti, 0))
        buffer_anterior = float(self.prev_txbuffer_by_ue.get(rnti, 0))

        return drop_actual, pdu_actual, buffer_actual, buffer_anterior

    def get_ue_cost(self, totals, rnti):
        # Obtengo los datos para el costo.
        drop_actual, pdu_actual, buffer_actual, buffer_anterior = self.get_cost_variables(totals, rnti)

        # Normalizo
        drop_norm = min(drop_actual / self.max_drop, 1.0)
        pdu_norm = min(pdu_actual / self.max_pdu, 1.0)
        buffer_norm = min(buffer_actual / self.max_buffer, 1)

        # Calculo el Costo
        costo = self.const_a * np.power(drop_norm,self.const_y) + self.const_b *  np.power(buffer_norm - pdu_norm,self.const_x)
        return costo
    
    # Guardo transición y entreno
    def on_transition(self, prev_s, prev_a, reward, s2):
        # Función que procesa la transición.

        # Si el reward no existe o es infinito lo pongo en 0
        if reward is None:
            reward = 0.0

        if not math.isfinite(float(reward)):
            reward = 0.0

        # Guardo la transición en el replay buffer
        self.agent.store_transition(state=prev_s, action=prev_a, reward=reward, next_state=s2)

        # Si estoy en el período de warmup no entreno
        if self.agent.replay_buffer.size() < MIN_REPLAY_SIZE_FOR_TRAINING:
            self.last_td_error = None
            self.last_loss  = None
            return None
          
        # Entreno la red neuronal
        train_result = self.agent.train_step()


        # Guardo los errores y perdidas del entrenamiento
        if train_result is None:
            self.last_td_error = 0.0
            self.last_loss = 0.0
        else:
            loss, td_error = train_result
            self.last_loss = float(loss)
            self.last_td_error = float(td_error)


        return self.last_td_error


    # Selección de acción
    def select_action(self, state_id):

        # Me fijo el tamaño actual del replay buffer
        size_real_replay_buffer = self.agent.replay_buffer.size()

        # Pongo un tamaño minimo en el replay buffer para empezar a entenar, esto es el warmup.
        ready_4_training= size_real_replay_buffer >= MIN_REPLAY_SIZE_FOR_TRAINING 

        # Selecciono la accion. 
        #Si estoy en el periodo de warmup el epsilon no baja
        action_id = int(self.agent.select_action(state_id, do_decay=ready_4_training))

        # Guardo la accion para usarla como anterior para el caso siguiente
        self.last_action = action_id
        return action_id

    # Convertir acción en shares por UE
    def shares_from_action(self, totals, action_id) :
        # Combierto la accion en recursos para asignar.

        ue_rntis = list(totals.txbuffer_by_ue.keys())

        if len(ue_rntis) == 0:
            return {}

        grupos = split_rntis_in_groups(ue_rntis, n_groups=2)
        shares_grupo = self.space.actions[action_id]

        ue_shares = {}

        # Divido los UEs en grupos para asignar recursos.
        for indice_grupo, miembros in enumerate(grupos):
            if len(miembros) == 0:
                continue

            # Obtengo los porsentajes a asignar del grupo.
            share_del_grupo = float(shares_grupo[indice_grupo])

            pesos = []
            for rnti in miembros:
                buffer_ue = float(totals.txbuffer_by_ue.get(rnti, 0))
                # Evito dividir por 0 si tiene un buffer muy bajo
                if buffer_ue < 1.0:
                    buffer_ue = 1.0
                pesos.append(buffer_ue)

            suma_pesos = sum(pesos)

            for rnti, peso in zip(miembros, pesos):
                ue_shares[rnti] = share_del_grupo * (peso / suma_pesos)

        # Normalización final
        suma_total = sum(ue_shares.values())

        if suma_total > 0.0:
            for rnti in list(ue_shares.keys()):
                ue_shares[rnti] = ue_shares[rnti] / suma_total

        self.last_ue_shares = dict(ue_shares)

        return ue_shares


    # Después de cada observación
    def after_observation(self, totals):
        # Función para actualizar la información historica despues de una iteración.

        self.prev_txbuffer_by_ue = dict(totals.txbuffer_by_ue)
        self.have_prev_buffer = True

        # Si no hay accion elegida, no hay que actualizar nada 
        if self.last_action is None:
            return
        

        if self.last_reward is not None:
            reward = self.last_reward 
        else:
            reward = 0.0

        if self.last_loss is not None:
            loss = self.last_loss

        else:
            loss = float("nan")

        if self.last_td_error is not None:
            td_error = self.last_td_error
        else:
            td_error = float("nan")



        if self.curr_state_vec is None:
            return

        # Guardo los datos de la iteración en el csv
        self.dqn_csv_logger.append(tti_counter=totals.tti_counter,
            reward=reward,td_error=td_error,loss=loss,
            epsilon=self.agent.eps,action_id=self.last_action,
            replay_size=self.agent.replay_buffer.size(),learn_steps=self.agent.learn_steps,
            state_vec=self.curr_state_vec,txbuffer_by_ue=dict(totals.txbuffer_by_ue),
            txdrop_by_ue=dict(totals.txdrop_by_ue),txpdu_by_ue=dict(totals.txpdu_by_ue),
            ue_shares=dict(self.last_ue_shares),)

############
### MAIN ###
############
def main():
    # Crea el servicer de gRPC el cual escucha las solicitudes de la LM
    servicer: pb2_grpc.RlData4QTableServicer = RlData4QTableDqnServicer(const_a,const_b,const_x,const_y,METRICS_WINDOW)
    #Inicializo el servicer y queda escuchando 
    serve_forever(servicer, GRPC_ADDR, max_workers=4)


if __name__ == "__main__":
    # Configuro el formato para loggear en consola
    logging.basicConfig(level=logging.INFO, format="%(message)s",)

    main()