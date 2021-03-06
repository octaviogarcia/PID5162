/****************************************************************/
/****************************************************************/
/* 				SLAVE_COPY										*/
/* SLAVE_COPY algorithm routines for intra-nodes RDISKs			*/
/****************************************************************/
#define _MULTI_THREADED
#define _GNU_SOURCE     
#define  MOL_USERSPACE	1

// #define TASKDBG		1

#define TRUE 1
#define WAIT4BIND_MS 1000

#include "rdisk.h"
#include <sys/syscall.h>
#include "const.h"


/*for master-slave struct*/

struct msdevvec {			/* vector for minor devices */
  blksize_t st_trblksize; 	/* of stat */
  unsigned char *localbuff;	/* buffer to the device*/
  int buff_size;			/* buffer size for this device*/
  int img_p; 				/*file descriptor - disk image*/
};

typedef struct msdevvec msdevvec_t;


#define DEV_USR_FORMAT "st_trblksize=%d localbuff=%X buff_size=%d\n"
#define DEV_USR_FIELDS(p) p->st_trblksize, p->localbuff,p->buff_size

message sl_msg;
msdevvec_t ms_devvec[NR_DEVS];
off_t sl_position;
unsigned long sl_size, bk_count; /*image file size -  total block count*/	
unsigned long int nr_transfered, nr_matched; /*number transfer block*/
unsigned char*localbuff = NULL; //julio

/*===========================================================================*
 *				init_slavecopy				     
 * It connects REPLICATE thread to the SPREAD daemon and initilize several local 
 * and replicated  var2iables--> tiene sentido en slave?
 *===========================================================================*/
int init_slavecopy(void)
{
	int rcode;

	TASKDEBUG("Initializing SLAVE_COPY\n"); 
	return(OK);
}


/*===========================================================================*
 *				get_maxTsize					     *
 * get the maximun transfer block size
 *===========================================================================*/
unsigned int get_maxTsize(unsigned long D, unsigned int b){
// CALCULO DEL TAMAÑO DE BLOQUE MAXIMO DE TRANSFERENCIA
// Problema: El tamaño de bloque de transferencia debe ser multiplo del tamaño de bloque de dispositivo 
			// y debe ser multiplo de el tamaño de bloque de dispositivo.

// b: tamaño de bloque de dispositivo
// D: tamano dispositivo => N * b
// T: tamaño de bloque de transferencia => (K * b) y (mod(D/T) = 0)
			
//-------------------------------------------------------- 
// Obtiene el tamaño maximo del bloque de 
// transferencia que es multiplo de bloque de dispositivo
// y es MCD del tamaño de dispositivo.	
//--------------------------------------------------------	
	unsigned int k, i, T;		
	TASKDEBUG("get_maxTsize, size=%u - block=%u\n", D, b);
	if( b <= 0) ERROR_EXIT(EDVSINVAL); 	// tamaño de bloque no negativo o cero
	if( D <= 0) ERROR_EXIT(EDVSINVAL); 	// tamaño de dispositivo no negativo o cero
	if( D < b) ERROR_EXIT(EDVSINVAL); 	// tamaño de dispositivo mayor tamaño de bloque	
	if( (D % b) == 0){
		TASKDEBUG("D=%u múltiplo b=%u\n", D, b);
	} 
	else{
		ERROR_EXIT(EDVSINVAL);// aseguro que el tamaño de disco sea multiplo 
	}
	// del tamaño de bloque de dispositivo.
	for ( i = 0; i < sizeof(unsigned int); i++){
		k = (1 << i);
		T = b * k;
		TASKDEBUG("T=%u, b=%u, k=%d\n", T, b, k);
		if ( (T > MAXCOPYBUF) || (D%T != 0)) {
			i--;
			k = (1 << i);
			T = b * k;
			TASKDEBUG("T=%u, b=%u, k=%d\n", T, b, k);
			return(T);
		}
	}
	TASKDEBUG("T=%u, b=%u, k=%d\n", T, b, k);
	return(T);
} 

/*=============================================================================*
 *				sync_request
 *Master y Slave acuerdan hacer la sincronización, evalúan tamaño de los dispositivos
 *a sincronizarse, posición de inicio y buffer en origen-destino
 * *===========================================================================*/
