# Changelog

[Español más abajo](#español)

## 1.0.2.1

It installs over 1.0.2, and your code library stays valid: there's no need to build it again.

- **Light and shadows on wide screens (20:9 and wider):** the sunlight and the shadows were missing on these screens (a
  darker picture, no shadows under hunters and monsters): the top picture was too tall for the game's own video memory.
  It is now drawn at most 520 rows tall and widened to your screen; at 19.5:9 and narrower nothing changes.
- **Low-end mode no longer darkens the game:** MH3U's bloom is part of its lighting, so Low-end mode leaves it on. If you
  had Low-end mode on, the bloom comes back on once, at the first start of 1.0.2.1.
- **The log no longer grows without limit:** two messages that filled it (thousands of lines a minute on some phones) are
  said a few times with their count, and a session's log stops at 16 MB.

## 1.0.2

The first version of MH3U for Android and PC: Monster Hunter 3 Ultimate, recompiled the way MH4U for Android and PC is,
with everything MH4U for Android and PC 1.0.2 has that exists in MH3U. It starts at 1.0.2 so the three ports (MHGU, MH4U
and MH3U) go together from now on.

The release has the APK, `MH3U-1.0.2.apk`, and the PC version, `MH3U-PC-1.0.2.zip`, each with its SHA-256. Neither
has anything of the game: the app and the program build the game's code from your own copy, once (on the phone about an
hour and a half on a low-end one; on a PC 10 to 40 minutes).

### What you need

- Your own copy of Monster Hunter 3 Ultimate for the USA (title 00040000000AE400), decrypted, as a `.3ds` or `.cci`. It
  has no update. The European copy isn't supported yet.
- Android 9 or newer, 64-bit, with Vulkan; or Windows 10 or 11, 64-bit, with a Vulkan graphics card.

### On Android

- **The game's code built on the phone,** with no PC, as MH4U's: **Cancel**, **Go on**, **Start over**.
- **The picture:** your phone's own resolution, widescreen from 4:3 to ultra-wide with the HUD at the edges, 15 to 240
  fps with the game at its own speed, **Steady frame rate**, **Low-end mode** with **Toaster** and **Brick**, frame
  generation, FSR 1, **Fast scaling (more FPS)**, the PS1 filter, **Colour** with three presets, switches for shadows,
  effects, each screen filter and reverb, the field of view, HD texture packs (KTX2 and half size too) and your own GPU
  driver.
- **Single-screen mode as MH4U's:** the game itself draws the touch screen's menus on the main screen, sharp and beside
  what they belong to (the main menu, the save's choice, character creation, START and its Item Pouch, the item boxes,
  the supply box, the shops with how many and the total, the Smithy and the Armory, the quest counters and the Tavern's
  Quest Board, the rewards, Cha-Cha and Kayamba, the farm), worked with the buttons, the menu pad or a slide; OK and Back
  are A and B; nothing covers a talk, a sign or the hunter. The touch screen's button brings it back with the menus on it.
- **The app in the colours of water:** the settings, the buttons, the editor and the app's screens in the sea green and
  gold of MH3U's icon.
- **Touch controls** made for MH3U: the controls follow the game (the hunt, the menus, the talks, bows and bowguns), the
  menu pad (also in the supply box, the rewards and "Save your progress?"), the item bar without the D-pad, lock on (also
  under water), the area map as one piece with the hunter's arrow and the monsters' icons, every part of the HUD moved,
  resized or hidden with its frame right on it at any screen shape, HUD size, 10 profiles (as `.zip` too), 15 combined
  buttons, Hold, your own pictures, gyro aim with a bowgun.
- **The camera up and down on land too:** a drag moves it smoothly through the game's six heights; under water it turns
  every way and swims you up and down.
- **The HUD at 4:3** and with any field of view, whole and in its place.
- **A keyboard and a mouse on the phone:** held keys walk and turn, a captured mouse turns the camera.
- **Smoother:** the game's own short stops every 15 seconds or so (its local session being rebuilt) are gone, and the
  renderer leaves out what a hidden touch screen would draw.
- **Hunting aids** (optional): damage numbers with **By hit**'s colours, the monsters' health bars with their names, the
  monsters on the map, the bow aimed at the monster.
- **Comforts** (optional): gather everything in one press or hold A, move to cancel the flex, B cancels the sharpening, the
  rewards with no time limit, ZL+ZR ends the wait, **Send all to the box**, faster eating and drinking cheats, and the
  Smithy's weapon tree with the game's icons, from the Forge and the Upgrade lists.
- **Saves** from Azahar, Citra, Checkpoint and JKSM, export, a save folder of your own with `save_backups`; event quests;
  mods with their load order and conflicts; cheats in Azahar's format.
