from collections import deque
import os
import random

import numpy as np
import torch
import torch.nn as nn
import torch.optim as optim
from datetime import datetime

class ReplayBuffer:
    #Buffer para guardar las experiencias pasadas
    def __init__(self, buffer_capacity):
        # Inicializo el buffer.
        self.buffer = deque(maxlen=buffer_capacity)


    def add(self,state, action,reward, next_state):
        # Agrego la experiencia completa en el buffer 
        # (estado, acción, recompensa, siguiente estado)
        state_c = np.array(state, dtype=np.float32, copy=True)
        next_state_c = np.array(next_state, dtype=np.float32, copy=True)
        transition = (state_c, int(action),float(reward),next_state_c,) # guardo copia de los arrays porque es una foto del estado en el momento (y evito que se modifique)
        self.buffer.append(transition) # Como uso deque se borra la experiencia más nueva cuando se agrega una nueva experiencia y se supero buffer capacity

    def sample(self, batch_size):
        # Devuelvo un batch más chico con experiencias aleatorias.
        return random.sample(self.buffer, batch_size)

    def size(self):
        # Devuelvo el tamaño actual del buffer.
        return len(self.buffer)


class QNet(nn.Module):
    # Red neuronal para DQN.
    def __init__(self, state_dim, n_actions):
        super().__init__() # Inicializo nn.Module
        #Defino una red fullyconected de 2 capas ocultas y activación ReLu ambas capas
        self.net = nn.Sequential(
            nn.Linear(state_dim, 128),
            nn.ReLU(), 
            nn.Linear(128, 128),
            nn.ReLU(),
            nn.Linear(128, n_actions),) # devuelvo un valor Q para cada acción.
    def forward(self, x):
        # Calculo el valor Q para cada acción dado un estado de la red 
        return self.net(x)


