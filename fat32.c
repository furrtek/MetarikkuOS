#include <avr/io.h>
#include <stdio.h>
#include <util/delay.h>
#include <string.h>
#include "fat32.h"
#include "sd.h"
#include "lcd.h"
#include "io.h"
#include "ym2612.h"

unsigned char getBootSectorData (void) {
	struct BS_Structure *bpb;
	struct MBRinfo_Structure *mbr;
	struct partitionInfo_Structure *partition;
	unsigned long dataSectors;

	SD_readSingleBlock(0);

	bpb = (struct BS_Structure *)buffer;

	if(bpb->jumpBoot[0]!=0xE9 && bpb->jumpBoot[0]!=0xEB) {	//check if it is boot sector
		mbr = (struct MBRinfo_Structure *) buffer;         	//if it is not boot sector, it must be MBR
		if(mbr->signature != 0xAA55) return 1;            	//if it is not even MBR then it's not FAT32  

		partition = (struct partitionInfo_Structure *)(mbr->partitionData);//first partition
		unusedSectors = partition->firstSector;	 	//the unused sectors, hidden to the FAT
 
		SD_readSingleBlock(partition->firstSector);	//read the bpb sector
		bpb = (struct BS_Structure *)buffer;
		if(bpb->jumpBoot[0]!=0xE9 && bpb->jumpBoot[0]!=0xEB)  return 1; 
	}

	bytesPerSector = bpb->bytesPerSector;
	sectorPerCluster = bpb->sectorPerCluster;
	reservedSectorCount = bpb->reservedSectorCount;
	rootCluster = bpb->rootCluster;
	firstDataSector = bpb->hiddenSectors + reservedSectorCount + (bpb->numberofFATs * bpb->FATsize_F32);
	dataSectors = bpb->totalSectors_F32
	              - bpb->reservedSectorCount
	              - ( bpb->numberofFATs * bpb->FATsize_F32);
	totalClusters = dataSectors / sectorPerCluster;

	return 0;
}

unsigned long getFirstSector(unsigned long clusterNumber) {
	return (((clusterNumber - 2) * sectorPerCluster) + firstDataSector);
}

//********************************************************************************************
//Function: to get or set next free cluster or total free clusters in FSinfo sector of SD card
//Arguments: 1.flag:TOTAL_FREE or NEXT_FREE, 
//			 2.flag: GET or SET 
//			 3.new FS entry, when argument2 is SET; or 0, when argument2 is GET
//return: next free cluster, if arg1 is NEXT_FREE & arg2 is GET
//        total number of free clusters, if arg1 is TOTAL_FREE & arg2 is GET
//		  0xffffffff, if any error or if arg2 is SET
//********************************************************************************************
unsigned long getSetFreeCluster(unsigned char totOrNext, unsigned char get_set, unsigned long FSEntry)
{
struct FSInfo_Structure *FS = (struct FSInfo_Structure *) &buffer;
unsigned char error;

SD_readSingleBlock(unusedSectors + 1);

if((FS->leadSignature != 0x41615252) || (FS->structureSignature != 0x61417272) || (FS->trailSignature !=0xaa550000))
  return 0xffffffff;

 if(get_set == GET)
 {
   if(totOrNext == TOTAL_FREE)
      	return(FS->freeClusterCount);
   else // when totOrNext = NEXT_FREE
		return(FS->nextFreeCluster);
 }
 else
 {
   if(totOrNext == TOTAL_FREE)
      	FS->freeClusterCount = FSEntry;
   else // when totOrNext = NEXT_FREE
	  	FS->nextFreeCluster = FSEntry;
 
   error = SD_writeSingleBlock(unusedSectors + 1);	//update FSinfo
 }
 return 0xffffffff;
}

