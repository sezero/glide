#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifndef __linux__
#include <conio.h>
#endif
#include <3dfx.h>
#include <fxpci.h>
#define FX_DLL_DEFINITION
#include <fxdll.h>

#define SIZE_SST1_NEEDED   0x100000
#define END_ADDRESS        0x10000000
#define S3_SHIFT           0x400000

struct RangeSTRUCT
{
   FxU32                address;
   FxU32                range;
   FxU32                id;
   FxU32                is_voodoo; /* 1 = is voodoo, 2 = hidden SLI */
   FxU32                is_S3;
   struct RangeSTRUCT   *next;
   struct RangeSTRUCT   *prev;
};

static int silent = 1;

typedef struct RangeSTRUCT RangeStruct;

static void InitRemap(void);
static void CloseRemap(void);
static void GetMemoryMap(void);
static void RemapVoodoo(RangeStruct *conflict);
static void AdjustMapForS3(void);
static RangeStruct *TestForConflicts(void);
static void RemoveEntry(RangeStruct *del);
static void InsertEntry(RangeStruct *ins);
static FxBool FindHole(RangeStruct *conflict);
static FxU32 SnapToDecentAddress(FxU32 address,RangeStruct *conflict);
static FxBool fits_in_hole(RangeStruct *begin,FxU32 end,RangeStruct *hole,RangeStruct *conflict);
static FxBool fits_under(RangeStruct *first,FxU32 minimum,RangeStruct *hole,RangeStruct *conflict);
static FxU32 pciGetType(long i);
static void pciGetRange(PciRegister reg,FxU32 device_number,FxU32 *data);
static FxBool pciGetAddress(PciRegister reg,FxU32 device_number,FxU32 *data);

static void ForceCleanUp(void);
static FxBool FindNecessaryCards(void);
static void ProcessCommandLine(char **argv,int argc);
static FxBool IsCardVoodoo(long i);
static FxBool IsCardS3(long i);
static FxBool ReadHex(char *string,FxU32 *num);
static void AddMapEntry(FxU32 address,FxU32 range,FxU32 id,FxBool VoodooCard,FxBool S3Card);
#if 0 /* not used */
static void HandleMemoryOverlap(void);
static FxBool overlap_map(RangeStruct *begin,FxU32 end);
#endif

static FxBool switch_S3_flag_ignore=FXFALSE;
//static FxBool switch_force=FXFALSE;
static FxBool switch_C0_bias=FXTRUE;
static int switch_voodoo_loc = 0;
static FxU32 num_voodoos=0;

//#define TESTING 1

#ifdef TESTING
static RangeStruct test_data[6]=
                           {{0xF0000000,0x100000,1,0,0,0,0},
                            {0xF3000000,0x200000,4,1,0,0,0},
                            {0xF3000000,0x200000,8,0,0,0,0},
                            {0xF5000000,0x200000,2,0,0,0,0},
                            {0xE6000000,0x200000,5,0,0,0,0},
                            {0xD3001000,0x200000,3,0,0,0,0}};
#endif

static RangeStruct map[80];
static RangeStruct hole[80];
static RangeStruct *first_entry;
static RangeStruct *last_entry;
static long entries=0;
static RangeStruct master_hole;
static FxU32 conflicts_found=0;

static void fxremap_dowork(int argc,char **argv,int doit_silently)
{
   RangeStruct *conflict;

   silent = doit_silently;

   ProcessCommandLine(argv,argc);

   InitRemap();

   if (!FindNecessaryCards())
   {
      if (!silent) {
       printf("This program was only meant to be used with the 3dfx Voodoo chipset\n");
       printf("to correct possible pci address conflicts.\n");
       printf("No Voodoo chipset was detected\n");
      }
      ForceCleanUp();
   }

   GetMemoryMap();

   /* expand region of mapping for S3 card */
   AdjustMapForS3();
   /* see if we find any conflicts with any voodoo card */
   while ((conflict=TestForConflicts()))
   {
      conflicts_found++;
      /* since it is going to move */
      /* remove entry, so we can possibly use it as a hole */
      RemoveEntry(conflict);
      if (FindHole(conflict))
      {
         conflict->address=master_hole.address;
         RemapVoodoo(conflict);
      }
      else
      {
         if (!silent) {
            printf("Unable to find region to map conflicting board\n");
         }
         ForceCleanUp();
         return;
      }
   }

   if (!conflicts_found) {
      if (!silent) {
        printf("No conflict with the Voodoo cards was found\n");
      }
   }
   CloseRemap();
}