void sync_request(int nr_dev, int stype){

	int rcode, ret;
	// unsigned *localbuff; //julio

	/*---------- Open image device ---------------*/
	ms_devvec[nr_dev].img_p = open(devvec[nr_dev].img_ptr, O_RDWR);
	TASKDEBUG("Open imagen FD=%d\n", ms_devvec[nr_dev].img_p);
				
	if(ms_devvec[nr_dev].img_p < 0) {
		TASKDEBUG("img_p=%d\n", ms_devvec[nr_dev].img_p);
		rcode = errno;
		TASKDEBUG("rcode=%d\n", rcode);
		exit(EXIT_FAILURE);
		}

	devvec[nr_dev].available = 0; /*lo dejo no disponible hasta que termine de actualizar*/
			
	/*----------------- message -------------------*/	
	sl_msg.m_type = DEV_TRANS;
	TASKDEBUG("m_type=%d\n", sl_msg.m_type);
		
	sl_msg.DEVICE = nr_dev; 
	TASKDEBUG("Device: m_m2_i1=%d\n", sl_msg.DEVICE);
		
	sl_msg.IO_ENDPT = sc_ep; /*proc_nr*/ //O PUEDE SER mc_ep
	TASKDEBUG("IO_ENDPT: m_m2_i2=%d\n", sl_msg.IO_ENDPT);
		
	sl_msg.POSITION = sl_position; /*desde donde inicio la copia* VER ESTO, podría ser nr_block=0; sino me quedo sin campos*/
	TASKDEBUG("POSITION - offset(position): m_m2_l1=%u\n", sl_msg.POSITION);

	sl_msg.m2_l2 = devvec[nr_dev].st_size; /*device size*/
	TASKDEBUG("sl_msg.m2_l2(size): m_m2_l2=%u\n", sl_msg.m2_l2);
				
				
	/*get max transfer block*/	
	TASKDEBUG("devvec[%d].st_size=%u, devvec[%d].st_blksize=%u\n", nr_dev, devvec[nr_dev].st_size, nr_dev, devvec[nr_dev].st_blksize);
	ms_devvec[nr_dev].st_trblksize = get_maxTsize(devvec[nr_dev].st_size, devvec[nr_dev].st_blksize);
	TASKDEBUG("ms_devvec[%d].st_trblksize=%d\n",nr_dev, ms_devvec[nr_dev].st_trblksize);
	sl_msg.COUNT = ms_devvec[nr_dev].st_trblksize; /*bytes transfer block size*/				
	r_trblksize = ms_devvec[nr_dev].st_trblksize; /*for replicate.c*/
	TASKDEBUG("r_trblksize: %u\n", r_trblksize);

	// if ( (stype == DEV_UFULL) || (stype == DEV_CMD5) || (stype == DEV_UMD5) ) {
		bk_count = devvec[nr_dev].st_size / ms_devvec[nr_dev].st_trblksize;
		TASKDEBUG("Block count: %d\n", bk_count);
		fprintf(stdout, "Block-count estimated to be transfered: %d  \n", bk_count);
		fflush(stdout);
	// }
				
/*---------------- Allocate memory for buffer  ---------------*/
	posix_memalign( (void **) &localbuff, getpagesize(), ms_devvec[nr_dev].st_trblksize );
	ms_devvec[nr_dev].localbuff = localbuff;			
	if ( ms_devvec[nr_dev].localbuff == NULL) {
		fprintf(stderr, "posix_memalign\n");
		fflush(stderr);
		exit(1);
		}
	
	sl_msg.ADDRESS = ms_devvec[nr_dev].localbuff; /*buffer of backup*/ 
	TASKDEBUG("ms_devvec[%d].localbuff=%p\n",nr_dev, ms_devvec[nr_dev].localbuff);

	TASKDEBUG("SENDREC msg DEV_TRANS: m_type=%d, DEVICE=%d, IO_ENDPT=%d, POSITION=%u, size(m2_l2) =%u, COUNT=%u, ADDRESS=%p\n",
				sl_msg.m_type,
				sl_msg.DEVICE,
				sl_msg.IO_ENDPT,
				sl_msg.POSITION,
				sl_msg.m2_l2,
				sl_msg.COUNT,
				sl_msg.ADDRESS);		
	ret = dvk_sendrec(RDISK_MASTER, (long) &sl_msg); /*para enviar y recibir de un cliente a un servidor*/		
	if( ret != 0 ){
		fprintf(stderr,"sendrec - ret=%d\n",ret);
		fflush(stderr);
		exit(ret);
	}

	if ( sl_msg.m_type != DEV_TRANSR ){
		TASKDEBUG("REPLAY ERROR=%d\n", sl_msg.m_type);
		exit(EXIT_FAILURE);
	}else{
		sl_size = sl_msg.m2_l2; /*size of primary */	
		sl_position = sl_msg.POSITION; /*desde donde se va a iniciar la copia del dispositivo, VER QUE VENGA EL NRO DE BLOQUE 0MARIE*/
		TASKDEBUG("sl_size=%u, sl_position=%u\n", sl_size, sl_position);
	}
	return(OK); 
}				

/*===========================================================================*
 *				sync_updates
 *slave actualiza su dispositivo según los datos del master hasta completar el
 *tamaño del mismo
 *===========================================================================*/