struct dir_Structure* findFiles (unsigned char flag, unsigned char *fileName)
{
unsigned long cluster, sector, firstSector, firstCluster, nextCluster;
struct dir_Structure *dir;
unsigned int i;
unsigned char j;

cluster = rootCluster; //root cluster

while(1)
{
   firstSector = getFirstSector (cluster);

   for(sector = 0; sector < sectorPerCluster; sector++)
   {
     SD_readSingleBlock (firstSector + sector);

     for(i=0; i<bytesPerSector; i+=32)
     {
	    dir = (struct dir_Structure *) &buffer[i];

        if(dir->name[0] == EMPTY) //indicates end of the file list of the directory
		{
		  if(flag == DELETE) return 0;   // File does not exist
		}
		if((dir->name[0] != DELETED) && (dir->attrib != ATTR_LONG_NAME))
        {
          if((flag == GET_FILE) || (flag == DELETE))
          {

            for(j=0; j<11; j++)
            if(dir->name[j] != fileName[j]) break;
            if(j == 11)
			{
			  if(flag == GET_FILE)
              {
			    appendFileSector = firstSector + sector;
				appendFileLocation = i;
				appendStartCluster = (((unsigned long) dir->firstClusterHI) << 16) | dir->firstClusterLO;
				fileSize = dir->fileSize;
			    return (dir);
			  }	
			  else    //when flag = DELETE
			  {
			  	
				 firstCluster = (((unsigned long) dir->firstClusterHI) << 16) | dir->firstClusterLO;
                
				 //mark file as 'deleted' in FAT table
				 dir->name[0] = DELETED;    
				 SD_writeSingleBlock (firstSector+sector);
				 			 
				 freeMemoryUpdate (ADD, dir->fileSize);

				 //update next free cluster entry in FSinfo sector
				 cluster = getSetFreeCluster (NEXT_FREE, GET, 0); 
				 if(firstCluster < cluster)
				     getSetFreeCluster (NEXT_FREE, SET, firstCluster);

				 //mark all the clusters allocated to the file as 'free'
			     while(1)  
			     {
			        nextCluster = getSetNextCluster (firstCluster, GET, 0);
					getSetNextCluster (firstCluster, SET, 0);

					if(nextCluster > 0x0ffffff6) {
						return 1;
					}
					firstCluster = nextCluster;
			  	 } 
			  }
            }
          }
          /*else
		  {
			 for(j=0; j<11; j++)
		     {
			   //if(j == 8) transmitByte(' ');
			   //transmitByte (dir->name[j]);
			 }
		     //transmitString_F (PSTR("   "));
		     if((dir->attrib != 0x10) && (dir->attrib != 0x08))
			 {
			     //transmitString_F (PSTR("FILE" ));
		         //transmitString_F (PSTR("   "));
			     //displayMemory (LOW, dir->fileSize);
			 }
			 //else
			   //transmitString_F ((dir->attrib == 0x10)? PSTR("DIR") : PSTR("ROOT"));
		  }*/
       }
     }
   }

   cluster = (getSetNextCluster (cluster, GET, 0));

   if(cluster > 0x0ffffff6)
   	 return 0;
   if(cluster == 0) 
   {
   return 0;
   }
 }
return 0;
}

//***************************************************************************
//Function: if flag=READ then to read file from SD card and send contents to UART 
//if flag=VERIFY then functions will verify whether a specified file is already existing
//Arguments: flag (READ or VERIFY) and pointer to the file name
//return: 0, if normal operation or flag is READ
//	      1, if file is already existing and flag = VERIFY; or if flag=READ and file does not exist
//		  2, if file name is incompatible
//***************************************************************************
unsigned char readFile (unsigned char flag, unsigned char *fileName) {
struct dir_Structure *dir;
unsigned long cluster, byteCounter = 0, fileSize, firstSector;
unsigned int k;
unsigned char j;

//error = convertFileName (fileName); //convert fileName into FAT format
//if(error) return 2;

dir = findFiles (GET_FILE, fileName); //get the file location
if(dir == 0) 
{
  if(flag == READ) return (1);
  else return (0);
}

if(flag == VERIFY) return (1);	//specified file name is already existing

cluster = (((unsigned long) dir->firstClusterHI) << 16) | dir->firstClusterLO;

fileSize = dir->fileSize;

//TX_NEWLINE;
//TX_NEWLINE;

while(1)
{
  firstSector = getFirstSector (cluster);

  for(j=0; j<sectorPerCluster; j++)
  {
    SD_readSingleBlock(firstSector + j);
    
	for(k=0; k<512; k++)
    {
//      transmitByte(buffer[k]);
      if ((byteCounter++) >= fileSize ) return 0;
    }
  }
  cluster = getSetNextCluster (cluster, GET, 0);
  if(cluster == 0) {
  //transmitString_F(PSTR("Error in getting cluster"));
  return 0;}
}
return 0;
}