void fxremap(void) {
  fxremap_dowork(0,NULL,1);
}

void fxremap_main(int argc,char **argv) {
  fxremap_dowork(argc,argv,0);
}


static void InitRemap(void)
{
#if !DIRECTX
   pciOpen();
#endif
}

static void CloseRemap(void)
{
   // pciClose();
}

FxU32 pciGetConfigData_R(PciRegister reg, FxU32 devNum) {
   FxU32 data;

   if (pciGetConfigData(reg,devNum,&data) == FXTRUE) {
      return (data);
   } else {
      return (0xFFFFFFFF);
   }
}

#define PCI_NORMAL_TYPE 0
#define PCI_BRIDGE_TYPE 1

static void GetMemoryMap(void)
{
   FxU32    temp,temp2;
   FxU32    type;
   long     devNum;
   int fn;     /* function number iterator */
   int maxFnNumber;
   int multi_fn = 0;

#ifdef TESTING
   for (i=0;i<6;i++)
   {
      temp=test_data[i].address;
      temp2=~(test_data[i].range - 0x1);
      AddMapEntry(temp,temp2,test_data[i].id,test_data[i].is_voodoo,test_data[i].is_S3);
   }
#else   
   for (devNum=0;devNum<MAX_PCI_DEVICES;devNum++)
   {
      if (pciDeviceExists(devNum))
      {

         if (pciGetConfigData_R(PCI_HEADER_TYPE,devNum) & (1<<7)) {
             maxFnNumber = 8; /* multifunction! */
             multi_fn = 1;
         } else {
             multi_fn = 0;
             if ((pciGetConfigData_R(PCI_VENDOR_ID,devNum) == 0x121a) &&
                 (pciGetConfigData_R(PCI_DEVICE_ID,devNum) == 0x02)) {
                 maxFnNumber = 8; /* single board SLI! */
             } else {
                 maxFnNumber = 1;
             }
         }

         for(fn=0;fn<maxFnNumber;fn++) {

           int i = devNum | (fn << 13); /* add function number */
           if (pciGetConfigData_R(PCI_VENDOR_ID,i) != 0xFFFF) {

             /* two header types */
             /* one for bridges and one for everything else */
             type=pciGetType(i);
             if (type==PCI_NORMAL_TYPE) {
               if (pciGetAddress(PCI_BASE_ADDRESS_0,i,&temp)) {
                 pciGetRange(PCI_BASE_ADDRESS_0,i,&temp2);
                 AddMapEntry(temp,temp2,i,IsCardVoodoo(i),IsCardS3(i));
               }
               if (pciGetAddress(PCI_BASE_ADDRESS_1,i,&temp)) {
                 pciGetRange(PCI_BASE_ADDRESS_1,i,&temp2);
                 AddMapEntry(temp,temp2,i,IsCardVoodoo(i),IsCardS3(i));
               }

#if 0
               /* Legacy address which is not needed for sst1 type
                * things w/o 2d.
                */
               if (pciGetAddress(PCI_IO_BASE_ADDRESS,i,&temp)) {
                 pciGetRange(PCI_IO_BASE_ADDRESS,i,&temp2);
                 AddMapEntry(temp,temp2,i,IsCardVoodoo(i),IsCardS3(i));
               }
#endif
               if (pciGetAddress(PCI_ROM_BASE_ADDRESS,i,&temp)) {
                 pciGetRange(PCI_ROM_BASE_ADDRESS,i,&temp2);
                 AddMapEntry(temp,temp2,i,IsCardVoodoo(i),IsCardS3(i));
               }
             } else if (type==PCI_BRIDGE_TYPE) {
               if (pciGetAddress(PCI_BASE_ADDRESS_0,i,&temp)) {
                 pciGetRange(PCI_BASE_ADDRESS_0,i,&temp2);
                 AddMapEntry(temp,temp2,i,IsCardVoodoo(i),IsCardS3(i));
               }
               if (pciGetAddress(PCI_BASE_ADDRESS_1,i,&temp)) {
                 pciGetRange(PCI_BASE_ADDRESS_1,i,&temp2);
                 AddMapEntry(temp,temp2,i,IsCardVoodoo(i),IsCardS3(i));
               }
             }
           } /* if function number exists */
         } /* for all function numbers */
      }
   }
#endif
}