unsigned long sync_updates(int nr_dev, int stype){

int ret, size, i, cond;
unsigned long bytes, tbytes, nr_block, tblock, total;


tbytes = 0;
size = sl_size;
TASKDEBUG("size: %u, sl_size: %u\n", size, sl_size);					

i = 0;
nr_block = 0; /*block number*/ 
tblock = 0;
tbytes = 0,
total = 0;
cond = FALSE;
TASKDEBUG("cond:%d\n", cond);

if ( stype == DEV_UFULL ) {
	TASKDEBUG("Total nr_block: %u\n", nr_block);
}
				
do{
	sl_msg.m_type = stype; /*de las opciones de rdisk*/
	TASKDEBUG("m_type=%d\n", sl_msg.m_type);
				
	sl_msg.DEVICE = nr_dev; 
	TASKDEBUG("Device: m_m2_i1=%d\n", sl_msg.DEVICE);
	
	sl_msg.IO_ENDPT = sc_ep; /*proc_nr*/
	TASKDEBUG("IO_ENDPT: m_m2_i2=%d\n", sl_msg.IO_ENDPT);
					
	sl_msg.POSITION = sl_position; /*desde donde inicio la copia* VER ESTO, inicialmente=0*/
	TASKDEBUG("POSITION =%u\n", sl_msg.POSITION); 
	
	if ( stype == DEV_UFULL ) {
		s_nrblock = sl_position; /*nro de block slave */
		TASKDEBUG("POSITION - block_nr: =%u, s_nrblock=%d\n", sl_msg.POSITION, s_nrblock); /*envío el nro de bloque*/
		}
					
	TASKDEBUG("buff: %X\n", ms_devvec[nr_dev].localbuff);
						
	TASKDEBUG("SENDREC msg: m_type=%d, DEVICE=%d, IO_ENDPT=%d, BLOCK_NR/POSITION=%u, COUNT=%u, ADDRESS=%p\n",
				sl_msg.m_type,
				sl_msg.DEVICE,
				sl_msg.IO_ENDPT,
				sl_msg.POSITION,
				sl_msg.COUNT,
				sl_msg.ADDRESS);		

	ret = dvk_sendrec(RDISK_MASTER, (long) &sl_msg); /*para enviar y recibir de un cliente a un servidor*/
		
	if( ret != 0 )
		fprintf(stderr,"sendrec - ret=%d\n",ret);
		fflush(stderr);
	
	switch(sl_msg.m_type){
		case DEV_CFULLR: {			
			TASKDEBUG("Position %d (%d)\n", sl_msg.POSITION, sl_position);
			TASKDEBUG("ms_devvec[nr_dev].img_p=%d, ms_devvec[nr_dev].localbuff=%s, ms_devvec[nr_dev].st_trblksize=%u, sl_msg.POSITION=%u\n",
					ms_devvec[nr_dev].img_p, 
					ms_devvec[nr_dev].localbuff,
					ms_devvec[nr_dev].st_trblksize,
					sl_msg.POSITION);
									
			bytes = pwrite(ms_devvec[nr_dev].img_p, ms_devvec[nr_dev].localbuff, ms_devvec[nr_dev].st_trblksize, sl_msg.POSITION);
					
			TASKDEBUG("pwrite: %s\n", ms_devvec[nr_dev].localbuff);
			TASKDEBUG("bytes: %u\n", bytes);
								
			if(bytes < 0) ERROR_EXIT(errno);
		
			sl_position += bytes;
			tbytes += bytes;
			TASKDEBUG("position: %u, tbytes: %u, bytes: %u\n", sl_position, tbytes, bytes);
			size -= bytes;
			TASKDEBUG("size: %u, bytes: %u\n", size, bytes);				
			sumdevvec[nr_dev].nr_transfered++; /*number transfer block*/
			TASKDEBUG("nr_transfered: %u - dev: %d\n", sumdevvec[nr_dev].nr_transfered, nr_dev);				
			
			cond = ((size > 0) && (tbytes < sl_size))?FALSE:TRUE;
			TASKDEBUG("cond:%d\n", cond);
						
			break;
			}
		
		case DEV_UFULLR:{
			TASKDEBUG("Block %d (%d)\n", sl_msg.POSITION, sl_position);
			TASKDEBUG("ms_devvec[nr_dev].img_p=%d, ms_devvec[nr_dev].localbuff=%s, ms_devvec[nr_dev].st_trblksize=%u, (sl_msg.POSITION * ms_devvec[nr_dev].st_trblksize=%u\n",
						ms_devvec[nr_dev].img_p, 
						ms_devvec[nr_dev].localbuff,
						ms_devvec[nr_dev].st_trblksize,
						(sl_msg.POSITION * ms_devvec[nr_dev].st_trblksize));
							
													
			TASKDEBUG("lock slave to write position=%u,  (%u)\n", sl_msg.POSITION, sl_position);
			// pthread_mutex_lock(&write_mutex);
			MTX_LOCK(write_mutex);
							
			bytes = pwrite(ms_devvec[nr_dev].img_p, ms_devvec[nr_dev].localbuff, ms_devvec[nr_dev].st_trblksize, (sl_msg.POSITION * ms_devvec[nr_dev].st_trblksize));
					
			TASKDEBUG("pwrite: %s\n", ms_devvec[nr_dev].localbuff);
			TASKDEBUG("bytes: %u\n", bytes);
			
			// pthread_mutex_unlock(&write_mutex);			
			MTX_UNLOCK(write_mutex);
		
			if(bytes < 0) ERROR_EXIT(errno);
			
			tbytes += bytes;			
			sl_position++;
			tblock++;
			sumdevvec[nr_dev].nr_transfered = tblock;
			TASKDEBUG("nr_block: %d, tblock: %d, tbytes: %u\n", sl_position, tblock, tbytes);						
			
			cond = ((tblock < bk_count) && (( tblock * ms_devvec[nr_dev].st_trblksize ) < sl_size)) ?FALSE:TRUE;
			TASKDEBUG("total bytes:%u\n", ( tblock * ms_devvec[nr_dev].st_trblksize ));
			TASKDEBUG("cond:%d\n", cond);
			break;
			}
		
		default:{
			fprintf(stderr,"Bad transfer type: %d\n", sl_msg.m_type);
			fflush(stderr);
			return(0);
			break;
			} 
		}
		
	}while( cond != TRUE ); 

stype == DEV_CFULL?(total = tbytes):(total = tblock);	
TASKDEBUG("tbytes: %u\n", tbytes);					
TASKDEBUG("tblock: %u\n", tblock);					
TASKDEBUG("Total nr_transfered: %u - dev\n", sumdevvec[nr_dev].nr_transfered, nr_dev);				
return(total);
}
/*===========================================================================*
 *				sync_diffupdates
 *slave actualiza su dispositivo según los datos del master hasta completar el
 *tamaño del mismo
 *===========================================================================*/
