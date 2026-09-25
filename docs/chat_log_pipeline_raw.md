# Log crudo de la conversación sobre la arquitectura y flujo de datos (Pipeline)

## 1. Planteo inicial del Flujo (Por el usuario)
bien vamos con el flujo del proceso.
En el bhi existen dos FIFOs uno llamado wake up y otro non wake up. Cada uno de ellos almacena los datos de los distintos sensores virtuales que esten activados. Un sensor virtual puede ser de tipo wake up o no wake up dependiendo su tipo sus datos van a una u otra FIFO.
ambas fifos tienen 19kb aprox y los datos que se guardan en ellas son eventos. Los eventos son paquetes que incluyen el resultado del sensor instanciado en el formato que corresponda. Por ejemplo un evento de accelerometro sera un paquete de 7 bytes que contiene los datos de los 3 ejes y su identificador.
Cada vez que un nuevo evento sea generado se activa el pin de interrupt y por este se indica al micro que hay datos en alguna de las fifos. A su vez el bhi tiene dos modos de trabajo, con host activo o host dormido. Segun si esta en un modo u otro se interrumpe o no con los datos de la fifo non wake up.
El flujo por lo tanto es el siguiente: 
Dentro del microcontrolador (host) existira una task que atienda las interrupciones. Inicialmente esta task pone al bhi en modo host dormido, de forma que solo evento de wake up seran notificados.
Al hacer esto solo se estara escuchando a dos eventos: movimiento detectado o no movimiento detectado. Ambos funcionan con la premisa que si por 5 segundos hay o no hay movimiento el evento correspondiente se genera.
Estos eventos son de tipo one shot por lo que cada vez que se ejecuta uno se debe volver a iniciar el sensor.
Cuando se recibe una interrupcion en este modo mencionado, el microcontrolador parsea los datos de la fifo wake up y evalua que evento se recibio. Si el evento corresponde a uno de movimeinto entonces esta tarea encendera un conjunto de sub tareas para procesar los posibles eventos que ocurran.
Por otro lado si se detecta un evento de no movimeinto entonces el proceso contrario es realizado.
De esta forma esta tarea asegura que no se ejecute constantemente todo el procesamiento y que el micro pueda hibernar.
Una vez la tarea system detecta el moviento activa un conjunto de sub tareas. Entre ellas una tarea de procesamiento de datos (task_alarmas_events). Esta tarea tendrá la mision de procesar los datos del acelerometro y validar si se detecta una caida en este.
La actualizacion de datos del buffer del acelerometro en el micro se resuelve en la tarea system (esto esta a chequear). Cuando se recibe la confirmacion de que hubo movimeinto esta tarea despierta la fifo de non wake up y comienza a recibir datos de esta.
Para obtener los datos del micro existe una funcion dentro del driver de bhi que obtiene los datos de las fifos activas y los parsea. El parseo implica direccionar los datos a un conjunto de clases donde cada una almacena los datos del sensor correspondiente y provee de herramientas para manejar los datos.