static void AdjustMapForS3(void)
{
   RangeStruct *cur;

   cur=first_entry;

   while(cur)
   {
      if (cur->is_S3)
      {
         cur->address-=S3_SHIFT;
         cur->range=S3_SHIFT<<1;
      }
      cur=cur->next;
   }
}

static RangeStruct *TestForConflicts(void)
{
   RangeStruct *cur,*next;

   cur=first_entry;

   while(cur)
   {
      /* if this is a poorly mapped voodoo2 single board SLI, then remap */
      if ((cur->is_voodoo == 2) && (cur->address == 0xFF00000)) {
        return (cur);
      }

      if (cur->next)
      {
         if ((cur->address + cur->range) > cur->next->address)
         {
            next=cur->next;
            if ((cur->is_voodoo)||(next->is_voodoo))
            {
               if (cur->is_voodoo)
               {
                  return cur;
               }
               return next;
            }
            else {
               if (!silent) {
                 printf("FxRemap: Possible PCI conflict not with Voodoo device\n");
                 printf("%X (%X) <-> %X:%X (%X)\n",cur->id, cur->address,  
                       cur->next->id, cur->next->address);
               }
            }
         }
      }
      else
      {
         if ((cur->address + cur->range) > END_ADDRESS)
            return cur;
      }
      cur=cur->next;
   }
   return NULL;
}

static void AddMapEntry(FxU32 address,FxU32 range,FxU32 id,FxBool VoodooCard,FxBool S3Card)
{
   RangeStruct *temp,*cur,*next;

//jcochrane@3dfx.com
   long        entry=0;
   FxU32	   tmp_address=0;
//END

#if 0
   static long    test_entry=0;

   address=test_data[test_entry].address;
   range=~(test_data[test_entry++].range - 0x1);
#endif
   /* only if address != 0 */

//jcochrane@3dfx.com
//check for duplicate entries in the map table,ignore if there is

	tmp_address=address>>4;
	for(entry=0;entry<entries;entry++)
	{
		if( tmp_address == map[entry].address)
			address=0;
	}
//END

   if(address)
   {
      map[entries].address=address>>4;
      map[entries].range=((~range)>>4)+0x1;

      map[entries].id=id;
      map[entries].is_voodoo=VoodooCard;
      map[entries].is_S3=S3Card;

      temp=&map[entries++];
      if (entries<=1)
      {
         first_entry=temp;
         last_entry=temp;
         temp->next=NULL;
         temp->prev=NULL;
         return;
      }

      cur=first_entry;
      next=NULL;
      while(cur)
      {
         if (temp->address < cur->address)
         {
            next=cur;
            break;
         }
         cur=cur->next;
      }
      if (next)
      {
         temp->next=next;
         temp->prev=next->prev;
         next->prev=temp;
         if (next==first_entry)
            first_entry=temp;
         else
            (temp->prev)->next=temp;
      }
      else
      {
         last_entry->next=temp;
         temp->prev=last_entry;
         last_entry=temp;
         temp->next=NULL;
      }
   }
}

