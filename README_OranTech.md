# ORAN-NS3 con integración de aprendizaje por refuerzo
## Modificaciones principales

Este repositorio extiende ORAN-NS3 con soporte para ejecutar escenarios de aprendizaje por refuerzo aplicados a la asignación de recursos en la RAN. Para ello, se incorporó una integración entre ns-3 y agentes externos desarrollados en Python, utilizando gRPC como mecanismo de comunicación.

En términos generales, el flujo implementado permite que ORAN-NS3 genere observaciones del estado de la red, las envíe a un servidor Python, reciba una acción calculada por el agente y aplique dicha acción durante la simulación.

Las modificaciones principales incluyen:

- nuevos ejemplos de simulación para Q-Learning y DQN;
- incorporación de métricas de estado por UE, como buffer, paquetes transmitidos y paquetes descartados;
- implementación de servidores Python para los agentes de aprendizaje por refuerzo;
- comunicación entre ns-3 y Python mediante gRPC;
- adaptación de scheduler RR OFDMA de ORAN-NS3 para utilizar las acciones generadas por el agente.
  
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
./ns3 run oran-nr-example-rl-qlearn
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
./ns3 run oran-nr-example-rl-dqn
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
