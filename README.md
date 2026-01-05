Goldstein capitulos 1 y 2 laboratorio con OpenGL

Proposito

Este proyecto es un visor interactivo para estudiar mecanica de una particula.
Muestra un punto que se mueve en el plano y dibuja
un vector verde para la velocidad
un vector rojo para la aceleracion
una estela para la trayectoria

Cada segundo imprime en la terminal
energia cinetica mas potencial
momento angular en el eje z
posicion y velocidad

Escenas

1 Particula libre
2 Fuerza constante hacia abajo con arrastre
3 Oscilador armonico con arrastre
4 Fuerza central inversa al cuadrado tipo Kepler
5 Restriccion holonoma a un circulo con una fuerza hacia el origen
6 Oscilador con rigidez dependiente del tiempo
7 Noether por rotacion en fuerza central imprime velocidad areolar
8 Noether por traslacion en x gravedad sin arrastre imprime px

Integradores

I alterna entre Euler semimplicito y Runge Kutta 4.
La diferencia se nota cuando subes el paso de tiempo.

Controles

1 a 8 Cambiar escena
Espacio Pausa
R Reiniciar
I Cambiar integrador
Flecha arriba y abajo Ajustar paso de tiempo
Flecha izquierda y derecha Ajustar intensidad k
H Mostrar ayuda en la terminal

Compilacion con CMake

Requisitos

Un compilador de C mas mas
CMake
GLFW

El proyecto intenta usar GLFW del sistema.
Si no lo encuentra, CMake descarga GLFW.

Linux

cmake -S . -B build
cmake --build build -j
./build/goldstein_cap1

Windows con Visual Studio

cmake -S . -B build
cmake --build build --config Release
build\Release\goldstein_cap1.exe

macOS

cmake -S . -B build
cmake --build build -j
./build/goldstein_cap1

Como leer lo que ves

Vector verde es velocidad.
Vector rojo es aceleracion.
Si el rojo es perpendicular al verde, la rapidez se conserva y solo gira la direccion.
En la fuerza inversa al cuadrado, si eliges un paso de tiempo razonable, la energia y el momento angular casi se conservan.
En la restriccion de circulo, la posicion se proyecta al circulo y la velocidad se vuelve tangencial.
Eso imita la idea de fuerzas de ligadura del capitulo 1.

Objetivo didactico

Capitulo 1 trata de describir el movimiento y luego imponer leyes.
Este programa te deja ver
que significa aceleracion como derivada de la velocidad
que significa fuerza como causa del cambio de momento lineal
y como una ligadura puede imponer una geometria al movimiento

Capitulo 2 trata de principios variacionales, simetrias y conservaciones.
Este programa te deja comprobar tres ideas que suelen decirse como dogma

Si el lagrangiano no depende del tiempo, la energia se conserva.
En la escena 6 la rigidez cambia con el tiempo y la energia deja de ser constante.

Si el lagrangiano es invariante bajo rotaciones, el momento angular se conserva.
En la escena 7 se imprime Lz y tambien la velocidad areolar para ver la segunda ley de Kepler en accion.

Si el lagrangiano no depende de una coordenada, su momento conjugado se conserva.
En la escena 8 el potencial depende de y pero no de x, por eso px se mantiene constante.