static void RemoveEntry(RangeStruct *del)
{
   RangeStruct *prev;

   if (!(del->next))
   {
      if (!(del->prev))
      {
         if (!silent) {
           printf("FxRemap: No entries mapped\n");
         }
         ForceCleanUp();
         return;
      }
      prev=del->prev;
      last_entry=prev;

      prev->next=NULL;
      del->prev=NULL;
      del->next=NULL;
   }
   else
   {
      if (!(del->prev))
      {
         del->next->prev=NULL;
         first_entry=del->next;
      }
      else
      {
         del->next->prev=del->prev;
         del->prev->next=del->next;
      }
      del->next=NULL;
      del->prev=NULL;
   }
}

static void InsertEntry(RangeStruct *ins)
{
   RangeStruct *cur;

   cur=first_entry;
   
   ins->next=NULL;
   ins->prev=NULL;

   if (!first_entry)
   {
      first_entry=ins;
      last_entry=ins;
      return;
   }
   while(cur)
   {
      if (ins->address < cur->address)
      {
         ins->next=cur;
         ins->prev=cur->prev;
         cur->prev=ins;
         
         if (!ins->prev)
         {
            first_entry=ins;
         }
         else
         {
            (ins->prev)->next=ins;
         }
         return;
      }
      cur=cur->next;
   }

   /* if it got this far it needs to go at the end */
   ins->prev=last_entry;
   last_entry->next=ins;
   last_entry=ins;
}

static FxU32 SnapToDecentAddress(FxU32 address,RangeStruct *conflict)
{
   FxU32 range;
   FxU32 mask;
   FxU32 not_mask;

   range=conflict->range;
   if (range<0x10000)
      range=0x10000;
   mask=range;
   mask-=1;
   not_mask=~mask;

   if (address & mask)
   {
      address=(address & not_mask) + range;
   }
   return address;
}

static FxBool fits_in_hole(RangeStruct *begin,FxU32 end,RangeStruct *hole,RangeStruct *conflict)
{
   FxU32 address;

   address=begin->address+begin->range;

   address=SnapToDecentAddress(address,conflict);

   /* note could be <= */
   /* this is safer but more inefficient memory wise */
   if ((address+conflict->range)<end)
   {
      hole->address=address;
      hole->range=end-address;
      return FXTRUE;
   }
   return FXFALSE;
}

static FxBool fits_under(RangeStruct *first,FxU32 minimum,RangeStruct *hole,RangeStruct *conflict)
{
   FxU32 address;

   address=minimum;
   address=SnapToDecentAddress(address,conflict);

   if ((address+conflict->range) < first->address)
   {
      hole->address=address;
      hole->range=first->address - address;
      return FXTRUE;
   }
   return FXFALSE;
}


static FxBool FindHole(RangeStruct *conflict)
{
   RangeStruct *cur;

   cur=first_entry;

   while(cur)
   {
      if (!(cur->next))
      {
         if (fits_in_hole(cur,END_ADDRESS,&master_hole,conflict))
         {
            return FXTRUE;
         }
      }
      else
      {
         if (fits_in_hole(cur,cur->next->address,&master_hole,conflict))
         {
            return FXTRUE;
         }
      }
      cur=cur->next;
   }

   /* see if we can find a whole located below addressed boards */
   /* don't want to go below 0xA000000 for addressing our boards */
   if (first_entry->address > 0xA000000)
   {
      if (fits_under(first_entry,0xA000000,&master_hole,conflict))
         return FXTRUE;
   }
   return FXFALSE;
}

static void RemapVoodoo(RangeStruct *conflict)
{
   FxU32    address;

   /* put conflict back into memory map */
   InsertEntry(conflict);

#ifndef TESTING
   address=(conflict->address)<<4;
   pciSetConfigData(PCI_BASE_ADDRESS_0,conflict->id,&address);
#endif
   if (!silent) {
     printf("Remapped Voodoo Board to avoid a conflict\n");
   }
}

