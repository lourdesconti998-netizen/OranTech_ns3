# Ejecución de algoritmos RL en ORAN-NS3
## Ejecución de escenarios con aprendizaje por refuerzo
En esta sección se indican los pasos para ejecutar los escenarios de aprendizaje por refuerzo desarrollados en ORAN-NS3, considerando las dos técnicas implementadas: Q-Learning y DQN.
## Iniciar servidor Q-Learning

En una nueva terminal:

```bash
cd ORAN-NS3
```

Activar el entorno virtual:

```bash
source .venv/bin/activate
```

Ir a la carpeta `RL`:

```bash
cd src/rl
```

Encender el servidor:

```bash
python3 rl_qlearn.py
```
### Importante: WARNING

Los servidores de Q-Learning y DQN no deben iniciarse al mismo tiempo.

## Iniciar entorno Q-Learning

En una nueva terminal:

```bash
cd ORAN-NS3
```

Configurar ejemplos:

```bash
./ns3 configure --enable-examples
```

Compilar:

```bash
./ns3 build
```

Ejecutar el escenario Q-Learning:

```bash
./ns3 run oran-nr-example-mdp-rl-v4
```

## Iniciar servidor DQN

En una nueva terminal:

```bash
cd ORAN-NS3
```

Activar el entorno virtual:

```bash
source .venv/bin/activate
```

Ir a la carpeta `RL`:

```bash
cd src/rl
```

Encender el servidor:

```bash
python3 rl_dqn.py
```

## Iniciar entorno DQN

En una nueva terminal:

```bash
cd ORAN-NS3
```

Configurar ejemplos:

```bash
./ns3 configure --enable-examples
```

Compilar:

```bash
./ns3 build
```

Ejecutar el escenario DQN:

```bash
./ns3 run oran-nr-example-rl-dqn-v1
```

## Entorno virtual
En una nueva terminal:

```bash
cd ORAN-NS3
```
Crear entorno virtual:

```bash
python3 -m venv .venv
```

Activar entorno virtual:

```bash
source .venv/bin/activate
```

Instalar dependencias:

```bash
pip install -r requirements.txt
```

Nota: Se utilizaron herramientas de inteligencia artificial como apoyo en procesos puntuales de instalación y depuración.
