# MH3U para Android y PC

Juega tu propia copia de Monster Hunter 3 Ultimate en Android, de forma nativa y con controles táctiles pensados para
teléfonos, y en una PC con Windows con teclado, ratón o mando ([PC (Windows)](#pc-windows)).
No es un emulador: el código del juego se recompila de antemano a código nativo para tu dispositivo, y corre sobre un
runtime propio que hace el trabajo de la consola, con los gráficos dibujados con Vulkan.

**Este proyecto no incluye código del juego, archivos del juego, ROMs ni keys. Necesitas tu propia copia del juego.**

MH3U para Android y PC es el tercer port de la serie, hecho de la misma forma que
[MH4U para Android y PC](https://github.com/khorne-hub/4u-native) y
[MHGU para Android y PC](https://github.com/khorne-hub/gu-native), con todo lo que tiene MH4U para Android y PC 1.0.2 que
existe en MH3U: la misma app, las mismas opciones, controles, multijugador y comodidades, sobre el juego propio de MH3U.
Los tres ports van a la par.

[Read this guide in English](README.md)

Habla con otros jugadores y pide ayuda en el [Discord](https://discord.gg/sk5xEjEMwF) de la comunidad.

## Qué ofrece

### Imagen y rendimiento

- **La resolución de tu teléfono:** la imagen 3D se dibuja de fábrica a la resolución de tu pantalla (tres veces la de la
  consola en una pantalla de 720 líneas). Se baja en el engranaje, **Gráficos y audio**, **Resolución 3D**, hasta la de la
  consola
- **Pantalla ancha igual a tu pantalla**, de los teléfonos ultra anchos a las consolas portátiles 4:3, sin bandas, y el
  **HUD en los bordes** de la pantalla
- **De 15 a 240 FPS:** el **Límite de cuadros por segundo** va de 15 a 240. El juego mantiene su velocidad; solo se dibujan
  más o menos cuadros. **Ritmo estable**, apagado de fábrica, corre una parte pareja del límite cuando el teléfono no llega
- **Modo gama baja** para teléfonos débiles, con **Tostadora** (30 FPS parejos, la resolución de la consola, sin
  reverberación) y **Ladrillo** (la resolución de la consola, texturas chatas y píxeles nítidos, para los más débiles) como
  sus otras opciones
- Interruptores propios para las sombras, los **efectos** de partículas del juego, cada uno de sus filtros de pantalla
  (resplandor, desenfoque de movimiento, profundidad de campo, desenfoque radial, reverberación del calor) y la
  reverberación del sonido, y el **campo de visión**
- **Generación de cuadros**, **FSR 1** con su nitidez, **Escalado rápido (más FPS)**, el **filtro PS1**, **Color** (tono,
  temperatura, tinte, brillo, contraste, saturación, gamma) con tres presets, **packs de texturas HD** (también en KTX2 y a
  mitad de tamaño) y **tu propio driver de GPU**, bajo tu responsabilidad
- Un contador de FPS opcional, con la temperatura del teléfono, que el editor del diseño mueve a cualquier lugar
- **Modo de pantalla única, como el del MH4U:** el propio juego dibuja los menús de la pantalla táctil en la principal,
  tan nítidos como el resto de la imagen y al lado de lo que acompañan: el menú principal, la elección de partida, la
  creación del personaje, el menú START y su Bolsa, las cajas de objetos, la caja de suministros, las tiendas con cuántos y
  el total, la herrería y la armería, los mostradores de misiones y el tablón de la Taberna, las recompensas, los menús de
  Cha-Cha y Kayamba, la granja. Se manejan con los botones, el pad de menú o deslizando, como en el MH4U; OK y Volver son A
  y B. La pantalla táctil queda oculta, y su botón la trae para lo que todavía pide un dedo (las acciones del pueblo y los
  objetos rápidos de una misión se abren grandes)
- **La app con los colores del agua:** las opciones, los botones, el editor y las pantallas de la app en el verde mar y el
  dorado del ícono del MH3U
- La pantalla táctil también puede ir al lado del juego, debajo o en una esquina (movida, con otro tamaño, transparente u
  oculta), o en la segunda pantalla de un aparato con dos

### Controles táctiles

- Una palanca, la cámara arrastrando, una píldora de ataques, y botones para rodar, cubrirse, envainar, el objetivo, fijar
  y centrar la cámara
- **Los controles siguen al juego:** la caza, los menús, las charlas, y los arcos y ballestas con un diseño propio (un
  botón grande de disparo, recargar, apuntar, capas y munición)
- **El pad de menú:** una cruceta con L y R bajo el pulgar en los menús del juego, también en las ventanas que preguntan
  cuántos
- **La barra de objetos sin la cruceta:** un toque usa el objeto, deslizar gira la barra, y sus flechas la siguen
- **Fijar al monstruo:** la cámara sigue girando hacia el monstruo (el de la Target Cam, o el grande más cercano), también
  bajo el agua; los arcos y ballestas giran hacia él al disparar
- **El mapa como una pieza del HUD:** el mapa de la zona, con la flecha del cazador y los íconos de los monstruos, se mueve,
  crece, se achica y se oculta junto; un botón **Mapa** lo muestra u oculta
- **Personalizar controles y HUD:** mover y cambiar el tamaño de cada botón y de cada parte del HUD (las barras, el reloj,
  el nombre del cazador, el mapa, los objetos, la munición), con el marco de cada parte justo encima en cualquier forma de
  pantalla, y **Tamaño del HUD** de 60 a 100 %. En 4:3 y con cualquier campo de visión el HUD queda entero y en su lugar
- **10 perfiles de controles** (el diseño propio de la app, o uno como el de un emulador), que se exportan e importan como
  `.zip`; **15 botones combinados** por perfil; botones que quedan apretados (**Mantener**) y **tus propias imágenes** en
  los botones
- **Apuntar con el giroscopio** mientras una ballesta apunta; la cámara solo en la mitad derecha; una palanca flotante;
  ocultar los controles con una pulsación larga en un lugar vacío
- **El teclado del teléfono** para el nombre de tu cazador y el chat
- **La cámara arriba y abajo, también en tierra:** deslizar hacia arriba o abajo mueve la cámara suave por las seis alturas
  del juego (los pasos de la cruceta, sin los saltos); bajo el agua gira hacia todos lados y te hace nadar arriba y abajo,
  y aparece el medidor de aire

### Mandos y teclado

- Un mando oculta los controles táctiles; **START+SELECT** abre las opciones
- Sin el Circle Pad Pro, la palanca derecha gira la cámara, y su clic fija al monstruo
- Las acciones del pueblo (Tarjetas del Gremio, Gestos, Objetos, Cazadores de alquiler) sin la pantalla táctil
- Botones por su lugar o por su letra, la velocidad de la cámara, la zona muerta, y **Mapear un mando** para uno que el
  juego no lee bien
- **Teclado y ratón en el teléfono:** las teclas mantenidas caminan y giran la cámara, y un ratón capturado la gira como en
  la PC

### Ayudas de caza (todas opcionales)

- **Números de daño**, cuatro estilos, con los colores de **Por golpe** (puntos débiles, partes duras, golpes críticos, el
  elemento), también en los monstruos chicos
- **Las barras de vida de los monstruos**, con los nombres del juego
- **Los monstruos siempre en el mapa**, como con una bola de pintura
- **El arco dispara al monstruo** que tiene enfrente el cazador

### Comodidades (todas opcionales)

- **Recolectar todo** lo que tiene un punto de un toque, o **mantener A** para recolectar una y otra vez
- **Moverse cancela el flex**, y **B cancela el afilado**
- **La pantalla de recompensas sin límite de tiempo**, y **ZL+ZR termina la espera** después de una misión
- **Enviar todo a la caja:** el **Enviar a la caja de objetos** de las recompensas manda todas de una vez
- **Comer y beber más rápido:** los trucos propios del MH3U vienen con la app (apagados)
- **El árbol de armas de la herrería:** mientras la lista de Forjar o Mejorar de la herrería muestra un arma, un botón (o
  la palanca derecha) abre su árbol con los íconos del juego: el ataque de cada arma, su elemento o estado (uno oculto, con
  Despertar), el filo, la afinidad, las ranuras, la rareza y lo que pide forjarla o mejorarla

### Y más

- **Partidas:** importar desde Azahar, Citra, Checkpoint, JKSM o cualquier carpeta, exportar, una **Carpeta de partidas**
  propia (para una app de sincronización), y una copia en `save_backups` de cada partida que un cambio reemplaza
- **Misiones de evento:** se importan; en el MH3U ya son de todos los cazadores
- **Mods y trucos:** mods en el orden de carga que elijas, con sus conflictos a la vista, y trucos en el formato de Azahar
- **Multijugador:** hubs en internet sin abrir puertos, salas en tu red y las de Azahar, teléfonos y PC juntos, y el chat.
  Mira [Jugar con otros](#jugar-con-otros)
- **Verificar archivos del juego**, una pantalla de error con **Compartir registro**, el juego espera mientras las opciones
  están abiertas (jugando solo) y mientras la app no está a la vista, un acceso directo en el inicio, y un aviso cuando sale
  una versión nueva
- **El código del juego se crea en el teléfono**, sin PC

Lo que tiene el MH4U y el MH3U no, no se muestra: el glaive insecto y el hacha cargada, las recompensas del Felyne, el
botón de patada del port, la espera de la caja de suministros y un antialiasing propio del juego.

## ¿Qué tan bien anda?

Medido en el teléfono durante el desarrollo, con los ajustes de fábrica del teléfono salvo que diga otra cosa:

| Aparato | GPU | RAM | Cuadros por segundo |
|---|---|---|---|
| Galaxy S25 Ultra | Adreno 830 | 12 GB | Modo gama baja: 60 en la Aldea Moga, el Bosque Moga y Puerto Tanzia con límite 60; 120 en Puerto Tanzia con 120 |
| Anbernic RG406V | Mali-G57 | 8 GB | Modo gama baja, límite 60: 60 en la Aldea Moga y el Bosque Moga, unos 59 en Puerto Tanzia; resolución 2x: 60 en Puerto Tanzia |
| moto g06 power | Mali-G52 MC2 | 4 GB | Modo gama baja, límite 60: unos 56 a 57; resolución 2x: unos 52 en Puerto Tanzia; Tostadora y Ladrillo: 30 parejos |

Ya no están las paradas cortas del propio juego (cada 15 segundos más o menos, mientras rehacía su sesión local), y el
renderer deja afuera lo que dibujaría una pantalla táctil oculta: donde el teléfono llega, el MH3U anda tan parejo como el
MH4U. **Si tu teléfono se calienta, baja el límite de cuadros por segundo**, o prueba el modo Tostadora.

## Qué necesitas

- **Un aparato Android:** Android 9 o más nuevo, de 64 bits, con Vulkan. Probado en GPU Mali y Adreno. Un teléfono con Android de 32 bits (muchos teléfonos Android Go) no puede
  instalar la app.
- **Unos 3 GB libres** en el aparato: el juego ocupa unos 2 GB, y la app necesita 1 GB más mientras crea el código del
  juego.
- **Conexión a internet** la primera vez que la app crea el código del juego (sus herramientas, unos 100 MB), y **tiempo**:
  alrededor de una hora y media en un teléfono de gama baja, una sola vez.
- **Tu propia copia del juego, descifrada:** Monster Hunter 3 Ultimate de EE. UU. (título 00040000000AE400) como archivo
  `.3ds` o `.cci`. No tiene actualización: alcanza con el juego. La copia europea todavía no es compatible.

Este proyecto no explica cómo volcar un juego de tu consola, y no da archivos del juego, keys ni enlaces a ellos.

## Instalación

Lo haces una vez (y el tercer paso otra vez cuando una versión nueva de la app lo pida).

### Paso 1: Pon tu juego en una carpeta

Pon tu juego en una carpeta propia del teléfono o de su tarjeta SD, por ejemplo `MH3U`, en cualquier lugar menos dentro de
`Android/data`. Cualquier nombre de archivo sirve. Usa tu propia copia: los archivos del juego de otros lados suelen
estar cifrados, incompletos o ser de otra región, y entonces la app se detiene con un error. Una vez que el juego abre,
engranaje, **Sistema**, **Tu partida**, **Verificar archivos del juego** lee el juego entero y te dice qué falta, qué está
cortado o qué cambió.

Si también juegas al MH4U, su carpeta puede tener también el juego del MH3U: cada app crea y lee su propio código del
juego.

### Paso 2: Instala la app

1. En tu aparato, descarga el APK desde [Releases](../../releases/latest) y ábrelo. Android puede pedirte permitir la
   instalación de apps desde el navegador.
2. Abre **MH3U**, toca **Dar permiso y elegir carpeta** y activa el acceso a los archivos cuando Android lo pida. El juego
   solo lee esa carpeta: ahí no se copia ni se cambia nada.
3. Busca tu carpeta `MH3U` (aparece con un ✓) y toca **Usar esta carpeta**. Si prefieres el selector de Android, toca
   **Selector de Android**.

MH3U es una app propia (`com.mh3u.native`): se instala junto a MH4U para Android y MHGU para Android, con sus propias
partidas y ajustes, que se guardan en `Android/data/com.mh3u.native/files`. Para elegir otra carpeta después: engranaje,
**Sistema**, **Tu partida**, **Carpeta del juego**.

### Paso 3: Crea el código del juego

La app no trae código del juego, así que creas el tuyo a partir de tu copia, una sola vez. No se envía nada a ningún lado.

La app dice que falta la librería: toca **Crear en este teléfono** y después **Crear**. Más tarde también está en el
engranaje, **Sistema**, **Código del juego**, **Crear el código del juego**.

- La primera vez descarga sus herramientas, unos 100 MB: Python de python.org y Zig, un compilador de C, de PyPI. Los dos
  son software libre, se comprueban antes de usarlos, y nunca se descarga nada del juego.
- Necesita 1 GB libre mientras trabaja; al terminar quedan solo las herramientas, y **Borrar las herramientas** también
  las quita. En un teléfono con 4 GB de memoria (RAM), cierra las otras apps antes.
- Tardó unos 88 minutos en un moto g06 power (un Mali-G52 de gama baja). Sigue con la pantalla apagada o en otra app, con
  su avance en una notificación, y si se corta **Seguir** continúa desde donde quedó. **Empezar de cero** borra lo que dejó
  un armado cortado, cancelado o fallido, para que el próximo **Crear** empiece desde el principio.
- El teléfono se calienta: tenlo cargando, y fuera de la funda si puedes.

Cuando dice que el código del juego está listo, `libmh4u_code_native.so` y `libmh4u_code_native.id` están en tu carpeta,
y **Jugar** abre el juego. **Estos dos archivos se hacen con tu juego: guárdalos para ti y no los compartas.**

Quien desarrolla puede crear la misma librería en una PC con el código de este repositorio: mira
[Armar la app](#armar-la-app).

## Actualizar a una versión nueva

Descarga el APK nuevo desde [Releases](../../releases/latest) y ábrelo: se instala encima de la app que tienes. Tus
partidas, ajustes, diseño y carpeta del juego quedan. Si después la app dice que la librería es de otra versión, créala
de nuevo (engranaje, **Sistema**, **Código del juego**, **Crear el código del juego**). Las notas de cada versión dicen si
la librería sigue sirviendo.

## Opciones

Toca el engranaje en la pantalla para abrir las opciones. Una pulsación larga abre el editor del diseño. Las opciones
tienen seis secciones, en la columna de la izquierda: **Controles**, **Gráficos y audio**, **Juego**, **Multijugador**,
**Mods y trucos** y **Sistema**, con **Créditos y licencias** debajo. Una fila con flecha abre una página con el resto
(**Pantalla y HUD**, **Avanzado**, **Entrar a una sala**...). El **?** a la derecha de una fila dice qué hace. Cambia lo que
quieras y toca **Guardar**; algunas opciones avisan que necesitan reiniciar, y **Guardar y reiniciar** lo hace por ti.
Solo aparece lo que tiene el MH3U.

- **Resolución 3D:** 100 % es la de tu teléfono; menos es más liviano para el teléfono. **1x** es la de la consola, para
  los teléfonos más débiles. Solo puede ser un número entero de veces la pantalla de la consola (400x240), así que cada
  porcentaje se lleva al más cercano.
- **Límite de cuadros por segundo:** de 15 a 240. 30 es el de la consola, calienta menos el teléfono y ahorra batería; más
  de 60 es para teléfonos rápidos con una pantalla así de rápida.
- **Modo gama baja:** **No**, **Sí**, **Tostadora** o **Ladrillo**. Después puedes cambiar igual cada una de sus opciones.
- **Modo de pantalla única** (engranaje, **Gráficos y audio**, **Pantalla y HUD**, prendido salvo que lo apagues): mira
  [Qué ofrece](#imagen-y-rendimiento). El botón (o la tecla) de la pantalla táctil la trae de vuelta, con los menús en ella,
  como en la consola.
- Las **ayudas de caza**, el **Tamaño del HUD**, el **Contorno del texto** y el **contador de FPS** también están en
  **Pantalla y HUD**; las comodidades, **Enviar todo a la caja** y el árbol de armas, en **Juego**.
- **Borrar la caché de shaders** (engranaje, **Gráficos y audio**, **Avanzado**): el próximo inicio borra los shaders que
  el juego compiló y los vuelve a compilar, para una imagen que muestra algo mal después de actualizar el driver de la GPU.

## Importar tu partida

1. Copia tu partida al aparato, por ejemplo en `Download`: desde Azahar o Citra, la carpeta que abre **Open Save Data
   Location** (clic derecho en el juego), o la carpeta de usuario entera del emulador, la que tiene `sdmc` adentro; o una
   copia de Checkpoint o JKSM.
2. En la app: engranaje, **Sistema**, **Tu partida**, **Importar partida**, **Elegir carpeta**, y elígela.

La partida que reemplaza queda en `save_backups`. **Exportar partida** escribe la tuya con la forma de los emuladores
(`MH3U save`).

**Carpeta de partidas** (engranaje, **Sistema**, **Tu partida**) guarda tu partida en una carpeta que elijas, donde una
app de sincronización (FolderSync, Autosync for Google Drive) puede mantenerla igual a una de la nube. Si esa carpeta ya
tiene una partida, la app pregunta cuál sigue, y guarda la otra en `save_backups`. Juega en un aparato a la vez, y guarda
antes de cambiar.

## Mods y trucos

Engranaje, **Mods y trucos**. Los mods para MH3U de EE. UU. van en la carpeta `mods` de tu carpeta del juego (cada uno con
una carpeta `romfs`, una `exefs` o las dos, o con la forma de los emuladores, con una carpeta `00040000000AE400`), en el
orden de carga que elijas; los mods que cambian los mismos archivos se muestran. Los trucos van en el formato de Azahar
(**Agregar trucos**). Los packs de texturas HD hechos para Azahar van en la carpeta `textures` de tu carpeta del juego.

## Jugar con otros

El MH3U se juega con otros en **Puerto Tanzia**: en el muelle de la Aldea Moga, el barco de Neko te lleva. **Multi** abre
un puerto propio para que otros entren; **Friend Search** encuentra los puertos de los cazadores de tu hub o tu sala.
Después, el tablón de misiones de la Taberna, como en la consola.

- **Hubs: en línea sin abrir puertos.** Engranaje, **Multijugador**, **Entrar a un hub** lista los hubs del servidor de la
  comunidad: entra a uno, o usa **Crear un hub** para tus amigos (un nombre, y una contraseña si quieres). Los teléfonos y las PC
  juegan en los mismos hubs. Los hubs del MH3U son propios: los jugadores del MH3U y del MH4U nunca coinciden en uno.
- **Salas en tu red:** **Crear una sala**, y los demás **Buscar en esta red** en **Entrar a una sala**. Las salas con el
  formato de Azahar también funcionan con Azahar.
- **El chat:** una ventana sobre el juego, con pestañas, en salas y hubs; en la PC, **/** lo abre.
- **Esperar al jugador que se traba** (engranaje, **Multijugador**) detiene un momento el juego para un jugador con la
  conexión lenta.
- **Salir juntos a una misión:** en el muelle de la Taberna la A del anfitrión dice **Depart on Quest** y la de los demás
  **Stand By for Departure**, en el mismo lugar. Si la A del anfitrión muestra el nombre del otro cazador (parado justo
  detrás de él), da un paso al costado o bajo el arco hasta que diga **Depart on Quest**: entonces sale **Confirm
  Departure**.

Jugado durante el desarrollo: un teléfono y una PC en un hub y en una misma red, cada uno de anfitrión, con misiones juntos
de 8 a 10 minutos sin que nadie se cortara; dos teléfonos en un hub, con una misión juntos; y una misión entera entre un
teléfono anfitrión y una PC, con la pantalla única en los dos, de la salida a las recompensas y la vuelta a la Taberna.

## Si algo falla

| Problema | Qué hacer |
|---|---|
| La app dice que la carpeta no tiene el juego, o que tiene otro | La carpeta necesita Monster Hunter 3 Ultimate de EE. UU., descifrado. La copia europea todavía no es compatible |
| "No se instaló la app" | Hay una copia firmada con otra clave: exporta tu partida, desinstala esa copia, instala el APK oficial e importa la partida |
| La librería es de otra versión | Créala de nuevo: engranaje, **Sistema**, **Código del juego**, **Crear el código del juego** |
| El juego se cierra con un error | La pantalla de error tiene **Compartir registro**: adjunta el registro a un [issue](../../issues) con el modelo de tu teléfono, o pregunta en el [Discord](https://discord.gg/sk5xEjEMwF). No adjuntes archivos del juego |
| Anda lento o el teléfono se calienta | **Modo gama baja**, un **Límite de cuadros por segundo** o una **Resolución 3D** más bajos |

## PC (Windows)

La misma carpeta del juego se juega en una PC con Windows: `MH3U.exe` corre tu juego de forma nativa, con el mismo runtime,
las mismas opciones y funciones que la app, y lo que agrega una PC: teclado y ratón, ventanas de cualquier tamaño, FPS
altos y Discord.

La descarga, `MH3U-PC-<versión>.zip`, trae solo el programa: el código del juego (`mh3u_code_native.dll` y
`mh3u_code_native.id`) se crea una vez a partir de tu propia copia, con el mismo programa
([Paso 3 en PC](#paso-3-en-pc-crear-el-código-del-juego)). No hace falta nada más: ni Python ni un compilador propio.

### Qué hace en PC

Todo lo que tiene la app (arriba), con lo propio de la PC:

- **Teclado y ratón,** con las teclas del MH4U para PC: WASD mueve, el ratón gira la cámara, el clic izquierdo ataca, F es
  A, Shift izquierdo corre, Ctrl izquierdo camina, E envaina y usa objetos, un toque de **Alt** fija al monstruo (Alt+Tab
  no). Cada tecla se puede cambiar, hasta tres por botón, también los botones del ratón. **F2** muestra tus controles
  sobre el juego. Mira [Controles en PC](#controles-en-pc)
- **Fijar al monstruo:** un toque de **Alt** (o un clic de la palanca derecha) y la cámara sigue girando hacia el monstruo,
  también bajo el agua; **Tab** (ZL) pasa la Target Cam por los monstruos grandes
- **El árbol de armas de la herrería:** **Alt** (o el clic de la palanca derecha) mientras la lista de Forjar o Mejorar de
  la herrería muestra un arma
- **Escribir con el teclado:** el nombre de tu cazador y los otros textos del juego; en una sala o un hub, **/** abre el
  chat
- **La pantalla táctil al lado del juego** (lo de fábrica en la PC), debajo, en una esquina o en su propia ventana, con el
  ratón como tu dedo; o el modo de pantalla única, como en la app
- **Cualquier ventana:** pantalla completa sin bordes a la resolución de tu pantalla de fábrica, una ventana de cualquier
  tamaño, o pantalla completa exclusiva; **F11** o **Alt+Enter** cambia. La imagen toma la forma de la ventana al instante,
  con **Anclaje del HUD** para pantallas anchas
- **Cuadros por segundo:** límites de hasta 240, o **Sin límite**, con **VSync** prendido o apagado; el juego mantiene su
  velocidad normal con cualquier FPS. **Mostrar FPS y datos de la PC** muestra los FPS, y con **Completo** el uso de la GPU
  y de la CPU
- **Las opciones de imagen de la app,** con un **Filtro de escalado** para la imagen escalada (Suave, Píxeles nítidos,
  Vecino o FSR 1), **Salida HDR** en una pantalla con el HDR de Windows prendido, y **GPU** en una PC con dos placas de video
- **Un mando:** conéctalo y juega. START y SELECT juntos abren las opciones; los menús pueden mostrar los botones de tu
  mando (**Íconos de los botones**)
- **Discord:** con la app de Discord abierta en la misma PC, tu perfil muestra qué estás haciendo: la Aldea Moga o Puerto
  Tanzia, la misión que juegas, sus monstruos, el mapa y la zona, tu arma, y el nombre y el HR de tu cazador (cuando el
  Gremio de Puerto Tanzia te registró). **F1**, **Multijugador**, **Discord** lo apaga, o deja afuera tu nombre
- **Partidas, mods, trucos, misiones de evento, hubs y chat** como en la app, desde el mismo menú **F1**. Una PC y un
  teléfono juegan juntos en un hub o en una misma red
- Si el juego se cierra por un error, una ventana dice qué pasó y abre la carpeta del registro; el juego se detiene
  mientras su ventana está minimizada (**F1**, **Sistema**, **Pausar en segundo plano**)

### Qué necesitas en PC

- **Windows 10 u 11,** de 64 bits.
- **Una placa de video con Vulkan,** con su driver al día.
- **8 GB de RAM** recomendados.
- **Espacio libre:** unos 2 GB más que tu juego mientras se crea el código del juego (quedan unos 300 MB, las herramientas,
  que se pueden borrar después), y unos 200 MB para el programa y sus cachés.
- **Conexión a internet** la primera vez que se crea el código del juego: sus herramientas ocupan unos 90 MB.
- **Tu propia copia del juego, descifrada:** Monster Hunter 3 Ultimate de EE. UU. (título 00040000000AE400) como archivo
  `.3ds` o `.cci`. No tiene actualización: alcanza con el juego.

### Instalación en PC

Lo haces una vez.

#### Paso 1 en PC: pon tu juego en una carpeta

Tu juego en una carpeta propia, por ejemplo `C:\MH3U`. Si también juegas al MH4U en esta PC, la misma carpeta puede tener
los dos juegos: cada programa crea y lee su propio código del juego (`mh3u_code_native.dll`, `mh4u_code_native.dll`).

#### Paso 2 en PC: instálalo

No hay instalador: el programa es una carpeta.

1. Descomprime todo `MH3U-PC-<versión>.zip` en una carpeta tuya, como `C:\Juegos` (desde dentro del zip `MH3U.exe` no
   arranca). No en Archivos de programa: el programa guarda sus ajustes, cachés, registro y las herramientas que crean el
   código del juego en su propia carpeta, y Windows no lo deja escribir ahí.
2. Abre `MH3U.exe`. Windows puede decir que no conoce el programa, porque no está firmado: **Más información**, **Ejecutar
   de todas formas**.
3. La primera vez, una pantalla pide tu carpeta del juego: **Elegir carpeta**, y elígela. El programa la revisa y dice
   exactamente qué falta si algo está mal. Recuerda la carpeta.

#### Paso 3 en PC: crear el código del juego

El programa no trae código del juego, así que crea el tuyo con tu propia copia, una vez, en tu PC. No se envía nada a
ningún lado.

Cuando tu carpeta tiene el juego pero no el código del juego, la pantalla dice que falta `mh3u_code_native.dll`: toca
**Crear el código del juego en esta PC** y después **Crear**.

- La primera vez descarga sus herramientas, unos 90 MB: Python de python.org y Zig, un compilador de C, de PyPI, o de
  ziglang.org cuando PyPI no responde. Los dos son software libre, se comprueban con su SHA-256 antes de usarlos, y nunca
  se descarga nada del juego.
- Necesita unos 2 GB libres en el disco donde está `MH3U.exe` mientras trabaja. Al terminar quedan solo las herramientas
  (unos 300 MB, en la carpeta `builder` junto a `MH3U.exe`), y **Borrar las herramientas** también las quita.
- Tarda de unos 10 a 40 minutos, según la PC. Usa todos los núcleos del procesador menos uno, con prioridad baja, así
  puedes usar la PC mientras tanto.
- **Cancelar** lo detiene, igual que cerrar la ventana o reiniciar la PC: lo que hizo se guarda, y **Seguir** continúa desde
  ahí. **Empezar de cero** olvida un armado a medias y empieza desde el principio.

Cuando dice que el código del juego está listo, tu carpeta tiene dos archivos nuevos, `mh3u_code_native.dll` y
`mh3u_code_native.id`, y **Jugar** abre el juego. **Estos archivos se hacen con tu juego: guárdalos para ti y no los
compartas.**

El zip trae una carpeta con los archivos del programa y nada del juego: `MH3U.exe` y `SDL3.dll`; `zigcc.exe` y
`builder_tools.zip`, lo que usa el creador; `code_library_id.txt`, el id del código del juego que necesita esta versión
(el `mh3u_code_native.id` de tu carpeta tiene que decir lo mismo); `cheats`, los trucos opcionales de la app; `licenses`;
y `README.txt`, una guía corta.

### Controles en PC

- **Teclado y ratón** (las mismas teclas que el MH4U y el MHGU para PC): WASD mueve, Ctrl izquierdo camina, I, J, K, L
  giran la cámara. El clic izquierdo o R es X; F o el clic derecho es A (el clic derecho solo con el arma afuera, en un
  menú o en una charla); Espacio es B; E o el botón del medio es Y (envainar, usar un objeto); Q es L, Shift izquierdo es
  R (correr, cubrirse, apuntar); Tab y T son ZL y ZR; Enter es START, Retroceso SELECT, y las flechas son la cruceta. M (o
  Ctrl derecho) muestra u oculta la pantalla táctil. Un toque de Alt fija al monstruo (Alt+Tab no). La rueda del ratón
  pasa los objetos de la barra. Haz clic en la ventana para girar la cámara con el ratón; Esc suelta el ratón.
- **Las teclas propias del programa:** **F1** las opciones, **F2** la lista de los controles, **F11** o **Alt+Enter**
  pantalla completa o ventana, **1** el panel de la Bolsa de la pantalla táctil en una misión, **G** los Gestos del pueblo,
  y **/** el chat en una sala o un hub.
- **Cada tecla se puede cambiar,** hasta tres por botón, también los botones del ratón: **F1**, **Controles**, **Teclado y
  ratón**, **Asignación de teclas**. Ahí también se invierten los ejes del ratón. Si no habías cambiado ninguna tecla, las de un armado
  anterior pasan solas a estas; con una tecla tuya, no se cambia nada.
- **La pantalla táctil:** un clic en ella es un toque.
- **El menú de opciones (F1)** funciona con el ratón, con el teclado (las flechas, Enter, Esc, Tab y Re Pág/Av Pág cambian
  de sección, Ctrl+S guarda) o con un mando.
- **Mando:** conéctalo y juega. START y SELECT juntos abren las opciones. La palanca derecha gira la cámara; en los
  pueblos, donde la cámara del juego es fija, es la del Circle Pad Pro, como en la consola. Si el juego no lee bien tu
  mando: **Controles**, **Mando**, **Mapear un mando**.

### Partidas en PC

Tu partida está en `%APPDATA%\MH3U` (escríbelo en la barra de direcciones del Explorador), aparte de la del MH4U para PC,
así una versión nueva descomprimida en otro lado la encuentra. **F1**, **Sistema**, **Tu partida**: **Importar partida**,
**Exportar partida** y **Carpeta de partidas** (no se pisa nada sin preguntar: la partida que no sigue queda en
`%APPDATA%\MH3U\save_backups`).

### Dónde se guarda cada cosa en PC

| Qué | Dónde |
|---|---|
| Los ajustes, la carpeta que elegiste, el registro | Junto a `MH3U.exe`: `mh4u-app.ini`, `game_folder.txt`, `log.txt` (las sesiones anteriores: `log_prev.txt`, `log_prev2.txt`) |
| Los archivos chicos del juego, los ajustes propios del juego, trucos, mods | Junto a `MH3U.exe`: `game`, `user` (`mh4u.ini`, `cheats`, `load/mods`) |
| La caché de shaders | `%LOCALAPPDATA%\MH3U\cache` |
| Las herramientas que crean el código del juego, y su registro | Junto a `MH3U.exe`: `builder`, `builder_log.txt` |
| Tu partida | `%APPDATA%\MH3U` (o la carpeta que elegiste en **Carpeta de partidas**) |
| Las copias de las partidas que un cambio reemplazó | `%APPDATA%\MH3U\save_backups` |
| Tu juego, el código del juego, mods y packs HD | Tu carpeta del juego: `mh3u_code_native.dll` y `.id`, `mods`, `textures` |

### Actualizar o desinstalar la versión de PC

**Actualizar:** descomprime la versión nueva en una carpeta nueva y abre su `MH3U.exe`; tu partida está en
`%APPDATA%\MH3U` y se encuentra sola. Si dice que el código del juego es de otra versión, toca **Crear el código del juego
en esta PC**.

**Desinstalar:** borra la carpeta del programa, la que tiene `MH3U.exe`. Tu partida queda en `%APPDATA%\MH3U`, y la caché
de shaders en `%LOCALAPPDATA%\MH3U`: bórralas también solo si quieres que desaparezcan (guarda antes una copia de la
partida). No se instala nada más.

### Si algo falla en PC

| Problema | Qué hacer |
|---|---|
| "No se encontró SDL3.dll" | Descomprime el zip entero en una carpeta antes: `MH3U.exe` no arranca desde dentro del zip |
| "Acá no hay un juego ... El juego que tiene es otro" | La carpeta tiene otro juego (MH4U, o el MH3U de otra región): este programa corre Monster Hunter 3 Ultimate de EE. UU. |
| "La librería del código del juego (mh3u_code_native.dll) no está en la carpeta" | La descarga no la trae: toca **Crear el código del juego en esta PC** en esa pantalla |
| "Este mh3u_code_native.dll es de otra versión" | Se hizo para otra versión: toca **Crear el código del juego en esta PC**, y se vuelve a hacer para esta |
| Se cierra al arrancar, o la ventana queda negra | Actualiza el driver de tu placa de video |
| El ratón no gira la cámara | Haz clic en la ventana primero. Esc suelta el ratón |
| Nadie puede entrar al hub que abres | El Firewall de Windows pregunta la primera vez si MH3U puede usar la red: permítelo |

Si el juego se cierra por un error, una ventana lo dice; **Abrir la carpeta del registro** abre la carpeta con `log.txt`.
Adjúntalo a un [issue](../../issues) con el procesador y la placa de video de tu PC, o pregunta en el
[Discord](https://discord.gg/sk5xEjEMwF). No adjuntes archivos del juego.

## Armar la app

Para quien desarrolla; los jugadores solo necesitan el APK o el zip de PC de Releases. En Windows (Git Bash), Linux o
macOS, con el SDK de Android (plataforma 35) y el NDK 27.0.12077973, CMake 3.25 o más nuevo, Ninja, Python 3.10 o más
nuevo y un JDK 17:

```
sh scripts/build_sdl3.sh    # SDL 3.4.18 desde su release, una vez
sh scripts/build_apk.sh     # libmain.so, las librerías de los hooks, los lanzadores y las herramientas del creador del teléfono, y Gradle
```

`scripts/android_env.sh` dice dónde se busca cada herramienta. Firma con tu propia clave en `android/keystore.properties` y
`android/mh3u.keystore`; sin ellas sale un APK de depuración. `app_native/` es la app (SDL3 y Dear ImGui), `android/` su
lado de Android, `app_native/pc/` el de la PC, y `mh3n/` el runtime y el recompilador (mira [mh3n/README.md](mh3n/README.md)).

La versión de PC, en Git Bash en Windows, con [llvm-mingw](https://github.com/mstorsjo/llvm-mingw/releases) 20241217 para
x86_64 con la UCRT, CMake 3.25 o más nuevo, Ninja y Python 3.10 o más nuevo:

```
LLVM_MINGW=C:/llvm-mingw-20241217-ucrt-x86_64 bash scripts/build_pc.sh            # build/pc/MH3U, como lo descomprime un jugador
LLVM_MINGW=C:/llvm-mingw-20241217-ucrt-x86_64 PACK=1 bash scripts/build_pc.sh     # y el zip de la release con su .sha256
```

La primera vez arma SDL 3.4.18 para Windows desde su release.

La librería del código del juego para la app también se puede crear en una computadora, con tu propia copia (Python 3.10
o más nuevo; en Windows `--fetch-compiler` descarga llvm-mingw, en los demás se usa clang o el compilador de C de Zig):

```
python mh3n/tools/build_game.py --dump <tu juego .cci> --native --android --fetch-compiler --out build/game
```

Escribe `libmh4u_code_native.so` y su `.id` en `build/game/recomp/android/`: copia los dos a tu carpeta del juego en el
teléfono.

## Aviso legal

Este proyecto no está afiliado, respaldado ni patrocinado por la Gran C ni la Gran N. Monster Hunter es una marca de la
Gran C.

Este repositorio y sus versiones no contienen código del juego, datos del juego, ROMs ni keys. Úsalo solo con una copia del
juego que te pertenezca. Los pedidos de archivos del juego, keys o archivos `libmh4u_code_native.so` o
`mh3u_code_native.dll` ya hechos se cierran.

Titulares de derechos: abran un issue para ponerse en contacto.

## Créditos

- El recompilador del juego y el runtime nativo vienen de [MH4UltimateDC](https://github.com/CompaLarios18/MH4UltimateDC),
  de CompaLarios18 y sus colaboradores.
- Partes del runtime derivan de [Azahar](https://github.com/azahar-emu/azahar) y Citra.
- MH3U para Android y PC está hecho a partir de [MH4U para Android y PC](https://github.com/khorne-hub/4u-native), del
  mismo autor; también del mismo autor, [MHGU para Android y PC](https://github.com/khorne-hub/gu-native). Partes de la
  versión de PC (sus datos de la PC, su lector de zip y Discord Rich Presence) siguen a las del MHGU para PC.
- Aportaron al desarrollo:
  - tabarong
  - Klark231
  - Alfonso Matador
  - popzol
  - BlitzWOLF
  - jM5557
- El creador de la app, que crea el código del juego en el teléfono, la primera vez descarga Python
  ([python.org](https://www.python.org), licencia PSF) y [Zig](https://ziglang.org) (MIT; con LLVM, Apache 2.0 con
  excepciones de LLVM); sus licencias también están en **Licencias** de la app.
- Las librerías que usa este proyecto, con sus licencias, están en [THIRD_PARTY_NOTICES](THIRD_PARTY_NOTICES). Sus
  licencias también están en la app: engranaje, **Créditos y licencias**, **Licencias**.

## Descargas oficiales

MH3U para Android y PC es gratis. Las únicas descargas oficiales son los enlaces que da khorne-hub, empezando por las
[Releases](https://github.com/khorne-hub/3u-native/releases) de este repositorio. Si lo conseguiste en otro lado, pagaste
por él o tu copia trae publicidad, no es la oficial. En la app, el botón de GitHub arriba en el menú de opciones, junto a
**Guardar**, abre esta página en tu navegador, y el botón de Discord a su lado el [Discord](https://discord.gg/sk5xEjEMwF)
de la comunidad.

Todos los APK oficiales están firmados con la misma clave, la propia del MH3U. La huella SHA-256 de su certificado es:

```
83:7B:CE:4A:04:04:FB:5D:F2:BC:C0:46:BE:91:0A:D0:1F:87:E6:52:98:BF:75:5E:87:55:9C:A2:EC:F6:BE:D3
```

Para revisar un APK: `apksigner verify --print-certs MH3U-<versión>.apk` (build-tools del SDK de Android). Una copia
firmada con otra clave no se puede instalar encima de la app oficial, y la app lo avisa una vez al arrancar.

La versión de PC es el zip `MH3U-PC-<versión>.zip`, con su SHA-256 al lado en `MH3U-PC-<versión>.zip.sha256`. Para
revisar el tuyo, en un Símbolo del sistema: `certutil -hashfile MH3U-PC-<versión>.zip SHA256`, y compáralo con ese archivo.

La app también te avisa cuando sale una versión nueva. Como mucho una vez al día le pregunta a GitHub por la última
versión de este repositorio (no se envía nada más, ni siquiera el modelo del teléfono), y en el próximo inicio ofrece
abrir la página de la versión. Nunca descarga ni instala nada por su cuenta. Engranaje, **Sistema**, **Buscar
actualizaciones** lo apaga.

## Licencia

MH3U para Android y PC es software libre bajo la [Licencia Pública General de GNU, versión 3](LICENSE) o (a tu elección)
cualquier versión posterior, como el runtime de MH4UltimateDC sobre el que se construye. Las partes de terceros mantienen
sus propias licencias.

El código de khorne-hub lleva [términos adicionales](ADDITIONAL-TERMS), que la sección 7 de la GPL permite. Quien comparta
la app, cambiada o no, tiene que:

- mantener los créditos del autor: los que la app muestra al abrirse, su página **Créditos y licencias**, y las secciones
  **Créditos** y **Licencia** de este archivo;
- marcar una versión cambiada como cambiada, y no presentarla como la oficial;
- dar, junto con la app, su licencia y su código fuente completo, como pide la GPL.