static void pciGetRange(PciRegister reg,FxU32 device_number,FxU32 *data)
{
   FxU32    temp=0xFFFFFFFF;
   FxU32    size,save;
   
   pciGetConfigData(reg,device_number,&save);
   pciSetConfigData(reg,device_number,&temp);
   pciGetConfigData(reg,device_number,&size);
   pciSetConfigData(reg,device_number,&save);

#ifdef TESTING
   printf("PciGetRange: save %08x \n",save);
   printf("PciGetRange: temp %08x \n",temp);
   printf("PciGetRange: size %08x \n",size);
   printf("PciGetRange: save %08x \n",save);
#endif
   *data=size;
}

static FxBool pciGetAddress(PciRegister reg,FxU32 device_number,FxU32 *data)
{
   pciGetConfigData(reg,device_number,data);
   if ((*data)==0)
      return FXFALSE;
   if (*data & 0x01)
      return FXFALSE;
   return FXTRUE;
}

static void ForceCleanUp(void)
{
   // pciClose();
   // exit(1);
}

static FxBool FindNecessaryCards(void)
{
   FxBool voodoo_found=FXFALSE;
   long   i;

   for (i=0;i<MAX_PCI_DEVICES;i++)
   {
      if (pciDeviceExists(i))
      {
         if(IsCardVoodoo(i))
         {
            voodoo_found=FXTRUE;
            num_voodoos++;
         }
      }
   }
   if (!voodoo_found)
   {
      if (!silent) {
        printf("Warning no known voodoo card was found\n");
      }
      return FXFALSE;
   }
   return FXTRUE;
}

static void ProcessCommandLine(char **argv,int argc)
{
   long     i;
   FxU32    temp,temp2;
   FxU32    address,range;
   char     *hex_ptr;
   
   for (i=1;i<argc;i++)
   {
      if (strcmp(argv[i],"/dS3")==0)
      {
         switch_S3_flag_ignore=FXTRUE;
      }
      else if(strcmp(argv[i],"/f")==0)
      {
         if ((i+1)<argc)
         {
            if (ReadHex(argv[i+1],&temp))
            {
               i++;
            }
            else
            {
               if (!silent) {
                printf("Command line: improper format\n");
                printf("ex: fxremap.exe /f 0xC0000000\n");
               }
               ForceCleanUp();
            }
         }
         else
         {
            if (!silent) {
              printf("Command line: improper format\n");
              printf("ex: fxremap.exe /f 0xC0000000\n");

            }
            ForceCleanUp();
         }
         if (!silent) {
           printf("Command line option /f ignored in this version\n");

         }
/* this stuff was from the interactive test version */
#if 0
         while(!kbhit())
         {
            ;
         }
         getch();
#endif
      }
      else if(strcmp(argv[i],"/x")==0)
      {
         if ((i+1)<argc)
         {
            hex_ptr=strchr(argv[i+1],'-');
            if (!hex_ptr)
            {
               if (!silent) {
                 printf("Command line: improper format\n");
                 printf("ex: fxremap.exe /x 0xE0000000-0xF0000000\n");
               }
               ForceCleanUp();
            }
            if ((ReadHex(argv[i+1],&temp))&&(ReadHex(hex_ptr+1,&temp2)))
            {
               address=temp;
               range=temp2-temp;
               range=~(range - 0x1);
               i++;
               AddMapEntry(address,range,0x500,FXFALSE,FXFALSE);
            }
            else
            {
               if (!silent) {
                 printf("Command line: improper format\n");
                 printf("ex: fxremap.exe /x 0xE0000000-0xF0000000\n");
               }
               ForceCleanUp();
            }
         }
         else
         {
            if (!silent) {
              printf("Command line: improper format\n");
              printf("ex: fxremap.exe /x 0xE0000000-0xF0000000\n");
            }
            ForceCleanUp();
         }
      }
      else if (strcmp(argv[i],"/nb")==0)
      {
         switch_C0_bias=FXFALSE;
      }
      else if (strcmp(argv[i],"/i")==0)
      {
         switch_voodoo_loc = atoi(argv[++i]);
      }
      else
      {
         if (!silent) {
           printf("Command line: improper options specified\n");
           printf("Valid options are /dS3 /f /x /i\n");
         }
      }
   }
}

