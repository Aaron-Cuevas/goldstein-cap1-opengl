Goldstein capitulo 1 laboratorio con OpenGL

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

Integradores

I alterna entre Euler semimplicito y Runge Kutta 4.
La diferencia se nota cuando subes el paso de tiempo.

Controles

1 a 5 Cambiar escena
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