- **Multiplayer** in Port Tanzia: hubs on the internet with no ports to open, rooms on your network and Azahar's, phones
  and PCs together, the chat, a whole quest together. MH3U's hubs and rooms are its own: they never mix with MH4U's. At
  the pier, if the host's A shows the other hunter's name, a step aside makes it **Depart on Quest**.
- Verify game files, the error screen with Share log, the game waits while the settings are open and while the app is out
  of sight, a home screen shortcut, update notices.

### On PC (MH3U for PC)

- `MH3U.exe`, MH4U for PC 1.0.2's program on MH3U's game: it builds the game code itself from your game, with keyboard and
  mouse (MH4U for PC's keys), any window, up to 240 fps or no limit, VSync, HDR, the touch screen beside the game or
  single-screen mode, F1 (the app's settings), F2 (your controls), Discord with the place, the quest, the monsters, your
  weapon and HR, controllers, and multiplayer with phones.

### Found and fixed while making it (also in MH4U's code, for its next version)

- The game kept running behind the home screen on some phones (Android 15): now it waits from the moment the app is out
  of sight.
- A host now drops a player who left a room without leaving the network (the app closed, a crash, the network gone).
- The rewards screen's background could show noise when the game's shaders were still being prepared.
- A host could close when a guest dropped out of a quest (the picture behind the supply box, written back late over
  memory the game had given to the hunters' armour): the late write-back now leaves alone what the game wrote after.
- After moving a piece in Customize controls and HUD, the game runs a moment more, so the piece is drawn where its frame
  is; a keyboard key held on a phone no longer lets go by itself, and a captured mouse turns the right way on Android 12
  and 13.

---

## Español

## 1.0.2.1

Se instala encima de la 1.0.2, y tu librería de código sigue sirviendo: no hace falta crearla de nuevo.

- **Luz y sombras en pantallas anchas (20:9 o más):** en esas pantallas faltaban la luz del sol y las sombras (una imagen más
  oscura, sin sombras debajo de cazadores y monstruos): la imagen de arriba era demasiado alta para la memoria de video del
  propio juego. Ahora se dibuja con 520 filas como mucho y se ensancha a tu pantalla; a 19,5:9 o menos no cambia nada.
- **El modo gama baja ya no oscurece el juego:** el resplandor del MH3U es parte de su luz, así que el modo gama baja lo deja
  prendido. Si tenías el modo gama baja prendido, el resplandor se vuelve a prender una vez, en el primer inicio de la
  1.0.2.1.
- **El registro ya no crece sin control:** dos mensajes que lo llenaban (miles de líneas por minuto en algunos teléfonos) se
  dicen pocas veces con su cuenta, y el registro de una sesión se corta a los 16 MB.

## 1.0.2

La primera versión de MH3U para Android y PC: Monster Hunter 3 Ultimate, recompilado como MH4U para Android y PC, con todo
lo que tiene MH4U para Android y PC 1.0.2 que existe en MH3U. Empieza en la 1.0.2 para que los tres ports (MHGU, MH4U y
MH3U) vayan a la par desde ahora.

La release trae el APK, `MH3U-1.0.2.apk`, y la versión de PC, `MH3U-PC-1.0.2.zip`, cada uno con su SHA-256. Ninguno trae
nada del juego: la app y el programa crean el código del juego con tu propia copia, una vez (en el teléfono, alrededor de
una hora y media en uno de gama baja; en una PC, de 10 a 40 minutos).

### Qué necesitas

- Tu propia copia de Monster Hunter 3 Ultimate de EE. UU. (título 00040000000AE400), descifrada, como `.3ds` o `.cci`. No
  tiene actualización. La copia europea todavía no es compatible.
- Android 9 o más nuevo, de 64 bits, con Vulkan; o Windows 10 u 11, de 64 bits, con una placa de video con Vulkan.

### En Android

- **El código del juego se crea en el teléfono,** sin PC, como en el MH4U: **Cancelar**, **Seguir**, **Empezar de cero**.
- **La imagen:** la resolución de tu teléfono, pantalla ancha de 4:3 a ultra ancha con el HUD en los bordes, de 15 a 240
  FPS con el juego a su velocidad, **Ritmo estable**, **Modo gama baja** con **Tostadora** y **Ladrillo**, generación de
  cuadros, FSR 1, **Escalado rápido (más FPS)**, el filtro PS1, **Color** con tres presets, interruptores para sombras,
  efectos, cada filtro de pantalla y la reverberación, el campo de visión, packs de texturas HD (también KTX2 y a mitad de
  tamaño) y tu propio driver de GPU.
- **Modo de pantalla única como el del MH4U:** el propio juego dibuja los menús de la pantalla táctil en la principal,
  nítidos y al lado de lo que acompañan (el menú principal, la elección de partida, la creación del personaje, START y su
  Bolsa, las cajas de objetos, la caja de suministros, las tiendas con cuántos y el total, la herrería y la armería, los
  mostradores de misiones y el tablón de la Taberna, las recompensas, Cha-Cha y Kayamba, la granja), y se manejan con los
  botones, el pad de menú o deslizando; OK y Volver son A y B; nada tapa una charla, un cartel ni al cazador. El botón de
  la pantalla táctil la trae con los menús en ella.
- **La app con los colores del agua:** las opciones, los botones, el editor y las pantallas de la app en el verde mar y el
  dorado del ícono del MH3U.
- **Controles táctiles** hechos para el MH3U: los controles siguen al juego (la caza, los menús, las charlas, arcos y
  ballestas), el pad de menú (también en la caja de suministros, las recompensas y "Save your progress?"), la barra de
  objetos sin la cruceta, fijar al monstruo (también bajo el agua), el mapa de la zona como una pieza con la flecha del
  cazador y los íconos de los monstruos, cada parte del HUD movida, con otro tamaño u oculta con su marco justo encima en
  cualquier forma de pantalla, el tamaño del HUD, 10 perfiles (también como `.zip`), 15 botones combinados, Mantener, tus
  propias imágenes, apuntar con el giroscopio con una ballesta.
- **La cámara arriba y abajo también en tierra:** deslizar la mueve suave por las seis alturas del juego; bajo el agua gira
  hacia todos lados y te hace nadar arriba y abajo.
- **El HUD en 4:3** y con cualquier campo de visión, entero y en su lugar.
- **Teclado y ratón en el teléfono:** las teclas mantenidas caminan y giran, un ratón capturado gira la cámara.
- **Más parejo:** ya no están las paradas cortas del propio juego cada 15 segundos más o menos (al rehacer su sesión
  local), y el renderer deja afuera lo que dibujaría una pantalla táctil oculta.
- **Ayudas de caza** (opcionales): números de daño con los colores de **Por golpe**, las barras de vida de los monstruos
  con sus nombres, los monstruos en el mapa, el arco apuntado al monstruo.
- **Comodidades** (opcionales): recolectar todo de un toque o manteniendo A, moverse cancela el flex, B cancela el
  afilado, las recompensas sin límite de tiempo, ZL+ZR termina la espera, **Enviar todo a la caja**, trucos para comer y
  beber más rápido, y el árbol de armas de la herrería con los íconos del juego, desde las listas de Forjar y de Mejorar.
- **Partidas** desde Azahar, Citra, Checkpoint y JKSM, exportar, una carpeta de partidas propia con `save_backups`;
  misiones de evento; mods con su orden de carga y sus conflictos; trucos en el formato de Azahar.
- **Multijugador** en Puerto Tanzia: hubs en internet sin abrir puertos, salas en tu red y las de Azahar, teléfonos y PC
  juntos, el chat, una misión entera juntos. Los hubs y las salas del MH3U son propios: nunca se mezclan con los del MH4U.
  En el muelle, si la A del anfitrión muestra el nombre del otro cazador, un paso al costado la vuelve **Depart on Quest**.
- Verificar archivos del juego, la pantalla de error con Compartir registro, el juego espera mientras las opciones están
  abiertas y mientras la app no está a la vista, un acceso directo en el inicio, avisos de versiones nuevas.

### En PC (MH3U para PC)

- `MH3U.exe`, el programa del MH4U para PC 1.0.2 sobre el juego del MH3U: crea el código del juego por su cuenta con tu
  juego, con teclado y ratón (las teclas del MH4U para PC), cualquier ventana, hasta 240 FPS o sin límite, VSync, HDR, la
  pantalla táctil al lado del juego o el modo de pantalla única, F1 (las opciones de la app), F2 (tus controles), Discord
  con el lugar, la misión, los monstruos, tu arma y tu HR, mandos, y multijugador con teléfonos.

### Encontrado y arreglado al hacerlo (también en el código del MH4U, para su próxima versión)

- En algunos teléfonos (Android 15) el juego seguía corriendo detrás de la pantalla de inicio: ahora espera desde el
  momento en que la app deja de estar a la vista.
- Un anfitrión ahora saca a un jugador que se fue de una sala sin salir de la red (la app cerrada, un cierre por error, la
  red caída).
- El fondo de la pantalla de recompensas podía mostrar ruido cuando los shaders del juego todavía se estaban preparando.
- Un anfitrión podía cerrarse cuando un invitado se cortaba en una misión (la imagen detrás de la caja de suministros,
  escrita tarde sobre la memoria que el juego ya había dado a las armaduras de los cazadores): esa escritura tardía ahora
  deja como está lo que el juego escribió después.
- Después de mover una pieza en Personalizar controles y HUD el juego corre un momento más, así la pieza se dibuja donde
  está su marco; una tecla mantenida de un teclado en el teléfono ya no se suelta sola, y un ratón capturado gira para el
  lado correcto en Android 12 y 13.