//************************************************************************************
//Function: to create a file in FAT32 format in the root directory if given 
//			file name does not exist; if the file already exists then append the data
//Arguments: pointer to the file name
//return: none
//************************************************************************************
unsigned char writeFile (unsigned char *fileName) {
unsigned char j,k, data, error, fileCreatedFlag = 0, start = 0, appendFile = 0, sector=0;
unsigned int i, firstClusterHigh=0, firstClusterLow=0;  //value 0 is assigned just to avoid warning in compilation
struct dir_Structure *dir;
unsigned long cluster, nextCluster, prevCluster, firstSector, clusterCount, extraMemory;

j = readFile (VERIFY, fileName);

if(j == 1) {
  //transmitString_F(PSTR(" File already exists, appending data..")); 
  appendFile = 1;
  cluster = appendStartCluster;
  clusterCount=0;
  while(1)
  {
    nextCluster = getSetNextCluster (cluster, GET, 0);
    if(nextCluster == EOF) break;
	cluster = nextCluster;
	clusterCount++;
  }

  sector = (fileSize - (clusterCount * sectorPerCluster * bytesPerSector)) / bytesPerSector; //last sector number of the last cluster of the file
  start = 1;
} else if(j == 2) {
   return 1; //invalid file name
} else {

  cluster = getSetFreeCluster (NEXT_FREE, GET, 0);
  if(cluster > totalClusters) cluster = rootCluster;

  cluster = searchNextFreeCluster(cluster);
   if(cluster == 0)
   {
   		LCD_XY(0,1);
		wrtr("NO FREE");
		for(;;) {}
	  return 1;
   }
  getSetNextCluster(cluster, SET, EOF);   //last cluster of the file, marked EOF
   
  firstClusterHigh = (unsigned int) ((cluster & 0xffff0000) >> 16 );
  firstClusterLow = (unsigned int) ( cluster & 0x0000ffff);
  fileSize = 0;
}

k=0;

while(1)
{
   if(start)
   {
      start = 0;
	  startBlock = getFirstSector (cluster) + sector;
	  SD_readSingleBlock (startBlock);
	  i = fileSize % bytesPerSector;
	  j = sector;
   }
   else
   {
      startBlock = getFirstSector (cluster);
	  i=0;
	  j=0;
   }
   
   
   do
   {
	 data = dataString[k++];
	 //transmitByte(data);
     buffer[i++] = data;
	 fileSize++;
     
     if(i >= 512)   //though 'i' will never become greater than 512, it's kept here to avoid 
	 {				//infinite loop in case it happens to be greater than 512 due to some data corruption
	   i=0;
	   error = SD_writeSingleBlock (startBlock);
       j++;
	   if(j == sectorPerCluster) {j = 0; break;}
	   startBlock++; 
     }
   } while((data != '\n') && (k < MAX_STRING_SIZE)); //stop when newline character is found
   													 //or when string size limit reached

   if((data == '\n') || (k >= MAX_STRING_SIZE))
   {
      for(;i<512;i++)  //fill the rest of the buffer with 0x00
        buffer[i]= 0x00;
   	  error = SD_writeSingleBlock (startBlock);

      break;
   } 
 
   prevCluster = cluster;

   cluster = searchNextFreeCluster(prevCluster); //look for a free cluster starting from the current cluster

   if(cluster == 0)
   {
      //TX_NEWLINE;
      //transmitString_F(PSTR(" No free cluster!"));
	  return 1;
   }

   getSetNextCluster(prevCluster, SET, cluster);
   getSetNextCluster(cluster, SET, EOF);   //last cluster of the file, marked EOF
}

getSetFreeCluster (NEXT_FREE, SET, cluster); //update FSinfo next free cluster entry

//error = getDateTime_FAT();    //get current date & time from the RTC
//if(error) { dateFAT = 0; timeFAT = 0;}

if(appendFile)  //executes this loop if file is to be appended
{
  SD_readSingleBlock (appendFileSector);    
  dir = (struct dir_Structure *) &buffer[appendFileLocation]; 

  dir->lastAccessDate = 0;   //date of last access ignored
  dir->writeTime = 0;//timeFAT;  //setting new time of last write, obtained from RTC
  dir->writeDate = 0b0000000000100001;//dateFAT;  //setting new date of last write, obtained from RTC
  extraMemory = fileSize - dir->fileSize;
  dir->fileSize = fileSize;
  SD_writeSingleBlock (appendFileSector);
  freeMemoryUpdate (REMOVE, extraMemory); //updating free memory count in FSinfo sector;

  
 // TX_NEWLINE;
 // transmitString_F(PSTR(" File appended!"));

  return 0;
}

//executes following portion when new file is created

prevCluster = rootCluster; //root cluster

while(1)
{
   firstSector = getFirstSector (prevCluster);

   for(sector = 0; sector < sectorPerCluster; sector++)
   {
     SD_readSingleBlock (firstSector + sector);
	

     for(i=0; i<bytesPerSector; i+=32)
     {
	    dir = (struct dir_Structure *) &buffer[i];

		if(fileCreatedFlag)   //to mark last directory entry with 0x00 (empty) mark
		 { 					  //indicating end of the directory file list
		   //dir->name[0] = EMPTY;
		   //SD_writeSingleBlock (firstSector + sector);
           return 0;
         }

        if((dir->name[0] == EMPTY) || (dir->name[0] == DELETED))  //looking for an empty slot to enter file info
		{
		  for(j=0; j<11; j++)
  			dir->name[j] = fileName[j];
		  dir->attrib = ATTR_ARCHIVE;	//settting file attribute as 'archive'
		  dir->NTreserved = 0;			//always set to 0
		  dir->timeTenth = 0;			//always set to 0
		  dir->createTime = 0; //timeFAT; 	//setting time of file creation, obtained from RTC
		  dir->createDate = 0b0000000000100001; //dateFAT; 	//setting date of file creation, obtained from RTC
		  dir->lastAccessDate = 0;   	//date of last access ignored
		  dir->writeTime = 0; //timeFAT;  	//setting new time of last write, obtained from RTC
		  dir->writeDate = 0b0000000000100001; //dateFAT;  	//setting new date of last write, obtained from RTC
		  dir->firstClusterHI = firstClusterHigh;
		  dir->firstClusterLO = firstClusterLow;
		  dir->fileSize = fileSize;

		  SD_writeSingleBlock (firstSector + sector);
		  fileCreatedFlag = 1;

		  //TX_NEWLINE;
		  //TX_NEWLINE;
		  //transmitString_F(PSTR(" File Created! "));

		  freeMemoryUpdate (REMOVE, fileSize); //updating free memory count in FSinfo sector
	     
        }
     }
   }

   cluster = getSetNextCluster (prevCluster, GET, 0);

   if(cluster > 0x0ffffff6)
   {
      if(cluster == EOF)   //this situation will come when total files in root is multiple of (32*sectorPerCluster)
	  {  
		cluster = searchNextFreeCluster(prevCluster); //find next cluster for root directory entries
		getSetNextCluster(prevCluster, SET, cluster); //link the new cluster of root to the previous cluster
		getSetNextCluster(cluster, SET, EOF);  //set the new cluster as end of the root directory
      } 

      else
      {	
	    //transmitString_F(PSTR("End of Cluster Chain")); 
	    return 1;
      }
   }
   if(cluster == 0) {
   //transmitString_F(PSTR("Error in getting cluster")); 
   return 1;}
   
   prevCluster = cluster;
 }
 
 return 0;
}

