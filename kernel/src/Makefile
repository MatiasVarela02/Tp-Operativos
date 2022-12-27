# Globales
CC = gcc
OUTFILE = kernel
UTILS =../../utils/src/utils.c
LINK = -lcommons -lpthread -lm

### Reglas ###

build:
	$(CC) -o $(OUTFILE) $(OUTFILE).c $(UTILS) $(LINK)

# Clean
clean:
	rm -rf $(OUTFILE)
	rm -rf *.log

start:
	./$(OUTFILE)

# gcc -o kernel kernel.c ../../utils/src/utils.c -lcommons -pthread -lm

# CONFIGS PARA ENTREGA LABO

ebase:
	cd /home/utnso/Escritorio && \
	rm -rf kernel.config && \
	rm -rf procesador.config && \
	rm -rf memoria.config

	cp ../../configsEntrega/base/kernel.config /home/utnso/Escritorio/ && \
	cp ../../configsEntrega/base/procesador.config /home/utnso/Escritorio/ && \
	cp ../../configsEntrega/base/memoria.config /home/utnso/Escritorio/ && \
	cp ../../configsEntrega/base/terminal.config /home/utnso/Escritorio/

	cd /home/utnso/Escritorio && \
	nano kernel.config

eplani:
	cd /home/utnso/Escritorio && \
	rm -rf kernel.config && \
	rm -rf procesador.config && \
	rm -rf memoria.config

	cp ../../configsEntrega/planificacion/kernel.config /home/utnso/Escritorio/ && \
	cp ../../configsEntrega/planificacion/procesador.config /home/utnso/Escritorio/ && \
	cp ../../configsEntrega/planificacion/memoria.config /home/utnso/Escritorio/

	cd /home/utnso/Escritorio && \
	nano kernel.config

esuspe:
	cd /home/utnso/Escritorio && \
	rm -rf kernel.config && \
	rm -rf procesador.config && \
	rm -rf memoria.config

	cp ../../configsEntrega/suspension/kernel.config /home/utnso/Escritorio/ && \
	cp ../../configsEntrega/suspension/procesador.config /home/utnso/Escritorio/ && \
	cp ../../configsEntrega/suspension/memoria.config /home/utnso/Escritorio/

	cd /home/utnso/Escritorio && \
	nano kernel.config

eclock:
	cd /home/utnso/Escritorio && \
	rm -rf kernel.config && \
	rm -rf procesador.config && \
	rm -rf memoria.config

	cp ../../configsEntrega/clock/kernel.config /home/utnso/Escritorio/ && \
	cp ../../configsEntrega/clock/procesador.config /home/utnso/Escritorio/ && \
	cp ../../configsEntrega/clock/memoria.config /home/utnso/Escritorio/

	cd /home/utnso/Escritorio && \
	nano kernel.config

etlb:
	cd /home/utnso/Escritorio && \
	rm -rf kernel.config && \
	rm -rf procesador.config && \
	rm -rf memoria.config

	cp ../../configsEntrega/tlb/kernel.config /home/utnso/Escritorio/ && \
	cp ../../configsEntrega/tlb/procesador.config /home/utnso/Escritorio/ && \
	cp ../../configsEntrega/tlb/memoria.config /home/utnso/Escritorio/

	cd /home/utnso/Escritorio && \
	nano kernel.config

eintegral:
	cd /home/utnso/Escritorio && \
	rm -rf kernel.config && \
	rm -rf procesador.config && \
	rm -rf memoria.config

	cp ../../configsEntrega/integral/kernel.config /home/utnso/Escritorio/ && \
	cp ../../configsEntrega/integral/procesador.config /home/utnso/Escritorio/ && \
	cp ../../configsEntrega/integral/memoria.config /home/utnso/Escritorio/

	cd /home/utnso/Escritorio && \
	nano kernel.config