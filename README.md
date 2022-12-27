# ***tp-2022-1c-TheBug***

## **Compiling**

gcc -o procesador procesador.c ../../utils/src/utils.c -lcommons -pthread -lm
gcc -o kernel kernel.c ../../utils/src/utils.c -lcommons -pthread -lm
gcc -o terminal terminal.c ../../utils/src/utils.c -lcommons -pthread -lm
gcc -o memoria memoria.c ../../utils/src/utils.c -lcommons -pthread -lm

---

## **Uso de Makefile**
Cada modulo tiene su makefile, por consola se puede mandar:
- make -> compila [todos los modulos]
- make clean -> borra el ejecutable y el log de ese modulo [todos los modulos]
- make start -> ejecuta [todos los modulos menos terminal]\

Hay un makefile global, se puede mandar:
- make -> compila todos los modulos de una
- make clean -> borra ejecutable y log de todos los modulos de una
- make commons -> clona el repo de las commons en el directorio de afuera del tp, lo compila y lo instala
- make tests -> clona el repo de las pruebas de la catedra en el directorio de afuera del tp

## **Make para los configs**

Se pueden mandar makes para los configs. El programa va a leer el config respectivo de cada modulo desde la ruta /home/utnso/Escritorio por lo que los make particulares para cada prueba van a sacar los .config de kernel, procesador y memoria de una carpeta llamada configs que tiene todo. Comandos a usar
- make base -> Elimina si hay los .config y copia los archivos para las pruebas BASE [IMPORTANTE: Unico make que copia el terminal.config, arrancar con este make]
- make plani -> Elimina si hay los .config y copia los archivos para las pruebas de PLANIFICACION
- make suspe -> Elimina si hay los .config y copia los archivos para las pruebas de SUSPENSION
- make clock -> Elimina si hay los .config y copia los archivos para las pruebas de CLOCK
- make tlb -> Elimina si hay los .config y copia los archivos para las pruebas de TLB
- make integral -> Elimina si hay los .config y copia los archivos para las pruebas INTEGRALES
- make remconfigs -> Elimina los 4 archivos .config por las dudas

### **Make para las entregas en el labo**

Dentro de cada makefile de cada módulo se pueden hacer los mismos makes que en el config global, con la diferencia que estos tienen una "e" (entrega) antes del nombre, ej. "make **e**base", "make **e**suspe" y así. \
Estos make van a copiar tal cual el config de cada prueba, cambiando que ahora los puertos en lugar de tener el "127.0.0.1" van a tener un "0.0.0.0" para poder conectar las pcs en el labo y por ultimo ya te van a dejar parado en un "nano ..." al archivo que se tenga que cambiar en el momento con la ip propia de la pc del labo de Medrano. ---> (esto todavía tengo que ver cual archivo es el que hace el nano cada módulo)

---

## **Pruebas finales**
↓↓↓ Repo de las pruebas de la cátedra ↓↓↓ \
https://github.com/sisoputnfrba/kiss-pruebas.git -\
Para simplicidad (no tener que clonar tambien este repo para la entrega) tenemos una carpeta que contiene al día de la fecha los mismos txt para las pruebas. En caso de que en un futuro se actualicen, ahí está el repo.