unsigned long sync_diffupdates(int nr_dev, int stype){

int ret, size, i;
unsigned long bytes, tbytes, nr_block, tblock, total;

size = sl_size;
TASKDEBUG("size: %u, sl_size: %u\n", size, sl_size);					
TASKDEBUG("bk_count: %u\n", bk_count);					

i = 0;
nr_block = 0; /*block number*/ 
tblock = 0;
tbytes = 0,
total = 0;

//if ( stype == DEV_UMD5 ) {
	TASKDEBUG("Total nr_block: %u\n", nr_block);
//}
				
do{
	sl_msg.m_type = stype; /* por las opciones de rdisk*/
	TASKDEBUG("m_type=%d\n", sl_msg.m_type);
				
	// sl_msg.DEVICE = nr_dev; //este
	// TASKDEBUG("Device: m_m2_i1=%d\n", sl_msg.DEVICE);
	
	// sl_msg.IO_ENDPT = sc_ep; /*proc_nr*/
	// TASKDEBUG("IO_ENDPT: m_m2_i2=%d\n", sl_msg.IO_ENDPT);
					
	sl_msg.mB_nr = sl_position; /*desde donde inicio la copia* VER ESTO, inicialmente=0*/ //este
	TASKDEBUG("POSITION =%u\n", sl_msg.mB_nr); 
	
	if ( stype == DEV_UMD5 ) {
		s_nrblock = sl_position; /*nro de block slave */
		TASKDEBUG("POSITION - block_nr: =%u, s_nrblock=%d\n", sl_msg.mB_nr, s_nrblock); /*envío el nro de bloque*/
	}
					
	// TASKDEBUG("buff: %X\n", ms_devvec[nr_dev].localbuff);
	
	/*aplico MD5: leer el bloque, calcular el "sig" y enviar al master: posición, cantidad y sig*/
	
	TASKDEBUG("md5_compute: fd=%d, buffer=%X, bytes=%u, position=%u\n",
							ms_devvec[nr_dev].img_p,
							ms_devvec[nr_dev].localbuff,
							ms_devvec[nr_dev].st_trblksize,
							(ms_devvec[nr_dev].st_trblksize * sl_position));
							
	md5_compute(ms_devvec[nr_dev].img_p, ms_devvec[nr_dev].localbuff, ms_devvec[nr_dev].st_trblksize, (ms_devvec[nr_dev].st_trblksize * sl_position), sigs);
	
	TASKDEBUG("sigs = %s\n", sigs);
	memcpy(sl_msg.mB_md5, sigs, MD5_SIZE);
	TASKDEBUG("sl_msg.mB_md5: %s\n", sl_msg.mB_md5);
	
						
	TASKDEBUG("SENDREC msg: m_type=%d, BLOCK_NR/POSITION=%u, sig(m2_l2) =%s\n",
				sl_msg.m_type,
				sl_msg.mB_nr,
				sl_msg.mB_md5);		

	ret = dvk_sendrec(RDISK_MASTER, (long) &sl_msg); /*para enviar y recibir de un cliente a un servidor*/
		
	if( ret != 0 )
		fprintf(stderr,"sendrec - ret=%d\n",ret);
		fflush(stderr);
	
	switch(sl_msg.m_type){
		case DEV_CMD5R: {
			
			TASKDEBUG("sl_msg.mB_md5 %s\n", sl_msg.mB_md5);	
			TASKDEBUG("sigs %s\n", sigs);	
						
			if ( memcmp(sl_msg.mB_md5, sigs, MD5_SIZE) == 0 ) {
							
				TASKDEBUG("Block %d (%d) matches\n", sl_msg.mB_nr, sl_position);	
				sumdevvec[nr_dev].nr_matched++;					
				TASKDEBUG("sumdevvec[nr_dev].nr_matched= %d\n", sumdevvec[nr_dev].nr_matched);	
					
				}
			else{
				
				TASKDEBUG("Block %d (%d) NOT matches\n", sl_msg.mB_nr, sl_position);
				TASKDEBUG("ms_devvec[nr_dev].img_p=%d, ms_devvec[nr_dev].localbuff=%s, ms_devvec[nr_dev].st_trblksize=%u, (m.mB_nr * ms_devvec[nr_dev].st_trblksize=%u\n",
							ms_devvec[nr_dev].img_p, 
							ms_devvec[nr_dev].localbuff,
							ms_devvec[nr_dev].st_trblksize,
							(sl_msg.mB_nr * ms_devvec[nr_dev].st_trblksize));
				
				bytes = pwrite(ms_devvec[nr_dev].img_p, ms_devvec[nr_dev].localbuff, ms_devvec[nr_dev].st_trblksize, (sl_msg.mB_nr * ms_devvec[nr_dev].st_trblksize));
				
				TASKDEBUG("pwrite: %s\n", ms_devvec[nr_dev].localbuff);
				TASKDEBUG("bytes: %u\n", bytes);
			
				sumdevvec[nr_dev].nr_transfered++;
				TASKDEBUG("sumdevvec[nr_dev].nr_transfered= %d\n", sumdevvec[nr_dev].nr_transfered);	
				tbytes += bytes;
				
				if(bytes < 0) ERROR_EXIT(errno);
				}
			
						
			sl_position ++;
			tblock++;
			TASKDEBUG("position: %u, tblock: %u, bytes: %u\n", sl_position, tblock, bytes);
			//size -= bytes;
			//TASKDEBUG("size: %u, bytes: %u\n", size, bytes);				
			TASKDEBUG("dev: %d, nr_transfered: %u\n", nr_dev, sumdevvec[nr_dev].nr_transfered);				
			break;
			
			}
		
		case DEV_UMD5R:{
			
			TASKDEBUG("sl_msg.mB_md5 %s\n", sl_msg.mB_md5);	
			TASKDEBUG("sigs %s\n", sigs);	
						
			if ( memcmp(sl_msg.mB_md5, sigs, MD5_SIZE) == 0 ) {
										
				TASKDEBUG("Block %d (%d) matches\n", sl_msg.mB_nr, sl_position);	
				sumdevvec[nr_dev].nr_matched++;					
				TASKDEBUG("sumdevvec[nr_dev].nr_matched=%d\n", sumdevvec[nr_dev].nr_matched);
						
				}
			else{
				
				TASKDEBUG("Block %d (%d) NOT matches\n", sl_msg.mB_nr, sl_position);
				TASKDEBUG("ms_devvec[nr_dev].img_p=%d, ms_devvec[nr_dev].localbuff=%s, ms_devvec[nr_dev].st_trblksize=%u, (sl_msg.mB_nr * ms_devvec[nr_dev].st_trblksize=%u\n",
							ms_devvec[nr_dev].img_p, 
							ms_devvec[nr_dev].localbuff,
							ms_devvec[nr_dev].st_trblksize,
							(sl_msg.mB_nr * ms_devvec[nr_dev].st_trblksize));
				
				TASKDEBUG("lock slave to write position=%d\n", sl_msg.mB_nr, sl_position);
				// pthread_mutex_lock(&write_mutex);
				MTX_LOCK(write_mutex);
			
				bytes = pwrite(ms_devvec[nr_dev].img_p, ms_devvec[nr_dev].localbuff, ms_devvec[nr_dev].st_trblksize, (sl_msg.mB_nr * ms_devvec[nr_dev].st_trblksize));
				
				TASKDEBUG("pwrite: %s\n", ms_devvec[nr_dev].localbuff);
				TASKDEBUG("bytes: %u\n", bytes);
			
				TASKDEBUG("unlock slave to write=%d\n", sl_msg.mB_nr);
				// pthread_mutex_unlock(&write_mutex);
				MTX_UNLOCK(write_mutex);
				
				sumdevvec[nr_dev].nr_transfered++;
				TASKDEBUG("sumdevvec[nr_dev].nr_transfered=%d\n", sumdevvec[nr_dev].nr_transfered);
				tbytes += bytes;
				
				if(bytes < 0) ERROR_EXIT(errno);
				}
							
			sl_position++;
			tblock++;
			TASKDEBUG("nr_block: %d, tblock: %d, tbytes: %u\n", sl_position, tblock, tbytes);						
			break;
			}
		
		default:{
			fprintf(stderr,"Bad transfer type: %d\n", sl_msg.m_type);
			fflush(stderr);
			return(0);
			break;
		} 
	}
	
	TASKDEBUG("tblock: %u, bk_count: %u\n", tblock, bk_count);						
	} while ( tblock < bk_count);

total = tblock;	
TASKDEBUG("tbytes: %u\n", tbytes);					
TASKDEBUG("tblock: %u\n", tblock);					
TASKDEBUG("Total dev: %d - nr_transfered: %u\n", nr_dev, sumdevvec[nr_dev].nr_transfered);				
TASKDEBUG("Total nr_matched: %u - Dev: %d\n", sumdevvec[nr_dev].nr_matched);				
return(total);
}