class DQNAgent:
    # Agente DQN
    def __init__(self,state_dim,n_actions,gamma = 0.1,lr = 1e-4,
        eps_start = 1.0,eps_end = 0.1,eps_decay = 0.99999,buffer_capacity = 5000,batch_size = 256, target_update_every = 300,
        seed = 42,device = None, checkpoint_dir = "checkpoints_nn"):
        
        self.state_dim = int(state_dim) # Guardo la cantidad de features, lo necesito para la primer capa de la red neuronal
        self.n_actions = int(n_actions) # Guardo la cantidad de acciones posibles
        self.gamma = float(gamma) # Guardo el factor de descuento (gamma)

        #inicializo parametros de epsilon greedy
        self.eps = float(eps_start)
        self.eps_start = float(eps_start)
        self.eps_end = float(eps_end)
        self.eps_decay = float(eps_decay)

        
        self.batch_size = int(batch_size) # Tamaño del batch que entreno por iteración
        self.target_update_every = int(target_update_every) # Cada cuantas iteraciones actualizo la red target
        self.learn_steps = 0 # Inicializo variable para contar las iteraciones
        
        self.checkpoint_dir = checkpoint_dir # Directorio para guardar los .pt de la red neuronal

        # Se utilizan seeds para poder reproducir experimentos
        if seed is not None:
            random.seed(seed)
            np.random.seed(seed)
            torch.manual_seed(seed)

        # Si la computadora tiene GPU la uso, si no uso CPU
        self.device = torch.device( device or ("cuda" if torch.cuda.is_available() else "cpu"))

        # Inicializo red online y target (iguales)
        self.online_net = QNet(self.state_dim, self.n_actions).to(self.device)
        self.target_net = QNet(self.state_dim, self.n_actions).to(self.device)

        self.target_net.load_state_dict(self.online_net.state_dict()) # me aseguro que las redes empiecen con los mismos pesos
        self.target_net.eval() # La red target no se entrena, solo se usa el forward.

        # Inicializo el optimizador Adam para actualizar los pesos .
        self.optimizer = optim.Adam(self.online_net.parameters(), lr=lr)
        # Uso MSE como error entre el valor Q online y el target
        self.loss_fn = nn.MSELoss()

        #Inicializo replay Buffer.
        self.replay_buffer = ReplayBuffer(buffer_capacity)


    def select_action(self, state_vec, do_decay = True):
        # Función para seleccionar una acción mediante epsilon greedy.
        
        if do_decay:
            # Reduzco el decay
            self.decay_epsilon()

        if random.random() < self.eps:
            # Sorteo un numero del 0 al 1, si es menor al valor actual de epsilon
            # tomo una acción de manera aleatoria (exploración).
            return random.randrange(self.n_actions)

        # Tomo la acción con mayor valor de Q (explotación).
        s = torch.tensor(state_vec,dtype=torch.float32,device=self.device,).unsqueeze(0)
        with torch.no_grad(): # No calculo gradientes para evaluar
            q_values = self.online_net(s) # Obtengo los valores Q evaluando el estado  
            accion = int(torch.argmax(q_values, dim=1).item()) # Elijo la accion con mayor Q
        return accion
        
    def decay_epsilon(self):
        # Reduczco el episolon hasta llegar al minimo (eps_end)
        self.eps = max(self.eps_end, self.eps * self.eps_decay)
        return self.eps

    def store_transition(self,state,action,reward,next_state):
        # Guardo la transición en el replay buffer.
        self.replay_buffer.add(state, action, reward, next_state)

    def train_step(self):
        # Tomo un batch aleatorio dentro del replay buffer.
        batch = self.sample_batch()

        if batch is None:
            # No entreno si no tengo experiencias suficientes
            return None

        states, actions, rewards, next_states = batch

        # Calculo el valor Q del proximo estado
        q_next = self.target_net(next_states).max(dim=1, keepdim=True)[0].detach() # uso detach para que no se entrenen gradientes en las nn de target
        q_target = rewards +  self.gamma * q_next 

        # Entreno la red online
        loss_value, td_error_value = self.train_live_nn(states, actions, q_target)


        self.learn_steps += 1 # Incremento el numero de iteración
        
        self.update_target_nn() # Si corresponde, actualizo la red target

        return loss_value, td_error_value
    
    def sample_batch(self):
        # Función para tomar un batch aleatorio dentro del replay buffer.

        # Si no hay suficientes iteraciones no entreno
        if self.replay_buffer.size() < self.batch_size:
            return None
        
        # Armo el batch
        batch = self.replay_buffer.sample(self.batch_size)

        # Uso listas auxiliares para poder separar un tensor con todas las experiencias pero separado un tensor por componente de la experiencia (ej: estado)
        state_list = []
        action_list = []
        rewards_list = []
        next_states_list = []

        for transition in batch:
            state, action, reward, next_state = transition
            state_list.append(state)
            action_list.append(action)
            rewards_list.append(reward)
            next_states_list.append(next_state)

        # Como las listas de estados son listas de vectores, uso stack para que quede como un vector. Como reward y action son int los paso directo de lista a tensor.
        states_array = np.stack(state_list)
        next_states_array = np.stack(next_states_list)

        # Transformo las listas en tensores para poder alimentar a las neuronas.
        states = torch.tensor(states_array,dtype = torch.float32, device = self.device,)
        actions = torch.tensor(action_list,dtype = torch.long, device = self.device,).unsqueeze(1)
        rewards = torch.tensor(rewards_list, dtype = torch.float32, device = self.device,).unsqueeze(1)
        next_states = torch.tensor(next_states_array,dtype = torch.float32, device = self.device,)
        
        return states, actions, rewards, next_states

    def train_live_nn(self, states, actions, q_target):
        # Función para entrenar red online.

        # Tomo el valor Q predecido como el resultado de la red para el estado actual y la acción tomada
        q_pred = self.online_net(states).gather(1, actions) 
        # Calculo la diferencia en el estado actual (TD) entre Q online y Q target
        td_error = q_target - q_pred
        # Calculo el MSE entre Q online y target.
        loss = self.loss_fn(q_pred, q_target)
        # Seteo los gradientes en 0 antes de hacer el backward.
        self.optimizer.zero_grad()

        # Hago el backward en la red neuronal online para actualizar los pesos a partir del error MSE calculado.
        loss.backward() 
        # Limito el modulo de los gradientes para que no queden muy grandes
        # evito que la red sea muy sensible a experiencias aisladas.
        nn.utils.clip_grad_norm_(self.online_net.parameters(), max_norm=10.0) 
        # Actualizo los pesos de la red online
        self.optimizer.step()

        # Calculo el valor absoluto del TD error promedio para todo el batch.
        td_error_abs_mean = float(td_error.detach().abs().mean().item())
        return float(loss.item()), td_error_abs_mean

    def update_target_nn(self):
        # Actualizo la red target cada un numero (target_update_every) de iteraciones.
        if self.learn_steps % self.target_update_every == 0:
            # Obtengo los pesos actuales de la red online
            online_nn = self.online_net.state_dict()

            #Actualizo la red target con esos pesos.
            self.target_net.load_state_dict(online_nn)

            #Tomo el tiempo actual para que los archivos .pt no se sobreescriban.
            time = datetime.now().strftime("%y%m%d_%H%M%S")
            # Defino el path para guardar la red neuronal.
            checkpoint_path = os.path.join(self.checkpoint_dir, "dqn_" + time + ".pt")
            # Guardo la red neuronal.
            self.save(checkpoint_path)

    def save(self, path):
        # Guardo la red neuronal
        os.makedirs(os.path.dirname(path) or ".", exist_ok=True)
        # Defino los parametros de la red a ser guardados
        torch.save({"online_net": self.online_net.state_dict(),"target_net": self.target_net.state_dict(),
                "optimizer": self.optimizer.state_dict(),"eps": self.eps,
                "state_dim": self.state_dim,"n_actions": self.n_actions,
                "gamma": self.gamma,
                "eps_start": self.eps_start,"eps_end": self.eps_end,"eps_decay": self.eps_decay,
                "batch_size": self.batch_size,"target_update_every": self.target_update_every,
                "checkpoint_dir": self.checkpoint_dir,
                "replay_buffer": list(self.replay_buffer.buffer),
                "learn_steps": self.learn_steps, "buffer_capacity": self.replay_buffer.buffer.maxlen},path)

