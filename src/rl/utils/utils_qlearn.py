import numpy as np
import os

########################
### Se crea tabla Q. ###
########################
class QTable:     
    def __init__(self,n_states,n_actions,alpha = 0.03,gamma= 0.95,
        eps_start = 1.0,eps_end= 0.01,eps_decay= 0.9995,seed=42):
        
        #Inicializo numero de estado y acciones
        self.n_states = int(n_states)
        self.n_actions = int(n_actions)
        
        # Inicializo variables de entrenamiento
        self.alpha = float(alpha)
        self.gamma = float(gamma)

        # Incializo variables de epsilon-greedy
        self.eps = float(eps_start)
        self.eps_start = float(eps_start)
        self.eps_end = float(eps_end)
        self.eps_decay = float(eps_decay)

        np.random.seed(seed)

        # Inicializo la Q-Table vacía
        self.Q = np.zeros((self.n_states, self.n_actions), dtype=np.float32)

    def reset(self): 
        # Reinicia el entrenamiento, vuelve a cero Q y reinicia epsilon.
        self.Q.fill(0.0)
        #Reincio epsilon-greedy
        self.eps = self.eps_start

    def decay_epsilon(self):     
        # Aplica decaimiento multiplicativo para epsilon-greedy.
        self.eps = max(self.eps_end, self.eps * self.eps_decay)
        return self.eps

    def best_action(self, s_id): 
        # Retorna la acción greedy: la que maximiza Q en el estado s_id.                           
        return int(np.argmax(self.Q[s_id]))

    def select_action(self, s_id, do_decay = True):
        # Función que selecciona la acción a ser tomada
        if do_decay:
            self.decay_epsilon()

        random_val = np.random.rand()

        if random_val < self.eps:
            return int(np.random.randint(0, self.n_actions))
        else:
            return self.best_action(s_id)

    def update(self, s_id, a_id, reward, s2_id):     
        # Función actualiza tabla Q.

        #Calculo los valores de q 
        q_actual = float(self.Q[s_id, a_id])
        q_objetivo = float(reward) + self.gamma * float(np.max(self.Q[s2_id]))

        #Calculo td error
        td_error = q_objetivo - q_actual

        # Actualizo la matriz Q
        self.Q[s_id, a_id] = q_actual + self.alpha * td_error
        return td_error

    def save(self, path):                                              
        # Guarda la matriz Q en formato .npz 

        os.makedirs(os.path.dirname(path) or ".", exist_ok=True)
        np.savez_compressed(path, Q=self.Q,n_states=self.n_states,n_actions=self.n_actions,alpha=self.alpha,gamma=self.gamma,
            eps=self.eps,eps_start=self.eps_start,eps_end=self.eps_end,eps_decay=self.eps_decay,)

def load_qtable(path, seed=42):
    # Función que carga una matriz Q ya entrenada
    data = np.load(path, allow_pickle=False)
    obj = QTable(n_states=int(data["n_states"]),n_actions=int(data["n_actions"]),alpha=float(data["alpha"]),gamma=float(data["gamma"]),
        eps_start=float(data["eps_start"]),eps_end=float(data["eps_end"]),eps_decay=float(data["eps_decay"]),seed=seed,)
    obj.Q = data["Q"].astype(np.float32, copy=False)
    obj.eps = float(data["eps"])
    return obj