/*===========================================================================*
 *				sync_devready
 *fin de la sincronización, dispositivo listo
 *===========================================================================*/
int sync_devready(int nr_dev, unsigned long t, int stype)
{
	int ret, rcode;
	unsigned long tbytes;
	// unsigned *buff_devready;

	TASKDEBUG("DEV_READY..\n");					
	tbytes = (( stype == DEV_UFULL )||( stype == DEV_CMD5 )||( stype == DEV_UMD5 ))?(t * ms_devvec[nr_dev].st_trblksize):t; 
	TASKDEBUG("t: %u\n", t);					

	TASKDEBUG("free ms_devvec[%d].localbuff=%p\n", nr_dev, ms_devvec[nr_dev].localbuff);					
	free(ms_devvec[nr_dev].localbuff); 

	TASKDEBUG("tbytes: %u, sl_size: %u\n", tbytes, sl_size);					
	if ( tbytes  == sl_size ){ /*tbytes = size of device*/
		TASKDEBUG("Dev[%d] - Backup ready: %u bytes\n", nr_dev, tbytes);	
		devvec[nr_dev].available = 1; /*dispositivo disponible*/
					
		/*---------- Close image device ---------------*/
		TASKDEBUG("Close imagen FD=%d\n", ms_devvec[nr_dev].img_p);
		
		if ( rcode=close(ms_devvec[nr_dev].img_p) < 0) {
				TASKDEBUG("Error close=%d\n", rcode);
				exit(EXIT_FAILURE);
				}
				
		/*---------- To MASTER - dev ready ---------------*/				
		sl_msg.m_type = DEV_EOF; 
		TASKDEBUG("DEV_EOF: %d\n", sl_msg.m_type);
					
		sl_msg.DEVICE = nr_dev; 
		TASKDEBUG("Device: m_m2_i1=%d\n", sl_msg.DEVICE);
		
		sl_msg.IO_ENDPT = sc_ep; /*proc_nr*/ 
		TASKDEBUG("IO_ENDPT: m_m2_i2=%d\n", sl_msg.IO_ENDPT);
					
		TASKDEBUG("SENDREC msg DEV_EOF: m_type=%d, DEVICE=%d, IO_ENDPT=%d\n",
							sl_msg.m_type,
							sl_msg.DEVICE,
							sl_msg.IO_ENDPT
							);

		ret = dvk_sendrec(RDISK_MASTER, (long) &sl_msg); /*para enviar y recibir de un cliente a un servidor*/
				
		if( ret != 0 )
			fprintf(stderr,"sendrec - ret=%d\n",ret);
			fflush(stderr);	
		
		if ( sl_msg.m_type == DEV_EOFR ){
			
			devvec[nr_dev].active=sl_msg.m2_l2;
			TASKDEBUG("sl_msg.m2_l2=%d, devvec[%d].active=%d\n", sl_msg.m2_l2, nr_dev, devvec[nr_dev].active);
			nr_dev++;
			TASKDEBUG("nr_dev=%d\n", nr_dev); 
			}
		else{
			TASKDEBUG("Dev[%d] - Backup ERROR: %u bytes\n", nr_dev, tbytes);	
			sl_msg.m_type = RD_DISK_ERR;
			TASKDEBUG("m_type=%d\n", sl_msg.m_type);
			nr_dev = NR_DEVS; /*corto el ciclo, hay al menos un dispositivo que no pudo sincronizarse*/
			/*MARIE_VER:ver por acá no debería tmb hacer el unbind y finalizar el thread???*/
			}	
					
		if ( nr_dev == (NR_DEVS - 1) ){	
			sl_msg.m_type = RD_DISK_EOF; /*si queda disponible reasigno el disk_eof*/
			TASKDEBUG("m_type=%d, nr_dev=%d\n", sl_msg.m_type, nr_dev); 
			TASKDEBUG("FSM_state=%d\n", FSM_state);
			FSM_state == STS_WAIT4PRIMARY;
			}
		}
	else{
		TASKDEBUG("Dev[%d] - Backup ERROR: %u bytes\n", nr_dev, tbytes);	
		sl_msg.m_type = RD_DISK_ERR;
		TASKDEBUG("m_type=%d\n", sl_msg.m_type);
		nr_dev = NR_DEVS; /*corto el ciclo, hay al menos un dispositivo que no pudo sincronizarse*/
		/*ver por acá no debería tmb hacer el unbind y finalizar el thread???*/
		FSM_state == STS_LEAVE;
		TASKDEBUG("FSM_state=%d\n", FSM_state);
		}	
	TASKDEBUG("nr_dev=%d\n", nr_dev);
	return(nr_dev);
}	

