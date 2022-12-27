### Reglas ###

all:
	make -C procesador/src
	make -C kernel/src
	make -C terminal/src
	make -C memoria/src	

# Clean

clean:
	make clean -C procesador/src
	make clean -C kernel/src
	make clean -C terminal/src
	make clean -C memoria/src

# Commons

COMMONS=so-commons-library
TP=tp-2022-1c-TheFix

commons:
	cd .. && \
	git clone "https://github.com/sisoputnfrba/$(COMMONS).git" && \
	cd $(COMMONS) && \
	sudo make uninstall && \
	make all && \
	sudo make install &&\
	cd .. && \
	cd $(TP)

# Pruebas

PRUEBAS=kiss-pruebas

tests:
	cd .. && \
	git clone "https://github.com/sisoputnfrba/$(PRUEBAS).git" && \
	cd $(TP)

# CONFIGS
# TODO: que no copie el config de una carpeta propia sino directamente del repo de pruebas, si hay tiempo lo hago despues -> (matiP)

base:
	cd /home/utnso/Escritorio && \
	rm -rf kernel.config && \
	rm -rf procesador.config && \
	rm -rf memoria.config
	cp configs/base/kernel.config /home/utnso/Escritorio/ && \
	cp configs/base/procesador.config /home/utnso/Escritorio/ && \
	cp configs/base/memoria.config /home/utnso/Escritorio/ && \
	cp configs/base/terminal.config /home/utnso/Escritorio/

plani:
	cd /home/utnso/Escritorio && \
	rm -rf kernel.config && \
	rm -rf procesador.config && \
	rm -rf memoria.config
	cp configs/planificacion/kernel.config /home/utnso/Escritorio/ && \
	cp configs/planificacion/procesador.config /home/utnso/Escritorio/ && \
	cp configs/planificacion/memoria.config /home/utnso/Escritorio/

suspe:
	cd /home/utnso/Escritorio && \
	rm -rf kernel.config && \
	rm -rf procesador.config && \
	rm -rf memoria.config
	cp configs/suspension/kernel.config /home/utnso/Escritorio/ && \
	cp configs/suspension/procesador.config /home/utnso/Escritorio/ && \
	cp configs/suspension/memoria.config /home/utnso/Escritorio/

clock:
	cd /home/utnso/Escritorio && \
	rm -rf kernel.config && \
	rm -rf procesador.config && \
	rm -rf memoria.config
	cp configs/clock/kernel.config /home/utnso/Escritorio/ && \
	cp configs/clock/procesador.config /home/utnso/Escritorio/ && \
	cp configs/clock/memoria.config /home/utnso/Escritorio/

tlb:
	cd /home/utnso/Escritorio && \
	rm -rf kernel.config && \
	rm -rf procesador.config && \
	rm -rf memoria.config
	cp configs/tlb/kernel.config /home/utnso/Escritorio/ && \
	cp configs/tlb/procesador.config /home/utnso/Escritorio/ && \
	cp configs/tlb/memoria.config /home/utnso/Escritorio/

integral:
	cd /home/utnso/Escritorio && \
	rm -rf kernel.config && \
	rm -rf procesador.config && \
	rm -rf memoria.config
	cp configs/integral/kernel.config /home/utnso/Escritorio/ && \
	cp configs/integral/procesador.config /home/utnso/Escritorio/ && \
	cp configs/integral/memoria.config /home/utnso/Escritorio/

remconfigs:
	cd /home/utnso/Escritorio && \
	rm -rf kernel.config && \
	rm -rf procesador.config && \
	rm -rf memoria.config && \
	rm -rf terminal.config	

# NGROK

ngrok:
	cd .. && \
	wget https://faq.utnso.com.ar/ngrok.zip && \
	unzip ngrok.zip && \
	./ngrok --version