//***************************************************************************
//Function: to search for the next free cluster in the root directory
//          starting from a specified cluster
//Arguments: Starting cluster
//return: the next free cluster
//****************************************************************
unsigned long searchNextFreeCluster (unsigned long startCluster)
{
  unsigned long cluster, *value, sector;
  unsigned char i;
    
	startCluster -=  (startCluster % 128);   //to start with the first file in a FAT sector
    for(cluster =startCluster; cluster <totalClusters; cluster+=128) 
    {
      sector = unusedSectors + reservedSectorCount + ((cluster * 4) / bytesPerSector);
      SD_readSingleBlock(sector);
      for(i=0; i<128; i++)
      {
       	 value = (unsigned long *) &buffer[i*4];
         if(((*value) & 0x0fffffff) == 0)
            return(cluster+i);
      }  
    } 

 return 0;
}

uint8_t deleteFile (unsigned char *fileName) {
	return findFiles(DELETE, fileName);
}

//********************************************************************
//Function: update the free memory count in the FSinfo sector. 
//			Whenever a file is deleted or created, this function will be called
//			to ADD or REMOVE clusters occupied by the file
//Arguments: #1.flag ADD or REMOVE #2.file size in Bytes
//return: none
//********************************************************************
void freeMemoryUpdate (unsigned char flag, unsigned long size)
{
  unsigned long freeClusters;
  //convert file size into number of clusters occupied
  if((size % 512) == 0) size = size / 512;
  else size = (size / 512) +1;
  if((size % 8) == 0) size = size / 8;
  else size = (size / 8) +1;

  if(freeClusterCountUpdated)
  {
	freeClusters = getSetFreeCluster (TOTAL_FREE, GET, 0);
	if(flag == ADD)
  	   freeClusters = freeClusters + size;
	else  //when flag = REMOVE
	   freeClusters = freeClusters - size;
	getSetFreeCluster (TOTAL_FREE, SET, freeClusters);
  }
}

