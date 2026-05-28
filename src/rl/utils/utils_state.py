import numpy as np
from utils_grpc import rntis_from_totals

# Clase para gestionar el espacio de estado discretizado de la red para Q-Learning
class QlearnSpace:
    # El estado queda codificado de la siguiente manera:
    # (buff_ue1_bin, drop_ue1_bin, buff_ue2_bin, drop_ue2_bin)
    def __init__(self, n_ues, buffer_bin_edges_t0, buffer_bin_edges_t1, drop_bin_edges_t0, drop_bin_edges_t1, actions):
        #Inicializamos los parametros del espacio de estado
        self.n_ues = int(n_ues)

        # defino los ubrales para los bins de buffer
        self.buff_bin_t0 = float(buffer_bin_edges_t0)
        self.buff_bin_t1 = float(buffer_bin_edges_t1)
        # defino los ubrales para los bins de drop
        self.drop_bin_t0 = float(drop_bin_edges_t0)
        self.drop_bin_t1 = float(drop_bin_edges_t1)
        
       
        self.n_states = 3 ** (2 * self.n_ues)  #el numero de estados posibles se calcula asi porque cada UE tiene 3 posibles bins de buffer y 3 de drops
        self.actions = actions
        self.n_actions = len(self.actions)

    def encode_state_from_totals(self, totals):
        # Extrago las rntis activas en la iteación actual 
        rntis= self.get_rntis(totals)
        # Me aseguro que la lista de rntis tenga n_ues, si no completo con None los faltantes
        rntis = self.adjust_rntis(rntis)
        # Codifico el estado
        state_id = self.bulid_state_id(rntis, totals)
        return state_id
    
    def bulid_state_id(self, rntis, totals):
        # Construyo un id de estado a partir de la combinación de los bins
        state_id = 0
        for rnti in rntis:
            buffer_value, drop_value = self.get_buffer_drop_rntis(rnti, totals) #obtengo los valores por UE de buffer y drop a partir de los datos totales (vienen de grpc)
            # Discretizo los valores en bins
            buffer_bin = self.get_bin(buffer_value, self.buff_bin_t0, self.buff_bin_t1)
            drop_bin = self.get_bin(drop_value, self.drop_bin_t0, self.drop_bin_t1)
            state_id = state_id * 9 + buffer_bin * 3 + drop_bin 
            # Como son 3 bins (niveles 0,1,2) al multiplicar buffer por 3 la combinación lineal entre las variables da un valor entre 0 y 8
            # Multiplico el estado por nueve para que no se superpongan estados de distintas UEs
        return state_id

    def get_bin(self, x, t0, t1):
    #Función para clasificar en bins un valor X
    #Nos devuelve un bin (0,1,2) dependiendo de los umbrales
        if x <= t0:
            return 0
        if x <= t1:
            return 1
        return 2

    def get_rntis(self, totals):
        # A partir de los datos obtenidos por grpc, obtengo las rntis activas. 
        # considero activas las que hayan reportado valores de buffer o drop en esta iteración.
        buffer_rntis = set(totals.txbuffer_by_ue.keys())
        drop_rntis = set(totals.txdrop_by_ue.keys())
        # Hago un OR entre los conjuntos para asegurarme de incluir todas las que hayan reportado actividad en esta iteración.
        rntis = buffer_rntis | drop_rntis
        #Reordeno para asegurarme de que en todas las iteraciones me aparezcan en el mismo orden
        rntis = sorted(rntis)
        return rntis
    def adjust_rntis(self, rntis):
        # Me aseguro que la lista de rntis tenga n_ues. Ya que Q-table espera un número fijo de UEs para toda iteración.
        # en caso contrario completo con none

        rntis_encontradas = len(rntis)

        if rntis_encontradas < self.n_ues:
            # Si hay menos rntis encontradas que n_ues, completo con None la lista
            rntis_faltantes = self.n_ues - rntis_encontradas
            rntis = rntis + [None] * rntis_faltantes
        else:
            # Si hay más rntis encontradas que n_ues, me quedo con las primeras hasta n_ues. 
            rntis = rntis[: self.n_ues]
        return rntis

    def get_buffer_drop_rntis(self,rnti, totals):
        # Devuelvo los valores de buffer y drop para un rnti dado.
        if rnti is None: 
            # Si se tuvo que rellenar con None en adjust_rntis, entonces le asigno un valor de cero para buffer y drop.
            buffer_value = 0.0
            drop_value = 0.0
        else:
            buffer_value = float(totals.txbuffer_by_ue.get(rnti, 0))
            drop_value = float(totals.txdrop_by_ue.get(rnti, 0))
        return buffer_value, drop_value
    
