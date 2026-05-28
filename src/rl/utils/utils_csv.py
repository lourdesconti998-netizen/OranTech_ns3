import csv
import json
import math
import os
import threading
from datetime import datetime


class TtiCsvLogger:
    # Clase para guardar logs para el entrenamiento Qlearn
    def __init__(self, base_dir):
        self.lock = threading.Lock()

        # Creo las carpetas para logs
        logs_dir = os.path.join(base_dir, "logs")
        os.makedirs(logs_dir, exist_ok=True)

        # Defino variables para nombrar los .csv
        timestamp = datetime.now().strftime("%Y%m%d_%H%M%S")
        filename = f"rl_qlearn_log_{timestamp}.csv"
        self.path = os.path.join(logs_dir, filename)

        #Creo el encabezado del csv
        self.create_header()

    def create_header(self):
        # Creo los encabezados 
        with open(self.path, "w", newline="") as file:
            writer = csv.writer(file)
            writer.writerow(["tti_counter","rnti","txbuffer","txdrop",
                 "txpdu","state_id","action_id","share","reward","td_error",])

    def get_rntis(self, totals):
        # Obtengo las rntis de las UEs activas
        rntis = (set(totals.txbuffer_by_ue) | set(totals.txdrop_by_ue) | set(totals.txpdu_by_ue))

        return sorted(rntis)

    def append(self,totals,state_id,action_id, ue_shares,reward=None,td_error=None,):
        # Función que agrega a la ultima fila del csv los datos del entrenamiento para una iteración dada.

        rntis = self.get_rntis(totals)

        if len(rntis) == 0:
            return

        with self.lock:
            with open(self.path, "a", newline="") as file:
                writer = csv.writer(file)

                for rnti in rntis:
                    # recorro las UE para guardar sus parametros
                    txbuffer = int(totals.txbuffer_by_ue.get(rnti, 0))
                    txdrop = int(totals.txdrop_by_ue.get(rnti, 0))
                    txpdu = int(totals.txpdu_by_ue.get(rnti, 0))
                    share = float(ue_shares.get(rnti, 0.0))

                    reward_value = ""
                    if reward is not None:
                        reward_value = float(reward)

                    td_error_value = ""
                    if td_error is not None:
                        td_error_value = float(td_error)
                    #Escribo las variables en la ultima fila del .csv 
                    writer.writerow([int(totals.tti_counter),int(rnti),txbuffer,txdrop,txpdu,int(state_id),
                            int(action_id),share,reward_value,td_error_value,])


class DqnCsvLogger:
    # Clase para guardar logs para el entrenamiento DQN

    def __init__(self, base_dir):
        self.lock = threading.Lock()

        # Creo las carpetas para logs
        logs_dir = os.path.join(base_dir, "logs")
        os.makedirs(logs_dir, exist_ok=True)

        # Defino variables para nombrar los .csv
        timestamp = datetime.now().strftime("%Y%m%d_%H%M%S")
        filename = f"rl_dqn_log_{timestamp}.csv"
        self.path = os.path.join(logs_dir, filename)

        #Creo el encabezado del csv
        self.create_header()

    def create_header(self):
        # Creo los encabezados 
        with open(self.path, "w", newline="") as file:
            writer = csv.writer(file)
            writer.writerow(["tti_counter","reward","td_error",
                    "loss","epsilon","action_id",
                    "replay_size","learn_steps","state_vec",
                    "txbuffer_by_ue","txdrop_by_ue","txpdu_by_ue",
                    "ue_shares",])

    def state_vec_to_json(self, state_vec):
        # Combierto el vector de estado (np) en json 
        state_vec = state_vec.tolist() # Antes de pasar a Json lo tengo que pasar de array a lista.
        state_vec_json = json.dumps(state_vec)
        return state_vec_json

    def serialize_float(self, value):
        # Función que transforma los valores para que sean aceptados por el CSV
        if value is None:
            return ""
        value = float(value)
        if math.isnan(value):
            return "nan"
        return value

    def append(self,tti_counter,state_vec,reward,td_error,loss,epsilon,action_id,replay_size,
        learn_steps,txbuffer_by_ue,txdrop_by_ue,txpdu_by_ue,ue_shares,):
        # Función que agrega a la ultima fila del csv los datos del entrenamiento para una iteración dada.

        # Combierto las variables al formato JSON para poder guardarlas en .csv
        state_vec_json = self.state_vec_to_json(state_vec)
        txbuffer_json = json.dumps(txbuffer_by_ue)
        txdrop_json = json.dumps(txdrop_by_ue)
        txpdu_json = json.dumps(txpdu_by_ue)
        shares_json = json.dumps(ue_shares)

        with self.lock:
            with open(self.path, "a", newline="") as file:
                writer = csv.writer(file)
                # En la ultima fila, escribo datos del entrenamiento para esta iteración.
                writer.writerow([int(tti_counter),self.serialize_float(reward),self.serialize_float(td_error),self.serialize_float(loss),self.serialize_float(epsilon),
                        int(action_id),int(replay_size),int(learn_steps),
                        state_vec_json,txbuffer_json,txdrop_json,txpdu_json,shares_json,])