//***************************************************************************
//Function: get cluster entry value from FAT to find out the next cluster in the chain
//or set new cluster entry in FAT
//Arguments: 1. current cluster number, 2. get_set (=GET, if next cluster is to be found or = SET,
//if next cluster is to be set 3. next cluster number, if argument#2 = SET, else 0
//return: next cluster number, if if argument#2 = GET, else 0
//****************************************************************************
unsigned long getSetNextCluster (unsigned long clusterNumber,
                                 unsigned char get_set,
                                 unsigned long clusterEntry)
{
unsigned int FATEntryOffset;
unsigned long *FATEntryValue;
unsigned long FATEntrySector;
unsigned char retry = 0;

//get sector number of the cluster entry in the FAT
FATEntrySector = unusedSectors + reservedSectorCount + ((clusterNumber * 4) / bytesPerSector) ;

//get the offset address in that sector number
FATEntryOffset = (unsigned int) ((clusterNumber * 4) % bytesPerSector);

//read the sector into a buffer
while(retry <10)
{ if(!SD_readSingleBlock(FATEntrySector)) break; retry++;}

//get the cluster address from the buffer
FATEntryValue = (unsigned long *) &buffer[FATEntryOffset];

if(get_set == GET)
  return ((*FATEntryValue) & 0x0fffffff);


*FATEntryValue = clusterEntry;   //for setting new value in cluster entry in FAT

SD_writeSingleBlock(FATEntrySector);

return (0);
}

uint8_t getfilebyte() {
	if (ifi == 512) {
		ifi = 0;
		jfi++;
		if (jfi == 8) {
			jfi = 0;
			cluster = getSetNextCluster(cluster, GET, 0);
		}
		dfirstSector = getFirstSector(cluster);
		SD_readSingleBlock(dfirstSector + jfi);
	}
	return buffer[ifi++];
}