# Clase para gestionar el espacio de estado discretizado de la red para DQN.
# En este caso, el estado se representa como un vector con valores normalizados de buffer, drop, pdu y count (cuantos UEs caen en cada combinación de bins) para cada combinación.
class DQNSpace:
    def __init__(self, actions, buffer_bin_edges, drop_bin_edges,buf_norm, drop_norm, pdu_norm):
        # Guardo los parámetros de las acciones posibles que se pueden tomar
        self.actions = actions
        self.n_actions = len(self.actions)

        # Guardo los umbrales para los bins de buffer.
        self.buffer_bin_edges = list(buffer_bin_edges)
        self.n_buffer_bins = len(self.buffer_bin_edges) - 1

        # Guardo los umbrales para los bins de drop.
        self.drop_bin_edges = list(drop_bin_edges)
        self.n_drop_bins = len(self.drop_bin_edges) - 1

        #Guardo constantes para normalizar valores
        self.buf_norm = buf_norm
        self.drop_norm = drop_norm
        self.pdu_norm = pdu_norm
        self.count_norm = 100.0

        # Son 4 parametros por cada combinación de bins (buffer, drop, pdu y count))
        self.features_per_cell = 4
        # Dimención total del vector de estado.
        self.state_dim = self.n_buffer_bins * self.n_drop_bins * self.features_per_cell

    def encode_state_from_totals(self, totals):
        #Esta función construye el vector de estado para DQN
        #Obtengo las rntis de los UEs activas en esta iteración.
        ue_rntis = self.get_active_rntis(totals)
        # Genero una matriz para cada feature vacia, de manera auxiliar para organizar los datos de cada combinación.
        matriz_buffer, matriz_drop, matriz_pdu, matriz_count = self.create_empty_matrices()

        for rnti in ue_rntis:
             # Por cada UEs activo, obtengo sus metricas 
            buffer_ue, drop_ue, pdu_ue = self.get_ue_metrics(rnti, totals)
            # Discretizo en bins las metricas
            buffer_bin, drop_bin = self.ue_metrics_to_bins(buffer_ue, drop_ue) 
            # Guardo en las matrices (dentro de la celda que corresponde a la combinación de bins)
            matriz_buffer, matriz_drop, matriz_pdu, matriz_count = self.update_matrices(matriz_buffer, matriz_drop, matriz_pdu, matriz_count,buffer_bin, drop_bin, buffer_ue, drop_ue, pdu_ue,)
        # Normalizo las matrices para tener todos los valores entre 0 y 1
        matriz_buffer, matriz_drop, matriz_pdu, matriz_count = self.normalize_matrices(matriz_buffer, matriz_drop, matriz_pdu, matriz_count)
        # Genero el vector de estado
        vector_estado = self.create_state_vector(matriz_buffer, matriz_drop, matriz_pdu, matriz_count)
        return vector_estado

    def create_state_vector(self, matriz_buffer, matriz_drop, matriz_pdu, matriz_count):
        # Construyo el vector de estado a partir de las matrices las cuales ya tienen dentro los valores correspondientes para cada bin.
        
        lista_features = [] # Creo una lista auxiliar vacía.

        for buffer_bin in range(self.n_buffer_bins):
            for drop_bin in range(self.n_drop_bins):
                #Recorro las combinaciones de bins de buffer y drop
                valor_buffer = matriz_buffer[buffer_bin][drop_bin]
                valor_drop = matriz_drop[buffer_bin][drop_bin]
                valor_pdu = matriz_pdu[buffer_bin][drop_bin]
                valor_count = matriz_count[buffer_bin][drop_bin]
                
                #Agrego en orden a la lista auxiliar los valores 
                lista_features = self.append_values_to_lists(lista_features, valor_buffer, valor_drop, valor_pdu, valor_count)
        # Genero el vector de estado a partir de la lista auxiliar.
        state_vector = np.array(lista_features, dtype=np.float32)
        return state_vector

    def get_ue_metrics(self, rnti, totals):
        # Obtengo las metricas de un UE especifico.
        buffer_ue = float(self.get_buffer(rnti, totals))
        drop_ue = float(self.get_drop(rnti, totals))
        pdu_ue = float(self.get_pdu(rnti, totals))
        return buffer_ue, drop_ue, pdu_ue

    def ue_metrics_to_bins(self, buffer_ue, drop_ue):
        # Discretizo las metricas de un UE especifico en bins.
        buffer_bin = self.get_bin(buffer_ue, self.buffer_bin_edges)
        drop_bin = self.get_bin(drop_ue, self.drop_bin_edges)
        return buffer_bin, drop_bin

    def get_active_rntis(self, totals):
        # Obtengo las rntis de los UEs activos
        active_rntis = rntis_from_totals(totals)
        return list(active_rntis)

    def create_empty_matrices(self):
        # Genero las matrices vacias para cada feature con tamaño [n_buffer_bins, n_drop_bins].]
        matriz_buffer = np.zeros((self.n_buffer_bins, self.n_drop_bins), dtype=np.float32)
        matriz_drop = np.zeros((self.n_buffer_bins, self.n_drop_bins), dtype=np.float32)
        matriz_pdu = np.zeros((self.n_buffer_bins, self.n_drop_bins), dtype=np.float32)
        matriz_count = np.zeros((self.n_buffer_bins, self.n_drop_bins), dtype=np.float32)
        return matriz_buffer, matriz_drop, matriz_pdu, matriz_count


    def update_matrices(self, matriz_buffer, matriz_drop, matriz_pdu, matriz_count, buffer_bin, drop_bin, buffer_ue, drop_ue, pdu_ue):
        # Actualizo las matrices en la celda que pertenecen los bins para dicha metrica.
        matriz_buffer[buffer_bin][drop_bin] += buffer_ue
        matriz_drop[buffer_bin][drop_bin] += drop_ue
        matriz_pdu[buffer_bin][drop_bin] += pdu_ue
        matriz_count[buffer_bin][drop_bin] += 1.0
        return matriz_buffer, matriz_drop, matriz_pdu, matriz_count

    def normalize_matrices(self, matriz_buffer, matriz_drop, matriz_pdu, matriz_count):
        # Normalizo las matrices para tener valores entre 0 y 1.
        # Como el valor de las normalizaciones se obtuvieron empiricamente para sierto tiempo de ejecución se usa np.clip para asegurar que no se pasen del rango [0,1]
        matriz_buffer = np.clip(matriz_buffer / self.buf_norm,0,1)
        matriz_drop = np.clip(matriz_drop / self.drop_norm,0,1)
        matriz_pdu = np.clip(matriz_pdu / self.pdu_norm,0,1)
        matriz_count = np.clip(matriz_count / self.count_norm,0,1)
        return matriz_buffer, matriz_drop, matriz_pdu, matriz_count

    def append_values_to_lists(self, lista_features, valor_buffer, valor_drop, valor_pdu, valor_count):
        # Agrego las 4 features a la lista auxiliar en orden.
        lista_features.append(valor_buffer)
        lista_features.append(valor_drop)
        lista_features.append(valor_pdu)
        lista_features.append(valor_count)
        return lista_features

    def get_bin(self, valor, edges):
        # Clasifico un valor en bins a partir de los umbrales
        cantidad_intervalos = len(edges) - 1
        for i in range(cantidad_intervalos):
            limite_inferior = edges[i]
            limite_superior = edges[i + 1]
            if limite_inferior <= valor < limite_superior:
                return i
        return cantidad_intervalos - 1

    def get_buffer(self, rnti, totals):
        # obtengo los valores de buffer para un rnti dado a partir de los datos tomados por grpc.
        return totals.txbuffer_by_ue.get(rnti, 0)

    def get_drop(self, rnti, totals):
        # Obtengo los valores de drop para un rnti dado a partir de los datos tomados por grpc.
        return totals.txdrop_by_ue.get(rnti, 0)

    def get_pdu(self, rnti, totals):
        # Obtengo los valores de pdu para un rnti dado a partir de los datos tomados por grpc.
        return totals.txpdu_by_ue.get(rnti, 0)