/*===========================================================================*
 *				sync_summary
 *estadísticas de la actualización
 *===========================================================================*/
void sync_summary(struct tm *ptr_tm, int stype){


int i = 0;
unsigned long total;
total = 0;

switch(stype) {
	
		case DEV_CFULL:		
					if ( dynup_flag != DO_DYNUPDATES ) {
						fprintf(stdout, "Transfer method: FULL UPDATE\n");	
						fflush(stdout);
						break;
					}else{
						fprintf(stdout,"Transfer method: FULL UPDATE whit Dynamic updates\n");	
						fflush(stdout);
						break;							
					}
		case DEV_CMD5:	
					if ( dynup_flag != DO_DYNUPDATES ) {
						fprintf(stdout,"Transfer method: DIFF UPDATE\n");	
						fflush(stdout);
						break;
					}else{
						fprintf(stdout,"Transfer method: DIFF UPDATE whit Dynamic updates\n");	
						fflush(stdout);
						break;							
					}
		default:		
			fprintf(stderr,"Invalid Transfer method\n");
			fflush(stderr);
			return;
			break;
		}	 

fprintf(stdout,"Started: %02d/%02d/%02d - %02d:%02d:%02d\n", ptr_tm->tm_year, ptr_tm->tm_mon, ptr_tm->tm_mday, ptr_tm->tm_hour, ptr_tm->tm_min, ptr_tm->tm_sec);  
fflush(stdout);

for ( i = 0; i < NR_DEVS; i++){ 
	if ( devvec[i].available == 1 ){ /*device open in primary*/ 
		fprintf(stdout,"Total blocks transfered: %u - Dev: %d\n", sumdevvec[i].nr_transfered, i);		
		fflush(stdout);
		fprintf(stdout,"Blocks matched: %u - Dev: %d\n", sumdevvec[i].nr_matched, i);						
		fflush(stdout);
		fprintf(stdout,"Blocks dynamic updates: %u - Dev: %d\n", sumdevvec[i].nr_updated, i);				
		fflush(stdout);
		fprintf(stdout,"Total bytes Dev=%i : %u\n", i, sumdevvec[i].tbytes);
		fflush(stdout);
		total = total + sumdevvec[i].tbytes;	
	}
}
fprintf(stdout,"Size of transfer: %u\n", total); 
fflush(stdout);
return(OK);	 
}
 
 
/*===========================================================================*
 *				init_m3ipc					     *
 *===========================================================================*/