def load_dqn_agent(path, device = None):
    # Cargo una red neuronal ya entrenada 
    ckpt = torch.load(path, map_location=device or "cpu", weights_only=False)
    #Le cargo los parametros guardados a un agente nuevo.
    agent = DQNAgent(state_dim=int(ckpt["state_dim"]),n_actions=int(ckpt["n_actions"]),gamma=float(ckpt["gamma"]),
        eps_start=float(ckpt["eps_start"]),eps_end=float(ckpt["eps_end"]),eps_decay=float(ckpt["eps_decay"]),
        buffer_capacity= int(ckpt.get("buffer_capacity", 5000)),
        batch_size=int(ckpt["batch_size"]),target_update_every=int(ckpt["target_update_every"]),
        device=device,checkpoint_dir=ckpt.get("checkpoint_dir", "checkpoints_nn"),)
    
    # Le cargo al agente los pesos guardados de las redes entrenadas
    agent.online_net.load_state_dict(ckpt["online_net"])
    agent.target_net.load_state_dict(ckpt["target_net"])
    agent.optimizer.load_state_dict(ckpt["optimizer"])

    #Le cargo la agente el valor del epsilon y el numero de iteraciones que tenia la red pre entrenada al momento de ser guardada.
    agent.eps = float(ckpt["eps"])
    agent.learn_steps = int(ckpt.get("learn_steps", 0))
    # Cargo el replay buffer guardado
    get_replay_buffer = ckpt.get("replay_buffer", []) # si no hay replay buffer cargo una lista vacia
    agent.replay_buffer.buffer = deque(get_replay_buffer, maxlen=agent.replay_buffer.buffer.maxlen)

    return agent