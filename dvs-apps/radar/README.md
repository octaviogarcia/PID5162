## Topología de Red
La virtualización se realizó con vmware, la configuración de este se presenta a continuación:

	RED 192.168.1.0/24
	PC(NAT) 192.168.1.1
		|_node0 192.168.1.100
		|_node1 192.168.1.101
		|_node2 192.168.1.102
		|_ . . .
		|_nodeXX 192.168.1.1XX


## Poniéndolo en vmware

	sudo vmware-netcfg

En el NAT completar
subnet IP = 192.168.1.0
subnet mask  = 255.255.255.0

## Poniendolo en cada nodo

En cada nodo el archivo /etc/network/interfaces

	source /etc/network/interfaces.d/*
	auto lo
	iface lo inet loopback
		address 192.168.1.1XX
		netmask 255.255.255.0
		network 192.168.1.0
		gateway 192.168.1.2
		broadcast 192.168.1.255

Y en el archivo /etc/hostname

	nodeXX

##Formato archivo configuración radar
En nuestro caso el servicio de rdisk arranca con endpoint 3, y usamos
el DC 0

	service RDISK0 {
		replica		RPB
		dcid  		0;
		endpoint 	3;
		group		"RDISK";
	};


## Inicialización del DVS para la prueba del proyecto RADAR

En la carpeta scripts del proyecto hay dos archivos .sh que realizan el proceso de inicialización por defecto del DVS, él inicio del contenedor distribuido y el proceso de rdisk con el que se probó la aplicación. Los archivos se llaman rdisk_radar.sh y rdisk.sh. Más adelante se explicará en detalle que hace cada uno.

En el servidor primario (que en este ejemplo corre en el nodo0), corriendo rdisk_radar.sh se realiza el inicio y configuracion del DVS para luego levantar el servicio de rdisk.

	cd /usr/src/dvs/scripts
	./rdisk_radar.sh 0

El argumento pasado al script representa el nodo donde este se esta ejecutando. 
En el nodo 1 repetimos la operación para así levantar el servicio de rdisk nuevamente, pero esta vez, el nodo1 representa el servidor de backup.

En el nodo donde corre el radar (en este caso nodo2), se inicia también el DVS y se une al contenedor distribuido DC0 (por defecto). Todo esto con el script rdisk.sh con argumentos 2 y 0 respectivamente.

	cd /usr/src/dvs/scripts
	./rdisk.sh 2 0

Luego, en el nodo2 se inicia el servicio de radar con su archivo compilado:

	cd /usr/src/dvs/dvs-apps/radar
	./radar radar.cfg
	
Desde el nodo2 (donde corre radar) se puede ver el estado del proceso remoto utilizado en este ejemplo (rdisk).

	cat /proc/dvs/DC0/procs 

La salida tiene que ser por el estilo:

	DC pnr -endp -lpid/vpid- nd flag misc -getf -sndt -wmig -prxy name
	0   3     3    -1/-1     0 1000    0 27342 27342 27342 27342 RDISK 
	
El proceso remoto fue identificado con el número 3 y podemos ver que se asignó un valor de flag 1000 que representa que dicho proceso está activo. En la columna “nd” se nos informa en qué nodo se encuentra corriendo el servicio primario.
En caso de una desconexión eventual de la placa de red del nodo0, el mismo comando debería cambiar su salida, mostrando que ahora el primario se ejecuta sobre el nodo1.

## Prueba Cliente de RDISK en NODE2

Topologia

	NODE0: RDISK PRIMARIO
	NODE1: RDISK BACKUP
	NODE2: CLIENT (RADAR)
 
 En NODE2
 
 	root@node2:/usr/src/dvs/dvs-apps/radar# ./test_radar 0 3 70
 	
 EN NODE0 SE VE EL ENDPOINT DEL CLIENTE BINDEADO AUTOMATICAMENTE
 
	 root@node0:/usr/src/dvs/vos/mol/drivers/rdisk# cat /proc/dvs/DC0/procs 
	DC pnr -endp -lpid/vpid- nd flag misc -getf -sndt -wmig -prxy name
	0   3     3   641/4      0    8   20 31438 27342 27342 27342 rdisk          
	0  70    70    -1/-1     2 1000    0 27342 27342 27342 27342 rclient 
	
NODE0 MUERE => NODE1 PRIMARIO

	root@node1:/usr/src/dvs/vos/mol/drivers/rdisk# cat /proc/dvs/DC0/procs 
	DC pnr -endp -lpid/vpid- nd flag misc -getf -sndt -wmig -prxy name
	0   3     3   633/4      1    8   20 31438 27342 27342 27342 rdisk 

EN NODE2

	root@node2:/usr/src/dvs/dvs-apps/radar# cat /proc/dvs/DC0/procs
	DC pnr -endp -lpid/vpid- nd flag misc -getf -sndt -wmig -prxy name
	0   3     3    -1/-1     1 1000    0 27342 27342 27342 27342 RDISK0
	
PRUEBO CLIENTE DE RDISK 

	root@node2:/usr/src/dvs/dvs-apps/radar# ./test_radar 0 3 70
	
EN NODE1 SE VE EL ENDPOINT DEL CLIENTE BINDEADO AUTOMATICAMENTE

	root@node1:/usr/src/dvs/vos/mol/drivers/rdisk# cat /proc/dvs/DC0/procs 
	DC pnr -endp -lpid/vpid- nd flag misc -getf -sndt -wmig -prxy name
	0   3     3   633/4      1    8   20 31438 27342 27342 27342 rdisk          
	0  70    70    -1/-1     2 1000    0 27342 27342 27342 27342 rclient 
	
## PRUEBA DE TRANSPARENCIA 

	NODE0  PRIMARIO
	NODE1  BACKUP 
	NODE2  CLIENT
 
	root@node2:/usr/src/dvs/dvs-apps/radar# cat /proc/dvs/DC0/procs 
	DC pnr -endp -lpid/vpid- nd flag misc -getf -sndt -wmig -prxy name
	0   3     3    -1/-1     0 1000    0 27342 27342 27342 27342 RDISK0  <<<< NODE0 PRIMARIO


SE HACEN 30 LOOPS DE SENDREC EN 1 SEGUNDO CADA UNO DONDE m1i1 CONTIENE EL NUMERO DE ITERACION  

	root@node2:/usr/src/dvs/dvs-apps/radar# ./test_radar 0 3 71
	
EN EL MEDIO NOD0 MUERE, NODE1 PASA A SER EL PRIMARIO Y ASI TERMINA EN NODE2

	root@node2:/usr/src/dvs/dvs-apps/radar# cat /proc/dvs/DC0/procs
	DC pnr -endp -lpid/vpid- nd flag misc -getf -sndt -wmig -prxy name
	0   3     3    -1/-1     1 1000    0 27342 27342 27342 27342 RDISK0   