void init_m3ipc(void)
{
	int rcode;

	TASKDEBUG("Binding RDISK SLAVE=%d\n", RDISK_SLAVE);
	sc_ep = dvk_tbind(dcid, RDISK_SLAVE);
	if(sc_ep < 0) ERROR_EXIT(sc_ep);
	TASKDEBUG("sc_ep=%d\n", sc_ep);

	sc_lpid = (pid_t) syscall (SYS_gettid);
	TASKDEBUG("sc_ep=%d sc_lpid=%d\n", mc_ep, mc_lpid);	
	
	if( endp_flag == 0) { // RDISK runs under MOL 
		TASKDEBUG("Bind SLAVE to SYSTASK\n");
		rcode = sys_bindproc(sc_ep, sc_lpid, REPLICA_BIND);
		if(rcode < 0) ERROR_EXIT(rcode);				
		
		do {
			rcode = dvk_wait4bind_T(WAIT4BIND_MS);
			TASKDEBUG("SLAVE dvk_wait4bind_T  rcode=%d\n", rcode);
			if (rcode == EDVSTIMEDOUT) {
				TASKDEBUG("CLIENT dvk_wait4bind_T TIMEOUT\n");
				continue ;
			} else if ( rcode < 0)
				ERROR_EXIT(EXIT_FAILURE);
		} while	(rcode < OK);
	} else { // RDISK runs autonomous 
		TASKDEBUG("Binding Remote RDISK MASTER=%d on node=%d\n", RDISK_MASTER, primary_mbr);
		mc_ep = dvk_rmtbind(dcid,"master",RDISK_MASTER, primary_mbr);
		if(mc_ep < 0) ERROR_EXIT(mc_ep);
		TASKDEBUG("mc_ep=%d\n", mc_ep);
	}
   return(OK);
}


/*===========================================================================*
 *				slavecopy_main											     *
 *===========================================================================*/
void *slavecopy_main(void *arg)
{
	static 	char source[MAX_GROUP_NAME];
	int rcode, mtype, ret, i, nr_dev, r;
	unsigned bytes, tblock;
	time_t the_time;
	char buffTime[20]; 
	struct tm *infotime_init, *infotime_end;


	TASKDEBUG("replicate_main dcid=%d local_nodeid=%d\n"
		,dcid, local_nodeid);
	
    init_m3ipc();	
	
	TASKDEBUG("r_type: %d\n", r_type);
	
	time(&the_time);
	infotime_init = gmtime(&the_time);
	
	switch(r_type) {
		case DEV_CFULL:		
			if ( dynup_flag != DO_DYNUPDATES ) {
				TASKDEBUG("r_type: %d - DEV_CFULL\n", r_type);	
				fprintf(stdout,"Transfer Method: COPY FULL\n");
				fflush(stdout);
				r = (scopy_full)(DEV_CFULL);
				TASKDEBUG("r(scopy_full): %d\n", r);
				break;
			}else{
				TASKDEBUG("r_type: %d - DEV_UFULL\n", r_type);	
				fprintf(stdout,"Transfer Method: COPY FULL (+DYN_UPDATES)\n");
				fflush(stdout);
				r = (sufull_copy) (DEV_UFULL); 
				TASKDEBUG("r(sufull_copy): %d\n", r);
				break;							
			}
		case DEV_CMD5:	
			if ( dynup_flag != DO_DYNUPDATES ) {
				TASKDEBUG("r_type: %d - DEV_CMD5\n", r_type);	
				fprintf(stdout,"Transfer Method: DIFF UPDATE\n");
				fflush(stdout);
				r = (sdiff_copy)(DEV_CMD5);
				TASKDEBUG("r(sdiff_copy): %d\n", r);
			}else{
				TASKDEBUG("r_type: %d - DEV_UMD5\n", r_type);	
				fprintf(stdout,"Transfer Method: DIFF UPDATE (+DYN_UPDATES)\n");
				fflush(stdout);
				r = (sdiff_copy) (DEV_UMD5); 
				TASKDEBUG("r(sdiff_copy): %d\n", r);
			}
			break;							
		default:		
			TASKDEBUG("Invalid Transfer Method: %d\n", r_type);
			if(rcode < 0) ERROR_EXIT(rcode);
			break;
	}

	/*esto sería fuera del case y terminaría la sincronizacióN*/
	/*NODE ready backup*/
	sl_msg.m2_l2 = local_nodeid; 
	TASKDEBUG("local_nodeid: sl_msg.m2_l2=%d (ready backup)\n", sl_msg.m2_l2);

	/*check open device*/
	TASKDEBUG("chek open device\n");
	for( i = 0; i < NR_DEVS; i++){
		TASKDEBUG("devvec[%d].available=%d\n", i, devvec[i].available);
		if ( devvec[i].available == 1 ){ /*device open in primary*/ 
			TASKDEBUG("devvec[i].available=%d\n", devvec[i].available);
		}
	}
	
	TASKDEBUG("SENDREC msg RD_DISK_EOF: m_type=%d, nodeid(m2_l2) =%u\n", sl_msg.m_type, sl_msg.m2_l2);

	ret = dvk_send(RDISK_MASTER, &sl_msg); /*envío respuesta al master diciendo que el bk está ok*/
	if( ret != 0 ) {
		fprintf( stderr,"SEND ret=%d\n",ret);
		fflush(stderr);
		// pthread_mutex_unlock(&ms_mutex);	
		exit(1);
	}

	FSM_state  = STS_WAIT4PRIMARY; // PAP	

	TASKDEBUG("unbinding RDISK MASTER=%d\n", RDISK_MASTER);
	TASKDEBUG("dcid=%d, mc_ep=%d\n", dcid, mc_ep);
	rcode = dvk_unbind(dcid,mc_ep);
	TASKDEBUG("rcode unbind=%d\n", rcode);
	if(rcode < 0) ERROR_EXIT(rcode);

	TASKDEBUG("unbinding RDISK SLAVE=%d\n", RDISK_SLAVE);
	TASKDEBUG("dcid=%d, sc_ep=%d\n", dcid, sc_ep);
	rcode = dvk_unbind(dcid,sc_ep);
	TASKDEBUG("rcode unbind=%d\n", rcode);
	if(rcode < 0) ERROR_EXIT(rcode);

	sync_summary(infotime_init, r_type); 

	if ( dynup_flag == DO_DYNUPDATES ){ 
		COND_SIGNAL(bk_barrier); //MARIE
	}

	TASKDEBUG("Exit slavecopy\n");
	pthread_exit(NULL);
}