static FxU32 pciGetType(long i)
{
   FxU32 header_type;

   pciGetConfigData(PCI_HEADER_TYPE,i,&header_type);

   return header_type;
}

static FxBool IsCardVoodoo(long i)
{
   FxU32    vendor,dev_id;
   FxU32    fn_num = (i >> 13) & 0x7; 
   int      true_val;

   if (fn_num) {
     true_val = 2;
   } else {
     true_val = 1;
   }
   
   pciGetConfigData(PCI_VENDOR_ID,i,&vendor);
   pciGetConfigData(PCI_DEVICE_ID,i,&dev_id);
   /* if sst1 */
   if ((vendor==0x121a)&&(dev_id==0x0001))
      return FXTRUE;
   /* if voodoo2 */
   if ((vendor==0x121a)&&(dev_id==0x0002)) {
      if (true_val == 2) if (!silent) { printf("found voodoo2 hidden sli\n"); }
      return true_val;
   }
   /* if banshee */
   if ((vendor==0x121a)&&(dev_id==0x0003))
      return FXTRUE;
   /* if h4? or whatever is next */
   if ((vendor==0x121a)&&(dev_id==0x0004))
      return FXTRUE;
   return FXFALSE;
}

static FxBool IsCardS3(long i)
{
   FxU32    vendor,dev_id;
   
   pciGetConfigData(PCI_VENDOR_ID,i,&vendor);
   pciGetConfigData(PCI_DEVICE_ID,i,&dev_id);
   if ((vendor==0x5333)&&((dev_id==0x88f0)||(dev_id==0x8880)))
      return FXTRUE;

   return FXFALSE;
}

static FxBool ReadHex(char *string,FxU32 *num)
{
   long  i=0;
   FxU32 temp=0,temp2;
   long  num_count=0;

   /* bypass leading spaces */
   while((string[i])&&(string[i]==' '))
      i++;
   /* verify leading 0x */
   if (string[i]=='0')
      i++;
   else
      return FXFALSE;
   if (string[i]=='x')
      i++;
   else
      return FXFALSE;

   /* read in number */
   while(((string[i]>=0x30)&&(string[i]<0x3A))||((string[i]>=0x41)&&(string[i]<0x47))||((string[i]>=0x61)&&(string[i]<0x67)))
   {
      if ((string[i]>=0x30)&&(string[i]<0x3A))
         temp2=string[i] - 0x30;
      else if ((string[i]>=0x41)&&(string[i]<0x47))
         temp2=string[i] - 0x37;
      else if ((string[i]>=0x61)&&(string[i]<0x67))
         temp2=string[i] - 0x57;
      if (num_count!=0)
         temp=(temp<<4)+temp2;
      else if (num_count<8)
         temp=temp2;
      else
         return FXFALSE;
      num_count++;i++;
   }
   *num=temp;
   return FXTRUE;
}

#if 0 /* not used */
static void HandleMemoryOverlap(void)
{
   RangeStruct *cur;

   cur=first_entry;
   while(cur)
   {
      if (cur!=last_entry)
      {
         if (overlap_map(cur,cur->next->address))
         {
            if (cur->range<(cur->next->address+cur->next->range-cur->address))
               cur->range=cur->next->address+cur->next->range-cur->address;
            if (cur->next==last_entry)
            {
               last_entry=cur;
               cur->next=NULL;
            }
            else
            {
               cur->next=cur->next->next;
               cur->next->prev=cur;
            }
         }
         else
            cur=cur->next;
      }
      else
         cur=cur->next;
   }
}

static FxBool overlap_map(RangeStruct *begin,FxU32 end)
{
   if ((begin->address+begin->range)>end)
      return FXTRUE;
   return FXFALSE;
}
#endif