Continuo: 
Ademas los sensores se configuran para tener una latencia en los datos. Esto quiere decir que el sensor "publica" los datos luego de que vence ese tiempo. Este tiempo lo vamos a llamar LATENCIA_DATOS_ACCELEROMETRO.
Por lo tanto nuestra task system estara escuchando a nuevas interrupciones y cuando una ocurra parseara todos los datos de sus fifos, entre ellos los del acelerometro, barometro, temperatura.
Estos datos que son almacenados por las clases correspondientes, en sus buffers internos. NOTA: las clases de momento no son single tone por lo que instanciar las en tareas distintas puede generar un problema de los buffers.
Cuando una interrupcion se recibe un conjunto de datos del sensor es recibido llamando al metodo de parseo y luego se envia por medio de una queue un evento de "procesar sensor" a la tarea correspondiente. Notar que en este caso si la fifo del sensor estab completamente populada de datos (mas de los que soporta el buffer del host) entonces se pierden los datos mas antiguos, esto no deberia ser un problema ya que esos datos corresponden a un tiempo donde no habia movimiento. 
Por otro lado hasta que no se parsean todos los datos de la fifo no se envia el comando de parsear datos ya que la funcion es bloqueante.
Debido a que la tarea de procesar va a tocar el buffer donde se escribe puede ser una buena idea realizar algun mecanismo de control para que no intervengan ambos proceso en simultaneo sobre la tarea.
Una vez el comando es recibido en la task de procesamiento, esta registra un chunk de datos del buffer de CHUNK_PROCESAMIENTO muestras que es analizado buscando signos de una posible caida.
Los tiempos luegos seran determinados,
Este chunk de datos debe ser tal que permita mantener en el buffer de la clase datos previos y futuros al evento de caida, para que todo esto sea enviado al servidor.
Si una caida es detectada la task de procesamiento notifica a las demas tareas para que ejecuten el proceso de notificacion y alerta del evento. Entre esas tareas habra una task de comunicacion que obtiene el buffer completo y lo envia junto con los datos de presion y temperatura al dispositivo conectado.
Cada vez que no haya sido detectado un evento de caida la tarea notifica a la tarea system y esta habilita el ingreso de nuevos datos (cuando ocurra una interrupcion) Notar que en este caso el procesamiento de datos debe ser mas rapido que la latencia de los datos en llegar lo cual se estima es suficiente.
El buffer del acelerometro que es aquel que se procesa, es de tipo circular y siempre se pusehan en su head los datos. Por otro lado los datos que se procesan se encuentran en un punto intermedio del buffer de forma que al detectar una caida se tienen todo el historico de datos previos y futuros.


## 2. Refinamiento y críticas abordadas (Por el usuario)

1. CRÍTICO: La paradoja del "Pre-Caída" vs la Hibernación
Vamos con este primero. En este escenario el imu se encuentra registrando datos constantemente en su fifo interna. Al hacer un evento de movimeinto (originado por la caida) esto despertara al micro y pedira los datos que ya se encuentran en la fifo y son en consecuencia los ultimos datos almacenados. El sistema los procesaria igual que si hubiese estaod fuera de hibernacion.

2. CRÍTICO: El error lógico del "Post-Caída"
Ojo en este caso los utlimos datos que se ingresan a la cola circular no son los que se procesan. De alguna manera podria decir que proceso datos mas viejos. Por lo que cuando el sistema evaluar la seccion del buffer donde hubo una caida ya se empujaron previamente datos de post caida en el buffer.

3. CRÍTICO: El Cuello de Botella del RTOS (Bloqueo en Cascada)
Esto es correcto, sin embargo aca tengo dos mecanismos para mitigar esto. En la bibliografia se menicona que un evento de caida dura aprox 3 segundos. Cointemplando esto, mi area de procesamiento sera mas grande (6 segundos) Por lo que si bien esto me da la desventaja de procesar multiples veces lo mismo me permite tener dos ventajas, uno es el solapamiento de los datos (ingresan 3 segundo proceso 3 segundos viejos con 3 segundos nuevos) por lo que si la caida esta cortada entre ambas muestras esto permitiria verlo.
Por otro lado en caso de que se retrase mas en procesar sigo teniendo una ventana que es capaz de ver mas alla de una muestra. El escenario que no se contempla es el solapameinto de ese caso es decir existira una parte de datos que no se analiza en conjunto. 
Se podria solventar haciendo un flag de este suceso (se recibio una interrupt pero no se tenia el procesmaiento disponible) haciendo un recorrido de todo el buffer en busqueda de caidas. 


## 3. Sugerencia final de TODOs (Por el usuario)
como TODO aca quedaria revisar el scope de las clases de sensores para que sean visibles desde distintas tareas (podrian ser globales hay que verlos). Ademas el mecanismo de proteccion del buffer y por ultimo el metodo para procesar el buffer completo en caso de que corresponda.
Como ultimo TODO o sugerencia se me ocurre que la funcion de procesar podria ser ejecutada dentro de la clase de IMU para evitar copiar el buffer continuamente aunque habri que evaluar bien este caso.
Podes agregar esto en la documentacion en forma clara y tecnica