/*===========================================================================*
 *				scopy_full									     *
 *===========================================================================*/		
/*slave: copy full - not dynamic updates*/
int scopy_full( int stype)
{
	int i, nr_dev, rcode, ret;
	unsigned sl_tbytes;
	 
	i = 0;
	nr_dev = 0; /*count devices*/
	sumdevvec[nr_dev].nr_transfered = 0;
	sumdevvec[nr_dev].nr_matched = 0;
	sumdevvec[nr_dev].tbytes = 0;
		
	while (nr_dev < NR_DEVS) { /*menor q cantidad de dispositivos*/
			
		if ( devvec[nr_dev].available == 1 ){ /*esté disponible en el archivo de configuración, sería cómo el flag de READ_ONLY??*/
			
			sl_position = 0; /*inicialmente del dispositivo 0*/
			TASKDEBUG("(Init) sl_position: %u\n", sl_position); 
			
			sync_request(nr_dev, stype);
			
			TASKDEBUG("nr_dev=%d, sl_size=%u, sl_position=%X\n", nr_dev, sl_size, sl_position);
		
			sl_tbytes = sync_updates(nr_dev, stype);	
		
			TASKDEBUG("sl_tbytes=%u\n", sl_tbytes);	
			
			sumdevvec[nr_dev].tbytes = sl_tbytes;
			
			nr_dev = sync_devready(nr_dev, sl_tbytes, stype);
			
			TASKDEBUG("nr_dev=%d\n", nr_dev);	
			}
		else{
			TASKDEBUG("Minor device %d is not available\n", nr_dev);	
			nr_dev++;
			sl_position = 0 ;
		}
	}
	return(OK);
}
 
/*===========================================================================*
 *				sufull_copy									     *
 *===========================================================================*/		
 /*slave: updates + copy*/
int sufull_copy( stype )
int stype;
{
/*copy + updates*/
int i, nr_dev, nr_block, rcode, ret;
unsigned long bytes, sl_tblock;
	
nr_dev = 0; /*count devices*/
nr_block = 0;
sumdevvec[nr_dev].nr_transfered = 0;
sumdevvec[nr_dev].nr_matched = 0;
sumdevvec[nr_dev].tbytes = 0;


	
while (nr_dev < NR_DEVS) { /*menor q cantidad de dispositivos*/
		
	if ( devvec[nr_dev].available == 1 ){ /*esté disponible en el archivo de configuración, sería cómo el flag de READ_ONLY??*/
	
		sl_position = nr_block; /*inicialmente del dispositivo 0*/
		TASKDEBUG("(Init) sl_position: %u\n", sl_position); 
		
		sync_request(nr_dev, stype);
			
		sl_tblock = sync_updates(nr_dev, stype);	
	
		TASKDEBUG("sl_tblock=%u\n", sl_tblock);		
		
		sumdevvec[nr_dev].tbytes = sl_tblock * ms_devvec[nr_dev].st_trblksize;

		nr_dev = sync_devready(nr_dev, sl_tblock, stype);
		
		TASKDEBUG("nr_dev=%d\n", nr_dev);	
		}
	else{
		TASKDEBUG("Minor device %d is not available\n", nr_dev);	
		nr_dev++;
		sl_position = 0 ;
	}
}
return(OK);
}

/*===========================================================================*
 *				sdiff_copy									     *
 *===========================================================================*/		
 /*slave: diff copy - dynamic/not dynamic updates*/
int sdiff_copy( int stype)
{
	int i, nr_dev, nr_block, rcode, ret;
	unsigned long bytes, sl_tblock;
		
	nr_dev = 0; /*count devices*/
	nr_block = 0;
	sumdevvec[nr_dev].nr_transfered = 0;
	sumdevvec[nr_dev].nr_matched = 0;
	sumdevvec[nr_dev].tbytes = 0;
	
	while (nr_dev < NR_DEVS) { /*menor q cantidad de dispositivos*/
			
		if ( devvec[nr_dev].available == 1 ){ /*esté disponible en el archivo de configuración, sería cómo el flag de READ_ONLY??*/
		
			sl_position = nr_block; /*inicialmente del dispositivo 0*/
			TASKDEBUG("(Init) sl_position: %u\n", sl_position); 
			
			sync_request(nr_dev, stype);
				
			sl_tblock = sync_diffupdates(nr_dev, stype);	
			TASKDEBUG("sl_tblock=%u\n", sl_tblock);		
			
			sumdevvec[nr_dev].tbytes = sl_tblock * ms_devvec[nr_dev].st_trblksize;

			nr_dev = sync_devready(nr_dev, sl_tblock, stype);
			
			TASKDEBUG("nr_dev=%d\n", nr_dev);	
			}
		else{
			TASKDEBUG("Minor device %d is not available\n", nr_dev);	
			nr_dev++;
			sl_position = 0 ;
		}
	}
	return(OK);
}
